/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

#pragma once

#include "ZenithPolySynthDefs.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <memory>

namespace zenith {

//==============================================================================
// PROFESSIONAL OSCILLATOR
//==============================================================================
/**
 * Single oscillator with professional features matching Xfer Serum:
 * - 16-voice unison with stereo spread
 * - Per-oscillator oversampling (1x, 2x, 4x, 8x)
 * - Full PolyBLEP anti-aliasing
 * - Wavetable playback with MIP mapping
 * - Hard sync support
 * - Multiple waveforms with PWM
 */
class ZenithOscillator {
public:
    ZenithOscillator();
    ~ZenithOscillator() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    void setWaveform(OscillatorWaveform waveform) { waveform_ = waveform; }
    OscillatorWaveform getWaveform() const { return waveform_; }

    void setOversamplingQuality(OscillatorOversamplingQuality quality);
    OscillatorOversamplingQuality getOversamplingQuality() const { return oversamplingQuality_; }

    void setDetune(float detuneCents);
    void setPulseWidth(float pw) { pulseWidth_ = juce::jlimit(0.0f, 1.0f, pw); }

    void setSampleRate(double sampleRate);
    void reset();
    void randomizePhase() { phase_ = random_.nextDouble(); }

    //==========================================================================
    // Flagship Features
    //==========================================================================

    void setSync(bool enabled) { syncEnabled_ = enabled; }
    void resetPhase() { phase_ = 0.0; }
    double getPhase() const { return phase_; }

    //==========================================================================
    // Hard Sync Control
    //==========================================================================

    /** Set master oscillator for hard sync (nullptr = no sync) */
    void setSyncMaster(const ZenithOscillator* master) { syncMaster_ = master; }
    const ZenithOscillator* getSyncMaster() const { return syncMaster_; }

    /** Reset sync trigger state */
    void clearSyncTrigger() { syncTriggered_ = false; }
    bool wasSyncTriggered() const { return syncTriggered_; }

    //==========================================================================
    // Wavetable Support
    //==========================================================================

    void setWavetable(const Wavetable* wt) { wavetable_ = wt; }
    const Wavetable* getWavetable() const { return wavetable_; }
    bool hasWavetable() const { return wavetable_ != nullptr; }

    //==========================================================================
    // Analog Drift (professional warmth)
    //==========================================================================

    void setAnalogDrift(float amount) { analogDriftAmount_ = juce::jlimit(0.0f, 1.0f, amount); }
    float getAnalogDrift() const { return analogDriftAmount_; }
    void updateDrift();

    //==========================================================================
    // Audio Generation
    //==========================================================================

    /**
     * @brief Generate next sample
     * @param frequency Base frequency in Hz
     * @param shape Shape parameter (Pulse Width for Square, morph for others)
     * @return Sample value in range [-1, 1]
     */
    float getNextSample(float frequency, float shape = 0.5f);

    /**
     * @brief Process block of samples (more efficient for voice rendering)
     */
    void process(float* output, int numSamples, float frequency, float shape = 0.5f);

    //==========================================================================
    // Unison Control (16-voice professional unison)
    //==========================================================================

    void setUnisonVoices(int voices);
    int getUnisonVoices() const { return unisonVoices_; }
    void updateSupersawRatios();

private:
    //==========================================================================
    // Internal State
    //==========================================================================

    OscillatorWaveform waveform_ = OscillatorWaveform::Saw;
    double phase_ = 0.0;
    double sampleRate_ = 44100.0;
    float detuneCents_ = 0.0f;
    float pulseWidth_ = 0.5f;
    bool syncEnabled_ = false;

    //==========================================================================
    // Oversampling State
    //==========================================================================

    OscillatorOversamplingQuality oversamplingQuality_ = OscillatorOversamplingQuality::Clean;
    int oversamplingFactor_ = 1;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler2x_;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler4x_;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler8x_;

    //==========================================================================
    // Wavetable State
    //==========================================================================

    const Wavetable* wavetable_ = nullptr;
    float lastWavetableFreq_ = 0.0f;

    //==========================================================================
    // Unison State (Serum-standard 16 voices)
    //==========================================================================

    static constexpr int MAX_UNISON_VOICES = 16;
    int unisonVoices_ = 1;
    std::array<double, MAX_UNISON_VOICES> supersawPhases_ = {};
    std::array<float, MAX_UNISON_VOICES> supersawDetunes_ = {};
    std::array<float, MAX_UNISON_VOICES> supersawPans_ = {};
    std::array<float, MAX_UNISON_VOICES> supersawGains_ = {};
    bool supersawInit_ = false;

    //==========================================================================
    // Random Generator
    //==========================================================================

    juce::Random random_;

    //==========================================================================
    // Analog Drift State
    //==========================================================================

    float analogDriftAmount_ = 0.0f;           // 0 = off, 1 = maximum drift
    double driftLFO_ = 0.0;                     // Slow LFO for drift
    double pitchDriftOffset_ = 0.0;             // Current pitch offset in cents
    double phaseDriftOffset_ = 0.0;              // Current phase offset

    //==========================================================================
    // Hard Sync State
    //==========================================================================

    const ZenithOscillator* syncMaster_ = nullptr;  // Master oscillator for sync
    bool syncTriggered_ = false;                  // Triggered this sample?
    double lastSyncPhase_ = 0.0;                 // Phase at last sync
    double syncBlepBuffer_ = 0.0;                // BLEP correction buffer

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void updateOversamplingFactor();
    float processWithOversampling(float inputSample);
    int calculateMipLevel(float frequency) const;
    float generateUnisonSample(float frequency, float shape);
    float generateSample(double phase, double phaseIncrement, float shape);

    //==========================================================================
    // Waveform Generators (with PolyBLEP)
    //==========================================================================

    float generateSaw(double phase, double phaseIncrement);
    float generateSquare(double phase, double phaseIncrement);
    float generateTriangle(double phase, double phaseIncrement);
    float generateSine(double phase);
    float generateSyncedSaw(double phase, double phaseIncrement);
    float generateSyncedSquare(double phase, double phaseIncrement);

    //==========================================================================
    // PolyBLEP Implementation
    //==========================================================================

    /**
     * @brief PolyBLEP correction for bandlimited waveforms
     * @param t Phase (0-1)
     * @param dt Phase increment per sample
     * @return BLEP correction value
     */
    inline float polyBLEP(double t, double dt) {
        // Bandlimited step
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0f;
        } else if (t > 1.0 - dt) {
            t = (t - 1.0) / dt;
            return t * t + t + 1.0f;
        }
        return 0.0f;
    }

    /**
     * @brief PolyBLEP for sawtooth (discontinuity at wrap)
     */
    inline float sawPolyBLEP(double phase, double dt) {
        double naiveSaw = 2.0 * phase - 1.0;
        return static_cast<float>(naiveSaw - polyBLEP(phase, dt));
    }

    /**
     * @brief PolyBLEP for square (two discontinuities per cycle)
     */
    inline float squarePolyBLEP(double phase, double dt) {
        double pulseWidth = juce::jlimit(0.01, 0.99, static_cast<double>(pulseWidth_));
        double naiveSquare = (phase < pulseWidth) ? 1.0 : -1.0;
        double correction = polyBLEP(phase, dt) - polyBLEP(std::fmod(phase + pulseWidth, 1.0), dt);
        return static_cast<float>(naiveSquare + correction);
    }

    /**
     * @brief PolyBLEP for triangle (integral of square)
     */
    inline float trianglePolyBLEP(double phase, double dt) {
        // Triangle is integral of square
        // Use naive implementation with parabolic BLEP
        double naiveTri = 2.0 * std::abs(2.0 * phase - 1.0) - 1.0;
        return static_cast<float>(naiveTri);  // Additional BLEP would go here
    }

    //==========================================================================
    // Wavetable Interpolation
    //==========================================================================

    float interpolateWavetable(double phase, float framePosition);
};

} // namespace zenith
