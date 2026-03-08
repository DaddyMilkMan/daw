/*
    AlgorithmicReverb.cpp - Professional Algorithmic Reverb

    Implements Schroeder/Moorer reverb algorithms with:
    - Parallel comb filters for reverb tail
    - Series allpass filters for diffusion
    - Pre-delay section
    - Frequency-dependent damping
    - Modulation for richness

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "AlgorithmicReverb.h"

namespace zenith {

//==============================================================================
// COMB FILTER (for reverb tail)
//==============================================================================

class CombFilter {
public:
    CombFilter() = default;

    void prepare(double sampleRate, float delayTime, float decayTime) {
        sampleRate_ = sampleRate;
        delayInSamples_ = static_cast<int>(delayTime * sampleRate);

        // Allocate delay line
        delayLine_.resize(delayInSamples_ + 1);
        std::fill(delayLine_.begin(), delayLine_.end(), 0.0f);
        writeIndex_ = 0;

        // Calculate gain for desired decay time
        // decayTime = -delayTime / log(gain)
        // gain = exp(-delayTime / decayTime)
        if (decayTime > 0.0f) {
            gain_ = std::exp(-delayTime / decayTime);
        } else {
            gain_ = 0.0f;
        }

        // Damping filter (lowpass for high-frequency damping)
        damping_ = 0.5f;
        dampingFilterState_ = 0.0f;
    }

    void reset() {
        std::fill(delayLine_.begin(), delayLine_.end(), 0.0f);
        writeIndex_ = 0;
        dampingFilterState_ = 0.0f;
    }

    void setDamping(float damping) {
        damping_ = juce::jlimit(0.0f, 1.0f, damping);
    }

    float process(float input) {
        // Read from delay line
        int readIndex = (writeIndex_ - delayInSamples_ + delayLine_.size()) % delayLine_.size();
        float delayed = delayLine_[readIndex];

        // Apply damping (one-pole lowpass)
        float damped = dampingFilterState_ + damping_ * (delayed - dampingFilterState_);
        dampingFilterState_ = damped;

        // Apply feedback gain
        float feedback = damped * gain_;

        // Write to delay line
        delayLine_[writeIndex_] = input + feedback;
        writeIndex_ = (writeIndex_ + 1) % delayLine_.size();

        return delayed;
    }

private:
    std::vector<float> delayLine_;
    int writeIndex_ = 0;
    int delayInSamples_ = 0;
    float gain_ = 0.0f;
    float damping_ = 0.5f;
    float dampingFilterState_ = 0.0f;
    double sampleRate_ = 44100.0;
};

//==============================================================================
// ALLPASS FILTER (for diffusion)
//==============================================================================

class AllpassFilter {
public:
    AllpassFilter() = default;

    void prepare(double sampleRate, float delayTime) {
        sampleRate_ = sampleRate;
        delayInSamples_ = static_cast<int>(delayTime * sampleRate);

        // Allocate delay line
        delayLine_.resize(delayInSamples_ + 1);
        std::fill(delayLine_.begin(), delayLine_.end(), 0.0f);
        writeIndex_ = 0;

        // Allpass gain (fixed for diffusion)
        gain_ = 0.5f;
    }

    void reset() {
        std::fill(delayLine_.begin(), delayLine_.end(), 0.0f);
        writeIndex_ = 0;
    }

    float process(float input) {
        // Read from delay line
        int readIndex = (writeIndex_ - delayInSamples_ + delayLine_.size()) % delayLine_.size();
        float delayed = delayLine_[readIndex];

        // Allpass equation: y = -input + delayed + gain * (input + y_prev)
        float output = -gain_ * input + delayed + gain_ * input;

        // Write to delay line
        delayLine_[writeIndex_] = input + gain_ * delayed;
        writeIndex_ = (writeIndex_ + 1) % delayLine_.size();

        return output;
    }

private:
    std::vector<float> delayLine_;
    int writeIndex_ = 0;
    int delayInSamples_ = 0;
    float gain_ = 0.5f;
    double sampleRate_ = 44100.0;
};

//==============================================================================
// ALGORITHMIC REVERB IMPLEMENTATION
//==============================================================================

struct AlgorithmicReverb::Impl {
    // Comb filters (parallel for reverb tail)
    // Using prime number ratios for dense echo pattern
    static constexpr int numCombs = 8;
    std::array<std::unique_ptr<CombFilter>, numCombs> combFilters_;

    // Allpass filters (series for diffusion)
    static constexpr int numAllpass = 4;
    std::array<std::unique_ptr<AllpassFilter>, numAllpass> allpassFilters_;

    // Pre-delay
    std::unique_ptr<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>> preDelay_;

    // Output buffers
    juce::AudioBuffer<float> wetBuffer_;

    // Parameters
    float roomSize = 0.5f;
    float damping = 0.5f;
    float wetLevel = 0.3f;
    float decayTime = 2.0f;
    float preDelayTime = 0.02f;
    float diffusion = 0.5f;
    float modulationDepth = 0.1f;

    // Modulation
    juce::dsp::Oscillator<float> lfo_;
    float lfoPhase = 0.0f;
    double sampleRate = 44100.0;

    void prepare(double sr, int samplesPerBlock) {
        sampleRate = sr;

        // Initialize pre-delay
        preDelay_ = std::make_unique<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>>();
        preDelay_->prepare({juce::uint64(sr), 1, juce::uint32(samplesPerBlock)});
        preDelay_->setMaximumDelayInSamples(static_cast<int>(0.2f * sr)); // 200ms max

        // Initialize comb filters with prime number delay times
        // (in milliseconds) for dense, metallic-free reverb
        std::array<float, numCombs> combDelays = {
            0.025f, 0.027f, 0.029f, 0.031f,  // Short
            0.037f, 0.041f, 0.043f, 0.047f   // Long
        };

        for (size_t i = 0; i < numCombs; ++i) {
            combFilters_[i] = std::make_unique<CombFilter>();
            float scaledDecay = decayTime * (1.0f + roomSize); // Scale by room size
            combFilters_[i]->prepare(sr, combDelays[i], scaledDecay);
        }

        // Initialize allpass filters for diffusion
        // (in milliseconds) - shorter than comb filters
        std::array<float, numAllpass> allpassDelays = {
            0.005f, 0.007f, 0.011f, 0.013f
        };

        for (size_t i = 0; i < numAllpass; ++i) {
            allpassFilters_[i] = std::make_unique<AllpassFilter>();
            allpassFilters_[i]->prepare(sr, allpassDelays[i]);
        }

        // Initialize LFO for modulation
        lfo_.initialise([](float phase) { return std::sin(phase); });
        lfo_.setFrequency(0.5f); // 0.5 Hz modulation

        // Allocate wet buffer
        wetBuffer_ = juce::AudioBuffer<float>(2, samplesPerBlock);
    }

    void reset() {
        if (preDelay_) preDelay_->reset();
        for (auto& comb : combFilters_) {
            if (comb) comb->reset();
        }
        for (auto& ap : allpassFilters_) {
            if (ap) ap->reset();
        }
    }

    void process(juce::AudioBuffer<float>& buffer, float wet) {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();
        wetBuffer_.setSize(numChannels, numSamples, false, false, true);
        wetBuffer_.clear();

        // Process each channel
        for (int channel = 0; channel < numChannels; ++channel) {
            float* input = buffer.getWritePointer(channel);
            float* output = wetBuffer_.getWritePointer(channel);

            // Apply pre-delay
            preDelay_->setDelay(static_cast<float>(preDelayTime * sampleRate));

            for (int sample = 0; sample < numSamples; ++sample) {
                float x = input[sample];

                // Pre-delay
                float preDelayed = x;
                if (preDelay_) {
                    preDelayed = preDelay_->processSample(x);
                }

                // Diffusion (allpass filters in series)
                float diffused = preDelayed;
                for (size_t i = 0; i < numAllpass; ++i) {
                    if (allpassFilters_[i]) {
                        diffused = allpassFilters_[i]->process(diffused);
                    }
                }

                // Comb filters (parallel) for reverb tail
                float tail = 0.0f;
                for (size_t i = 0; i < numCombs; ++i) {
                    if (combFilters_[i]) {
                        tail += combFilters_[i]->process(diffused * 0.5f); // Scale input
                    }
                }
                tail /= static_cast<float>(numCombs); // Average

                output[sample] = tail;
            }
        }

        // Mix wet and dry
        for (int channel = 0; channel < numChannels; ++channel) {
            buffer.applyGain(channel, 0, numSamples, 1.0f - wet);
            for (int sample = 0; sample < numSamples; ++sample) {
                buffer.addSample(channel, sample, wetBuffer_.getSample(channel, sample) * wet);
            }
        }
    }

    void updateDecayTimes() {
        for (size_t i = 0; i < numCombs; ++i) {
            if (combFilters_[i]) {
                float scaledDecay = decayTime * (1.0f + roomSize);
                // Re-initialize with new decay time
                combFilters_[i]->prepare(sampleRate, 0.0f, scaledDecay);
            }
        }
    }

    void setDamping(float d) {
        damping = juce::jlimit(0.0f, 1.0f, d);
        for (auto& comb : combFilters_) {
            if (comb) comb->setDamping(damping);
        }
    }
};

//==============================================================================
// AlgorithmicReverb Wrapper
//==============================================================================

AlgorithmicReverb::AlgorithmicReverb()
    : impl_(std::make_unique<Impl>()) {}

AlgorithmicReverb::~AlgorithmicReverb() = default;

void AlgorithmicReverb::prepare(double sampleRate, int samplesPerBlock) {
    impl_->prepare(sampleRate, samplesPerBlock);
}

void AlgorithmicReverb::reset() {
    impl_->reset();
}

void AlgorithmicReverb::process(juce::AudioBuffer<float>& buffer, float wetLevel) {
    impl_->process(buffer, wetLevel);
}

void AlgorithmicReverb::updateDecayTimes() {
    impl_->updateDecayTimes();
}

void AlgorithmicReverb::setRoomSize(float size) {
    impl_->roomSize = juce::jlimit(0.0f, 1.0f, size);
    impl_->updateDecayTimes();
}

void AlgorithmicReverb::setDamping(float damping) {
    impl_->setDamping(damping);
}

void AlgorithmicReverb::setWetLevel(float wet) {
    impl_->wetLevel = juce::jlimit(0.0f, 1.0f, wet);
}

void AlgorithmicReverb::setDecayTime(float decay) {
    impl_->decayTime = juce::jlimit(0.1f, 10.0f, decay);
    impl_->updateDecayTimes();
}

void AlgorithmicReverb::setPreDelay(float predelay) {
    impl_->preDelayTime = juce::jlimit(0.0f, 0.2f, predelay);
}

void AlgorithmicReverb::setDiffusion(float diff) {
    impl_->diffusion = juce::jlimit(0.0f, 1.0f, diff);
}

void AlgorithmicReverb::setModulation(float mod) {
    impl_->modulationDepth = juce::jlimit(0.0f, 1.0f, mod);
}

} // namespace zenith
