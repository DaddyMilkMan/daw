// plugin.cpp — a real VST3 plugin built against the official Steinberg VST3 SDK
// interface headers. Compiler-generated COM vtables (the genuine third-party ABI),
// packaged as a standard .vst3 bundle. A 440 Hz stereo tone generator — minimal,
// but exercises Zenith's host against real SDK-shaped objects, not hand-rolled ones.
//
// IIDs are defined locally via INLINE_UID so we needn't link any SDK .cpp.

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include <cstring>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;

static const double PI = 3.14159265358979323846;

static const TUID kCompCID         = INLINE_UID(0x11112222, 0x33334444, 0x55556666, 0x77778888);
static const TUID kFUnknown        = INLINE_UID(0x00000000, 0x00000000, 0xC0000000, 0x00000046);
static const TUID kIPluginBase     = INLINE_UID(0x22888DDB, 0x156E45AE, 0x8358B348, 0x08190625);
static const TUID kIComponent      = INLINE_UID(0xE831FF31, 0xF2D54301, 0x928EBBEE, 0x25697802);
static const TUID kIAudioProcessor = INLINE_UID(0x42043F99, 0xB7DA453C, 0xA569E79D, 0x9AAEC33D);
static const TUID kIPluginFactory  = INLINE_UID(0x7A4D811C, 0x52114A1F, 0xAED9D2EE, 0x0B43BF9F);

static inline bool iidEq(const TUID a, const TUID b) { return std::memcmp(a, b, 16) == 0; }

static double gSr = 48000.0;
static double gPhase = 0.0;
static double gGain = 0.8; // a persisted parameter (round-trips via IBStream)

struct Comp; // fwd
static Comp* gCompPtr;

struct Proc : public IAudioProcessor {
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) SMTG_OVERRIDE;
    uint32 PLUGIN_API addRef() SMTG_OVERRIDE { return 1; }
    uint32 PLUGIN_API release() SMTG_OVERRIDE { return 1; }
    tresult PLUGIN_API setBusArrangements(SpeakerArrangement*, int32, SpeakerArrangement*, int32) SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API getBusArrangement(BusDirection, int32, SpeakerArrangement& arr) SMTG_OVERRIDE { arr = 0x3; return kResultOk; }
    tresult PLUGIN_API canProcessSampleSize(int32 s) SMTG_OVERRIDE { return s == kSample32 ? kResultOk : kResultFalse; }
    uint32 PLUGIN_API getLatencySamples() SMTG_OVERRIDE { return 0; }
    tresult PLUGIN_API setupProcessing(ProcessSetup& s) SMTG_OVERRIDE { gSr = s.sampleRate; gPhase = 0; return kResultOk; }
    tresult PLUGIN_API setProcessing(TBool) SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE {
        if (data.numOutputs < 1 || data.symbolicSampleSize != kSample32) return kResultOk;
        AudioBusBuffers& bus = data.outputs[0];
        const int n = data.numSamples;
        const double inc = 2.0 * PI * 440.0 / gSr;
        for (int i = 0; i < n; ++i) {
            const float s = (float)(std::sin(gPhase) * 0.5);
            gPhase += inc;
            if (gPhase > 2.0 * PI) gPhase -= 2.0 * PI;
            for (int c = 0; c < bus.numChannels; ++c) bus.channelBuffers32[c][i] = s;
        }
        return kResultOk;
    }
    uint32 PLUGIN_API getTailSamples() SMTG_OVERRIDE { return 0; }
};
static Proc gProc;

struct Comp : public IComponent {
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) SMTG_OVERRIDE;
    uint32 PLUGIN_API addRef() SMTG_OVERRIDE { return 1; }
    uint32 PLUGIN_API release() SMTG_OVERRIDE { return 1; }
    tresult PLUGIN_API initialize(FUnknown*) SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API terminate() SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API getControllerClassId(TUID) SMTG_OVERRIDE { return kNotImplemented; }
    tresult PLUGIN_API setIoMode(IoMode) SMTG_OVERRIDE { return kResultOk; }
    int32 PLUGIN_API getBusCount(MediaType t, BusDirection d) SMTG_OVERRIDE { return (t == kAudio && d == kOutput) ? 1 : 0; }
    tresult PLUGIN_API getBusInfo(MediaType t, BusDirection d, int32 idx, BusInfo& bus) SMTG_OVERRIDE {
        if (t == kAudio && d == kOutput && idx == 0) {
            std::memset(&bus, 0, sizeof(bus));
            bus.mediaType = kAudio;
            bus.direction = kOutput;
            bus.channelCount = 2;
            bus.busType = kMain;
            bus.flags = BusInfo::kDefaultActive;
            return kResultOk;
        }
        return kInvalidArgument;
    }
    tresult PLUGIN_API getRoutingInfo(RoutingInfo&, RoutingInfo&) SMTG_OVERRIDE { return kNotImplemented; }
    tresult PLUGIN_API activateBus(MediaType, BusDirection, int32, TBool) SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API setActive(TBool) SMTG_OVERRIDE { return kResultOk; }
    tresult PLUGIN_API setState(IBStream* s) SMTG_OVERRIDE {
        if (!s) return kResultOk;
        int32 n = 0;
        s->read(&gGain, 8, &n);
        return kResultOk;
    }
    tresult PLUGIN_API getState(IBStream* s) SMTG_OVERRIDE {
        if (!s) return kResultOk;
        int32 n = 0;
        s->write(&gGain, 8, &n);
        return kResultOk;
    }
};
static Comp gComp;

tresult PLUGIN_API Proc::queryInterface(const TUID iid, void** obj) {
    if (iidEq(iid, kIAudioProcessor) || iidEq(iid, kFUnknown)) { *obj = (IAudioProcessor*)this; return kResultOk; }
    if (iidEq(iid, kIComponent)) { *obj = (IComponent*)&gComp; return kResultOk; }
    *obj = nullptr;
    return kNoInterface;
}
tresult PLUGIN_API Comp::queryInterface(const TUID iid, void** obj) {
    if (iidEq(iid, kIComponent) || iidEq(iid, kIPluginBase) || iidEq(iid, kFUnknown)) { *obj = (IComponent*)this; return kResultOk; }
    if (iidEq(iid, kIAudioProcessor)) { *obj = (IAudioProcessor*)&gProc; return kResultOk; }
    *obj = nullptr;
    return kNoInterface;
}

struct Factory : public IPluginFactory {
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) SMTG_OVERRIDE {
        if (iidEq(iid, kIPluginFactory) || iidEq(iid, kFUnknown)) { *obj = (IPluginFactory*)this; return kResultOk; }
        *obj = nullptr;
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() SMTG_OVERRIDE { return 1; }
    uint32 PLUGIN_API release() SMTG_OVERRIDE { return 1; }
    tresult PLUGIN_API getFactoryInfo(PFactoryInfo* info) SMTG_OVERRIDE {
        std::memset(info, 0, sizeof(*info));
        std::strncpy(info->vendor, "ZenithReal", PFactoryInfo::kNameSize);
        info->flags = PFactoryInfo::kUnicode;
        return kResultOk;
    }
    int32 PLUGIN_API countClasses() SMTG_OVERRIDE { return 1; }
    tresult PLUGIN_API getClassInfo(int32 idx, PClassInfo* info) SMTG_OVERRIDE {
        if (idx != 0) return kInvalidArgument;
        std::memset(info, 0, sizeof(*info));
        std::memcpy(info->cid, kCompCID, 16);
        info->cardinality = PClassInfo::kManyInstances;
        std::strncpy(info->category, "Audio Module Class", PClassInfo::kCategorySize);
        std::strncpy(info->name, "Zenith Real (SDK C++)", PClassInfo::kNameSize);
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(FIDString cid, FIDString iid, void** obj) SMTG_OVERRIDE {
        if (!iidEq((const int8*)cid, kCompCID)) { *obj = nullptr; return kNoInterface; }
        if (iidEq((const int8*)iid, kIComponent) || iidEq((const int8*)iid, kIPluginBase) || iidEq((const int8*)iid, kFUnknown)) {
            *obj = (IComponent*)&gComp;
            return kResultOk;
        }
        if (iidEq((const int8*)iid, kIAudioProcessor)) { *obj = (IAudioProcessor*)&gProc; return kResultOk; }
        *obj = nullptr;
        return kNoInterface;
    }
};
static Factory gFactory;

extern "C" __attribute__((visibility("default"))) IPluginFactory* GetPluginFactory() {
    gCompPtr = &gComp;
    return &gFactory;
}
extern "C" __attribute__((visibility("default"))) bool ModuleEntry(void*) { return true; }
extern "C" __attribute__((visibility("default"))) bool ModuleExit() { return true; }
