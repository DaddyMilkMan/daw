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

#include "ZenithEnvelope.h"

namespace zenith {

//==============================================================================
// CONSTRUCTOR/DESTRUCTOR
//==============================================================================

ZenithEnvelope::ZenithEnvelope() = default;

ZenithEnvelope::~ZenithEnvelope() = default;

//==============================================================================
// CONFIGURATION
//==============================================================================

void ZenithEnvelope::reset() {
    phase_ = 0.0;
    currentLevel_ = 0.0f;
    active_ = false;
    inRelease_ = false;
    inDelay_ = false;
    inHold_ = false;
    inAttack_ = false;
    inDecay_ = false;
    inSustain_ = false;

    // Reset delay/hold states
    delaySamplesRemaining_ = 0;
    holdSamplesRemaining_ = 0;
}

void ZenithEnvelope::setSampleRate(double sr) {
    sampleRate_ = sr;
}

void ZenithEnvelope::setAttack(float seconds) {
    attackSamples_ = static_cast<float>(seconds * sampleRate_);
}

void ZenithEnvelope::setDecay(float seconds) {
    decaySamples_ = static_cast<float>(seconds * sampleRate_);
}

void ZenithEnvelope::setSustain(float level) {
    sustainLevel_ = juce::jlimit(0.0f, 1.0f, level);
}

void ZenithEnvelope::setRelease(float seconds) {
    releaseSamples_ = static_cast<float>(seconds * sampleRate_);
}

void ZenithEnvelope::setDelay(float seconds) {
    delaySamples_ = static_cast<float>(seconds * sampleRate_);
}

void ZenithEnvelope::setHoldTime(float seconds) {
    holdSamples_ = static_cast<float>(seconds * sampleRate_);
}

void ZenithEnvelope::setAttackCurve(EnvelopeCurve curve) {
    attackCurve_ = curve;
}

void ZenithEnvelope::setDecayCurve(EnvelopeCurve curve) {
    decayCurve_ = curve;
}

void ZenithEnvelope::setReleaseCurve(EnvelopeCurve curve) {
    releaseCurve_ = curve;
}

void ZenithEnvelope::setVelocityDepth(float amount) {
    velocityDepth_ = juce::jlimit(0.0f, 1.0f, amount);
}

void ZenithEnvelope::setVelocityCurve(EnvelopeCurve curve) {
    velocityCurve_ = curve;
}

//==============================================================================
// TRIGGERING
//==============================================================================

void ZenithEnvelope::noteOn(float velocity) {
    noteVelocity_ = velocity;
    active_ = true;
    inRelease_ = false;

    // Start delay phase if set
    if (delaySamples_ > 0) {
        inDelay_ = true;
        delaySamplesRemaining_ = delaySamples_;
    }

    // Reset to start of attack
    phase_ = 0.0;
    currentLevel_ = 0.0f;
}

void ZenithEnvelope::noteOff(bool allowTailOff) {
    if (allowTailOff) {
        inRelease_ = true;
    } else {
        active_ = false;
        inRelease_ = false;
        inDelay_ = false;
        inHold_ = false;
        inAttack_ = false;
        inDecay_ = false;
        inSustain_ = false;
    }
}

void ZenithEnvelope::retrigger() {
    // Reset to start of attack
    phase_ = 0.0;
    currentLevel_ = 0.0f;
    inRelease_ = false;
    inDelay_ = false;
    inHold_ = false;
    inAttack_ = false;
    inDecay_ = false;
    inSustain_ = false;
}

//==============================================================================
// PROCESSING
//==============================================================================

float ZenithEnvelope::getNextSample() {
    if (!active_) {
        return 0.0f;
    }

    // Process delay phase
    if (inDelay_) {
        if (delaySamplesRemaining_ > 0) {
            delaySamplesRemaining_--;
            return 0.0f;
        }
        inDelay_ = false;
        phase_ = 0.0;
    }

    // Calculate phase increment for current stage
    float currentInc = 0.0f;
    int currentStage = 0;

    if (!inRelease_ && !inSustain_) {
        // Determine which stage we're in
        if (!inAttack_ && !inDecay_) {
            currentStage = 0; // Attack
            currentInc = 1.0f / (attackSamples_ + 0.001f);
            inAttack_ = true;
        } else if (inAttack_ && !inDecay_) {
            currentStage = 1; // Still in attack
            currentInc = 1.0f / (attackSamples_ + 0.001f);
        } else if (!inAttack_ && inDecay_) {
            currentStage = 2; // Decay
            currentInc = 1.0f / (decaySamples_ + 0.001f);
            inDecay_ = false;
        }
    }

    // Check if attack is complete
    if (inAttack_ && phase_ >= 1.0) {
        inAttack_ = false;
        inHold_ = true;
        holdSamplesRemaining_ = holdSamples_;
    }

    // Check if hold is complete
    if (inHold_ && holdSamplesRemaining_ > 0) {
        if (phase_ >= 1.0f) {
            holdSamplesRemaining_--;
            if (holdSamplesRemaining_ <= 0) {
                inHold_ = false;
                inSustain_ = true;
            }
        }
    }

    // Check if we've entered sustain
    if (inSustain_ && phase_ >= 1.0f) {
        // Stay at sustain level
        currentLevel_ = sustainLevel_;
    }

    // Process release stage
    if (inRelease_) {
        if (releaseSamples_ > 0) {
            currentInc = 1.0f / (releaseSamples_ + 0.001f);
            float releasePhase = phase_;

            // Apply release curve
            float shapedPhase = applyCurve(releasePhase, releaseCurve_);
            currentLevel_ = sustainLevel_ * (1.0f - shapedPhase);

            phase_ += currentInc;

            if (phase_ >= 1.0f) {
                active_ = false;
                inRelease_ = false;
                currentLevel_ = 0.0f;
            }
        }
    } else {
        // Normal processing
        phase_ += currentInc;
    }

    // Apply velocity curve
    float velocityInfluence = applyVelocity(noteVelocity_);

    return currentLevel_ * velocityInfluence;
}

//==============================================================================
// INTERNAL HELPERS
//==============================================================================

float ZenithEnvelope::applyCurve(float phase, EnvelopeCurve curve) {
    switch (curve) {
        case EnvelopeCurve::Linear:
            return phase;
        case EnvelopeCurve::Exponential:
            return phase * phase;
        case EnvelopeCurve::Logarithmic:
            return std::sin(phase * 1.5708f);
        case EnvelopeCurve::TCurve:
            return phase >= 0.5f ?
                1.0f - 2.0f * (1.0f - phase) * (1.0f - phase) * 2.0f :
                2.0f * phase - 1.0f;
        case EnvelopeCurve::SCurve:
            return phase < 0.5f ?
                2.0f * phase * phase :
                1.0f - 2.0f * (1.0f - phase) * (1.0f - phase);
        case EnvelopeCurve::Step:
            return phase >= 0.5f ? 1.0f : 0.0f;
        case EnvelopeCurve::Slow:
            return std::pow(phase, 3.0f);
        default:
            return phase;
    }
}

float ZenithEnvelope::applyVelocity(float input) {
    switch (velocityCurve_) {
        case EnvelopeCurve::Linear:
            return input;
        case EnvelopeCurve::Exponential:
            return input * input;
        case EnvelopeCurve::Logarithmic:
            return input >= 0.5f ?
                std::sqrt(input * 2.0f - 1.0f) :
                input * input;
        default:
            return input;
    }
}

float ZenithEnvelope::calculateIncrement(float timeSeconds) {
    if (timeSeconds <= 0.001f) {
        return 1.0f; // Instant
    }
    return 1.0f / static_cast<float>(timeSeconds * sampleRate_);
}

} // namespace zenith
