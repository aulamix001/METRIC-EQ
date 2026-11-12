#include "PluginProcessor.h"
#include "PluginEditor.h"
MetricProcessor::MetricProcessor()
 : AudioProcessor(BusesProperties().withInput("Mix A", juce::AudioChannelSet::stereo(), true).withOutput("Monitor", juce::AudioChannelSet::stereo(), true)),
 state(*this, nullptr, "CHiState", {std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1}, "Bypass", false)}) {}
bool MetricProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
 auto channels = layout.getMainOutputChannelSet();
 return (channels == juce::AudioChannelSet::mono() || channels == juce::AudioChannelSet::stereo()) && channels == layout.getMainInputChannelSet();
}
void MetricProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
 juce::ScopedNoDenormals noDenormals;
 for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
 // Foundation: unity pass-through in both active and bypass states. No allocations or locks.
}
void MetricProcessor::getStateInformation(juce::MemoryBlock& dest) {
 if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void MetricProcessor::setStateInformation(const void* data, int size) {
 if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* MetricProcessor::createEditor() { return new MetricEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MetricProcessor(); }
