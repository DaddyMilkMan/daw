/*
    Advanced Oscillator Implementation for Zenith Ultra Synth
    Features: Anti-aliasing, hard sync, phase offset, drift, FM/PM, wavetable
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
// Oscillator Temperature Drift Model
//==============================================================================

class OscillatorDrift {
public:
    OscillatorDrift() {
        seedTemperature();
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        driftRate_ = 0.1f / sampleRate;  // Drift update rate
    }

    void setDriftAmount(float amount) {
        driftAmount_ = juce::jlimit(0.0f, 1.0f, amount);
    }

    float getCurrentDetune() const {
        return currentDetune_;
    }

    void setTemperature(float temperature) {
        // Temperature in Celsius (typical studio: 20-25°C)
        temperature_ = juce::jlimit(0.0f, 50.0f, temperature);
    }

    void update() {
        // Simulate thermal drift based on temperature
        float thermalNoise = random_.nextFloat() * 2.0f - 1.0f;

        // Higher temperature = more drift
        float tempFactor = temperature_ / 25.0f;
        float driftChange = thermalNoise * driftAmount_ * tempFactor * driftRate_;

        currentDetune_ += driftChange;

        // Clamp to reasonable range
        currentDetune_ = juce::jlimit(-10.0f, 10.0f, currentDetune_);

        // Slow return to center (thermal equilibrium)
        currentDetune_ *= 0.9999f;
    }

    void reset() {
        currentDetune_ = 0.0f;
        seedTemperature();
    }

private:
    void seedTemperature() {
        // Simulate initial temperature variation
        float baseTemp = 22.0f;
        float variation = random_.nextFloat() * 6.0f - 3.0f;  // ±3°C
        temperature_ = baseTemp + variation;
    }

    float sampleRate_ = 44100.0f;
    float driftAmount_ = 0.5f;  // 0-1
    float driftRate_ = 0.00001f;
    float temperature_ = 22.0f;
    float currentDetune_ = 0.0f;
    juce::Random random_;
};

//==============================================================================
// Anti-Aliasing Oscillator Base
//==============================================================================

class AntiAliasingOscillator {
public:
    enum class Waveform {
        Sine,
        Saw,
        Square,
        Triangle,
        Supersaw,
        Pulse
    };

    AntiAliasingOscillator() {
        setSampleRate(44100.0f);
    }

    virtual ~AntiAliasingOscillator() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        oversampleFactor_ = calculateOversampleFactor();
        setupAntiAliasingFilter();
    }

    void setFrequency(float frequency) {
        // Clamp to Nyquist
        float maxFreq = sampleRate_ * 0.49f / oversampleFactor_;
        frequency_ = juce::jlimit(1.0f, maxFreq, frequency);
        updatePhaseIncrement();
    }

    void setPhaseOffset(float offsetDegrees) {
        phaseOffset_ = offsetDegrees / 360.0f;  // Normalize to 0-1
    }

    void setPulseWidth(float width) {
        pulseWidth_ = juce::jlimit(0.01f, 0.99f, width);
    }

    void setWaveform(Waveform waveform) {
        waveform_ = waveform;
    }

    float processSample() {
        // Update phase
        phase_ += phaseIncrement_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        // Add phase offset (doesn't accumulate)
        float modPhase = phase_ + phaseOffset_;
        if (modPhase >= 1.0f) modPhase -= 1.0f;

        // Generate waveform
        float output = generateWaveform(modPhase);

        // Apply anti-aliasing filter
        output = aaFilter_.processSample(output);

        return output;
    }

    void processBlock(float* output, size_t numSamples) {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = processSample();
        }
    }

    void reset() {
        phase_ = 0.0f;
        aaFilter_.reset();
    }

protected:
    virtual float generateWaveform(float phase) = 0;

    void updatePhaseIncrement() {
        phaseIncrement_ = frequency_ / sampleRate_;
    }

    int calculateOversampleFactor() {
        // Higher frequencies need more oversampling
        if (frequency_ > sampleRate_ * 0.25f) return 4;
        if (frequency_ > sampleRate_ * 0.125f) return 2;
        return 1;
    }

    void setupAntiAliasingFilter() {
        aaFilter_.setSampleRate(sampleRate_ * oversampleFactor_);
        aaFilter_.setCutoff(sampleRate_ * 0.45f);
    }

    float sampleRate_ = 44100.0f;
    float frequency_ = 440.0f;
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    float phaseOffset_ = 0.0f;
    float pulseWidth_ = 0.5f;
    int oversampleFactor_ = 1;
    Waveform waveform_ = Waveform::Sine;

    // One-pole lowpass for anti-aliasing
    struct OnePoleFilter {
        float setSampleRate(float sr) { sampleRate_ = sr; return 0; }
        void setCutoff(float cutoff) {
            float wc = 2.0f * juce::MathConstants<float>::pi * cutoff / sampleRate_;
            coef_ = wc / (1.0f + wc);
        }
        float processSample(float input) {
            float output = z1_ + coef_ * (input - z1_);
            z1_ = output;
            return output;
        }
        void reset() { z1_ = 0.0f; }
        float sampleRate_ = 44100.0f;
        float coef_ = 0.9f;
        float z1_ = 0.0f;
    } aaFilter_;
};

//==============================================================================
// PolyBLEP Saw Oscillator (Bandlimited)
//==============================================================================

class PolyBLEPSaw : public AntiAliasingOscillator {
protected:
    float generateWaveform(float phase) override {
        // Naive saw
        float saw = 2.0f * phase - 1.0f;

        // PolyBLEP correction at discontinuity
        if (phase < phaseIncrement_) {
            float t = phase / phaseIncrement_;
            float blep = -t * t + 2.0f * t - 1.0f;
            saw += blep;
        }

        return saw;
    }
};

//==============================================================================
// PolyBLEP Square Oscillator (Bandlimited)
//==============================================================================

class PolyBLEPSquare : public AntiAliasingOscillator {
protected:
    float generateWaveform(float phase) override {
        // Determine if we're at rising edge
        float risingEdge = (phase < phaseIncrement_) ? 1.0f : 0.0f;
        float fallingEdge = (phase < pulseWidth_ && phase + phaseIncrement_ >= pulseWidth_) ? 1.0f : 0.0f;

        // Naive square
        float square = (phase < pulseWidth_) ? 1.0f : -1.0f;

        // PolyBLEP correction
        if (risingEdge > 0.5f) {
            float t = phase / phaseIncrement_;
            float blep = t * t - 2.0f * t + 1.0f;
            square += 2.0f * blep;
        }

        if (fallingEdge > 0.5f) {
            float t = (phase - pulseWidth_) / phaseIncrement_;
            float blep = t * t - 2.0f * t + 1.0f;
            square -= 2.0f * blep;
        }

        return square;
    }
};

//==============================================================================
// Hard Sync Oscillator
//==============================================================================

class HardSyncOscillator : public AntiAliasingOscillator {
public:
    void setSyncRatio(float ratio) {
        syncRatio_ = juce::jlimit(1.0f, 16.0f, ratio);
    }

    void setMasterPhase(float masterPhase) {
        masterPhase_ = masterPhase;
        lastMasterPhase_ = lastMasterPhase_;
    }

protected:
    float generateWaveform(float phase) override {
        // Check for sync reset
        bool didReset = false;
        if (masterPhase_ < lastMasterPhase_) {
            // Master wrapped - reset slave
            slavePhase_ = 0.0f;
            didReset = true;
        }

        lastMasterPhase_ = masterPhase_;

        // Advance slave phase
        slavePhase_ += phaseIncrement_ * syncRatio_;
        if (slavePhase_ >= 1.0f) slavePhase_ -= 1.0f;

        // Generate waveform from slave phase
        float output = 2.0f * slavePhase_ - 1.0f;  // Saw

        // Smooth the reset to avoid clicks
        if (didReset) {
            output *= 0.0f;  // Fade out
        }

        return output;
    }

private:
    float syncRatio_ = 2.0f;
    float masterPhase_ = 0.0f;
    float lastMasterPhase_ = 0.0f;
    float slavePhase_ = 0.0f;
};

//==============================================================================
// FM Oscillator with Feedback and Saturation
//==============================================================================

class FMOscillator : public AntiAliasingOscillator {
public:
    void setFMAmount(float amount) {
        fmAmount_ = juce::jlimit(0.0f, 1000.0f, amount);
    }

    void setFMDepth(float depth) {
        fmDepth_ = juce::jlimit(0.0f, 1.0f, depth);
    }

    void setFeedback(float feedback) {
        feedback_ = juce::jlimit(0.0f, 0.95f, feedback);
    }

    void setModulator(const FMOscillator* modulator) {
        modulator_ = modulator;
    }

    float processSample() {
        // Get modulation from modulator
        float modulation = 0.0f;
        if (modulator_) {
            modulation = modulator_->getLastOutput() * fmAmount_;
        }

        // Apply feedback
        float feedbackSample = lastOutput_ * feedback_;

        // Update phase with modulation and feedback
        float totalMod = (modulation + feedbackSample) * fmDepth_;
        phase_ += phaseIncrement_ * (1.0f + totalMod);

        if (phase_ >= 1.0f) phase_ -= 1.0f;
        if (phase_ < 0.0f) phase_ += 1.0f;

        // Generate sine (most common for FM)
        float angle = phase_ * juce::MathConstants<float>::twoPi;
        float output = std::sin(angle);

        // Soft saturation to prevent aliasing at high modulation
        output = softSaturate(output);

        lastOutput_ = output;
        return output;
    }

    float getLastOutput() const { return lastOutput_; }

protected:
    float generateWaveform(float phase) override {
        float angle = phase * juce::MathConstants<float>::twoPi;
        return std::sin(angle);
    }

private:
    float softSaturate(float x) {
        // Soft clipping polynomial
        if (x > 1.0f) return 1.0f - std::exp(-x + 1.0f);
        if (x < -1.0f) return -1.0f + std::exp(x + 1.0f);
        return x;
    }

    float fmAmount_ = 0.0f;
    float fmDepth_ = 0.0f;
    float feedback_ = 0.0f;
    float lastOutput_ = 0.0f;
    const FMOscillator* modulator_ = nullptr;
};

//==============================================================================
// PM Oscillator (Phase Modulation)
//==============================================================================

class PMOscillator : public AntiAliasingOscillator {
public:
    void setPMAmount(float amount) {
        pmAmount_ = juce::jlimit(0.0f, juce::MathConstants<float>::pi, amount);
    }

    void setModulator(const PMOscillator* modulator) {
        modulator_ = modulator;
    }

    float processSample() {
        // Get modulation
        float modulation = 0.0f;
        if (modulator_) {
            modulation = modulator_->getLastOutput() * pmAmount_;
        }

        // Apply phase modulation
        float modPhase = phase_ + modulation;

        // Wrap phase
        while (modPhase >= 1.0f) modPhase -= 1.0f;
        while (modPhase < 0.0f) modPhase += 1.0f;

        // Generate waveform
        float output = generateWaveform(modPhase);

        // Update phase
        phase_ += phaseIncrement_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        lastOutput_ = output;
        return output;
    }

    float getLastOutput() const { return lastOutput_; }

protected:
    float generateWaveform(float phase) override {
        float angle = phase * juce::MathConstants<float>::twoPi;
        return std::sin(angle);
    }

private:
    float pmAmount_ = 0.0f;
    float lastOutput_ = 0.0f;
    const PMOscillator* modulator_ = nullptr;
};

//==============================================================================
// Wavetable Oscillator with Morphing
//==============================================================================

class WavetableOscillator : public AntiAliasingOscillator {
public:
    static constexpr int tableSize = 2048;
    static constexpr int numTables = 64;

    struct Wavetable {
        std::vector<std::vector<float>> tables;  // [tableIndex][sample]
        juce::String name;
    };

    WavetableOscillator() {
        // Allocate memory for tables
        currentWavetable_.tables.resize(numTables);
        for (auto& table : currentWavetable_.tables) {
            table.resize(tableSize);
        }
    }

    void setMorphPosition(float position) {
        morphPosition_ = juce::jlimit(0.0f, 1.0f, position);
    }

    void setInterpolationQuality(int quality) {
        // 0 = none, 1 = linear, 2 = cubic, 3 = spectral
        interpQuality_ = juce::jlimit(0, 3, quality);
    }

    void loadWavetable(const Wavetable& wavetable) {
        currentWavetable_ = wavetable;
    }

    // Generate basic wavetables
    void generateBasicWavetables() {
        for (int table = 0; table < numTables; ++table) {
            float t = static_cast<float>(table) / (numTables - 1);

            for (int i = 0; i < tableSize; ++i) {
                float phase = static_cast<float>(i) / tableSize;

                // Morph from sine to saw
                float sine = std::sin(phase * juce::MathConstants<float>::twoPi);
                float saw = 2.0f * phase - 1.0f;

                currentWavetable_.tables[table][i] = sine * (1.0f - t) + saw * t;
            }
        }
    }

protected:
    float generateWaveform(float phase) override {
        // Determine which tables to use
        float tableFloat = morphPosition_ * (numTables - 1);
        int table1 = static_cast<int>(tableFloat);
        int table2 = juce::jmin(numTables - 1, table1 + 1);
        float mix = tableFloat - table1;

        // Get sample from each table
        float sample1 = getTableSample(table1, phase);
        float sample2 = getTableSample(table2, phase);

        // Interpolate between tables
        return sample1 * (1.0f - mix) + sample2 * mix;
    }

private:
    float getTableSample(int tableIndex, float phase) {
        // Convert phase to sample index
        float indexFloat = phase * tableSize;
        int index = static_cast<int>(indexFloat);
        float frac = indexFloat - index;

        const auto& table = currentWavetable_.tables[tableIndex];

        switch (interpQuality_) {
            case 0:  // None
                return table[index % tableSize];

            case 1:  // Linear
                {
                    int next = (index + 1) % tableSize;
                    return table[index] * (1.0f - frac) + table[next] * frac;
                }

            case 2:  // Cubic
                {
                    int i0 = (index - 1 + tableSize) % tableSize;
                    int i1 = index;
                    int i2 = (index + 1) % tableSize;
                    int i3 = (index + 2) % tableSize;

                    float frac2 = frac * frac;
                    float frac3 = frac2 * frac;

                    float c0 = table[i1];
                    float c1 = (table[i2] - table[i0]) * 0.5f;
                    float c2 = table[i0] - 2.5f * table[i1] + 2.0f * table[i2] - 0.5f * table[i3];
                    float c3 = 1.5f * (table[i1] - table[i2]) + 0.5f * (table[i3] - table[i0]);

                    return c0 + c1 * frac + c2 * frac2 + c3 * frac3;
                }

            default:
                return table[index % tableSize];
        }
    }

    float morphPosition_ = 0.0f;
    int interpQuality_ = 2;
    Wavetable currentWavetable_;
};

//==============================================================================
// Stereo Oscillator with Pan Spread
//==============================================================================

class StereoOscillator {
public:
    StereoOscillator() {
        leftOsc_ = std::make_unique<PolyBLEPSaw>();
        rightOsc_ = std::make_unique<PolyBLEPSaw>();
    }

    void setSampleRate(float sampleRate) {
        leftOsc_->setSampleRate(sampleRate);
        rightOsc_->setSampleRate(sampleRate);
    }

    void setFrequency(float frequency) {
        leftOsc_->setFrequency(frequency);
        rightOsc_->setFrequency(frequency);
    }

    void setPanSpread(float spread) {
        // Spread in detune cents
        panSpread_ = juce::jlimit(0.0f, 50.0f, spread);  // ±50 cents

        // Apply detune to create stereo width
        float leftRatio = std::pow(2.0f, -panSpread_ / 1200.0f);
        float rightRatio = std::pow(2.0f, panSpread_ / 1200.0f);

        leftOsc_->setFrequency(leftOsc_->getFrequency() * leftRatio);
        rightOsc_->setFrequency(rightOsc_->getFrequency() * rightRatio);
    }

    void setPhaseOffset(float offsetDegrees) {
        leftOsc_->setPhaseOffset(-offsetDegrees / 2.0f);
        rightOsc_->setPhaseOffset(offsetDegrees / 2.0f);
    }

    void processSample(float& left, float& right) {
        left = leftOsc_->processSample();
        right = rightOsc_->processSample();
    }

    void processBlock(float* left, float* right, size_t numSamples) {
        leftOsc_->processBlock(left, numSamples);
        rightOsc_->processBlock(right, numSamples);
    }

    void reset() {
        leftOsc_->reset();
        rightOsc_->reset();
    }

private:
    std::unique_ptr<AntiAliasingOscillator> leftOsc_;
    std::unique_ptr<AntiAliasingOscillator> rightOsc_;
    float panSpread_ = 0.0f;
};

//==============================================================================
// Advanced Oscillator Voice (combines all features)
//==============================================================================

class AdvancedOscillatorVoice {
public:
    AdvancedOscillatorVoice() {
        setSampleRate(44100.0f);
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        osc_.setSampleRate(sampleRate);
        drift_.setSampleRate(sampleRate);
    }

    void setFrequency(float frequency) {
        baseFrequency_ = frequency;

        // Apply drift
        float driftCents = drift_.getCurrentDetune();
        float detuneRatio = std::pow(2.0f, driftCents / 1200.0f);

        osc_.setFrequency(frequency * detuneRatio);
    }

    void setWaveform(AntiAliasingOscillator::Waveform waveform) {
        osc_.setWaveform(waveform);
    }

    void setDetune(float cents) {
        float ratio = std::pow(2.0f, cents / 1200.0f);
        osc_.setFrequency(baseFrequency_ * ratio);
    }

    void setPhaseOffset(float degrees) {
        osc_.setPhaseOffset(degrees);
    }

    void setPulseWidth(float width) {
        osc_.setPulseWidth(width);
    }

    void setDriftAmount(float amount) {
        drift_.setDriftAmount(amount);
    }

    void setTemperature(float temperature) {
        drift_.setTemperature(temperature);
    }

    void setPanSpread(float spread) {
        panSpread_ = juce::jlimit(-1.0f, 1.0f, spread);
    }

    float processSample() {
        drift_.update();
        float sample = osc_.processSample();

        // Apply pan spread
        if (panSpread_ != 0.0f) {
            // Simple pan based on phase
            float pan = std::sin(osc_.getPhase() * juce::MathConstants<float>::twoPi) * panSpread_;
            sample *= std::sqrt(1.0f - pan * pan);  // Constant power
        }

        return sample;
    }

    void reset() {
        osc_.reset();
        drift_.reset();
    }

private:
    PolyBLEPSaw osc_;
    OscillatorDrift drift_;
    float sampleRate_ = 44100.0f;
    float baseFrequency_ = 440.0f;
    float panSpread_ = 0.0f;
};

//==============================================================================
// Oscillator Factory
//==============================================================================

class OscillatorFactory {
public:
    enum class Type {
        Basic,           // Simple bandlimited
        HardSync,        // With hard sync
        FM,              // FM synthesis
        PM,              // Phase modulation
        Wavetable,       // Wavetable with morphing
        Stereo           // Stereo with spread
    };

    static std::unique_ptr<AntiAliasingOscillator> create(Type type) {
        switch (type) {
            case Type::Basic:
                return std::make_unique<PolyBLEPSaw>();
            case Type::HardSync:
                return std::make_unique<HardSyncOscillator>();
            case Type::FM:
                return std::make_unique<FMOscillator>();
            case Type::PM:
                return std::make_unique<PMOscillator>();
            case Type::Wavetable:
                return std::make_unique<WavetableOscillator>();
            default:
                return std::make_unique<PolyBLEPSaw>();
        }
    }

    static std::unique_ptr<StereoOscillator> createStereo() {
        return std::make_unique<StereoOscillator>();
    }
};

} // namespace zenith
