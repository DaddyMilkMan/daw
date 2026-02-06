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
    along with this program.  if not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <map>
#include <memory>

namespace zenith::ui {

//==============================================================================
// Gesture Data Structures
//==============================================================================

/**
 * @brief Touch point for multi-touch gestures
 */
struct TouchPoint {
    int touchId = 0;              // Unique touch identifier
    juce::Point<float> position{ 0.0f, 0.0f }; // Current position
    juce::Point<float> startPosition{ 0.0f, 0.0f }; // Initial position
    juce::Point<float> previousPosition{ 0.0f, 0.0f }; // Previous position
    float pressure = 1.0f;           // Touch pressure (0-1)
    juce::uint32 timestamp;     // Touch timestamp
    bool isActive = false;           // True if touch is active
};

/**
 * @brief Gesture type enumeration
 */
enum class GestureType {
    None,
    Tap,                    // Single touch tap
    DoubleTap,              // Double tap
    LongPress,              // Long press touch
    Swipe,                  // Single finger swipe
    SwipeMulti,            // Multi-finger swipe
    Pinch,                 // Pinch to zoom
    Rotate,                // Rotate with two fingers
    Pan,                   // Pan/scroll
    LongPressPan,         // Long press + pan
    TwoFingerTap,         // Two finger tap
    ThreeFingerTap,       // Three finger tap
    FourFingerTap,        // Four finger tap
    Shake,                // Device shake
    Custom                // Custom gesture
};

/**
 * @brief Gesture information
 */
struct GestureInfo {
    GestureType type = GestureType::None;
    std::vector<TouchPoint> touchPoints;
    juce::Point<float> center{ 0.0f, 0.0f };
    juce::Point<float> velocity{ 0.0f, 0.0f };
    float scale = 1.0f;             // For pinch gestures
    float rotation = 0.0f;         // For rotation gestures
    float pressure = 0.0f;         // Average pressure
    juce::uint32 startTime;
    juce::uint32 endTime;
    bool isActive = false;
    bool isCompleted = false;

    // Gesture-specific data
    struct SwipeData {
        juce::Point<float> direction{ 0.0f, 0.0f };
        float distance = 0.0f;
        float speed = 0.0f;
    } swipe{};

    struct PinchData {
        float startDistance = 0.0f;
        float currentDistance = 0.0f;
        float scaleFactor = 1.0f;
        juce::Point<float> center{ 0.0f, 0.0f };
    } pinch{};

    struct RotateData {
        float startAngle = 0.0f;
        float currentAngle = 0.0f;
        float rotationDelta = 0.0f;
        juce::Point<float> center{ 0.0f, 0.0f };
    } rotate{};
};

/**
 * @brief Gesture recognition configuration
 */
struct GestureConfig {
    // Tap detection
    float tapDistanceThreshold = 10.0f;      // Max distance for tap
    juce::uint64 tapTimeoutMs = 300;         // Max time between taps
    juce::uint64 doubleTapIntervalMs = 500;   // Time between double taps

    // Long press detection
    juce::uint64 longPressThresholdMs = 500;  // Time for long press
    float longPressDistanceThreshold = 20.0f; // Max distance during long press

    // Swipe detection
    float swipeMinDistance = 50.0f;           // Min distance for swipe
    float swipeVelocityThreshold = 300.0f;    // Min velocity for swipe
    float swipeMaxAngleDeviation = 30.0f;     // Max angle deviation

    // Pinch detection
    float pinchMinDistance = 30.0f;          // Min distance for pinch
    float pinchSensitivity = 1.0f;            // Pinch sensitivity multiplier

    // Rotation detection
    float rotationMinDistance = 50.0f;        // Min distance for rotation
    float rotationSensitivity = 1.0f;         // Rotation sensitivity multiplier

    // Shake detection
    float shakeThreshold = 100.0f;           // Acceleration threshold for shake
    juce::uint64 shakeTimeoutMs = 1000;      // Timeout for shake gesture

    // General settings
    bool enableMultiTouch = true;             // Enable multi-touch gestures
    bool enableHapticFeedback = false;         // Enable haptic feedback
    float smoothingFactor = 0.8f;            // Smoothing for position tracking
    int maxTouchPoints = 10;                  // Maximum concurrent touch points
};

/**
 * @brief Gesture state for tracking ongoing gestures
 */
struct GestureState {
    GestureType type = GestureType::None;
    std::vector<TouchPoint> touchPoints;
    juce::Point<float> startPoint{ 0.0f, 0.0f };
    juce::Point<float> currentPoint{ 0.0f, 0.0f };
    juce::uint32 startTime;
    juce::Point<float> initialCenter{ 0.0f, 0.0f };
    float initialDistance = 0.0f; // For pinch/rotate
    float initialAngle = 0.0f;   // For rotate
    bool isRecognized = false;

    // Prediction data
    struct Prediction {
        juce::Point<float> predictedPosition{ 0.0f, 0.0f };
        float confidence = 0.0f;
        juce::uint32 predictionTime;
    } prediction{};
};

//==============================================================================
// Advanced Gesture System Class
//==============================================================================

/**
 * @brief Advanced multi-touch gesture recognition system
 */
class AdvancedGestureSystem : public juce::Component,
                             public juce::MouseListener,
                             public juce::Timer {
public:
    //==========================================================================
    // Construction
    //==========================================================================

    AdvancedGestureSystem();
    ~AdvancedGestureSystem() override;

    //==========================================================================
    // Configuration
    //==========================================================================

    /**
     * @brief Set gesture configuration
     * @param config Configuration parameters
     */
    void setGestureConfig(const GestureConfig& config);

    /**
     * @brief Get current configuration
     */
    const GestureConfig& getConfig() const { return config_; }

    /**
     * @brief Enable/disable specific gesture types
     * @param gestureType Gesture type to enable/disable
     * @param enabled True to enable
     */
    void setGestureEnabled(GestureType gestureType, bool enabled);

    /**
     * @brief Check if gesture is enabled
     */
    bool isGestureEnabled(GestureType gestureType) const;

    //==========================================================================
    // Touch Input
    //==========================================================================

    /**
     * @brief Handle touch down
     * @param position Touch position
     * @param touchId Touch identifier
     * @param pressure Touch pressure
     */
    void touchDown(const juce::Point<float>& position, int touchId, float pressure = 1.0f);

    /**
     * @brief Handle touch move
     * @param position Touch position
     * @param touchId Touch identifier
     * @param pressure Touch pressure
     */
    void touchMoved(const juce::Point<float>& position, int touchId, float pressure = 1.0f);

    /**
     * @brief Handle touch up
     * @param position Touch position
     * @param touchId Touch identifier
     */
    void touchUp(const juce::Point<float>& position, int touchId);

    /**
     * @brief Handle touch cancelled
     * @param touchId Touch identifier
     */
    void touchCancelled(int touchId);

    //==========================================================================
    // Gesture Recognition
    //==========================================================================

    /**
     * @brief Get current active gesture
     */
    const GestureInfo& getActiveGesture() const { return currentGesture_; }

    /**
     * @brief Get all recognized gestures (history)
     */
    const std::vector<GestureInfo>& getGestureHistory() const { return gestureHistory_; }

    /**
     * @brief Clear gesture history
     */
    void clearGestureHistory();

    /**
     * @brief Predict gesture end position
     * @param gesture Gesture to predict
     * @return Predicted position with confidence
     */
    GestureState::Prediction predictGestureEnd(const GestureInfo& gesture);

    //==========================================================================
    // Callbacks
    //==========================================================================

    /**
     * @brief Callback for gesture start
     */
    std::function<void(const GestureInfo&)> onGestureStart;

    /**
     * @brief Callback for gesture update
     */
    std::function<void(const GestureInfo&)> onGestureUpdate;

    /**
     * @brief Callback for gesture end
     */
    std::function<void(const GestureInfo&)> onGestureEnd;

    /**
     * @brief Callback for gesture prediction
     */
    std::function<void(const GestureState::Prediction&)> onGesturePrediction;

    //==========================================================================
    // Mouse Input Compatibility
    //==========================================================================

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

    //==========================================================================
    // Component Override
    //==========================================================================

    bool keyPressed(const juce::KeyPress& key) override;
    void paint(juce::Graphics& g) override;

private:
    //==========================================================================
    // Touch Management
    //==========================================================================

    /**
     * @brief Add touch point
     * @param position Touch position
     * @param touchId Touch identifier
     * @param pressure Touch pressure
     */
    void addTouchPoint(const juce::Point<float>& position, int touchId, float pressure);

    /**
     * @brief Update touch point
     * @param position Touch position
     * @param touchId Touch identifier
     * @param pressure Touch pressure
     */
    void updateTouchPoint(const juce::Point<float>& position, int touchId, float pressure);

    /**
     * @brief Remove touch point
     * @param touchId Touch identifier
     */
    void removeTouchPoint(int touchId);

    /**
     * @brief Get touch point
     * @param touchId Touch identifier
     * @return Touch point pointer or nullptr
     */
    TouchPoint* getTouchPoint(int touchId);

    //==========================================================================
    // Gesture Recognition Methods
    /**
     * @brief Process gesture updates
     */
    void processGestureUpdate();

    /**
     * @brief Recognize gesture from touch points
     * @param touchPoints Current touch points
     * @return Recognized gesture info
     */
    GestureInfo recognizeGesture(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Check for tap gesture
     */
    GestureInfo recognizeTap(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Check for double tap gesture
     */
    GestureInfo recognizeDoubleTap(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Check for long press gesture
     */
    GestureInfo recognizeLongPress(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Check for swipe gesture
     */
    GestureInfo recognizeSwipe(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Check for pinch gesture
     */
    GestureInfo recognizePinch(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Check for rotation gesture
     */
    GestureInfo recognizeRotate(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Check for pan gesture
     */
    GestureInfo recognizePan(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Check for shake gesture
     */
    GestureInfo recognizeShake(const std::vector<TouchPoint>& touchPoints);

    //==========================================================================
    // Gesture Processing
    //==========================================================================

    /**
     * @brief Start new gesture
     */
    void startGesture(const GestureInfo& gesture);

    /**
     * @brief Update current gesture
     */
    void updateGesture(const GestureInfo& gesture);

    /**
     * @brief End current gesture
     */
    void endGesture(const GestureInfo& gesture);

    /**
     * @brief Calculate gesture velocity
     * @param currentPosition Current touch position
     * @param previousPosition Previous touch position
     * @param currentTime Current time for velocity calculation
     * @return Velocity as point (pixels per second)
     */
    juce::Point<float> calculateVelocity(const juce::Point<float>& currentPosition,
                                        const juce::Point<float>& previousPosition,
                                        juce::uint32 currentTime);

    /**
     * @brief Calculate gesture center
     * @param touchPoints Active touch points
     * @return Center point of all touches
     */
    juce::Point<float> calculateGestureCenter(const std::vector<TouchPoint>& touchPoints);

    /**
     * @brief Calculate gesture distance
     * @param point1 First point
     * @param point2 Second point
     * @return Euclidean distance between points
     */
    float calculateGestureDistance(const juce::Point<float>& point1,
                                  const juce::Point<float>& point2);

    /**
     * @brief Calculate gesture angle
     * @param point1 First point
     * @param point2 Second point
     * @return Angle in degrees
     */
    float calculateGestureAngle(const juce::Point<float>& point1,
                               const juce::Point<float>& point2);

    /**
     * @brief Apply smoothing to touch points
     * @param currentPos Current touch position
     * @param previousPos Previous touch position
     * @return Smoothed position
     */
    juce::Point<float> smoothTouchPosition(const juce::Point<float>& currentPos,
                                         const juce::Point<float>& previousPos);

    /**
     * @brief Check if gesture matches expected type
     * @param gesture Gesture to check
     * @param expectedType Expected gesture type
     * @return True if gesture matches type
     */
    bool gestureMatchesType(const GestureInfo& gesture, GestureType expectedType);

    //==========================================================================
    // Prediction Methods
    //==========================================================================

    /**
     * @brief Update gesture prediction
     * @param gesture Current gesture to predict for
     */
    void updateGesturePrediction(const GestureInfo& gesture);

    /**
     * @brief Predict gesture path using curve fitting
     * @param gesture Current gesture
     * @return Vector of predicted path points
     */
    std::vector<juce::Point<float>> predictGesturePath(const GestureInfo& gesture);

    /**
     * @brief Calculate prediction confidence
     * @param gesture Current gesture
     * @return Confidence score from 0.0 to 1.0
     */
    float calculatePredictionConfidence(const GestureInfo& gesture);

    //==========================================================================
    // Timer Callback
    //==========================================================================

    void timerCallback() override;

    //==========================================================================
    // Member Variables
    //==========================================================================

    // Touch tracking
    std::map<int, TouchPoint> touchPoints_;
    int nextTouchId_ = 1;

    // Gesture state
    GestureInfo currentGesture_;
    std::vector<GestureInfo> gestureHistory_;
    GestureConfig config_;
    std::map<GestureType, bool> enabledGestures_;

    // Prediction
    GestureState::Prediction currentPrediction_;

    // Timing
    juce::uint64 lastGestureTime_;
    juce::Point<float> lastMousePosition_;
    juce::uint32 lastMouseTime_;

    // State tracking
    bool isLongPressActive_ = false;
    juce::Point<float> longPressStartPos_;
    juce::uint32 longPressStartTime_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedGestureSystem)
};

} // namespace zenith::ui