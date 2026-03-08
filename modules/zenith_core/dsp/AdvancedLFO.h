/*
    Advanced LFO Implementation for Zenith Ultra Synth
    Features: All waveforms, BPM sync, phase offset, fade in/out, unipolar/bipolar, one-shot, LFO envelope
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <functional>

namespace zenith {

//==============================================================================
// LFO Waveforms
//==============================================================================

class LFOWaveforms {
public:
    enum class Type {
        Sine,
        Triangle,
        Saw,
        Square,
        SampleAndHold,
        Random,
        Noise,
        Custom
    };

    static float generate(Type type, float phase, juce::Random& rng) {
        switch (type) {
            case Type::Sine:
                return std::sin(phase * juce::MathConstants<float>::twoPi);

            case Type::Triangle:
                // Triangle: 0 → 1 → -1 → 0
                {
                    float p = phase * 4.0f;
                    if (p < 1.0f) return p;
                    if (p < 3.0f) return 2.0f - p;
                    return p - 4.0f;
                }

            case Type::Saw:
                return 2.0f * phase - 1.0f;

            case Type::Square:
                return (phase < 0.5f) ? 1.0f : -1.0f;

            case Type::SampleAndHold:
                // Generate new value at phase wrap
                if (phase < 0.01f && rng.nextFloat() < 0.1f) {
                    return rng.nextFloat() * 2.0f - 1.0f;
                }
                return 0.0f;  // Will be replaced by actual S&H state

            case Type::Random:
                // Smooth random interpolation
                return 0.0f;  // Handled by SmoothRandomLFO

            case Type::Noise:
                return rng.nextFloat() * 2.0f - 1.0f;

            case Type::Custom:
                return 0.0f;  // Use custom waveform

            default:
                return 0.0f;
        }
    }

    static juce::String getTypeName(Type type) {
        switch (type) {
            case Type::Sine: return "Sine";
            case Type::Triangle: return "Triangle";
            case Type::Saw: return "Saw";
            case Type::Square: return "Square";
            case Type::SampleAndHold: return "S&H";
            case Type::Random: return "Random";
            case Type::Noise: return "Noise";
            case Type::Custom: return "Custom";
            default: return "Unknown";
        }
    }

    static juce::StringArray getTypeNames() {
        return {"Sine", "Triangle", "Saw", "Square", "S&H", "Random", "Noise", "Custom"};
    }
};

//==============================================================================
// Smooth Random LFO (interpolated random values)
//==============================================================================

class SmoothRandomLFO {
public:
    SmoothRandomLFO() {
        generateNewTarget();
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
    }

    void setRate(float rateHz) {
        rateHz_ = juce::jlimit(0.01f, 200.0f, rateHz);
    }

    float processSample() {
        // Interpolate toward target
        float increment = rateHz_ / sampleRate_;

        currentValue_ += (targetValue_ - currentValue_) * increment;

        // Check if we've reached the target
        if (std::abs(targetValue_ - currentValue_) < 0.01f) {
            generateNewTarget();
        }

        return currentValue_;
    }

    void reset() {
        currentValue_ = 0.0f;
        targetValue_ = 0.0f;
        generateNewTarget();
    }

private:
    void generateNewTarget() {
        juce::Random rng;
        targetValue_ = rng.nextFloat() * 2.0f - 1.0f;
    }

    float sampleRate_ = 44100.0f;
    float rateHz_ = 4.0f;
    float currentValue_ = 0.0f;
    float targetValue_ = 0.0f;
};

//==============================================================================
// Sample and Hold LFO
//==============================================================================

class SampleAndHoldLFO {
public:
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
    }

    void setRate(float rateHz) {
        rateHz_ = juce::jlimit(0.01f, 200.0f, rateHz);
        phaseIncrement_ = rateHz_ / sampleRate_;
    }

    float processSample() {
        phase_ += phaseIncrement_;

        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
            // Generate new sample to hold
            juce::Random rng;
            currentValue_ = rng.nextFloat() * 2.0f - 1.0f;
        }

        return currentValue_;
    }

    void reset() {
        phase_ = 0.0f;
        currentValue_ = 0.0f;
    }

private:
    float sampleRate_ = 44100.0f;
    float rateHz_ = 4.0f;
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    float currentValue_ = 0.0f;
};

//==============================================================================
// Custom Waveform LFO (user-drawable)
//==============================================================================

class CustomWaveformLFO {
public:
    static constexpr int tableSize = 256;

    CustomWaveformLFO() {
        // Initialize with sine wave
        for (int i = 0; i < tableSize; ++i) {
            float phase = static_cast<float>(i) / tableSize;
            waveform_[i] = std::sin(phase * juce::MathConstants<float>::twoPi);
        }
    }

    void setSamplePoint(int index, float value) {
        index = juce::jlimit(0, tableSize - 1, index);
        waveform_[index] = juce::jlimit(-1.0f, 1.0f, value);
    }

    void setWaveform(const std::vector<float>& waveform) {
        int size = juce::jmin(static_cast<int>(waveform.size()), tableSize);
        std::copy(waveform.begin(), waveform.begin() + size, waveform_.begin());
    }

    void setPhase(float phase) {
        phase_ = juce::jlimit(0.0f, 1.0f, phase);
    }

    float processSample() {
        float indexFloat = phase_ * tableSize;
        int index1 = static_cast<int>(indexFloat) % tableSize;
        int index2 = (index1 + 1) % tableSize;
        float frac = indexFloat - index1;

        // Linear interpolation
        return waveform_[index1] * (1.0f - frac) + waveform_[index2] * frac;
    }

    const float* getWaveform() const { return waveform_.data(); }
    int getTableSize() const { return tableSize; }

private:
    float phase_ = 0.0f;
    std::array<float, tableSize> waveform_;
};

//==============================================================================
// Advanced LFO with All Features
//==============================================================================

class AdvancedLFO {
public:
    enum class TriggerMode {
        Free,           // Runs freely
        Retrigger,      // Resets on note on
        Single,         // One-shot mode
        Pattern         // Follows pattern
    };

    AdvancedLFO() {
        setSampleRate(44100.0f);
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updatePhaseIncrement();
    }

    // Waveform
    void setWaveType(LFOWaveforms::Type type) {
        waveType_ = type;
    }

    void setCustomWaveform(const float* data, int size) {
        customLFO_.setWaveform(std::vector<float>(data, data + size));
    }

    // Rate (0.01Hz to 200Hz)
    void setRate(float rateHz) {
        rateHz_ = juce::jlimit(0.01f, 200.0f, rateHz);
        updatePhaseIncrement();
    }

    // BPM Sync
    void setTempo(float bpm) {
        tempo_ = bpm;
        updateSyncRate();
    }

    void setSynced(bool synced) {
        synced_ = synced;
        updateSyncRate();
    }

    enum class SyncDivision {
        Whole = 1,
        Half = 2,
        Quarter = 4,
        Eighth = 8,
        EighthTriplet = 12,
        Sixteenth = 16,
        SixteenthTriplet = 24,
        ThirtySecond = 32
    };

    void setSyncDivision(SyncDivision div) {
        syncDivision_ = div;
        updateSyncRate();
    }

    // Phase
    void setPhaseOffset(float offsetDegrees) {
        phaseOffset_ = offsetDegrees / 360.0f;  // 0-1
    }

    void setPhase(float phase) {
        phase_ = juce::jlimit(0.0f, 1.0f, phase);
    }

    // Fade
    void setFadeIn(float timeSeconds) {
        fadeInTime_ = juce::jmax(0.0f, timeSeconds);
        fadeInInc_ = fadeInTime_ > 0 ? 1.0f / (fadeInTime_ * sampleRate_) : 1.0f;
    }

    void setFadeOut(float timeSeconds) {
        fadeOutTime_ = juce::jmax(0.0f, timeSeconds);
        // Fade out is handled by external gate
    }

    // Polarity
    void setBipolar(bool bipolar) {
        bipolar_ = bipolar;
    }

    // One-shot
    void setOneShot(bool oneShot) {
        oneShot_ = oneShot;
    }

    // Trigger mode
    void setTriggerMode(TriggerMode mode) {
        triggerMode_ = mode;
    }

    // LFO Envelope
    void setEnvelopeEnabled(bool enabled) {
        envelopeEnabled_ = enabled;
    }

    void setEnvelopeADSR(float a, float d, float s, float r) {
        envAttack_ = a;
        envDecay_ = d;
        envSustain_ = s;
        envRelease_ = r;
    }

    // Key trigger
    void trigger() {
        if (triggerMode_ == TriggerMode::Retrigger || triggerMode_ == TriggerMode::Single) {
            phase_ = phaseOffset_;
            oneShotComplete_ = false;

            // Start fade in
            fadeLevel_ = 0.0f;

            // Start envelope
            if (envelopeEnabled_) {
                envPhase_ = EnvelopePhase::Attack;
                envLevel_ = 0.0f;
            }
        }
    }

    void release() {
        if (envelopeEnabled_ && envPhase_ != EnvelopePhase::Idle) {
            envPhase_ = EnvelopePhase::Release;
        }
    }

    // Process
    float processSample() {
        // Update envelope
        updateEnvelope();

        // Update fade
        if (fadeLevel_ < 1.0f && fadeInInc_ > 0) {
            fadeLevel_ = juce::jmin(1.0f, fadeLevel_ + fadeInInc_);
        }

        // Check one-shot
        if (oneShot_ && oneShotComplete_) {
            return 0.0f;
        }

        // Update phase
        phase_ += phaseIncrement_;

        // Wrap phase
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;

            if (oneShot_) {
                oneShotComplete_ = true;
            }
        }

        // Add phase offset (doesn't accumulate)
        float modPhase = phase_ + phaseOffset_;
        if (modPhase >= 1.0f) modPhase -= 1.0f;

        // Generate waveform
        float output = generateWaveform(waveType_, modPhase);

        // Apply fade
        output *= fadeLevel_;

        // Apply envelope
        if (envelopeEnabled_) {
            output *= envLevel_;
        }

        // Apply unipolar if needed
        if (!bipolar_) {
            output = output * 0.5f + 0.5f;
        }

        return output;
    }

    void reset() {
        phase_ = phaseOffset_;
        fadeLevel_ = 0.0f;
        oneShotComplete_ = false;
        envPhase_ = EnvelopePhase::Idle;
        envLevel_ = 0.0f;
    }

    // Getters
    float getRate() const { return rateHz_; }
    float getPhase() const { return phase_; }
    bool isSynced() const { return synced_; }
    LFOWaveforms::Type getWaveType() const { return waveType_; }

private:
    float generateWaveform(LFOWaveforms::Type type, float phase) {
        switch (type) {
            case LFOWaveforms::Type::Random:
                return smoothRandom_.processSample();

            case LFOWaveforms::Type::SampleAndHold:
                return sampleAndHold_.processSample();

            case LFOWaveforms::Type::Custom:
                customLFO_.setPhase(phase);
                return customLFO_.processSample();

            default:
                return LFOWaveforms::generate(type, phase, random_);
        }
    }

    void updatePhaseIncrement() {
        if (synced_) {
            updateSyncRate();
        } else {
            phaseIncrement_ = rateHz_ / sampleRate_;
        }

        // Update dependent LFOs
        sampleAndHold_.setRate(rateHz_);
        sampleAndHold_.setSampleRate(sampleRate_);
        smoothRandom_.setRate(rateHz_);
        smoothRandom_.setSampleRate(sampleRate_);
    }

    void updateSyncRate() {
        if (!synced_) return;

        float beatsPerSecond = tempo_ / 60.0f;
        float division = static_cast<float>(syncDivision_);
        float rateHz = beatsPerSecond / division;

        phaseIncrement_ = rateHz / sampleRate_;
    }

    void updateEnvelope() {
        if (!envelopeEnabled_) {
            envLevel_ = 1.0f;
            return;
        }

        float inc = 1.0f / sampleRate_;

        switch (envPhase_) {
            case EnvelopePhase::Idle:
                envLevel_ = 0.0f;
                break;

            case EnvelopePhase::Attack:
                envLevel_ += inc / envAttack_;
                if (envLevel_ >= 1.0f) {
                    envLevel_ = 1.0f;
                    envPhase_ = EnvelopePhase::Decay;
                }
                break;

            case EnvelopePhase::Decay:
                float decayInc = inc / envDecay_;
                envLevel_ -= decayInc * (1.0f - envSustain_);
                if (envLevel_ <= envSustain_) {
                    envLevel_ = envSustain_;
                    envPhase_ = EnvelopePhase::Sustain;
                }
                break;

            case EnvelopePhase::Sustain:
                envLevel_ = envSustain_;
                break;

            case EnvelopePhase::Release:
                envLevel_ -= inc / envRelease_;
                if (envLevel_ <= 0.0f) {
                    envLevel_ = 0.0f;
                    envPhase_ = EnvelopePhase::Idle;
                }
                break;
        }
    }

    enum class EnvelopePhase {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    // Basic parameters
    float sampleRate_ = 44100.0f;
    float rateHz_ = 4.0f;
    float tempo_ = 120.0f;
    bool synced_ = false;
    SyncDivision syncDivision_ = SyncDivision::Eighth;
    float phase_ = 0.0f;
    float phaseOffset_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    LFOWaveforms::Type waveType_ = LFOWaveforms::Type::Sine;

    // Fade
    float fadeInTime_ = 0.0f;
    float fadeOutTime_ = 0.0f;
    float fadeInInc_ = 0.0f;
    float fadeLevel_ = 1.0f;

    // Polarity
    bool bipolar_ = true;

    // One-shot
    bool oneShot_ = false;
    bool oneShotComplete_ = false;

    // Trigger
    TriggerMode triggerMode_ = TriggerMode::Free;

    // LFO Envelope
    bool envelopeEnabled_ = false;
    float envAttack_ = 0.01f;
    float envDecay_ = 0.3f;
    float envSustain_ = 0.7f;
    float envRelease_ = 0.5f;
    EnvelopePhase envPhase_ = EnvelopePhase::Idle;
    float envLevel_ = 1.0f;

    // Special LFOs
    SmoothRandomLFO smoothRandom_;
    SampleAndHoldLFO sampleAndHold_;
    CustomWaveformLFO customLFO_;
    juce::Random random_;
};

//==============================================================================
// LFO Preset Shapes
//==============================================================================

class LFOPresets {
public:
    struct Preset {
        juce::String name;
        LFOWaveforms::Type waveform;
        float rate;          // Hz
        bool bipolar;
        juce::String description;
    };

    static std::vector<Preset> getPresets() {
        return {
            {"Vibrato", LFOWaveforms::Type::Sine, 6.0f, true, "Classic vibrato"},
            {"Tremolo", LFOWaveforms::Type::Triangle, 4.0f, false, "Amplitude modulation"},
            {"Wah", LFOWaveforms::Type::Triangle, 1.5f, true, "Filter sweep"},
            {"Pulse", LFOWaveforms::Type::Square, 8.0f, true, "Square wave modulation"},
            {"Sweep", LFOWaveforms::Type::Saw, 2.0f, true, "Sawtooth sweep"},
            {"Random", LFOWaveforms::Type::Random, 4.0f, true, "Smooth random"},
            {"S&H", LFOWaveforms::Type::SampleAndHold, 8.0f, true, "Sample & hold"},
            {"Fast", LFOWaveforms::Type::Sine, 24.0f, true, "Fast modulation"},
            {"Slow", LFOWaveforms::Type::Triangle, 0.5f, true, "Slow evolution"},
            {"Texture", LFOWaveforms::Type::Random, 0.2f, true, "Very slow texture"}
        };
    }

    static juce::StringArray getPresetNames() {
        auto presets = getPresets();
        juce::StringArray names;
        for (const auto& preset : presets) {
            names.add(preset.name);
        }
        return names;
    }
};

//==============================================================================
// LFO Factory
//==============================================================================

class LFOFactory {
public:
    static std::unique_ptr<AdvancedLFO> create() {
        return std::make_unique<AdvancedLFO>();
    }

    static std::unique_ptr<AdvancedLFO> createFromPreset(const juce::String& presetName) {
        auto lfo = std::make_unique<AdvancedLFO>();

        auto presets = LFOPresets::getPresets();
        for (const auto& preset : presets) {
            if (preset.name == presetName) {
                lfo->setWaveType(preset.waveform);
                lfo->setRate(preset.rate);
                lfo->setBipolar(preset.bipolar);
                break;
            }
        }

        return lfo;
    }
};

} // namespace zenith
