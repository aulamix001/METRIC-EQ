#pragma once
#include "ReferenceLibrary.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
class MetricProcessor final : public juce::AudioProcessor {
  public:
    MetricProcessor();
    using juce::AudioProcessor::processBlock;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override {
        return true;
    }
    const juce::String getName() const override {
        return "CHi Metric Match Pro";
    }
    bool acceptsMidi() const override {
        return false;
    }
    bool producesMidi() const override {
        return false;
    }
    double getTailLengthSeconds() const override {
        return 0;
    }
    int getNumPrograms() override {
        return 1;
    }
    int getCurrentProgram() override {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {
        return {};
    }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    juce::AudioProcessorParameter *getBypassParameter() const override {
        return state.getParameter("bypass");
    }
    float value(const char *id) const {
        for (const auto &entry : parameterCache)
            if (entry.first == id)
                return entry.second->load(std::memory_order_relaxed);
        return 0;
    }
    void set(const char *id, float actual) {
        auto *p = state.getParameter(id);
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(actual));
        p->endChangeGesture();
    }
    int selected() const {
        return juce::jlimit(0, 15, juce::roundToInt(value("reference")));
    }
    void choose(int index, bool keepA);
    void cue(int index, bool remove = false);
    std::array<std::array<std::atomic<double>, 4>, 16> cues{};
    std::atomic<double> seekSeconds{-1}, position{0};
    std::atomic<float> peakA[2]{}, peakB[2]{}, rmsA[2]{}, rmsB[2]{}, correlationA{0},
        correlationB{0};
    static constexpr int fifoCapacity = 32768;
    juce::AbstractFifo analysisFifo{fifoCapacity};
    std::array<std::vector<float>, 4> analysis;
    std::atomic<double> currentRate{44100};
    ReferenceLibrary library;
    juce::AudioProcessorValueTreeState state;

  private:
    static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
    struct Biquad {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float tick(float x) {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            if (!std::isfinite(y)) {
                z1 = z2 = 0;
                return 0;
            }
            return static_cast<float>(juce::jlimit(-1.0e12, 1.0e12, y));
        }
    };
    struct FilterBank {
        std::array<std::array<Biquad, 8>, 4> filters{};
        int count = 0;
        void configure(double rate, double low, double high, int mode, int slope);
        void tick(std::array<float, 4> &x) {
            for (size_t c = 0; c < 4; ++c)
                for (int j = 0; j < count; ++j)
                    x[c] = filters[c][static_cast<size_t>(j)].tick(x[c]);
        }
    } filters, oldFilters;
    std::array<float, 5> lastFilter{{-1, -1, -1, -1, -1}};
    int filterFadeRemaining = 0, filterFadeLength = 1;
    juce::SmoothedValue<float> gainA, gainB, abFade, bypassFade;
    const ReferenceLibrary::Track *lastTrack = nullptr;
    double playFrame = 0;
    int lastCue = -1;
    std::vector<std::pair<std::string, std::atomic<float> *>> parameterCache;
    std::array<float, 2> lastReference{}, referenceOrigin{};
    int referenceFade = 0;
    bool referenceWasPlaying = false, wasB = false;
    std::atomic<float> *bypassParam = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetricProcessor)
};
