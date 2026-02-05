/*
  ==============================================================================

    WingmanSynthBridge.cpp
    Created: 2025-01-29
    Author:  Zenith DAW

    Implementation of Wingman → Synth real-time bridge.

  ==============================================================================
*/

#include "WingmanSynthBridge.h"
#include "../instruments/ZenithPolySynth.h"
#include "../instruments/ZenithPolySynthParameterManager.h"

namespace zenith {

//==============================================================================
WingmanSynthBridge::WingmanSynthBridge(ZenithPolySynthProcessor& processor)
    : processor_(processor) {
}

WingmanSynthBridge::~WingmanSynthBridge() {
}

//==============================================================================
void WingmanSynthBridge::setParameter(const juce::String& paramId, float value,
                                     const juce::String& displayValue,
                                     float animateSpeed) {
    if (auto* param = getParameter(paramId)) {
        param->setValueNotifyingHost(value);

        WingmanParameterChange change(paramId, value,
                                     displayValue.isNotEmpty() ? displayValue
                                                               : juce::String(value, 2),
                                     animateSpeed);
        notifyParameterChanged(change);
    }
}

//==============================================================================
void WingmanSynthBridge::setParameters(const juce::StringArray& paramIds,
                                      const juce::Array<float>& values,
                                      const juce::String& operation) {
    {
        juce::ScopedLock lock(listenerLock_);
        for (auto* listener : listeners_) {
            listener->wingmanBatchStart();
        }
    }

    jassert(paramIds.size() == values.size());
    for (int i = 0; i < juce::jmin(paramIds.size(), values.size()); ++i) {
        setParameter(paramIds[i], values[i], {}, 0.1f);
    }

    {
        juce::ScopedLock lock(listenerLock_);
        for (auto* listener : listeners_) {
            listener->wingmanBatchEnd();
        }
    }
}

//==============================================================================
void WingmanSynthBridge::setOscillatorWaveform(int oscIndex, OscillatorWaveform wave) {
    jassert(oscIndex >= 1 && oscIndex <= 3);
    const juce::String paramId = getParameterIdForEnum(oscIndex, "Wave");
    setParameter(paramId, static_cast<float>(wave));
}

void WingmanSynthBridge::setOscillatorDetune(int oscIndex, float cents) {
    jassert(oscIndex >= 1 && oscIndex <= 3);
    const juce::String paramId = getParameterIdForEnum(oscIndex, "Detune");
    setParameter(paramId, cents, juce::String(cents, 1) + " ct");
}

void WingmanSynthBridge::setOscillatorMix(int oscIndex, float mix) {
    jassert(oscIndex >= 1 && oscIndex <= 3);
    const juce::String paramId = getParameterIdForEnum(oscIndex, "Mix");
    setParameter(paramId, juce::jlimit(0.0f, 1.0f, mix),
                juce::String(static_cast<int>(mix * 100)) + "%");
}

void WingmanSynthBridge::setOscillatorShape(int oscIndex, float shape) {
    jassert(oscIndex >= 1 && oscIndex <= 3);
    const juce::String paramId = getParameterIdForEnum(oscIndex, "Shape");
    setParameter(paramId, juce::jlimit(0.0f, 1.0f, shape));
}

//==============================================================================
void WingmanSynthBridge::setFilterType(FilterType type) {
    setParameter(ZenithPolySynthProcessor::FilterType,
                static_cast<float>(type));
}

void WingmanSynthBridge::setFilterCutoff(float hz) {
    setParameter(ZenithPolySynthProcessor::FilterCutoff,
                juce::jlimit(20.0f, 20000.0f, hz),
                juce::String(static_cast<int>(hz)) + " Hz");
}

void WingmanSynthBridge::setFilterResonance(float resonance) {
    setParameter(ZenithPolySynthProcessor::FilterResonance,
                juce::jlimit(0.0f, 1.0f, resonance),
                juce::String(static_cast<int>(resonance * 100)) + "%");
}

void WingmanSynthBridge::setFilterDrive(float drive) {
    setParameter(ZenithPolySynthProcessor::FilterDrive,
                juce::jlimit(1.0f, 10.0f, drive),
                juce::String(drive, 1) + "x");
}

//==============================================================================
void WingmanSynthBridge::setAmpEnvelope(float attack, float decay,
                                       float sustain, float release) {
    juce::StringArray ids;
    ids.add(ZenithPolySynthProcessor::AmpAttack);
    ids.add(ZenithPolySynthProcessor::AmpDecay);
    ids.add(ZenithPolySynthProcessor::AmpSustain);
    ids.add(ZenithPolySynthProcessor::AmpRelease);

    juce::Array<float> values;
    values.add(juce::jlimit(0.0f, 10.0f, attack));
    values.add(juce::jlimit(0.0f, 10.0f, decay));
    values.add(juce::jlimit(0.0f, 1.0f, sustain));
    values.add(juce::jlimit(0.0f, 10.0f, release));

    setParameters(ids, values, "Amp Envelope");
}

void WingmanSynthBridge::setFilterEnvelope(float attack, float decay,
                                          float sustain, float release) {
    juce::StringArray ids;
    ids.add(ZenithPolySynthProcessor::ModAttack);
    ids.add(ZenithPolySynthProcessor::ModDecay);
    ids.add(ZenithPolySynthProcessor::ModSustain);
    ids.add(ZenithPolySynthProcessor::ModRelease);

    juce::Array<float> values;
    values.add(juce::jlimit(0.0f, 10.0f, attack));
    values.add(juce::jlimit(0.0f, 10.0f, decay));
    values.add(juce::jlimit(0.0f, 1.0f, sustain));
    values.add(juce::jlimit(0.0f, 10.0f, release));

    setParameters(ids, values, "Filter Envelope");
}

//==============================================================================
void WingmanSynthBridge::setLFORate(int lfoIndex, float rate) {
    jassert(lfoIndex >= 1 && lfoIndex <= 2);
    const juce::String paramId = (lfoIndex == 1)
        ? ZenithPolySynthProcessor::LFO1Rate
        : ZenithPolySynthProcessor::LFO2Rate;
    setParameter(paramId, rate, juce::String(rate, 2) + " Hz");
}

void WingmanSynthBridge::setLFOAmount(int lfoIndex, float amount) {
    jassert(lfoIndex >= 1 && lfoIndex <= 2);
    const juce::String paramId = (lfoIndex == 1)
        ? ZenithPolySynthProcessor::LFO1Amount
        : ZenithPolySynthProcessor::LFO2Amount;
    setParameter(paramId, amount, juce::String(static_cast<int>(amount * 100)) + "%");
}

void WingmanSynthBridge::setLFOWaveform(int lfoIndex, LFOWaveform wave) {
    jassert(lfoIndex >= 1 && lfoIndex <= 2);
    const juce::String paramId = (lfoIndex == 1)
        ? ZenithPolySynthProcessor::LFO1Waveform
        : ZenithPolySynthProcessor::LFO2Waveform;
    setParameter(paramId, static_cast<float>(wave));
}

//==============================================================================
void WingmanSynthBridge::setDistortion(float amount) {
    setParameter(ZenithPolySynthProcessor::DistortionAmount,
                juce::jlimit(0.0f, 1.0f, amount),
                juce::String(static_cast<int>(amount * 100)) + "%");
}

void WingmanSynthBridge::setChorus(float amount) {
    setParameter(ZenithPolySynthProcessor::ChorusAmount,
                juce::jlimit(0.0f, 1.0f, amount),
                juce::String(static_cast<int>(amount * 100)) + "%");
}

void WingmanSynthBridge::setReverb(float amount) {
    setParameter(ZenithPolySynthProcessor::ReverbAmount,
                juce::jlimit(0.0f, 1.0f, amount),
                juce::String(static_cast<int>(amount * 100)) + "%");
}

void WingmanSynthBridge::setDelay(float time, float feedback, float mix) {
    juce::StringArray ids;
    ids.add(ZenithPolySynthProcessor::DelayTime);
    ids.add(ZenithPolySynthProcessor::DelayFeedback);
    ids.add(ZenithPolySynthProcessor::DelayMix);

    juce::Array<float> values;
    values.add(juce::jlimit(0.0f, 1.0f, time));
    values.add(juce::jlimit(0.0f, 0.95f, feedback));
    values.add(juce::jlimit(0.0f, 1.0f, mix));

    setParameters(ids, values, "Delay");
}

//==============================================================================
// Unison Control
//==============================================================================

void WingmanSynthBridge::setUnisonVoices(int voices) {
    setParameter(ZenithPolySynthProcessor::UnisonVoices,
                static_cast<float>(juce::jlimit(1, 7, voices)),
                juce::String(juce::jlimit(1, 7, voices)) + " voices");
}

void WingmanSynthBridge::setUnisonDetune(float cents) {
    setParameter(ZenithPolySynthProcessor::UnisonDetune,
                juce::jlimit(0.0f, 100.0f, cents),
                juce::String(cents, 1) + " ct");
}

void WingmanSynthBridge::setUnisonSpread(float spread) {
    setParameter(ZenithPolySynthProcessor::UnisonSpread,
                juce::jlimit(0.0f, 1.0f, spread),
                juce::String(static_cast<int>(spread * 100)) + "%");
}

void WingmanSynthBridge::setUnisonPanRandom(bool random) {
    setParameter(ZenithPolySynthProcessor::UnisonPanRandom,
                random ? 1.0f : 0.0f,
                random ? "Random" : "Even");
}

//==============================================================================
// Arpeggiator Control
//==============================================================================

void WingmanSynthBridge::setArpEnable(bool enable) {
    setParameter(ZenithPolySynthProcessor::ArpEnable,
                enable ? 1.0f : 0.0f,
                enable ? "On" : "Off");
}

void WingmanSynthBridge::setArpMode(int mode) {
    setParameter(ZenithPolySynthProcessor::ArpMode,
                static_cast<float>(juce::jlimit(0, 6, mode)),
                juce::String(juce::jlimit(0, 6, mode)));
}

void WingmanSynthBridge::setArpRate(float rate) {
    setParameter(ZenithPolySynthProcessor::ArpRate,
                juce::jlimit(0.25f, 32.0f, rate),
                juce::String(rate, 2) + " Hz");
}

void WingmanSynthBridge::setArpGate(float gate) {
    setParameter(ZenithPolySynthProcessor::ArpGate,
                juce::jlimit(0.0f, 1.0f, gate),
                juce::String(static_cast<int>(gate * 100)) + "%");
}

void WingmanSynthBridge::setArpSwing(float swing) {
    setParameter(ZenithPolySynthProcessor::ArpSwing,
                juce::jlimit(0.0f, 1.0f, swing),
                juce::String(static_cast<int>(swing * 100)) + "%");
}

void WingmanSynthBridge::setArpHold(bool hold) {
    setParameter(ZenithPolySynthProcessor::ArpHold,
                hold ? 1.0f : 0.0f,
                hold ? "Hold" : "Restart");
}

//==============================================================================
// Step LFO Control
//==============================================================================

void WingmanSynthBridge::setStepLFOEnable(int lfoIndex, bool enable) {
    jassert(lfoIndex >= 1 && lfoIndex <= 4);
    const juce::String paramId = getStepLFOParamId(lfoIndex, "Enable");
    setParameter(paramId, enable ? 1.0f : 0.0f, enable ? "On" : "Off");
}

void WingmanSynthBridge::setStepLFOSteps(int lfoIndex, int steps) {
    jassert(lfoIndex >= 1 && lfoIndex <= 4);
    const juce::String paramId = getStepLFOParamId(lfoIndex, "Steps");
    setParameter(paramId, static_cast<float>(juce::jlimit(1, 64, steps)),
                juce::String(juce::jlimit(1, 64, steps)) + " steps");
}

void WingmanSynthBridge::setStepLFORate(int lfoIndex, float rate) {
    jassert(lfoIndex >= 1 && lfoIndex <= 4);
    const juce::String paramId = getStepLFOParamId(lfoIndex, "Rate");
    setParameter(paramId, juce::jlimit(0.0f, 1.0f, rate),
                juce::String(rate, 2) + " Hz");
}

void WingmanSynthBridge::setStepLFOSmoothing(int lfoIndex, int smoothing) {
    jassert(lfoIndex >= 1 && lfoIndex <= 4);
    const juce::String paramId = getStepLFOParamId(lfoIndex, "Smoothing");
    setParameter(paramId, static_cast<float>(juce::jlimit(0, 2, smoothing)),
                juce::String(juce::jlimit(0, 2, smoothing)));
}

//==============================================================================
void WingmanSynthBridge::randomizePatch(float amount) {
    juce::Random rand;

    setFilterCutoff(200.0f + rand.nextFloat() * 10000.0f);
    setFilterResonance(rand.nextFloat() * 0.5f);

    setOscillatorDetune(1, rand.nextFloat() * 50.0f);
    setOscillatorDetune(2, rand.nextFloat() * 50.0f);

    setAmpEnvelope(0.01f + rand.nextFloat() * 0.5f,
                   0.1f + rand.nextFloat() * 1.0f,
                   0.3f + rand.nextFloat() * 0.7f,
                   0.1f + rand.nextFloat() * 2.0f);
}

//==============================================================================
void WingmanSynthBridge::addListener(WingmanSynthListener* listener) {
    juce::ScopedLock lock(listenerLock_);
    listeners_.addIfNotAlreadyThere(listener);
}

void WingmanSynthBridge::removeListener(WingmanSynthListener* listener) {
    juce::ScopedLock lock(listenerLock_);
    listeners_.removeAllInstancesOf(listener);
}

//==============================================================================
juce::var WingmanSynthBridge::getCurrentPatchState() const {
    return juce::var("ZenithPolySynth patch");
}

juce::String WingmanSynthBridge::analyzePatch() const {
    return "Current patch is a subtractive synth setup";
}

//==============================================================================
juce::AudioProcessorParameter* WingmanSynthBridge::getParameter(const juce::String& paramId) {
    // Try to get parameter from AudioProcessorValueTreeState
    auto* rangedParam = processor_.getParameters().getParameter(paramId);
    if (rangedParam != nullptr) {
        // RangedAudioParameter is a subclass of AudioProcessorParameter
        return reinterpret_cast<juce::AudioProcessorParameter*>(rangedParam);
    }
    return nullptr;
}

void WingmanSynthBridge::notifyParameterChanged(const WingmanParameterChange& change) {
    juce::ScopedLock lock(listenerLock_);
    for (auto* listener : listeners_) {
        listener->wingmanParameterChanged(change);
    }
}

juce::String WingmanSynthBridge::getParameterIdForEnum(int oscIndex,
                                                       const juce::String& param) const {
    if (oscIndex == 1) return ZenithPolySynthProcessor::Osc1Wave;
    if (oscIndex == 2) return ZenithPolySynthProcessor::Osc2Wave;
    if (oscIndex == 3) return ZenithPolySynthProcessor::Osc3Wave;
    return {};
}

juce::String WingmanSynthBridge::getStepLFOParamId(int lfoIndex,
                                                    const juce::String& param) const {
    juce::String base;
    if (lfoIndex == 1) base = ZenithPolySynthProcessor::StepLFO1Enable;
    else if (lfoIndex == 2) base = ZenithPolySynthProcessor::StepLFO2Enable;
    else if (lfoIndex == 3) base = ZenithPolySynthProcessor::StepLFO3Enable;
    else if (lfoIndex == 4) base = ZenithPolySynthProcessor::StepLFO4Enable;
    else return {};

    // Replace "Enable" with the requested param name
    if (param == "Enable") return base;
    if (base.endsWith("Enable")) {
        auto prefix = base.dropLastCharacters(6);  // Remove "Enable"
        return prefix + param;
    }
    return base + param;
}

//==============================================================================
WingmanParameterAnimator::WingmanParameterAnimator() {
}

WingmanParameterAnimator::~WingmanParameterAnimator() {
}

void WingmanParameterAnimator::startAnimation(const juce::String& paramId,
                                             float from, float to,
                                             float duration) {
    juce::ScopedLock lock(animLock_);

    Animation anim;
    anim.parameterId = paramId;
    anim.startValue = from;
    anim.targetValue = to;
    anim.progress = 0.0f;
    anim.duration = duration;
    anim.startTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;

    animations_.set(paramId, anim);
}

bool WingmanParameterAnimator::isAnimating(const juce::String& paramId) const {
    return animations_.contains(paramId);
}

float WingmanParameterAnimator::getCurrentValue(const juce::String& paramId) {
    juce::ScopedLock lock(animLock_);

    if (animations_.contains(paramId)) {
        const auto& anim = animations_.getReference(paramId);
        return anim.startValue + (anim.targetValue - anim.startValue) * anim.progress;
    }

    return 0.0f;
}

void WingmanParameterAnimator::update(float deltaTime) {
    juce::ScopedLock lock(animLock_);

    juce::StringArray completed;

    juce::StringArray keys;
    for (auto it = animations_.begin(); it != animations_.end(); ++it) {
        keys.add(it.getKey());
    }

    for (auto& key : keys) {
        Animation& anim = animations_.getReference(key);
        anim.progress += deltaTime / anim.duration;

        if (anim.progress >= 1.0f) {
            anim.progress = 1.0f;
            completed.add(key);
        }
    }

    for (auto& key : completed) {
        animations_.remove(key);
    }
}

juce::StringArray WingmanParameterAnimator::getAnimatingParameters() const {
    juce::StringArray params;

    for (auto it = animations_.begin(); it != animations_.end(); ++it) {
        params.add(it.getKey());
    }

    return params;
}

} // namespace zenith
