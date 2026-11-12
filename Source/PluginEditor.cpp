#include "PluginEditor.h"
MetricEditor::MetricEditor(MetricProcessor& p) : AudioProcessorEditor(p), attachment(p.state, "bypass", bypass) {
 addAndMakeVisible(bypass);
 bypass.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffd5d8de));
 setResizable(true, true); setResizeLimits(800, 480, 1800, 1080); setSize(1200, 720);
}
void MetricEditor::resized() { bypass.setBounds(getWidth() - 150, 24, 120, 32); }
void MetricEditor::paint(juce::Graphics& g) {
 const auto w = static_cast<float>(getWidth()), h = static_cast<float>(getHeight());
 g.fillAll(juce::Colour(0xff17191d));
 g.setColour(juce::Colour(0xffe8a35e)); g.setFont(26.0f);
 g.drawText("CHi", 30, 20, 70, 40, juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xffd5d8de)); g.setFont(20.0f);
 g.drawText("METRIC MATCH PRO", 105, 20, 330, 40, juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xff25282d)); g.fillRoundedRectangle(30, 92, w-60, h-160, 8);
 g.setColour(juce::Colour(0xff65b7e8)); g.setFont(30.0f);
 g.drawText("A  /  MIX INPUT", 60, 130, getWidth()-120, 50, juce::Justification::centred);
 g.setColour(juce::Colour(0xffd5d8de)); g.setFont(18.0f);
 g.drawText("Foundation / unity gain pass-through", 60, 195, getWidth()-120, 40, juce::Justification::centred);
 g.setFont(15.0f);
 g.drawText("Spectrum, reference playback and EQ Match are pending implementation.", 60, 250, getWidth()-120, 60, juce::Justification::centred);
 g.drawText("CHi Audio   /   0.1.0   /   PHASE 1", 30, getHeight()-50, getWidth()-60, 25, juce::Justification::centredLeft);
}
