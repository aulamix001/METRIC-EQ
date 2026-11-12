#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main(int argc, char** argv) {
 try {
  juce::ScopedJuceInitialiser_GUI initialiseJuce;
  if (argc == 3 && juce::String(argv[1]) == "--snapshot") {
   MetricProcessor processor;
   std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
   auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true);
   juce::File target = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
   juce::FileOutputStream output(target);
   require(output.openedOk(), "Cannot open screenshot file");
   require(juce::PNGImageFormat().writeImageToStream(image, output), "Cannot encode screenshot");
   std::cout << "Actual JUCE editor rendered to " << target.getFullPathName() << "\n";
   return 0;
  }
  int cases = 0;
  for (double rate : {44100., 48000., 88200., 96000., 192000.})
   for (int block : {32, 2048}) for (int channels : {1, 2}) {
    MetricProcessor processor;
    auto layout = processor.getBusesLayout();
    layout.inputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
    layout.outputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
    require(processor.setBusesLayout(layout), "Mono/stereo layout rejected");
    processor.prepareToPlay(rate, block);
    for (bool bypass : {false, true}) {
     processor.state.getParameter("bypass")->setValueNotifyingHost(bypass ? 1.0f : 0.0f);
     for (int signal = 0; signal < 4; ++signal) {
      juce::AudioBuffer<float> buffer(channels, block), expected(channels, block);
      for (int c = 0; c < channels; ++c) for (int s = 0; s < block; ++s) {
       float value = signal == 0 ? 0.0f : signal == 1 ? (s == 0 ? 1.0f : 0.0f) : static_cast<float>(0.5 * std::sin(2.0 * juce::MathConstants<double>::pi * 1000.0 * s / rate));
       if (signal == 3 && c == 1) value = -value;
       buffer.setSample(c, s, value); expected.setSample(c, s, value);
      }
      juce::MidiBuffer midi;
      processor.processBlock(buffer, midi);
      for (int c = 0; c < channels; ++c) for (int s = 0; s < block; ++s)
       require(std::isfinite(buffer.getSample(c,s)) && buffer.getSample(c,s) == expected.getSample(c,s), "Pass-through changed audio");
      ++cases;
     }
    }
    juce::MemoryBlock saved;
    processor.getStateInformation(saved);
    MetricProcessor restored;
    restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
    require(restored.state.getRawParameterValue("bypass")->load() == 1.0f, "State restoration failed");
    const char invalid[] = "invalid";
    restored.setStateInformation(invalid, sizeof(invalid));
    require(restored.state.getRawParameterValue("bypass")->load() == 1.0f, "Invalid state changed parameters");
    processor.releaseResources();
   }
  std::cout << cases << " audio cases passed; 20 state round trips passed.\n";
  return 0;
 } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
