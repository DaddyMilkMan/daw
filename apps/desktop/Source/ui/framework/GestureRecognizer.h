/*
  ==============================================================================

    GestureRecognizer.h
    Created: 2026-02-03
    Author:  Zenith DAW

    Advanced gesture recognition for fluid panel interactions.
    
    Features:
    - Edge swipe detection (left/right edge triggers)
    - Velocity tracking for momentum-based animations
    - Drag gesture with threshold and direction locking
    - Swipe actions (horizontal swipe to reveal actions)
    - Pinch/spread (future: zoom)
    
    Design Philosophy:
    - Gesture position directly controls UI element position during drag
    - Velocity at release determines final animation (fling vs. snap back)
    - Configurable thresholds for different interaction contexts

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>
#include <cmath>
#include <functional>

namespace zenith {
namespace gesture {

// ============================================================================
// VELOCITY TRACKER
// ============================================================================

/**
 * Tracks pointer velocity over time using a rolling window of samples.
 * Provides smoothed velocity estimates for momentum-based animations.
 */
class VelocityTracker {
public:
    struct Sample {
        juce::Point<float> position;
        double timestamp; // milliseconds
    };
    
    VelocityTracker(int maxSamples = 10, double maxAge = 100.0)
        : maxSamples_(maxSamples), maxAgeMs_(maxAge) {}
    
    void addSample(juce::Point<float> position, double timestampMs) {
        samples_.push_back({ position, timestampMs });
        
        // Prune old samples
        while (samples_.size() > (size_t)maxSamples_) {
            samples_.pop_front();
        }
        
        // Prune samples older than maxAge
        double cutoff = timestampMs - maxAgeMs_;
        while (!samples_.empty() && samples_.front().timestamp < cutoff) {
            samples_.pop_front();
        }
    }
    
    void reset() {
        samples_.clear();
    }
    
    /**
     * Calculate velocity in pixels per second.
     * Returns zero if insufficient samples.
     */
    juce::Point<float> getVelocity() const {
        if (samples_.size() < 2) {
            return { 0.0f, 0.0f };
        }
        
        // Use weighted linear regression for smoother estimate
        // Simple approach: just use first and last samples
        const auto& first = samples_.front();
        const auto& last = samples_.back();
        
        double dt = (last.timestamp - first.timestamp) / 1000.0; // seconds
        if (dt < 0.001) {
            return { 0.0f, 0.0f };
        }
        
        float vx = (last.position.x - first.position.x) / (float)dt;
        float vy = (last.position.y - first.position.y) / (float)dt;
        
        return { vx, vy };
    }
    
    float getVelocityX() const { return getVelocity().x; }
    float getVelocityY() const { return getVelocity().y; }
    
private:
    std::deque<Sample> samples_;
    int maxSamples_;
    double maxAgeMs_;
};

// ============================================================================
// EDGE SWIPE DETECTOR
// ============================================================================

/**
 * Detects swipe gestures starting from screen edges.
 * Used for sidebar reveal, sheet dismissal, etc.
 */
class EdgeSwipeDetector {
public:
    enum class Edge { Left, Right, Top, Bottom };
    
    struct Config {
        Edge edge = Edge::Left;
        float edgeWidth = 20.0f;       // Pixels from edge to trigger
        float minVelocity = 200.0f;    // Min px/sec to count as swipe
        float minDistance = 50.0f;     // Min drag distance
        float maxPerpendicular = 100.0f; // Max perpendicular movement
        Config() = default;
    };
    
    EdgeSwipeDetector() : config_() {}
    explicit EdgeSwipeDetector(const Config& config) : config_(config) {}
    
    /**
     * Called on mouse/touch down.
     * Returns true if the gesture started in the edge zone.
     */
    bool onDown(juce::Point<float> position, float viewWidth, float viewHeight) {
        reset();
        
        bool inEdge = false;
        switch (config_.edge) {
            case Edge::Left:
                inEdge = position.x < config_.edgeWidth;
                break;
            case Edge::Right:
                inEdge = position.x > viewWidth - config_.edgeWidth;
                break;
            case Edge::Top:
                inEdge = position.y < config_.edgeWidth;
                break;
            case Edge::Bottom:
                inEdge = position.y > viewHeight - config_.edgeWidth;
                break;
        }
        
        if (inEdge) {
            isTracking_ = true;
            startPosition_ = position;
            currentPosition_ = position;
            velocityTracker_.addSample(position, juce::Time::getMillisecondCounterHiRes());
        }
        
        return inEdge;
    }
    
    /**
     * Called on mouse/touch move.
     * Returns current drag progress (0.0 to 1.0+ based on distance).
     */
    float onMove(juce::Point<float> position, float maxDistance) {
        if (!isTracking_) return 0.0f;
        
        currentPosition_ = position;
        velocityTracker_.addSample(position, juce::Time::getMillisecondCounterHiRes());
        
        float distance = getMainAxisDistance();
        return juce::jlimit(0.0f, 1.5f, distance / maxDistance);
    }
    
    /**
     * Called on mouse/touch up.
     * Returns true if this qualifies as a completed swipe gesture.
     */
    bool onUp() {
        if (!isTracking_) return false;
        
        float velocity = getMainAxisVelocity();
        float distance = getMainAxisDistance();
        float perpendicular = getPerpendicularDistance();
        
        isTracking_ = false;
        
        // Check swipe criteria
        bool fastEnough = std::abs(velocity) >= config_.minVelocity;
        bool farEnough = std::abs(distance) >= config_.minDistance;
        bool straightEnough = std::abs(perpendicular) <= config_.maxPerpendicular;
        bool correctDirection = velocity > 0; // Positive = away from edge
        
        return (fastEnough || farEnough) && straightEnough && correctDirection;
    }
    
    void reset() {
        isTracking_ = false;
        velocityTracker_.reset();
    }
    
    bool isTracking() const { return isTracking_; }
    juce::Point<float> getStartPosition() const { return startPosition_; }
    juce::Point<float> getCurrentPosition() const { return currentPosition_; }
    float getVelocity() const { return getMainAxisVelocity(); }
    
private:
    float getMainAxisDistance() const {
        switch (config_.edge) {
            case Edge::Left:
            case Edge::Right:
                return currentPosition_.x - startPosition_.x;
            case Edge::Top:
            case Edge::Bottom:
                return currentPosition_.y - startPosition_.y;
        }
        return 0.0f;
    }
    
    float getPerpendicularDistance() const {
        switch (config_.edge) {
            case Edge::Left:
            case Edge::Right:
                return currentPosition_.y - startPosition_.y;
            case Edge::Top:
            case Edge::Bottom:
                return currentPosition_.x - startPosition_.x;
        }
        return 0.0f;
    }
    
    float getMainAxisVelocity() const {
        auto v = velocityTracker_.getVelocity();
        switch (config_.edge) {
            case Edge::Left:
            case Edge::Right:
                return v.x;
            case Edge::Top:
            case Edge::Bottom:
                return v.y;
        }
        return 0.0f;
    }
    
    Config config_;
    bool isTracking_ = false;
    juce::Point<float> startPosition_;
    juce::Point<float> currentPosition_;
    VelocityTracker velocityTracker_;
};

// ============================================================================
// SWIPE ACTION DETECTOR
// ============================================================================

/**
 * Detects horizontal swipe on list items to reveal actions.
 * Common pattern: swipe left to reveal delete, swipe right to reveal edit.
 */
class SwipeActionDetector {
public:
    enum class Direction { None, Left, Right };
    
    struct Config {
        float activationThreshold = 60.0f;  // Distance to reveal action
        float commitThreshold = 120.0f;     // Distance to auto-execute
        float velocityThreshold = 400.0f;   // Velocity to trigger even if not far
        float lockAngle = 30.0f;            // Max degrees from horizontal
        Config() = default;
    };
    
    struct State {
        Direction direction = Direction::None;
        float offset = 0.0f;          // Current horizontal offset
        float progress = 0.0f;        // 0.0 to 1.0 progress to activation
        bool isCommitted = false;     // Exceeded commit threshold
        bool isLocked = false;        // Direction is locked
    };
    
    SwipeActionDetector() : config_() {}
    explicit SwipeActionDetector(const Config& config) : config_(config) {}
    
    void onDown(juce::Point<float> position) {
        reset();
        startPosition_ = position;
        currentPosition_ = position;
        isActive_ = true;
        velocityTracker_.addSample(position, juce::Time::getMillisecondCounterHiRes());
    }
    
    State onMove(juce::Point<float> position) {
        if (!isActive_) return state_;
        
        currentPosition_ = position;
        velocityTracker_.addSample(position, juce::Time::getMillisecondCounterHiRes());
        
        float dx = position.x - startPosition_.x;
        float dy = position.y - startPosition_.y;
        
        // Check if we should lock direction
        if (!state_.isLocked && (std::abs(dx) > 10.0f || std::abs(dy) > 10.0f)) {
            float angle = std::abs(std::atan2(dy, dx)) * 180.0f / juce::MathConstants<float>::pi;
            
            // Lock to horizontal if angle is within threshold
            if (angle <= config_.lockAngle || angle >= (180.0f - config_.lockAngle)) {
                state_.isLocked = true;
                state_.direction = dx > 0 ? Direction::Right : Direction::Left;
            } else {
                // Vertical swipe - cancel
                isActive_ = false;
                return state_;
            }
        }
        
        if (state_.isLocked) {
            // Only allow movement in locked direction
            if ((state_.direction == Direction::Right && dx < 0) ||
                (state_.direction == Direction::Left && dx > 0)) {
                dx = 0;
            }
            
            state_.offset = dx;
            state_.progress = std::abs(dx) / config_.activationThreshold;
            state_.isCommitted = std::abs(dx) >= config_.commitThreshold;
        }
        
        return state_;
    }
    
    /**
     * Called on release. Returns final action direction if triggered.
     */
    Direction onUp() {
        if (!isActive_ || !state_.isLocked) {
            reset();
            return Direction::None;
        }
        
        float velocity = velocityTracker_.getVelocityX();
        bool velocityTriggered = std::abs(velocity) >= config_.velocityThreshold;
        bool committed = state_.isCommitted;
        
        Direction result = Direction::None;
        
        if (committed || velocityTriggered) {
            // Check velocity direction matches swipe direction
            if (state_.direction == Direction::Right && velocity > 0) {
                result = Direction::Right;
            } else if (state_.direction == Direction::Left && velocity < 0) {
                result = Direction::Left;
            }
        }
        
        reset();
        return result;
    }
    
    void reset() {
        isActive_ = false;
        state_ = State();
        velocityTracker_.reset();
    }
    
    State getState() const { return state_; }
    bool isActive() const { return isActive_; }
    
private:
    Config config_;
    bool isActive_ = false;
    juce::Point<float> startPosition_;
    juce::Point<float> currentPosition_;
    State state_;
    VelocityTracker velocityTracker_;
};

// ============================================================================
// DRAG REORDER DETECTOR
// ============================================================================

/**
 * Detects drag-to-reorder gestures on list items.
 * Provides visual feedback index for where item would be dropped.
 */
class DragReorderDetector {
public:
    struct Config {
        float holdDelay = 200.0f;        // Ms to hold before drag starts
        float itemHeight = 40.0f;        // Height of each item
        float dragThreshold = 10.0f;     // Movement to confirm drag
        float scrollEdgeSize = 60.0f;    // Edge zone for auto-scroll
        float scrollSpeed = 200.0f;      // Pixels per second when in scroll zone
        Config() = default;
    };
    
    struct State {
        bool isDragging = false;
        int sourceIndex = -1;
        int targetIndex = -1;
        float dragOffsetY = 0.0f;        // Visual offset from original position
        float scrollVelocity = 0.0f;     // Auto-scroll velocity
    };
    
    std::function<void(int fromIndex, int toIndex)> onReorder;
    
    DragReorderDetector() : config_() {}
    explicit DragReorderDetector(const Config& config) : config_(config) {}
    
    void onDown(juce::Point<float> position, int itemIndex, double timestamp) {
        reset();
        startPosition_ = position;
        currentPosition_ = position;
        state_.sourceIndex = itemIndex;
        downTimestamp_ = timestamp;
        isPending_ = true;
    }
    
    State onMove(juce::Point<float> position, float listTop, float listBottom, double timestamp) {
        currentPosition_ = position;
        
        if (isPending_) {
            // Check if hold delay passed
            if (timestamp - downTimestamp_ >= config_.holdDelay) {
                float dy = std::abs(position.y - startPosition_.y);
                if (dy >= config_.dragThreshold) {
                    state_.isDragging = true;
                    isPending_ = false;
                }
            }
        }
        
        if (state_.isDragging) {
            state_.dragOffsetY = position.y - startPosition_.y;
            
            // Calculate target index
            float itemCenter = startPosition_.y + state_.dragOffsetY;
            int newTarget = (int)((itemCenter - listTop) / config_.itemHeight);
            newTarget = juce::jmax(0, newTarget);
            state_.targetIndex = newTarget;
            
            // Auto-scroll near edges
            if (position.y < listTop + config_.scrollEdgeSize) {
                float t = 1.0f - (position.y - listTop) / config_.scrollEdgeSize;
                state_.scrollVelocity = -config_.scrollSpeed * t;
            } else if (position.y > listBottom - config_.scrollEdgeSize) {
                float t = 1.0f - (listBottom - position.y) / config_.scrollEdgeSize;
                state_.scrollVelocity = config_.scrollSpeed * t;
            } else {
                state_.scrollVelocity = 0.0f;
            }
        }
        
        return state_;
    }
    
    void onUp() {
        if (state_.isDragging && state_.sourceIndex != state_.targetIndex && 
            state_.targetIndex >= 0 && onReorder) {
            onReorder(state_.sourceIndex, state_.targetIndex);
        }
        reset();
    }
    
    void cancel() {
        reset();
    }
    
    void reset() {
        isPending_ = false;
        state_ = State();
    }
    
    State getState() const { return state_; }
    bool isDragging() const { return state_.isDragging; }
    
private:
    Config config_;
    State state_;
    bool isPending_ = false;
    juce::Point<float> startPosition_;
    juce::Point<float> currentPosition_;
    double downTimestamp_ = 0.0;
};

// ============================================================================
// PANEL DRAG CONTROLLER
// ============================================================================

/**
 * Controls panel position during drag gesture.
 * Handles the sidebar/sheet drag-to-dismiss pattern.
 */
class PanelDragController {
public:
    struct Config {
        float minPosition = 0.0f;        // Minimum (closed) position
        float maxPosition = 300.0f;      // Maximum (open) position
        float snapThreshold = 0.4f;      // Progress threshold to snap open
        float velocityThreshold = 300.0f; // Velocity to override position
        float rubberBandFactor = 0.3f;   // Resistance when past bounds
        Config() = default;
    };
    
    struct State {
        float position = 0.0f;           // Current position
        float progress = 0.0f;           // 0.0 = closed, 1.0 = open
        bool isDragging = false;
        bool willSnapOpen = false;       // Prediction of final state
    };
    
    PanelDragController() : config_() {}
    explicit PanelDragController(const Config& config) : config_(config) {}
    
    void setConfig(Config config) { config_ = config; }
    
    void beginDrag(float startPosition) {
        state_.isDragging = true;
        dragStartPosition_ = startPosition;
        state_.position = startPosition;
        velocityTracker_.reset();
    }
    
    State updateDrag(float currentPosition, double timestamp) {
        if (!state_.isDragging) return state_;
        
        velocityTracker_.addSample({ currentPosition, 0 }, timestamp);
        
        float range = config_.maxPosition - config_.minPosition;
        
        // Apply rubber-band effect at boundaries
        if (currentPosition < config_.minPosition) {
            float over = config_.minPosition - currentPosition;
            currentPosition = config_.minPosition - over * config_.rubberBandFactor;
        } else if (currentPosition > config_.maxPosition) {
            float over = currentPosition - config_.maxPosition;
            currentPosition = config_.maxPosition + over * config_.rubberBandFactor;
        }
        
        state_.position = currentPosition;
        state_.progress = (currentPosition - config_.minPosition) / range;
        state_.progress = juce::jlimit(0.0f, 1.0f, state_.progress);
        
        // Predict snap target
        float velocity = velocityTracker_.getVelocityX();
        if (std::abs(velocity) >= config_.velocityThreshold) {
            state_.willSnapOpen = velocity > 0;
        } else {
            state_.willSnapOpen = state_.progress >= config_.snapThreshold;
        }
        
        return state_;
    }
    
    /**
     * End drag and return whether to snap open (true) or closed (false).
     */
    bool endDrag() {
        bool result = state_.willSnapOpen;
        state_.isDragging = false;
        return result;
    }
    
    void reset() {
        state_ = State();
        velocityTracker_.reset();
    }
    
    State getState() const { return state_; }
    float getVelocity() const { return velocityTracker_.getVelocityX(); }
    
private:
    Config config_;
    State state_;
    float dragStartPosition_ = 0.0f;
    VelocityTracker velocityTracker_;
};

} // namespace gesture
} // namespace zenith
