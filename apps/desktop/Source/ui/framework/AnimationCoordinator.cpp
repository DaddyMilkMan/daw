/*
  ==============================================================================

    AnimationCoordinator.cpp
    Created: 2025-12-31
    Author:  Zenith DAW

    Implementation of the central animation coordinator.

  ==============================================================================
*/

#include "AnimationCoordinator.h"
#include "../../engine/ZenithLogger.h"
#include <algorithm>
#include <chrono>

namespace zenith {
namespace animation {

// ============================================================================
// SINGLETON
// ============================================================================

AnimationCoordinator& AnimationCoordinator::getInstance() {
    static AnimationCoordinator instance;
    return instance;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AnimationCoordinator::AnimationCoordinator() {
    // Get current time for delta calculation
    lastTickTimeMs_ = juce::Time::getMillisecondCounterHiRes();
    lastStatsResetTime_ = lastTickTimeMs_;
    
    // Start the master timer at 60 Hz (16.67ms interval)
    // This is the ONLY timer for all UI animations!
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) {
        startTimerHz(60);
    }
    
    ZENITH_LOG_INFO("[AnimationCoordinator] Initialized - Single master timer at 60Hz");
}

AnimationCoordinator::~AnimationCoordinator() {
    stopTimer();
    ZENITH_LOG_INFO("[AnimationCoordinator] Shutdown - Master timer stopped");
}

// ============================================================================
// REGISTRATION
// ============================================================================

void AnimationCoordinator::registerListener(AnimationListener* listener, Priority priority) {
    if (listener == nullptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already registered
    for (auto& entry : listeners_) {
        if (entry.listener == listener) {
            entry.priority = priority;
            return;
        }
    }
    
    ListenerEntry entry;
    entry.listener = listener;
    entry.priority = priority;
    
    // Try to get Component pointer for visibility checks
    entry.component = dynamic_cast<juce::Component*>(listener);
    
    listeners_.push_back(entry);
    
    // Wake up if we were idle
    idleTickCount_ = 0;
    
    // Ensure timer is running
    if (!isTimerRunning() && juce::MessageManager::getInstanceWithoutCreating() != nullptr) {
        startTimerHz(60);
    }
}

void AnimationCoordinator::unregisterListener(AnimationListener* listener) {
    if (listener == nullptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    listeners_.erase(
        std::remove_if(listeners_.begin(), listeners_.end(),
            [listener](const ListenerEntry& entry) {
                return entry.listener == listener;
            }),
        listeners_.end()
    );
}

void AnimationCoordinator::setPriority(AnimationListener* listener, Priority priority) {
    if (listener == nullptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& entry : listeners_) {
        if (entry.listener == listener) {
            entry.priority = priority;
            return;
        }
    }
}

// ============================================================================
// CONTROL
// ============================================================================

void AnimationCoordinator::wake() {
    idleTickCount_ = 0;
    if (!isTimerRunning() && juce::MessageManager::getInstanceWithoutCreating() != nullptr) {
        startTimerHz(60);
    }
}

void AnimationCoordinator::pause() {
    paused_ = true;
}

void AnimationCoordinator::resume() {
    paused_ = false;
    idleTickCount_ = 0;
}

// ============================================================================
// TIMER CALLBACK - THE MASTER CLOCK
// ============================================================================

void AnimationCoordinator::timerCallback() {
    if (paused_) return;
    
    // Calculate delta time
    double currentTime = juce::Time::getMillisecondCounterHiRes();
    float deltaMs = static_cast<float>(currentTime - lastTickTimeMs_);
    lastTickTimeMs_ = currentTime;
    
    // Clamp delta to reasonable range (in case of system sleep, etc.)
    deltaMs = std::clamp(deltaMs, 0.0f, 100.0f);
    
    // Increment tick counter for priority scheduling
    tickCount_++;
    
    // Determine which priority levels run this tick
    bool runCritical = true;                          // Every tick
    bool runHigh = true;                              // Every tick
    bool runMedium = (tickCount_ % 2 == 0);           // Every 2nd tick (30 fps)
    bool runLow = (tickCount_ % 6 == 0);              // Every 6th tick (10 fps)
    
    // Track if any listener was updated
    int activeCount = 0;
    
    // Snapshot listeners to avoid holding lock during callbacks
    std::vector<ListenerEntry> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = listeners_;
    }
    
    // Performance timing
    auto tickStart = std::chrono::high_resolution_clock::now();
    
    // Dispatch to listeners based on priority
    for (const auto& entry : snapshot) {
        if (entry.listener == nullptr) continue;
        if (entry.priority == Priority::Idle) continue;
        
        // Check if this priority level runs this tick
        bool shouldRun = false;
        switch (entry.priority) {
            case Priority::Critical: shouldRun = runCritical; break;
            case Priority::High:     shouldRun = runHigh; break;
            case Priority::Medium:   shouldRun = runMedium; break;
            case Priority::Low:      shouldRun = runLow; break;
            default: break;
        }
        
        if (!shouldRun) continue;
        
        // Visibility check (skip invisible components unless Critical or explicitly opting out)
        if (entry.priority != Priority::Critical && 
            entry.listener->requiresVisibility() &&
            entry.component != nullptr && 
            !entry.component->isVisible()) {
            continue;
        }
        
        // Animation check (allow listeners to self-report if they're idle)
        if (!entry.listener->isAnimating()) {
            continue;
        }
        
        // Dispatch the tick
        try {
            entry.listener->onAnimationTick(deltaMs);
            activeCount++;
        } catch (const std::exception& e) {
            ZENITH_LOG_ERROR("[AnimationCoordinator] Exception in listener: " + juce::String(e.what()));
        }
    }
    
    // Performance timing end
    auto tickEnd = std::chrono::high_resolution_clock::now();
    double tickTimeUs = std::chrono::duration<double, std::micro>(tickEnd - tickStart).count();
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        statsTickCount_++;
        statsAccumTime_ += tickTimeUs;
        
        // Reset stats every second
        if (currentTime - lastStatsResetTime_ >= 1000.0) {
            cachedStats_.totalListeners = static_cast<int>(snapshot.size());
            cachedStats_.activeListeners = activeCount;
            cachedStats_.ticksPerSecond = statsTickCount_;
            cachedStats_.averageTickTimeUs = statsTickCount_ > 0 ? statsAccumTime_ / statsTickCount_ : 0.0;
            
            // Count by priority
            cachedStats_.criticalCount = 0;
            cachedStats_.highCount = 0;
            cachedStats_.mediumCount = 0;
            cachedStats_.lowCount = 0;
            for (const auto& entry : snapshot) {
                switch (entry.priority) {
                    case Priority::Critical: cachedStats_.criticalCount++; break;
                    case Priority::High:     cachedStats_.highCount++; break;
                    case Priority::Medium:   cachedStats_.mediumCount++; break;
                    case Priority::Low:      cachedStats_.lowCount++; break;
                    default: break;
                }
            }
            
            statsTickCount_ = 0;
            statsAccumTime_ = 0.0;
            lastStatsResetTime_ = currentTime;
        }
    }
    
    // Auto-suspend when idle
    if (activeCount == 0) {
        idleTickCount_++;
        if (idleTickCount_ > kIdleThreshold) {
            // Could stop timer here for extreme power saving, but most DAWs
            // keep animating (playhead, meters). Keep running but at reduced rate.
            // For now, we just track idle state for stats.
        }
    } else {
        idleTickCount_ = 0;
    }
}

// ============================================================================
// STATISTICS
// ============================================================================

AnimationCoordinator::Stats AnimationCoordinator::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return cachedStats_;
}

} // namespace animation
} // namespace zenith
