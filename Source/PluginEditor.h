#pragma once
#include "PluginProcessor.h"
#include <functional>
#include <map>
class MetricEditor final : public juce::AudioProcessorEditor, private juce::Timer {
  public:
    explicit MetricEditor(MetricProcessor &);
    ~MetricEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    int getControlParameterIndex(juce::Component &) override;
    float spectrumDb(int stream, double frequency) const {
        const int bin = juce::jlimit(
            0, fftSize / 2, juce::roundToInt(frequency * fftSize / processor.currentRate.load()));
        return curves[static_cast<size_t>(juce::jlimit(0, 1, stream))][0][static_cast<size_t>(bin)];
    }

  private:
    class Style final : public juce::LookAndFeel_V4 {
      public:
        void drawButtonBackground(juce::Graphics &, juce::Button &, const juce::Colour &, bool,
                                  bool) override;
        void drawButtonText(juce::Graphics &, juce::TextButton &, bool, bool) override;
        void drawToggleButton(juce::Graphics &, juce::ToggleButton &, bool, bool) override;
        void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float, float,
                              juce::Slider::SliderStyle, juce::Slider &) override;
    } style;
    class Panel final : public juce::Component, public juce::FileDragAndDropTarget {
      public:
        explicit Panel(MetricEditor &e) : owner(e) {}
        void paint(juce::Graphics &g) override {
            owner.draw(g);
        }
        bool isInterestedInFileDrag(const juce::StringArray &) override {
            return true;
        }
        void filesDropped(const juce::StringArray &, int, int) override;
        void mouseDown(const juce::MouseEvent &e) override {
            owner.graphMouse(e, false);
        }
        void mouseDrag(const juce::MouseEvent &e) override {
            owner.graphMouse(e, true);
        }
        void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &d) override;

      private:
        MetricEditor &owner;
    } panel;
    class ABButton final : public juce::ToggleButton {
      public:
        void paintButton(juce::Graphics &, bool, bool) override;
    } ab;
    void timerCallback() override;
    void draw(juce::Graphics &);
    void drawGraph(juce::Graphics &, juce::Rectangle<float>, int stream, int type,
                   bool background = true);
    void graphMouse(const juce::MouseEvent &, bool drag);
    void loadFiles(const juce::StringArray &, int start);
    void selectFile(int slot);
    void slotMenu(int slot);
    const char *viewParameter() const {
        static constexpr const char *names[]{"viewPlayback", "viewSpectrum", "viewCorrelation",
                                             "viewStereo", "viewDynamics"};
        return names[juce::jlimit(0, 4, juce::roundToInt(processor.value("tab")))];
    }
    float minHz() const {
        return processor.value("zoomHz") > .5f
                   ? juce::jmin(processor.value("zoomLow"), processor.value("zoomHigh"))
                   : 20;
    }
    float maxHz() const {
        return processor.value("zoomHz") > .5f
                   ? juce::jmax(minHz() * 1.01f,
                                juce::jmax(processor.value("zoomHigh"), processor.value("zoomLow")))
                   : 20000;
    }
    void resetAnalysis();
    void configureFFT();
    void transformFrame();
    juce::TextButton &button(const juce::String &, const juce::String &, juce::Rectangle<int>,
                             std::function<void()>);
    juce::ToggleButton &toggle(const juce::String &, const juce::String &, juce::Rectangle<int>);
    juce::Slider &slider(const juce::String &, juce::Rectangle<int>, bool vertical = false);
    juce::ComboBox &combo(const juce::String &, juce::Rectangle<int>);
    MetricProcessor &processor;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonLinks;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderLinks;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboLinks;
    std::map<juce::String, juce::Component *> byID;
    std::array<juce::TextButton *, 16> slots{};
    std::array<juce::TextButton *, 7> tabs{};
    std::array<juce::TextButton *, 3> displays{};
    std::array<juce::TextButton *, 4> cueButtons{};
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    std::array<std::vector<float>, 4> frame;
    std::array<std::array<std::vector<float>, 2>, 2> curves; // stream, current/average
    std::array<std::vector<float>, 2> held, holdAge, powerAverage, pan, phase;
    std::array<std::array<float, 2>, 2> meterPeak{}, meterRms{}, meterHold{};
    std::array<std::array<float, 512>, 2> history{};
    uint64_t analyzedFrames = 0;
    int fftSize = 0, framePosition = 0, historyPosition = 0, lastTab = -1, lastSelected = -1;
    bool closing = false;
    float waveZoom = 1, zoomCentre = .5f;
    double lastTick = 0;
    juce::TooltipWindow tooltips{this, 600};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetricEditor)
};
