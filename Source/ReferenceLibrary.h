#pragma once
#include <array>
#include <atomic>
#include <deque>
#include <juce_audio_formats/juce_audio_formats.h>
#include <memory>
#include <mutex>
#include <vector>
class ReferenceLibrary final : private juce::Thread {
  public:
    struct Track {
        juce::AudioBuffer<float> audio;
        juce::String path, name;
        double rate = 44100;
        std::array<float, 1024> waveform{};
        double duration() const {
            return audio.getNumSamples() / rate;
        }
    };
    ReferenceLibrary() : Thread("CHi file decoder") {
        startThread();
    }
    ~ReferenceLibrary() override {
        signalThreadShouldExit();
        wake.signal();
        stopThread(-1);
    }
    void load(int slot, const juce::File &file) {
        if (slot < 0 || slot >= 16)
            return;
        {
            const std::lock_guard<std::mutex> lock(mutex);
            requests.push_back({slot, file, ++revisions[static_cast<size_t>(slot)]});
        }
        wake.signal();
    }
    const Track *get(int slot) const {
        return slots[static_cast<size_t>(juce::jlimit(0, 15, slot))].load(
            std::memory_order_acquire);
    }
    bool busy() const {
        return loading.load();
    }
    juce::String status() const {
        const std::lock_guard<std::mutex> lock(mutex);
        return message;
    }
    void clear(int slot) {
        const std::lock_guard<std::mutex> lock(mutex);
        const auto i = static_cast<size_t>(juce::jlimit(0, 15, slot));
        ++revisions[i];
        slots[i].store(nullptr, std::memory_order_release);
    }

  private:
    void run() override {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        while (!threadShouldExit()) {
            Request request;
            {
                const std::lock_guard<std::mutex> lock(mutex);
                if (!requests.empty()) {
                    request = requests.front();
                    requests.pop_front();
                } else
                    request.slot = -1;
            }
            if (request.slot < 0) {
                wake.wait(100);
                continue;
            }
            loading.store(true);
            juce::String result;
            try {
                std::unique_ptr<juce::AudioFormatReader> reader(
                    formats.createReaderFor(request.file));
                const auto bytes =
                    reader ? static_cast<double>(reader->lengthInSamples) * 2.0 * sizeof(float)
                           : 0.0;
                if (!reader)
                    result = "Cannot decode: " + request.file.getFileName();
                else if (reader->numChannels > 2 || reader->lengthInSamples <= 0 ||
                         reader->sampleRate <= 0 || bytes + retainedBytes > 256.0 * 1024 * 1024)
                    result = "Unsupported channels/length or 256 MB RAM budget reached";
                else {
                    auto track = std::make_unique<Track>();
                    track->audio.setSize(2, static_cast<int>(reader->lengthInSamples));
                    bool ok = true;
                    for (int pos = 0; pos < track->audio.getNumSamples() && !threadShouldExit();
                         pos += 16384)
                        ok = reader->read(&track->audio, pos,
                                          juce::jmin(16384, track->audio.getNumSamples() - pos),
                                          pos, true, true) &&
                             ok;
                    if (reader->numChannels == 1)
                        track->audio.copyFrom(1, 0, track->audio, 0, 0,
                                              track->audio.getNumSamples());
                    if (ok && !threadShouldExit()) {
                        track->rate = reader->sampleRate;
                        track->name = request.file.getFileNameWithoutExtension();
                        track->path = request.file.getFullPathName();
                        for (int i = 0; i < 1024; ++i) {
                            const int begin = static_cast<int>(static_cast<int64_t>(i) *
                                                               track->audio.getNumSamples() / 1024);
                            const int end = static_cast<int>(static_cast<int64_t>(i + 1) *
                                                             track->audio.getNumSamples() / 1024);
                            float peak = 0;
                            for (int n = begin; n < end; ++n)
                                for (int c = 0; c < 2; ++c) {
                                    float &sample = track->audio.getWritePointer(c)[n];
                                    if (!std::isfinite(sample))
                                        sample = 0;
                                    peak = juce::jmax(peak, std::abs(sample));
                                }
                            track->waveform[static_cast<size_t>(i)] = peak;
                        }
                        const auto *pointer = track.get();
                        {
                            const std::lock_guard<std::mutex> lock(mutex);
                            if (revisions[static_cast<size_t>(request.slot)] == request.revision) {
                                retained.push_back(std::move(track));
                                retainedBytes += bytes;
                                slots[static_cast<size_t>(request.slot)].store(
                                    pointer, std::memory_order_release);
                                result = "Loaded " + pointer->name;
                            } else
                                result = "Reference load superseded";
                        }
                    } else
                        result = "Reference decoding interrupted or failed";
                }
            } catch (const std::exception &e) {
                result = "Cannot load reference: " + juce::String(e.what());
            }
            {
                const std::lock_guard<std::mutex> lock(mutex);
                message = result;
            }
            loading.store(false);
        }
    }
    // Published tracks are immutable and retained until shutdown: audio never frees a buffer.
    std::array<std::atomic<const Track *>, 16> slots{};
    std::vector<std::unique_ptr<Track>> retained;
    double retainedBytes = 0;
    mutable std::mutex mutex;
    struct Request {
        int slot = -1;
        juce::File file;
        uint64_t revision = 0;
    };
    std::deque<Request> requests;
    std::array<uint64_t, 16> revisions{};
    juce::String message = "Drop WAV, AIFF or FLAC references into the library";
    std::atomic<bool> loading{false};
    juce::WaitableEvent wake;
};
