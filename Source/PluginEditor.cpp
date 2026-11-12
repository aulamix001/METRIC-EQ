#include "PluginEditor.h"
namespace {
class ActionButton : public juce::TextButton {
  public:
    explicit ActionButton(const juce::String &s) : TextButton(s) {}
    void mouseDown(const juce::MouseEvent &e) override {
        if (e.mods.isPopupMenu()) {
            if (onClick)
                onClick();
        } else
            TextButton::mouseDown(e);
    }
};
const juce::Colour blue(0xff65b7e8), amber(0xffe8a35e), silver(0xffd5d8de), edge(0xff080b0e);
void uiText(juce::Graphics &g, const juce::String &s, juce::Rectangle<float> r, float size,
            juce::Colour c = silver, int align = juce::Justification::centredLeft) {
    g.setColour(c);
    g.setFont(juce::FontOptions(size));
    g.drawText(s, r, align);
}
void box(juce::Graphics &g, juce::Rectangle<float> r, juce::Colour c = juce::Colour(0xff252a30),
         float radius = 4) {
    g.setColour(edge);
    g.fillRoundedRectangle(r, radius);
    g.setGradientFill(juce::ColourGradient(c.brighter(.13f), r.getX(), r.getY(), c.darker(.15f),
                                           r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r.reduced(1), radius);
    g.setColour(juce::Colour(0xff69717b).withAlpha(.45f));
    g.drawRoundedRectangle(r.reduced(1), radius, 1);
}
float db(float gain) {
    return juce::Decibels::gainToDecibels(juce::jmax(0.0f, gain), -120.0f);
}
juce::String level(float gain) {
    return gain > 0.000001f ? juce::String(db(gain), 1) : "-inf";
}
juce::String clockTime(double time) {
    int seconds = juce::jmax(0, static_cast<int>(time));
    return juce::String(seconds / 60) + "m " + juce::String(seconds % 60) + "s";
}
} // namespace
void MetricEditor::Style::drawButtonBackground(juce::Graphics &g, juce::Button &b,
                                               const juce::Colour &, bool over, bool down) {
    auto r = b.getLocalBounds().toFloat().reduced(.5f);
    juce::Colour c(0xff30363c);
    if (b.getToggleState())
        c = b.findColour(juce::TextButton::buttonOnColourId);
    if (over)
        c = c.brighter(.12f);
    if (down)
        c = c.darker(.12f);
    if (!b.isEnabled())
        c = c.darker(.35f);
    box(g, r, c, 3);
}
void MetricEditor::Style::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool, bool) {
    const bool tab = b.getComponentID().startsWith("page");
    auto r = b.getLocalBounds().toFloat();
    if (tab) {
        g.setColour(b.isEnabled() ? silver : juce::Colour(0xff58606a));
        int i = b.getComponentID().getTrailingIntValue();
        float x = r.getCentreX(), y = 14;
        if (i == 0) {
            for (int j = -10; j <= 10; j += 2) {
                float h = 3 + std::abs(std::sin(j * 1.7f)) * 9;
                g.drawLine(x + j, y - h * .5f, x + j, y + h * .5f, 1);
            }
        } else if (i == 1 || i == 4) {
            for (int j = 0; j < 6; ++j)
                g.fillRect(x - 13 + j * 4, y + 8 - (j % 3 + 1) * 5, 3.0f, (j % 3 + 1) * 5.0f);
        } else if (i == 2) {
            g.fillRect(x - 9, y, 7.0f, 9.0f);
            g.fillRect(x + 2, y - 9, 7.0f, 18.0f);
        } else if (i == 3) {
            g.drawEllipse(x - 12, y - 8, 17, 17, 1.2f);
            g.drawEllipse(x - 4, y - 8, 17, 17, 1.2f);
        } else if (i == 5) {
            g.drawRoundedRectangle(x - 9, y - 12, 18, 25, 2, 1);
            g.drawEllipse(x - 5, y + 1, 10, 10, 1);
            g.drawEllipse(x - 2, y - 8, 4, 4, 1);
        } else {
            g.drawLine(x - 13, y + 7, x + 13, y + 7, 1);
            g.drawLine(x - 13, y, x + 13, y, 1);
            g.drawLine(x - 13, y - 7, x + 13, y - 7, 1);
            g.fillEllipse(x - 7, y - 10, 6, 6);
            g.fillEllipse(x + 4, y - 3, 6, 6);
        }
        r.removeFromTop(30);
        uiText(g, b.getButtonText(), r, 11,
               b.isEnabled() ? (b.getToggleState() ? juce::Colour(0xff182028) : silver)
                             : juce::Colour(0xff626972),
               juce::Justification::centred);
        return;
    }
    uiText(g, b.getButtonText(), r.reduced(4, 0), 11,
           b.isEnabled() ? (b.getToggleState() ? juce::Colour(0xff11181d) : silver)
                         : juce::Colour(0xff626972),
           juce::Justification::centred);
}
void MetricEditor::Style::drawToggleButton(juce::Graphics &g, juce::ToggleButton &b, bool over,
                                           bool down) {
    drawButtonBackground(g, b, {}, over, down);
    uiText(g, b.getButtonText(), b.getLocalBounds().toFloat().reduced(3), 11,
           b.getToggleState() ? juce::Colour(0xff10151a) : silver, juce::Justification::centred);
}
void MetricEditor::Style::drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                                           float pos, float, float, juce::Slider::SliderStyle mode,
                                           juce::Slider &slider) {
    if (mode == juce::Slider::LinearVertical) {
        float centre = x + width * .5f;
        g.setColour(edge);
        g.fillRoundedRectangle(centre - 2, static_cast<float>(y), 4, static_cast<float>(height), 2);
        g.setColour(juce::Colour(0xff66727b));
        g.drawVerticalLine(static_cast<int>(centre), static_cast<float>(y),
                           static_cast<float>(y + height));
        g.setGradientFill(juce::ColourGradient(silver, centre, pos - 7, juce::Colour(0xff626b74),
                                               centre, pos + 7, false));
        g.fillRoundedRectangle(centre - 7, pos - 7, 14, 14, 2);
        g.setColour(edge);
        g.drawLine(centre - 6, pos, centre + 6, pos, 1);
    } else {
        juce::ignoreUnused(g, x, y, width, height, slider);
    }
}
void MetricEditor::ABButton::paintButton(juce::Graphics &g, bool over, bool down) {
    auto r = getLocalBounds().toFloat().reduced(3);
    const auto c = getToggleState() ? amber : blue;
    g.setColour(juce::Colour(0xff06090b));
    g.fillEllipse(r);
    g.setColour(juce::Colour(0xff77818b));
    g.drawEllipse(r.reduced(2), 1);
    g.setColour(juce::Colour(0xff161c22));
    g.fillEllipse(r.reduced(7));
    g.setColour(c.withAlpha(.6f));
    g.drawEllipse(r.reduced(12), 2);
    auto face = r.reduced(16);
    auto centre = face.getCentre();
    g.setGradientFill(juce::ColourGradient(c.brighter(over ? .35f : .1f), centre.x - 20,
                                           centre.y - 25, c.darker(down ? .8f : .65f),
                                           face.getRight(), centre.y, true));
    g.fillEllipse(face);
    g.setColour(c.brighter(.35f));
    g.drawEllipse(face.reduced(1), 1.5f);
    uiText(g, "A", {centre.x - 57, centre.y - 32, 48, 70}, 51,
           getToggleState() ? juce::Colour(0xff583a25) : juce::Colour(0xff102a3b),
           juce::Justification::centred);
    uiText(g, "|", {centre.x - 7, centre.y - 34, 14, 70}, 43, c.brighter(.4f),
           juce::Justification::centred);
    uiText(g, "B", {centre.x + 7, centre.y - 32, 48, 70}, 51,
           getToggleState() ? juce::Colour(0xff422a19) : juce::Colour(0xff46647a),
           juce::Justification::centred);
    uiText(g, "CHi", {centre.x - 30, centre.y + 51, 60, 22}, 13, juce::Colour(0xff10202c),
           juce::Justification::centred);
}
juce::TextButton &MetricEditor::button(const juce::String &id, const juce::String &label,
                                       juce::Rectangle<int> bounds, std::function<void()> action) {
    auto p = std::make_unique<ActionButton>(label);
    auto &b = *p;
    b.setComponentID(id);
    b.setBounds(bounds);
    b.onClick = std::move(action);
    b.setColour(juce::TextButton::buttonOnColourId, blue);
    panel.addAndMakeVisible(b);
    byID[id] = &b;
    controls.push_back(std::move(p));
    return b;
}
juce::ToggleButton &MetricEditor::toggle(const juce::String &id, const juce::String &label,
                                         juce::Rectangle<int> bounds) {
    auto p = std::make_unique<juce::ToggleButton>(label);
    auto &b = *p;
    b.setComponentID(id);
    b.setBounds(bounds);
    b.setColour(juce::TextButton::buttonOnColourId, amber);
    panel.addAndMakeVisible(b);
    byID[id] = &b;
    controls.push_back(std::move(p));
    buttonLinks.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.state, id, b));
    return b;
}
juce::Slider &MetricEditor::slider(const juce::String &id, juce::Rectangle<int> bounds,
                                   bool vertical) {
    auto p = std::make_unique<juce::Slider>();
    auto &s = *p;
    s.setComponentID(id);
    s.setBounds(bounds);
    s.setSliderStyle(vertical ? juce::Slider::LinearVertical : juce::Slider::LinearBar);
    s.setTextBoxStyle(vertical ? juce::Slider::TextBoxBelow : juce::Slider::TextBoxLeft, false,
                      vertical ? 45 : bounds.getWidth(), vertical ? 18 : bounds.getHeight());
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b2127));
    s.setColour(juce::Slider::textBoxOutlineColourId, edge);
    s.setColour(juce::Slider::textBoxTextColourId, silver);
    s.setColour(juce::Slider::trackColourId, juce::Colour(0xff67737d));
    panel.addAndMakeVisible(s);
    byID[id] = &s;
    controls.push_back(std::move(p));
    sliderLinks.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.state, id, s));
    return s;
}
juce::ComboBox &MetricEditor::combo(const juce::String &id, juce::Rectangle<int> bounds) {
    auto p = std::make_unique<juce::ComboBox>();
    auto &c = *p;
    c.setComponentID(id);
    c.setBounds(bounds);
    auto *parameter = dynamic_cast<juce::AudioParameterChoice *>(processor.state.getParameter(id));
    if (parameter)
        c.addItemList(parameter->choices, 1);
    c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff20262d));
    c.setColour(juce::ComboBox::outlineColourId, edge);
    c.setColour(juce::ComboBox::textColourId, silver);
    panel.addAndMakeVisible(c);
    byID[id] = &c;
    controls.push_back(std::move(p));
    comboLinks.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.state, id, c));
    return c;
}
MetricEditor::MetricEditor(MetricProcessor &p)
    : AudioProcessorEditor(p), panel(*this), processor(p) {
    setLookAndFeel(&style);
    panel.setComponentID("content");
    addAndMakeVisible(panel);
    setResizable(true, true);
    setResizeLimits(960, 576, 1920, 1152);
    getConstrainer()->setFixedAspectRatio(5.0 / 3.0);
    setSize(1200, 720);
    button("preset", "Studio Reference", {55, 47, 152, 27}, [this] {
        juce::PopupMenu m;
        m.addItem(1, "Load reference files...");
        m.addItem(2, "Reset analysis");
        m.addItem(3, "Reset A / B gains");
        m.showMenuAsync({}, [safe = juce::Component::SafePointer<MetricEditor>(this)](int id) {
            if (!safe || safe->closing)
                return;
            if (id == 1)
                safe->selectFile(safe->processor.selected());
            if (id == 2)
                safe->resetAnalysis();
            if (id == 3) {
                safe->processor.set("gainA", 0);
                safe->processor.set("gainB", 0);
            }
        });
    });
    button("size", "Resize", {212, 47, 48, 27}, [this] {
        juce::PopupMenu m;
        m.addItem(1, "Small (960 x 576)");
        m.addItem(2, "Normal (1200 x 720)");
        m.addItem(3, "Large (1440 x 864)");
        m.addItem(4, "Extra Large (1680 x 1008)");
        m.showMenuAsync({}, [safe = juce::Component::SafePointer<MetricEditor>(this)](int id) {
            if (safe && id > 0) {
                int w = 720 + id * 240;
                safe->setSize(w, w * 3 / 5);
            }
        });
    });
    toggle("bypass", "BYP", {15, 47, 35, 27});
    slider("gainA", {13, 128, 43, 205}, true).setTextValueSuffix(" dB");
    slider("gainB", {222, 128, 43, 205}, true).setTextValueSuffix(" dB");
    dynamic_cast<juce::Slider *>(byID["gainA"])->setDoubleClickReturnValue(true, 0);
    dynamic_cast<juce::Slider *>(byID["gainB"])->setDoubleClickReturnValue(true, 0);
    for (int i = 0; i < 5; ++i) {
        auto labels = juce::StringArray{"Stereo", "Mono", "L", "R", "Sides"};
        auto &b = button("monitor" + juce::String(i), labels[i],
                         {std::array<int, 5>{16, 61, 108, 173, 207}[static_cast<size_t>(i)], 344,
                          std::array<int, 5>{43, 45, 31, 30, 55}[static_cast<size_t>(i)], 25},
                         [this, i] { processor.set("stereo", static_cast<float>(i)); });
        b.setTooltip("Monitor " + labels[i]);
    }
    toggle("solo", "S", {142, 344, 28, 25})
        .setTooltip("Solo In Place modifies Left or Right monitoring");
    ab.setComponentID("ab");
    ab.setBounds(35, 394, 210, 210);
    ab.setTooltip("Switch monitoring between A (DAW mix) and B (reference)");
    panel.addAndMakeVisible(ab);
    byID["ab"] = &ab;
    buttonLinks.push_back(
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state, "ab", ab));
    button("load", "Load reference...", {22, 620, 236, 27}, [this] {
        if (processor.library.get(processor.selected())) {
            if (juce::ModifierKeys::getCurrentModifiersRealtime().isPopupMenu())
                slotMenu(processor.selected());
            else
                processor.set("ab", 1 - processor.value("ab"));
        } else
            selectFile(processor.selected());
    });
    const juce::StringArray tabNames{"Playback", "Spectrum", "Correlation", "Stereo Image",
                                     "Dynamics", "Loudness", "EQ Match"};
    int x = 285;
    for (int i = 0; i < 7; ++i) {
        int w = i == 3 ? 98 : 82;
        auto &b = button("page" + juce::String(i), tabNames[i], {x, 47, w, 49},
                         [this, i] { processor.set("tab", static_cast<float>(i)); });
        tabs[static_cast<size_t>(i)] = &b;
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffb9c4ce));
        if (i >= 5) {
            b.setEnabled(false);
            b.setTooltip(i == 5 ? "Standards-compliant loudness measurement is not implemented yet"
                                : "EQ Match processing is not implemented yet");
        }
        x += w + 3;
    }
    button("settings", "i", {1154, 50, 30, 30}, [this] {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, "CHi Metric Match Pro 0.2",
            "Implemented: reference RAM player, A/B, gains, channel monitoring, filters, FFT, "
            "frequency correlation, stereo balance and crest-factor history.\n\nNot implemented: "
            "LUFS/True Peak/PSR, EQ Match, disk streaming, PDC and full manual workflow. No "
            "simulated measurements are displayed.");
    });
    for (int i = 0; i < 3; ++i) {
        auto labels = juce::StringArray{"Single", "Dual", "Layered"};
        displays[static_cast<size_t>(i)] =
            &button("displayButton" + juce::String(i), labels[i], {296, 543 + i * 23, 96, 22},
                    [this, i] { processor.set(viewParameter(), static_cast<float>(i)); });
    }
    toggle("playing", ">", {402, 519, 33, 24}).setTooltip("Reference Play / Pause");
    button("reset", "R", {402, 546, 33, 24}, [this] {
        if (processor.value("tab") < .5f)
            processor.seekSeconds.store(0);
        else
            resetAnalysis();
    }).setTooltip("Reset analysis / rewind reference on Playback page");
    toggle("freeze", "Hold", {402, 573, 33, 24})
        .setTooltip("Freeze graphical analysis without stopping audio");
    combo("fft", {512, 519, 80, 23});
    toggle("peak", "Peak", {447, 550, 62, 23});
    toggle("hold", "Hold", {513, 550, 62, 23});
    toggle("average", "Average", {579, 550, 70, 23});
    slider("holdTime", {513, 577, 62, 22}).setTextValueSuffix(" s");
    slider("averageTime", {579, 577, 70, 22}).setTextValueSuffix(" ms");
    slider("peakDecay", {447, 577, 62, 22}).setTextValueSuffix(" dB/s");
    toggle("zoomDb", "Zoom dB", {661, 519, 76, 23});
    toggle("zoomHz", "Zoom Hz", {743, 519, 76, 23});
    slider("zoomHigh", {743, 549, 76, 23}).setTextValueSuffix(" Hz");
    slider("zoomLow", {743, 576, 76, 23}).setTextValueSuffix(" Hz");
    slider("dbTop", {661, 549, 76, 23}).setTextValueSuffix(" dB");
    slider("dbBottom", {661, 576, 76, 23}).setTextValueSuffix(" dB");
    toggle("filters", "Filters", {825, 519, 81, 23});
    combo("slope", {912, 519, 76, 23});
    combo("filterMode", {994, 519, 184, 23});
    slider("high", {825, 549, 81, 23}).setTextValueSuffix(" Hz");
    slider("low", {825, 576, 81, 23}).setTextValueSuffix(" Hz");
    const juce::StringArray names{"Low Mid", "Mid", "High", "Sub", "Bass", "Spot"};
    const float lows[]{250, 800, 4000, 20, 80, 79}, highs[]{800, 4000, 20000, 80, 250, 317};
    for (int i = 0; i < 6; ++i)
        button("band" + juce::String(i), names[i], {916 + (i % 3) * 85, 549 + (i / 3) * 27, 81, 23},
               [this, i, l = lows[i], h = highs[i]] {
                   processor.set("low", l);
                   processor.set("high", h);
                   processor.set("filterMode", 1);
                   processor.set("filters", 1);
               });
    combo("playMode", {447, 549, 98, 25});
    toggle("loop", "Loop", {553, 549, 80, 25}).setTooltip("Loop the entire reference track");
    for (int i = 0; i < 4; ++i)
        cueButtons[static_cast<size_t>(i)] = &button(
            "cueButton" + juce::String(i), juce::String(i + 1), {296 + i * 24, 519, 22, 20},
            [this, i] {
                processor.cue(i, juce::ModifierKeys::getCurrentModifiersRealtime().isPopupMenu());
            });
    for (int i = 0; i < 16; ++i) {
        const int column = i / 4, row = i % 4;
        auto &b = button("slot" + juce::String(i), juce::String(i + 1) + ". Empty",
                         {286 + column * 224, 617 + row * 21, 221, 20}, [this, i] {
                             auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
                             auto *component = slots[static_cast<size_t>(i)];
                             if (mods.isPopupMenu() ||
                                 component->getMouseXYRelative().x > component->getWidth() - 22)
                                 slotMenu(i);
                             else if (processor.library.get(i))
                                 processor.choose(i, mods.isShiftDown());
                             else
                                 selectFile(i);
                         });
        slots[static_cast<size_t>(i)] = &b;
        b.setColour(juce::TextButton::buttonOnColourId, amber);
    }
    configureFFT();
    timerCallback();
    startTimerHz(30);
}
MetricEditor::~MetricEditor() {
    closing = true;
    stopTimer();
    chooser.reset();
    setLookAndFeel(nullptr);
}
void MetricEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xff171b20));
}
void MetricEditor::resized() {
    panel.setBounds(0, 0, 1200, 720);
    panel.setTransform(juce::AffineTransform::scale(getWidth() / 1200.0f, getHeight() / 720.0f));
}
int MetricEditor::getControlParameterIndex(juce::Component &component) {
    if (!component.isEnabled())
        return -1;
    for (auto *current = &component; current && current != &panel;
         current = current->getParentComponent()) {
        juce::String id = current->getComponentID();
        if (id.startsWith("monitor"))
            id = "stereo";
        else if (id.startsWith("page"))
            id = "tab";
        else if (id.startsWith("slot"))
            id = "reference";
        else if (id.startsWith("cueButton"))
            id = "cue";
        else if (id.startsWith("displayButton"))
            id = viewParameter();
        if (auto *p = processor.state.getParameter(id))
            return p->getParameterIndex();
    }
    return -1;
}
void MetricEditor::Panel::filesDropped(const juce::StringArray &files, int x, int y) {
    int slot = owner.processor.selected();
    if (y >= 617 && x >= 286)
        slot = juce::jlimit(0, 15, ((x - 286) / 224) * 4 + (y - 617) / 21);
    owner.loadFiles(files, slot);
}
void MetricEditor::Panel::mouseWheelMove(const juce::MouseEvent &e,
                                         const juce::MouseWheelDetails &d) {
    if (!juce::Rectangle<float>(286, 104, 896, 395).contains(e.position))
        return;
    if (owner.processor.value("tab") < .5f) {
        owner.waveZoom = juce::jlimit(1.0f, 32.0f, owner.waveZoom * std::pow(1.3f, d.deltaY * 5));
    } else {
        float lo = owner.processor.value("low"), hi = owner.processor.value("high"),
              centre = std::sqrt(lo * hi),
              ratio = juce::jlimit(1.02f, 1000.0f, (hi / lo) * std::pow(1.3f, -d.deltaY * 5));
        owner.processor.set("low", juce::jlimit(20.0f, 20000.0f, centre / std::sqrt(ratio)));
        owner.processor.set("high", juce::jlimit(20.0f, 20000.0f, centre * std::sqrt(ratio)));
    }
    repaint();
}
void MetricEditor::loadFiles(const juce::StringArray &files, int start) {
    for (int i = 0; i < files.size() && start + i < 16; ++i)
        processor.library.load(start + i, juce::File(files[i]));
}
void MetricEditor::selectFile(int slot) {
    chooser = std::make_unique<juce::FileChooser>("Load reference audio", juce::File{},
                                                  "*.wav;*.aif;*.aiff;*.flac");
    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                             juce::FileBrowserComponent::canSelectFiles |
                             juce::FileBrowserComponent::canSelectMultipleItems,
                         [safe = juce::Component::SafePointer<MetricEditor>(this),
                          slot](const juce::FileChooser &fc) {
                             if (!safe || safe->closing)
                                 return;
                             juce::StringArray files;
                             for (auto f : fc.getResults())
                                 files.add(f.getFullPathName());
                             safe->loadFiles(files, slot);
                             if (!files.isEmpty()) {
                                 safe->processor.set("reference", static_cast<float>(slot));
                                 safe->processor.set("ab", 1);
                                 safe->processor.set("playing", 1);
                             }
                         });
}
void MetricEditor::slotMenu(int slot) {
    juce::PopupMenu menu;
    menu.addItem(1, "Load Audio File...");
    menu.addItem(2, "Show in Explorer / file manager", processor.library.get(slot) != nullptr);
    menu.addItem(3, "Remove Audio File", processor.library.get(slot) != nullptr);
    menu.addItem(4, "Clear cue settings");
    menu.showMenuAsync({}, [safe = juce::Component::SafePointer<MetricEditor>(this), slot](int id) {
        if (!safe || safe->closing)
            return;
        if (id == 1)
            safe->selectFile(slot);
        if (id == 2)
            if (auto *t = safe->processor.library.get(slot))
                juce::File(t->path).revealToUser();
        if (id == 3)
            safe->processor.library.clear(slot);
        if (id == 4)
            for (auto &cue : safe->processor.cues[static_cast<size_t>(slot)])
                cue.store(-1);
    });
}
void MetricEditor::graphMouse(const juce::MouseEvent &e, bool drag) {
    if (!drag && e.position.x > 114 && e.position.x < 160 && e.position.y > 137 &&
        e.position.y < 323 && e.getNumberOfClicks() >= 2) {
        processor.set("meterZoom", 1 - processor.value("meterZoom"));
        return;
    }
    if (!drag && e.position.y > 100 && e.position.y < 130) {
        for (int s = 0; s < 2; ++s)
            for (int c = 0; c < 2; ++c) {
                float x = (s == 0 ? 61.f : 164.f) + c * 25;
                if (e.position.x >= x - 10 && e.position.x < x + 24)
                    meterHold[static_cast<size_t>(s)][static_cast<size_t>(c)] = 0;
            }
    }
    auto r = juce::Rectangle<float>(287, 108, 900, 391);
    if (!r.contains(e.position))
        return;
    if (processor.value("tab") < .5f) {
        if (auto *track = processor.library.get(processor.selected())) {
            const double normal = juce::jlimit(
                0.0, 1.0, static_cast<double>((e.position.x - r.getX()) / r.getWidth()));
            if (e.position.y < 190) {
                zoomCentre = static_cast<float>(normal);
            } else
                processor.seekSeconds.store(
                    juce::jlimit(0.0, track->duration(),
                                 (juce::jlimit(0.0, 1.0 - 1.0 / waveZoom,
                                               static_cast<double>(zoomCentre) - .5 / waveZoom) +
                                  normal / waveZoom) *
                                     track->duration()));
        }
        return;
    }
    if (e.mods.isPopupMenu()) {
        if (!drag)
            processor.set("filters", 1 - processor.value("filters"));
        return;
    }
    if (e.mods.isAltDown()) {
        processor.set("low", 20);
        processor.set("high", 20000);
        return;
    }
    const int page = juce::roundToInt(processor.value("tab"));
    if (page == 4)
        return;
    auto plot = r.reduced(12, 10).withTrimmedBottom(18);
    const float normalized =
        juce::jlimit(0.0f, 1.0f,
                     page == 3 ? (plot.getBottom() - e.position.y) / plot.getHeight()
                               : (e.position.x - plot.getX()) / plot.getWidth());
    const float centre = minHz() * std::pow(maxHz() / minHz(), normalized);
    float ratio = drag ? processor.value("high") / juce::jmax(20.0f, processor.value("low")) : 4;
    processor.set("low", juce::jlimit(20.0f, 20000.0f, centre / std::sqrt(ratio)));
    processor.set("high", juce::jlimit(20.0f, 20000.0f, centre * std::sqrt(ratio)));
    processor.set("filterMode", 1);
    processor.set("filters", 1);
}
void MetricEditor::configureFFT() {
    const int requested = 1 << (11 + juce::roundToInt(processor.value("fft")));
    if (requested == fftSize)
        return;
    fftSize = requested;
    fft = std::make_unique<juce::dsp::FFT>(11 + juce::roundToInt(processor.value("fft")));
    window = std::make_unique<juce::dsp::WindowingFunction<float>>(
        static_cast<size_t>(fftSize), juce::dsp::WindowingFunction<float>::hann, false);
    for (auto &data : frame)
        data.assign(static_cast<size_t>(fftSize * 2), 0);
    for (auto &data : fftWork)
        data.assign(static_cast<size_t>(fftSize * 2), 0);
    for (int s = 0; s < 2; ++s) {
        for (auto &curve : curves[static_cast<size_t>(s)])
            curve.assign(static_cast<size_t>(fftSize / 2 + 1), -120);
        held[static_cast<size_t>(s)].assign(static_cast<size_t>(fftSize / 2 + 1), -120);
        holdAge[static_cast<size_t>(s)].assign(static_cast<size_t>(fftSize / 2 + 1), 0);
        powerAverage[static_cast<size_t>(s)].assign(static_cast<size_t>(fftSize / 2 + 1), 0);
        pan[static_cast<size_t>(s)].assign(static_cast<size_t>(fftSize / 2 + 1), 0);
        phase[static_cast<size_t>(s)].assign(static_cast<size_t>(fftSize / 2 + 1), 0);
    }
    framePosition = 0;
    analyzedFrames = 0;
}
void MetricEditor::resetAnalysis() {
    for (auto &a : meterHold)
        a.fill(0);
    for (auto &h : history)
        h.fill(0);
    historyPosition = 0;
    fftSize = 0;
    configureFFT();
    panel.repaint();
}
void MetricEditor::transformFrame() {
    for (size_t c = 0; c < 4; ++c) {
        auto &data = fftWork[c];
        std::copy_n(frame[c].begin(), fftSize, data.begin());
        window->multiplyWithWindowingTable(data.data(), static_cast<size_t>(fftSize));
        std::fill(data.begin() + fftSize, data.end(), 0);
        fft->performRealOnlyForwardTransform(data.data(), true);
    }
    const float dt = static_cast<float>(juce::jmin(2048, fftSize / 4) /
                                        processor.currentRate.load()),
                time = processor.value("averageTime") * .001f,
                alpha = time <= 0 ? 1.0f / static_cast<float>(++analyzedFrames)
                                  : 1 - std::exp(-dt / time);
    for (int s = 0; s < 2; ++s)
        for (int i = 0; i <= fftSize / 2; ++i) {
            const auto bin = static_cast<size_t>(i), j = static_cast<size_t>(i * 2);
            auto &l = fftWork[static_cast<size_t>(s * 2)];
            auto &r = fftWork[static_cast<size_t>(s * 2 + 1)];
            const float pl = l[j] * l[j] + l[j + 1] * l[j + 1],
                        pr = r[j] * r[j] + r[j + 1] * r[j + 1],
                        normal = i == 0 ? 2.0f / fftSize : 4.0f / fftSize;
            const float power = (pl + pr) * .5f * normal * normal;
            auto &current = curves[static_cast<size_t>(s)][0][bin];
            current = juce::jlimit(-120.0f, 36.0f, 10 * std::log10(juce::jmax(1e-12f, power)));
            auto &mean = powerAverage[static_cast<size_t>(s)][bin];
            mean += alpha * (power - mean);
            curves[static_cast<size_t>(s)][1][bin] = 10 * std::log10(juce::jmax(1e-12f, mean));
            auto &hold = held[static_cast<size_t>(s)][bin];
            auto &age = holdAge[static_cast<size_t>(s)][bin];
            if (current >= hold) {
                hold = current;
                age = 0;
            } else {
                age += dt;
                if (age > processor.value("holdTime"))
                    hold = juce::jmax(current, hold - dt * processor.value("peakDecay"));
            }
            const float balance =
                (std::sqrt(pr) - std::sqrt(pl)) / (std::sqrt(pr) + std::sqrt(pl) + 1e-12f);
            pan[static_cast<size_t>(s)][bin] +=
                alpha * (balance - pan[static_cast<size_t>(s)][bin]);
            const float correlation =
                (l[j] * r[j] + l[j + 1] * r[j + 1]) / (std::sqrt(pl) * std::sqrt(pr) + 1e-12f);
            phase[static_cast<size_t>(s)][bin] += alpha * (juce::jlimit(-1.0f, 1.0f, correlation) -
                                                           phase[static_cast<size_t>(s)][bin]);
        }
}
void MetricEditor::timerCallback() {
    configureFFT();
    for (int s = 0; s < 2; ++s)
        for (int c = 0; c < 2; ++c) {
            const float p = s == 0 ? processor.peakA[c].load() : processor.peakB[c].load(),
                        r = s == 0 ? processor.rmsA[c].load() : processor.rmsB[c].load();
            meterPeak[static_cast<size_t>(s)][static_cast<size_t>(c)] =
                juce::jmax(p, meterPeak[static_cast<size_t>(s)][static_cast<size_t>(c)] * .88f);
            meterRms[static_cast<size_t>(s)][static_cast<size_t>(c)] = r;
            meterHold[static_cast<size_t>(s)][static_cast<size_t>(c)] =
                juce::jmax(p, meterHold[static_cast<size_t>(s)][static_cast<size_t>(c)]);
        }
    int start1 = 0, n1 = 0, start2 = 0, n2 = 0;
    processor.analysisFifo.prepareToRead(processor.analysisFifo.getNumReady(), start1, n1, start2,
                                         n2);
    const bool freeze = processor.value("freeze") > .5f;
    for (int n = 0; n < n1 + n2; ++n)
        if (!freeze) {
            int source = n < n1 ? start1 + n : start2 + n - n1;
            for (size_t c = 0; c < 4; ++c)
                frame[c][static_cast<size_t>(framePosition)] =
                    processor.analysis[c][static_cast<size_t>(source)];
            if (++framePosition == fftSize) {
                transformFrame();
                const int hop = juce::jmin(2048, fftSize / 4);
                for (auto &data : frame)
                    std::memmove(data.data(), data.data() + hop,
                                 static_cast<size_t>(fftSize - hop) * sizeof(float));
                framePosition = fftSize - hop;
            }
        }
    processor.analysisFifo.finishedRead(n1 + n2);
    for (int s = 0; s < 2; ++s) {
        const float peak = juce::jmax(meterPeak[static_cast<size_t>(s)][0],
                                      meterPeak[static_cast<size_t>(s)][1]),
                    rms = std::sqrt((std::pow(meterRms[static_cast<size_t>(s)][0], 2) +
                                     std::pow(meterRms[static_cast<size_t>(s)][1], 2)) *
                                    .5f);
        history[static_cast<size_t>(s)][static_cast<size_t>(historyPosition % 512)] =
            rms > 1e-6f ? juce::jlimit(0.0f, 36.0f, db(peak) - db(rms)) : 0;
    }
    if (!freeze)
        ++historyPosition;
    const int tab = juce::roundToInt(processor.value("tab"));
    for (const auto *id : {"peak", "hold", "holdTime", "peakDecay", "dbTop", "dbBottom", "zoomDb"})
        byID[id]->setEnabled(tab == 1);
    for (const auto *id : {"average", "averageTime", "fft", "zoomHz", "zoomLow", "zoomHigh"})
        byID[id]->setEnabled(tab >= 1 && tab <= 3);
    for (const auto *id : {"fft", "peak", "hold", "average", "holdTime", "averageTime", "peakDecay",
                           "zoomDb", "dbTop", "dbBottom", "zoomHz", "zoomHigh", "zoomLow"})
        byID[id]->setVisible(tab != 0);
    byID["playMode"]->setVisible(tab == 0);
    byID["loop"]->setVisible(tab == 0);
    for (auto *cue : cueButtons)
        cue->setVisible(tab == 0);
    for (auto *display : displays)
        display->setEnabled(tab != 0);
    if (auto *b = dynamic_cast<juce::TextButton *>(byID["load"]))
        b->setButtonText(processor.library.get(processor.selected())
                             ? processor.library.get(processor.selected())->name
                             : "Load reference...");
    for (int i = 0; i < 7; ++i)
        tabs[static_cast<size_t>(i)]->setToggleState(i == tab, juce::dontSendNotification);
    for (int i = 0; i < 3; ++i)
        displays[static_cast<size_t>(i)]->setToggleState(
            i == juce::roundToInt(processor.value(viewParameter())), juce::dontSendNotification);
    for (int i = 0; i < 5; ++i)
        dynamic_cast<juce::Button *>(byID["monitor" + juce::String(i)])
            ->setToggleState(i == juce::roundToInt(processor.value("stereo")),
                             juce::dontSendNotification);
    for (int i = 0; i < 16; ++i) {
        auto *track = processor.library.get(i);
        auto *b = slots[static_cast<size_t>(i)];
        b->setButtonText(juce::String(i + 1) + ". " + (track ? track->name : "Empty") + "   =");
        b->setToggleState(i == processor.selected(), juce::dontSendNotification);
        b->setTooltip(track ? track->path : "Click to load or drop an audio file");
    }
    for (int i = 0; i < 4; ++i) {
        bool present =
            processor.cues[static_cast<size_t>(processor.selected())][static_cast<size_t>(i)]
                .load() >= 0;
        cueButtons[static_cast<size_t>(i)]->setToggleState(present, juce::dontSendNotification);
    }
    lastTab = tab;
    lastSelected = processor.selected();
    panel.repaint();
}
void MetricEditor::drawGraph(juce::Graphics &g, juce::Rectangle<float> area, int stream, int type,
                             bool background) {
    juce::Graphics::ScopedSaveState clip(g);
    g.reduceClipRegion(area.toNearestInt());
    if (background) {
        g.setColour(juce::Colour(0xff0b0f13));
        g.fillRect(area);
    }
    auto plot = area.reduced(12, 10).withTrimmedBottom(18);
    const float top = type == 4 ? 36
                                : (processor.value("zoomDb") > .5f ? processor.value("dbTop") : 0),
                bottom = type == 4 ? 0
                                   : (processor.value("zoomDb") > .5f
                                          ? juce::jmin(top - 12, processor.value("dbBottom"))
                                          : -96);
    const auto colour = stream == 0 ? blue : amber;
    const double rate = processor.currentRate.load();
    if (type == 3) {
        if (processor.value("average") < .5f)
            return;
        for (float f :
             {20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f}) {
            float y = plot.getBottom() -
                      std::log(f / minHz()) / std::log(maxHz() / minHz()) * plot.getHeight();
            g.setColour(juce::Colour(0xff28323c));
            g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
            uiText(g, f >= 1000 ? juce::String(f / 1000, 0) + "k" : juce::String(f, 0),
                   {plot.getRight() - 30, y - 10, 30, 16}, 9);
        }
        g.setColour(juce::Colour(0xff687583));
        g.drawVerticalLine(static_cast<int>(plot.getCentreX()), plot.getY(), plot.getBottom());
        juce::Path path;
        for (int n = 0; n < 400; ++n) {
            const float f = minHz() * std::pow(maxHz() / minHz(), n / 399.0f);
            int bin = juce::jlimit(0, fftSize / 2, juce::roundToInt(f * fftSize / rate));
            float x = plot.getCentreX() +
                      pan[static_cast<size_t>(stream)][static_cast<size_t>(bin)] * plot.getWidth() *
                          .48f,
                  y = plot.getBottom() - n / 399.0f * plot.getHeight();
            if (n == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(1.5f));
        uiText(g, "LEFT", {plot.getX(), plot.getBottom() + 3, 80, 15}, 9);
        uiText(g, "RIGHT", {plot.getRight() - 80, plot.getBottom() + 3, 80, 15}, 9, silver,
               juce::Justification::centredRight);
        return;
    }
    for (int d = static_cast<int>(top); d >= static_cast<int>(bottom); d -= 6) {
        float y = plot.getY() + (top - d) / (top - bottom) * plot.getHeight();
        g.setColour(juce::Colour(0xff26303a).withAlpha(d % 12 == 0 ? 1.0f : .5f));
        g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
        if ((type == 1 || type == 4) && d % 12 == 0)
            uiText(g, juce::String(d), {plot.getRight() - 28, y - 8, 28, 14}, 8);
    }
    for (float f : {20.f, 40.f, 80.f, 200.f, 400.f, 800.f, 2000.f, 4000.f, 8000.f, 20000.f}) {
        float x =
            plot.getX() + std::log(f / minHz()) / std::log(maxHz() / minHz()) * plot.getWidth();
        g.setColour(juce::Colour(0xff35414c));
        g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
        uiText(g,
               type == 4 ? juce::String((x - plot.getX()) / plot.getWidth() * 512.0 / 30.0, 0) + "s"
                         : (f >= 1000 ? juce::String(f / 1000, 0) + "k" : juce::String(f, 0)),
               {x - 15, plot.getBottom() + 4, 32, 15}, 9, juce::Colour(0xffa0aab4),
               juce::Justification::centred);
    }
    if (type == 4) {
        juce::Path path;
        const int count = juce::jmin(512, historyPosition);
        for (int i = 0; i < count; ++i) {
            float x = plot.getX() + i / 511.f * plot.getWidth(),
                  value = history[static_cast<size_t>(stream)]
                                 [static_cast<size_t>((historyPosition - count + i) % 512)],
                  y = plot.getBottom() - value / 36 * plot.getHeight();
            if (i == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(1.5f));
        uiText(g, "CREST FACTOR (Peak / RMS) - not PSR", plot.withHeight(25), 12, silver);
        return;
    }
    auto makePath = [&](const std::vector<float> &data, bool correlation) {
        juce::Path path;
        for (int n = 0; n < 650; ++n) {
            const float f = minHz() * std::pow(maxHz() / minHz(), n / 649.0f);
            const int bin = juce::jlimit(0, fftSize / 2, juce::roundToInt(f * fftSize / rate));
            float value = data[static_cast<size_t>(bin)];
            float y = correlation ? plot.getCentreY() - value * plot.getHeight() * .5f
                                  : plot.getY() + (top - value) / (top - bottom) * plot.getHeight();
            y = juce::jlimit(plot.getY(), plot.getBottom(), y);
            float x = plot.getX() + n / 649.f * plot.getWidth();
            if (n == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }
        return path;
    };
    if (type == 2) {
        if (processor.value("average") < .5f)
            return;
        auto path = makePath(phase[static_cast<size_t>(stream)], true);
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(1.5f));
        g.setColour(juce::Colours::grey);
        g.drawHorizontalLine(static_cast<int>(plot.getCentreY()), plot.getX(), plot.getRight());
        uiText(g, "+1", {plot.getRight() - 25, plot.getY(), 25, 15}, 10);
        uiText(g, "-1", {plot.getRight() - 25, plot.getBottom() - 15, 25, 15}, 10);
        return;
    }
    if (processor.value("average") > .5f) {
        auto path = makePath(curves[static_cast<size_t>(stream)][1], false);
        auto fill = path;
        fill.lineTo(plot.getRight(), plot.getBottom());
        fill.lineTo(plot.getX(), plot.getBottom());
        fill.closeSubPath();
        g.setGradientFill(juce::ColourGradient(colour.withAlpha(.45f), 0, plot.getY(),
                                               colour.withAlpha(.05f), 0, plot.getBottom(), false));
        g.fillPath(fill);
        g.setColour(colour.withAlpha(.8f));
        g.strokePath(path, juce::PathStrokeType(1));
    }
    if (processor.value("peak") > .5f) {
        auto path = makePath(processor.value("hold") > .5f ? held[static_cast<size_t>(stream)]
                                                           : curves[static_cast<size_t>(stream)][0],
                             false);
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(1.4f));
    }
}
void MetricEditor::draw(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xff181d23));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff373f48), 0, 0, juce::Colour(0xff181e25),
                                           1200, 720, false));
    g.fillRect(0, 0, 1200, 720);
    box(g, {2, 2, 1196, 34}, juce::Colour(0xff262e37));
    uiText(g, "CHi METRIC MATCH PRO", {18, 6, 310, 25}, 21);
    uiText(g, "Professional Audio Reference Comparison", {338, 7, 540, 24}, 12,
           juce::Colour(0xffa0aab4));
    uiText(g, "0.2", {1145, 7, 40, 24}, 11);
    box(g, {7, 39, 267, 672}, juce::Colour(0xff272f37));
    box(g, {282, 39, 910, 61}, juce::Colour(0xff242b33));
    box(g, {13, 83, 254, 251}, juce::Colour(0xff141b21));
    for (int s = 0; s < 2; ++s) {
        const auto c = s == 0 ? blue : amber;
        for (int channel = 0; channel < 2; ++channel) {
            float x = (s == 0 ? 61.f : 164.f) + channel * 25;
            const auto index = static_cast<size_t>(s), ch = static_cast<size_t>(channel);
            uiText(g, juce::String(s == 0 ? "A " : "B ") + (channel == 0 ? "L" : "R"),
                   {x - 5, 88, 35, 14}, 9, silver, juce::Justification::centred);
            uiText(g, "Pk " + level(meterHold[index][ch]), {x - 10, 103, 46, 13}, 8, silver,
                   juce::Justification::centred);
            uiText(g, "RMS " + level(meterRms[index][ch]), {x - 12, 116, 50, 13}, 8, silver,
                   juce::Justification::centred);
            g.setColour(juce::Colour(0xff080c10));
            g.fillRoundedRectangle(x, 137, 23, 186, 2);
            for (int segment = 0; segment < 48; ++segment) {
                float y = 319 - segment * 3.78f;
                const float range = processor.value("meterZoom") > .5f ? 18.f : 48.f;
                bool lit = db(meterPeak[index][ch]) >= -range + segment * range / 48;
                g.setColour(
                    (segment > 45 ? juce::Colours::indianred : c).withAlpha(lit ? .95f : .075f));
                g.fillRect(x + 1, y, 21.0f, 2.8f);
            }
        }
    }
    const float meterRange = processor.value("meterZoom") > .5f ? 18.f : 48.f;
    for (int d = 0; d >= -meterRange; d -= 6)
        uiText(g, juce::String(d), {115, 131 + (-d) / meterRange * 182, 45, 14}, 9,
               juce::Colour(0xff9aa7b2), juce::Justification::centred);
    uiText(g, processor.value("ab") > .5f ? "B / REFERENCE" : "A / DAW MIX", {24, 377, 232, 17}, 11,
           processor.value("ab") > .5f ? amber : blue, juce::Justification::centred);
    box(g, {22, 608, 236, 10}, juce::Colour(0xff11181e));
    uiText(g, "CHi Audio", {20, 665, 130, 28}, 24);
    uiText(g, "REFERENCE MONITOR", {157, 675, 102, 15}, 8, juce::Colour(0xffa0aab4));
    const auto *track = processor.library.get(processor.selected());
    uiText(g, track ? track->name : "No reference loaded", {895, 48, 251, 19}, 13);
    uiText(g,
           track ? clockTime(processor.position.load()) + " / " + clockTime(track->duration())
                 : "Drop an audio file below",
           {895, 70, 250, 18}, 10, juce::Colour(0xffb2bbc4));
    box(g, {282, 103, 910, 402}, juce::Colour(0xff0b1015));
    const int type = juce::roundToInt(processor.value("tab")),
              display = juce::roundToInt(processor.value(viewParameter()));
    auto graph = juce::Rectangle<float>(287, 108, 900, 391);
    if (type == 0) {
        g.setColour(juce::Colour(0xff0c1116));
        g.fillRect(graph);
        if (track) {
            for (int section = 0; section < 2; ++section) {
                auto wave = section == 0 ? graph.withHeight(76)
                                         : graph.withTrimmedTop(93).withTrimmedBottom(25);
                const float start = section == 0 ? 0
                                                 : juce::jlimit(0.0f, 1.0f - 1 / waveZoom,
                                                                zoomCentre - .5f / waveZoom),
                            extent = section == 0 ? 1 : 1 / waveZoom;
                for (int i = 0; i < static_cast<int>(wave.getWidth()); ++i) {
                    const int bin = juce::jlimit(
                        0, 1023, static_cast<int>((start + i / wave.getWidth() * extent) * 1023));
                    const float amplitude =
                        juce::jmin(1.0f, track->waveform[static_cast<size_t>(bin)]) *
                        wave.getHeight() * .46f;
                    g.setColour(amber.withAlpha(section == 0 ? .65f : .8f));
                    g.drawVerticalLine(static_cast<int>(wave.getX() + i),
                                       wave.getCentreY() - amplitude,
                                       wave.getCentreY() + amplitude);
                }
                const float play =
                    static_cast<float>(processor.position.load() / track->duration());
                if (play >= start && play <= start + extent) {
                    g.setColour(silver);
                    g.drawVerticalLine(
                        static_cast<int>(wave.getX() + (play - start) / extent * wave.getWidth()),
                        wave.getY(), wave.getBottom());
                }
                for (int cue = 0; cue < 4; ++cue) {
                    const double t = processor
                                         .cues[static_cast<size_t>(processor.selected())]
                                              [static_cast<size_t>(cue)]
                                         .load();
                    if (t >= 0) {
                        const float normal = static_cast<float>(t / track->duration());
                        if (normal >= start && normal <= start + extent) {
                            float x = wave.getX() + (normal - start) / extent * wave.getWidth();
                            g.setColour(amber);
                            g.drawVerticalLine(static_cast<int>(x), wave.getY(), wave.getBottom());
                            uiText(g, juce::String(cue + 1), {x, wave.getY(), 15, 15}, 10, amber);
                        }
                    }
                }
            }
            uiText(g,
                   juce::String(track->rate, 0) + " Hz   /   RAM   /   " +
                       clockTime(track->duration()),
                   graph.withTrimmedTop(graph.getHeight() - 20), 10, silver,
                   juce::Justification::centredRight);
        } else
            uiText(g, "Drop a reference audio file here or select an empty slot", graph, 17, silver,
                   juce::Justification::centred);
    } else if (display == 1) {
        if (type == 3) {
            auto left = graph.withWidth(graph.getWidth() * .5f);
            drawGraph(g, left, 0, type);
            drawGraph(g, left.translated(left.getWidth(), 0), 1, type);
        } else {
            auto top = graph.withHeight(graph.getHeight() * .5f);
            drawGraph(g, top, 0, type);
            drawGraph(g, top.translated(0, top.getHeight()), 1, type);
        }
    } else {
        const int selected = processor.value("ab") > .5f ? 1 : 0;
        if (display == 2) {
            drawGraph(g, graph, 1 - selected, type);
            drawGraph(g, graph, selected, type, false);
        } else
            drawGraph(g, graph, selected, type);
    }

    if (type >= 1 && type <= 3 && processor.value("filters") > .5f) {
        const float low = juce::jmin(processor.value("low"), processor.value("high"));
        const float high = juce::jmax(processor.value("low"), processor.value("high"));
        const int region = juce::roundToInt(processor.value("filterMode"));
        const int panels = display == 1 ? 2 : 1;
        for (int i = 0; i < panels; ++i) {
            auto area = graph;
            if (panels == 2) {
                if (type == 3)
                    area = graph.withWidth(graph.getWidth() * .5f)
                               .translated(i * graph.getWidth() * .5f, 0);
                else
                    area = graph.withHeight(graph.getHeight() * .5f)
                               .translated(0, i * graph.getHeight() * .5f);
            }
            auto plot = area.reduced(12, 10).withTrimmedBottom(18);
            float l = juce::jlimit(0.f, 1.f, std::log(low / minHz()) / std::log(maxHz() / minHz()));
            float h =
                juce::jlimit(0.f, 1.f, std::log(high / minHz()) / std::log(maxHz() / minHz()));
            if (region == 0) {
                h = l;
                l = 0;
            } else if (region == 2) {
                l = h;
                h = 1;
            }
            auto band =
                type == 3
                    ? juce::Rectangle<float>(plot.getX(), plot.getBottom() - h * plot.getHeight(),
                                             plot.getWidth(), (h - l) * plot.getHeight())
                    : juce::Rectangle<float>(plot.getX() + l * plot.getWidth(), plot.getY(),
                                             (h - l) * plot.getWidth(), plot.getHeight());
            g.setColour(silver.withAlpha(.07f));
            g.fillRect(band);
            g.setColour(silver.withAlpha(.4f));
            g.drawRect(band, 1.f);
        }
    }
    uiText(g, "A MIX", {302, 115, 70, 16}, 10, blue);
    uiText(g, "B REFERENCE", {374, 115, 130, 16}, 10, amber);
    box(g, {282, 510, 910, 96}, juce::Colour(0xff29313a));
    uiText(g, type == 0 ? "CUES" : "DISPLAY", {296, type == 0 ? 502.f : 519.f, 96, 17}, 10);
    uiText(g, type == 0 ? "PLAYBACK" : "ANALYSIS", {447, 519, 66, 22}, 10);
    box(g, {282, 612, 910, 91}, juce::Colour(0xff10171d));
    uiText(g, processor.library.status(), {289, 703, 895, 15}, 9, juce::Colour(0xffa4afb8));
}
