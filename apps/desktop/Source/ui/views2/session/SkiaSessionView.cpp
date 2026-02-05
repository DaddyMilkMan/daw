/*
  ==============================================================================

    SkiaSessionView.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of clip launcher grid view.

  ==============================================================================
*/

#include "SkiaSessionView.h"
#include "../../design-system/ZenithTheme.h"
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <cmath>

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

SkiaSessionView::SkiaSessionView() {
    setWantsKeyboardFocus(true);
    
    // Start empty - SessionController will populate with real data
}

SkiaSessionView::~SkiaSessionView() = default;

//==============================================================================
// Data Access
//==============================================================================

void SkiaSessionView::setTracks(const std::vector<SessionTrackData>& tracks) {
    tracks_ = tracks;
    markDirty();
}

void SkiaSessionView::setScenes(const std::vector<SceneData>& scenes) {
    scenes_ = scenes;
    markDirty();
}

void SkiaSessionView::setClipState(int trackIndex, int sceneIndex, const ClipSlotData& data) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
        if (sceneIndex >= 0 && sceneIndex < static_cast<int>(tracks_[trackIndex].clips.size())) {
            tracks_[trackIndex].clips[sceneIndex] = data;
            // Invalidate just this slot
            float x = getTrackX(trackIndex);
            float y = getSceneY(sceneIndex);
            markDirtyRect(juce::Rectangle<float>(x, y, kTrackWidth, kClipSlotHeight));
        }
    }
}

void SkiaSessionView::setTrackMeterLevel(int trackIndex, float level) {
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks_.size())) {
        tracks_[trackIndex].meterLevel = level;
        // Invalidate meter area
        float x = getTrackX(trackIndex);
        markDirtyRect(juce::Rectangle<float>(x, getHeight() - kMixerHeight, 
                                              kTrackWidth, kMixerHeight));
    }
}

void SkiaSessionView::setMasterMeterLevel(float left, float right) {
    left = std::clamp(left, 0.0f, 1.0f);
    right = std::clamp(right, 0.0f, 1.0f);
    
    if (std::abs(masterMeterLeft_ - left) > 0.01f || 
        std::abs(masterMeterRight_ - right) > 0.01f) {
        masterMeterLeft_ = left;
        masterMeterRight_ = right;
        // Invalidate master track area
        markDirtyRect(juce::Rectangle<float>(getWidth() - 80, getHeight() - kMixerHeight, 
                                              80, kMixerHeight));
    }
}

//==============================================================================
// Selection
//==============================================================================

void SkiaSessionView::setSelectedSlot(int trackIndex, int sceneIndex) {
    if (selectedSlot_.first != trackIndex || selectedSlot_.second != sceneIndex) {
        // Invalidate old selection
        if (selectedSlot_.first >= 0) {
            float x = getTrackX(selectedSlot_.first);
            float y = getSceneY(selectedSlot_.second);
            markDirtyRect(juce::Rectangle<float>(x, y, kTrackWidth, kClipSlotHeight));
        }
        
        selectedSlot_ = {trackIndex, sceneIndex};
        
        // Invalidate new selection
        if (trackIndex >= 0) {
            float x = getTrackX(trackIndex);
            float y = getSceneY(sceneIndex);
            markDirtyRect(juce::Rectangle<float>(x, y, kTrackWidth, kClipSlotHeight));
        }
    }
}

//==============================================================================
// Scrolling
//==============================================================================

void SkiaSessionView::setScrollPosition(float x, float y) {
    x = std::max(0.0f, x);
    y = std::max(0.0f, y);
    
    if (scrollX_ != x || scrollY_ != y) {
        scrollX_ = x;
        scrollY_ = y;
        markDirty();
    }
}

//==============================================================================
// Layout
//==============================================================================

void SkiaSessionView::resized() {
    // Layout is calculated dynamically based on track/scene counts
}

float SkiaSessionView::getTrackX(int trackIndex) const {
    return kSceneLauncherWidth + trackIndex * kTrackWidth - scrollX_;
}

float SkiaSessionView::getSceneY(int sceneIndex) const {
    return kTrackHeaderHeight + sceneIndex * kClipSlotHeight - scrollY_;
}

int SkiaSessionView::getVisibleTrackCount() const {
    return static_cast<int>((getWidth() - kSceneLauncherWidth) / kTrackWidth) + 2;
}

int SkiaSessionView::getVisibleSceneCount() const {
    return static_cast<int>((getHeight() - kTrackHeaderHeight - kMixerHeight) / kClipSlotHeight) + 2;
}

//==============================================================================
// Drawing
//==============================================================================

void SkiaSessionView::drawSkia(SkCanvas* canvas) {
    SkAutoCanvasRestore acr(canvas, true);
    
    // 1. Background
    drawBackground(canvas);
    
    // 2. Clip grid (main content area, scrolled)
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(
        kSceneLauncherWidth, kTrackHeaderHeight,
        getWidth() - kSceneLauncherWidth, 
        getHeight() - kTrackHeaderHeight - kMixerHeight - kStopRowHeight));
    drawClipGrid(canvas);
    canvas->restore();
    
    // 3. Stop row
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(
        kSceneLauncherWidth, getHeight() - kMixerHeight - kStopRowHeight,
        getWidth() - kSceneLauncherWidth, kStopRowHeight));
    drawStopRow(canvas);
    canvas->restore();
    
    // 4. Scene launcher (left column, scrolled Y only)
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(
        0, kTrackHeaderHeight,
        kSceneLauncherWidth, getHeight() - kTrackHeaderHeight - kMixerHeight));
    drawSceneLauncher(canvas);
    canvas->restore();
    
    // 5. Track headers (top row, scrolled X only)
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(
        kSceneLauncherWidth, 0,
        getWidth() - kSceneLauncherWidth, kTrackHeaderHeight));
    drawTrackHeaders(canvas);
    canvas->restore();
    
    // 6. Mixer strips (bottom, scrolled X only)
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(
        kSceneLauncherWidth, getHeight() - kMixerHeight,
        getWidth() - kSceneLauncherWidth, kMixerHeight));
    drawMixerStrips(canvas);
    canvas->restore();
    
    // 7. Master track (bottom right corner)
    drawMasterTrack(canvas);
    
    // 8. Top-left corner decoration
    SkPaint cornerPaint;
    cornerPaint.setColor(ZenithTheme::Colors::bg_02.getARGB());
    canvas->drawRect(SkRect::MakeXYWH(0, 0, kSceneLauncherWidth, kTrackHeaderHeight), cornerPaint);
    
    // "Session" label
    SkPaint textPaint;
    textPaint.setColor(ZenithTheme::Colors::text_secondary.getARGB());
    textPaint.setAntiAlias(true);
    SkFont font = design::typography::getSkFont(12.0f);
    canvas->drawString("SESSION", 12, 28, font, textPaint);
}

void SkiaSessionView::drawBackground(SkCanvas* canvas) {
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
}

void SkiaSessionView::drawSceneLauncher(SkCanvas* canvas) {
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_02.getARGB());
    canvas->drawRect(SkRect::MakeXYWH(0, 0, kSceneLauncherWidth, getHeight()), bgPaint);
    
    for (size_t i = 0; i < scenes_.size(); ++i) {
        float y = getSceneY(static_cast<int>(i));
        if (y > getHeight() || y + kClipSlotHeight < kTrackHeaderHeight) continue;
        
        SkRect sceneRect = SkRect::MakeXYWH(4, y + 4, kSceneLauncherWidth - 8, kClipSlotHeight - 8);
        SkRRect sceneRRect = SkRRect::MakeRectXY(sceneRect, 8, 8);
        
        // Background
        SkPaint scenePaint;
        bool isPlaying = scenes_[i].isPlaying;
        if (isPlaying) {
            scenePaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.3f).getARGB());
        } else {
            scenePaint.setColor(ZenithTheme::Colors::bg_03.getARGB());
        }
        canvas->drawRRect(sceneRRect, scenePaint);
        
        // Scene name
        SkPaint textPaint;
        textPaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
        textPaint.setAntiAlias(true);
        SkFont font = design::typography::getSkFont(12.0f);
        canvas->drawString(scenes_[i].name.toRawUTF8(), 12, y + 24, font, textPaint);
        
        // Play all button
        float buttonY = y + kClipSlotHeight - 28;
        SkPaint buttonPaint;
        buttonPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
        buttonPaint.setAntiAlias(true);
        
        // Triangle
        SkPath playPath;
        float cx = kSceneLauncherWidth / 2;
        playPath.moveTo(cx - 8, buttonY);
        playPath.lineTo(cx + 8, buttonY + 8);
        playPath.lineTo(cx - 8, buttonY + 16);
        playPath.close();
        canvas->drawPath(playPath, buttonPaint);
    }
    
    // Right border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    canvas->drawLine(kSceneLauncherWidth - 1, 0, kSceneLauncherWidth - 1, getHeight(), borderPaint);
}

void SkiaSessionView::drawTrackHeaders(SkCanvas* canvas) {
    for (size_t t = 0; t < tracks_.size(); ++t) {
        float x = getTrackX(static_cast<int>(t));
        if (x > getWidth() || x + kTrackWidth < kSceneLauncherWidth) continue;
        
        // Background
        SkPaint bgPaint;
        bgPaint.setColor(ZenithTheme::Colors::bg_02.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(x, 0, kTrackWidth, kTrackHeaderHeight), bgPaint);
        
        // Color bar
        SkPaint colorPaint;
        colorPaint.setColor(tracks_[t].color.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(x, 0, kTrackWidth, 3), colorPaint);
        
        // Track name
        SkPaint textPaint;
        textPaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
        textPaint.setAntiAlias(true);
        SkFont font = design::typography::getSkFont(12.0f);
        canvas->drawString(tracks_[t].name.toRawUTF8(), x + 8, 26, font, textPaint);
        
        // Divider
        SkPaint divPaint;
        divPaint.setColor(ZenithTheme::Colors::border_subtle.getARGB());
        canvas->drawLine(x + kTrackWidth - 1, 0, x + kTrackWidth - 1, kTrackHeaderHeight, divPaint);
    }
    
    // Bottom border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    canvas->drawLine(0, kTrackHeaderHeight - 1, getWidth(), kTrackHeaderHeight - 1, borderPaint);
}

void SkiaSessionView::drawClipGrid(SkCanvas* canvas) {
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
            
            drawClipSlot(canvas, x, y, kTrackWidth, kClipSlotHeight,
                         tracks_[t].clips[s], isHovered, isSelected);
        }
    }
}

void SkiaSessionView::drawClipSlot(SkCanvas* canvas, float x, float y,
                                    float w, float h, const ClipSlotData& data,
                                    bool isHovered, bool isSelected) {
    float padding = 4.0f;
    SkRect slotRect = SkRect::MakeXYWH(x + padding, y + padding, 
                                        w - padding * 2, h - padding * 2);
    SkRRect slotRRect = SkRRect::MakeRectXY(slotRect, 8, 8);
    
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    
    float cx = x + w / 2;
    float cy = y + h / 2 - 8;
    
    switch (data.state) {
        case ClipSlotState::Empty: {
            // Empty slot
            bgPaint.setColor(ZenithTheme::Colors::bg_02.getARGB());
            canvas->drawRRect(slotRRect, bgPaint);
            
            // Empty circle indicator
            SkPaint circlePaint;
            circlePaint.setStyle(SkPaint::kStroke_Style);
            circlePaint.setStrokeWidth(1.5f);
            circlePaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
            circlePaint.setAntiAlias(true);
            canvas->drawCircle(cx, cy, 10, circlePaint);
            break;
        }
        
        case ClipSlotState::Stopped: {
            // Clip loaded but stopped
            bgPaint.setColor(data.color.withAlpha(0.7f).getARGB());
            canvas->drawRRect(slotRRect, bgPaint);
            
            // Play triangle
            SkPaint playPaint;
            playPaint.setColor(SK_ColorWHITE);
            playPaint.setAntiAlias(true);
            
            SkPath playPath;
            playPath.moveTo(cx - 6, cy - 8);
            playPath.lineTo(cx + 8, cy);
            playPath.lineTo(cx - 6, cy + 8);
            playPath.close();
            canvas->drawPath(playPath, playPaint);
            
            // Clip name
            SkPaint textPaint;
            textPaint.setColor(SK_ColorWHITE);
            textPaint.setAntiAlias(true);
            SkFont font = design::typography::getSkFont(10.0f);
            
            canvas->save();
            canvas->clipRRect(slotRRect);
            canvas->drawString(data.name.toRawUTF8(), x + 10, y + h - 12, font, textPaint);
            canvas->restore();
            break;
        }
        
        case ClipSlotState::Playing: {
            // Playing clip with glow
            bgPaint.setColor(data.color.withAlpha(0.85f).getARGB());
            canvas->drawRRect(slotRRect, bgPaint);
            
            // Animated glow border
            float glowIntensity = 0.5f + 0.3f * std::sin(animPhase_ * 2.0f);
            SkPaint glowPaint;
            glowPaint.setColor(SkColorSetARGB(static_cast<int>(glowIntensity * 150), 255, 255, 255));
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
            canvas->drawRRect(slotRRect, glowPaint);
            
            // Playing indicator (filled circle with progress)
            drawPlayingIndicator(canvas, cx, cy, 10, data.playProgress);
            
            // White border
            SkPaint borderPaint;
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(2.0f);
            borderPaint.setColor(SkColorSetARGB(180, 255, 255, 255));
            canvas->drawRRect(slotRRect, borderPaint);
            
            // Clip name
            SkPaint textPaint;
            textPaint.setColor(SK_ColorWHITE);
            textPaint.setAntiAlias(true);
            SkFont font = design::typography::getSkFont(10.0f);
            canvas->save();
            canvas->clipRRect(slotRRect);
            canvas->drawString(data.name.toRawUTF8(), x + 10, y + h - 12, font, textPaint);
            canvas->restore();
            break;
        }
        
        case ClipSlotState::Queued: {
            // Queued - blinking
            float blink = std::fmod(animPhase_ * 3.0f, juce::MathConstants<float>::twoPi);
            float alpha = (std::sin(blink) > 0) ? 0.85f : 0.5f;
            
            bgPaint.setColor(data.color.withAlpha(alpha).getARGB());
            canvas->drawRRect(slotRRect, bgPaint);
            
            // Queued ring indicator
            drawQueuedIndicator(canvas, cx, cy, 10);
            
            // Clip name
            SkPaint textPaint;
            textPaint.setColor(SK_ColorWHITE);
            textPaint.setAntiAlias(true);
            SkFont font = design::typography::getSkFont(10.0f);
            canvas->save();
            canvas->clipRRect(slotRRect);
            canvas->drawString(data.name.toRawUTF8(), x + 10, y + h - 12, font, textPaint);
            canvas->restore();
            break;
        }
        
        case ClipSlotState::Recording: {
            // Recording - red with pulse
            float pulse = 0.7f + 0.3f * std::sin(animPhase_ * 4.0f);
            
            bgPaint.setColor(ZenithTheme::Colors::error.withAlpha(pulse).getARGB());
            canvas->drawRRect(slotRRect, bgPaint);
            
            // Red glow
            SkPaint glowPaint;
            glowPaint.setColor(ZenithTheme::Colors::error.withAlpha(0.5f * pulse).getARGB());
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
            canvas->drawRRect(slotRRect, glowPaint);
            
            // Record circle
            drawRecordingIndicator(canvas, cx, cy, 10);
            break;
        }
        
        case ClipSlotState::Stopping: {
            // Stopping - fading
            float fade = 0.5f + 0.3f * std::sin(animPhase_ * 3.0f);
            bgPaint.setColor(data.color.withAlpha(fade).getARGB());
            canvas->drawRRect(slotRRect, bgPaint);
            break;
        }
    }
    
    // Selection ring
    if (isSelected) {
        SkPaint selPaint;
        selPaint.setStyle(SkPaint::kStroke_Style);
        selPaint.setStrokeWidth(2.0f);
        selPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
        canvas->drawRRect(slotRRect, selPaint);
    }
    
    // Hover effect
    if (isHovered && !isSelected) {
        SkPaint hoverPaint;
        hoverPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
        canvas->drawRRect(slotRRect, hoverPaint);
    }
}

void SkiaSessionView::drawPlayingIndicator(SkCanvas* canvas, float cx, float cy,
                                            float radius, float progress) {
    // Background circle
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
    bgPaint.setAntiAlias(true);
    canvas->drawCircle(cx, cy, radius, bgPaint);
    
    // Progress arc
    SkPaint arcPaint;
    arcPaint.setStyle(SkPaint::kStroke_Style);
    arcPaint.setStrokeWidth(3.0f);
    arcPaint.setColor(SK_ColorWHITE);
    arcPaint.setAntiAlias(true);
    arcPaint.setStrokeCap(SkPaint::kRound_Cap);
    
    SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);
    float sweepAngle = progress * 360.0f;
    canvas->drawArc(arcRect, -90, sweepAngle, false, arcPaint);
    
    // Center filled circle
    SkPaint centerPaint;
    centerPaint.setColor(SK_ColorWHITE);
    centerPaint.setAntiAlias(true);
    canvas->drawCircle(cx, cy, radius - 4, centerPaint);
}

void SkiaSessionView::drawQueuedIndicator(SkCanvas* canvas, float cx, float cy, float radius) {
    // Pulsing ring
    float scale = 1.0f + 0.1f * std::sin(animPhase_ * 4.0f);
    
    SkPaint ringPaint;
    ringPaint.setStyle(SkPaint::kStroke_Style);
    ringPaint.setStrokeWidth(3.0f);
    ringPaint.setColor(SK_ColorWHITE);
    ringPaint.setAntiAlias(true);
    canvas->drawCircle(cx, cy, radius * scale, ringPaint);
    
    // Inner dot
    SkPaint dotPaint;
    dotPaint.setColor(SK_ColorWHITE);
    dotPaint.setAntiAlias(true);
    canvas->drawCircle(cx, cy, 3, dotPaint);
}

void SkiaSessionView::drawRecordingIndicator(SkCanvas* canvas, float cx, float cy, float radius) {
    // White filled circle
    SkPaint recPaint;
    recPaint.setColor(SK_ColorWHITE);
    recPaint.setAntiAlias(true);
    canvas->drawCircle(cx, cy, radius, recPaint);
}

void SkiaSessionView::drawStopRow(SkCanvas* canvas) {
    float y = getHeight() - kMixerHeight - kStopRowHeight;
    
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_01.getARGB());
    canvas->drawRect(SkRect::MakeXYWH(kSceneLauncherWidth, y, getWidth(), kStopRowHeight), bgPaint);
    
    for (size_t t = 0; t < tracks_.size(); ++t) {
        float x = getTrackX(static_cast<int>(t));
        if (x > getWidth() || x + kTrackWidth < kSceneLauncherWidth) continue;
        
        // Stop button
        float btnX = x + kTrackWidth / 2;
        float btnY = y + kStopRowHeight / 2;
        
        SkPaint stopPaint;
        stopPaint.setStyle(SkPaint::kStroke_Style);
        stopPaint.setStrokeWidth(2.0f);
        stopPaint.setColor(ZenithTheme::Colors::text_secondary.getARGB());
        stopPaint.setAntiAlias(true);
        
        SkRect stopRect = SkRect::MakeXYWH(btnX - 8, btnY - 8, 16, 16);
        canvas->drawRect(stopRect, stopPaint);
    }
    
    // Top border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_subtle.getARGB());
    canvas->drawLine(kSceneLauncherWidth, y, getWidth(), y, borderPaint);
}

void SkiaSessionView::drawMixerStrips(SkCanvas* canvas) {
    float y = getHeight() - kMixerHeight;
    
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_02.getARGB());
    canvas->drawRect(SkRect::MakeXYWH(kSceneLauncherWidth, y, getWidth(), kMixerHeight), bgPaint);
    
    for (size_t t = 0; t < tracks_.size(); ++t) {
        float x = getTrackX(static_cast<int>(t));
        if (x > getWidth() || x + kTrackWidth < kSceneLauncherWidth) continue;
        
        // S/M buttons
        float btnY = y + 8;
        float btnSize = 24;
        
        // Solo
        SkPaint soloPaint;
        soloPaint.setStyle(tracks_[t].isSolo ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
        soloPaint.setStrokeWidth(1.5f);
        soloPaint.setColor(SkColorSetRGB(234, 179, 8));  // Yellow
        soloPaint.setAntiAlias(true);
        
        SkRect soloRect = SkRect::MakeXYWH(x + 8, btnY, btnSize, btnSize);
        canvas->drawRoundRect(soloRect, 4, 4, soloPaint);
        
        SkPaint soloTextPaint;
        soloTextPaint.setColor(tracks_[t].isSolo ? SK_ColorBLACK : SkColorSetRGB(234, 179, 8));
        soloTextPaint.setAntiAlias(true);
        SkFont smallFont = design::typography::getSkFont(10.0f);
        canvas->drawString("S", x + 15, btnY + 16, smallFont, soloTextPaint);
        
        // Mute
        SkPaint mutePaint;
        mutePaint.setStyle(tracks_[t].isMuted ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
        mutePaint.setStrokeWidth(1.5f);
        mutePaint.setColor(SkColorSetRGB(239, 68, 68));  // Red
        mutePaint.setAntiAlias(true);
        
        SkRect muteRect = SkRect::MakeXYWH(x + 36, btnY, btnSize, btnSize);
        canvas->drawRoundRect(muteRect, 4, 4, mutePaint);
        
        SkPaint muteTextPaint;
        muteTextPaint.setColor(tracks_[t].isMuted ? SK_ColorWHITE : SkColorSetRGB(239, 68, 68));
        muteTextPaint.setAntiAlias(true);
        canvas->drawString("M", x + 43, btnY + 16, smallFont, muteTextPaint);
        
        // Volume fader
        float faderX = x + 70;
        float faderY = y + 8;
        float faderW = 8;
        float faderH = kMixerHeight - 16;
        
        // Fader track
        SkPaint trackPaint;
        trackPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(faderX, faderY, faderW, faderH), trackPaint);
        
        // Fader position
        float faderLevel = tracks_[t].volume;
        float faderFillH = faderH * faderLevel;
        
        SkPaint fillPaint;
        fillPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(faderX, faderY + faderH - faderFillH, faderW, faderFillH), fillPaint);
        
        // Meter
        float meterX = x + 84;
        float meterLevel = tracks_[t].meterLevel;
        float meterFillH = faderH * meterLevel;
        
        // Meter background
        SkPaint meterBgPaint;
        meterBgPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(meterX, faderY, 4, faderH), meterBgPaint);
        
        // Meter fill (gradient: green -> yellow -> red)
        if (meterLevel > 0) {
            SkColor meterColor;
            if (meterLevel < 0.7f) {
                meterColor = SkColorSetRGB(34, 197, 94);  // Green
            } else if (meterLevel < 0.9f) {
                meterColor = SkColorSetRGB(234, 179, 8);   // Yellow
            } else {
                meterColor = SkColorSetRGB(239, 68, 68);  // Red
            }
            
            SkPaint meterPaint;
            meterPaint.setColor(meterColor);
            canvas->drawRect(SkRect::MakeXYWH(meterX, faderY + faderH - meterFillH, 4, meterFillH), meterPaint);
        }
        
        // dB label
        float dbValue = 20.0f * std::log10(std::max(0.001f, tracks_[t].volume));
        juce::String dbStr = juce::String(dbValue, 1) + " dB";
        
        SkPaint dbPaint;
        dbPaint.setColor(ZenithTheme::Colors::text_tertiary.getARGB());
        dbPaint.setAntiAlias(true);
        canvas->drawString(dbStr.toRawUTF8(), x + 100, y + kMixerHeight - 8, smallFont, dbPaint);
        
        // Divider
        SkPaint divPaint;
        divPaint.setColor(ZenithTheme::Colors::border_subtle.getARGB());
        canvas->drawLine(x + kTrackWidth - 1, y, x + kTrackWidth - 1, y + kMixerHeight, divPaint);
    }
    
    // Top border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    canvas->drawLine(kSceneLauncherWidth, y, getWidth(), y, borderPaint);
}

void SkiaSessionView::drawMasterTrack(SkCanvas* canvas) {
    // Master track is always visible in bottom-right corner
    float x = getWidth() - 80;
    float y = getHeight() - kMixerHeight;
    float w = 80;
    float h = kMixerHeight;
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(ZenithTheme::Colors::bg_03.getARGB());
    canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), bgPaint);
    
    // "Master" label
    SkPaint textPaint;
    textPaint.setColor(ZenithTheme::Colors::text_primary.getARGB());
    textPaint.setAntiAlias(true);
    SkFont font = design::typography::getSkFont(10.0f);
    canvas->drawString("MASTER", x + 8, y + 16, font, textPaint);
    
    // Master meter (stereo)
    float meterX = x + 20;
    float meterY = y + 24;
    float meterW = 16;
    float meterH = h - 32;
    
    SkPaint meterBgPaint;
    meterBgPaint.setColor(ZenithTheme::Colors::bg_00.getARGB());
    canvas->drawRect(SkRect::MakeXYWH(meterX, meterY, meterW, meterH), meterBgPaint);
    
    // Real master levels (updated via setMasterMeterLevel)
    float masterL = masterMeterLeft_;
    float masterR = masterMeterRight_;
    
    SkPaint meterPaint;
    meterPaint.setColor(SkColorSetRGB(34, 197, 94));
    
    // Left channel
    float lH = meterH * masterL;
    canvas->drawRect(SkRect::MakeXYWH(meterX, meterY + meterH - lH, 7, lH), meterPaint);
    
    // Right channel
    float rH = meterH * masterR;
    canvas->drawRect(SkRect::MakeXYWH(meterX + 9, meterY + meterH - rH, 7, rH), meterPaint);
    
    // Left border
    SkPaint borderPaint;
    borderPaint.setColor(ZenithTheme::Colors::border_default.getARGB());
    canvas->drawLine(x, y, x, y + h, borderPaint);
}

//==============================================================================
// Animation
//==============================================================================

void SkiaSessionView::onAnimationTick(float deltaMs) {
    animPhase_ += deltaMs * 0.003f;
    if (animPhase_ > juce::MathConstants<float>::twoPi * 10) {
        animPhase_ -= juce::MathConstants<float>::twoPi * 10;
    }
    
    // Check if any clips are animating
    bool needsRedraw = false;
    for (const auto& track : tracks_) {
        for (const auto& clip : track.clips) {
            if (clip.state == ClipSlotState::Playing ||
                clip.state == ClipSlotState::Queued ||
                clip.state == ClipSlotState::Recording) {
                needsRedraw = true;
                break;
            }
        }
        if (needsRedraw) break;
    }
    
    if (needsRedraw) {
        markDirty();
    }
}

//==============================================================================
// Hit Testing
//==============================================================================

std::pair<int, int> SkiaSessionView::hitTestSlot(float x, float y) const {
    if (x < kSceneLauncherWidth || y < kTrackHeaderHeight) {
        return {-1, -1};
    }
    if (y > getHeight() - kMixerHeight - kStopRowHeight) {
        return {-1, -1};
    }
    
    int trackIdx = static_cast<int>((x - kSceneLauncherWidth + scrollX_) / kTrackWidth);
    int sceneIdx = static_cast<int>((y - kTrackHeaderHeight + scrollY_) / kClipSlotHeight);
    
    if (trackIdx < 0 || trackIdx >= static_cast<int>(tracks_.size())) return {-1, -1};
    if (sceneIdx < 0 || sceneIdx >= static_cast<int>(scenes_.size())) return {-1, -1};
    
    return {trackIdx, sceneIdx};
}

int SkiaSessionView::hitTestScene(float y) const {
    if (y < kTrackHeaderHeight || y > getHeight() - kMixerHeight) return -1;
    return static_cast<int>((y - kTrackHeaderHeight + scrollY_) / kClipSlotHeight);
}

int SkiaSessionView::hitTestTrack(float x) const {
    if (x < kSceneLauncherWidth) return -1;
    return static_cast<int>((x - kSceneLauncherWidth + scrollX_) / kTrackWidth);
}

//==============================================================================
// Mouse Handling
//==============================================================================

void SkiaSessionView::mouseDown(const juce::MouseEvent& e) {
    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);

    // Check scene launcher column (for scene launch)
    int sceneIdx = hitTestScene(y);
    if (sceneIdx >= 0) {
        setSelectedSlot(-1, sceneIdx);
        if (onSceneClicked) {
            onSceneClicked(sceneIdx);
        }
        return;
    }

    // Check stop row
    int stopTrackIdx = hitTestStopButton(x, y);
    if (stopTrackIdx >= 0) {
        if (onTrackStopClicked) {
            onTrackStopClicked(stopTrackIdx);
        }
        return;
    }

    // Check clip slots
    auto slot = hitTestSlot(x, y);
    if (slot.first >= 0) {
        setSelectedSlot(slot.first, slot.second);

        // Trigger clip launch callback
        if (onClipSlotClicked) {
            onClipSlotClicked(slot.first, slot.second);
        }
        return;
    }
}

void SkiaSessionView::mouseDoubleClick(const juce::MouseEvent& e) {
    float x = static_cast<float>(e.x);
    float y = static_cast<float>(e.y);

    // Check clip slots for double-click
    auto slot = hitTestSlot(x, y);
    if (slot.first >= 0) {
        if (onClipSlotDoubleClicked) {
            onClipSlotDoubleClicked(slot.first, slot.second);
        }
        return;
    }
}

int SkiaSessionView::hitTestStopButton(float x, float y) const {
    float stopRowY = getHeight() - kMixerHeight - kStopRowHeight;

    // Check if y is in stop row
    if (y < stopRowY || y > stopRowY + kStopRowHeight) {
        return -1;
    }

    // Check if x is in track area (excluding scene launcher)
    if (x < kSceneLauncherWidth) {
        return -1;
    }

    // Find which track's stop button
    int trackIdx = static_cast<int>((x - kSceneLauncherWidth + scrollX_) / kTrackWidth);
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks_.size())) {
        return trackIdx;
    }

    return -1;
}

void SkiaSessionView::mouseWheelMove(const juce::MouseEvent& e,
                                      const juce::MouseWheelDetails& wheel) {
    juce::ignoreUnused(e);
    
    float scrollSpeed = 30.0f;
    setScrollPosition(scrollX_ - wheel.deltaX * scrollSpeed,
                      scrollY_ - wheel.deltaY * scrollSpeed);
}

} // namespace zenith::ui
