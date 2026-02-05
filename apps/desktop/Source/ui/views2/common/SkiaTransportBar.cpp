/*
  ==============================================================================

    SkiaTransportBar.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Transport bar implementation.

  ==============================================================================
*/

#include "SkiaTransportBar.h"
#include "../../design-system/ZenithTheme.h"
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <cmath>

namespace zenith::ui {

SkiaTransportBar::SkiaTransportBar() {
    setSize(1920, static_cast<int>(kBarHeight));
}

//==============================================================================
// State Management
//==============================================================================

void SkiaTransportBar::setState(TransportState state) {
    if (state_ != state) {
        state_ = state;
        markDirty();
    }
}

void SkiaTransportBar::setRecordArmed(bool armed) {
    if (recordArmed_ != armed) {
        recordArmed_ = armed;
        markDirty();
    }
}

void SkiaTransportBar::setLoopEnabled(bool enabled) {
    if (loopEnabled_ != enabled) {
        loopEnabled_ = enabled;
        markDirty();
    }
}

void SkiaTransportBar::setMetronomeEnabled(bool enabled) {
    if (metronomeEnabled_ != enabled) {
        metronomeEnabled_ = enabled;
        markDirty();
    }
}

void SkiaTransportBar::setPosition(double beats) {
    positionBeats_ = beats;
    markDirty();
}

void SkiaTransportBar::setPositionSMPTE(int hours, int mins, int secs, int frames) {
    // Convert to beats (simplified)
    juce::ignoreUnused(hours, mins, secs, frames);
    markDirty();
}

void SkiaTransportBar::setTempo(float bpm) {
    bpm = std::clamp(bpm, 20.0f, 999.0f);
    if (tempo_ != bpm) {
        tempo_ = bpm;
        markDirty();
        if (onTempoChange) onTempoChange(bpm);
    }
}

void SkiaTransportBar::setTimeSignature(int numerator, int denominator) {
    timeSigNum_ = numerator;
    timeSigDenom_ = denominator;
    markDirty();
}

void SkiaTransportBar::setProjectLength(double beats) {
    projectLength_ = beats;
    markDirty();
}

void SkiaTransportBar::setActiveView(ActiveView view) {
    if (activeView_ != view) {
        activeView_ = view;
        markDirty();
        if (onViewChange) onViewChange(view);
    }
}

//==============================================================================
// Drawing
//==============================================================================

void SkiaTransportBar::drawSkia(SkCanvas* canvas) {
    SkAutoCanvasRestore acr(canvas, true);
    
    drawBackground(canvas);
    drawTransportButtons(canvas);
    drawTempoSection(canvas);
    drawTimeDisplay(canvas);
    drawPositionSlider(canvas);
    drawViewToggle(canvas);
    drawCPUMeter(canvas);
}

void SkiaTransportBar::drawBackground(SkCanvas* canvas) {
    // Main background
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_02.getARGB());
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
    
    // Bottom border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    canvas->drawLine(0, getHeight() - 1, getWidth(), getHeight() - 1, borderPaint);
}

void SkiaTransportBar::drawTransportButtons(SkCanvas* canvas) {
    float startX = 16;
    float y = (getHeight() - kButtonSize) / 2;
    
    // Rewind
    drawButton(canvas, startX, y, BTN_REWIND, "⏮", false);
    startX += kButtonSize + kButtonSpacing;
    
    // Stop
    bool isStopActive = (state_ == TransportState::Stopped);
    drawButton(canvas, startX, y, BTN_STOP, "⏹", isStopActive);
    startX += kButtonSize + kButtonSpacing;
    
    // Play/Pause
    bool isPlaying = (state_ == TransportState::Playing);
    drawButton(canvas, startX, y, BTN_PLAY, isPlaying ? "⏸" : "▶", isPlaying);
    startX += kButtonSize + kButtonSpacing;
    
    // Record
    bool isRecording = (state_ == TransportState::Recording);
    drawButton(canvas, startX, y, BTN_RECORD, "⏺", isRecording || recordArmed_);
    startX += kButtonSize + kButtonSpacing + 8;
    
    // Divider
    SkPaint divPaint;
    divPaint.setColor(ZenithTheme::Colors::border_subtle.getARGB());
    canvas->drawLine(startX, 8, startX, getHeight() - 8, divPaint);
    startX += 16;
    
    // Loop
    drawButton(canvas, startX, y, BTN_LOOP, "⟲", loopEnabled_, true);
    startX += kButtonSize + kButtonSpacing;
    
    // Metronome
    drawButton(canvas, startX, y, BTN_METRONOME, "🔔", metronomeEnabled_, true);
}

void SkiaTransportBar::drawButton(SkCanvas* canvas, float x, float y, int buttonId,
                                   const char* icon, bool isActive, bool isToggle) {
    SkRect btnRect = SkRect::MakeXYWH(x, y, kButtonSize, kButtonSize);
    SkRRect btnRRect = SkRRect::MakeRectXY(btnRect, 6, 6);
    
    bool isHovered = (hoveredButton_ == buttonId);
    bool isRecord = (buttonId == BTN_RECORD);
    
    // Background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    
    if (isRecord && (isActive || recordArmed_)) {
        // Record button gets red with pulse
        float pulse = 0.7f + 0.3f * std::sin(recordPulse_);
        bgPaint.setColor(SkColorSetARGB(static_cast<int>(pulse * 255), 239, 68, 68));
        
        // Glow
        SkPaint glowPaint;
        glowPaint.setColor(SkColorSetARGB(static_cast<int>(pulse * 100), 239, 68, 68));
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
        canvas->drawRRect(btnRRect, glowPaint);
    } else if (isActive && !isToggle) {
        bgPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    } else if (isActive && isToggle) {
        bgPaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.3f).getARGB());
    } else if (isHovered) {
        bgPaint.setColor(ZenithTheme::Colors::hover_overlay.getARGB());
    } else {
        bgPaint.setColor(SkColorSetARGB(0, 0, 0, 0));  // Transparent
    }
    
    canvas->drawRRect(btnRRect, bgPaint);
    
    // Border for toggle buttons
    if (isToggle && isActive) {
        SkPaint borderPaint;
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(1.5f);
        borderPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
        borderPaint.setAntiAlias(true);
        canvas->drawRRect(btnRRect, borderPaint);
    }
    
    // Icon
    SkPaint iconPaint;
    if (isRecord && isActive) {
        iconPaint.setColor(SK_ColorWHITE);
    } else if (isActive && isToggle) {
        iconPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
    } else if (isActive) {
        iconPaint.setColor(SK_ColorWHITE);
    } else {
        iconPaint.setColor(ZenithTheme::Colors::text_secondary.getARGB());
    }
    iconPaint.setAntiAlias(true);
    
    SkFont iconFont = design::typography::getSkFont(14.0f);
    canvas->drawString(icon, x + 8, y + 22, iconFont, iconPaint);
}

void SkiaTransportBar::drawTempoSection(SkCanvas* canvas) {
    float x = 280;
    float y = getHeight() / 2;
    
    // Tempo value
    SkPaint tempoPaint;
    tempoPaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
    tempoPaint.setAntiAlias(true);
    SkFont tempoFont = design::typography::getSkFont(16.0f, design::FontWeight::Medium);
    
    juce::String tempoStr = juce::String(tempo_, 2);
    canvas->drawString(tempoStr.toRawUTF8(), x, y + 6, tempoFont, tempoPaint);
    
    // BPM label
    SkPaint labelPaint;
    labelPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
    labelPaint.setAntiAlias(true);
    SkFont labelFont = design::typography::getSkFont(10.0f);
    canvas->drawString("BPM", x + 60, y + 6, labelFont, labelPaint);
    
    // Divider
    SkPaint divPaint;
    divPaint.setColor(ZenithTheme::Colors::border_subtle.getARGB());
    canvas->drawLine(x + 90, 8, x + 90, getHeight() - 8, divPaint);
    
    // Time signature
    x += 105;
    juce::String timeSigStr = juce::String(timeSigNum_) + "/" + juce::String(timeSigDenom_);
    canvas->drawString(timeSigStr.toRawUTF8(), x, y + 6, tempoFont, tempoPaint);
}

void SkiaTransportBar::drawTimeDisplay(SkCanvas* canvas) {
    float x = 430;
    float y = getHeight() / 2;
    
    // Calculate bars/beats/ticks from position
    int bar = static_cast<int>(positionBeats_ / timeSigNum_) + 1;
    int beat = static_cast<int>(std::fmod(positionBeats_, timeSigNum_)) + 1;
    int tick = static_cast<int>(std::fmod(positionBeats_ * 480, 480));
    
    // Format: BAR.BEAT.TICK
    juce::String posStr = juce::String::formatted("%d.%d.%03d", bar, beat, tick);
    
    SkPaint posPaint;
    posPaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
    posPaint.setAntiAlias(true);
    SkFont posFont = design::typography::getSkFont(16.0f, design::FontWeight::Medium);
    canvas->drawString(posStr.toRawUTF8(), x, y + 6, posFont, posPaint);
}

void SkiaTransportBar::drawPositionSlider(SkCanvas* canvas) {
    float x = 580;
    float w = 300;
    float y = getHeight() / 2 - 3;
    float h = 6;
    
    // Track
    SkRect trackRect = SkRect::MakeXYWH(x, y, w, h);
    SkRRect trackRRect = SkRRect::MakeRectXY(trackRect, 3, 3);
    
    SkPaint trackPaint;
    trackPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRRect(trackRRect, trackPaint);
    
    // Progress
    float progress = static_cast<float>(positionBeats_ / projectLength_);
    progress = std::clamp(progress, 0.0f, 1.0f);
    float progressW = w * progress;
    
    SkRect progressRect = SkRect::MakeXYWH(x, y, progressW, h);
    SkRRect progressRRect = SkRRect::MakeRectXY(progressRect, 3, 3);
    
    SkPaint progressPaint;
    SkPoint gradPts[2] = {{x, 0}, {x + progressW, 0}};
    SkColor gradColors[2] = {
        ZenithTheme::Colors::accent_primary.getARGB(),
        ZenithTheme::Colors::accent_secondary.getARGB()
    };
    progressPaint.setShader(SkGradientShader::MakeLinear(
        gradPts, gradColors, nullptr, 2, SkTileMode::kClamp
    ));
    canvas->drawRRect(progressRRect, progressPaint);
    
    // Playhead
    float playheadX = x + progressW;
    SkPaint playheadPaint;
    playheadPaint.setColor(SK_ColorWHITE);
    playheadPaint.setAntiAlias(true);
    canvas->drawCircle(playheadX, y + h/2, 5, playheadPaint);
    
    // End position label
    int endBar = static_cast<int>(projectLength_ / timeSigNum_) + 1;
    juce::String endStr = juce::String(endBar) + ".1.000";
    
    SkPaint endPaint;
    endPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
    endPaint.setAntiAlias(true);
    SkFont smallFont = design::typography::getSkFont(10.0f);
    canvas->drawString(endStr.toRawUTF8(), x + w + 8, getHeight() / 2 + 4, smallFont, endPaint);
}

void SkiaTransportBar::drawViewToggle(SkCanvas* canvas) {
    float x = getWidth() - 220;
    float y = (getHeight() - 28) / 2;
    
    // Background pill
    SkRect pillRect = SkRect::MakeXYWH(x, y, 160, 28);
    SkRRect pillRRect = SkRRect::MakeRectXY(pillRect, 14, 14);
    
    SkPaint pillPaint;
    pillPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRRect(pillRRect, pillPaint);
    
    // Buttons
    float btnW = 50;
    float btnH = 24;
    float btnY = y + 2;
    
    const char* labels[] = {"Arr", "Ses", "Jam"};
    ActiveView views[] = {ActiveView::Arrangement, ActiveView::Session, ActiveView::AIJam};
    
    for (int i = 0; i < 3; ++i) {
        float btnX = x + 3 + i * (btnW + 2);
        bool isActive = (activeView_ == views[i]);
        
        if (isActive) {
            // Active indicator
            SkRect activeRect = SkRect::MakeXYWH(btnX, btnY, btnW, btnH);
            SkRRect activeRRect = SkRRect::MakeRectXY(activeRect, 12, 12);
            
            SkPaint activePaint;
            SkPoint gradPts[2] = {{btnX, 0}, {btnX + btnW, 0}};
            SkColor gradColors[2] = {
                ZenithTheme::Colors::accent_primary.getARGB(),
                ZenithTheme::Colors::accent_secondary.getARGB()
            };
            activePaint.setShader(SkGradientShader::MakeLinear(
                gradPts, gradColors, nullptr, 2, SkTileMode::kClamp
            ));
            canvas->drawRRect(activeRRect, activePaint);
        }
        
        // Label
        SkPaint labelPaint;
        labelPaint.setColor(isActive ? SK_ColorWHITE : ZenithTheme::Colors::text_secondary.getARGB());
        labelPaint.setAntiAlias(true);
        SkFont labelFont = design::typography::getSkFont(10.0f);
        canvas->drawString(labels[i], btnX + 14, btnY + 16, labelFont, labelPaint);
    }
}

void SkiaTransportBar::setCPULoad(float load) {
    load = std::clamp(load, 0.0f, 1.0f);
    if (std::abs(cpuLoad_ - load) > 0.01f) {
        cpuLoad_ = load;
        markDirty();
    }
}

void SkiaTransportBar::drawCPUMeter(SkCanvas* canvas) {
    float x = getWidth() - 50;
    float y = getHeight() / 2;
    
    SkPaint labelPaint;
    // Color based on load: green -> yellow -> red
    if (cpuLoad_ < 0.5f) {
        labelPaint.setColor(ZenithTheme::Colors::text_secondary.getARGB());
    } else if (cpuLoad_ < 0.8f) {
        labelPaint.setColor(SkColorSetRGB(234, 179, 8));  // Yellow
    } else {
        labelPaint.setColor(SkColorSetRGB(239, 68, 68));  // Red
    }
    labelPaint.setAntiAlias(true);
    SkFont smallFont = design::typography::getSkFont(10.0f);
    
    juce::String cpuStr = juce::String(static_cast<int>(cpuLoad_ * 100)) + "%";
    canvas->drawString(cpuStr.toRawUTF8(), x, y + 4, smallFont, labelPaint);
}

//==============================================================================
// Animation
//==============================================================================

void SkiaTransportBar::onAnimationTick(float deltaMs) {
    if (state_ == TransportState::Recording || recordArmed_) {
        recordPulse_ += deltaMs * 0.008f;
        if (recordPulse_ > juce::MathConstants<float>::twoPi) {
            recordPulse_ -= juce::MathConstants<float>::twoPi;
        }
        markDirty();
    }
}

//==============================================================================
// Hit Testing
//==============================================================================

float SkiaTransportBar::getButtonX(int buttonId) const {
    float x = 16;
    for (int i = 0; i < buttonId; ++i) {
        x += kButtonSize + kButtonSpacing;
        if (i == BTN_RECORD) x += 24;  // Extra space after record
    }
    return x;
}

int SkiaTransportBar::hitTestButton(float x, float y) const {
    float btnY = (getHeight() - kButtonSize) / 2;
    
    if (y < btnY || y > btnY + kButtonSize) return -1;
    
    for (int i = 0; i <= BTN_METRONOME; ++i) {
        float btnX = getButtonX(i);
        if (x >= btnX && x < btnX + kButtonSize) {
            return i;
        }
    }
    return -1;
}

bool SkiaTransportBar::hitTestTempo(float x, float y) const {
    juce::ignoreUnused(y);
    return (x >= 280 && x < 350);
}

int SkiaTransportBar::hitTestViewToggle(float x, float y) const {
    float startX = getWidth() - 220;
    float endX = startX + 160;
    float startY = (getHeight() - 28) / 2;
    float endY = startY + 28;
    
    if (x < startX || x > endX || y < startY || y > endY) return -1;
    
    float relX = x - startX - 3;
    float btnW = 52;
    
    if (relX < btnW) return 0;
    if (relX < btnW * 2) return 1;
    return 2;
}

//==============================================================================
// Mouse Handling
//==============================================================================

void SkiaTransportBar::mouseDown(const juce::MouseEvent& e) {
    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);
    
    // Check transport buttons
    int btn = hitTestButton(x, y);
    if (btn >= 0) {
        switch (btn) {
            case BTN_REWIND: if (onRewind) onRewind(); break;
            case BTN_STOP: if (onStop) onStop(); break;
            case BTN_PLAY: if (onPlay) onPlay(); break;
            case BTN_RECORD: if (onRecord) onRecord(); break;
            case BTN_LOOP: if (onLoopToggle) onLoopToggle(); break;
            case BTN_METRONOME: if (onMetronomeToggle) onMetronomeToggle(); break;
        }
        return;
    }
    
    // Check tempo
    if (hitTestTempo(x, y)) {
        isDraggingTempo_ = true;
        tempoDragStartY_ = y;
        tempoDragStartBPM_ = tempo_;
        return;
    }
    
    // Check view toggle
    int view = hitTestViewToggle(x, y);
    if (view >= 0) {
        setActiveView(static_cast<ActiveView>(view));
        return;
    }
}

void SkiaTransportBar::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingTempo_) {
        float deltaY = tempoDragStartY_ - static_cast<float>(e.y);
        float newBPM = tempoDragStartBPM_ + deltaY * 0.5f;
        setTempo(newBPM);
    }
}

void SkiaTransportBar::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    isDraggingTempo_ = false;
}

} // namespace zenith::ui
