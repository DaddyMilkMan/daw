/*
  ==============================================================================

    ZenithPolySynth.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Multi-oscillator subtractive polyphonic synthesizer optimized for EDM/trap/future-bass.

    Features:
    - 2-3 oscillators with sine, saw, square, triangle, noise, and supersaw modes
    - Multimode filter (lowpass, bandpass, highpass) with resonance and drive
    - 2 ADSR envelopes (amplitude and modulation)
    - 2 LFOs with multiple targets
    - Unison/detune for supersaw
    - Glide (portamento)
    - RT-safe: all buffers preallocated, no locks in audio thread

    //==========================================================================
    // AI-FRIENDLY MODULATION MATRIX + MACROS
    //==========================================================================

    This synthesizer provides TWO ways for AI to create expressive sounds:

    1. MACRO CONTROLS (Recommended for AI)
       - 8 high-level semantic controls with human-readable names
       - Each macro affects multiple related parameters automatically
       - Easy to reason about musically

       Available Macros:
       - Brightness:  Filter cutoff and resonance (brighter/darker tone)
       - Thickness:   Unison voices and oscillator layering (thin/thick sound)
       - Movement:    LFO modulation amounts (static/dynamic)
       - Attack:      Envelope attack times (slow fade/instant punch)
       - Release:     Envelope release times (short/long tail)
       - Warmth:      Filter character and saturation (cold/warm analog tone)
       - Detune:      Oscillator detuning (tight/chorus-like width)
       - Depth:       Modulation envelope intensity (subtle/pronounced evolution)

       Usage from CommandAPI:
         setMacro("macro_brightness", 0.8);  // Increase brightness to 80%
         setMacro("macro_thickness", 0.6);   // Add some thickness
         setMacro("macro_movement", 0.4);    // Add subtle movement

    2. MODULATION MATRIX (Advanced)
       - Flexible routing of sources to destinations
       - 8 modulation slots for custom routing

       Sources:  LFO1, LFO2, Env1, Env2, Velocity, ModWheel, Aftertouch
       Destinations: FilterCutoff, FilterResonance, Osc1/2/3Pitch,
                     WavetablePos, Pan, Volume, Osc1/2/3Mix

       Usage Examples:
       - Route LFO1 to filter cutoff for wobble bass:
         setModulationSlot(0, LFO1, FilterCutoff, 0.5);

       - Route velocity to filter cutoff for dynamic response:
         setModulationSlot(1, Velocity, FilterCutoff, 0.7);

       - Route Env2 to oscillator pitch for plucky sounds:
         setModulationSlot(2, Env2, Osc1Pitch, 0.3);

    AI DESIGN RECOMMENDATIONS:
    - For quick sound design, use macros (easier to reason about)
    - For advanced modulation, use the modulation matrix
    - Combine both: use macros for basic tone, matrix for special effects
    - Macros are centered at 0.5 (neutral), 0.0 (minimum), 1.0 (maximum)
    - Modulation matrix amounts are bipolar: -1.0 to +1.0

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Instrument.h"
#include <array>

namespace zenith {

//==============================================================================
/**
    Waveform types for oscillators
*/
enum class OscillatorWaveform
{
    Sine = 0,
    Saw,
    Square,
    Triangle,
    Noise,
    Supersaw,
    NumWaveforms
};

/**
    Filter types
*/
enum class FilterType
{
    Lowpass = 0,
    Bandpass,
    Highpass,
    NumTypes
};

/**
    Quality preset for CPU optimization
*/
enum class QualityPreset
{
    Low = 0,    // Max 3 unison voices, optimized for CPU
    Medium,     // Max 5 unison voices, balanced
    High,       // Max 7 unison voices, full quality
    NumPresets
};

/**
    LFO target parameters (legacy - now part of modulation matrix)
*/
enum class LFOTarget
{
    FilterCutoff = 0,
    Osc1Pitch,
    Osc2Pitch,
    Osc1Mix,
    Osc2Mix,
    NumTargets
};

//==============================================================================
/**
    Modulation Matrix System

    AI-FRIENDLY MODULATION SYSTEM:
    This modulation matrix allows flexible routing of modulation sources to
    multiple destinations. AI agents can use this to create expressive sounds
    by routing LFOs, envelopes, velocity, and MIDI controllers to various
    synthesis parameters.

    Usage from AI:
    - Route LFO1 to filter cutoff for wobble bass effects
    - Route Velocity to filter cutoff for dynamic response
    - Route ModWheel to vibrato depth for expressive performance
    - Route Env2 to wavetable position for evolving timbres
*/

/**
    Modulation sources available in the matrix
*/
enum class ModulationSource
{
    None = 0,       // No modulation
    LFO1,           // Low-frequency oscillator 1 (sine wave, -1 to +1)
    LFO2,           // Low-frequency oscillator 2 (sine wave, -1 to +1)
    Env1,           // Amplitude envelope (0 to 1, ADSR)
    Env2,           // Modulation envelope (0 to 1, ADSR)
    Velocity,       // Note-on velocity (0 to 1)
    ModWheel,       // MIDI mod wheel CC#1 (0 to 1)
    Aftertouch,     // MIDI channel pressure (0 to 1)
    NumSources
};

/**
    Modulation destinations available in the matrix
*/
enum class ModulationDestination
{
    None = 0,           // No destination
    FilterCutoff,       // Filter cutoff frequency
    FilterResonance,    // Filter resonance/Q
    Osc1Pitch,          // Oscillator 1 pitch (semitones)
    Osc2Pitch,          // Oscillator 2 pitch (semitones)
    Osc3Pitch,          // Oscillator 3 pitch (semitones)
    WavetablePos,       // Wavetable/phase position (0 to 1)
    Pan,                // Stereo panning (-1 left to +1 right)
    Volume,             // Output volume/gain
    Osc1Mix,            // Oscillator 1 mix level
    Osc2Mix,            // Oscillator 2 mix level
    Osc3Mix,            // Oscillator 3 mix level
    OscShape,           // Oscillator Shape/PulseWidth (0 to 1)
    NumDestinations
};

/**
    Single modulation routing slot

    Each slot defines: source -> destination with an amount
    Example: LFO1 -> FilterCutoff with amount 0.5 (50% modulation depth)
*/
struct ModulationSlot
{
    ModulationSource source = ModulationSource::None;
    ModulationDestination destination = ModulationDestination::None;
    float amount = 0.0f;  // Modulation depth/amount (-1 to +1)

    bool isActive() const {
        return source != ModulationSource::None &&
               destination != ModulationDestination::None;
    }
};

/**
    RT-safe modulation state per voice

    Stores computed modulation values for each destination.
    Updated once per audio buffer to avoid redundant calculations.
*/
struct ModulationState
{
    // Pre-computed modulation amounts for each destination
    std::array<float, static_cast<size_t>(ModulationDestination::NumDestinations)> values;

    ModulationState() { reset(); }

    void reset() { values.fill(0.0f); }

    float get(ModulationDestination dest) const
    {
        return values[static_cast<size_t>(dest)];
    }

    void set(ModulationDestination dest, float value) [[maybe_unused]]
    {
        values[static_cast<size_t>(dest)] = value;
    }

    void add(ModulationDestination dest, float value) [[maybe_unused]]
    {
        values[static_cast<size_t>(dest)] += value;
    }
};

//==============================================================================
/**
    Single oscillator with multiple waveforms and detune
*/
class ZenithOscillator
{
public:
    ZenithOscillator() = default;

    void setWaveform(OscillatorWaveform waveform) [[maybe_unused]] { waveform_ = waveform; }
    void setDetune(float detuneCents) [[maybe_unused]] { detuneCents_ = detuneCents; }
    void setSampleRate(double sampleRate) [[maybe_unused]] { sampleRate_ = sampleRate; }
    void reset() { phase_ = 0.0; }

    /**
     * @brief Generate next sample
     * @param frequency Base frequency in Hz
     * @param shape Shape parameter (Pulse Width for Square, etc.)
     * @return Sample value in range [-1, 1]
     */
    float getNextSample(float frequency, float shape = 0.5f);

private:
    OscillatorWaveform waveform_ = OscillatorWaveform::Saw;
    double phase_ = 0.0;
    double sampleRate_ = 44100.0;
    float detuneCents_ = 0.0f;
    juce::Random random_;

    float processSine(float frequency);
    float processSaw(float frequency);
    float processSquare(float frequency, float pulseWidth);
    float processTriangle(float frequency);
    float processNoise();
};

//==============================================================================
/**
    Multimode filter with smoothed parameters
*/
class ZenithFilter
{
public:
    ZenithFilter() = default;

    void setType(FilterType type) [[maybe_unused]] { type_ = type; }
    void setSampleRate(double sampleRate) [[maybe_unused]];
    void setCutoff(float cutoffHz) [[maybe_unused]];
    void setResonance(float resonance) [[maybe_unused]];
    void setDrive(float drive) [[maybe_unused]] { drive_ = drive; }
    void reset();

    /**
     * @brief Process one sample
     * @param input Input sample
     * @return Filtered sample
     */
    float processSample(float input);

private:
    FilterType type_ = FilterType::Lowpass;
    double sampleRate_ = 44100.0;

    // Smoothed parameters to avoid zipper noise
    juce::SmoothedValue<float> cutoffSmoothed_;
    juce::SmoothedValue<float> resonanceSmoothed_;

    float drive_ = 1.0f;

    // State variables filter implementation
    float v0_ = 0.0f, v1_ = 0.0f, v2_ = 0.0f;
    float ic1eq_ = 0.0f, ic2eq_ = 0.0f;
};

//==============================================================================
/**
    Per-Voice Effects Chain
*/
class ZenithEffects
{
public:
    ZenithEffects() = default;

    void setSampleRate(double sampleRate) { sampleRate_ = sampleRate; }
    void reset() { /* No state to reset for simple distortion */ }

    void setDistortion(float amount) { distortionAmount_ = amount; }
    void setChorus(float amount) { chorusAmount_ = amount; }

    void process(float& left, float& right);

private:
    double sampleRate_ = 44100.0;
    float distortionAmount_ = 0.0f;
    float chorusAmount_ = 0.0f;
    
    // Simple chorus LFO
    float chorusPhase_ = 0.0f;
};

//==============================================================================
/**
    Voice for ZenithPolySynth - RT-safe polyphonic voice
*/
class ZenithPolySynthVoice : public juce::SynthesiserVoice
{
public:
    ZenithPolySynthVoice();
    ~ZenithPolySynthVoice() override = default;

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) [[maybe_unused]] override;
    void stopNote(float velocity, bool allowTailOff) [[maybe_unused]] override;
    void pitchWheelMoved(int newPitchWheelValue) [[maybe_unused]] override;
    void controllerMoved(int controllerNumber, int newControllerValue) [[maybe_unused]] override;
    void channelPressureChanged(int newChannelPressureValue) [[maybe_unused]] override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) [[maybe_unused]] override;

    //==========================================================================
    // Parameter setters (called from message thread or via atomic parameters)
    //==========================================================================
    void setOsc1Waveform(OscillatorWaveform waveform) [[maybe_unused]] { osc1_.setWaveform(waveform); }
    void setOsc2Waveform(OscillatorWaveform waveform) [[maybe_unused]] { osc2_.setWaveform(waveform); }
    void setOsc3Waveform(OscillatorWaveform waveform) [[maybe_unused]] { osc3_.setWaveform(waveform); }

    void setOsc1Detune(float cents) [[maybe_unused]] { osc1_.setDetune(cents); }
    void setOsc2Detune(float cents) [[maybe_unused]] { osc2_.setDetune(cents); }
    void setOsc3Detune(float cents) [[maybe_unused]] { osc3_.setDetune(cents); }

    void setOsc1Mix(float mix) [[maybe_unused]] { osc1Mix_ = mix; }
    void setOsc2Mix(float mix) [[maybe_unused]] { osc2Mix_ = mix; }
    void setOsc3Mix(float mix) [[maybe_unused]] { osc3Mix_ = mix; }

    void setUnisonVoices(int voices) [[maybe_unused]] { unisonVoices_ = juce::jlimit(1, 7, voices); }
    void setUnisonDetune(float cents) [[maybe_unused]] { unisonDetune_ = cents; }

    void setFilterType(FilterType type) [[maybe_unused]] { filter1_.setType(type); }
    void setFilterCutoff(float cutoff) [[maybe_unused]] { filterCutoff_ = cutoff; }
    void setFilterResonance(float resonance) [[maybe_unused]] { filter1_.setResonance(resonance); }
    void setFilterDrive(float drive) [[maybe_unused]] { filter1_.setDrive(drive); }
    
    void setFilter2Type(FilterType type) [[maybe_unused]] { filter2_.setType(type); }
    void setFilter2Cutoff(float cutoff) [[maybe_unused]] { filter2Cutoff_ = cutoff; }
    void setFilter2Resonance(float resonance) [[maybe_unused]] { filter2_.setResonance(resonance); }
    void setFilterRouting(bool serial) [[maybe_unused]] { filterSerial_ = serial; }

    void setAmpEnvelope(float attack, float decay, float sustain, float release) [[maybe_unused]];
    void setModEnvelope(float attack, float decay, float sustain, float release) [[maybe_unused]];

    void setLFO1(float rate, float amount, LFOTarget target) [[maybe_unused]];
    void setLFO2(float rate, float amount, LFOTarget target) [[maybe_unused]];

    void setGlideTime(float glideTimeSeconds) [[maybe_unused]] { glideTime_ = glideTimeSeconds; }
    void setMonoMode(bool mono) [[maybe_unused]] { monoMode_ = mono; }
    void setQualityPreset(QualityPreset quality) [[maybe_unused]] { qualityPreset_ = quality; }
    
    void setDistortion(float amount) { effects_.setDistortion(amount); }
    void setChorus(float amount) { effects_.setChorus(amount); }

    void setSampleRate(double sampleRate) [[maybe_unused]];

    //==========================================================================
    // Modulation Matrix Control
    //==========================================================================

    /**
     * @brief Set a modulation slot
     * @param slotIndex Slot index (0-7)
     * @param source Modulation source
     * @param destination Modulation destination
     * @param amount Modulation amount (-1 to +1)
     */
    void setModulationSlot(int slotIndex, ModulationSource source,
                          ModulationDestination destination, float amount);

    /**
     * @brief Set MIDI controller values (called from controllerMoved)
     */
    void setModWheel(float value) [[maybe_unused]] { modWheel_ = juce::jlimit(0.0f, 1.0f, value); }
    void setAftertouch(float value) [[maybe_unused]] { aftertouch_ = juce::jlimit(0.0f, 1.0f, value); }

    /**
     * @brief Get current output amplitude for voice stealing
     * @return Current amplitude level (0-1)
     */
    float getCurrentAmplitude() const { return currentAmplitude_; }

private:
    //==========================================================================
    // Oscillators
    //==========================================================================
    ZenithOscillator osc1_, osc2_, osc3_;
    std::array<ZenithOscillator, 7> unisonOscillators_; // For supersaw unison

    float osc1Mix_ = 1.0f;
    float osc2Mix_ = 0.5f;
    float osc3Mix_ = 0.0f;

    int unisonVoices_ = 1;
    float unisonDetune_ = 10.0f; // cents

    //==========================================================================
    // Filters (Dual)
    //==========================================================================
    ZenithFilter filter1_;
    ZenithFilter filter2_;
    float filterCutoff_ = 2000.0f;
    float filter2Cutoff_ = 2000.0f;
    bool filterSerial_ = true;

    //==========================================================================
    // Envelopes
    //==========================================================================
    juce::ADSR ampEnvelope_;
    juce::ADSR::Parameters ampEnvParams_;

    juce::ADSR modEnvelope_;
    juce::ADSR::Parameters modEnvParams_;

    //==========================================================================
    // LFOs
    //==========================================================================
    struct LFO
    {
        float phase = 0.0f;
        float rate = 5.0f;      // Hz
        float amount = 0.0f;
        LFOTarget target = LFOTarget::FilterCutoff;

        float getNextValue(double sampleRate)
        {
            float value = std::sin(phase * juce::MathConstants<float>::twoPi);
            phase += rate / static_cast<float>(sampleRate);
            if (phase >= 1.0f)
                phase -= 1.0f;
            return value * amount;
        }

        void reset() { phase = 0.0f; }
    };

    LFO lfo1_, lfo2_;
    
    //==========================================================================
    // Effects
    //==========================================================================
    ZenithEffects effects_;

    //==========================================================================
    // Voice state
    //==========================================================================
    double sampleRate_ = 44100.0;
    int currentMidiNote_ = 0;
    float currentFrequency_ = 440.0f;
    float targetFrequency_ = 440.0f;
    float glideTime_ = 0.0f;
    bool monoMode_ = false;
    float velocity_ = 1.0f;
    QualityPreset qualityPreset_ = QualityPreset::High;
    float currentAmplitude_ = 0.0f; // For voice stealing prioritization

    //==========================================================================
    // Modulation Matrix
    //==========================================================================

    // Fixed-size modulation slots (pre-allocated for RT-safety)
    static constexpr int kNumModSlots = 8;
    std::array<ModulationSlot, kNumModSlots> modulationSlots_;

    // Computed modulation state (updated per buffer)
    ModulationState modulationState_;

    // MIDI controller values
    float modWheel_ = 0.0f;      // CC#1
    float aftertouch_ = 0.0f;    // Channel pressure
    float pan_ = 0.0f;           // Stereo pan (-1 to +1)
    float oscShape_ = 0.5f;      // Oscillator shape/PWM (0 to 1)

    //==========================================================================
    // Helper methods
    //==========================================================================
    void updateFrequency();
    float applyLFOs();

    /**
     * @brief Compute modulation matrix values for current sample
     *
     * This method:
     * 1. Reads all modulation sources (LFOs, envelopes, velocity, MIDI)
     * 2. Computes contributions to each destination
     * 3. Stores results in modulationState_ for RT-safe access
     *
     * Called once per audio buffer before processing samples.
     * RT-safe: no allocations, uses pre-allocated arrays.
     */
    void computeModulation();

    /**
     * @brief Get modulation source value
     * @param source Source to read
     * @return Value in appropriate range for source type
     */
    float getModulationSourceValue(ModulationSource source);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthVoice)
};

//==============================================================================
/**
    Sound for ZenithPolySynth
*/
class ZenithPolySynthSound : public juce::SynthesiserSound
{
public:
    ZenithPolySynthSound() = default;
    ~ZenithPolySynthSound() override = default;

    bool appliesToNote(int midiNoteNumber) override { return true; }
    bool appliesToChannel(int midiChannel) override { return true; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthSound)
};

//==============================================================================
/**
    ZenithPolySynth AudioProcessor

    Main synthesizer processor that manages voices and parameters.
*/
class ZenithPolySynthProcessor : public juce::Synthesiser,
                                  public juce::AudioProcessor
{
public:
    //==========================================================================
    // Parameter indices - must match order in constructor
    //==========================================================================
    enum Parameters
    {
        // Oscillators
        Osc1Wave = 0,
        Osc1Detune,
        Osc1Mix,
        Osc2Wave,
        Osc2Detune,
        Osc2Mix,
        Osc3Wave,
        Osc3Detune,
        Osc3Mix,

        // Unison
        UnisonVoices,
        UnisonDetune,

        // Filter 1
        FilterType,
        FilterCutoff,
        FilterResonance,
        FilterDrive,
        
        // Filter 2
        Filter2Type,
        Filter2Cutoff,
        Filter2Resonance,
        FilterRouting, // 0=Serial, 1=Parallel

        // Amp Envelope
        AmpAttack,
        AmpDecay,
        AmpSustain,
        AmpRelease,

        // Mod Envelope
        ModAttack,
        ModDecay,
        ModSustain,
        ModRelease,

        // LFO 1
        LFO1Rate,
        LFO1Amount,
        LFO1Target,

        // LFO 2
        LFO2Rate,
        LFO2Amount,
        LFO2Target,
        
        // Effects
        DistortionAmount,
        ChorusAmount,

        // Global
        GlideTime,
        MonoMode,
        MasterGain,

        // CPU Optimization
        MaxVoices,
        QualitySetting,

        NumParameters
    };

    //==========================================================================
    ZenithPolySynthProcessor();
    ~ZenithPolySynthProcessor() override = default;

    //==========================================================================
    // AudioProcessor interface
    //==========================================================================
    const juce::String getName() const override { return "Zenith Poly Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) [[maybe_unused]] override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) [[maybe_unused]] override;

protected:
    //==========================================================================
    // Custom voice stealing - JUCE 8: findFreeVoice no longer virtual
    //==========================================================================
    juce::SynthesiserVoice* findFreeVoice(juce::SynthesiserSound* soundToPlay,
                                           int midiChannel,
                                           int midiNoteNumber,
                                           bool stealIfNoneAvailable) const override;

private:
    //==========================================================================
    // Update voices with current parameters (called from audio thread)
    //==========================================================================
    void updateVoiceParameters();
    void updateVoiceCount();

    juce::SmoothedValue<float> masterGainSmoothed_;
    int currentMaxVoices_ = 16;
    
    // Cached parameter pointers for fast access
    std::vector<juce::RangedAudioParameter*> cachedParams_;

#if JUCE_DEBUG
    //==========================================================================
    // Debug profiling helpers
    //==========================================================================
    int maxActiveVoices_ = 0;
    double maxBlockProcessingTime_ = 0.0;
    int blockCount_ = 0;

    void logCPUStats();
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthProcessor)
};

//==============================================================================
/**
    Zenith Poly Synth instrument wrapper

    Exposes the synthesizer with comprehensive metadata and presets.
*/
class ZenithPolySynth : public InstrumentBase
{
public:
    ZenithPolySynth();
    ~ZenithPolySynth() override = default;

    /**
     * @brief Create metadata for this instrument
     */
    static InstrumentMetadata createMetadata();

private:
    void registerPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynth)
};

} // namespace zenith

