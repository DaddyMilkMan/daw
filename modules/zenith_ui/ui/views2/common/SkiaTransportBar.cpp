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

/*
    ==============================================================================
    Original file header:
*/

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

    timeSigNumEditor_ = std::make_unique<juce::Label>();
    timeSigNumEditor_->setEditable(true, true, false);
    timeSigNumEditor_->setJustificationType(juce::Justification::centred);
    timeSigNumEditor_->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    timeSigNumEditor_->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    timeSigNumEditor_->setColour(juce::Label::textColourId, juce::Colours::white);
    timeSigNumEditor_->onTextChange = [this] {
        int value = timeSigNumEditor_->getText().trim().getIntValue();
        int newNum = juce::jlimit(1, 32, value);
        if (newNum != timeSigNum_) {
            setTimeSignature(newNum, timeSigDenom_);
            if (onTimeSignatureChange) onTimeSignatureChange(timeSigNum_, timeSigDenom_);
        }
    };
    timeSigNumEditor_->onEditorHide = [this] { timeSigNumEditor_->setVisible(false); };
    timeSigNumEditor_->setVisible(false);
    addChildComponent(timeSigNumEditor_.get());

    timeSigDenEditor_ = std::make_unique<juce::Label>();
    timeSigDenEditor_->setEditable(true, true, false);
    timeSigDenEditor_->setJustificationType(juce::Justification::centred);
    timeSigDenEditor_->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    timeSigDenEditor_->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    timeSigDenEditor_->setColour(juce::Label::textColourId, juce::Colours::white);
    timeSigDenEditor_->onTextChange = [this] {
        int value = timeSigDenEditor_->getText().trim().getIntValue();
        int newDen = sanitizeDenominator(value);
        if (newDen != timeSigDenom_) {
            setTimeSignature(timeSigNum_, newDen);
            if (onTimeSignatureChange) onTimeSignatureChange(timeSigNum_, timeSigDenom_);
        }
    };
    timeSigDenEditor_->onEditorHide = [this] { timeSigDenEditor_->setVisible(false); };
    timeSigDenEditor_->setVisible(false);
    addChildComponent(timeSigDenEditor_.get());

    updateTimeSigEditorBounds();
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

void SkiaTransportBar::setWingmanActive(bool active) {
    if (wingmanActive_ != active) {
        wingmanActive_ = active;
        markDirty();
    }
}

void SkiaTransportBar::setPosition(double beats) {
    positionBeats_ = beats;
    markDirty();
}

void SkiaTransportBar::setPositionSMPTE(int hours, int mins, int secs, int frames) {
    // Convert SMPTE to beats based on current tempo
    double totalSeconds = hours * 3600.0 + mins * 60.0 + secs + frames / frameRate_;
    double beats = (totalSeconds * tempo_) / 60.0;
    setPosition(beats);
}

void SkiaTransportBar::setTempo(float bpm) {
    bpm = std::clamp(bpm, 20.0f, 999.0f);
    if (tempo_ != bpm) {
        tempo_ = bpm;
        markDirty();
    }
}

void SkiaTransportBar::setTimeSignature(int numerator, int denominator) {
    int newNum = juce::jlimit(1, 32, numerator);
    int newDen = sanitizeDenominator(denominator);
    if (newNum == timeSigNum_ && newDen == timeSigDenom_) {
        return;
    }
    timeSigNum_ = newNum;
    timeSigDenom_ = newDen;
    markDirty();
}

void SkiaTransportBar::setUpdateAvailable(bool available) {
    if (updateAvailable_ != available) {
        updateAvailable_ = available;
        markDirty();
    }
}

void SkiaTransportBar::resized() {
    updateTimeSigEditorBounds();
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
    drawUtilityButtons(canvas);
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
    float x = kTempoX;
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
    x += kTempoSectionGap;
    juce::String timeSigStr = juce::String(timeSigNum_) + "/" + juce::String(timeSigDenom_);
    canvas->drawString(timeSigStr.toRawUTF8(), x, y + 6, tempoFont, tempoPaint);

    // Hover hint outline for time signature edit
    const float rectY = (getHeight() - kControlHeight) / 2.0f;
    SkRect tsRect = SkRect::MakeXYWH(x - 2.0f, rectY, kTimeSigWidth, kControlHeight);
    SkPaint hintPaint;
    hintPaint.setAntiAlias(true);
    hintPaint.setStyle(SkPaint::kStroke_Style);
    hintPaint.setStrokeWidth(1.0f);
    if (timeSigHover_) {
        hintPaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.7f).getARGB());
    } else {
        hintPaint.setColor(ZenithTheme::Colors::border_subtle.withAlpha(0.6f).getARGB());
    }
    canvas->drawRect(tsRect, hintPaint);
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
    constexpr float viewToggleWidth = 160.0f;
    constexpr float cpuTextWidth = 40.0f;
    const float rightControlsWidth =
        kRightPadding + cpuTextWidth + (kButtonSize * 2.0f) + (kButtonSpacing * 2.0f);
    float x = getWidth() - rightControlsWidth - viewToggleWidth - kButtonSpacing;
    float y = (getHeight() - 28) / 2;
    
    // Background pill
    SkRect pillRect = SkRect::MakeXYWH(x, y, viewToggleWidth, 28);
    SkRRect pillRRect = SkRRect::MakeRectXY(pillRect, 14, 14);

    SkPaint pillPaint;
    pillPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRRect(pillRRect, pillPaint);

    // Buttons
    float btnW = 50;
    float btnH = 24;
    float btnY = y + 2;

    const char* labels[] = {"Arr", "Ses"};
    ActiveView views[] = {ActiveView::Arrangement, ActiveView::Session};

    for (int i = 0; i < 2; ++i) {
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

void SkiaTransportBar::drawUtilityButtons(SkCanvas* canvas) {
    constexpr float cpuTextWidth = 40.0f;
    float y = (getHeight() - kButtonSize) / 2;
    float cpuX = getWidth() - kRightPadding - cpuTextWidth;
    float settingsX = cpuX - kButtonSpacing - kButtonSize;
    float wingmanX = settingsX - kButtonSpacing - kButtonSize;

    drawButton(canvas, wingmanX, y, BTN_WINGMAN, "AI", wingmanActive_, true);
    drawButton(canvas, settingsX, y, BTN_SETTINGS, "⚙", false, false);

    if (updateAvailable_) {
        SkPaint dotPaint;
        dotPaint.setAntiAlias(true);
        dotPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
        canvas->drawCircle(settingsX + kButtonSize - 6.0f, y + 6.0f, 4.0f, dotPaint);
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
    constexpr float cpuTextWidth = 40.0f;
    float x = getWidth() - kRightPadding - cpuTextWidth;
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
    return (x >= kTempoX && x < (kTempoX + kTempoWidth));
}

int SkiaTransportBar::hitTestViewToggle(float x, float y) const {
    constexpr float viewToggleWidth = 160.0f;
    constexpr float cpuTextWidth = 40.0f;
    const float rightControlsWidth =
        kRightPadding + cpuTextWidth + (kButtonSize * 2.0f) + (kButtonSpacing * 2.0f);
    float startX = getWidth() - rightControlsWidth - viewToggleWidth - kButtonSpacing;
    float endX = startX + viewToggleWidth;
    float startY = (getHeight() - 28) / 2;
    float endY = startY + 28;

    if (x < startX || x > endX || y < startY || y > endY) return -1;

    float relX = x - startX - 3;
    float btnW = 52;

    if (relX < btnW) return 0;
    if (relX < btnW * 2) return 1;
    return -1;  // No third button
}

int SkiaTransportBar::hitTestTimeSignature(float x, float y) const {
    float timeSigX = kTempoX + kTempoSectionGap;
    float rectY = (getHeight() - kControlHeight) / 2.0f;
    float rectX = timeSigX;
    float rectW = kTimeSigWidth;
    float rectH = kControlHeight;

    if (x < rectX || x > rectX + rectW || y < rectY || y > rectY + rectH) {
        return -1;
    }

    float mid = rectX + rectW / 2.0f;
    return (x < mid) ? 0 : 1; // 0 = numerator, 1 = denominator
}

void SkiaTransportBar::beginTimeSigEdit(bool numerator) {
    updateTimeSigEditorBounds();
    if (numerator) {
        if (!timeSigNumEditor_) return;
        timeSigNumEditor_->setText(juce::String(timeSigNum_), juce::dontSendNotification);
        timeSigNumEditor_->setVisible(true);
        timeSigNumEditor_->toFront(false);
        timeSigNumEditor_->showEditor();
        timeSigNumEditor_->grabKeyboardFocus();
    } else {
        if (!timeSigDenEditor_) return;
        timeSigDenEditor_->setText(juce::String(timeSigDenom_), juce::dontSendNotification);
        timeSigDenEditor_->setVisible(true);
        timeSigDenEditor_->toFront(false);
        timeSigDenEditor_->showEditor();
        timeSigDenEditor_->grabKeyboardFocus();
    }
}

void SkiaTransportBar::endTimeSigEdit() {
    if (timeSigNumEditor_) timeSigNumEditor_->setVisible(false);
    if (timeSigDenEditor_) timeSigDenEditor_->setVisible(false);
    pendingTimeSigEdit_ = -1;
}

void SkiaTransportBar::updateTimeSigEditorBounds() {
    float timeSigX = kTempoX + kTempoSectionGap;
    int y = static_cast<int>((getHeight() - kControlHeight) / 2.0f);
    int w = static_cast<int>(kTimeSigWidth / 2.0f);
    int h = static_cast<int>(kControlHeight);
    if (timeSigNumEditor_) {
        timeSigNumEditor_->setBounds(static_cast<int>(timeSigX), y, w, h);
    }
    if (timeSigDenEditor_) {
        timeSigDenEditor_->setBounds(static_cast<int>(timeSigX + w), y, w, h);
    }
}

int SkiaTransportBar::sanitizeDenominator(int denominator) {
    static constexpr int kDenoms[] = {1, 2, 4, 8, 16, 32, 64};
    constexpr int kDenomCount = static_cast<int>(sizeof(kDenoms) / sizeof(kDenoms[0]));
    if (denominator <= kDenoms[0]) return kDenoms[0];
    if (denominator >= kDenoms[kDenomCount - 1]) return kDenoms[kDenomCount - 1];
    for (int i = 0; i < kDenomCount; ++i) {
        if (kDenoms[i] == denominator) return denominator;
        if (kDenoms[i] > denominator) return kDenoms[i];
    }
    return 4;
}

int SkiaTransportBar::hitTestUtilityButton(float x, float y) const {
    constexpr float cpuTextWidth = 40.0f;
    float btnY = (getHeight() - kButtonSize) / 2;
    if (y < btnY || y > btnY + kButtonSize) return -1;

    float cpuX = getWidth() - kRightPadding - cpuTextWidth;
    float settingsX = cpuX - kButtonSpacing - kButtonSize;
    float wingmanX = settingsX - kButtonSpacing - kButtonSize;

    if (x >= wingmanX && x < wingmanX + kButtonSize) return BTN_WINGMAN;
    if (x >= settingsX && x < settingsX + kButtonSize) return BTN_SETTINGS;
    return -1;
}

//==============================================================================
// Mouse Handling
//==============================================================================

void SkiaTransportBar::mouseDown(const juce::MouseEvent& e) {
    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);

    endTimeSigEdit();

    int newHover = hitTestButton(x, y);
    if (newHover != hoveredButton_) {
        hoveredButton_ = newHover;
        markDirty();
    }
    
    // Check transport buttons
    int btn = hoveredButton_;
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

    // Check time signature
    int timeSigHit = hitTestTimeSignature(x, y);
    if (timeSigHit >= 0) {
        pendingTimeSigEdit_ = timeSigHit;
        timeSigMouseDownPos_ = {x, y};
        isDraggingTimeSig_ = false;
        draggingTimeSigNumerator_ = (timeSigHit == 0);
        timeSigDragStartY_ = y;
        timeSigDragStartValue_ = draggingTimeSigNumerator_ ? timeSigNum_ : timeSigDenom_;
        return;
    }
    
    // Check utility buttons (Wingman/Settings)
    int util = hitTestUtilityButton(x, y);
    if (util == BTN_WINGMAN) {
        if (onWingman) onWingman();
        return;
    }
    if (util == BTN_SETTINGS) {
        if (onSettings) onSettings();
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
        float previous = tempo_;
        setTempo(newBPM);
        if (previous != tempo_ && onTempoChange) {
            onTempoChange(tempo_);
        }
    }
    if (pendingTimeSigEdit_ >= 0) {
        float deltaMove = std::abs(static_cast<float>(e.y) - timeSigMouseDownPos_.y);
        if (!isDraggingTimeSig_ && deltaMove > 3.0f) {
            isDraggingTimeSig_ = true;
        }
    }
    if (isDraggingTimeSig_) {
        float deltaY = timeSigDragStartY_ - static_cast<float>(e.y);
        int deltaSteps = static_cast<int>(deltaY / kTimeSigDragStep);

        if (draggingTimeSigNumerator_) {
            int newNum = juce::jlimit(1, 32, timeSigDragStartValue_ + deltaSteps);
            if (newNum != timeSigNum_) {
                setTimeSignature(newNum, timeSigDenom_);
                if (onTimeSignatureChange) onTimeSignatureChange(timeSigNum_, timeSigDenom_);
            }
        } else {
            static constexpr int kDenoms[] = {1, 2, 4, 8, 16, 32, 64};
            constexpr int kDenomCount = static_cast<int>(sizeof(kDenoms) / sizeof(kDenoms[0]));
            int startIndex = 2; // default to 4
            for (int i = 0; i < kDenomCount; ++i) {
                if (kDenoms[i] == timeSigDragStartValue_) {
                    startIndex = i;
                    break;
                }
            }
            int newIndex = juce::jlimit(0, kDenomCount - 1, startIndex + deltaSteps);
            int newDen = kDenoms[newIndex];
            if (newDen != timeSigDenom_) {
                setTimeSignature(timeSigNum_, newDen);
                if (onTimeSignatureChange) onTimeSignatureChange(timeSigNum_, timeSigDenom_);
            }
        }
    }
}

void SkiaTransportBar::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    isDraggingTempo_ = false;
    if (pendingTimeSigEdit_ >= 0 && !isDraggingTimeSig_) {
        beginTimeSigEdit(pendingTimeSigEdit_ == 0);
    }
    pendingTimeSigEdit_ = -1;
    isDraggingTimeSig_ = false;
}

void SkiaTransportBar::mouseMove(const juce::MouseEvent& e) {
    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);
    bool hover = hitTestTimeSignature(x, y) >= 0;
    if (hover != timeSigHover_) {
        timeSigHover_ = hover;
        markDirty();
    }
}

void SkiaTransportBar::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    if (timeSigHover_) {
        timeSigHover_ = false;
        markDirty();
    }
}

} // namespace zenith::ui
