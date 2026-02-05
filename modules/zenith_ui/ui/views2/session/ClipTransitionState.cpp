/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    ClipTransitionState.cpp
    Created: 2026-02-05
    Author:  Zenith DAW Team

    Implementation of advanced clip state transitions.
    ==============================================================================
*/

#include "ClipTransitionState.h"
#include <algorithm>
#include <cmath>

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

ClipStateTransition::ClipStateTransition() {
    currentState_ = ClipTransitionPresets::createEmptyState();
    previousState_ = currentState_;
    targetState_ = currentState_;
}

//==============================================================================
// State Management
//==============================================================================

void ClipStateTransition::transitionTo(const AnimationState& targetState,
                                      const TransitionConfig& config,
                                      std::function<void()> onComplete) {
    // Save current state as previous state
    previousState_ = currentState_;
    targetState_ = targetState;
    currentConfig_ = config;
    onCompleteCallback_ = onComplete;

    // Reset animation state
    isAnimating_ = true;
    isPaused_ = false;
    progress_ = 0.0f;
    elapsedMs_ = 0.0f;

    // If auto-update is disabled, mark as dirty
    if (!autoUpdate_) {
        invalidateComponent();
    }
}

void ClipStateTransition::cancelAnimation() {
    if (isAnimating_) {
        // Snap to target state
        currentState_ = targetState_;
        previousState_ = targetState_;
        progress_ = 1.0f;
        isAnimating_ = false;
        isPaused_ = false;
        elapsedMs_ = 0.0f;

        completeTransition();
        invalidateComponent();
    }
}

void ClipStateTransition::pauseAnimation() {
    if (isAnimating_ && !isPaused_) {
        isPaused_ = true;
    }
}

void ClipStateTransition::resumeAnimation() {
    if (isAnimating_ && isPaused_) {
        isPaused_ = false;
    }
}

//==============================================================================
// Animation Query
//==============================================================================

AnimationState ClipStateTransition::getCurrentState() const {
    return currentState_;
}

//==============================================================================
// Configuration
//==============================================================================

void ClipStateTransition::setDefaultEasing(zenith::animation::Easing easing) {
    defaultEasing_ = easing;
}

void ClipStateTransition::setDefaultDuration(float durationMs) {
    defaultDuration_ = durationMs;
}

void ClipStateTransition::setAutoUpdate(bool enabled) {
    autoUpdate_ = enabled;
}

//==============================================================================
// Update Loop
//==============================================================================

void ClipStateTransition::update(float deltaMs) {
    if (!isAnimating_ || isPaused_) {
        return;
    }

    // Update elapsed time
    elapsedMs_ += deltaMs;
    progress_ = std::min(elapsedMs_ / currentConfig_.durationMs, 1.0f);

    // Calculate eased progress
    float easedProgress = ease(progress_, currentConfig_.easing);

    // Interpolate between previous and target states
    currentState_ = interpolate(previousState_, targetState_, easedProgress, currentConfig_);

    // Handle physics-based animations
    if (currentConfig_.easing == zenith::animation::Easing::Spring) {
        float springProgress = calculateSpring(progress_, 0.3f, 0.8f);
        currentState_.scale = previousState_.scale +
                              (targetState_.scale - previousState_.scale) * springProgress;
    }

    // Update pulse animation
    if (currentConfig_.enablePulse) {
        currentState_.pulsePhase += deltaMs * 0.001f * currentConfig_.pulseFrequency;
        float pulseScale = 1.0f + (currentConfig_.maxPulseScale - 1.0f) *
                          (std::sin(currentState_.pulsePhase * juce::MathConstants<float>::twoPi) * 0.5f + 0.5f);
        currentState_.scale *= pulseScale;
    }

    // Invalidate component if needed
    if (!autoUpdate_) {
        invalidateComponent();
    }

    // Check if animation is complete
    if (progress_ >= 1.0f) {
        currentState_ = targetState_;
        completeTransition();
    }
}

void ClipStateTransition::onAnimationTick(float deltaMs) {
    update(deltaMs);
}

//==============================================================================
// Interpolation Methods
//==============================================================================

AnimationState ClipStateTransition::interpolate(const AnimationState& from,
                                               const AnimationState& to,
                                               float progress,
                                               const TransitionConfig& config) {
    AnimationState result;

    // Linear interpolation with easing applied externally
    result.opacity = from.opacity + (to.opacity - from.opacity) * progress;
    result.scale = from.scale + (to.scale - from.scale) * progress;
    result.rotation = from.rotation + (to.rotation - from.rotation) * progress;
    result.brightness = from.brightness + (to.brightness - from.brightness) * progress;
    result.saturation = from.saturation + (to.saturation - from.saturation) * progress;
    result.offset.x = from.offset.x + (to.offset.x - from.offset.x) * progress;
    result.offset.y = from.offset.y + (to.offset.y - from.offset.y) * progress;

    // Interpolate glow color
    if (config.enableGlow && (from.glowColor != 0 || to.glowColor != 0)) {
        int fromR = SkColorGetR(from.glowColor);
        int fromG = SkColorGetG(from.glowColor);
        int fromB = SkColorGetB(from.glowColor);
        int fromA = SkColorGetA(from.glowColor);

        int toR = SkColorGetR(to.glowColor);
        int toG = SkColorGetG(to.glowColor);
        int toB = SkColorGetB(to.glowColor);
        int toA = SkColorGetA(to.glowColor);

        result.glowColor = SkColorSetARGB(
            static_cast<U8CPU>(fromA + (toA - fromA) * progress),
            static_cast<U8CPU>(fromR + (toR - fromR) * progress),
            static_cast<U8CPU>(fromG + (toG - fromG) * progress),
            static_cast<U8CPU>(fromB + (toB - fromB) * progress)
        );
    }

    // Interpolate glow intensity
    result.glowIntensity = from.glowIntensity + (to.glowIntensity - from.glowIntensity) * progress;
    result.glowIntensity = std::clamp(result.glowIntensity, 0.0f, config.maxGlowIntensity);

    // Interpolate pulse phase (wrap around)
    result.pulsePhase = from.pulsePhase + (to.pulsePhase - from.pulsePhase) * progress;

    // Apply overshoot for elastic animations
    if (config.overshoot > 1.0f && progress > 0.8f) {
        float overshootProgress = (progress - 0.8f) / 0.2f;
        float overshootAmount = std::sin(overshootProgress * juce::MathConstants<float>::pi) *
                              (config.overshoot - 1.0f);
        result.scale += overshootAmount * 0.1f;
    }

    return result;
}

float ClipStateTransition::ease(float progress, zenith::animation::Easing easing) {
    using namespace zenith::animation;

    switch (easing) {
        case Easing::Linear:
            return progress;

        case Easing::EaseIn:
            return progress * progress;

        case Easing::EaseOut:
            return progress * (2.0f - progress);

        case Easing::EaseInOut:
            return progress < 0.5f ? 2.0f * progress * progress : -1.0f + (4.0f - 2.0f * progress) * progress;

        case Easing::EaseInCubic:
            return progress * progress * progress;

        case Easing::EaseOutCubic: {
            float f = progress - 1.0f;
            return f * f * f + 1.0f;
        }

        case Easing::EaseInOutCubic:
            return progress < 0.5f
                ? 4.0f * progress * progress * progress
                : 1.0f - std::pow(-2.0f * progress + 2.0f, 3.0f) / 2.0f;

        case Easing::EaseOutBack: {
            float f = 1.0f - progress;
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
            return 1.0f + c3 * f * f * f + c1 * f * f;
        }

        case Easing::EaseOutElastic: {
            float c4 = (2.0f * juce::MathConstants<float>::pi) / 3.0f;
            return progress == 0.0f ? 0.0f :
                   progress == 1.0f ? 1.0f :
                   std::pow(2.0f, -10.0f * progress) * std::sin((progress * 10.0f - 0.75f) * c4) + 1.0f;
        }

        case Easing::Spring: {
            // Spring physics with damped oscillation
            float stiffness = 0.3f;
            float damping = 0.8f;
            return calculateSpring(progress, stiffness, damping);
        }

        default:
            return progress;
    }
}

float ClipStateTransition::calculateSpring(float progress, float stiffness, float damping) {
    // Simplified spring calculation
    float omega = std::sqrt(stiffness);
    float gamma = damping * 0.5f;
    float amplitude = 1.0f;

    // Damped oscillation
    float exponential = std::exp(-gamma * progress);
    float oscillation = std::cos(omega * progress);

    return exponential * (amplitude + progress * gamma * amplitude) * oscillation;
}

//==============================================================================
// Private Helpers
//==============================================================================

void ClipStateTransition::completeTransition() {
    isAnimating_ = false;
    isPaused_ = false;
    progress_ = 1.0f;
    elapsedMs_ = 0.0f;

    // Execute completion callback
    if (onCompleteCallback_) {
        onCompleteCallback_();
        onCompleteCallback_ = nullptr;
    }
}

void ClipStateTransition::invalidateComponent() {
    // This would typically call markDirty() on the parent component
    // For now, we'll leave this as a placeholder
    // In real implementation, this would be:
    // if (auto* component = getParentComponent()) {
    //     component->markDirty();
    // }
}

} // namespace zenith::ui