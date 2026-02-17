/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

// ClipTransitionState.h

#include "../../framework/Animation.h"
#include "../../design-system/ZenithTheme.h"
#include <functional>
#include <map>
#include <memory>

namespace zenith::ui {

/**
 * @brief Represents a single state in the transition system
 */
class ClipStateTransition {
public:
    //==========================================================================
    // Constructor and lifecycle
    //==========================================================================

    ClipStateTransition();
    ~ClipStateTransition() = default;

    //==========================================================================
    // State Management
    //==========================================================================

    /**
     * @brief Transition to a new state
     * @param targetState The target animation state
     * @param config Animation configuration
     * @param onComplete Callback when transition completes
     */
    void transitionTo(const AnimationState& targetState,
                     const TransitionConfig& config,
                     std::function<void()> onComplete = nullptr);

    /**
     * @brief Cancel current animation and snap to target
     */
    void cancelAnimation();

    /**
     * @brief Pause current animation
     */
    void pauseAnimation();

    /**
     * @brief Resume paused animation
     */
    void resumeAnimation();

    //==========================================================================
    // Animation Query
    //==========================================================================

    /**
     * @brief Get current animation state (interpolated)
     */
    AnimationState getCurrentState() const;

    /**
     * @brief Check if animation is active
     */
    bool isAnimating() const { return isAnimating_; }

    /**
     * @brief Check if animation is paused
     */
    bool isPaused() const { return isPaused_; }

    /**
     * @brief Get animation progress (0.0 to 1.0)
     */
    float getProgress() const { return progress_; }

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set easing function for all transitions
     */
    void setDefaultEasing(zenith::animation::Easing easing) { defaultEasing_ = easing; }

    /**
     * @brief Set default animation duration
     */
    void setDefaultDuration(float durationMs) { defaultDuration_ = durationMs; }

    /**
     * @brief Enable/disable automatic animation updates
     */
    void setAutoUpdate(bool enabled) { autoUpdate_ = enabled; }

    //==========================================================================
    // Update Loop
    //==========================================================================

    /**
     * @brief Update animation state (call from animation system)
     * @param deltaMs Time elapsed since last update
     */
    void update(float deltaMs);

    /**
     * @brief Animation tick for SkiaComponent integration
     */
    void onAnimationTick(float deltaMs) override;

private:
    //==========================================================================
    // Animation State
    //==========================================================================

    AnimationState currentState_;
    AnimationState targetState_;
    AnimationState previousState_;

    TransitionConfig currentConfig_;
    zenith::animation::Easing defaultEasing_ = zenith::animation::Easing::EaseInOutCubic;
    float defaultDuration_ = 200.0f;

    bool isAnimating_ = false;
    bool isPaused_ = false;
    float progress_ = 0.0f;
    float elapsedMs_ = 0.0f;

    std::function<void()> onCompleteCallback_;

    //==========================================================================
    // Interpolation Methods
    //==========================================================================

    AnimationState interpolate(const AnimationState& from,
                              const AnimationState& to,
                              float progress,
                              const TransitionConfig& config);

    float ease(float progress, zenith::animation::Easing easing);
    float calculateSpring(float progress, float stiffness, float damping);

    //==========================================================================
    // Private Helpers
    //==========================================================================

    void completeTransition();
    void invalidateComponent();
};

/**
 * @brief Transition presets for common clip states
 */
namespace ClipTransitionPresets {

    static AnimationState createEmptyState() {
        AnimationState state;
        state.opacity = 0.3f;
        state.scale = 0.95f;
        state.brightness = 0.8f;
        return state;
    }

    static AnimationState createStoppedState() {
        AnimationState state;
        state.opacity = 1.0f;
        state.scale = 1.0f;
        state.brightness = 1.0f;
        state.saturation = 1.0f;
        return state;
    }

    static AnimationState createPlayingState() {
        AnimationState state;
        state.opacity = 1.0f;
        state.scale = 1.05f;
        state.brightness = 1.2f;
        state.saturation = 1.3f;
        state.glowColor = SkColorSetARGB(180, 59, 130, 246);
        state.glowIntensity = 0.6f;
        state.pulsePhase = 0.0f;
        return state;
    }

    static AnimationState createQueuedState() {
        AnimationState state;
        state.opacity = 0.8f;
        state.scale = 1.02f;
        state.brightness = 1.1f;
        state.glowColor = SkColorSetARGB(120, 251, 191, 36);
        state.glowIntensity = 0.4f;
        return state;
    }

    static AnimationState createRecordingState() {
        AnimationState state;
        state.opacity = 1.0f;
        state.scale = 1.0f;
        state.brightness = 1.3f;
        state.glowColor = SkColorSetARGB(220, 239, 68, 68);
        state.glowIntensity = 0.8f;
        state.pulsePhase = 0.0f;
        return state;
    }

    static AnimationState createStoppingState() {
        AnimationState state;
        state.opacity = 0.7f;
        state.scale = 0.98f;
        state.brightness = 0.9f;
        state.glowColor = SkColorSetARGB(100, 156, 163, 175);
        state.glowIntensity = 0.3f;
        return state;
    }

    static TransitionConfig getTransitionForClipState(zenith::ui::ClipSlotState fromState,
                                                    zenith::ui::ClipSlotState toState) {
        using zenith::ui::ClipSlotState;

        // Quick transitions for state changes that shouldn't be distracting
        if (fromState == ClipSlotState::Empty && toState == ClipSlotState::Stopped) {
            return TransitionConfig::createQuickTransition();
        }

        if (fromState == ClipSlotState::Stopped && toState == ClipSlotState::Empty) {
            return TransitionConfig::createQuickTransition();
        }

        // Smooth transitions for playing/recording
        if (fromState == ClipSlotState::Stopped && toState == ClipSlotState::Playing) {
            return TransitionConfig::createSmoothTransition();
        }

        if (fromState == ClipSlotState::Playing && toState == ClipSlotState::Stopped) {
            return TransitionConfig::createSmoothTransition();
        }

        // Elastic transitions for queueing
        if (fromState == ClipSlotState::Stopped && toState == ClipSlotState::Queued) {
            return TransitionConfig::createElasticTransition();
        }

        // High-energy transitions for recording
        if (fromState == ClipSlotState::Stopped && toState == ClipSlotState::Recording) {
            TransitionConfig config = TransitionConfig::createElasticTransition();
            config.durationMs = 150.0f;
            config.easing = zenith::animation::Easing::EaseOutExpo;
            return config;
        }

        // Default smooth transition
        return TransitionConfig::createSmoothTransition();
    }
}

} // namespace zenith::ui