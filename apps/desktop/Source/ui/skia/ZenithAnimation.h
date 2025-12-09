/*
  ==============================================================================

    ZenithAnimation.h
    Created: 2025-12-08
    Author: UI Overhaul - Industry-Standard Animation System

    Professional spring physics and easing animation system.
    Based on research from:
    - Material Design motion specs
    - Apple's spring animation guidelines
    - Framer Motion spring implementation

    Key parameters for spring animations:
    - Mass: 1.0 (default) - affects momentum
    - Stiffness: 100-500 typical (higher = snappier)
    - Damping: 10-30 typical (higher = less bounce)
    
    Presets calibrated for DAW UI feedback:
    - Snappy: Quick response for buttons (stiffness: 400, damping: 30)
    - Smooth: Faders and knobs (stiffness: 200, damping: 25)
    - Bouncy: Fun microinteractions (stiffness: 300, damping: 15)

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <functional>
#include <map>
#include <memory>

namespace zenith {
namespace animation {

// ============================================================================
// SPRING PHYSICS - Physically accurate spring simulation
// ============================================================================

/**
 * High-quality spring animation based on damped harmonic oscillator.
 * Uses the equation: F = -kx - cv
 * Where k = stiffness, c = damping, x = displacement, v = velocity
 */
class Spring {
public:
    struct Config {
        float mass = 1.0f;
        float stiffness = 300.0f;  // k - spring constant
        float damping = 20.0f;     // c - friction coefficient
        
        // Presets based on research
        static Config snappy() { return {1.0f, 400.0f, 30.0f}; }
        static Config smooth() { return {1.0f, 200.0f, 25.0f}; }
        static Config bouncy() { return {1.0f, 300.0f, 15.0f}; }
        static Config gentle() { return {1.0f, 150.0f, 20.0f}; }
        static Config instant() { return {1.0f, 800.0f, 40.0f}; }
        
        // Calculate damping ratio (zeta)
        // zeta < 1: underdamped (oscillates)
        // zeta = 1: critically damped (fastest without oscillation)
        // zeta > 1: overdamped (slow, no oscillation)
        float dampingRatio() const {
            float criticalDamping = 2.0f * std::sqrt(stiffness * mass);
            return damping / criticalDamping;
        }
    };
    
    Spring(float initialValue = 0.0f, Config config = Config::smooth())
        : value_(initialValue)
        , target_(initialValue)
        , velocity_(0.0f)
        , config_(config)
        , isAnimating_(false) {}
    
    void setTarget(float target) {
        target_ = target;
        isAnimating_ = true;
    }
    
    void setConfig(const Config& config) {
        config_ = config;
    }
    
    void setValue(float value) {
        value_ = value;
        target_ = value;
        velocity_ = 0.0f;
        isAnimating_ = false;
    }
    
    void setVelocity(float velocity) {
        velocity_ = velocity;
        isAnimating_ = true;
    }
    
    /**
     * Update spring physics. Call every frame.
     * @param deltaSeconds Time since last update in seconds
     */
    void update(float deltaSeconds) {
        if (!isAnimating_) return;
        
        // Clamp delta to prevent instability
        deltaSeconds = std::min(deltaSeconds, 0.064f); // Max ~15fps minimum
        
        // Spring physics: F = -kx - cv
        float displacement = value_ - target_;
        float springForce = -config_.stiffness * displacement;
        float dampingForce = -config_.damping * velocity_;
        float acceleration = (springForce + dampingForce) / config_.mass;
        
        // Semi-implicit Euler integration (more stable than explicit)
        velocity_ += acceleration * deltaSeconds;
        value_ += velocity_ * deltaSeconds;
        
        // Check if animation is complete
        if (std::abs(displacement) < 0.0001f && std::abs(velocity_) < 0.0001f) {
            value_ = target_;
            velocity_ = 0.0f;
            isAnimating_ = false;
        }
    }
    
    float getValue() const { return value_; }
    float getTarget() const { return target_; }
    float getVelocity() const { return velocity_; }
    bool isAnimating() const { return isAnimating_; }
    
private:
    float value_;
    float target_;
    float velocity_;
    Config config_;
    bool isAnimating_;
};

// ============================================================================
// EASING CURVES - Bezier-based timing functions
// ============================================================================

/**
 * Cubic bezier easing curve.
 * Common presets match CSS timing functions.
 */
class BezierEasing {
public:
    // CSS standard easing curves
    static float easeInOut(float t) {
        return cubicBezier(t, 0.42f, 0.0f, 0.58f, 1.0f);
    }
    
    static float easeOut(float t) {
        return cubicBezier(t, 0.0f, 0.0f, 0.58f, 1.0f);
    }
    
    static float easeIn(float t) {
        return cubicBezier(t, 0.42f, 0.0f, 1.0f, 1.0f);
    }
    
    // Material Design curves
    static float materialStandard(float t) {
        return cubicBezier(t, 0.4f, 0.0f, 0.2f, 1.0f);
    }
    
    static float materialDecelerate(float t) {
        return cubicBezier(t, 0.0f, 0.0f, 0.2f, 1.0f);
    }
    
    static float materialAccelerate(float t) {
        return cubicBezier(t, 0.4f, 0.0f, 1.0f, 1.0f);
    }
    
    // Apple curves
    static float appleDefault(float t) {
        return cubicBezier(t, 0.25f, 0.1f, 0.25f, 1.0f);
    }
    
private:
    static float cubicBezier(float t, float x1, float y1, float x2, float y2) {
        // Newton-Raphson iteration to find t for x
        float guess = t;
        for (int i = 0; i < 8; ++i) {
            float currentX = bezierValue(guess, x1, x2);
            float slope = bezierSlope(guess, x1, x2);
            if (std::abs(slope) < 0.0001f) break;
            guess -= (currentX - t) / slope;
        }
        return bezierValue(guess, y1, y2);
    }
    
    static float bezierValue(float t, float p1, float p2) {
        float t2 = t * t;
        float t3 = t2 * t;
        return 3.0f * (1.0f - t) * (1.0f - t) * t * p1 +
               3.0f * (1.0f - t) * t2 * p2 +
               t3;
    }
    
    static float bezierSlope(float t, float p1, float p2) {
        return 3.0f * (1.0f - t) * (1.0f - t) * p1 +
               6.0f * (1.0f - t) * t * (p2 - p1) +
               3.0f * t * t * (1.0f - p2);
    }
};

// ============================================================================
// TWEEN - Simple time-based animation
// ============================================================================

class Tween {
public:
    using EasingFunc = std::function<float(float)>;
    
    Tween(float from = 0.0f, float to = 1.0f, float durationMs = 200.0f,
          EasingFunc easing = BezierEasing::easeOut)
        : from_(from)
        , to_(to)
        , durationMs_(durationMs)
        , elapsedMs_(0.0f)
        , easing_(easing)
        , isAnimating_(false) {}
    
    void start(float from, float to, float durationMs = 200.0f) {
        from_ = from;
        to_ = to;
        durationMs_ = durationMs;
        elapsedMs_ = 0.0f;
        isAnimating_ = true;
    }
    
    void update(float deltaMs) {
        if (!isAnimating_) return;
        
        elapsedMs_ += deltaMs;
        if (elapsedMs_ >= durationMs_) {
            elapsedMs_ = durationMs_;
            isAnimating_ = false;
        }
    }
    
    float getValue() const {
        float t = durationMs_ > 0 ? elapsedMs_ / durationMs_ : 1.0f;
        float easedT = easing_ ? easing_(t) : t;
        return from_ + (to_ - from_) * easedT;
    }
    
    bool isAnimating() const { return isAnimating_; }
    float getProgress() const { return durationMs_ > 0 ? elapsedMs_ / durationMs_ : 1.0f; }
    
private:
    float from_;
    float to_;
    float durationMs_;
    float elapsedMs_;
    EasingFunc easing_;
    bool isAnimating_;
};

// ============================================================================
// ANIMATED PROPERTY - High-level animation wrapper
// ============================================================================

class AnimatedProperty {
public:
    enum class Mode { Spring, Tween };
    
    AnimatedProperty(float initialValue = 0.0f)
        : spring_(initialValue, Spring::Config::smooth())
        , mode_(Mode::Spring) {}
    
    // Spring animation (preferred for interactive elements)
    void animateWithSpring(float target, Spring::Config config = Spring::Config::smooth()) {
        spring_.setConfig(config);
        spring_.setTarget(target);
        mode_ = Mode::Spring;
    }
    
    // Tween animation (for fixed-duration animations)
    void animateWithTween(float target, float durationMs = 200.0f,
                          Tween::EasingFunc easing = BezierEasing::easeOut) {
        tween_.start(spring_.getValue(), target, durationMs);
        mode_ = Mode::Tween;
    }
    
    // Set immediately (no animation)
    void set(float value) {
        spring_.setValue(value);
    }
    
    void update(float deltaMs) {
        if (mode_ == Mode::Spring) {
            spring_.update(deltaMs / 1000.0f);
        } else {
            tween_.update(deltaMs);
            if (!tween_.isAnimating()) {
                spring_.setValue(tween_.getValue());
            }
        }
    }
    
    float getValue() const {
        if (mode_ == Mode::Spring) {
            return spring_.getValue();
        } else {
            return tween_.getValue();
        }
    }
    
    bool isAnimating() const {
        return mode_ == Mode::Spring ? spring_.isAnimating() : tween_.isAnimating();
    }
    
private:
    Spring spring_;
    Tween tween_;
    Mode mode_;
};

// ============================================================================
// ANIMATION CONTROLLER - Manages multiple animated properties
// ============================================================================

class AnimationController {
public:
    AnimatedProperty& property(const juce::String& name) {
        auto it = properties_.find(name);
        if (it == properties_.end()) {
            properties_[name] = std::make_unique<AnimatedProperty>(0.0f);
            it = properties_.find(name);
        }
        return *it->second;
    }
    
    void update(float deltaMs) {
        for (auto& pair : properties_) {
            pair.second->update(deltaMs);
        }
    }
    
    bool isAnyAnimating() const {
        for (const auto& pair : properties_) {
            if (pair.second->isAnimating()) return true;
        }
        return false;
    }
    
    void stopAll() {
        for (auto& pair : properties_) {
            pair.second->set(pair.second->getValue());
        }
    }
    
private:
    std::map<juce::String, std::unique_ptr<AnimatedProperty>> properties_;
};

// ============================================================================
// COMMON ANIMATION DURATIONS (in ms) - Based on research
// ============================================================================

namespace duration {
    constexpr float INSTANT = 0.0f;
    constexpr float MICRO = 50.0f;      // Hover feedback
    constexpr float FAST = 100.0f;      // Button press
    constexpr float NORMAL = 200.0f;    // Standard transition
    constexpr float SLOW = 300.0f;      // Panel transitions
    constexpr float SLOWER = 500.0f;    // Complex animations
}

} // namespace animation
} // namespace zenith