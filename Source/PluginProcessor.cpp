#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace {
class MappedChoice final : public juce::AudioParameterChoice {
  public:
    MappedChoice(const juce::ParameterID &id, const juce::String &parameterName,
                 const juce::StringArray &names, int initial)
        : juce::AudioParameterChoice(id, parameterName, names, initial),
          mapping(
              0.0f, static_cast<float>(names.size() - 1),
              [](float, float end, float v) { return juce::jlimit(0.0f, end, v * end); },
              [](float, float end, float v) { return juce::jlimit(0.0f, 1.0f, v / end); },
              [](float start, float end, float v) {
                  return std::floor(juce::jlimit(start, end, v) + .5f);
              }) {
        mapping.interval = 1;
    }
    const juce::NormalisableRange<float> &getNormalisableRange() const override {
        return mapping;
    }

  private:
    juce::NormalisableRange<float> mapping;
};
} // namespace
juce::AudioProcessorValueTreeState::ParameterLayout MetricProcessor::parameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto boolean = [&](const char *id, const char *name, bool initial = false) {
        p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, name, initial));
    };
    auto number = [&](const char *id, const char *name, float low, float high, float initial,
                      float step = 0.1f, float skew = 1.0f) {
        p.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(low, high, step, skew),
            initial));
    };
    auto choice = [&](const char *id, const char *name, juce::StringArray names, int initial = 0) {
        p.add(std::make_unique<MappedChoice>(juce::ParameterID{id, 1}, name, names, initial));
    };
    boolean("bypass", "Bypass");
    boolean("ab", "Monitor B");
    boolean("playing", "Reference Play");
    boolean("loop", "Reference Loop", true);
    boolean("solo", "Solo In Place");
    boolean("filters", "Filters");
    number("gainA", "A Stream Gain", -18, 18, 0);
    number("gainB", "B Stream Gain", -18, 18, 0);
    juce::StringArray tracks;
    for (int i = 1; i <= 16; ++i)
        tracks.add(juce::String(i));
    choice("reference", "Selected Reference", tracks);
    choice("stereo", "Stereo Mode", {"Stereo", "Mono", "Left", "Right", "Sides"});
    choice("playMode", "Playback Mode", {"Manual", "Latch", "Cue", "Sync"}, 1);
    choice("cue", "Selected Cue", {"1", "2", "3", "4"});
    choice("tab", "Selected Page",
           {"Playback", "Spectrum", "Correlation", "Stereo Image", "Dynamics"}, 1);
    for (const auto *id :
         {"viewPlayback", "viewSpectrum", "viewCorrelation", "viewStereo", "viewDynamics"})
        choice(id, id, {"Single", "Dual", "Layered"}, 2);
    boolean("zoomDb", "Zoom dB", true);
    boolean("zoomHz", "Zoom Hz");
    boolean("meterZoom", "Meter Zoom");
    number("zoomLow", "Zoom Low Hz", 20, 20000, 20, 1, .3f);
    number("zoomHigh", "Zoom High Hz", 20, 20000, 20000, 1, .3f);
    number("peakDecay", "Peak Decay dB per Second", 1, 80, 32);
    choice("filterMode", "Filter Region", {"Below", "Inside", "Above"}, 1);
    choice("slope", "Filter Slope", {"12 dB", "24 dB", "36 dB", "48 dB"});
    number("low", "Filter Low", 20, 20000, 79, 1, 0.3f);
    number("high", "Filter High", 20, 20000, 317, 1, 0.3f);
    choice("fft", "FFT Size", {"2048", "4096", "8192", "16384", "32768"}, 2);
    boolean("freeze", "Freeze Analysis");
    boolean("peak", "Peak", true);
    boolean("average", "Average", true);
    boolean("hold", "Hold", true);
    number("holdTime", "Peak Hold Seconds", 0, 10, 1);
    number("averageTime", "Average Milliseconds", 0, 4000, 1000, 1);
    number("dbTop", "Graph Top dB", -24, 36, 12);
    number("dbBottom", "Graph Bottom dB", -144, -36, -96);
    return p;
}
MetricProcessor::MetricProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Mix A", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Monitor", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "CHiState", parameters()) {
    for (auto &channel : analysis)
        channel.assign(fifoCapacity, 0);
    for (auto *parameter : getParameters())
        if (auto *identified = dynamic_cast<juce::AudioProcessorParameterWithID *>(parameter))
            parameterCache.emplace_back(identified->paramID.toStdString(),
                                        state.getRawParameterValue(identified->paramID));
    for (auto &track : cues)
        for (auto &point : track)
            point.store(-1);
    bypassParam = state.getRawParameterValue("bypass");
}
void MetricProcessor::prepareToPlay(double rate, int) {
    rate = std::isfinite(rate) && rate >= 8000 ? rate : 44100;
    currentRate.store(rate);
    filterFadeLength = juce::jmax(1, static_cast<int>(rate * .01));
    for (auto *gain : {&gainA, &gainB, &abFade, &bypassFade})
        gain->reset(rate, .01);
    gainA.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(value("gainA")));
    gainB.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(value("gainB")));
    abFade.setCurrentAndTargetValue(value("ab"));
    bypassFade.setCurrentAndTargetValue(value("bypass"));
    filters = {};
    oldFilters = {};
    lastFilter.fill(-1);
    filterFadeRemaining = 0;
}
bool MetricProcessor::isBusesLayoutSupported(const BusesLayout &layout) const {
    auto c = layout.getMainOutputChannelSet();
    return (c == juce::AudioChannelSet::mono() || c == juce::AudioChannelSet::stereo()) &&
           c == layout.getMainInputChannelSet();
}
void MetricProcessor::FilterBank::configure(double rate, double low, double high, int mode,
                                            int slope) {
    count = 0;
    low = juce::jlimit(5.0, rate * .49, low);
    high = juce::jlimit(low, rate * .49, high);
    const std::array<std::array<double, 4>, 4> q{{{{.70710678, 0, 0, 0}},
                                                  {{.5411961, 1.306563, 0, 0}},
                                                  {{.517638, .70710678, 1.931852, 0}},
                                                  {{.509796, .601345, .899976, 2.562915}}}};
    auto append = [&](bool highpass, double frequency) {
        for (int section = 0; section <= slope; ++section) {
            const double omega = 2 * juce::MathConstants<double>::pi * frequency / rate,
                         cosine = std::cos(omega), sine = std::sin(omega),
                         alpha = sine /
                                 (2 * q[static_cast<size_t>(slope)][static_cast<size_t>(section)]),
                         a0 = 1 + alpha;
            for (auto &channel : filters) {
                auto &f = channel[static_cast<size_t>(count)];
                f.b0 = (highpass ? (1 + cosine) : (1 - cosine)) * .5 / a0;
                f.b1 = (highpass ? -(1 + cosine) : (1 - cosine)) / a0;
                f.b2 = f.b0;
                f.a1 = -2 * cosine / a0;
                f.a2 = (1 - alpha) / a0;
                f.z1 = f.z2 = 0;
            }
            ++count;
        }
    };
    if (mode == 0)
        append(false, low);
    else if (mode == 2)
        append(true, high);
    else {
        append(true, low);
        append(false, high);
    }
}
void MetricProcessor::choose(int index, bool keepA) {
    const bool same = selected() == index;
    set("reference", static_cast<float>(juce::jlimit(0, 15, index)));
    set("ab", keepA ? 0.0f : (same ? 1.0f - value("ab") : 1.0f));
    if (!keepA && library.get(index))
        set("playing", 1);
}
void MetricProcessor::cue(int index, bool remove) {
    index = juce::jlimit(0, 3, index);
    auto &point = cues[static_cast<size_t>(selected())][static_cast<size_t>(index)];
    if (remove)
        point.store(-1);
    else if (point.load() < 0)
        point.store(position.load());
    else
        seekSeconds.store(point.load());
    set("cue", static_cast<float>(index));
}
void MetricProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) {
    juce::ScopedNoDenormals noDenormals;
    const int samples = buffer.getNumSamples(),
              channels = juce::jmin(getTotalNumInputChannels(), buffer.getNumChannels());
    if (samples <= 0 || channels <= 0)
        return;
    const double rate = currentRate.load();
    const auto *track = library.get(selected());
    const bool trackChanged = track != lastTrack;
    if (trackChanged) {
        playFrame = 0;
        lastTrack = track;
        referenceOrigin = lastReference;
        referenceFade = filterFadeLength;
    }
    const double request = seekSeconds.exchange(-1);
    if (request >= 0 && track) {
        playFrame = juce::jlimit(0.0, static_cast<double>(track->audio.getNumSamples() - 1),
                                 request * track->rate);
        referenceOrigin = lastReference;
        referenceFade = filterFadeLength;
    }
    bool play = value("playing") > .5f;
    const int playMode = juce::roundToInt(value("playMode"));
    if (playMode != 0)
        if (auto *host = getPlayHead())
            if (auto info = host->getPosition()) {
                play = info->getIsPlaying();
                if (playMode == 3 && track)
                    if (auto time = info->getTimeInSeconds()) {
                        const double target = juce::jmax(0.0, *time * track->rate);
                        if (std::abs(target - playFrame) > 4) {
                            referenceOrigin = lastReference;
                            referenceFade = filterFadeLength;
                        }
                        playFrame = target;
                        if (*time < 0)
                            play = false;
                    }
            }
    if (play != referenceWasPlaying) {
        referenceOrigin = lastReference;
        referenceFade = filterFadeLength;
        referenceWasPlaying = play;
    }
    const int cueIndex = juce::roundToInt(value("cue"));
    const bool monitoringB = value("ab") > .5f;
    if (cueIndex != lastCue || (playMode == 2 && (trackChanged || (monitoringB && !wasB)))) {
        if (playMode == 2 && track) {
            auto point =
                cues[static_cast<size_t>(selected())][static_cast<size_t>(cueIndex)].load();
            if (point >= 0)
                playFrame = point * track->rate;
        }
        lastCue = cueIndex;
    }
    wasB = monitoringB;
    gainA.setTargetValue(juce::Decibels::decibelsToGain(value("gainA")));
    gainB.setTargetValue(juce::Decibels::decibelsToGain(value("gainB")));
    abFade.setTargetValue(value("ab"));
    bypassFade.setTargetValue(bypassParam->load());
    const int mode = juce::roundToInt(value("stereo"));
    const bool solo = value("solo") > .5f, loop = value("loop") > .5f,
               filterOn = value("filters") > .5f;
    const std::array<float, 5> config{
        {value("low"), value("high"), value("filterMode"), value("slope"), filterOn ? 1.0f : 0.0f}};
    if (config != lastFilter) {
        oldFilters = filters;
        filters.configure(rate, juce::jmin(config[0], config[1]), juce::jmax(config[0], config[1]),
                          juce::roundToInt(config[2]), juce::roundToInt(config[3]));
        if (!filterOn)
            filters.count = 0;
        filterFadeRemaining = filterFadeLength;
        lastFilter = config;
    }
    std::array<double, 4> squares{};
    std::array<float, 4> peaks{};
    double crossA = 0, crossB = 0;
    int first = 0, size1 = 0, second = 0, size2 = 0;
    analysisFifo.prepareToWrite(samples, first, size1, second, size2);
    for (int n = 0; n < samples; ++n) {
        const float inputL = buffer.getSample(0, n),
                    inputR = channels > 1 ? buffer.getSample(1, n) : inputL;
        const float dryL = std::isfinite(inputL) ? juce::jlimit(-1.0e6f, 1.0e6f, inputL) : 0,
                    dryR = std::isfinite(inputR) ? juce::jlimit(-1.0e6f, 1.0e6f, inputR) : 0;
        const float ga = gainA.getNextValue(), gb = gainB.getNextValue();
        std::array<float, 4> x{{dryL * ga, dryR * ga, 0, 0}};
        if (track && play) {
            const int length = track->audio.getNumSamples();
            if (playFrame >= length) {
                if (loop) {
                    playFrame = std::fmod(playFrame, length);
                    referenceOrigin = lastReference;
                    referenceFade = filterFadeLength;
                } else
                    playFrame = length;
            }
            if (playFrame < length) {
                const int i = static_cast<int>(playFrame), next = juce::jmin(i + 1, length - 1);
                const float fraction = static_cast<float>(playFrame - i);
                for (int c = 0; c < 2; ++c) {
                    const auto *data = track->audio.getReadPointer(c);
                    x[static_cast<size_t>(c + 2)] =
                        (data[i] + fraction * (data[next] - data[i])) * gb;
                }
                playFrame += track->rate / rate;
            }
        }
        if (referenceFade > 0) {
            const float mix = 1 - static_cast<float>(referenceFade) / filterFadeLength;
            for (int c = 0; c < 2; ++c)
                x[static_cast<size_t>(c + 2)] =
                    referenceOrigin[static_cast<size_t>(c)] +
                    mix * (x[static_cast<size_t>(c + 2)] - referenceOrigin[static_cast<size_t>(c)]);
            --referenceFade;
        }
        for (int c = 0; c < 2; ++c)
            lastReference[static_cast<size_t>(c)] = x[static_cast<size_t>(c + 2)];
        for (auto &sample : x)
            sample = std::isfinite(sample) ? juce::jlimit(-1.0e6f, 1.0e6f, sample) : 0;
        if (channels == 1)
            x[2] = x[3] = (x[2] + x[3]) * .5f;
        auto previous = x;
        filters.tick(x);
        if (filterFadeRemaining > 0) {
            oldFilters.tick(previous);
            const float blend = 1 - static_cast<float>(filterFadeRemaining) / filterFadeLength;
            for (size_t c = 0; c < 4; ++c)
                x[c] = previous[c] + blend * (x[c] - previous[c]);
            --filterFadeRemaining;
        }
        for (int stream = 0; stream < 2; ++stream) {
            auto &l = x[static_cast<size_t>(stream * 2)];
            auto &r = x[static_cast<size_t>(stream * 2 + 1)];
            const float mid = (l + r) * .5f, side = (l - r) * .5f;
            if (mode == 1)
                l = r = mid;
            else if (mode == 2) {
                r = solo ? 0 : l;
            } else if (mode == 3) {
                l = solo ? 0 : r;
            } else if (mode == 4) {
                l = side;
                r = -side;
            }
        }
        for (size_t c = 0; c < 4; ++c) {
            squares[c] += static_cast<double>(x[c]) * x[c];
            peaks[c] = juce::jmax(peaks[c], std::abs(x[c]));
        }
        crossA += static_cast<double>(x[0]) * x[1];
        crossB += static_cast<double>(x[2]) * x[3];
        if (n < size1 + size2) {
            const int dest = n < size1 ? first + n : second + n - size1;
            for (size_t c = 0; c < 4; ++c)
                analysis[c][static_cast<size_t>(dest)] = x[c];
        }
        const float b = abFade.getNextValue(), bp = bypassFade.getNextValue();
        for (int c = 0; c < channels; ++c) {
            const float monitor =
                b >= 1 ? x[static_cast<size_t>(c + 2)]
                       : (b <= 0 ? x[static_cast<size_t>(c)]
                                 : x[static_cast<size_t>(c)] + b * (x[static_cast<size_t>(c + 2)] -
                                                                    x[static_cast<size_t>(c)]));
            const float dry = c == 0 ? dryL : dryR;
            buffer.setSample(c, n,
                             bp >= 1 ? dry : (bp <= 0 ? monitor : monitor + bp * (dry - monitor)));
        }
    }
    analysisFifo.finishedWrite(size1 + size2);
    if (track)
        position.store(playFrame / track->rate);
    else
        position.store(0);
    for (int c = 0; c < 2; ++c) {
        peakA[c].store(peaks[static_cast<size_t>(c)]);
        peakB[c].store(peaks[static_cast<size_t>(c + 2)]);
        rmsA[c].store(static_cast<float>(std::sqrt(squares[static_cast<size_t>(c)] / samples)));
        rmsB[c].store(static_cast<float>(std::sqrt(squares[static_cast<size_t>(c + 2)] / samples)));
    }
    const double denominatorA = std::sqrt(squares[0] * squares[1]),
                 denominatorB = std::sqrt(squares[2] * squares[3]);
    correlationA.store(denominatorA > 1e-15 ? static_cast<float>(crossA / denominatorA) : 0);
    correlationB.store(denominatorB > 1e-15 ? static_cast<float>(crossB / denominatorB) : 0);
    for (int c = channels; c < buffer.getNumChannels(); ++c)
        buffer.clear(c, 0, samples);
}
void MetricProcessor::getStateInformation(juce::MemoryBlock &dest) {
    auto snapshot = state.copyState();
    for (int i = 0; i < 16; ++i) {
        juce::ValueTree item("Reference");
        item.setProperty("slot", i, nullptr);
        if (auto *track = library.get(i))
            item.setProperty("path", track->path, nullptr);
        for (int c = 0; c < 4; ++c)
            item.setProperty("cue" + juce::String(c),
                             cues[static_cast<size_t>(i)][static_cast<size_t>(c)].load(), nullptr);
        snapshot.addChild(item, -1, nullptr);
    }
    if (auto xml = snapshot.createXml())
        copyXmlToBinary(*xml, dest);
}
void MetricProcessor::setStateInformation(const void *data, int size) {
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(state.state.getType())) {
        auto snapshot = juce::ValueTree::fromXml(*xml);
        for (int i = 0; i < 16; ++i)
            library.clear(i);
        for (auto item : snapshot)
            if (item.hasType("Reference")) {
                int slot = juce::jlimit(0, 15, static_cast<int>(item["slot"]));
                for (int c = 0; c < 4; ++c)
                    cues[static_cast<size_t>(slot)][static_cast<size_t>(c)].store(
                        static_cast<double>(item.getProperty("cue" + juce::String(c), -1.0)));
                const auto path = item["path"].toString();
                library.clear(slot);
                if (path.isNotEmpty())
                    library.load(slot, juce::File(path));
            }
        auto restored = juce::ValueTree::fromXml(*xml);
        for (int i = restored.getNumChildren(); --i >= 0;)
            if (restored.getChild(i).hasType("Reference"))
                restored.removeChild(i, nullptr);
        for (auto *parameter : getParameters())
            if (auto *ranged = dynamic_cast<juce::RangedAudioParameter *>(parameter))
                if (!restored.getChildWithProperty("id", ranged->paramID).isValid()) {
                    juce::ValueTree item("PARAM");
                    item.setProperty("id", ranged->paramID, nullptr);
                    item.setProperty("value", ranged->convertFrom0to1(ranged->getDefaultValue()),
                                     nullptr);
                    restored.addChild(item, -1, nullptr);
                }
        state.replaceState(restored);
    }
}
juce::AudioProcessorEditor *MetricProcessor::createEditor() {
    return new MetricEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new MetricProcessor();
}
