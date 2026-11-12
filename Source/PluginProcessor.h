#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
class MetricProcessor final : public juce::AudioProcessor {
public:
 MetricProcessor();
 using juce::AudioProcessor::processBlock;
 void prepareToPlay(double, int) override {}
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "CHi Metric Match Pro"; }
 bool acceptsMidi() const override { return false; }
 bool producesMidi() const override { return false; }
 double getTailLengthSeconds() const override { return 0; }
 int getNumPrograms() override { return 1; }
 int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override { return {}; }
 void changeProgramName(int, const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*, int) override;
 juce::AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
 juce::AudioProcessorValueTreeState state;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetricProcessor)
};
