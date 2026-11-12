// Independent legacy ABI adapter using the BSD-licensed clean-room header.
#include "PluginProcessor.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#define __cdecl
#endif
#include "../ThirdParty/LegacyABI/vst.h"
#include <cstring>
namespace {
struct HostTime {
    double samples, rate, nanos, ppq, tempo, bar, loopStart, loopEnd;
    int32_t numerator, denominator, smpteOffset, smpteRate, nextClock, flags;
};
class Legacy final : private juce::AudioProcessorParameter::Listener,
                     private juce::ComponentListener {
    struct PlayHead final : juce::AudioPlayHead {
        Legacy &owner;
        explicit PlayHead(Legacy &o) : owner(o) {}
        juce::Optional<PositionInfo> getPosition() const override {
            auto *time = reinterpret_cast<const HostTime *>(
                owner.host(&owner.effect, VST_HOST_OPCODE_07, 0, 0x3f00, nullptr, 0));
            if (!time || time->rate <= 0)
                return {};
            PositionInfo p;
            p.setIsPlaying((time->flags & 2) != 0);
            p.setIsRecording((time->flags & 8) != 0);
            p.setIsLooping((time->flags & 4) != 0);
            p.setTimeInSamples(static_cast<juce::int64>(time->samples));
            p.setTimeInSeconds(time->samples / time->rate);
            if (time->flags & 0x400)
                p.setPpqPosition(time->ppq);
            if (time->flags & 0x200)
                p.setBpm(time->tempo);
            return p;
        }
    } playhead;
    juce::ScopedJuceInitialiser_GUI initialise;
    MetricProcessor processor;
    vst_host_callback host;
    juce::MemoryBlock chunk;
    juce::MidiBuffer midi;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    vst_rect rect{0, 0, 720, 1200};
    double rate = 44100;
    int block = 512;
    bool prepared = false;
    static thread_local Legacy *updating;
    void parameterValueChanged(int index, float value) override {
        if (updating != this)
            host(&effect, VST_HOST_OPCODE_00, index, 0, nullptr, value);
    }
    void parameterGestureChanged(int index, bool starting) override {
        host(&effect, starting ? VST_HOST_OPCODE_2B : VST_HOST_OPCODE_2C, index, 0, nullptr, 0);
    }
    void componentMovedOrResized(juce::Component &, bool, bool resized) override {
        if (resized && editor) {
            rect.right = static_cast<int16_t>(editor->getWidth());
            rect.bottom = static_cast<int16_t>(editor->getHeight());
            host(&effect, VST_HOST_OPCODE_0F, rect.right, rect.bottom, nullptr, 0);
        }
    }
    void closeEditor() {
        if (editor) {
            editor->removeComponentListener(this);
            processor.editorBeingDeleted(editor.get());
            editor->removeFromDesktop();
            editor.reset();
        }
    }
    void prepare() {
        processor.prepareToPlay(rate, block);
        prepared = true;
    }
    static Legacy &self(vst_effect *e) {
        return *static_cast<Legacy *>(e->effect_internal);
    }
    static juce::AudioProcessorParameter *parameter(Legacy &o, uint32_t i) {
        return i < static_cast<uint32_t>(o.processor.getParameters().size())
                   ? o.processor.getParameters()[static_cast<int>(i)]
                   : nullptr;
    }
    static void copy(void *output, const juce::String &value, int size) {
        if (output)
            juce::String(value).copyToUTF8(static_cast<char *>(output), static_cast<size_t>(size));
    }
    static void VST_FUNCTION_INTERFACE setParameter(vst_effect *e, uint32_t i, float v) {
        auto &o = self(e);
        if (auto *p = parameter(o, i)) {
            auto *old = updating;
            updating = &o;
            p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, std::isfinite(v) ? v : 0));
            updating = old;
        }
    }
    static float VST_FUNCTION_INTERFACE getParameter(vst_effect *e, uint32_t i) {
        if (auto *p = parameter(self(e), i))
            return p->getValue();
        return 0;
    }
    static void VST_FUNCTION_INTERFACE process(vst_effect *e, const float *const *inputs,
                                               float **outputs, int32_t samples) {
        auto &o = self(e);
        if (samples <= 0 || !outputs || !outputs[0] || !outputs[1])
            return;
        for (int c = 0; c < 2; ++c) {
            if (inputs && inputs[c]) {
                if (inputs[c] != outputs[c])
                    std::memmove(outputs[c], inputs[c],
                                 static_cast<size_t>(samples) * sizeof(float));
            } else
                juce::FloatVectorOperations::clear(outputs[c], samples);
        }
        juce::AudioBuffer<float> audio(outputs, 2, samples);
        o.midi.clear();
        o.processor.processBlock(audio, o.midi);
    }
    static intptr_t VST_FUNCTION_INTERFACE dispatch(vst_effect *e, VST_EFFECT_OPCODE opcode,
                                                    int32_t index, intptr_t value, void *ptr,
                                                    float opt) {
        auto &o = self(e);
        try {
            switch (static_cast<int>(opcode)) {
            case 0:
                return 0;
            case 1:
                delete &o;
                return 0;
            case 2:
                return 0;
            case 3:
                return 0;
            case 4:
                return 0;
            case 5:
                copy(ptr, "Default", 24);
                return 1;
            case 6:
                if (auto *p = parameter(o, static_cast<uint32_t>(index)))
                    copy(ptr, p->getLabel(), 8);
                return 1;
            case 7:
                if (auto *p = parameter(o, static_cast<uint32_t>(index)))
                    copy(ptr, p->getText(p->getValue(), 8), 8);
                return 1;
            case 8:
                if (auto *p = parameter(o, static_cast<uint32_t>(index)))
                    copy(ptr, p->getName(8), 8);
                return 1;
            case 10:
                o.rate = std::isfinite(opt) && opt >= 8000 ? opt : 44100;
                if (o.prepared)
                    o.prepare();
                return 1;
            case 11:
                o.block = juce::jlimit(1, 65536, static_cast<int>(value));
                if (o.prepared)
                    o.prepare();
                return 1;
            case 12:
                if (value)
                    o.prepare();
                else {
                    o.processor.releaseResources();
                    o.prepared = false;
                }
                return 1;
            case 13:
                if (ptr) {
                    *static_cast<vst_rect **>(ptr) = &o.rect;
                    return 1;
                }
                return 0;
            case 14:
                if (!o.editor) {
                    o.editor.reset(o.processor.createEditorIfNeeded());
                    o.editor->addComponentListener(&o);
                    o.editor->setOpaque(true);
                    o.editor->addToDesktop(0, ptr);
                    o.editor->setVisible(true);
                }
                return 1;
            case 15:
                o.closeEditor();
                return 1;
            case 19:
                return 0;
            case 22:
                return 0x4e764566;
            case 23:
                if (ptr) {
                    o.processor.getStateInformation(o.chunk);
                    *static_cast<void **>(ptr) = o.chunk.getData();
                    return static_cast<intptr_t>(o.chunk.getSize());
                }
                return 0;
            case 24:
                if (ptr && value > 0 && value < 16 * 1024 * 1024)
                    o.processor.setStateInformation(ptr, static_cast<int>(value));
                return 1;
            case 26:
                return parameter(o, static_cast<uint32_t>(index)) ? 1 : 0;
            case 29:
                copy(ptr, "Default", 24);
                return index == 0 ? 1 : 0;
            case 33:
            case 34:
                if (ptr && index >= 0 && index < 2) {
                    struct Pin {
                        char label[64];
                        int32_t flags, arrangement;
                        char shortLabel[8];
                        char future[48];
                    };
                    auto *pin = static_cast<Pin *>(ptr);
                    std::memset(pin, 0, sizeof(Pin));
                    copy(pin->label, index == 0 ? "Left" : "Right", 64);
                    pin->flags = index == 0 ? 3 : 1;
                    return 1;
                }
                return 0;
            case 35:
                return 1;
            case 44:
                setParameter(e, 0, value ? 1.0f : 0.0f);
                return 1;
            case 45:
                copy(ptr, "CHi Metric Match Pro", 32);
                return 1;
            case 47:
                copy(ptr, "CHi Audio", 64);
                return 1;
            case 48:
                copy(ptr, "CHi Metric Match Pro", 64);
                return 1;
            case 49:
                return 0x000200;
            case 51:
                if (ptr) {
                    const auto *name = static_cast<const char *>(ptr);
                    return std::strcmp(name, "plugAsChannelInsert") == 0 ||
                                   std::strcmp(name, "plugAsSend") == 0 ||
                                   std::strcmp(name, "receiveVstTimeInfo") == 0
                               ? 1
                               : 0;
                }
                return 0;
            case 56:
                if (ptr) {
                    if (auto *p = parameter(o, static_cast<uint32_t>(index))) {
                        auto *properties = static_cast<vst_parameter_properties *>(ptr);
                        std::memset(properties, 0, sizeof(*properties));
                        copy(properties->name, p->getName(63), 64);
                        copy(properties->label, p->getLabel(), 8);
                        properties->flags = p->isBoolean() ? 1u : 0u;
                        properties->step_f32 =
                            p->isDiscrete() ? 1.0f / juce::jmax(1, p->getNumSteps() - 1) : .001f;
                        properties->step_small_f32 = properties->step_f32;
                        properties->step_large_f32 = .1f;
                        return 1;
                    }
                }
                return 0;
            case 52:
                return 1;
            case 58:
                return 2400;
            case 71:
                if (!o.prepared)
                    o.prepare();
                return 1;
            case 72:
                return 1;
            default:
                return 0;
            }
        } catch (...) {
            return 0;
        }
    }

  public:
    vst_effect effect{};
    explicit Legacy(vst_host_callback callback) : playhead(*this), host(callback) {
        effect.magic_number = 0x56737450;
        effect.control = dispatch;
        effect.process = process;
        effect.process_float = process;
        effect.set_parameter = setParameter;
        effect.get_parameter = getParameter;
        effect.num_programs = 1;
        effect.num_params = processor.getParameters().size();
        effect.num_inputs = effect.num_outputs = 2;
        effect.flags = 1 | 16 | 32;
        effect.effect_internal = this;
        effect.unique_id = 0x43686d32;
        effect.version = 0x000200;
        effect._unknown_float_00 = 1;
        processor.setPlayHead(&playhead);
        for (auto *p : processor.getParameters())
            p->addListener(this);
    }
    ~Legacy() {
        closeEditor();
        for (auto *p : processor.getParameters())
            p->removeListener(this);
        processor.setPlayHead(nullptr);
        processor.releaseResources();
    }
};
thread_local Legacy *Legacy::updating = nullptr;
} // namespace
#if defined(_WIN32)
#define CHI_EXPORT __declspec(dllexport)
#else
#define CHI_EXPORT __attribute__((visibility("default")))
#endif
extern "C" CHI_EXPORT vst_effect *VSTPluginMain(vst_host_callback callback) {
    if (!callback)
        return nullptr;
    try {
        return &(new Legacy(callback))->effect;
    } catch (...) {
        return nullptr;
    }
}

#if defined(_WIN32)
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH)
        juce::Process::setCurrentModuleInstanceHandle(instance);
    return TRUE;
}
#endif
