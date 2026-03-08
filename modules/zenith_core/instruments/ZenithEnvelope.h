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
#include <cmath>

namespace zenith {

//==============================================================================
// ENVELOPE CURVE TYPE
//==============================================================================
/**
 * Sub-sample accurate ADSR envelope with professional curve shaping
 * Matches Serum 2 envelope quality
 */
enum class EnvelopeCurve {
    Linear,          ///< Linear transitions
    Exponential,      ///< Exponential attack/decay
    Logarithmic,      ///< Logarithmic decay
    TCurve,          ///< T-shaped curve
    SCurve,          ///< S-shaped curve
    Steep,           ///< Instant attack
    Slow              ///< Slow attack/release
};

//==============================================================================
// SUB-SAMPLE ACCURATE ENVELOPE
//==============================================================================
/**
 * Professional ADSR envelope with sub-sample timing
 *
 * FEATURES:
 * - Sub-sample accurate timing (no zipper noise)
 * - Multiple curve types
 * - Velocity curve control
 * - Delay and hold times
 * - Retrigger options
 */
class ZenithEnvelope {
public:
    ZenithEnvelope() = default;

    //==========================================================================
    // Configuration
    //==========================================================================

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void reset();

    //==========================================================================
    // ADSR Parameters
    //==========================================================================

    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level);      // 0-1
    void setRelease(float seconds);
    void setDelay(float seconds);
    void setHoldTime(float seconds);

    //==========================================================================
    // Curve Control
    //==========================================================================

    void setAttackCurve(EnvelopeCurve curve) { attackCurve_ = curve; }
    void setDecayCurve(EnvelopeCurve curve) { decayCurve_ = curve; }
    void setReleaseCurve(EnvelopeCurve curve) { releaseCurve_ = curve; }

    //==========================================================================
    // Velocity Control
    //==========================================================================

    void setVelocityDepth(float amount);    // 0-1
    void setVelocityCurve(EnvelopeCurve curve);

    //==========================================================================
    // Triggering
    //==========================================================================

    void noteOn(float velocity = 1.0f);
    void noteOff(bool allowTailOff = true);
    void retrigger();   // Retrigger from start

    //==========================================================================
    // Processing
    //==========================================================================

    /**
     * @brief Generate next output sample with sub-sample accuracy
     * @return Envelope value in range [0, 1]
     */
    float getNextSample();
    float getCurrentLevel() const { return currentLevel_; }
    bool isActive() const { return active_ || inRelease_; }
    bool isInRelease() const { return inRelease_; }

private:
    //==========================================================================
    // Timing State (Sub-sample accurate)
    //==========================================================================

    double sampleRate_ = 44100.0;
    double phase_ = 0.0;              // Sub-sample phase counter
    double inc_ = 0.0;                 // Phase increment per sample
    float currentLevel_ = 0.0f;

    // Timing parameters (in samples)
    float attackSamples_ = 0.0f;
    float decaySamples_ = 0.0f;
    float sustainLevel_ = 0.7f;
    float releaseSamples_ = 0.0f;
    float delaySamples_ = 0.0f;
    float holdSamples_ = 0.0f;

    // Curve types
    EnvelopeCurve attackCurve_ = EnvelopeCurve::Exponential;
    EnvelopeCurve decayCurve_ = EnvelopeCurve::Exponential;
    EnvelopeCurve releaseCurve_ = EnvelopeCurve::Exponential;
    EnvelopeCurve velocityCurve_ = EnvelopeCurve::Linear;

    // Velocity
    float velocityDepth_ = 1.0f;
    float noteVelocity_ = 1.0f;

    // State
    bool active_ = false;
    bool inRelease_ = false;
    bool inDelay_ = false;
    bool inHold_ = false;
    bool inAttack_ = false;
    bool inDecay_ = false;
    bool inSustain_ = false;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    /**
     * @brief Apply curve shaping to normalized phase
     */
    float applyCurve(float phase, EnvelopeCurve curve);

    /**
     * @brief Apply velocity curve
     */
    float applyVelocity(float input);

    /**
     * @brief Calculate sub-sample phase increment
     */
    float calculateIncrement(float timeSeconds);
};

//==============================================================================
// MULTI-STAGE ENVELOPE (for LFO and Mod sources)
//==============================================================================
/**
 * Multi-stage envelope for LFO, with shape control
 */
class ZenithLFOPEnvelope {
public:
    ZenithLFOEnvelope() = default;

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void reset();

    // 5-stage: Attack - Decay 1 - Decay 2 - Sustain - Release
    void setADSR(float a, float d1, float d2, float s, float r);

    // Shape
    void setShape(float amount);  // -1 (triangle) to +1 (square)

    // Trigger
    void trigger();
    void release();

    // Output
    float getNextSample();
    float getCurrentLevel() const { return currentLevel_; }

private:
    double sampleRate_ = 44100.0;
    float currentLevel_ = 0.0f;
    float phase_ = 0.0f;
    float inc_ = 0.0f;
    float shape_ = 0.0f;

    // Stage times in samples
    float attackSamples_ = 0.0f;
    float decay1Samples_ = 0.0f;
    float decay2Samples_ = 0.0f;
    float sustainLevel_ = 0.5f;
    float releaseSamples_ = 0.0f;

    // State
    bool active_ = false;
    bool releasing_ = false;
    int currentStage_ = 0;
};

} // namespace zenith
