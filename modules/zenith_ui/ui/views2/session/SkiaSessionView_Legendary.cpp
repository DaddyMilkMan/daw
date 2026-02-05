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

    ==============================================================================
    SkiaSessionView_Legendary.cpp
    Created: 2026-02-05
    Author:  Zenith DAW Team

    Legendary implementation of SkiaSessionView with Phase 2 improvements.
    ==============================================================================
*/

#include "SkiaSessionView_Legendary.h"
#include "../../design-system/ZenithTheme.h"
#include "../../framework/SkiaAccessibility.h"
#include <algorithm>
#include <chrono>

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

SkiaSessionView_Legendary::SkiaSessionView_Legendary() {
    // Initialize accessibility manager
    accessibilityManager_ = std::make_shared<SessionAccessibilityManager>();
    accessibilityManager_->setPreferences(AccessibilityPreferences::createWCAGAA());

    // Initialize performance monitor
    performanceMonitor_ = std::make_shared<PerformanceMonitor>();
    performanceMonitor_->setQualitySettings(QualitySettings::createUltraQuality());

    // Set up event handlers
    performanceMonitor_->onQualityChange = [this](QualityLevel oldLevel, QualityLevel newLevel) {
        handleQualityChange(oldLevel, newLevel);
    };

    performanceMonitor_->onPerformanceWarning = [this](const PerformanceMetrics& metrics) {
        handlePerformanceWarning(metrics);
    };

    // Set initial animation states
    for (const auto& track : tracks_) {
        for (size_t s = 0; s < track.clips.size(); ++s) {
            std::pair<int, int> slotKey(static_cast<int>(&track - &tracks_[0]), static_cast<int>(s));
            animationStates_[slotKey] = createAnimationStateFromClip(track.clips[s]);
        }
    }

    // Start animation timer
    startTimerHz(60);  // 60fps animation updates
}

SkiaSessionView_Legendary::~SkiaSessionView_Legendary() {
    stopTimer();
}

//==============================================================================
// Phase 2 Features Configuration
//==============================================================================

void SkiaSessionView_Legendary::setTransitionAnimationsEnabled(bool enabled) {
    transitionAnimationsEnabled_ = enabled;
    if (!enabled) {
        cancelAllAnimations();
    }
    markDirty();
}

void SkiaSessionView_Legendary::setAccessibilityEnabled(bool enabled) {
    accessibilityEnabled_ = enabled;
    if (accessibilityManager_) {
        accessibilityManager_->setPreferences(
            enabled ? AccessibilityPreferences::createWCAGAA() : AccessibilityPreferences()
        );
    }
    markDirty();
}

void SkiaSessionView_Legendary::setPerformanceMonitoringEnabled(bool enabled) {
    performanceMonitoringEnabled_ = enabled;
    if (performanceMonitor_) {
        performanceMonitor_->setMonitoringEnabled(enabled);
    }
    markDirty();
}

void SkiaSessionView_Legendary::setAccessibilityPreferences(const AccessibilityPreferences& preferences) {
    if (accessibilityManager_) {
        accessibilityManager_->setPreferences(preferences);
        if (onAccessibilityChange) {
            onAccessibilityChange(preferences);
        }
    }
}

void SkiaSessionView_Legendary::setPerformanceQualitySettings(const QualitySettings& settings) {
    if (performanceMonitor_) {
        performanceMonitor_->setQualitySettings(settings);
    }
}

void SkiaSessionView_Legendary::setTransitionEasing(zenith::animation::Easing easing) {
    defaultTransitionEasing_ = easing;
}

void SkiaSessionView_Legendary::setTransitionDuration(float durationMs) {
    defaultTransitionDuration_ = durationMs;
}

//==============================================================================
// Enhanced State Management
//==============================================================================

void SkiaSessionView_Legendary::setClipStateWithTransition(int trackIndex, int sceneIndex,
                                                          const ClipSlotData& data,
                                                          const TransitionConfig& config) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size()) ||
        sceneIndex < 0 || sceneIndex >= static_cast<int>(tracks_[trackIndex].clips.size())) {
        return;
    }

    std::pair<int, int> slotKey(trackIndex, sceneIndex);

    // Create or get transition for this slot
    if (!clipTransitions_[slotKey]) {
        clipTransitions_[slotKey] = std::make_unique<ClipStateTransition>();
    }

    // Create target animation state
    AnimationState targetState = createAnimationStateFromClip(data);

    // Start transition
    clipTransitions_[slotKey]->transitionTo(targetState, config, [this, trackIndex, sceneIndex]() {
        // Transition completion callback
        invalidateClipSlot(trackIndex, sceneIndex);
    });

    // Update animation state
    animationStates_[slotKey] = targetState;

    // Handle accessibility
    if (accessibilityEnabled_ && accessibilityManager_) {
        handleClipStateChangeForAccessibility(trackIndex, sceneIndex, data);
    }

    // Mark slot as dirty for repainting
    invalidateClipSlot(trackIndex, sceneIndex);
}

void SkiaSessionView_Legendary::cancelAllAnimations() {
    for (auto& [slotKey, transition] : clipTransitions_) {
        if (transition) {
            transition->cancelAnimation();
        }
    }
    cleanupCompletedAnimations();
}

void SkiaSessionView_Legendary::pauseAllAnimations() {
    for (auto& [slotKey, transition] : clipTransitions_) {
        if (transition) {
            transition->pauseAnimation();
        }
    }
}

void SkiaSessionView_Legendary::resumeAllAnimations() {
    for (auto& [slotKey, transition] : clipTransitions_) {
        if (transition) {
            transition->resumeAnimation();
        }
    }
}

void SkiaSessionView_Legendary::forceStateTransition(int trackIndex, int sceneIndex, const AnimationState& targetState) {
    std::pair<int, int> slotKey(trackIndex, sceneIndex);

    if (clipTransitions_[slotKey]) {
        clipTransitions_[slotKey]->cancelAnimation();
    }

    animationStates_[slotKey] = targetState;
    invalidateClipSlot(trackIndex, sceneIndex);
}

//==============================================================================
// Enhanced Accessibility
//==============================================================================

void SkiaSessionView_Legendary::focusClipSlot(int trackIndex, int sceneIndex) {
    if (accessibilityManager_) {
        accessibilityManager_->setFocusPosition(trackIndex, sceneIndex);
        invalidateClipSlot(trackIndex, sceneIndex);
    }
}

bool SkiaSessionView_Legendary::handleAccessibilityKeyPress(const juce::KeyPress& key) {
    if (accessibilityManager_ && accessibilityEnabled_) {
        return accessibilityManager_->handleAccessibilityKeyPress(key);
    }
    return false;
}

ClipSlotAccessibilityInfo SkiaSessionView_Legendary::getClipSlotAccessibility(int trackIndex, int sceneIndex) const {
    if (accessibilityManager_) {
        return accessibilityManager_->getClipSlotAccessibility(trackIndex, sceneIndex);
    }

    // Return default info
    ClipSlotAccessibilityInfo info;
    info.id = "clip_" + juce::String(trackIndex) + "_" + juce::String(sceneIndex);
    info.label = "Clip " + juce::String(sceneIndex + 1);
    info.role = "button";
    info.position = "Track " + juce::String(trackIndex + 1) + ", Scene " + juce::String(sceneIndex + 1);
    info.state = "unknown";
    return info;
}

juce::String SkiaSessionView_Legendary::getAccessibilityReport() const {
    if (accessibilityManager_) {
        return accessibilityManager_->getComplianceReport();
    }
    return "Accessibility not available";
}

//==============================================================================
// Enhanced Performance Monitoring
//==============================================================================

PerformanceMetrics SkiaSessionView_Legendary::getPerformanceMetrics() const {
    if (performanceMonitor_) {
        return performanceMonitor_->getCurrentMetrics();
    }
    return PerformanceMetrics();
}

juce::String SkiaSessionView_Legendary::getPerformanceReport() const {
    if (performanceMonitor_) {
        return performanceMonitor_->getPerformanceReport();
    }
    return "Performance monitoring not available";
}

bool SkiaSessionView_Legendary::isPerformanceAcceptable() const {
    if (performanceMonitor_) {
        return performanceMonitor_->isPerformanceAcceptable();
    }
    return true;
}

void SkiaSessionView_Legendary::setQualityLevel(QualityLevel level) {
    currentQualityLevel_ = level;
    if (performanceMonitor_) {
        performanceMonitor_->setQualityLevel(level);
    }
}

QualityLevel SkiaSessionView_Legendary::getCurrentQualityLevel() const {
    if (performanceMonitor_) {
        return performanceMonitor_->getCurrentQualityLevel();
    }
    return currentQualityLevel_;
}

//==============================================================================
// Enhanced Drawing
//==============================================================================

void SkiaSessionView_Legendary::drawSkia(SkCanvas* canvas) {
    // Start performance monitoring
    if (performanceMonitoringEnabled_) {
        performanceMonitor_->beginFrame();
    }

    // Call base class drawing
    SkiaSessionView::drawSkia(canvas);

    // Draw enhanced features
    for (size_t t = 0; t < tracks_.size(); ++t) {
        float x = getTrackX(static_cast<int>(t));
        if (x > getWidth() || x + kTrackWidth < kSceneLauncherWidth) continue;

        for (size_t s = 0; s < tracks_[t].clips.size() && s < scenes_.size(); ++s) {
            float y = getSceneY(static_cast<int>(s));
            if (y > getHeight() - kMixerHeight - kStopRowHeight ||
                y + kClipSlotHeight < kTrackHeaderHeight) continue;

            bool isHovered = (hoveredSlot_.first == static_cast<int>(t) &&
                              hoveredSlot_.second == static_cast<int>(s));
            bool isSelected = (selectedSlot_.first == static_cast<int>(t) &&
                               selectedSlot_.second == static_cast<int>(s));

            std::pair<int, int> slotKey(static_cast<int>(t), static_cast<int>(s));

            if (animationStates_.find(slotKey) != animationStates_.end()) {
                drawClipSlotEnhanced(canvas, x, y, kTrackWidth, kClipSlotHeight,
                                   tracks_[t].clips[s], isHovered, isSelected,
                                   animationStates_[slotKey]);
            } else {
                drawClipSlot(canvas, x, y, kTrackWidth, kClipSlotHeight,
                            tracks_[t].clips[s], isHovered, isSelected);
            }
        }
    }

    // Draw performance overlay if enabled
    if (performanceMonitoringEnabled_) {
        drawPerformanceOverlay(canvas);
    }

    // End performance monitoring
    if (performanceMonitoringEnabled_) {
        performanceMonitor_->endFrame();
    }
}

void SkiaSessionView_Legendary::resized() {
    SkiaSessionView::resized();
    markDirty();
}

void SkiaSessionView_Legendary::mouseDown(const juce::MouseEvent& e) {
    SkiaSessionView::mouseDown(e);

    // Handle accessibility focus
    if (accessibilityEnabled_ && accessibilityManager_) {
        std::pair<int, int> slot = hitTestSlot(e.getPosition().toFloat().getX(), e.getPosition().toFloat().getY());
        if (slot.first >= 0 && slot.second >= 0) {
            focusClipSlot(slot.first, slot.second);
        }
    }
}

void SkiaSessionView_Legendary::mouseDoubleClick(const juce::MouseEvent& e) {
    SkiaSessionView::mouseDoubleClick(e);
}

void SkiaSessionView_Legendary::mouseWheelMove(const juce::MouseEvent& e,
                                               const juce::MouseWheelDetails& wheel) {
    SkiaSessionView::mouseWheelMove(e, wheel);
}

void SkiaSessionView_Legendary::onAnimationTick(float deltaMs) {
    // Update animations
    if (transitionAnimationsEnabled_) {
        updateAnimations(deltaMs);
    }

    // Update performance monitoring
    if (performanceMonitoringEnabled_ && performanceMonitor_) {
        performanceMonitor_->trackDirtyRects(getDirtyRectCount());
        performanceMonitor_->trackAnimatedClips(getAnimatingClipCount());
    }

    // Call base class
    SkiaSessionView::onAnimationTick(deltaMs);
}

void SkiaSessionView_Legendary::drawClipSlotEnhanced(SkCanvas* canvas, float x, float y,
                                                   float w, float h, const ClipSlotData& data,
                                                   bool isHovered, bool isSelected,
                                                   const AnimationState& animationState) {
    // Get quality adjustments
    VisualQualityAdjustments quality = getQualityAdjustments();

    // Draw base slot
    SkPaint bgPaint;
    if (accessibilityEnabled_ && accessibilityManager_) {
        bgPaint.setColor(accessibilityManager_->getHighContrastClipSlotColor(data).getARGB());
    } else {
        bgPaint.setColor(data.color.getARGB());
    }
    canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), bgPaint);

    // Apply animation transformations
    canvas->save();

    // Apply scale transformation
    if (animationState.scale != 1.0f) {
        float centerX = x + w / 2;
        float centerY = y + h / 2;
        canvas->translate(centerX, centerY);
        canvas->scale(animationState.scale, animationState.scale);
        canvas->translate(-centerX, -centerY);
    }

    // Apply offset
    if (animationState.offset.x != 0.0f || animationState.offset.y != 0.0f) {
        canvas->translate(animationState.offset.x, animationState.offset.y);
    }

    // Draw enhanced clip indicators
    drawEnhancedClipIndicators(canvas, x, y, w, h, data, animationState);

    // Draw focus indicator for accessibility
    if (isSelected && accessibilityEnabled_ && accessibilityManager_) {
        accessibilityManager_->drawFocusIndicator(canvas, SkRect::MakeXYWH(x, y, w, h), 8.0f);
    }

    canvas->restore();

    // Draw selection indicator
    if (isSelected) {
        SkPaint selectionPaint;
        selectionPaint.setColor(zenith::ui::ZenithTheme::Colors::accent_primary.getARGB());
        selectionPaint.setStyle(SkPaint::kStroke_Style);
        selectionPaint.setStrokeWidth(2.0f);
        canvas->drawRect(SkRect::MakeXYWH(x + 1, y + 1, w - 2, h - 2), selectionPaint);
    }

    // Draw hover effect
    if (isHovered && !isSelected) {
        SkPaint hoverPaint;
        hoverPaint.setColor(zenith::ui::ZenithTheme::Colors::hover_overlay.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), hoverPaint);
    }
}

void SkiaSessionView_Legendary::drawPerformanceOverlay(SkCanvas* canvas) {
    if (!performanceMonitor_) {
        return;
    }

    // Draw performance overlay in top-right corner
    float x = getWidth() - kPerformanceOverlaySize - 10;
    float y = 10;
    float w = kPerformanceOverlaySize;
    float h = 100;

    performanceMonitor_->drawPerformanceOverlay(canvas, x, y, w, h);

    // Draw quality indicator
    float qualityY = y + h + 10;
    performanceMonitor_->drawQualityIndicator(canvas, x, qualityY, kQualityIndicatorSize);
}

//==============================================================================
// Event Handlers
//==============================================================================

void SkiaSessionView_Legendary::handleClipStateChangeForAccessibility(int trackIndex, int sceneIndex, const ClipSlotData& data) {
    if (accessibilityManager_) {
        accessibilityManager_->handleClipStateChange(trackIndex, sceneIndex, data);
    }
}

void SkiaSessionView_Legendary::handleSelectionChangeForAccessibility(int trackIndex, int sceneIndex) {
    if (accessibilityManager_) {
        accessibilityManager_->handleSelectionChange(trackIndex, sceneIndex);
    }
}

void SkiaSessionView_Legendary::setAccessibilityManager(std::shared_ptr<SessionAccessibilityManager> manager) {
    accessibilityManager_ = manager;
    if (accessibilityManager_) {
        accessibilityManager_->setPreferences(AccessibilityPreferences::createWCAGAA());
    }
}

void SkiaSessionView_Legendary::setPerformanceMonitor(std::shared_ptr<PerformanceMonitor> monitor) {
    performanceMonitor_ = monitor;
    if (performanceMonitor_) {
        performanceMonitor_->setQualitySettings(QualitySettings::createUltraQuality());
    }
}

void SkiaSessionView_Legendary::trackGPUPerformance(float gpuTimeMs, float memoryMB, float textureCount, float drawCalls) {
    if (performanceMonitor_) {
        performanceMonitor_->trackGPUPerformance(gpuTimeMs, memoryMB, textureCount, drawCalls);
    }
}

void SkiaSessionView_Legendary::trackMemoryUsage(size_t bytes) {
    if (performanceMonitor_) {
        performanceMonitor_->trackMemoryUsage(bytes);
    }
}

//==============================================================================
// Utilities
//==============================================================================

juce::String SkiaSessionView_Legendary::saveLegendProfile() const {
    juce::StringArray profile;

    // Save accessibility settings
    if (accessibilityManager_) {
        profile.add("accessibility:" + accessibilityManager_->exportAccessibilitySettings());
    }

    // Save performance settings
    if (performanceMonitor_) {
        profile.add("performance:" + performanceMonitor_->exportPerformanceData());
    }

    // Save animation settings
    profile.add("animations:" + juce::String(transitionAnimationsEnabled_ ? "enabled" : "disabled"));
    profile.add("easing:" + juce::String(static_cast<int>(defaultTransitionEasing_)));
    profile.add("duration:" + juce::String(defaultTransitionDuration_));

    // Save quality settings
    profile.add("quality:" + juce::String(static_cast<int>(currentQualityLevel_)));

    return profile.joinIntoString("\n");
}

void SkiaSessionView_Legendary::loadLegendProfile(const juce::String& profile) {
    juce::StringArray lines = juce::StringArray::fromLines(profile);

    for (const auto& line : lines) {
        if (line.startsWith("accessibility:")) {
            juce::String settings = line.fromFirstOccurrenceOf(":", false, false);
            if (accessibilityManager_) {
                accessibilityManager_->importAccessibilitySettings(settings);
            }
        } else if (line.startsWith("performance:")) {
            juce::String data = line.fromFirstOccurrenceOf(":", false, false);
            if (performanceMonitor_) {
                performanceMonitor_->importPerformanceData(data);
            }
        } else if (line.startsWith("animations:")) {
            juce::String value = line.fromFirstOccurrenceOf(":", false, false);
            setTransitionAnimationsEnabled(value == "enabled");
        } else if (line.startsWith("easing:")) {
            juce::String value = line.fromFirstOccurrenceOf(":", false, false);
            setTransitionEasing(static_cast<zenith::animation::Easing>(value.getIntValue()));
        } else if (line.startsWith("duration:")) {
            juce::String value = line.fromFirstOccurrenceOf(":", false, false);
            setTransitionDuration(value.getFloatValue());
        } else if (line.startsWith("quality:")) {
            juce::String value = line.fromFirstOccurrenceOf(":", false, false);
            setQualityLevel(static_cast<QualityLevel>(value.getIntValue()));
        }
    }
}

void SkiaSessionView_Legendary::resetLegendFeatures() {
    setTransitionAnimationsEnabled(true);
    setAccessibilityEnabled(true);
    setPerformanceMonitoringEnabled(true);
    setTransitionEasing(zenith::animation::Easing::EaseInOutCubic);
    setTransitionDuration(200.0f);
    setQualityLevel(QualityLevel::Auto);

    if (accessibilityManager_) {
        accessibilityManager_->setPreferences(AccessibilityPreferences::createWCAGAA());
    }

    if (performanceMonitor_) {
        performanceMonitor_->setQualitySettings(QualitySettings::createUltraQuality());
    }
}

juce::String SkiaSessionView_Legendary::getFeatureStatus() const {
    juce::StringArray status;

    status.add("Transition Animations: " + juce::String(transitionAnimationsEnabled_ ? "Enabled" : "Disabled"));
    status.add("Accessibility: " + juce::String(accessibilityEnabled_ ? "Enabled" : "Disabled"));
    status.add("Performance Monitoring: " + juce::String(performanceMonitoringEnabled_ ? "Enabled" : "Disabled"));
    status.add("Current Quality: " + juce::String(static_cast<int>(getCurrentQualityLevel())));

    if (performanceMonitor_) {
        status.add("Performance: " + performanceMonitor_->getPerformanceIndicator());
    }

    return status.joinIntoString("\n");
}

//==============================================================================
// Private Methods
//==============================================================================

void SkiaSessionView_Legendary::updateAnimations(float deltaMs) {
    for (auto& [slotKey, transition] : clipTransitions_) {
        if (transition && transition->isAnimating()) {
            transition->update(deltaMs);
            invalidateClipSlot(slotKey.first, slotKey.second);
        }
    }

    cleanupCompletedAnimations();
}

void SkiaSessionView_Legendary::drawEnhancedClipIndicators(SkCanvas* canvas, float x, float y,
                                                        float w, float h, const ClipSlotData& data,
                                                        const AnimationState& animationState) {
    VisualQualityAdjustments quality = getQualityAdjustments();

    // Draw enhanced indicators based on state
    switch (data.state) {
        case ClipSlotState::Playing:
            // Animated play indicator
            if (quality.enableGlow && animationState.glowIntensity > 0.0f) {
                SkPaint glowPaint;
                glowPaint.setColor(animationState.glowColor);
                glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
                glowPaint.setAlpha(static_cast<int>(animationState.glowIntensity * 255));
                canvas->drawCircle(x + w / 2, y + h / 2, 12, glowPaint);
            }

            // Pulsing animation
            if (animationState.pulsePhase > 0.0f) {
                float pulseScale = 1.0f + std::sin(animationState.pulsePhase * juce::MathConstants<float>::twoPi) * 0.1f;
                SkPaint pulsePaint;
                pulsePaint.setColor(zenith::ui::ZenithTheme::Colors::success.getARGB());
                canvas->drawCircle(x + w / 2, y + h / 2, 8 * pulseScale, pulsePaint);
            }
            break;

        case ClipSlotState::Recording:
            // Red recording indicator with pulse
            if (quality.enableGlow) {
                SkPaint recordPaint;
                recordPaint.setColor(zenith::ui::ZenithTheme::Colors::error.getARGB());
                recordPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
                canvas->drawCircle(x + w / 2, y + h / 2, 10, recordPaint);
            }
            break;

        case ClipSlotState::Queued:
            // Yellow queued indicator
            if (quality.enableGlow) {
                SkPaint queuedPaint;
                queuedPaint.setColor(zenith::ui::ZenithTheme::Colors::warning.getARGB());
                canvas->drawCircle(x + w / 2, y + h / 2, 6, queuedPaint);
            }
            break;

        case ClipSlotState::Stopped:
            // Simple indicator
            canvas->drawCircle(x + w / 2, y + h / 2, 4, zenith::ui::ZenithTheme::Colors::text_secondary);
            break;
    }
}

void SkiaSessionView_Legendary::applyAccessibilityColors(SkPaint& paint) const {
    if (accessibilityEnabled_ && accessibilityManager_) {
        // This would apply accessibility-specific color adjustments
        // Implementation depends on accessibility preferences
    }
}

bool SkiaSessionView_Legendary::shouldSkipAnimationForAccessibility() const {
    if (!accessibilityEnabled_ || !accessibilityManager_) {
        return false;
    }

    return accessibilityManager_->shouldSkipAnimation();
}

TransitionConfig SkiaSessionView_Legendary::getTransitionConfigForState(ClipSlotState fromState, ClipSlotState toState) const {
    return ClipTransitionPresets::getTransitionForClipState(fromState, toState);
}

AnimationState SkiaSessionView_Legendary::createAnimationStateFromClip(const ClipSlotData& data) const {
    using namespace ClipTransitionPresets;

    switch (data.state) {
        case ClipSlotState::Empty:
            return createEmptyState();
        case ClipSlotState::Stopped:
            return createStoppedState();
        case ClipSlotState::Playing:
            return createPlayingState();
        case ClipSlotState::Queued:
            return createQueuedState();
        case ClipSlotState::Recording:
            return createRecordingState();
        case ClipSlotState::Stopping:
            return createStoppingState();
        default:
            return createEmptyState();
    }
}

void SkiaSessionView_Legendary::cleanupCompletedAnimations() {
    // Remove completed animations
    for (auto it = clipTransitions_.begin(); it != clipTransitions_.end();) {
        if (it->second && !it->second->isAnimating()) {
            it = clipTransitions_.erase(it);
        } else {
            ++it;
        }
    }
}

void SkiaSessionView_Legendary::drawPerformanceIndicators(SkCanvas* canvas) {
    if (performanceMonitor_) {
        performanceMonitor_->drawPerformanceGraph(canvas, 10, getHeight() - 110, 200, 100, 30);
    }
}

void SkiaSessionView_Legendary::handlePerformanceWarning(const PerformanceMetrics& metrics) {
    if (onPerformanceWarning) {
        onPerformanceWarning(metrics);
    }
}

void SkiaSessionView_Legendary::handleQualityChange(QualityLevel oldLevel, QualityLevel newLevel) {
    currentQualityLevel_ = newLevel;
    if (onQualityChange) {
        onQualityChange(oldLevel, newLevel);
    }
}

void SkiaSessionView_Legendary::announceStateChange(int trackIndex, int sceneIndex, const ClipSlotData& data) {
    if (accessibilityManager_) {
        handleClipStateChangeForAccessibility(trackIndex, sceneIndex, data);
    }
}

juce::Rectangle<float> SkiaSessionView_Legendary::getClipSlotBounds(int trackIndex, int sceneIndex) const {
    float x = getTrackX(trackIndex);
    float y = getSceneY(sceneIndex);
    return juce::Rectangle<float>(x, y, kTrackWidth, kClipSlotHeight);
}

void SkiaSessionView_Legendary::invalidateClipSlot(int trackIndex, int sceneIndex) {
    float x = getTrackX(trackIndex);
    float y = getSceneY(sceneIndex);
    markDirtyRect(juce::Rectangle<float>(x, y, kTrackWidth, kClipSlotHeight));
}

VisualQualityAdjustments SkiaSessionView_Legendary::getQualityAdjustments() const {
    VisualQualityAdjustments quality;

    if (performanceMonitor_) {
        QualitySettings settings = performanceMonitor_->getQualitySettings();
        quality.enableGlow = settings.enableGlowEffects;
        quality.enableShadows = settings.enableShadows;
        quality.enableGradients = settings.enableGradients;
        quality.enableSubpixelAA = settings.enableSubpixelAA;
        quality.textureScale = settings.textureScale;
        quality.maxVisibleClips = settings.maxVisibleClips;
        quality.maxDirtyRects = settings.maxDirtyRects;
        quality.maxDrawCalls = settings.maxDrawCalls;
    }

    return quality;
}

int SkiaSessionView_Legendary::getDirtyRectCount() const {
    // This would return actual dirty rect count
    // For now, return a placeholder
    return 0;
}

int SkiaSessionView_Legendary::getAnimatingClipCount() const {
    int count = 0;
    for (const auto& [slotKey, transition] : clipTransitions_) {
        if (transition && transition->isAnimating()) {
            count++;
        }
    }
    return count;
}

AnimationState SkiaSessionView_Legendary::getAnimationState(int trackIndex, int sceneIndex) const {
    std::pair<int, int> slotKey(trackIndex, sceneIndex);
    auto it = animationStates_.find(slotKey);
    if (it != animationStates_.end()) {
        return it->second;
    }
    return AnimationState();
}

void SkiaSessionView_Legendary::setAnimationState(int trackIndex, int sceneIndex, const AnimationState& state) {
    std::pair<int, int> slotKey(trackIndex, sceneIndex);
    animationStates_[slotKey] = state;
    invalidateClipSlot(trackIndex, sceneIndex);
}

bool SkiaSessionView_Legendary::isClipAnimating(int trackIndex, int sceneIndex) const {
    std::pair<int, int> slotKey(trackIndex, sceneIndex);
    auto it = clipTransitions_.find(slotKey);
    return it != clipTransitions_.end() && it->second && it->second->isAnimating();
}

} // namespace zenith::ui