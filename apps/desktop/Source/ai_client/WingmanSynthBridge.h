/*
  ==============================================================================

    WingmanSynthBridge.h
    Created: 2025-01-29
    Author:  Zenith DAW

    Real-time bridge between Wingman AI and ZenithPolySynth.

  ==============================================================================
*/

#pragma once

#include <zenith_core/instruments/ZenithPolySynthDefs.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {

//==============================================================================
// Forward declarations
class ZenithPolySynthProcessor;

//==============================================================================
/**
    Real-time parameter changes from Wingman that UI should animate
*/
struct WingmanParameterChange {
    juce::String parameterId;
    float newValue;
    juce::String displayValue;
    float animationSpeed;

    WingmanParameterChange()
        : newValue(0.0f), animationSpeed(0.3f) {}

    WingmanParameterChange(const juce::String& id, float val,
                          const juce::String& display = juce::String(),
                          float speed = 0.3f)
        : parameterId(id), newValue(val), displayValue(display), animationSpeed(speed) {}
};

//==============================================================================
/**
    Listener interface for UI components to observe Wingman changes
*/
class WingmanSynthListener {
public:
    virtual ~WingmanSynthListener() = default;

    virtual void wingmanParameterChanged(const WingmanParameterChange& change) = 0;
    virtual void wingmanBatchStart() {}
    virtual void wingmanBatchEnd() {}
    virtual void wingmanSoundGenerated(const juce::String& description) {}
};

//==============================================================================
/**
    Bridge between Wingman AI and ZenithPolySynth
    Handles all AI-to-synth communication with real-time UI feedback
*/
class WingmanSynthBridge {
public:
    explicit WingmanSynthBridge(ZenithPolySynthProcessor& processor);
    ~WingmanSynthBridge();

    //==========================================================================
    // Parameter Control (Real-time with UI updates)
    //==========================================================================

    void setParameter(const juce::String& paramId, float value,
                     const juce::String& displayValue = juce::String(),
                     float animateSpeed = 0.3f);

    void setParameters(const juce::StringArray& paramIds,
                      const juce::Array<float>& values,
                      const juce::String& operation = "batch");

    //==========================================================================
    // Oscillator Control
    //==========================================================================

    void setOscillatorWaveform(int oscIndex, OscillatorWaveform wave);
    void setOscillatorDetune(int oscIndex, float cents);
    void setOscillatorMix(int oscIndex, float mix);
    void setOscillatorShape(int oscIndex, float shape);

    //==========================================================================
    // Filter Control
    //==========================================================================

    void setFilterType(FilterType type);
    void setFilterCutoff(float hz);
    void setFilterResonance(float resonance);
    void setFilterDrive(float drive);

    //==========================================================================
    // Envelope Control
    //==========================================================================

    void setAmpEnvelope(float attack, float decay, float sustain, float release);
    void setFilterEnvelope(float attack, float decay, float sustain, float release);

    //==========================================================================
    // LFO Control
    //==========================================================================

    void setLFORate(int lfoIndex, float rate);
    void setLFOAmount(int lfoIndex, float amount);
    void setLFOWaveform(int lfoIndex, LFOWaveform wave);

    //==========================================================================
    // Effects Control
    //==========================================================================

    void setDistortion(float amount);
    void setChorus(float amount);
    void setReverb(float amount);
    void setDelay(float time, float feedback, float mix);

    //==========================================================================
    // Unison Control
    //==========================================================================

    void setUnisonVoices(int voices);
    void setUnisonDetune(float cents);
    void setUnisonSpread(float spread);
    void setUnisonPanRandom(bool random);

    //==========================================================================
    // Arpeggiator Control
    //==========================================================================

    void setArpEnable(bool enable);
    void setArpMode(int mode);
    void setArpRate(float rate);
    void setArpGate(float gate);
    void setArpSwing(float swing);
    void setArpHold(bool hold);

    //==========================================================================
    // Step LFO Control
    //==========================================================================

    void setStepLFOEnable(int lfoIndex, bool enable);
    void setStepLFOSteps(int lfoIndex, int steps);
    void setStepLFORate(int lfoIndex, float rate);
    void setStepLFOSmoothing(int lfoIndex, int smoothing);

    //==========================================================================
    // High-Level Commands
    //==========================================================================

    void randomizePatch(float amount = 0.5f);

    //==========================================================================
    // Listener Management
    //==========================================================================

    void addListener(WingmanSynthListener* listener);
    void removeListener(WingmanSynthListener* listener);

    //==========================================================================
    // Context
    //==========================================================================

    juce::var getCurrentPatchState() const;
    juce::String analyzePatch() const;

private:
    ZenithPolySynthProcessor& processor_;

    juce::Array<WingmanSynthListener*> listeners_;
    juce::CriticalSection listenerLock_;

    juce::AudioProcessorParameter* getParameter(const juce::String& paramId);
    void notifyParameterChanged(const WingmanParameterChange& change);
    juce::String getParameterIdForEnum(int oscIndex, const juce::String& param) const;
    juce::String getStepLFOParamId(int lfoIndex, const juce::String& param) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanSynthBridge)
};

//==============================================================================
/**
    Real-time animator for smooth parameter transitions
*/
class WingmanParameterAnimator {
public:
    struct Animation {
        juce::String parameterId;
        float startValue;
        float targetValue;
        float progress;
        float duration;
        double startTime;

        Animation()
            : startValue(0.0f), targetValue(0.0f),
              progress(0.0f), duration(0.5f), startTime(0.0) {}
    };

    WingmanParameterAnimator();
    ~WingmanParameterAnimator();

    void startAnimation(const juce::String& paramId, float from, float to,
                       float duration = 0.5f);

    bool isAnimating(const juce::String& paramId) const;
    float getCurrentValue(const juce::String& paramId);
    void update(float deltaTime);
    juce::StringArray getAnimatingParameters() const;

private:
    juce::HashMap<juce::String, Animation> animations_;
    juce::CriticalSection animLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WingmanParameterAnimator)
};

} // namespace zenith