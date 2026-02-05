/**
 * @file ZenithAdvancedEffects.h
 * @brief Premium Quality Audio Effects for Zenith DAW
 *
 * Professional-grade DSP effects matching commercial plugin quality:
 * - Algorithmic Reverb (Moogerfooger-style)
 * - Multi-mode Delay (tape, BBD, ping-pong)
 * - Distortion (tube, bitcrush, wavefolding)
 * - Modulation (chorus, flanger, phaser)
 * - Vocoder (band-limited, Sennheiser-style)
 *
 * @date 2025-02-01
 * @version 1.0
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

namespace zenith {

//==============================================================================
// ALGORITHMIC REVERB (Moogerfooger MF-104M Style)
//==============================================================================

/**
 * @class AlgorithmicReverb
 * @brief High-quality algorithmic reverb inspired by Moogerfooger MF-104M
 *
 * Features:
 * - 8 parallel delay lines for lush decay
 * - Diffusion network for smooth reverb tail
 * - 3-band decay time control
 * - Modulated delay times for richness
 * - Low/high frequency damping
 */
class AlgorithmicReverb {
public:
    AlgorithmicReverb();
    ~AlgorithmicReverb() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float wetLevel);

    // Parameters
    void setRoomSize(float size) { roomSize_ = juce::jlimit(0.0f, 1.0f, size); updateDecayTimes(); }
    void setDamping(float damping) { damping_ = juce::jlimit(0.0f, 1.0f, damping); }
    void setWetLevel(float wet) { wetLevel_ = juce::jlimit(0.0f, 1.0f, wet); }
    void setDecayTime(float decay) { decayTime_ = juce::jlimit(0.1f, 10.0f, decay); updateDecayTimes(); }
    void setPreDelay(float predelay) { preDelayTime_ = juce::jlimit(0.0f, 0.2f, predelay); }
    void setDiffusion(float diffusion) { diffusion_ = juce::jlimit(0.0f, 1.0f, diffusion); }
    void setModulation(float mod) { modulationDepth_ = juce::jlimit(0.0f, 1.0f, mod); }

private:
    void updateDecayTimes();

    // Delay lines (8 parallel for richness)
    static constexpr int numDelays = 8;
    std::array<std::unique_ptr<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange>>, numDelays> delays_;

    // Diffusion network (allpass filters)
    static constexpr int numDiffusionStages = 4;
    std::array<std::unique_ptr<juce::dsp::FirstOrderTPTFilter<float>>, numDiffusionStages> diffusionFilters_;

    // Parameters
    float roomSize_ = 0.5f;
    float damping_ = 0.5f;
    float wetLevel_ = 0.3f;
    float decayTime_ = 2.0f;  // seconds
    float preDelayTime_ = 0.02f;
    float diffusion_ = 0.5f;
    float modulationDepth_ = 0.1f;

    // Modulation LFO
    juce::dsp::Oscillator<float> lfo_;
    float lfoPhase_ = 0.0f;

    // State
    double sampleRate_ = 44100.0;
    juce::AudioBuffer<float> wetBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmicReverb)
};

//==============================================================================
// MULTI-MODE DELAY
//==============================================================================

/**
 * @enum DelayMode
 * @brief Different delay character emulations
 */
enum class DelayMode {
    Tape,           // Analog tape with wow/flutter
    BBD,            // Bucket Brigade Device (warmer, filtered)
    Digital,        // Clean digital delay
    PingPong        // Stereo ping-pong
};

/**
 * @class MultiModeDelay
 * @brief Versatile delay effect with multiple character modes
 *
 * Features:
 * - 4 delay modes (tape, BBD, digital, ping-pong)
 * - Tape saturation and compression
 * - LFO modulation for chorus effects
 * - Feedback with tone control
 * - Ping-pong stereo panning
 */
class MultiModeDelay {
public:
    MultiModeDelay();
    ~MultiModeDelay() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameters
    void setMode(DelayMode mode) { mode_ = mode; }
    void setTime(float timeSeconds) { delayTime_ = juce::jlimit(0.0f, 2.0f, timeSeconds); }
    void setFeedback(float feedback) { feedback_ = juce::jlimit(0.0f, 0.95f, feedback); }
    void setMix(float mix) { mix_ = juce::jlimit(0.0f, 1.0f, mix); }
    void setModulation(float mod) { modulationDepth_ = juce::jlimit(0.0f, 1.0f, mod); }
    void setFilter(float filter) { filterAmount_ = juce::jlimit(0.0f, 1.0f, filter); }
    void setSaturation(float sat) { saturation_ = juce::jlimit(0.0f, 1.0f, sat); }

private:
    void updateDelayTimes();

    // Delay lines
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange> leftDelay_;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange> rightDelay_;

    // Filters (BBD tone control)
    juce::dsp::FirstOrderTPTFilter<float> inputFilter_;
    juce::dsp::FirstOrderTPTFilter<float> feedbackFilter_;

    // Tape saturation (soft clipping)
    float applyTapeSaturation(float sample);

    // Modulation
    juce::dsp::Oscillator<float> lfo_;
    float lfoPhase_ = 0.0f;
    float lfoPhaseRight_ = 0.0f;  // Offset for stereo

    // Parameters
    DelayMode mode_ = DelayMode::Tape;
    float delayTime_ = 0.5f;
    float feedback_ = 0.5f;
    float mix_ = 0.3f;
    float modulationDepth_ = 0.1f;
    float filterAmount_ = 0.5f;
    float saturation_ = 0.3f;

    // State
    double sampleRate_ = 44100.0;
    int maxDelaySamples_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiModeDelay)
};

//==============================================================================
// DISTORTION (Multi-Type)
//==============================================================================

/**
 * @enum DistortionType
 * @brief Distortion circuit emulations
 */
enum class DistortionType {
    Tube,           // Vacuum tube preamp (soft clipping)
    Bitcrush,       // Bit reduction and sample rate reduction
    Wavefolder,     // West-coast wavefolding (rich harmonics)
    Fuzz,           // Germanium fuzz (hard clipping)
    Diode           // Diode ladder clipper
};

/**
 * @class MultiDistortion
 * @brief Collection of professional distortion types
 *
 * Features:
 * - 5 distortion algorithms
 * - Tone control (bass/treble)
 * - Mix control for parallel processing
 * - Input drive with automatic gain compensation
 */
class MultiDistortion {
public:
    MultiDistortion();
    ~MultiDistortion() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameters
    void setType(DistortionType type) { type_ = type; }
    void setDrive(float drive) { drive_ = juce::jlimit(0.0f, 1.0f, drive); }
    void setTone(float tone) { tone_ = juce::jlimit(0.0f, 1.0f, tone); updateFilters(); }
    void setMix(float mix) { mix_ = juce::jlimit(0.0f, 1.0f, mix); }
    void setBitDepth(float depth) { bitDepth_ = juce::jlimit(1.0f, 24.0f, depth); }
    void setSampleRate(float rate) { sampleRateReduction_ = juce::jlimit(1.0f, 64.0f, rate); }

private:
    void updateFilters();

    // Distortion algorithms
    float applyTubeDistortion(float sample);
    float applyBitcrush(float sample);
    float applyWavefolder(float sample);
    float applyFuzz(float sample);
    float applyDiodeClipper(float sample);

    // Tone control (bass/treble)
    juce::dsp::ProcessorChain<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
                              juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>> toneStack_;

    // Parameters
    DistortionType type_ = DistortionType::Tube;
    float drive_ = 0.5f;
    float tone_ = 0.5f;
    float mix_ = 1.0f;
    float bitDepth_ = 16.0f;
    float sampleRateReduction_ = 1.0f;

    // State
    double sampleRate_ = 44100.0;
    float sampleHold_ = 0.0f;
    int sampleHoldCounter_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiDistortion)
};

//==============================================================================
// CHORUS/FLANGER/PHASER
//==============================================================================

/**
 * @enum ModulationType
 * @brief Modulation effect types
 */
enum class ModulationType {
    Chorus,         // Multi-voice chorus
    Flanger,        // Through-zero flanging
    Phaser          // 6-stage phaser
};

/**
 * @class ModulationEffect
 * @brief Professional modulation effects ensemble
 *
 * Features:
 * - 4-voice chorus (Lush, Dimension-style)
 * - Through-zero flanger (negative feedback)
 * - 6-stage phaser (MXR Phase 90 style)
 * - Stereo width control
 * - LFO rate and depth
 */
class ModulationEffect {
public:
    ModulationEffect();
    ~ModulationEffect() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    // Parameters
    void setType(ModulationType type) { type_ = type; }
    void setRate(float rateHz) { rate_ = juce::jlimit(0.01f, 20.0f, rateHz); }
    void setDepth(float depth) { depth_ = juce::jlimit(0.0f, 1.0f, depth); }
    void setFeedback(float feedback) { feedback_ = juce::jlimit(-1.0f, 1.0f, feedback); }
    void setVoices(int voices) { numVoices_ = juce::jlimit(1, 8, voices); }
    void setSpread(float spread) { spread_ = juce::jlimit(0.0f, 1.0f, spread); }
    void setMix(float mix) { mix_ = juce::jlimit(0.0f, 1.0f, mix); }

private:
    void updateLFO();

    // Chorus voices
    static constexpr int maxVoices = 8;
    std::array<std::unique_ptr<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>>, maxVoices> chorusDelays_;
    std::array<float, maxVoices> lfoPhases_;

    // Phaser allpass filters (6 stages)
    static constexpr int numPhaserStages = 6;
    std::array<std::unique_ptr<juce::dsp::FirstOrderTPTFilter<float>>, numPhaserStages> phaserStages_;

    // Flanger delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange> flangerDelay_;

    // LFOs
    juce::dsp::Oscillator<float> lfo_;
    juce::dsp::Oscillator<float> lfoSlow_;

    // Parameters
    ModulationType type_ = ModulationType::Chorus;
    float rate_ = 0.5f;      // Hz
    float depth_ = 0.5f;
    float feedback_ = 0.5f;
    int numVoices_ = 4;
    float spread_ = 0.5f;
    float mix_ = 0.5f;

    // State
    double sampleRate_ = 44100.0;
    juce::AudioBuffer<float> processBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationEffect)
};

//==============================================================================
// VOCODER (Band-Limited)
//==============================================================================

/**
 * @class Vocoder
 * @brief High-quality band-limited vocoder (Sennheiser VSM-201 style)
 *
 * Features:
 * - 16 band analyzers (30Hz - 16kHz)
 * - Log-spaced frequency bands (musical)
 * - Adjustable envelope follower response
 * - Formant shifting
 * - Carrier/modulator swap
 */
class Vocoder {
public:
    Vocoder();
    ~Vocoder() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& modulator,
                 juce::AudioBuffer<float>& carrier,
                 juce::AudioBuffer<float>& output);

    // Parameters
    void setNumBands(int bands) { numBands_ = juce::jlimit(8, 32, bands); updateBands(); }
    void setAttack(float attackMs) { attackMs_ = juce::jlimit(0.1f, 100.0f, attackMs); updateEnvelope(); }
    void setRelease(float releaseMs) { releaseMs_ = juce::jlimit(10.0f, 1000.0f, releaseMs); updateEnvelope(); }
    void setFormantShift(float shift) { formantShift_ = juce::jlimit(-12.0f, 12.0f, shift); updateBands(); }
    void setQFactor(float q) { q_ = juce::jlimit(1.0f, 20.0f, q); updateBands(); }
    void setMix(float mix) { mix_ = juce::jlimit(0.0f, 1.0f, mix); }
    void setGate(float threshold) { gateThreshold_ = juce::jlimit(-60.0f, 0.0f, threshold); }

private:
    void updateBands();
    void updateEnvelope();

    // Band filters
    static constexpr int maxBands = 32;
    std::array<std::unique_ptr<juce::dsp::IIR::Filter<float>>, maxBands * 2> bandFilters_;  // modulator + carrier
    std::array<std::unique_ptr<juce::dsp::IIR::Filter<float>>, maxBands> envelopeFollowers_;

    // Bandpass frequencies (log-spaced)
    std::array<double, maxBands> centerFrequencies_;
    std::array<float, maxBands> bandEnvelopes_;

    // Smoothing
    std::array<float, maxBands> smoothedEnvelopes_;

    // Parameters
    int numBands_ = 16;
    float attackMs_ = 5.0f;
    float releaseMs_ = 100.0f;
    float formantShift_ = 0.0f;
    float q_ = 4.0f;
    float mix_ = 1.0f;
    float gateThreshold_ = -60.0f;

    // State
    double sampleRate_ = 44100.0;
    juce::AudioBuffer<float> analysisBuffer_;
    juce::AudioBuffer<float> synthesisBuffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Vocoder)
};

} // namespace zenith
