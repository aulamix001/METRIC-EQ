#include "../Source/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }

struct ParameterEvents final : juce::AudioProcessorParameter::Listener {
 explicit ParameterEvents(juce::AudioProcessorParameter& p) : parameter(p) { parameter.addListener(this); }
 ~ParameterEvents() override { parameter.removeListener(this); }
 void parameterValueChanged(int, float) override { ++values; }
 void parameterGestureChanged(int, bool starting) override { if (starting) ++begins; else ++ends; }
 juce::AudioProcessorParameter& parameter;
 int values = 0, begins = 0, ends = 0;
};
void testMapping() {
 MetricProcessor processor;
 require(processor.getParameters().size() == 1, "Update audit: parameter inventory changed");
 auto* parameter = processor.state.getParameter("bypass");
 require(parameter != nullptr && parameter == processor.getBypassParameter(), "Host bypass parameter mismatch");
 require(parameter->paramID == "bypass" && parameter->getName(64) == "Bypass", "Bypass identity changed");
 require(parameter->isAutomatable() && parameter->isBoolean() && parameter->getNumSteps() == 2, "Invalid bypass automation metadata");
 require(parameter->getDefaultValue() == 0.0f, "Bypass default must be off");
 ParameterEvents events(*parameter);
 std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
 auto* button = dynamic_cast<juce::ToggleButton*>(editor->findChildWithID("bypass"));
 require(button != nullptr, "Bypass GUI control missing");
 require(editor->getControlParameterIndex(*button) == parameter->getParameterIndex(), "GUI-to-host mapping failed");
 juce::Component unrelated;
 require(editor->getControlParameterIndex(unrelated) == -1, "Unrelated GUI control mapped to audio parameter");
 button->setToggleState(true, juce::sendNotificationSync);
 require(parameter->getValue() == 1.0f && processor.state.getRawParameterValue("bypass")->load() == 1.0f, "GUI did not update host parameter");
 require(events.values > 0 && events.begins == 1 && events.ends == 1, "GUI did not send complete host automation gesture");
 parameter->setValueNotifyingHost(0.0f);
 require(!button->getToggleState(), "Host automation did not update GUI");
 juce::MemoryBlock saved;
 parameter->setValueNotifyingHost(1.0f);
 processor.getStateInformation(saved);
 parameter->setValueNotifyingHost(0.0f);
 processor.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
 require(parameter->getValue() == 1.0f && button->getToggleState(), "State restore did not update GUI and parameter");
 // Verify absence explicitly: a successful bypass test must not certify nonexistent controls.
 for (const auto* missing : {"gainA", "gainB", "ab", "reference", "cue", "stereoMode", "filterHigh", "filterLow", "match"})
  require(processor.state.getParameter(missing) == nullptr, "Update audit: new functionality requires tests");
 std::cout << "PASS: bypass identity, host mapping, GUI-to-host gesture, host-to-GUI update, state restore.\n"
              "NOT IMPLEMENTED: A/B, reference player, gains, cues, stereo modes, filters, loudness match, FFT, EQ Match.\n";
}
void testLifecycle() {
 MetricProcessor processor, secondInstance;
 juce::MidiBuffer midi;
 for (int iteration = 0; iteration < 50; ++iteration) {
  const double rate = iteration % 2 == 0 ? 44100.0 : 192000.0;
  const int size = iteration % 2 == 0 ? 32 : 2048;
  processor.prepareToPlay(rate, size);
  juce::AudioBuffer<float> buffer(2, size);
  buffer.clear(); buffer.setSample(0, 0, 0.75f); buffer.setSample(1, 0, -0.25f);
  processor.processBlockBypassed(buffer, midi);
  require(buffer.getSample(0, 0) == 0.75f && buffer.getSample(1, 0) == -0.25f, "Host bypass altered audio");
  std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
  require(editor != nullptr && editor->getWidth() == 1200 && editor->getHeight() == 720, "Editor initial geometry wrong");
  editor->setSize(iteration % 2 == 0 ? 800 : 1800, iteration % 2 == 0 ? 480 : 1080);
  auto* button = dynamic_cast<juce::ToggleButton*>(editor->findChildWithID("bypass"));
  require(button && editor->getLocalBounds().contains(button->getBounds()), "Control outside resized editor");
  button->setToggleState(true, juce::sendNotificationSync);
  require(secondInstance.state.getRawParameterValue("bypass")->load() == 0.0f, "Parameter leaked into second instance");
  editor.reset(); // detach listeners before further host automation
  processor.state.getParameter("bypass")->setValueNotifyingHost(0.0f);
  processor.processBlock(buffer, midi);
  require(buffer.getSample(0, 0) == 0.75f && buffer.getSample(1, 0) == -0.25f, "Reprepare altered audio");
  processor.releaseResources();
 }
 for (int channels : {1, 2}) {
  auto layout = processor.getBusesLayout();
  layout.inputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
  layout.outputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels == 1 ? 2 : 1));
  require(!processor.isBusesLayoutSupported(layout), "Mismatched input/output layout accepted");
 }
 auto unsupported = processor.getBusesLayout();
 unsupported.inputBuses.set(0, juce::AudioChannelSet::create5point1());
 unsupported.outputBuses.set(0, juce::AudioChannelSet::create5point1());
 require(!processor.isBusesLayoutSupported(unsupported), "Surround layout accepted");
 require(processor.getLatencySamples() == 0 && processor.getTailLengthSeconds() == 0, "Foundation latency/tail changed");
 require(!processor.acceptsMidi() && !processor.producesMidi(), "Unexpected MIDI capability");
 std::cout << "PASS: 50 editor lifecycles, resizing, independent instances, reprepare/sample-rate changes, host bypass and invalid bus layouts.\n";
}

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
   require(output.setPosition(0) && output.truncate().wasOk(), "Cannot reset screenshot file");
   require(juce::PNGImageFormat().writeImageToStream(image, output), "Cannot encode screenshot");
   std::cout << "Actual JUCE editor rendered to " << target.getFullPathName() << "\n";
   return 0;
  }
  if (argc == 2 && juce::String(argv[1]) == "--inventory") {
   MetricProcessor processor;
   juce::Array<juce::var> entries;
   for (auto* parameter : processor.getParameters()) {
    auto item = std::make_unique<juce::DynamicObject>();
    auto* identified = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter);
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
  if (argc == 2 && juce::String(argv[1]) == "--mapping") { testMapping(); return 0; }
  if (argc == 2 && juce::String(argv[1]) == "--lifecycle") { testLifecycle(); return 0; }
  require(argc == 1, "Unknown test option");
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
