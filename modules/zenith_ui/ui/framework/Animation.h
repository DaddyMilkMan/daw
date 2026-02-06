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
*/

#pragma once

// Animation.h


#include <atomic>
#include <cmath>
#include <functional>
#include <juce_core/juce_core.h>

namespace zenith {
namespace animation {

// ============================================================================
// EASING FUNCTIONS
// ============================================================================

/**
 * Easing curve types for smooth animations.
 */
enum class Easing {
    Linear,      // Constant speed
    EaseIn,      // Slow start, fast end (quadratic)
    EaseOut,     // Fast start, slow end (quadratic)
    EaseInOut,   // Slow start and end (cubic)
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseOutBack,  // Slight overshoot
    EaseOutElastic, // Bouncy overshoot
    Spring        // Physics-based spring (use SpringConfig)
};

/**
 * Apply easing function to a normalized time value (0.0 to 1.0).
 * Returns the eased progress value.
 */
inline float applyEasing(float t, Easing easing) {
    // Clamp input
    t = juce::jlimit(0.0f, 1.0f, t);
    
    switch (easing) {
        case Easing::Linear:
            return t;
            
        case Easing::EaseIn:
            return t * t;
            
        case Easing::EaseOut:
            return t * (2.0f - t);
            
        case Easing::EaseInOut:
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
            
        case Easing::EaseInCubic:
            return t * t * t;
            
        case Easing::EaseOutCubic: {
            float f = t - 1.0f;
            return f * f * f + 1.0f;
        }
            
        case Easing::EaseInOutCubic:
            return t < 0.5f 
                ? 4.0f * t * t * t 
                : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
            
        case Easing::EaseInQuart:
            return t * t * t * t;
            
        case Easing::EaseOutQuart: {
            float f = t - 1.0f;
            return 1.0f - f * f * f * f;
        }
            
        case Easing::EaseInOutQuart:
            return t < 0.5f 
                ? 8.0f * t * t * t * t 
                : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
            
        case Easing::EaseInExpo:
            return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
            
        case Easing::EaseOutExpo:
            return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
            
        case Easing::EaseInOutExpo:
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            return t < 0.5f
                ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
            
        case Easing::EaseOutBack: {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;
            float f = t - 1.0f;
            return 1.0f + c3 * f * f * f + c1 * f * f;
        }
            
        case Easing::EaseOutElastic: {
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            const float c4 = (2.0f * juce::MathConstants<float>::pi) / 3.0f;
            return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
        }
            
        case Easing::Spring:
            // For spring, use SpringAnimator instead
            // Fallback to EaseOutBack for simple cases
            return applyEasing(t, Easing::EaseOutBack);
            
        default:
            return t;
    }
}

// ============================================================================
// SPRING PHYSICS
// ============================================================================

/**
 * Configuration for spring-based animations.
 * Based on the critically damped spring model for smooth, natural motion.
 */
struct SpringConfig {
    float stiffness = 300.0f;  // Spring constant (higher = faster)
    float damping = 20.0f;     // Damping coefficient (higher = less oscillation)
    float mass = 1.0f;         // Mass (higher = more inertia)
    
    // Presets
    static SpringConfig gentle() { return { 150.0f, 15.0f, 1.0f }; }
    static SpringConfig responsive() { return { 400.0f, 25.0f, 1.0f }; }
    static SpringConfig snappy() { return { 600.0f, 35.0f, 1.0f }; }
    static SpringConfig bouncy() { return { 200.0f, 10.0f, 1.0f }; }
};

/**
 * Spring physics solver for a single value.
 * Uses the semi-implicit Euler method for stability.
 */
class SpringSolver {
public:
    SpringSolver(const SpringConfig& config = SpringConfig())
        : config_(config), position_(0.0f), velocity_(0.0f), target_(0.0f) {}
    
    void setTarget(float target) {
        target_ = target;
    }
    
    void setPosition(float position) {
        position_ = position;
    }
    
    void setVelocity(float velocity) {
        velocity_ = velocity;
    }
    
    float getPosition() const { return position_; }
    float getVelocity() const { return velocity_; }
    float getTarget() const { return target_; }
    
    /**
     * Update the spring simulation.
     * @param deltaMs Time step in milliseconds
     * @return true if still animating, false if settled
     */
    bool update(float deltaMs) {
        float dt = deltaMs / 1000.0f;  // Convert to seconds
        
        // Spring force: F = -k * (x - target)
        // Damping force: F = -c * v
        // Acceleration: a = F / m
        
        float displacement = position_ - target_;
        float springForce = -config_.stiffness * displacement;
        float dampingForce = -config_.damping * velocity_;
        float acceleration = (springForce + dampingForce) / config_.mass;
        
        // Semi-implicit Euler integration (more stable than explicit)
        velocity_ += acceleration * dt;
        position_ += velocity_ * dt;
        
        // Check if settled (within epsilon and low velocity)
        const float positionEpsilon = 0.001f;
        const float velocityEpsilon = 0.01f;
        
        if (std::abs(displacement) < positionEpsilon && 
            std::abs(velocity_) < velocityEpsilon) {
            position_ = target_;
            velocity_ = 0.0f;
            return false;  // Settled
        }
        
        return true;  // Still animating
    }
    
    bool isAnimating() const {
        const float positionEpsilon = 0.001f;
        const float velocityEpsilon = 0.01f;
        return std::abs(position_ - target_) > positionEpsilon || 
               std::abs(velocity_) > velocityEpsilon;
    }
    
    void setConfig(const SpringConfig& config) { config_ = config; }
    
private:
    SpringConfig config_;
    float position_;
    float velocity_;
    float target_;
};

// ============================================================================
// ANIMATED VALUE
// ============================================================================

/**
 * A value that smoothly transitions between states.
 * 
 * @tparam T Value type (must support arithmetic operations)
 */
template<typename T>
class AnimatedValue {
public:
    AnimatedValue(T initialValue = T())
        : startValue_(initialValue)
        , targetValue_(initialValue)
        , currentValue_(initialValue)
        , duration_(0)
        , elapsed_(0)
        , easing_(Easing::EaseOut)
        , isAnimating_(false)
        , useSpring_(false) {}
    
    /**
     * Set target value with animation.
     * @param target Target value to animate to
     * @param durationMs Animation duration in milliseconds
     * @param easing Easing curve to use
     */
    void setTarget(T target, int durationMs, Easing easing = Easing::EaseOut) {
        if (target == targetValue_ && !isAnimating_) {
            return;  // Already at target
        }
        
        startValue_ = currentValue_;
        targetValue_ = target;
        duration_ = static_cast<float>(durationMs);
        elapsed_ = 0.0f;
        easing_ = easing;
        isAnimating_ = true;
        useSpring_ = (easing == Easing::Spring);
        
        if (useSpring_) {
            springSolver_.setPosition(static_cast<float>(startValue_));
            springSolver_.setTarget(static_cast<float>(target));
        }
    }
    
    /**
     * Set target with spring physics.
     * @param target Target value
     * @param config Spring configuration
     */
    void setTargetSpring(T target, const SpringConfig& config = SpringConfig()) {
        startValue_ = currentValue_;
        targetValue_ = target;
        isAnimating_ = true;
        useSpring_ = true;
        springSolver_.setConfig(config);
        springSolver_.setPosition(static_cast<float>(currentValue_));
        springSolver_.setTarget(static_cast<float>(target));
    }
    
    /**
     * Set value immediately without animation.
     */
    void set(T value) {
        startValue_ = value;
        targetValue_ = value;
        currentValue_ = value;
        isAnimating_ = false;
        elapsed_ = 0.0f;
    }
    
    /**
     * Get current animated value.
     */
    T get() const { return currentValue_; }
    
    /**
     * Get target value.
     */
    T getTarget() const { return targetValue_; }
    
    /**
     * Check if currently animating.
     */
    bool isAnimating() const { return isAnimating_; }
    
    /**
     * Update animation state.
     * @param deltaMs Time elapsed since last update in milliseconds
     * @return true if value changed, false otherwise
     */
    bool update(float deltaMs) {
        if (!isAnimating_) {
            return false;
        }
        
        if (useSpring_) {
            isAnimating_ = springSolver_.update(deltaMs);
            T newValue = static_cast<T>(springSolver_.getPosition());
            bool changed = newValue != currentValue_;
            currentValue_ = newValue;
            
            if (!isAnimating_) {
                currentValue_ = targetValue_;
            }
            return changed;
        }
        
        // Time-based easing animation
        elapsed_ += deltaMs;
        
        if (elapsed_ >= duration_) {
            currentValue_ = targetValue_;
            isAnimating_ = false;
            return true;
        }
        
        float t = elapsed_ / duration_;
        float easedT = applyEasing(t, easing_);
        
        T newValue = lerp(startValue_, targetValue_, easedT);
        bool changed = newValue != currentValue_;
        currentValue_ = newValue;
        
        return changed;
    }
    
    /**
     * Get animation progress (0.0 to 1.0).
     */
    float getProgress() const {
        if (!isAnimating_ || duration_ <= 0.0f) {
            return 1.0f;
        }
        return juce::jlimit(0.0f, 1.0f, elapsed_ / duration_);
    }
    
    /**
     * Cancel animation and jump to target.
     */
    void finish() {
        currentValue_ = targetValue_;
        isAnimating_ = false;
    }
    
    /**
     * Cancel animation and stay at current value.
     */
    void cancel() {
        isAnimating_ = false;
    }
    
private:
    static T lerp(T a, T b, float t) {
        return static_cast<T>(a + (b - a) * t);
    }
    
    T startValue_;
    T targetValue_;
    T currentValue_;
    float duration_;
    float elapsed_;
    Easing easing_;
    bool isAnimating_;
    bool useSpring_;
    SpringSolver springSolver_;
};

// ============================================================================
// ANIMATED COLOR
// ============================================================================

/**
 * Specialization for animating colors.
 */
class AnimatedColor {
public:
    AnimatedColor(juce::Colour initialColor = juce::Colours::black)
        : r_(initialColor.getFloatRed())
        , g_(initialColor.getFloatGreen())
        , b_(initialColor.getFloatBlue())
        , a_(initialColor.getFloatAlpha()) {}
    
    void setTarget(juce::Colour target, int durationMs, Easing easing = Easing::EaseOut) {
        r_.setTarget(target.getFloatRed(), durationMs, easing);
        g_.setTarget(target.getFloatGreen(), durationMs, easing);
        b_.setTarget(target.getFloatBlue(), durationMs, easing);
        a_.setTarget(target.getFloatAlpha(), durationMs, easing);
    }
    
    void set(juce::Colour color) {
        r_.set(color.getFloatRed());
        g_.set(color.getFloatGreen());
        b_.set(color.getFloatBlue());
        a_.set(color.getFloatAlpha());
    }
    
    juce::Colour get() const {
        return juce::Colour::fromFloatRGBA(r_.get(), g_.get(), b_.get(), a_.get());
    }
    
    bool isAnimating() const {
        return r_.isAnimating() || g_.isAnimating() || 
               b_.isAnimating() || a_.isAnimating();
    }
    
    bool update(float deltaMs) {
        bool r = r_.update(deltaMs);
        bool g = g_.update(deltaMs);
        bool b = b_.update(deltaMs);
        bool a = a_.update(deltaMs);
        return r || g || b || a;
    }
    
private:
    AnimatedValue<float> r_, g_, b_, a_;
};

// ============================================================================
// ANIMATED RECTANGLE
// ============================================================================

/**
 * Specialization for animating rectangles (position and size).
 */
class AnimatedRect {
public:
    AnimatedRect() = default;
    
    AnimatedRect(juce::Rectangle<float> initial)
        : x_(initial.getX())
        , y_(initial.getY())
        , w_(initial.getWidth())
        , h_(initial.getHeight()) {}
    
    void setTarget(juce::Rectangle<float> target, int durationMs, 
                   Easing easing = Easing::EaseOut) {
        x_.setTarget(target.getX(), durationMs, easing);
        y_.setTarget(target.getY(), durationMs, easing);
        w_.setTarget(target.getWidth(), durationMs, easing);
        h_.setTarget(target.getHeight(), durationMs, easing);
    }
    
    void set(juce::Rectangle<float> rect) {
        x_.set(rect.getX());
        y_.set(rect.getY());
        w_.set(rect.getWidth());
        h_.set(rect.getHeight());
    }
    
    juce::Rectangle<float> get() const {
        return { x_.get(), y_.get(), w_.get(), h_.get() };
    }
    
    bool isAnimating() const {
        return x_.isAnimating() || y_.isAnimating() || 
               w_.isAnimating() || h_.isAnimating();
    }
    
    bool update(float deltaMs) {
        bool x = x_.update(deltaMs);
        bool y = y_.update(deltaMs);
        bool w = w_.update(deltaMs);
        bool h = h_.update(deltaMs);
        return x || y || w || h;
    }
    
private:
    AnimatedValue<float> x_, y_, w_, h_;
};

} // namespace animation
} // namespace zenith
