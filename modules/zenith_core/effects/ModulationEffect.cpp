/*
    ModulationEffect.cpp - Professional Modulation Effects

    Implementation of chorus, phaser, and flanger with unique algorithms.

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
*/

#include "ModulationEffect.h"

namespace zenith {

//==============================================================================
// ModulationEffect Implementation
//==============================================================================

ModulationEffect::ModulationEffect() {
    // Initialize chorus delay lines
    for (int i = 0; i < maxVoices; ++i) {
        chorusDelays_[i] = std::make_unique<juce::dsp::DelayLine<float,
            juce::dsp::DelayLineInterpolationTypes::Thiran>>(12000);
    }

    // Initialize phaser allpass filters
    for (int i = 0; i < numPhaserStages; ++i) {
        phaserStages_[i] = std::make_unique<juce::dsp::FirstOrderTPTFilter<float>>(
            juce::dsp::FirstOrderTPTFilter<float>::Type::allpass);
    }

    // Initialize LFOs
    lfo_.initialise([](float x) { return std::sin(x); });
    lfo_.setFrequency(0.5f);

    lfoSlow_.initialise([](float x) { return std::sin(x); });
    lfoSlow_.setFrequency(0.1f);
}

ModulationEffect::~ModulationEffect() = default;

void ModulationEffect::prepare(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate;

    // Prepare chorus delays
    for (int i = 0; i < maxVoices; ++i) {
        chorusDelays_[i]->prepare({ sampleRate, (juce::uint32)samplesPerBlock, 2 });
        chorusDelays_[i]->setDelay(0.02);  // 20ms base delay
    }

    // Prepare phaser stages
    for (int i = 0; i < numPhaserStages; ++i) {
        phaserStages_[i]->prepare({ sampleRate, (juce::uint32)samplesPerBlock, 2 });
    }

    // Prepare flanger delay
    flangerDelay_.prepare({ sampleRate, (juce::uint32)samplesPerBlock, 2 });
    flangerDelay_.setDelay(0.005);  // 5ms base delay

    // Prepare LFOs
    juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32)samplesPerBlock, 2 };
    lfo_.prepare(spec);
    lfoSlow_.prepare(spec);

    // Prepare process buffer
    processBuffer_.setSize(2, samplesPerBlock);
}

void ModulationEffect::reset() {
    for (int i = 0; i < maxVoices; ++i) {
        chorusDelays_[i]->reset();
    }

    for (int i = 0; i < numPhaserStages; ++i) {
        phaserStages_[i]->reset();
    }

    flangerDelay_.reset();
    lfo_.reset();
    lfoSlow_.reset();

    // Reset LFO phases for chorus
    for (int i = 0; i < maxVoices; ++i) {
        lfoPhases_[i] = static_cast<float>(i) / maxVoices;
    }
}

void ModulationEffect::process(juce::AudioBuffer<float>& buffer) {
    switch (type_) {
        case ModulationType::Chorus:
            processChorus(buffer);
            break;
        case ModulationType::Phaser:
            processPhaser(buffer);
            break;
        case ModulationType::Flanger:
            processFlanger(buffer);
            break;
    }
}

//==============================================================================
// Chorus Processing
//==============================================================================

void ModulationEffect::processChorus(juce::AudioBuffer<float>& buffer) {
    lfo_.setFrequency(rate_);

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];

            // Process each chorus voice
            float chorusSum = 0.0f;
            for (int voice = 0; voice < numVoices_; ++voice) {
                // Update LFO phase for this voice
                lfoPhases_[voice] += rate_ / sampleRate_;
                if (lfoPhases_[voice] >= 1.0f) {
                    lfoPhases_[voice] -= 1.0f;
                }

                // LFO modulation with spread (voices are out of phase)
                float lfoVal = std::sin(lfoPhases_[voice] * 6.28318530718
                                      + (voice * spread_ * 0.5f));

                // Modulate delay time (20ms ± 5ms)
                float delayTime = 0.02 + lfoVal * depth_ * 0.005;
                chorusDelays_[voice]->setDelay(delayTime);

                // Read from delay (with write)
                float delayed = chorusDelays_[voice]->popSample(channel);
                chorusDelays_[voice]->pushSample(channel, input);

                chorusSum += delayed;
            }

            chorusSum /= numVoices_;

            // Mix dry/wet
            channelData[sample] = input * (1.0f - mix_) + chorusSum * mix_;
        }
    }
}

//==============================================================================
// Phaser Processing
//==============================================================================

void ModulationEffect::processPhaser(juce::AudioBuffer<float>& buffer) {
    lfo_.setFrequency(rate_);

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];
            float output = input;

            // Get LFO value
            float lfoVal = lfo_.processSample(0.0f);

            // Modulate allpass frequency (200Hz - 2000Hz)
            float modFreq = 200.0f + (lfoVal * 0.5f + 0.5f) * depth_ * 1800.0f;

            // Process through allpass stages
            for (int stage = 0; stage < numPhaserStages; ++stage) {
                phaserStages_[stage]->setCutoffFrequency(modFreq);

                // Allpass processing
                float allpassOut = phaserStages_[stage]->processSample(output, channel);
                output = allpassOut;
            }

            // Add feedback
            output += output * feedback_;

            // Mix dry/wet
            channelData[sample] = input * (1.0f - mix_) + output * mix_;
        }
    }
}

//==============================================================================
// Flanger Processing
//==============================================================================

void ModulationEffect::processFlanger(juce::AudioBuffer<float>& buffer) {
    lfo_.setFrequency(rate_);

    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float input = channelData[sample];

            // LFO modulation of delay time (0.1ms - 10ms for through-zero)
            float lfoVal = lfo_.processSample(0.0f);
            float delayTime = 0.005 + lfoVal * depth_ * 0.005;
            flangerDelay_.setDelay(delayTime);

            // Read from delay
            float delayed = flangerDelay_.popSample(channel);

            // Write to delay
            flangerDelay_.pushSample(channel, input + delayed * feedback_);

            // Mix dry/wet
            channelData[sample] = input * (1.0f - mix_) + delayed * mix_;
        }
    }
}

void ModulationEffect::updateLFO() {
    // LFO updates are handled per-sample in process methods
}

} // namespace zenith
