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

#include "AdvancedGestureSystem.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

AdvancedGestureSystem::AdvancedGestureSystem() {
    // Enable all gestures by default
    for (int i = 0; i <= static_cast<int>(GestureType::Custom); ++i) {
        enabledGestures_[static_cast<GestureType>(i)] = true;
    }

    // Set up default configuration
    setGestureConfig(GestureConfig());

    // Start timer for gesture processing
    startTimerHz(60); // 60 FPS processing

    lastGestureTime_ = juce::Time::getMillisecondCounter();
}

AdvancedGestureSystem::~AdvancedGestureSystem() {
    stopTimer();
}

//==============================================================================
// Configuration
//==============================================================================

void AdvancedGestureSystem::setGestureConfig(const GestureConfig& config) {
    config_ = config;

    // Update enabled gestures based on configuration
    enabledGestures_[GestureType::Tap] = true; // Always enable basic gestures
    enabledGestures_[GestureType::Swipe] = config_.enableMultiTouch;
    enabledGestures_[GestureType::Pinch] = config_.enableMultiTouch;
    enabledGestures_[GestureType::Rotate] = config_.enableMultiTouch;
    enabledGestures_[GestureType::Pan] = config_.enableMultiTouch;
}

void AdvancedGestureSystem::setGestureEnabled(GestureType gestureType, bool enabled) {
    enabledGestures_[gestureType] = enabled;
}

bool AdvancedGestureSystem::isGestureEnabled(GestureType gestureType) const {
    auto it = enabledGestures_.find(gestureType);
    return it != enabledGestures_.end() && it->second;
}

//==============================================================================
// Touch Input
//==============================================================================

void AdvancedGestureSystem::touchDown(const juce::Point<float>& position, int touchId, float pressure) {
    if (touchPoints_.size() >= static_cast<size_t>(config_.maxTouchPoints)) {
        return; // Too many touch points
    }

    addTouchPoint(position, touchId, pressure);
    processGestureUpdate();
}

void AdvancedGestureSystem::touchMoved(const juce::Point<float>& position, int touchId, float pressure) {
    updateTouchPoint(position, touchId, pressure);
    processGestureUpdate();
}

void AdvancedGestureSystem::touchUp(const juce::Point<float>& position, int touchId) {
    removeTouchPoint(touchId);
    processGestureUpdate();
}

void AdvancedGestureSystem::touchCancelled(int touchId) {
    removeTouchPoint(touchId);
    processGestureUpdate();
}

//==============================================================================
// Touch Management
//==============================================================================

void AdvancedGestureSystem::addTouchPoint(const juce::Point<float>& position, int touchId, float pressure) {
    TouchPoint touch;
    touch.touchId = touchId;
    touch.position = position;
    touch.startPosition = position;
    touch.previousPosition = position;
    touch.pressure = pressure;
    touch.timestamp = juce::Time::getMillisecondCounter();
    touch.isActive = true;

    touchPoints_[touchId] = touch;

    // Check for long press start
    if (touchPoints_.size() == 1 && isGestureEnabled(GestureType::LongPress)) {
        longPressStartPos_ = position;
        longPressStartTime_ = juce::Time::getMillisecondCounter();
        isLongPressActive_ = true;
    }
}

void AdvancedGestureSystem::updateTouchPoint(const juce::Point<float>& position, int touchId, float pressure) {
    auto it = touchPoints_.find(touchId);
    if (it != touchPoints_.end()) {
        TouchPoint& touch = it->second;
        touch.previousPosition = touch.position;
        touch.position = smoothTouchPosition(position, touch.previousPosition);
        touch.pressure = pressure;
        touch.timestamp = juce::Time::getMillisecondCounter();
    }
}

void AdvancedGestureSystem::removeTouchPoint(int touchId) {
    touchPoints_.erase(touchId);

    // End long press if active
    if (isLongPressActive_ && touchPoints_.empty()) {
        isLongPressActive_ = false;
    }
}

TouchPoint* AdvancedGestureSystem::getTouchPoint(int touchId) {
    auto it = touchPoints_.find(touchId);
    return it != touchPoints_.end() ? &it->second : nullptr;
}

//==============================================================================
// Gesture Recognition
//==============================================================================

void AdvancedGestureSystem::processGestureUpdate() {
    if (touchPoints_.empty()) {
        endGesture(currentGesture_);
        return;
    }

    // Convert touch points to vector for recognition
    std::vector<TouchPoint> activeTouchPoints;
    for (const auto& pair : touchPoints_) {
        activeTouchPoints.push_back(pair.second);
    }

    // Recognize gesture
    GestureInfo recognizedGesture = recognizeGesture(activeTouchPoints);

    // Process recognized gesture
    if (recognizedGesture.type != GestureType::None) {
        if (currentGesture_.type == GestureType::None) {
            startGesture(recognizedGesture);
        } else {
            updateGesture(recognizedGesture);
        }
    } else {
        // End current gesture if no longer recognized
        if (currentGesture_.type != GestureType::None) {
            endGesture(currentGesture_);
        }
    }
}

GestureInfo AdvancedGestureSystem::recognizeGesture(const std::vector<TouchPoint>& touchPoints) {
    if (touchPoints.empty()) {
        return GestureInfo{ GestureType::None };
    }

    // Try to recognize specific gestures in order of priority
    if (isGestureEnabled(GestureType::Pinch) && touchPoints.size() == 2) {
        GestureInfo pinch = recognizePinch(touchPoints);
        if (pinch.type != GestureType::None) {
            return pinch;
        }
    }

    if (isGestureEnabled(GestureType::Rotate) && touchPoints.size() == 2) {
        GestureInfo rotate = recognizeRotate(touchPoints);
        if (rotate.type != GestureType::None) {
            return rotate;
        }
    }

    if (isGestureEnabled(GestureType::Swipe) && touchPoints.size() == 1) {
        GestureInfo swipe = recognizeSwipe(touchPoints);
        if (swipe.type != GestureType::None) {
            return swipe;
        }
    }

    if (isGestureEnabled(GestureType::Tap) && touchPoints.size() == 1) {
        GestureInfo tap = recognizeTap(touchPoints);
        if (tap.type != GestureType::None) {
            return tap;
        }
    }

    if (isGestureEnabled(GestureType::LongPress) && touchPoints.size() == 1) {
        GestureInfo longPress = recognizeLongPress(touchPoints);
        if (longPress.type != GestureType::None) {
            return longPress;
        }
    }

    if (isGestureEnabled(GestureType::Pan) && touchPoints.size() == 1) {
        GestureInfo pan = recognizePan(touchPoints);
        if (pan.type != GestureType::None) {
            return pan;
        }
    }

    // Default to pan gesture
    if (touchPoints.size() == 1) {
        GestureInfo pan;
        pan.type = GestureType::Pan;
        pan.touchPoints = touchPoints;
        pan.center = calculateGestureCenter(touchPoints);
        pan.velocity = calculateVelocity(touchPoints[0].position,
                                       touchPoints[0].previousPosition,
                                       juce::Time::getMillisecondCounter());
        pan.isActive = true;
        return pan;
    }

    return GestureInfo{ GestureType::None };
}

GestureInfo AdvancedGestureSystem::recognizeTap(const std::vector<TouchPoint>& touchPoints) {
    if (touchPoints.size() != 1) {
        return GestureInfo{ GestureType::None };
    }

    const TouchPoint& touch = touchPoints[0];
    float distance = calculateGestureDistance(touch.position, touch.startPosition);
    juce::uint64 timeDiff = juce::Time::getMillisecondCounter() - touch.timestamp;

    if (distance < config_.tapDistanceThreshold && timeDiff < config_.tapTimeoutMs) {
        GestureInfo tap;
        tap.type = GestureType::Tap;
        tap.touchPoints = touchPoints;
        tap.center = touch.position;
        tap.pressure = touch.pressure;
        tap.isActive = true;
        return tap;
    }

    return GestureInfo{ GestureType::None };
}

GestureInfo AdvancedGestureSystem::recognizeSwipe(const std::vector<TouchPoint>& touchPoints) {
    if (touchPoints.size() != 1) {
        return GestureInfo{ GestureType::None };
    }

    const TouchPoint& touch = touchPoints[0];
    float distance = calculateGestureDistance(touch.position, touch.startPosition);
    juce::Point<float> velocity = calculateVelocity(touch.position, touch.previousPosition,
                                                   juce::Time::getMillisecondCounter());

    if (distance > config_.swipeMinDistance &&
        velocity.getDistanceFromOrigin() > config_.swipeVelocityThreshold) {
        GestureInfo swipe;
        swipe.type = GestureType::Swipe;
        swipe.touchPoints = touchPoints;
        swipe.center = touch.position;
        swipe.velocity = velocity;
        
        // Manual normalization
        float length = velocity.getDistanceFromOrigin();
        if (length > 0) {
             swipe.swipe.direction = velocity / length;
        } else {
             swipe.swipe.direction = {0.0f, 0.0f};
        }
        
        swipe.swipe.distance = distance;
        swipe.swipe.speed = length;
        swipe.isActive = true;
        return swipe;
    }

    return GestureInfo{ GestureType::None };
}

GestureInfo AdvancedGestureSystem::recognizePinch(const std::vector<TouchPoint>& touchPoints) {
    if (touchPoints.size() != 2) {
        return GestureInfo{ GestureType::None };
    }

    float currentDistance = calculateGestureDistance(touchPoints[0].position, touchPoints[1].position);
    juce::Point<float> center = calculateGestureCenter(touchPoints);

    // Initialize current gesture data if needed
    if (currentGesture_.type == GestureType::None) {
        currentGesture_.type = GestureType::Pinch;
        currentGesture_.pinch.startDistance = currentDistance;
        currentGesture_.pinch.center = center;
    }

    GestureInfo pinch;
    pinch.type = GestureType::Pinch;
    pinch.touchPoints = touchPoints;
    pinch.center = center;
    pinch.pinch.startDistance = currentDistance;
    pinch.pinch.currentDistance = currentDistance;
    pinch.pinch.center = center;
    pinch.pinch.scaleFactor = currentDistance / currentGesture_.pinch.startDistance;

    return pinch;
}

GestureInfo AdvancedGestureSystem::recognizeRotate(const std::vector<TouchPoint>& touchPoints) {
    if (touchPoints.size() != 2) {
        return GestureInfo{ GestureType::None };
    }

    juce::Point<float> center = calculateGestureCenter(touchPoints);
    float currentAngle = calculateGestureAngle(touchPoints[0].position, touchPoints[1].position);

    // Initialize current gesture data if needed
    if (currentGesture_.type == GestureType::None) {
        currentGesture_.type = GestureType::Rotate;
        currentGesture_.rotate.startAngle = currentAngle;
        currentGesture_.rotate.center = center;
    }

    GestureInfo rotate;
    rotate.type = GestureType::Rotate;
    rotate.touchPoints = touchPoints;
    rotate.center = center;
    rotate.rotate.startAngle = currentAngle;
    rotate.rotate.currentAngle = currentAngle;
    rotate.rotate.rotationDelta = currentAngle - currentGesture_.rotate.startAngle;
    rotate.rotate.center = center;

    return rotate;
}

GestureInfo AdvancedGestureSystem::recognizeLongPress(const std::vector<TouchPoint>& touchPoints) {
    if (touchPoints.size() != 1 || !isLongPressActive_) {
        return GestureInfo{ GestureType::None };
    }

    const TouchPoint& touch = touchPoints[0];
    float distance = calculateGestureDistance(touch.position, longPressStartPos_);
    juce::uint64 timeDiff = juce::Time::getMillisecondCounter() - longPressStartTime_;

    if (distance < config_.longPressDistanceThreshold &&
        timeDiff > config_.longPressThresholdMs) {
        GestureInfo longPress;
        longPress.type = GestureType::LongPress;
        longPress.touchPoints = touchPoints;
        longPress.center = touch.position;
        longPress.pressure = touch.pressure;
        longPress.isActive = true;
        longPress.startTime = longPressStartTime_;
        return longPress;
    }

    return GestureInfo{ GestureType::None };
}

GestureInfo AdvancedGestureSystem::recognizePan(const std::vector<TouchPoint>& touchPoints) {
    if (touchPoints.size() != 1) {
        return GestureInfo{ GestureType::None };
    }

    GestureInfo pan;
    pan.type = GestureType::Pan;
    pan.touchPoints = touchPoints;
    pan.center = calculateGestureCenter(touchPoints);
    pan.velocity = calculateVelocity(touchPoints[0].position,
                                   touchPoints[0].previousPosition,
                                   juce::Time::getMillisecondCounter());
    pan.isActive = true;
    return pan;
}

//==============================================================================
// Gesture Processing
//==============================================================================

void AdvancedGestureSystem::startGesture(const GestureInfo& gesture) {
    currentGesture_ = gesture;
    currentGesture_.startTime = juce::Time::getMillisecondCounter();
    currentGesture_.isCompleted = false;

    // Initialize gesture-specific data
    if (gesture.type == GestureType::Pinch) {
        currentGesture_.pinch.scaleFactor = 1.0f;
    } else if (gesture.type == GestureType::Rotate) {
        currentGesture_.rotate.rotationDelta = 0.0f;
    }

    // Trigger callback
    if (onGestureStart) {
        onGestureStart(currentGesture_);
    }

    // Start gesture prediction
    updateGesturePrediction(currentGesture_);
}

void AdvancedGestureSystem::updateGesture(const GestureInfo& gesture) {
    currentGesture_ = gesture;
    currentGesture_.isActive = true;

    // Trigger callback
    if (onGestureUpdate) {
        onGestureUpdate(currentGesture_);
    }

    // Update prediction
    updateGesturePrediction(currentGesture_);
}

void AdvancedGestureSystem::endGesture(const GestureInfo& gesture) {
    if (currentGesture_.type != GestureType::None) {
        currentGesture_.endTime = juce::Time::getMillisecondCounter();
        currentGesture_.isCompleted = true;
        currentGesture_.isActive = false;

        // Add to history
        gestureHistory_.push_back(currentGesture_);
        if (gestureHistory_.size() > 50) {
            gestureHistory_.erase(gestureHistory_.begin());
        }

        // Trigger callback
        if (onGestureEnd) {
            onGestureEnd(currentGesture_);
        }

        // Reset current gesture
        currentGesture_ = GestureInfo{ GestureType::None };
    }
}

juce::Point<float> AdvancedGestureSystem::calculateVelocity(const juce::Point<float>& currentPosition,
                                                         const juce::Point<float>& previousPosition,
                                                         juce::uint32 currentTime) {
    if (currentTime == lastMouseTime_) {
        return juce::Point<float>(0.0f, 0.0f);
    }

    float timeDiff = static_cast<float>(currentTime - lastMouseTime_);
    if (timeDiff < 1.0f) {
        timeDiff = 1.0f;
    }

    float dx = currentPosition.x - previousPosition.x;
    float dy = currentPosition.y - previousPosition.y;

    return juce::Point<float>(dx / timeDiff * 1000.0f, dy / timeDiff * 1000.0f);
}

juce::Point<float> AdvancedGestureSystem::calculateGestureCenter(const std::vector<TouchPoint>& touchPoints) {
    if (touchPoints.empty()) {
        return juce::Point<float>(0.0f, 0.0f);
    }

    float sumX = 0.0f, sumY = 0.0f;
    for (const auto& touch : touchPoints) {
        sumX += touch.position.x;
        sumY += touch.position.y;
    }

    return juce::Point<float>(sumX / static_cast<float>(touchPoints.size()),
                             sumY / static_cast<float>(touchPoints.size()));
}

float AdvancedGestureSystem::calculateGestureDistance(const juce::Point<float>& point1,
                                                     const juce::Point<float>& point2) {
    float dx = point1.x - point2.x;
    float dy = point1.y - point2.y;
    return std::sqrt(dx * dx + dy * dy);
}

float AdvancedGestureSystem::calculateGestureAngle(const juce::Point<float>& point1,
                                                 const juce::Point<float>& point2) {
    float dx = point2.x - point1.x;
    float dy = point2.y - point1.y;
    return std::atan2(dy, dx) * 180.0f / juce::MathConstants<float>::pi;
}

juce::Point<float> AdvancedGestureSystem::smoothTouchPosition(const juce::Point<float>& currentPos,
                                                            const juce::Point<float>& previousPos) {
    float smoothX = previousPos.x * config_.smoothingFactor + currentPos.x * (1.0f - config_.smoothingFactor);
    float smoothY = previousPos.y * config_.smoothingFactor + currentPos.y * (1.0f - config_.smoothingFactor);
    return juce::Point<float>(smoothX, smoothY);
}

bool AdvancedGestureSystem::gestureMatchesType(const GestureInfo& gesture, GestureType expectedType) {
    return gesture.type == expectedType;
}

//==============================================================================
// Mouse Input Compatibility
//==============================================================================

void AdvancedGestureSystem::mouseDown(const juce::MouseEvent& event) {
    lastMousePosition_ = event.position;
    lastMouseTime_ = juce::Time::getMillisecondCounter();
    touchDown(event.position, 1, 1.0f); // Use touch ID 1 for mouse
}

void AdvancedGestureSystem::mouseDrag(const juce::MouseEvent& event) {
    touchMoved(event.position, 1, 1.0f);
}

void AdvancedGestureSystem::mouseUp(const juce::MouseEvent& event) {
    touchUp(event.position, 1);
}

void AdvancedGestureSystem::mouseMove(const juce::MouseEvent& event) {
    if (event.mouseWasDraggedSinceMouseDown()) {
        touchMoved(event.position, 1, 1.0f);
    }
}

bool AdvancedGestureSystem::keyPressed(const juce::KeyPress& key) {
    // Handle keyboard gestures
    if (key.getKeyCode() == juce::KeyPress::escapeKey) {
        // Cancel current gesture
        if (currentGesture_.type != GestureType::None) {
            endGesture(currentGesture_);
        }
        return true;
    }
    return false;
}

void AdvancedGestureSystem::paint(juce::Graphics& g) {
    // Draw debug visualization of touch points and gestures
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);

    // Draw touch points
    for (const auto& pair : touchPoints_) {
        const TouchPoint& touch = pair.second;
        g.drawEllipse(touch.position.x - 5, touch.position.y - 5, 10, 10, 1.0f);

        // Draw touch ID
        g.drawText(juce::String(touch.touchId),
                  touch.position.x + 10, touch.position.y - 10, 50, 20,
                  juce::Justification::left);
    }

    // Draw current gesture info
    if (currentGesture_.type != GestureType::None) {
        g.drawText(juce::String("Gesture: ") + juce::String(static_cast<int>(currentGesture_.type)),
                  10, 10, 200, 20, juce::Justification::left);

        g.drawText(juce::String("Center: ") + juce::String(currentGesture_.center.x) + ", " + juce::String(currentGesture_.center.y),
                  10, 35, 200, 20, juce::Justification::left);
    }

    // Draw prediction
    if (currentPrediction_.confidence > 0.5f) {
        g.setColour(juce::Colours::yellow);
        g.drawEllipse(currentPrediction_.predictedPosition.x - 8, currentPrediction_.predictedPosition.y - 8,
                     16, 16, 2.0f);

        g.setColour(juce::Colours::white);
        g.drawText(juce::String("Prediction: ") + juce::String(currentPrediction_.confidence * 100) + "%",
                  10, 60, 200, 20, juce::Justification::left);
    }
}

//==============================================================================
// Timer Callback
//==============================================================================

void AdvancedGestureSystem::timerCallback() {
    // Update long press detection
    if (isLongPressActive_ && touchPoints_.size() == 1) {
        const TouchPoint& touch = touchPoints_.begin()->second;
        float distance = calculateGestureDistance(touch.position, longPressStartPos_);
        juce::uint64 timeDiff = juce::Time::getMillisecondCounter() - longPressStartTime_;

        if (distance > config_.longPressDistanceThreshold || timeDiff > 2000) {
            isLongPressActive_ = false;
        }
    }

    // Update gesture prediction
    if (currentGesture_.type != GestureType::None) {
        updateGesturePrediction(currentGesture_);
    }

    // Clean up old gesture history
    juce::uint64 currentTime = juce::Time::getMillisecondCounter();
    auto it = gestureHistory_.begin();
    while (it != gestureHistory_.end()) {
        if (currentTime - it->endTime > 5000) {
            it = gestureHistory_.erase(it);
        } else {
            ++it;
        }
    }
}

//==============================================================================
// Gesture History Management
//==============================================================================

void AdvancedGestureSystem::clearGestureHistory() {
    gestureHistory_.clear();
}

//==============================================================================
// Gesture Recognition Methods (continued)
//==============================================================================

GestureInfo AdvancedGestureSystem::recognizeShake(const std::vector<TouchPoint>& touchPoints) {
    // Shake detection requires analyzing acceleration over time
    // This is a simplified version that checks for rapid back-and-forth movement

    if (touchPoints.empty() || touchPoints.size() > 2) {
        return GestureInfo{ GestureType::None };
    }

    const TouchPoint& touch = touchPoints[0];

    // Calculate distance from start
    float distanceFromStart = calculateGestureDistance(touch.position, touch.startPosition);

    // Calculate current velocity
    juce::Point<float> velocity = calculateVelocity(touch.position, touch.previousPosition,
                                                     juce::Time::getMillisecondCounter());
    float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

    // Check if movement is rapid and oscillating
    juce::uint64 timeDiff = juce::Time::getMillisecondCounter() - touch.timestamp;

    if (distanceFromStart < config_.swipeMinDistance &&
        speed > config_.swipeVelocityThreshold * 0.5f &&
        timeDiff < config_.shakeTimeoutMs) {

        // Check for direction change (oscillation)
        if (gestureHistory_.size() > 0) {
            const auto& lastGesture = gestureHistory_.back();
            if (lastGesture.type == GestureType::Swipe) {
                float len = velocity.getDistanceFromOrigin();
                juce::Point<float> normalizedVelocity = len > 0 ? velocity / len : juce::Point<float>(0,0);
                
                float dotProduct = normalizedVelocity.x * lastGesture.swipe.direction.x +
                                  normalizedVelocity.y * lastGesture.swipe.direction.y;

                // If direction is opposite, it might be a shake
                if (dotProduct < -0.5f) {
                    GestureInfo shake;
                    shake.type = GestureType::Shake;
                    shake.touchPoints = touchPoints;
                    shake.center = touch.position;
                    shake.velocity = velocity;
                    shake.isActive = true;
                    return shake;
                }
            }
        }
    }

    return GestureInfo{ GestureType::None };
}

//==============================================================================
// Prediction Methods
//==============================================================================

GestureState::Prediction AdvancedGestureSystem::predictGestureEnd(const GestureInfo& gesture) {
    GestureState::Prediction prediction;

    if (gesture.touchPoints.empty() || gesture.type == GestureType::None) {
        prediction.confidence = 0.0f;
        return prediction;
    }

    const TouchPoint& touch = gesture.touchPoints[0];

    // Calculate current velocity
    juce::Point<float> velocity = calculateVelocity(touch.position, touch.previousPosition,
                                                     juce::Time::getMillisecondCounter());

    // Predict position based on velocity and gesture type
    float predictionFactor = 0.5f; // Predict 0.5 seconds into the future

    switch (gesture.type) {
        case GestureType::Swipe:
            prediction.predictedPosition = touch.position + (velocity * predictionFactor);
            prediction.confidence = calculatePredictionConfidence(gesture);
            break;

        case GestureType::Pan:
            // Panning tends to continue in same direction but slows down
            prediction.predictedPosition = touch.position + (velocity * predictionFactor * 0.7f);
            prediction.confidence = 0.7f;
            break;

        case GestureType::Pinch:
            // Pinch prediction is based on center point
            prediction.predictedPosition = gesture.center;
            prediction.confidence = 0.8f;
            break;

        case GestureType::Rotate:
            // Rotation stays around center
            prediction.predictedPosition = gesture.center;
            prediction.confidence = 0.85f;
            break;

        default:
            // Default to current position
            prediction.predictedPosition = touch.position;
            prediction.confidence = 0.0f;
            break;
    }

    prediction.predictionTime = juce::Time::getMillisecondCounter();

    return prediction;
}

std::vector<juce::Point<float>> AdvancedGestureSystem::predictGesturePath(const GestureInfo& gesture) {
    std::vector<juce::Point<float>> path;

    if (gesture.touchPoints.empty() || gesture.type == GestureType::None) {
        return path;
    }

    const TouchPoint& touch = gesture.touchPoints[0];
    juce::Point<float> velocity = calculateVelocity(touch.position, touch.previousPosition,
                                                     juce::Time::getMillisecondCounter());

    // Generate predicted path points
    const int numPoints = 10;
    const float timeStep = 0.05f; // 50ms between points

    for (int i = 0; i < numPoints; ++i) {
        float time = (i + 1) * timeStep;

        // Apply simple deceleration model
        float deceleration = 1.0f - (time * 0.3f);
        deceleration = juce::jlimit(0.0f, 1.0f, deceleration);

        juce::Point<float> predictedPoint = touch.position + (velocity * time * deceleration);
        path.push_back(predictedPoint);
    }

    return path;
}

float AdvancedGestureSystem::calculatePredictionConfidence(const GestureInfo& gesture) {
    // Base confidence on gesture consistency and duration
    float confidence = 0.5f;

    // Longer gestures have higher confidence
    juce::uint64 duration = juce::Time::getMillisecondCounter() - gesture.startTime;
    confidence += juce::jmin(duration / 500.0f, 0.3f); // Max +0.3 for duration

    // Consistent velocity increases confidence
    if (gesture.touchPoints.size() >= 1) {
        const TouchPoint& touch = gesture.touchPoints[0];
        juce::Point<float> velocity = calculateVelocity(touch.position, touch.previousPosition,
                                                         juce::Time::getMillisecondCounter());
        float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

        if (speed > 100.0f) {
            confidence += 0.2f;
        }
    }

    // Certain gesture types have higher predictability
    switch (gesture.type) {
        case GestureType::Rotate:
        case GestureType::Pinch:
            confidence += 0.2f;
            break;
        case GestureType::Swipe:
            confidence += 0.1f;
            break;
        default:
            break;
    }

    return juce::jlimit(0.0f, 1.0f, confidence);
}

void AdvancedGestureSystem::updateGesturePrediction(const GestureInfo& gesture) {
    currentPrediction_ = predictGestureEnd(gesture);

    // Trigger prediction callback if confidence is high enough
    if (currentPrediction_.confidence > 0.6f && onGesturePrediction) {
        onGesturePrediction(currentPrediction_);
    }
}

} // namespace zenith::ui