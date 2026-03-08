/*
  ==============================================================================

    AnimationCoordinator.h
    Created: 2025-12-31
    Author:  Zenith DAW

    Central animation coordinator singleton that eliminates the "timer holocaust"
    pattern where every component runs its own independent timer.

    Instead of 50+ individual timers, ONE master clock distributes ticks to
    registered listeners based on priority levels.

    Usage:
        // In constructor:
        AnimationCoordinator::getInstance().registerListener(this, Priority::High);
        
        // Implement AnimationListener:
        void onAnimationTick(float deltaMs) override {
            // Update animations
        }
        
        // In destructor:
        AnimationCoordinator::getInstance().unregisterListener(this);

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <mutex>
#include <atomic>

namespace zenith {
namespace animation {

// ============================================================================
// ANIMATION LISTENER INTERFACE
// ============================================================================

/**
 * Interface for components that need animation updates.
 * Implement this instead of using juce::Timer directly.
 */
class AnimationListener {
public:
    virtual ~AnimationListener() = default;
    
    /**
     * Called on animation tick from the coordinator.
     * @param deltaMs Approximate time since last tick in milliseconds
     */
    virtual void onAnimationTick(float deltaMs) = 0;
    
    /**
     * Return true if this listener is currently animating.
     * When false, the coordinator may skip updates to save CPU.
     * Default returns true (always update).
     */
    virtual bool isAnimating() const { return true; }
    
    /**
     * Return true if this listener should only be updated when visible.
     * Coordinator will check Component::isVisible() if this returns true.
     * Default returns true (visibility-aware updates).
     */
    virtual bool requiresVisibility() const { return true; }
};

// ============================================================================
// PRIORITY LEVELS
// ============================================================================

/**
 * Animation priority levels determine update frequency.
 * 
 * At 60Hz master clock:
 * - Critical: Every tick (60 fps) - Use sparingly! (playhead, real-time meters)
 * - High:     Every tick (60 fps) - Active animations, visible UI
 * - Medium:   Every 2nd tick (30 fps) - Secondary animations
 * - Low:      Every 6th tick (10 fps) - Background updates, rarely-changing UI
 * - Idle:     Updates suspended until reactivated
 */
enum class Priority {
    Critical,  // 60 fps - always updated, even if not visible (audio meters)
    High,      // 60 fps - standard animations
    Normal = High,  // Alias for High - standard animations
    Medium,    // 30 fps - secondary animations
    Low,       // 10 fps - slow updates
    Idle       // Suspended
};

// ============================================================================
// ANIMATION COORDINATOR SINGLETON
// ============================================================================

/**
 * Central animation coordinator that replaces individual component timers.
 * 
 * Benefits:
 * - Single timer instead of 50+ (massive CPU savings)
 * - Priority-based update frequency
 * - Automatic visibility culling
 * - Idle detection and suspension
 * 
 * Thread Safety:
 * - All public methods are thread-safe
 * - onAnimationTick() is called on the message thread
 */
class AnimationCoordinator : private juce::Timer {
public:
    static AnimationCoordinator& getInstance();
    
    // ========================================================================
    // REGISTRATION
    // ========================================================================
    
    /**
     * Register a listener for animation updates.
     * @param listener The listener to register (must outlive registration)
     * @param priority Update frequency priority
     */
    void registerListener(AnimationListener* listener, Priority priority = Priority::High);
    
    /**
     * Unregister a listener.
     * Safe to call from destructor.
     */
    void unregisterListener(AnimationListener* listener);
    
    /**
     * Change priority of an already-registered listener.
     */
    void setPriority(AnimationListener* listener, Priority priority);
    
    // ========================================================================
    // CONTROL
    // ========================================================================
    
    /**
     * Force the coordinator to start if it was auto-suspended.
     * Usually not needed - listeners auto-activate it.
     */
    void wake();
    
    /**
     * Temporarily pause all animations.
     * Use for modal dialogs, heavy processing, etc.
     */
    void pause();
    void resume();
    bool isPaused() const { return paused_; }
    
    // ========================================================================
    // STATISTICS (for debugging/profiling)
    // ========================================================================
    
    struct Stats {
        int totalListeners = 0;
        int activeListeners = 0;
        int criticalCount = 0;
        int highCount = 0;
        int mediumCount = 0;
        int lowCount = 0;
        int ticksPerSecond = 0;
        double averageTickTimeUs = 0.0;
    };
    
    Stats getStats() const;
    
private:
    AnimationCoordinator();
    ~AnimationCoordinator() override;
    
    // juce::Timer override
    void timerCallback() override;
    
    // Internal structures
    struct ListenerEntry {
        AnimationListener* listener = nullptr;
        Priority priority = Priority::High;
        juce::Component* component = nullptr; // For visibility checks (may be null)
    };
    
    // Thread-safe listener management
    std::vector<ListenerEntry> listeners_;
    mutable std::mutex mutex_;
    
    // Tick counters for priority scheduling
    int tickCount_ = 0;
    
    // Timing
    double lastTickTimeMs_ = 0.0;
    std::atomic<bool> paused_{false};
    
    // Statistics
    mutable std::mutex statsMutex_;
    int statsTickCount_ = 0;
    double statsAccumTime_ = 0.0;
    double lastStatsResetTime_ = 0.0;
    Stats cachedStats_;
    
    // Auto-suspend when no active listeners
    int idleTickCount_ = 0;
    static constexpr int kIdleThreshold = 60; // 1 second of no activity
    
    // Singleton
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimationCoordinator)
};

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

/**
 * Register this component with the animation coordinator.
 * Use in constructor. Assumes 'this' implements AnimationListener.
 */
#define ZENITH_REGISTER_ANIMATION(priority) \
    zenith::animation::AnimationCoordinator::getInstance().registerListener(this, priority)

/**
 * Unregister from the animation coordinator.
 * Use in destructor.
 */
#define ZENITH_UNREGISTER_ANIMATION() \
    zenith::animation::AnimationCoordinator::getInstance().unregisterListener(this)

} // namespace animation
} // namespace zenith
