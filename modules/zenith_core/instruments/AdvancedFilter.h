/*
    Advanced Filter Implementation for Zenith Ultra Synth
    Features: Self-oscillation, key tracking, drive saturation, multi-mode, formant, comb
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>

namespace zenith {

//==============================================================================
// Filter Base with Drive Saturation
//==============================================================================

class FilterBase {
public:
    enum class Type {
        Lowpass,
        Highpass,
        Bandpass,
        Notch,
        Allpass,
        Peak,
        Bell,
        Lowshelf,
        Highshelf
    };

    FilterBase() {
        setSampleRate(44100.0f);
    }

    virtual ~FilterBase() = default;

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateCoefficients();
    }

    void setCutoff(float cutoffHz) {
        cutoff_ = juce::jlimit(20.0f, sampleRate_ * 0.49f, cutoffHz);
        updateCoefficients();
    }

    void setResonance(float resonance) {
        // Resonance from 0.0 to 1.0 (self-oscillation at ~0.95)
        resonance_ = juce::jlimit(0.0f, 1.0f, resonance);
        updateCoefficients();
    }

    void setDrive(float drive) {
        drive_ = juce::jmax(1.0f, drive);
    }

    void setType(Type type) {
        type_ = type;
        updateCoefficients();
    }

    void setKeyTrack(int semitones) {
        keyTrack_ = juce::jlimit(0, 36, semitones);  // 0-3 octaves
        updateKeyTracking();
    }

    void setKeyBase(float noteFreq) {
        keyBaseFreq_ = noteFreq;
        updateKeyTracking();
    }

    float processSample(float input) {
        // Apply drive with soft saturation
        float driven = input * drive_;
        driven = softSaturate(driven);

        // Process through filter
        float output = processFilter(driven);

        // Compensate gain
        output /= std::sqrt(drive_);

        return output;
    }

    void processBlock(float* output, const float* input, size_t numSamples) {
        for (size_t i = 0; i < numSamples; ++i) {
            output[i] = processSample(input[i]);
        }
    }

    void reset() {
        for (auto& s : states_) s = 0.0f;
    }

    bool isSelfOscillating() const {
        return resonance_ > 0.9f;
    }

protected:
    virtual float processFilter(float input) = 0;
    virtual void updateCoefficients() = 0;

    float softSaturate(float x) {
        // Soft clipping using tanh approximation
        if (x > 1.5f) return 0.98f;
        if (x < -1.5f) return -0.98f;

        // Polynomial approximation of tanh
        float x2 = x * x;
        float x3 = x2 * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    void updateKeyTracking() {
        if (keyTrack_ > 0 && keyBaseFreq_ > 0) {
            float ratio = std::pow(2.0f, static_cast<float>(keyTrack_) / 12.0f);
            keyTrackedCutoff_ = cutoff_ * ratio;
        } else {
            keyTrackedCutoff_ = cutoff_;
        }
    }

    float sampleRate_ = 44100.0f;
    float cutoff_ = 1000.0f;
    float keyTrackedCutoff_ = 1000.0f;
    float keyBaseFreq_ = 440.0f;
    float resonance_ = 0.0f;
    float drive_ = 1.0f;
    int keyTrack_ = 0;
    Type type_ = Type::Lowpass;

    std::array<float, 4> states_{};
};

//==============================================================================
// Moog Ladder Filter (4-pole, self-oscillating)
//==============================================================================

class MoogLadderFilter : public FilterBase {
public:
    MoogLadderFilter() : FilterBase() {
        resonance_ = 0.0f;  // Start stable
    }

protected:
    float processFilter(float input) override {
        // Moog ladder implementation using 4 cascaded 1-pole filters
        float feedback = resonance_ * (4.0f + resonance_ * 4.0f) * states_[3];

        // Drive the input
        float driven = input - feedback;

        // Four cascaded stages
        for (int i = 0; i < 4; ++i) {
            float stageOut = states_[i] + coef_ * (driven - states_[i]);
            stageOut = juce::jlimit(-1.0f, 1.0f, stageOut);  // Nonlinear stage
            states_[i] = stageOut;
            driven = stageOut;
        }

        return states_[3];
    }

    void updateCoefficients() override {
        FilterBase::updateKeyTracking();

        float wc = 2.0f * juce::MathConstants<float>::pi * keyTrackedCutoff_ / sampleRate_;
        float wc2 = wc * wc;

        // Approximation of tanh for cutoff
        coef_ = wc * (1.0f - wc2 / 12.0f);
        coef_ = juce::jlimit(0.0f, 1.0f, coef_);
    }

private:
    float coef_ = 0.5f;
};

//==============================================================================
// State Variable Filter (Multi-mode)
//==============================================================================

class StateVariableFilter : public FilterBase {
public:
    StateVariableFilter() : FilterBase() {
        resonance_ = 0.0f;
    }

protected:
    float processFilter(float input) override {
        // Bandpass output (intermediate)
        float bp = states_[0];
        float low = states_[1];
        float high = input - low - resonance_ * bp;
        float band = coef_ * high;
        float notch = high - low;

        // Update states
        states_[0] = band + bp;  // Bandpass state
        states_[1] = band + low;  // Lowpass state

        // Output based on type
        switch (type_) {
            case Type::Lowpass: return low;
            case Type::Highpass: return high;
            case Type::Bandpass: return bp;
            case Type::Notch: return notch;
            case Type::Allpass: return notch - bp;  // Approximation
            case Type::Peak: return low + bp;
            case Type::Bell: return low + bp * 0.5f;
            default: return low;
        }
    }

    void updateCoefficients() override {
        FilterBase::updateKeyTracking();

        float f = keyTrackedCutoff_ / sampleRate_;
        coef_ = 2.0f * std::sin(juce::MathConstants<float>::pi * f);

        // Resonance to Q conversion
        // Q = 1 / (2 * (1 - resonance)) for high resonance
        float q = 1.0f / (2.0f * (1.0f - resonance_ * 0.99f));
        resonance_ = juce::jlimit(0.0f, 0.99f, q / 10.0f);
    }

private:
    float coef_ = 0.1f;
};

//==============================================================================
// MS-20 Style Filter (Sallen-Key with resonance)
//==============================================================================

class MS20Filter : public FilterBase {
public:
    MS20Filter() : FilterBase() {}

protected:
    float processFilter(float input) override {
        // Sallen-Key lowpass topology with nonlinear resonance
        float lp = states_[0];
        float hp = input - lp - states_[1] / resonance_;

        states_[1] += coef_ * hp;
        states_[0] += coef_ * states_[1];

        // Nonlinear distortion for resonance character
        float output = states_[0];
        if (resonance_ > 0.5f) {
            output = std::tanh(output * 2.0f) * 0.5f;
        }

        return output;
    }

    void updateCoefficients() override {
        FilterBase::updateKeyTracking();

        float wc = 2.0f * juce::MathConstants<float>::pi * keyTrackedCutoff_ / sampleRate_;
        coef_ = wc * (1.0f - wc * 0.3f);  // Compensate for nonlinearity
        coef_ = juce::jlimit(0.0f, 1.0f, coef_);
    }

private:
    float coef_ = 0.5f;
};

//==============================================================================
// Formant Filter (Vowel synthesis)
//==============================================================================

class FormantFilter : public FilterBase {
public:
    enum class Vowel {
        A,      // ah
        E,      // eh
        I,      // ee
        O,      // oh
        U       // oo
    };

    FormantFilter() : FilterBase() {
        setVowel(Vowel::A);
    }

    void setVowel(Vowel vowel) {
        currentVowel_ = vowel;
        updateFormantFrequencies();
    }

    void setVowelMix(float mix) {
        // Morph between vowels
        vowelMix_ = juce::jlimit(0.0f, 1.0f, mix);
        updateFormantFrequencies();
    }

protected:
    float processFilter(float input) override {
        // Parallel resonators for formants
        float output = 0.0f;

        for (int i = 0; i < numFormants_; ++i) {
            output += processFormant(input, i);
        }

        // Blend with dry
        float dry = input * (1.0f - resonance_);
        return output * resonance_ + dry;
    }

    void updateCoefficients() override {
        // Formants have fixed relationships
        updateFormantFrequencies();
    }

private:
    static constexpr int numFormants_ = 3;

    struct Formant {
        float freq;
        float bw;
        float amp;
    };

    void updateFormantFrequencies() {
        // Formant frequencies for different vowels
        static const std::vector<Formant> vowelA = {{800, 80, 1.0f}, {1150, 90, 0.5f}, {2900, 120, 0.2f}};
        static const std::vector<Formant> vowelE = {{350, 60, 1.0f}, {2000, 100, 0.3f}, {2800, 130, 0.2f}};
        static const std::vector<Formant> vowelI = {{270, 60, 1.0f}, {2100, 100, 0.2f}, {3100, 120, 0.1f}};
        static const std::vector<Formant> vowelO = {{450, 70, 1.0f}, {800, 80, 0.6f}, {2800, 130, 0.1f}};
        static const std::vector<Formant> vowelU = {{300, 50, 1.0f}, {870, 90, 0.5f}, {2400, 120, 0.1f}};

        const std::vector<Formant>* vowels[] = {&vowelA, &vowelE, &vowelI, &vowelO, &vowelU};
        const auto& vowel = *vowels[static_cast<int>(currentVowel_)];

        for (int i = 0; i < numFormants_; ++i) {
            formants_[i] = vowel[i];
        }
    }

    float processFormant(float input, int index) {
        // Resonator (bandpass) using biquad
        float f0 = formants_[index].freq;
        float bw = formants_[index].bw;
        float amp = formants_[index].amp;

        float wc = 2.0f * juce::MathConstants<float>::pi * f0 / sampleRate_;
        float wc2 = wc * wc;

        float alpha = std::sin(wc) * std::sinh(std::log(2.0f) / 2.0f * bw / f0 * wc / std::sin(wc));
        float b0 = alpha;
        float b1 = 0.0f;
        float b2 = -alpha;
        float a1 = -2.0f * std::cos(wc);
        float a2 = 1.0f - alpha;

        // Direct form I
        size_t offset = index * 4;
        float output = b0 * input + b1 * states_[offset] + b2 * states_[offset + 1]
                     - a1 * states_[offset + 2] - a2 * states_[offset + 3];

        states_[offset + 1] = states_[offset];
        states_[offset] = input;
        states_[offset + 3] = states_[offset + 2];
        states_[offset + 2] = output;

        return output * amp;
    }

    Vowel currentVowel_ = Vowel::A;
    float vowelMix_ = 0.0f;
    std::array<Formant, numFormants_> formants_;
};

//==============================================================================
// Comb Filter (Karplus-Strong)
//==============================================================================

class CombFilter : public FilterBase {
public:
    CombFilter() : FilterBase() {
        buffer_.resize(maxDelay_ * 2, 0.0f);
    }

    void setPitchTracking(bool track) {
        pitchTrack_ = track;
        updateDelay();
    }

protected:
    float processFilter(float input) override {
        // Update delay based on cutoff (if pitch tracking)
        if (pitchTrack_) {
            updateDelay();
        }

        float delay = delaySamples_;
        int delayInt = static_cast<int>(delay);
        float delayFrac = delay - delayInt;

        // Read from delay line with interpolation
        int readPos = (writePos_ - delayInt + buffer_.size()) % buffer_.size();
        int readPosNext = (readPos - 1 + buffer_.size()) % buffer_.size();

        float delayed = buffer_[readPos] * (1.0f - delayFrac) + buffer_[readPosNext] * delayFrac;

        // Comb filter: y[n] = x[n] + g * x[n - delay]
        float feedback = resonance_ * 0.95f;
        float output = input + feedback * delayed;

        // Write to buffer
        buffer_[writePos_] = output;
        writePos_ = (writePos_ + 1) % buffer_.size();

        return output;
    }

    void updateCoefficients() override {
        FilterBase::updateKeyTracking();
        updateDelay();
    }

private:
    void updateDelay() {
        // Delay time based on cutoff frequency
        float freq = pitchTrack_ ? keyTrackedCutoff_ : cutoff_;

        if (freq > 0) {
            delaySamples_ = sampleRate_ / freq;
            delaySamples_ = juce::jlimit(4.0f, static_cast<float>(maxDelay_), delaySamples_);
        }
    }

    static constexpr int maxDelay_ = 4096;
    std::vector<float> buffer_;
    int writePos_ = 0;
    float delaySamples_ = 100.0f;
    bool pitchTrack_ = true;
};

//==============================================================================
// Multi-Mode Filter (Parallel/Serial combinations)
//==============================================================================

class MultiModeFilter : public FilterBase {
public:
    enum class Config {
        Serial,      // Output of filter 1 feeds into filter 2
        Parallel,    // Both filters process input, outputs summed
        Morph,       // Morph between filter types
        Stereo       // Left channel through filter 1, right through filter 2
    };

    MultiModeFilter() {
        // Create default filters
        filter1_ = std::make_unique<StateVariableFilter>();
        filter2_ = std::make_unique<MoogLadderFilter>();
    }

    void setConfig(Config config) {
        config_ = config;
    }

    void setFilter1Type(Type type) {
        if (filter1_) filter1_->setType(type);
    }

    void setFilter2Type(Type type) {
        if (filter2_) filter2_->setType(type);
    }

    void setMorphAmount(float morph) {
        morphAmount_ = juce::jlimit(0.0f, 1.0f, morph);
    }

protected:
    float processFilter(float input) override {
        if (!filter1_ || !filter2_) return input;

        float output1 = filter1_->processSample(input);
        float output2 = filter2_->processSample(input);

        switch (config_) {
            case Config::Serial:
                // Filter 2 processes filter 1's output
                return filter2_->processSample(output1);

            case Config::Parallel:
                // Sum both outputs
                return (output1 + output2) * 0.5f;

            case Config::Morph:
                // Crossfade between filters
                return output1 * (1.0f - morphAmount_) + output2 * morphAmount_;

            case Config::Stereo:
                // Returns left channel (right handled separately)
                return output1;

            default:
                return output1;
        }
    }

    void updateCoefficients() override {
        FilterBase::updateKeyTracking();

        if (filter1_) {
            filter1_->setSampleRate(sampleRate_);
            filter1_->setCutoff(keyTrackedCutoff_);
            filter1_->setResonance(resonance_);
        }

        if (filter2_) {
            filter2_->setSampleRate(sampleRate_);
            filter2_->setCutoff(keyTrackedCutoff_);
            filter2_->setResonance(resonance_);
        }
    }

private:
    std::unique_ptr<FilterBase> filter1_;
    std::unique_ptr<FilterBase> filter2_;
    Config config_ = Config::Serial;
    float morphAmount_ = 0.0f;
};

//==============================================================================
// Filter Factory
//==============================================================================

class FilterFactory {
public:
    enum class Model {
        SVF,          // State Variable Filter
        Moog,         // Moog Ladder
        MS20,         // Korg MS-20
        SEM,          // Oberheim SEM (similar to SVF)
        TB303,        // Roland TB-303
        Formant,      // Vowel formant filter
        Comb,         // Comb filter / Karplus-Strong
        Multi         // Multi-mode filter
    };

    static std::unique_ptr<FilterBase> create(Model model) {
        switch (model) {
            case Model::SVF:
            case Model::SEM:
                return std::make_unique<StateVariableFilter>();
            case Model::Moog:
                return std::make_unique<MoogLadderFilter>();
            case Model::MS20:
                return std::make_unique<MS20Filter>();
            case Model::TB303:
                // MS-20 with specific settings for 303 character
                {
                    auto filter = std::make_unique<MS20Filter>();
                    filter->setResonance(0.8f);  // High resonance for 303
                    return filter;
                }
            case Model::Formant:
                return std::make_unique<FormantFilter>();
            case Model::Comb:
                return std::make_unique<CombFilter>();
            case Model::Multi:
                return std::make_unique<MultiModeFilter>();
            default:
                return std::make_unique<StateVariableFilter>();
        }
    }

    static juce::String getModelName(Model model) {
        switch (model) {
            case Model::SVF: return "State Variable";
            case Model::Moog: return "Moog Ladder";
            case Model::MS20: return "MS-20";
            case Model::SEM: return "Oberheim SEM";
            case Model::TB303: return "TB-303";
            case Model::Formant: return "Formant";
            case Model::Comb: return "Comb";
            case Model::Multi: return "Multi-Mode";
            default: return "Unknown";
        }
    }

    static juce::StringArray getModelNames() {
        return {
            "State Variable",
            "Moog Ladder",
            "MS-20",
            "Oberheim SEM",
            "TB-303",
            "Formant",
            "Comb",
            "Multi-Mode"
        };
    }
};

} // namespace zenith
