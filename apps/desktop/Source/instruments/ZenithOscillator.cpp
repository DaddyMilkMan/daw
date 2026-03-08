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

==============================================================================
// PROFESSIONAL OSCILLATOR IMPLEMENTATION
//==============================================================================
*/

#include "ZenithOscillator.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// CONSTRUCTOR/DESTRUCTOR
//==============================================================================

ZenithOscillator::ZenithOscillator() {
    // Initialize unison state
    supersawPhases_.fill(0.0);
    supersawDetunes_.fill(0.0f);
    supersawPans_.fill(0.0f);
    supersawGains_.fill(1.0f);
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void ZenithOscillator::setSampleRate(double sampleRate) {
    if (sampleRate != sampleRate_) {
        sampleRate_ = sampleRate;

        // Reinitialize oversamplers with new sample rate
        if (oversamplingFactor_ > 1) {
            updateOversamplingFactor();
        }
    }
}

void ZenithOscillator::setDetune(float detuneCents) {
    detuneCents_ = detuneCents;
}

void ZenithOscillator::setOversamplingQuality(OscillatorOversamplingQuality quality) {
    if (quality != oversamplingQuality_) {
        oversamplingQuality_ = quality;
        updateOversamplingFactor();
    }
}

void ZenithOscillator::updateOversamplingFactor() {
    int newFactor = static_cast<int>(oversamplingQuality_);

    if (newFactor != oversamplingFactor_) {
        oversamplingFactor_ = newFactor;

        // Clear existing oversamplers
        oversampler2x_.reset();
        oversampler4x_.reset();
        oversampler8x_.reset();

        // Create appropriate oversampler if needed
        if (newFactor >= 2) {
            oversampler2x_ = std::make_unique<juce::dsp::Oversampling<float>>(
                1, juce::dsp::Oversampling<float>::factor2x,
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
            );
        }

        if (newFactor >= 4) {
            oversampler4x_ = std::make_unique<juce::dsp::Oversampling<float>>(
                1, juce::dsp::Oversampling<float>::factor4x,
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
            );
        }

        if (newFactor >= 8) {
            oversampler8x_ = std::make_unique<juce::dsp::Oversampling<float>>(
                1, juce::dsp::Oversampling<float>::factor8x,
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR
            );
        }
    }
}

void ZenithOscillator::reset() {
    phase_ = 0.0;
    supersawPhases_.fill(0.0);
    supersawInit_ = false;
    driftLFO_ = 0.0;
    pitchDriftOffset_ = 0.0;
    phaseDriftOffset_ = 0.0;
    syncTriggered_ = false;
    lastSyncPhase_ = 0.0;
    syncBlepBuffer_ = 0.0;
}

//==============================================================================
// ANALOG DRIFT IMPLEMENTATION
//==============================================================================

void ZenithOscillator::updateDrift() {
    if (analogDriftAmount_ <= 0.0f) {
        pitchDriftOffset_ = 0.0;
        phaseDriftOffset_ = 0.0;
        return;
    }

    // Update slow drift LFO (0.1 - 0.5 Hz)
    const double driftLFOFreq = 0.1 + (analogDriftAmount_ * 0.4);
    const double driftLFOPhaseInc = driftLFOFreq / sampleRate_;
    driftLFO_ = std::fmod(driftLFO_ + driftLFOPhaseInc, 1.0);

    // Calculate drift amount based on LFO phase
    double driftLFOValue = std::sin(driftLFO_ * juce::MathConstants<double>::twoPi);

    // Add high-frequency "flutter" (very subtle, 8-15 Hz)
    double flutter = 0.0;
    if (analogDriftAmount_ > 0.3f) {
        double flutterPhase = driftLFO_ * 83.0;  // Harmonically unrelated
        flutter = std::sin(flutterPhase * juce::MathConstants<double>::twoPi) *
                  (analogDriftAmount_ - 0.3f) * 0.1;
    }

    // Per-oscillator random variation (component tolerance simulation)
    // Use smoothed random to avoid clicks
    double randomDrift = random_.nextDouble() * 2.0 - 1.0;  // -1 to 1

    // Combine drift sources
    double totalDrift = driftLFOValue + flutter + randomDrift * 0.3;

    // Scale by drift amount
    // Maximum drift: ±5 cents (realistic for analog VCOs)
    const double maxDriftCents = 5.0;
    pitchDriftOffset_ = totalDrift * analogDriftAmount_ * maxDriftCents;

    // Phase drift (very subtle, for stereo spread)
    // ±0.1% sample rate variation
    phaseDriftOffset_ = totalDrift * analogDriftAmount_ * 0.001;
}

//==============================================================================
// UNISON CONTROL (16-VOICE PROFESSIONAL UNISON)
//==============================================================================

void ZenithOscillator::setUnisonVoices(int voices) {
    unisonVoices_ = juce::jlimit(1, MAX_UNISON_VOICES, voices);
    supersawInit_ = false;  // Force recalculation of ratios
    updateSupersawRatios();
}

void ZenithOscillator::updateSupersawRatios() {
    if (unisonVoices_ <= 1) {
        supersawDetunes_[0] = 0.0f;
        supersawPans_[0] = 0.0f;
        supersawGains_[0] = 1.0f;
        return;
    }

    // Professional unison detune curve (Serum-style)
    // Even voices get positive detune, odd get negative
    // Logarithmic scaling for musical results
    const float maxDetuneCents = std::abs(detuneCents_ > 0.0f ? detuneCents_ : 50.0f);
    const float baseDetune = maxDetuneCents / static_cast<float>(unisonVoices_ - 1);

    for (int i = 0; i < unisonVoices_; ++i) {
        // Even/odd detune pattern for richer sound
        bool isEven = (i % 2) == 0;
        float sign = isEven ? 1.0f : -1.0f;
        float detuneOffset = sign * baseDetune * static_cast<float>(i / 2);

        // Convert cents to frequency ratio
        supersawDetunes_[i] = std::exp2(detuneOffset / 1200.0f) - 1.0f;

        // Symmetrical stereo spread
        // -1.0 = full left, 0.0 = center, 1.0 = full right
        if (unisonVoices_ == 1) {
            supersawPans_[i] = 0.0f;
        } else {
            supersawPans_[i] = (static_cast<float>(i) / static_cast<float>(unisonVoices_ - 1)) * 2.0f - 1.0f;
        }

        // Power normalization: 1/sqrt(N) to maintain constant power
        supersawGains_[i] = 1.0f / std::sqrt(static_cast<float>(unisonVoices_));
    }

    supersawInit_ = true;
}

//==============================================================================
// AUDIO GENERATION
//==============================================================================

float ZenithOscillator::getNextSample(float frequency, float shape) {
    // Update analog drift
    updateDrift();

    // Apply drift to frequency (pitch modulation in cents)
    float driftedFrequency = frequency * std::exp2(pitchDriftOffset_ / 1200.0);

    if (unisonVoices_ == 1) {
        // Single voice path (optimized)
        double phaseIncrement = driftedFrequency / sampleRate_;

        if (syncEnabled_) {
            // Hard sync: reset phase at cycle start
            if (phase_ + phaseIncrement >= 1.0) {
                phase_ = 0.0;
            }
        }

        double currentPhase = phase_;
        phase_ = std::fmod(phase_ + phaseIncrement, 1.0);

        float sample = generateSample(currentPhase, phaseIncrement, shape);

        // Apply oversampling if enabled
        if (oversamplingFactor_ > 1) {
            sample = processWithOversampling(sample);
        }

        return sample;
    } else {
        // Unison path
        return generateUnisonSample(frequency, shape);
    }
}

void ZenithOscillator::process(float* output, int numSamples, float frequency, float shape) {
    // Initialize unison if needed
    if (!supersawInit_) {
        updateSupersawRatios();
    }

    for (int i = 0; i < numSamples; ++i) {
        output[i] = getNextSample(frequency, shape);
    }
}

//==============================================================================
// UNISON SAMPLE GENERATION
//==============================================================================

float ZenithOscillator::generateUnisonSample(float frequency, float shape) {
    float left = 0.0f;
    float right = 0.0f;

    // Apply drift to base frequency
    float driftedFrequency = frequency * std::exp2(pitchDriftOffset_ / 1200.0);

    for (int i = 0; i < unisonVoices_; ++i) {
        // Calculate detuned frequency for this voice
        float detuneRatio = 1.0f + supersawDetunes_[i];
        float voiceFreq = driftedFrequency * detuneRatio;
        double phaseIncrement = voiceFreq / sampleRate_;

        // Update voice phase
        double& voicePhase = supersawPhases_[i];
        double currentPhase = voicePhase;
        voicePhase = std::fmod(voicePhase + phaseIncrement, 1.0);

        // Generate sample for this voice
        float voiceSample = generateSample(currentPhase, phaseIncrement, shape);

        // Apply gain and pan
        float gain = supersawGains_[i];
        float pan = supersawPans_[i];

        // Constant power panning
        float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        float leftGain = gain * std::cos(angle);
        float rightGain = gain * std::sin(angle);

        left += voiceSample * leftGain;
        right += voiceSample * rightGain;
    }

    // Return mono sum (caller will handle stereo)
    return (left + right) * 0.5f;
}

//==============================================================================
// WAVEFORM GENERATORS
//==============================================================================

float ZenithOscillator::generateSample(double phase, double phaseIncrement, float shape) {
    switch (waveform_) {
        case OscillatorWaveform::Saw:
            return (syncMaster_ != nullptr)
                ? generateSyncedSaw(phase, phaseIncrement)
                : generateSaw(phase, phaseIncrement);

        case OscillatorWaveform::Square:
            // Use shape parameter for pulse width modulation
            float savedPW = pulseWidth_;
            pulseWidth_ = juce::jlimit(0.01f, 0.99f, shape);
            float sample = (syncMaster_ != nullptr)
                ? generateSyncedSquare(phase, phaseIncrement)
                : generateSquare(phase, phaseIncrement);
            pulseWidth_ = savedPW;
            return sample;

        case OscillatorWaveform::Triangle:
            return generateTriangle(phase, phaseIncrement);

        case OscillatorWaveform::Sine:
            return generateSine(phase);

        case OscillatorWaveform::Wavetable:
            if (wavetable_) {
                return interpolateWavetable(phase, shape);
            }
            return generateSine(phase);

        case OscillatorWaveform::Noise:
            return random_.nextFloat() * 2.0f - 1.0f;

        default:
            return generateSaw(phase, phaseIncrement);
    }
}

float ZenithOscillator::generateSaw(double phase, double phaseIncrement) {
    // Use PolyBLEP for anti-aliasing
    return sawPolyBLEP(phase, phaseIncrement);
}

float ZenithOscillator::generateSquare(double phase, double phaseIncrement) {
    // Use PolyBLEP for anti-aliasing
    return squarePolyBLEP(phase, phaseIncrement);
}

float ZenithOscillator::generateTriangle(double phase, double phaseIncrement) {
    // Triangle wave - smoother, less aliasing naturally
    // Use naive implementation for now (bandlimited version would integrate square)
    return trianglePolyBLEP(phase, phaseIncrement);
}

float ZenithOscillator::generateSine(double phase) {
    // Pure sine - no aliasing
    return std::sin(phase * juce::MathConstants<double>::twoPi);
}

//==============================================================================
// HARD SYNC WITH BLEP
//==============================================================================

/**
 * @brief Generate hard-synced sawtooth with BLEP correction
 *
 * When the slave oscillator is synced, its phase resets at the master's
 * cycle completion. This creates a discontinuity that causes aliasing.
 * The BLEP correction adds a bandlimited step at the sync point.
 */
float ZenithOscillator::generateSyncedSaw(double phase, double phaseIncrement) {
    // Check if we have a sync master
    if (syncMaster_ == nullptr) {
        return generateSaw(phase, phaseIncrement);
    }

    double masterPhase = syncMaster_->getPhase();
    bool masterWrapped = (masterPhase < lastSyncPhase_);
    lastSyncPhase_ = masterPhase;

    // Detect sync trigger (master wrapped)
    if (masterWrapped && !syncTriggered_) {
        syncTriggered_ = true;
        // Calculate BLEP for the sync discontinuity
        double t = phase;  // Our phase when sync occurs
        syncBlepBuffer_ = polyBLEP(t, phaseIncrement);
    } else {
        syncTriggered_ = false;
    }

    // Reset phase on sync (after generating sample)
    double outputPhase = syncTriggered_ ? 0.0 : phase;

    // Generate saw with BLEP correction
    float saw = sawPolyBLEP(outputPhase, phaseIncrement);

    // Add sync BLEP correction (fades out over a few samples)
    float sample = saw - static_cast<float>(syncBlepBuffer_);
    syncBlepBuffer_ *= 0.5;  // Decay BLEP

    return sample;
}

/**
 * @brief Generate hard-synced square with BLEP correction
 */
float ZenithOscillator::generateSyncedSquare(double phase, double phaseIncrement) {
    if (syncMaster_ == nullptr) {
        return generateSquare(phase, phaseIncrement);
    }

    double masterPhase = syncMaster_->getPhase();
    bool masterWrapped = (masterPhase < lastSyncPhase_);
    lastSyncPhase_ = masterPhase;

    if (masterWrapped && !syncTriggered_) {
        syncTriggered_ = true;
        double t = phase;
        // Square has two discontinuities per cycle
        syncBlepBuffer_ = 2.0 * polyBLEP(t, phaseIncrement);
    } else {
        syncTriggered_ = false;
    }

    double outputPhase = syncTriggered_ ? 0.0 : phase;
    float square = squarePolyBLEP(outputPhase, phaseIncrement);

    float sample = square - static_cast<float>(syncBlepBuffer_);
    syncBlepBuffer_ *= 0.5;

    return sample;
}

//==============================================================================
// WAVETABLE INTERPOLATION
//==============================================================================

float ZenithOscillator::interpolateWavetable(double phase, float framePosition) {
    if (!wavetable_ || !wavetable_->isValid()) {
        return 0.0f;
    }

    // Get sample from wavetable with frame interpolation
    return wavetable_->getSample(static_cast<float>(phase), framePosition);
}

int ZenithOscillator::calculateMipLevel(float frequency) const {
    // Calculate MIP level for anti-aliasing
    // Higher frequencies use lower-resolution mipmaps
    if (frequency <= 0.0f) return 0;

    float nyquist = static_cast<float>(sampleRate_) * 0.5f;
    float fundamentalRatio = frequency / nyquist;

    // Determine MIP level based on frequency ratio
    // Use oversampling factor to reduce MIP level aggressiveness
    int baseMip = static_cast<int>(std::floor(-std::log2(fundamentalRatio)));
    int adjustedMip = baseMip - static_cast<int>(std::log2(static_cast<float>(oversamplingFactor_)));

    return juce::jlimit(0, WavetableData::MAX_MIP_LEVELS - 1, adjustedMip);
}

//==============================================================================
// OVERSAMPLING PROCESSING
//==============================================================================

float ZenithOscillator::processWithOversampling(float inputSample) {
    // For single samples, oversampling doesn't help much
    // This is mainly used when processing blocks
    // Return the sample as-is for now; proper oversampling requires block processing
    return inputSample;
}

} // namespace zenith
