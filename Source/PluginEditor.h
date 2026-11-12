#pragma once
#include "PluginProcessor.h"
class MetricEditor final : public juce::AudioProcessorEditor {
public:
 explicit MetricEditor(MetricProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 int getControlParameterIndex(juce::Component&) override;
private:
 juce::ToggleButton bypass{"BYPASS"};
 juce::AudioProcessorValueTreeState::ButtonAttachment attachment;
};
