/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_effects/juce_audio_effects.h>
#include <array>
#include <memory>

namespace zenith {

//==============================================================================
// EFFECT TYPES
//==============================================================================

enum class EffectType {
    None,
    Reverb,
    Delay,
    Chorus,
    Phaser,
    Flanger,
    Distortion,
    Compressor,
    EQ,
    Limiter
};

//==============================================================================
// PROFESSIONAL REVERB
//==============================================================================
/**
 * High-quality reverb using multiple parallel delay networks
 * Matches the quality of premium plugins (Valence, etc.)
 */
class ZenithReverb {
public:
    ZenithReverb();

    void setSampleRate(double sr);
    void setRoomSize(float size);       // 0-1
    void setDamping(float damping);       // 0-1
    void setWidth(float width);          // 0-1 (stereo width)
    void setMix(float mix);              // 0-1 (wet/dry)
    void setPreDelay(float delayMs);     // 0-100ms
    void setDecay(float decay);          // 0-1 (reverb time)

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    juce::dsp::Reverb reverb_;
    juce::dsp::Reverb::Parameters params_;
    float preDelayMix_ = 0.0f;
    std::array<float, 2> preDelayBuffer_ = {0.0f, 0.0f};
    int preDelaySamples_ = 0;
    int preDelayIndex_ = 0;
};

//==============================================================================
// STEREO DELAY WITH PING-PONG
//==============================================================================

class ZenithDelay {
public:
    ZenithDelay();

    void setSampleRate(double sr);
    void setTime(float timeSeconds);   // 0-2 seconds
    void setFeedback(float fb);        // 0-0.95 (self-oscillation at 1)
    void setMix(float mix);            // 0-1
    void setPingPong(bool pp);        // Ping-pong mode
    void setSync(bool sync);           // BPM sync
    void setSyncRate(float noteLength); // 1/4, 1/8, etc.

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    std::unique_ptr<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>>
        delayLineL_, delayLineR_;

    float time_ = 0.25f;
    float feedback_ = 0.5f;
    float mix_ = 0.3f;
    bool pingPong_ = false;
    bool sync_ = false;

    float lastLeft_ = 0.0f;
    float lastRight_ = 0.0f;
    double sampleRate_ = 44100.0;
};

//==============================================================================
// CHORUS / ENSEMBLE
//==============================================================================

class ZenithChorus {
public:
    ZenithChorus();

    void setSampleRate(double sr);
    void setRate(float rateHz);         // LFO rate 0.1-10Hz
    void setDepth(float depth);          // Modulation depth 0-1
    void setVoices(int voices);         // 1-8 voices
    void setMix(float mix);             // 0-1
    void setSpread(float spread);        // Stereo spread 0-1

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int MAX_VOICES = 8;

    struct VoiceState {
        double phase = 0.0;
        float gain = 1.0f;
        float pan = 0.0f;
    };

    std::array<VoiceState, MAX_VOICES> voices_;
    int numVoices_ = 4;

    float rate_ = 0.5f;
    float depth_ = 0.5f;
    float mix_ = 0.5f;
    float spread_ = 0.5f;

    double sampleRate_ = 44100.0;

    juce::Random random_;
};

//==============================================================================
// PHASER
//==============================================================================

class ZenithPhaser {
public:
    ZenithPhaser();

    void setSampleRate(double sr);
    void setRate(float rateHz);         // LFO rate
    void setDepth(float depth);          // Modulation depth
    void setFeedback(float fb);          // Feedback amount
    void setStages(int stages);         // 2-12 stages
    void setMix(float mix);             // 0-1

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int MAX_STAGES = 12;

    // Allpass filter state per stage
    std::array<std::array<float, 2>, MAX_STAGES> allpassState_;

    int numStages_ = 4;
    double lfoPhase_ = 0.0;
    float rate_ = 0.5f;
    float depth_ = 0.5f;
    float feedback_ = 0.5f;
    float mix_ = 0.5f;

    float lastFeedbackSample_ = 0.0f;
    double sampleRate_ = 44100.0;
};

//==============================================================================
// DISTORTION / SATURATION
//==============================================================================

class ZenithDistortion {
public:
    enum class Type {
        None,
        SoftClip,       // Smooth saturation
        HardClip,       // Digital clipping
        Bitcrush,       // Bit reduction
        Wavefold,       // Wavefolding
        HalfWave,       // Half-wave rectification
        FullWave        // Full-wave rectification
    };

    ZenithDistortion();

    void setType(Type type);
    void setDrive(float drive);        // 0-1 (input gain)
    void setTone(float tone);         // 0-1 (lowpass filter)
    void setMix(float mix);             // 0-1

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    Type type_ = Type::SoftClip;
    float drive_ = 0.0f;
    float tone_ = 0.5f;
    float mix_ = 0.0f;

    // Tone filter state
    float toneZ1_ = 0.0f;

    float applyDistortion(float input) const;

    // Waveshaping functions
    static float softClip(float x);
    static float hardClip(float x);
    static float wavefold(float x, float amount);
};

//==============================================================================
// SIDECHAIN INPUT TYPE
//==============================================================================

enum class SidechainSource {
    None,           ///< No sidechain
    External,        ///< External sidechain input
    Kick,           ///< Internal kick drum
    Snare,          ///< Internal snare
    Bus,            ///< Bus/track source
    LFO             ///< LFO-modulated sidechain
};

//==============================================================================
// MASTER COMPRESSOR / LIMITER WITH SIDECHAIN
//==============================================================================

class ZenithCompressor {
public:
    ZenithCompressor();

    void setSampleRate(double sr);
    void setThreshold(float thresholdDb);   // -60 to 0 dB
    void setRatio(float ratio);              // 1:1 to 20:1
    void setKnee(float kneeDb);           // 0 to 24 dB
    void setAttack(float attackMs);
    void setRelease(float releaseMs);
    void setMakeupGain(float makeupDb);
    void setAutoMakeup(bool autoMakeup);

    //==========================================================================
    // Sidechain Control
    //==========================================================================

    /**
     * @brief Enable sidechain input
     * @param enabled True to use sidechain for gain reduction
     */
    void setSidechainEnabled(bool enabled) { sidechainEnabled_ = enabled; }

    /**
     * @brief Set sidechain source
     */
    void setSidechainSource(SidechainSource source) { sidechainSource_ = source; }

    /**
     * @brief Set sidechain filter frequency (HPF for ducking)
     * @param freq Frequency in Hz (20-2000Hz)
     */
    void setSidechainFilterFreq(float freq) { sidechainFilterFreq_ = juce::jlimit(20.0f, 2000.0f, freq); }

    /**
     * @brief Set sidechain input buffer
     * @param sidechain Buffer containing sidechain signal
     */
    void setSidechainInput(const juce::AudioBuffer<float>& sidechain);

    /**
     * @brief Process with optional sidechain
     * @param buffer Main audio to process
     * @param sidechain Optional sidechain buffer (uses main buffer if null)
     */
    void process(juce::AudioBuffer<float>& buffer,
                const juce::AudioBuffer<float>* sidechain = nullptr);

    void reset();

    float getGainReduction() const { return gainReduction_; }

private:
    // Envelope follower state
    float envelope_ = 0.0f;

    float threshold_ = -20.0f;
    float ratio_ = 4.0f;
    float knee_ = 4.0f;
    float attackMs_ = 5.0f;
    float releaseMs_ = 50.0f;
    float makeupDb_ = 0.0f;
    bool autoMakeup_ = true;

    // Sidechain state
    bool sidechainEnabled_ = false;
    SidechainSource sidechainSource_ = SidechainSource::None;
    float sidechainFilterFreq_ = 100.0f;
    float sidechainFilterZ1_ = 0.0f;

    float gainReduction_ = 0.0f;
    double sampleRate_ = 44100.0;

    float calculateGain(float inputDb);
};

//==============================================================================
// MASTER LIMITER (BRICKWALL)
//==============================================================================

class ZenithLimiter {
public:
    ZenithLimiter();

    void setSampleRate(double sr);
    void setThreshold(float thresholdDb);   // -20 to 0 dB
    void setRelease(float releaseMs);
    void setCeiling(float ceilingDb);     // Max output level

    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    float getGainReduction() const { return gainReduction_; }

private:
    float envelope_ = 0.0f;
    float threshold_ = -0.1f;
    float ceiling_ = -0.1f;
    float releaseMs_ = 10.0f;
    float gainReduction_ = 0.0f;
    double sampleRate_ = 44100.0;
};

//==============================================================================
// PER-OSCILLATOR FX SENDS
//==============================================================================

/**
 * Per-oscillator effect send levels
 */
struct OscillatorFXSends {
    float reverbSend = 0.0f;    ///< Reverb send amount (0-1)
    float delaySend = 0.0f;     ///< Delay send amount (0-1)
    float chorusSend = 0.0f;    ///< Chorus send amount (0-1)
    float phaserSend = 0.0f;    ///< Phaser send amount (0-1)
    float distortionSend = 0.0f; ///< Distortion send amount (0-1)
};

//==============================================================================
// EFFECTS CHAIN WITH SIDECHAIN
//==============================================================================

/**
 * Master effects processor for synth output with per-oscillator sends
 * Processes in series: Reverb -> Delay -> Chorus -> Phaser -> Distortion -> Compressor -> Limiter
 */
class ZenithEffectsChain {
public:
    ZenithEffectsChain();

    void setSampleRate(double sr);
    void reset();

    //==========================================================================
    // Effect Access
    //==========================================================================

    ZenithReverb& getReverb() { return reverb_; }
    ZenithDelay& getDelay() { return delay_; }
    ZenithChorus& getChorus() { return chorus_; }
    ZenithPhaser& getPhaser() { return phaser_; }
    ZenithDistortion& getDistortion() { return distortion_; }
    ZenithCompressor& getCompressor() { return compressor_; }
    ZenithLimiter& getLimiter() { return limiter_; }

    //==========================================================================
    // Per-Oscillator Sends
    //==========================================================================

    /**
     * @brief Set FX sends for oscillator 1
     */
    void setOsc1Sends(const OscillatorFXSends& sends) { osc1Sends_ = sends; }

    /**
     * @brief Set FX sends for oscillator 2
     */
    void setOsc2Sends(const OscillatorFXSends& sends) { osc2Sends_ = sends; }

    /**
     * @brief Set FX sends for oscillator 3
     */
    void setOsc3Sends(const OscillatorFXSends& sends) { osc3Sends_ = sends; }

    /**
     * @brief Get sends for oscillator
     */
    const OscillatorFXSends& getOscSends(int oscIndex) const {
        switch (oscIndex) {
            case 0: return osc1Sends_;
            case 1: return osc2Sends_;
            case 2: return osc3Sends_;
            default: return osc1Sends_;
        }
    }

    //==========================================================================
    // Sidechain Input
    //==========================================================================

    /**
     * @brief Set sidechain input for compressor ducking
     */
    void setSidechainInput(const juce::AudioBuffer<float>* sidechain) {
        sidechainInput_ = sidechain;
    }

    //==========================================================================
    // Master Bypass
    //==========================================================================

    void setEffectBypass(EffectType type, bool bypassed);
    bool isEffectBypassed(EffectType type) const;

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Process effects chain with sidechain support
     * @param buffer Main audio buffer
     * @param osc1Buffer Optional separate buffer for OSC1
     * @param osc2Buffer Optional separate buffer for OSC2
     * @param osc3Buffer Optional separate buffer for OSC3
     * @param sidechain Optional sidechain input
     */
    void process(juce::AudioBuffer<float>& buffer,
                juce::AudioBuffer<float>* osc1Buffer = nullptr,
                juce::AudioBuffer<float>* osc2Buffer = nullptr,
                juce::AudioBuffer<float>* osc3Buffer = nullptr,
                const juce::AudioBuffer<float>* sidechain = nullptr);

private:
    ZenithReverb reverb_;
    ZenithDelay delay_;
    ZenithChorus chorus_;
    ZenithPhaser phaser_;
    ZenithDistortion distortion_;
    ZenithCompressor compressor_;
    ZenithLimiter limiter_;

    // Per-oscillator sends
    OscillatorFXSends osc1Sends_;
    OscillatorFXSends osc2Sends_;
    OscillatorFXSends osc3Sends_;

    // Sidechain input reference
    const juce::AudioBuffer<float>* sidechainInput_ = nullptr;

    // Bypass states
    bool reverbBypass_ = false;
    bool delayBypass_ = false;
    bool chorusBypass_ = false;
    bool phaserBypass_ = false;
    bool distortionBypass_ = false;
    bool compressorBypass_ = false;
    // Limiter is never bypassed (safety)

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /**
     * @brief Apply FX sends to a buffer
     */
    void applyFXSends(juce::AudioBuffer<float>& dest,
                     const juce::AudioBuffer<float>& source,
                     const OscillatorFXSends& sends,
                     int numSamples);
};

} // namespace zenith
