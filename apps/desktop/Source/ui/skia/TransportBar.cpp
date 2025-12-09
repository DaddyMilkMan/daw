/*
  ==============================================================================

    TransportBar.cpp
    Created: 2025-11-28
    Updated: 2025-12-08 - Major UI Overhaul with Spring Physics
    Author:  Leo "Lil Bit" Rossi, UI Overhaul Team

    Implementation of transport controls with modern microinteractions.

  ==============================================================================
*/

#include "TransportBar.h"
#include "ZenithDesignSystem.h"

#include "TransportBar.h"
#include "ZenithDesignSystem.h"

#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>
#ifdef ZENITH_USE_SKIA

namespace zenith {

<<<<<<< Updated upstream
TransportBar::TransportBar() {
    setSize(800, 60);
    initializeButtons();
    
    // Start animation timer at 60fps
    startTimerHz(60);
}

void TransportBar::initializeButtons() {
    // Play button - green
    playButton_.label = "▶";
    playButton_.activeColor = design::colors::NEON_GREEN;
    
    // Stop button - blue
    stopButton_.label = "■";
    stopButton_.activeColor = design::colors::BLUE;
    
    // Record button - red
    recordButton_.label = "●";
    recordButton_.activeColor = design::colors::RED;
    
    // View toggle - cyan
    viewToggleButton_.label = "↹";
    viewToggleButton_.activeColor = design::colors::CYAN;
    
    // Settings - white
    settingsButton_.label = "⚙";
    settingsButton_.activeColor = 0xFFFFFFFF;
}

void TransportBar::initializePaints() {
    if (paintsInitialized_) return;
    
    // Initialize typography
    ZenithTypography::initialize();
    transportFont_ = ZenithTypography::transportFont();
    labelFont_ = ZenithTypography::labelFont();
    
    paintsInitialized_ = true;
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

void TransportBar::setPlaying(bool playing) {
    if (isPlaying_ == playing) return;
    isPlaying_ = playing;
    playButton_.isActive = playing;
    
    // Trigger play pulse animation
    if (playing) {
        playPulse_.setTarget(1.0f);
    } else {
        playPulse_.setTarget(0.0f);
    }
    
    playButton_.glowIntensity.setTarget(playing ? 1.0f : 0.0f);
    stopButton_.isActive = !playing;
    stopButton_.glowIntensity.setTarget(!playing ? 0.3f : 0.0f);
    
    repaint();
}

void TransportBar::setRecording(bool recording) {
    if (isRecording_ == recording) return;
    isRecording_ = recording;
    recordButton_.isActive = recording;
    
    // Trigger record pulse
    if (recording) {
        recordPulse_.setTarget(1.0f);
    } else {
        recordPulse_.setTarget(0.0f);
    }
    
    recordButton_.glowIntensity.setTarget(recording ? 1.0f : 0.0f);
    repaint();
}

void TransportBar::setTempo(double bpm) {
    tempo_ = bpm;
    repaint();
}

void TransportBar::setCPU(float percent) {
    cpuUsage_ = percent;
    repaint();
}

void TransportBar::setPosition(double seconds) {
    position_ = seconds;
    repaint();
}

void TransportBar::setProjectName(const juce::String& name) {
    projectName_ = name;
    repaint();
}

void TransportBar::setTimeSignature(int num, int den) {
    timeSigNum_ = num;
    timeSigDen_ = den;
    repaint();
}

// ============================================================================
// LAYOUT
// ============================================================================

void TransportBar::resized() {
    initializePaints();
    updateButtonBounds();
}

void TransportBar::updateButtonBounds() {
    auto area = getLocalBounds().toFloat();
    const float buttonSize = 44.0f;
    const float buttonSpacing = 8.0f;
    const float padding = 12.0f;
    
    float x = padding;
    float y = (area.getHeight() - buttonSize) / 2.0f;
    
    // Transport buttons (left side)
    playButton_.bounds = juce::Rectangle<float>(x, y, buttonSize, buttonSize);
    x += buttonSize + buttonSpacing;
    
    stopButton_.bounds = juce::Rectangle<float>(x, y, buttonSize, buttonSize);
    x += buttonSize + buttonSpacing;
    
    recordButton_.bounds = juce::Rectangle<float>(x, y, buttonSize, buttonSize);
    
    // Right side buttons
    float rightX = area.getWidth() - padding - buttonSize;
    settingsButton_.bounds = juce::Rectangle<float>(rightX, y, buttonSize, buttonSize);
    rightX -= buttonSize + buttonSpacing;
    
    viewToggleButton_.bounds = juce::Rectangle<float>(rightX, y, buttonSize, buttonSize);
}

// ============================================================================
// ANIMATION
// ============================================================================

void TransportBar::timerCallback() {
    const float deltaSeconds = 1.0f / 60.0f;
    
    // Update button animations
    playButton_.updateAnimations(deltaSeconds);
    stopButton_.updateAnimations(deltaSeconds);
    recordButton_.updateAnimations(deltaSeconds);
    viewToggleButton_.updateAnimations(deltaSeconds);
    settingsButton_.updateAnimations(deltaSeconds);
    
    // Update pulse animations
    playPulse_.update(deltaSeconds);
    recordPulse_.update(deltaSeconds);
    
    // Continuous pulse phase for active states
    if (isPlaying_ || isRecording_) {
        pulsePhase_ += deltaSeconds * 2.0f;  // 2 cycles per second
        if (pulsePhase_ > 6.28318f) pulsePhase_ -= 6.28318f;
    }
    
    // Check if we need to keep repainting
    bool needsRepaint = playButton_.isAnimating() || 
                        stopButton_.isAnimating() ||
                        recordButton_.isAnimating() ||
                        viewToggleButton_.isAnimating() ||
                        settingsButton_.isAnimating() ||
                        playPulse_.isAnimating() ||
                        recordPulse_.isAnimating() ||
                        isPlaying_ || isRecording_;  // Always repaint when playing/recording
    
    if (needsRepaint) {
        repaint();
    }
}

// ============================================================================
// MOUSE HANDLING
// ============================================================================

TransportBar::ButtonState* TransportBar::findButtonAt(const juce::Point<int>& pos) {
    juce::Point<float> posF(static_cast<float>(pos.x), static_cast<float>(pos.y));
    
    if (playButton_.bounds.contains(posF)) return &playButton_;
    if (stopButton_.bounds.contains(posF)) return &stopButton_;
    if (recordButton_.bounds.contains(posF)) return &recordButton_;
    if (viewToggleButton_.bounds.contains(posF)) return &viewToggleButton_;
    if (settingsButton_.bounds.contains(posF)) return &settingsButton_;
    
    return nullptr;
}

void TransportBar::updateHoverState(const juce::Point<int>& pos) {
    ButtonState* buttons[] = {&playButton_, &stopButton_, &recordButton_, 
                              &viewToggleButton_, &settingsButton_};
    
    ButtonState* hoveredButton = findButtonAt(pos);
    
    for (auto* btn : buttons) {
        bool wasHovered = btn->isHovered;
        btn->isHovered = (btn == hoveredButton);
        
        if (btn->isHovered && !wasHovered) {
            // Just started hovering - animate scale up
            btn->hoverScale.setTarget(1.08f);  // 8% larger
            btn->glowIntensity.setTarget(btn->isActive ? 1.2f : 0.4f);
        } else if (!btn->isHovered && wasHovered) {
            // Just stopped hovering - animate back
            btn->hoverScale.setTarget(1.0f);
            btn->glowIntensity.setTarget(btn->isActive ? 1.0f : 0.0f);
        }
    }
}

void TransportBar::mouseMove(const juce::MouseEvent& e) {
    updateHoverState(e.getPosition());
}

void TransportBar::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    
    // Reset all hover states
    ButtonState* buttons[] = {&playButton_, &stopButton_, &recordButton_, 
                              &viewToggleButton_, &settingsButton_};
    for (auto* btn : buttons) {
        if (btn->isHovered) {
            btn->isHovered = false;
            btn->hoverScale.setTarget(1.0f);
            btn->glowIntensity.setTarget(btn->isActive ? 1.0f : 0.0f);
        }
    }
}

=======
// ============================================================================
// CONSTRUCTION
// ============================================================================

TransportBar::TransportBar() {
    setSize(800, 60);
    initializeButtons();
    
    // Start animation timer at 60fps
    startTimerHz(60);
}

void TransportBar::initializeButtons() {
    // Play button - green
    playButton_.label = "▶";
    playButton_.activeColor = design::colors::NEON_GREEN;
    
    // Stop button - blue
    stopButton_.label = "■";
    stopButton_.activeColor = design::colors::BLUE;
    
    // Record button - red
    recordButton_.label = "●";
    recordButton_.activeColor = design::colors::RED;
    
    // View toggle - cyan
    viewToggleButton_.label = "↹";
    viewToggleButton_.activeColor = design::colors::CYAN;
    
    // Settings - white
    settingsButton_.label = "⚙";
    settingsButton_.activeColor = 0xFFFFFFFF;
}

void TransportBar::initializePaints() {
    if (paintsInitialized_) return;
    
    // Initialize typography
    ZenithTypography::initialize();
    transportFont_ = ZenithTypography::transportFont();
    labelFont_ = ZenithTypography::labelFont();
    
    paintsInitialized_ = true;
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

void TransportBar::setPlaying(bool playing) {
    if (isPlaying_ == playing) return;
    isPlaying_ = playing;
    playButton_.isActive = playing;
    
    // Trigger play pulse animation
    if (playing) {
        playPulse_.setTarget(1.0f);
    } else {
        playPulse_.setTarget(0.0f);
    }
    
    playButton_.glowIntensity.setTarget(playing ? 1.0f : 0.0f);
    stopButton_.isActive = !playing;
    stopButton_.glowIntensity.setTarget(!playing ? 0.3f : 0.0f);
    
    repaint();
}

void TransportBar::setRecording(bool recording) {
    if (isRecording_ == recording) return;
    isRecording_ = recording;
    recordButton_.isActive = recording;
    
    // Trigger record pulse
    if (recording) {
        recordPulse_.setTarget(1.0f);
    } else {
        recordPulse_.setTarget(0.0f);
    }
    
    recordButton_.glowIntensity.setTarget(recording ? 1.0f : 0.0f);
    repaint();
}

void TransportBar::setTempo(double bpm) {
    tempo_ = bpm;
    repaint();
}

void TransportBar::setCPU(float percent) {
    cpuUsage_ = percent;
    repaint();
}

void TransportBar::setPosition(double seconds) {
    position_ = seconds;
    repaint();
}

void TransportBar::setProjectName(const juce::String& name) {
    projectName_ = name;
    repaint();
}

void TransportBar::setTimeSignature(int num, int den) {
    timeSigNum_ = num;
    timeSigDen_ = den;
    repaint();
}

// ============================================================================
// LAYOUT
// ============================================================================

void TransportBar::resized() {
    initializePaints();
    updateButtonBounds();
}

void TransportBar::updateButtonBounds() {
    auto area = getLocalBounds().toFloat();
    const float buttonSize = 44.0f;
    const float buttonSpacing = 8.0f;
    const float padding = 12.0f;
    
    float x = padding;
    float y = (area.getHeight() - buttonSize) / 2.0f;
    
    // Transport buttons (left side)
    playButton_.bounds = juce::Rectangle<float>(x, y, buttonSize, buttonSize);
    x += buttonSize + buttonSpacing;
    
    stopButton_.bounds = juce::Rectangle<float>(x, y, buttonSize, buttonSize);
    x += buttonSize + buttonSpacing;
    
    recordButton_.bounds = juce::Rectangle<float>(x, y, buttonSize, buttonSize);
    
    // Right side buttons
    float rightX = area.getWidth() - padding - buttonSize;
    settingsButton_.bounds = juce::Rectangle<float>(rightX, y, buttonSize, buttonSize);
    rightX -= buttonSize + buttonSpacing;
    
    viewToggleButton_.bounds = juce::Rectangle<float>(rightX, y, buttonSize, buttonSize);
}

// ============================================================================
// ANIMATION
// ============================================================================

void TransportBar::timerCallback() {
    const float deltaSeconds = 1.0f / 60.0f;
    
    // Update button animations
    playButton_.updateAnimations(deltaSeconds);
    stopButton_.updateAnimations(deltaSeconds);
    recordButton_.updateAnimations(deltaSeconds);
    viewToggleButton_.updateAnimations(deltaSeconds);
    settingsButton_.updateAnimations(deltaSeconds);
    
    // Update pulse animations
    playPulse_.update(deltaSeconds);
    recordPulse_.update(deltaSeconds);
    
    // Continuous pulse phase for active states
    if (isPlaying_ || isRecording_) {
        pulsePhase_ += deltaSeconds * 2.0f;  // 2 cycles per second
        if (pulsePhase_ > 6.28318f) pulsePhase_ -= 6.28318f;
    }
    
    // Check if we need to keep repainting
    bool needsRepaint = playButton_.isAnimating() || 
                        stopButton_.isAnimating() ||
                        recordButton_.isAnimating() ||
                        viewToggleButton_.isAnimating() ||
                        settingsButton_.isAnimating() ||
                        playPulse_.isAnimating() ||
                        recordPulse_.isAnimating() ||
                        isPlaying_ || isRecording_;  // Always repaint when playing/recording
    
    if (needsRepaint) {
        repaint();
    }
}

// ============================================================================
// MOUSE HANDLING
// ============================================================================

TransportBar::ButtonState* TransportBar::findButtonAt(const juce::Point<int>& pos) {
    juce::Point<float> posF(static_cast<float>(pos.x), static_cast<float>(pos.y));
    
    if (playButton_.bounds.contains(posF)) return &playButton_;
    if (stopButton_.bounds.contains(posF)) return &stopButton_;
    if (recordButton_.bounds.contains(posF)) return &recordButton_;
    if (viewToggleButton_.bounds.contains(posF)) return &viewToggleButton_;
    if (settingsButton_.bounds.contains(posF)) return &settingsButton_;
    
    return nullptr;
}

void TransportBar::updateHoverState(const juce::Point<int>& pos) {
    ButtonState* buttons[] = {&playButton_, &stopButton_, &recordButton_, 
                              &viewToggleButton_, &settingsButton_};
    
    ButtonState* hoveredButton = findButtonAt(pos);
    
    for (auto* btn : buttons) {
        bool wasHovered = btn->isHovered;
        btn->isHovered = (btn == hoveredButton);
        
        if (btn->isHovered && !wasHovered) {
            // Just started hovering - animate scale up
            btn->hoverScale.setTarget(1.08f);  // 8% larger
            btn->glowIntensity.setTarget(btn->isActive ? 1.2f : 0.4f);
        } else if (!btn->isHovered && wasHovered) {
            // Just stopped hovering - animate back
            btn->hoverScale.setTarget(1.0f);
            btn->glowIntensity.setTarget(btn->isActive ? 1.0f : 0.0f);
        }
    }
}

void TransportBar::mouseMove(const juce::MouseEvent& e) {
    updateHoverState(e.getPosition());
}

void TransportBar::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    
    // Reset all hover states
    ButtonState* buttons[] = {&playButton_, &stopButton_, &recordButton_, 
                              &viewToggleButton_, &settingsButton_};
    for (auto* btn : buttons) {
        if (btn->isHovered) {
            btn->isHovered = false;
            btn->hoverScale.setTarget(1.0f);
            btn->glowIntensity.setTarget(btn->isActive ? 1.0f : 0.0f);
        }
    }
}

void TransportBar::mouseDown(const juce::MouseEvent& e) {
    ButtonState* btn = findButtonAt(e.getPosition());
    if (btn) {
        btn->isPressed = true;
        btn->pressScale.setTarget(0.92f);  // Press shrinks button slightly
    }
}

void TransportBar::mouseUp(const juce::MouseEvent& e) {
    ButtonState* clickedBtn = findButtonAt(e.getPosition());
    
    // Reset all press states
    ButtonState* buttons[] = {&playButton_, &stopButton_, &recordButton_, 
                              &viewToggleButton_, &settingsButton_};
    for (auto* btn : buttons) {
        if (btn->isPressed) {
            btn->isPressed = false;
            btn->pressScale.setTarget(1.0f);
        }
    }
    
    // Trigger click callbacks
    if (clickedBtn == &playButton_ && onPlayClicked) {
        onPlayClicked();
    } else if (clickedBtn == &stopButton_ && onStopClicked) {
        onStopClicked();
    } else if (clickedBtn == &recordButton_ && onRecordClicked) {
        onRecordClicked();
    } else if (clickedBtn == &viewToggleButton_ && onViewToggleClicked) {
        onViewToggleClicked();
    } else if (clickedBtn == &settingsButton_ && onSettingsClicked) {
        onSettingsClicked();
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void TransportBar::drawSkia(SkCanvas* canvas) {
    initializePaints();
    
    drawBackground(canvas);
    
    // Draw transport buttons
    drawTransportButton(canvas, playButton_);
    drawTransportButton(canvas, stopButton_);
    drawTransportButton(canvas, recordButton_);
    drawTransportButton(canvas, viewToggleButton_);
    drawTransportButton(canvas, settingsButton_);
    
    // Draw displays
    drawTimeDisplay(canvas);
    drawTempoDisplay(canvas);
    drawCPUMeter(canvas);
    drawProjectName(canvas);
}

void TransportBar::drawBackground(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    
    // Gradient background
    SkPoint pts[2] = {{0, 0}, {0, skBounds.height()}};
    SkColor colors[2] = {
        SkColorSetARGB(245, 18, 18, 22),   // Slightly transparent dark
        SkColorSetARGB(245, 10, 10, 14)
    };
    
    SkPaint bgPaint;
    bgPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
    bgPaint.setAntiAlias(true);
    canvas->drawRect(skBounds, bgPaint);
    
    // Bottom border glow
    SkPaint borderPaint;
    borderPaint.setColor(design::colors::CYAN);
    borderPaint.setAlpha(40);
    borderPaint.setAntiAlias(true);
    canvas->drawLine(0, skBounds.height() - 1, skBounds.width(), skBounds.height() - 1, borderPaint);
    
    // Subtle highlight at top
    SkPaint highlightPaint;
    highlightPaint.setColor(SkColorSetARGB(15, 255, 255, 255));
    canvas->drawLine(0, 0, skBounds.width(), 0, highlightPaint);
}

void TransportBar::drawTransportButton(SkCanvas* canvas, ButtonState& button) {
    const float scale = button.getScale();
    const float glowIntensity = button.glowIntensity.getValue();
    
    // Calculate scaled bounds
    float centerX = button.bounds.getCentreX();
    float centerY = button.bounds.getCentreY();
    float halfWidth = button.bounds.getWidth() * scale * 0.5f;
    float halfHeight = button.bounds.getHeight() * scale * 0.5f;
    
    SkRect scaledBounds = SkRect::MakeXYWH(
        centerX - halfWidth, centerY - halfHeight,
        halfWidth * 2.0f, halfHeight * 2.0f
    );
    
    SkRRect rrect = SkRRect::MakeRectXY(scaledBounds, 10.0f * scale, 10.0f * scale);
    
    // Glow layer (when hovered or active)
    if (glowIntensity > 0.01f) {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setColor(button.activeColor);
        glowPaint.setAlpha(static_cast<U8CPU>(glowIntensity * 100));
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 12.0f * glowIntensity));
        canvas->drawRRect(rrect, glowPaint);
    }
    
    // Button background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    
    SkPoint pts[2] = {{scaledBounds.left(), scaledBounds.top()}, 
                      {scaledBounds.left(), scaledBounds.bottom()}};
    
    SkColor bgColors[2];
    if (button.isActive) {
        // Active state - vibrant gradient
        bgColors[0] = SkColorSetA(button.activeColor, 80);
        bgColors[1] = SkColorSetA(button.activeColor, 40);
    } else if (button.isHovered) {
        // Hover state - subtle highlight
        bgColors[0] = SkColorSetARGB(60, 255, 255, 255);
        bgColors[1] = SkColorSetARGB(30, 255, 255, 255);
    } else {
        // Default state - glass effect
        bgColors[0] = SkColorSetARGB(35, 255, 255, 255);
        bgColors[1] = SkColorSetARGB(15, 255, 255, 255);
    }
    
    bgPaint.setShader(SkGradientShader::MakeLinear(pts, bgColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRRect(rrect, bgPaint);
    
    // Border (rim light effect)
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    
    SkColor borderColors[2] = {
        SkColorSetARGB(80, 255, 255, 255),  // Top: bright
        SkColorSetARGB(30, 0, 0, 0)          // Bottom: shadow
    };
    borderPaint.setShader(SkGradientShader::MakeLinear(pts, borderColors, nullptr, 2, SkTileMode::kClamp));
    
    SkRRect borderRRect = rrect;
    borderRRect.inset(0.5f, 0.5f);
    canvas->drawRRect(borderRRect, borderPaint);
    
    // Icon/Label
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(button.isActive ? SK_ColorWHITE : SkColorSetARGB(220, 255, 255, 255));
    
    SkFont iconFont;
    iconFont.setSize(20.0f * scale);
    iconFont.setEdging(SkFont::Edging::kAntiAlias);
    
    // Measure and center text
    const char* label = button.label.toRawUTF8();
    float textWidth = iconFont.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    float textX = centerX - textWidth / 2.0f;
    float textY = centerY + 6.0f * scale;
    
    // Text shadow for depth
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
    canvas->drawSimpleText(label, strlen(label), SkTextEncoding::kUTF8, 
                           textX, textY + 1.5f, iconFont, shadowPaint);
    
    // Main text
    canvas->drawSimpleText(label, strlen(label), SkTextEncoding::kUTF8, 
                           textX, textY, iconFont, textPaint);
    
    // Active pulse ring (for play/record)
    if (button.isActive && (&button == &playButton_ || &button == &recordButton_)) {
        float pulseIntensity = (&button == &playButton_) ? 
            playPulse_.getValue() : recordPulse_.getValue();
        
        // Continuous gentle pulse when active
        float pulse = 0.5f + 0.5f * std::sin(pulsePhase_);
        pulseIntensity *= pulse;
        
        if (pulseIntensity > 0.01f) {
            SkPaint pulsePaint;
            pulsePaint.setAntiAlias(true);
            pulsePaint.setStyle(SkPaint::kStroke_Style);
            pulsePaint.setStrokeWidth(2.0f);
            pulsePaint.setColor(button.activeColor);
            pulsePaint.setAlpha(static_cast<U8CPU>(pulseIntensity * 150));
            
            SkRRect pulseRRect = rrect;
            pulseRRect.outset(3.0f * pulseIntensity, 3.0f * pulseIntensity);
            canvas->drawRRect(pulseRRect, pulsePaint);
        }
    }
}

void TransportBar::drawTimeDisplay(SkCanvas* canvas) {
    // Position after transport buttons
    float x = 180.0f;
    float y = getHeight() / 2.0f;
    
    // Format time as MM:SS.mmm
    int minutes = static_cast<int>(position_) / 60;
    int seconds = static_cast<int>(position_) % 60;
    int millis = static_cast<int>((position_ - std::floor(position_)) * 1000);
    
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d.%03d", minutes, seconds, millis);
    
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    
    canvas->drawString(timeStr, x, y + 6.0f, transportFont_, textPaint);
    
    // Label above
    SkPaint labelPaint;
    labelPaint.setAntiAlias(true);
    labelPaint.setColor(design::colors::TEXT_SECONDARY);
    labelPaint.setAlpha(150);
    
    canvas->drawString("TIME", x, y - 8.0f, labelFont_, labelPaint);
}

void TransportBar::drawTempoDisplay(SkCanvas* canvas) {
    float x = 320.0f;
    float y = getHeight() / 2.0f;
    
    char tempoStr[32];
    snprintf(tempoStr, sizeof(tempoStr), "%.1f", tempo_);
    
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    
    canvas->drawString(tempoStr, x, y + 6.0f, transportFont_, textPaint);
    
    // BPM label
    SkPaint labelPaint;
    labelPaint.setAntiAlias(true);
    labelPaint.setColor(design::colors::TEXT_SECONDARY);
    labelPaint.setAlpha(150);
    
    canvas->drawString("BPM", x, y - 8.0f, labelFont_, labelPaint);
    
    // Time signature
    char sigStr[16];
    snprintf(sigStr, sizeof(sigStr), "%d/%d", timeSigNum_, timeSigDen_);
    canvas->drawString(sigStr, x + 70.0f, y + 6.0f, labelFont_, textPaint);
}

void TransportBar::drawCPUMeter(SkCanvas* canvas) {
    float width = getWidth();
    float meterWidth = 80.0f;
    float meterHeight = 16.0f;
    float x = width - 280.0f;
    float y = (getHeight() - meterHeight) / 2.0f;
    
    SkRect meterBounds = SkRect::MakeXYWH(x, y, meterWidth, meterHeight);
    SkRRect meterRRect = SkRRect::MakeRectXY(meterBounds, 4.0f, 4.0f);
    
    // Background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(SkColorSetARGB(60, 0, 0, 0));
    canvas->drawRRect(meterRRect, bgPaint);
    
    // Fill
    float fillWidth = meterWidth * juce::jlimit(0.0f, 1.0f, cpuUsage_ / 100.0f);
    if (fillWidth > 0) {
        SkRect fillBounds = SkRect::MakeXYWH(x, y, fillWidth, meterHeight);
        SkRRect fillRRect = SkRRect::MakeRectXY(fillBounds, 4.0f, 4.0f);
        
        // Color based on load
        SkColor fillColor;
        if (cpuUsage_ > 80.0f) {
            fillColor = design::colors::RED;
        } else if (cpuUsage_ > 60.0f) {
            fillColor = design::colors::AMBER;
        } else {
            fillColor = design::colors::NEON_GREEN;
        }
        
        SkPaint fillPaint;
        fillPaint.setAntiAlias(true);
        fillPaint.setColor(fillColor);
        canvas->drawRRect(fillRRect, fillPaint);
    }
    
    // Label
    SkPaint labelPaint;
    labelPaint.setAntiAlias(true);
    labelPaint.setColor(design::colors::TEXT_SECONDARY);
    labelPaint.setAlpha(150);
    canvas->drawString("CPU", x, y - 4.0f, labelFont_, labelPaint);
}

void TransportBar::drawProjectName(SkCanvas* canvas) {
    float x = 450.0f;
    float y = getHeight() / 2.0f + 4.0f;
    
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::colors::TEXT_SECONDARY);
    textPaint.setAlpha(180);
    
    canvas->drawString(projectName_.toStdString().c_str(), x, y, labelFont_, textPaint);
>>>>>>> Stashed changes
}

} // namespace zenith

#endif // ZENITH_USE_SKIA

