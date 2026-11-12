#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}

struct ParameterEvents final : juce::AudioProcessorParameter::Listener {
    explicit ParameterEvents(juce::AudioProcessorParameter &p) : parameter(p) {
        parameter.addListener(this);
    }
    ~ParameterEvents() override {
        parameter.removeListener(this);
    }
    void parameterValueChanged(int, float) override {
        ++values;
    }
    void parameterGestureChanged(int, bool starting) override {
        if (starting)
            ++begins;
        else
            ++ends;
    }
    juce::AudioProcessorParameter &parameter;
    int values = 0, begins = 0, ends = 0;
};
juce::Component *findControl(juce::Component &root, const juce::String &id) {
    if (root.getComponentID() == id)
        return &root;
    for (auto *child : root.getChildren())
        if (auto *result = findControl(*child, id))
            return result;
    return nullptr;
}
void testMapping() {
    MetricProcessor processor;
    require(processor.getParameters().size() == 37,
            "Parameter inventory changed; add tests for new parameters");
    auto *parameter = processor.state.getParameter("bypass");
    require(parameter != nullptr && parameter == processor.getBypassParameter(),
            "Host bypass parameter mismatch");
    require(parameter->paramID == "bypass" && parameter->getName(64) == "Bypass",
            "Bypass identity changed");
    require(parameter->isAutomatable() && parameter->isBoolean() && parameter->getNumSteps() == 2,
            "Invalid bypass automation metadata");
    require(parameter->getDefaultValue() == 0.0f, "Bypass default must be off");
    ParameterEvents events(*parameter);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    auto *button = dynamic_cast<juce::ToggleButton *>(findControl(*editor, "bypass"));
    require(button != nullptr, "Bypass GUI control missing");
    require(editor->getControlParameterIndex(*button) == parameter->getParameterIndex(),
            "GUI-to-host mapping failed");
    juce::Component unrelated;
    require(editor->getControlParameterIndex(unrelated) == -1,
            "Unrelated GUI control mapped to audio parameter");
    button->setToggleState(true, juce::sendNotificationSync);
    require(parameter->getValue() == 1.0f &&
                processor.state.getRawParameterValue("bypass")->load() == 1.0f,
            "GUI did not update host parameter");
    require(events.values > 0 && events.begins == 1 && events.ends == 1,
            "GUI did not send complete host automation gesture");
    parameter->setValueNotifyingHost(0.0f);
    require(!button->getToggleState(), "Host automation did not update GUI");
    juce::MemoryBlock saved;
    parameter->setValueNotifyingHost(1.0f);
    processor.getStateInformation(saved);
    parameter->setValueNotifyingHost(0.0f);
    processor.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    require(parameter->getValue() == 1.0f && button->getToggleState(),
            "State restore did not update GUI and parameter");
    const float trackValues[]{1, 8, 15, 20, 25, 35, 40, 48, 54, 58, 64, 70, 77, 86, 92, 100};
    for (int i = 0; i < 16; ++i)
        require(juce::roundToInt(processor.state.getParameter("reference")
                                     ->convertFrom0to1(trackValues[i] * .01f)) == i,
                "Reference mapping percentage wrong");
    const float cueValues[]{1, 30, 60, 100};
    for (int i = 0; i < 4; ++i)
        require(juce::roundToInt(
                    processor.state.getParameter("cue")->convertFrom0to1(cueValues[i] * .01f)) == i,
                "Cue mapping percentage wrong");
    const float stereoValues[]{1, 30, 50, 80, 100};
    for (int i = 0; i < 5; ++i)
        require(juce::roundToInt(processor.state.getParameter("stereo")->convertFrom0to1(
                    stereoValues[i] * .01f)) == i,
                "Stereo mapping percentage wrong");
    for (const auto *id : {"gainA", "gainB", "ab", "playing", "loop", "reference", "stereo", "solo",
                           "filters", "high", "low", "slope", "filterMode", "fft", "tab",
                           "viewSpectrum", "viewCorrelation", "viewStereo", "viewDynamics"})
        require(processor.state.getParameter(id) != nullptr, "Required parameter missing");
    for (const auto *id : {"gainA", "gainB", "ab", "high", "low", "playing", "loop"})
        if (auto *control = findControl(*editor, id))
            require(editor->getControlParameterIndex(*control) ==
                        processor.state.getParameter(id)->getParameterIndex(),
                    "GUI parameter index mismatch");
    auto *display = dynamic_cast<juce::Button *>(findControl(*editor, "displayButton1"));
    require(display != nullptr, "Display button missing");
    processor.set("tab", 1);
    display->onClick();
    require(processor.value("viewSpectrum") == 1, "Spectrum display button failed");
    processor.set("tab", 2);
    processor.set("viewCorrelation", 0);
    processor.set("tab", 1);
    require(processor.value("viewSpectrum") == 1 && processor.value("viewCorrelation") == 0,
            "Page displays were not retained independently");
    auto *disabled = findControl(*editor, "page5");
    require(disabled && !disabled->isEnabled() && editor->getControlParameterIndex(*disabled) == -1,
            "Unsupported loudness must not pretend to be implemented");
    std::cout << "PASS: 37 real parameters; GUI/host bypass and gains, track/cue/stereo normalized "
                 "mapping, independent page views.\n";
}
void testLifecycle() {
    MetricProcessor processor, secondInstance;
    juce::MidiBuffer midi;
    for (int iteration = 0; iteration < 50; ++iteration) {
        const double rate = iteration % 2 == 0 ? 44100.0 : 192000.0;
        const int size = iteration % 2 == 0 ? 32 : 2048;
        processor.prepareToPlay(rate, size);
        juce::AudioBuffer<float> buffer(2, size);
        buffer.clear();
        buffer.setSample(0, 0, 0.75f);
        buffer.setSample(1, 0, -0.25f);
        processor.processBlockBypassed(buffer, midi);
        require(buffer.getSample(0, 0) == 0.75f && buffer.getSample(1, 0) == -0.25f,
                "Host bypass altered audio");
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr && editor->getWidth() == 1200 && editor->getHeight() == 720,
                "Editor initial geometry wrong");
        editor->setSize(iteration % 2 == 0 ? 800 : 1800, iteration % 2 == 0 ? 480 : 1080);
        auto *button = dynamic_cast<juce::ToggleButton *>(findControl(*editor, "bypass"));
        require(button && editor->getLocalBounds().contains(
                              editor->getLocalArea(button, button->getLocalBounds())),
                "Control outside resized editor");
        button->setToggleState(true, juce::sendNotificationSync);
        require(secondInstance.state.getRawParameterValue("bypass")->load() == 0.0f,
                "Parameter leaked into second instance");
        editor.reset(); // detach listeners before further host automation
        processor.state.getParameter("bypass")->setValueNotifyingHost(0.0f);
        processor.processBlock(buffer, midi);
        require(buffer.getSample(0, 0) == 0.75f && buffer.getSample(1, 0) == -0.25f,
                "Reprepare altered audio");
        processor.releaseResources();
    }
    for (int channels : {1, 2}) {
        auto layout = processor.getBusesLayout();
        layout.inputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
        layout.outputBuses.set(0,
                               juce::AudioChannelSet::canonicalChannelSet(channels == 1 ? 2 : 1));
        require(!processor.isBusesLayoutSupported(layout),
                "Mismatched input/output layout accepted");
    }
    auto unsupported = processor.getBusesLayout();
    unsupported.inputBuses.set(0, juce::AudioChannelSet::create5point1());
    unsupported.outputBuses.set(0, juce::AudioChannelSet::create5point1());
    require(!processor.isBusesLayoutSupported(unsupported), "Surround layout accepted");
    require(processor.getLatencySamples() == 0 && processor.getTailLengthSeconds() == 0,
            "Foundation latency/tail changed");
    require(!processor.acceptsMidi() && !processor.producesMidi(), "Unexpected MIDI capability");
    std::cout << "PASS: 50 editor lifecycles, resizing, independent instances, "
                 "reprepare/sample-rate changes, host bypass and invalid bus layouts.\n";
}

void testReferenceAndMonitoring() {
    juce::TemporaryFile file(".wav");
    {
        juce::WavAudioFormat format;
        std::unique_ptr<juce::OutputStream> stream = file.getFile().createOutputStream();
        require(stream != nullptr, "Cannot create fixture WAV");
        auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions{}
                                                         .withSampleRate(48000)
                                                         .withNumChannels(2)
                                                         .withBitsPerSample(24));
        require(writer != nullptr, "Cannot create WAV writer");
        juce::AudioBuffer<float> audio(2, 8192);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < audio.getNumSamples(); ++i)
                audio.setSample(c, i, -.5f);
        require(writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()),
                "Cannot write WAV fixture");
    }
    MetricProcessor processor;
    processor.prepareToPlay(48000, 2048);
    processor.library.load(0, file.getFile());
    for (int n = 0; n < 500 && !processor.library.get(0); ++n)
        juce::Thread::sleep(10);
    require(processor.library.get(0) != nullptr, "Reference did not decode");
    processor.set("playing", 1);
    processor.set("ab", 1);
    juce::AudioBuffer<float> audio(2, 2048);
    juce::MidiBuffer midi;
    auto constant = [&](float left, float right) {
        for (int i = 0; i < audio.getNumSamples(); ++i) {
            audio.setSample(0, i, left);
            audio.setSample(1, i, right);
        }
        processor.processBlock(audio, midi);
    };
    constant(.25f, .25f);
    require(std::abs(audio.getSample(0, 2047) + .5f) < 1e-5f,
            "B monitoring must output B, never A+B");
    for (int i = 0; i < 2048; ++i)
        require(audio.getSample(0, i) <= .251f && audio.getSample(0, i) >= -.501f,
                "A/B crossfade overshot signal bounds");
    processor.set("ab", 0);
    constant(.25f, .25f);
    require(std::abs(audio.getSample(0, 2047) - .25f) < 1e-6f,
            "A monitoring did not restore DAW mix");
    require(processor.peakB[0].load() > .49f, "B analysis stopped while A was monitored");
    processor.set("gainA", -6);
    constant(.25f, .25f);
    require(std::abs(audio.getSample(0, 2047) - .25f * std::pow(10.f, -6.f / 20.f)) < 1e-5f,
            "A gain wrong");
    processor.set("gainA", 0);
    processor.set("stereo", 1);
    constant(.6f, .2f);
    require(std::abs(audio.getSample(0, 2047) - .4f) < 1e-5f &&
                std::abs(audio.getSample(1, 2047) - .4f) < 1e-5f,
            "Mono sum wrong");
    processor.set("stereo", 2);
    processor.set("solo", 1);
    constant(.6f, .2f);
    require(std::abs(audio.getSample(0, 2047) - .6f) < 1e-5f && audio.getSample(1, 2047) == 0,
            "Solo-in-place wrong");
    processor.set("stereo", 4);
    constant(.6f, .2f);
    require(std::abs(audio.getSample(0, 2047) - .2f) < 1e-5f &&
                std::abs(audio.getSample(1, 2047) + .2f) < 1e-5f,
            "Side extraction wrong");
    processor.set("bypass", 1);
    constant(.6f, .2f);
    require(audio.getSample(0, 2047) == .6f && audio.getSample(1, 2047) == .2f,
            "Bypass must restore dry unprocessed A");
    processor.cues[0][2].store(.075);
    juce::MemoryBlock saved;
    processor.getStateInformation(saved);
    MetricProcessor restored;
    restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    require(restored.cues[0][2].load() == .075 && restored.value("bypass") == 1,
            "Reference/cue state round trip failed");
    for (int n = 0; n < 500 && !restored.library.get(0); ++n)
        juce::Thread::sleep(10);
    require(restored.library.get(0) != nullptr, "State did not reload reference path");
    restored.library.load(2, file.getFile());
    restored.library.clear(2);
    juce::Thread::sleep(100);
    require(restored.library.get(2) == nullptr,
            "Cleared slot resurrected an obsolete asynchronous load");
    restored.library.load(1, file.getFile().getSiblingFile("chi-missing-reference.wav"));
    juce::Thread::sleep(40);
    require(restored.library.get(1) == nullptr, "Missing reference should not fabricate audio");
    std::cout << "PASS: WAV decoding, real A/B isolation and crossfade bounds, concurrent B "
                 "analysis, gain, mono/solo/sides, dry bypass, file/cue state and missing file.\n";
}
void testFilters() {
    MetricProcessor processor;
    processor.set("filters", 1);
    processor.set("low", 300);
    processor.set("high", 3000);
    processor.set("filterMode", 0);
    processor.set("slope", 1);
    processor.prepareToPlay(48000, 2048);
    juce::AudioBuffer<float> audio(2, 2048);
    juce::MidiBuffer midi;
    auto tone = [&](double frequency) {
        double sum = 0;
        for (int block = 0; block < 12; ++block) {
            for (int i = 0; i < 2048; ++i) {
                float v = static_cast<float>(.5 * std::sin(2 * juce::MathConstants<double>::pi *
                                                           frequency * (block * 2048 + i) / 48000));
                audio.setSample(0, i, v);
                audio.setSample(1, i, v);
            }
            processor.processBlock(audio, midi);
            if (block == 11)
                for (int i = 0; i < 2048; ++i) {
                    require(std::isfinite(audio.getSample(0, i)), "Unstable filter");
                    sum += audio.getSample(0, i) * audio.getSample(0, i);
                }
        }
        return std::sqrt(sum / 2048);
    };
    double low = tone(100), high = tone(5000);
    require(low > .3 && high < .005, "Low-pass frequency response wrong");
    for (int slope = 0; slope < 4; ++slope)
        for (int mode = 0; mode < 3; ++mode) {
            processor.set("slope", static_cast<float>(slope));
            processor.set("filterMode", static_cast<float>(mode));
            for (int i = 0; i < 2048; ++i) {
                audio.setSample(0, i, i == 0 ? 1.0f : 0.0f);
                audio.setSample(1, i, 0);
            }
            processor.processBlock(audio, midi);
            for (int i = 0; i < 2048; ++i)
                require(std::isfinite(audio.getSample(0, i)) && std::abs(audio.getSample(0, i)) < 5,
                        "Filter automation instability");
        }
    processor.set("filters", 0);
    processor.set("bypass", 1);
    audio.clear();
    audio.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
    audio.setSample(1, 0, std::numeric_limits<float>::infinity());
    processor.processBlock(audio, midi);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < 2048; ++i)
            require(std::isfinite(audio.getSample(c, i)), "Nonfinite output");
    std::cout << "PASS: low-pass known response, all 12 filter/slope combinations, automation and "
                 "NaN/Inf sanitization.\n";
}

void testAnalysis() {
    MetricProcessor p;
    p.prepareToPlay(48000, 4096);
    juce::AudioBuffer<float> audio(2, 4096);
    juce::MidiBuffer midi;
    const double frequency = 48000.0 * 85 / 4096;
    for (int block = 0; block < 7; ++block) {
        for (int c = 0; c < 2; ++c)
            for (int n = 0; n < 4096; ++n)
                audio.setSample(
                    c, n,
                    static_cast<float>(.25 * std::sin(2 * juce::MathConstants<double>::pi *
                                                      frequency * (n + block * 4096) / 48000)));
        p.processBlock(audio, midi);
    }
    std::unique_ptr<juce::AudioProcessorEditor> base(p.createEditor());
    auto *editor = dynamic_cast<MetricEditor *>(base.get());
    require(editor != nullptr, "Wrong editor type");
    require(std::abs(editor->spectrumDb(0, frequency) - 20 * std::log10(.25)) < .1,
            "Hann FFT normalization wrong");
    require(editor->spectrumDb(1, frequency) < -100,
            "Silent reference must not fabricate spectrum");
    std::cout << "PASS: real FIFO/FFT path, calibrated sine magnitude and silent B spectrum.\n";
}
void snapshotSignal(const juce::String &path) {
    const auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getNonexistentChildFile("CHi calibration reference", ".wav");
    juce::AudioBuffer<float> source(2, 48000);
    juce::Random random(12345);
    std::array<float, 2> smooth{};
    for (int i = 0; i < 48000; ++i)
        for (int c = 0; c < 2; ++c) {
            smooth[static_cast<size_t>(c)] =
                .92f * smooth[static_cast<size_t>(c)] + .08f * (random.nextFloat() * 2 - 1);
            source.setSample(c, i, smooth[static_cast<size_t>(c)] * 3);
        }
    {
        juce::WavAudioFormat format;
        std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
        require(stream != nullptr, "Cannot create calibration WAV");
        auto writer = format.createWriterFor(stream, juce::AudioFormatWriterOptions{}
                                                         .withSampleRate(48000)
                                                         .withNumChannels(2)
                                                         .withBitsPerSample(24));
        require(writer && writer->writeFromAudioSampleBuffer(source, 0, 48000),
                "Cannot write calibration WAV");
    }
    {
        MetricProcessor p;
        p.prepareToPlay(48000, 4096);
        p.library.load(0, file);
        for (int n = 0; n < 500 && !p.library.get(0); ++n)
            juce::Thread::sleep(10);
        require(p.library.get(0) != nullptr, "Cannot load calibration reference");
        p.set("playing", 1);
        juce::AudioBuffer<float> block(2, 4096);
        juce::MidiBuffer midi;
        for (int b = 0; b < 7; ++b) {
            for (int c = 0; c < 2; ++c)
                for (int n = 0; n < 4096; ++n)
                    block.setSample(
                        c, n,
                        source.getSample(c, b * 4096 + n) * .75f +
                            static_cast<float>(.025 * std::sin(2 * juce::MathConstants<double>::pi *
                                                               82 * (n + b * 4096) / 48000)));
            p.processBlock(block, midi);
        }
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true);
        juce::FileOutputStream output(juce::File::getCurrentWorkingDirectory().getChildFile(path));
        require(output.openedOk() && output.setPosition(0) && output.truncate().wasOk(),
                "Cannot write screenshot");
        require(juce::PNGImageFormat().writeImageToStream(image, output),
                "Cannot encode screenshot");
    }
    file.deleteFile();
    std::cout << "Compiled editor rendered using actual processed calibration signals (not a "
                 "design mockup).\n";
}
int main(int argc, char **argv) {
    try {
        juce::ScopedJuceInitialiser_GUI initialiseJuce;
        if (argc == 3 && juce::String(argv[1]) == "--snapshot-signal") {
            snapshotSignal(argv[2]);
            return 0;
        }
        if (argc == 2 && juce::String(argv[1]) == "--analysis") {
            testAnalysis();
            return 0;
        }
        if (argc == 3 && juce::String(argv[1]) == "--snapshot") {
            MetricProcessor processor;
            std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
            auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true);
            juce::File target = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
            juce::FileOutputStream output(target);
            require(output.openedOk(), "Cannot open screenshot file");
            require(output.setPosition(0) && output.truncate().wasOk(),
                    "Cannot reset screenshot file");
            require(juce::PNGImageFormat().writeImageToStream(image, output),
                    "Cannot encode screenshot");
            std::cout << "Actual JUCE editor rendered to " << target.getFullPathName() << "\n";
            return 0;
        }
        if (argc == 2 && juce::String(argv[1]) == "--inventory") {
            MetricProcessor processor;
            juce::Array<juce::var> entries;
            for (auto *parameter : processor.getParameters()) {
                auto item = std::make_unique<juce::DynamicObject>();
                auto *identified = dynamic_cast<juce::AudioProcessorParameterWithID *>(parameter);
                require(identified != nullptr, "Parameter without persistent ID");
                item->setProperty("id", identified->paramID);
                item->setProperty("name", parameter->getName(64));
                item->setProperty("index", parameter->getParameterIndex());
                item->setProperty("automatable", parameter->isAutomatable());
                item->setProperty("steps", parameter->getNumSteps());
                item->setProperty("defaultNormalized", parameter->getDefaultValue());
                entries.add(juce::var(item.release()));
            }
            std::cout << juce::JSON::toString(juce::var(entries)).toStdString() << "\n";
            return 0;
        }
        if (argc == 2 && juce::String(argv[1]) == "--mapping") {
            testMapping();
            return 0;
        }
        if (argc == 2 && juce::String(argv[1]) == "--lifecycle") {
            testLifecycle();
            return 0;
        }
        if (argc == 2 && juce::String(argv[1]) == "--reference") {
            testReferenceAndMonitoring();
            return 0;
        }
        if (argc == 2 && juce::String(argv[1]) == "--filters") {
            testFilters();
            return 0;
        }
        require(argc == 1, "Unknown test option");
        int cases = 0;
        for (double rate : {44100., 48000., 88200., 96000., 192000.})
            for (int block : {32, 2048})
                for (int channels : {1, 2}) {
                    MetricProcessor processor;
                    auto layout = processor.getBusesLayout();
                    layout.inputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
                    layout.outputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
                    require(processor.setBusesLayout(layout), "Mono/stereo layout rejected");
                    processor.prepareToPlay(rate, block);
                    for (bool bypass : {false, true}) {
                        processor.state.getParameter("bypass")->setValueNotifyingHost(
                            bypass ? 1.0f : 0.0f);
                        for (int signal = 0; signal < 4; ++signal) {
                            juce::AudioBuffer<float> buffer(channels, block),
                                expected(channels, block);
                            for (int c = 0; c < channels; ++c)
                                for (int s = 0; s < block; ++s) {
                                    float value =
                                        signal == 0 ? 0.0f
                                        : signal == 1
                                            ? (s == 0 ? 1.0f : 0.0f)
                                            : static_cast<float>(
                                                  0.5 *
                                                  std::sin(2.0 * juce::MathConstants<double>::pi *
                                                           1000.0 * s / rate));
                                    if (signal == 3 && c == 1)
                                        value = -value;
                                    buffer.setSample(c, s, value);
                                    expected.setSample(c, s, value);
                                }
                            juce::MidiBuffer midi;
                            processor.processBlock(buffer, midi);
                            for (int c = 0; c < channels; ++c)
                                for (int s = 0; s < block; ++s)
                                    require(std::isfinite(buffer.getSample(c, s)) &&
                                                buffer.getSample(c, s) == expected.getSample(c, s),
                                            "Pass-through changed audio");
                            ++cases;
                        }
                    }
                    juce::MemoryBlock saved;
                    processor.getStateInformation(saved);
                    MetricProcessor restored;
                    restored.setStateInformation(saved.getData(),
                                                 static_cast<int>(saved.getSize()));
                    require(restored.state.getRawParameterValue("bypass")->load() == 1.0f,
                            "State restoration failed");
                    const char invalid[] = "invalid";
                    restored.setStateInformation(invalid, sizeof(invalid));
                    require(restored.state.getRawParameterValue("bypass")->load() == 1.0f,
                            "Invalid state changed parameters");
                    processor.releaseResources();
                }
        std::cout << cases << " audio cases passed; 20 state round trips passed.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
