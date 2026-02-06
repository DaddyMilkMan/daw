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

#include "SkiaSessionView.h"
#include "../../design-system/ZenithTheme.h"
#include "../ViewTheme.h"
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#include <cmath>

namespace zenith::ui {

//==============================================================================
// Construction
//==============================================================================

SkiaSessionView::SkiaSessionView()
    : layout_(zenith::ui::ViewTheme::getLayout())
    , colors_(zenith::ui::ViewTheme::getColors())
    , animation_(zenith::ui::ViewTheme::getAnimation())
    , errorHandling_(zenith::ui::ViewTheme::getErrorHandling()) {

    setWantsKeyboardFocus(true);

    // Initialize theme system
    initializeThemeSystem();

    // Start empty - SessionController will populate with real data
}

void SkiaSessionView::initializeThemeSystem() {
    // Set up error handling
    if (errorHandling_.logErrors) {
        zenith::ui::ViewTheme::getThemeManager().addThemeChangeListener(
            [this](const zenith::ui::ViewTheme::Theme& theme) {
                if (!theme.isValid()) {
                    zenith::ui::ViewTheme::getThemeManager().reportError(
                        "SkiaSessionView", "Invalid theme configuration applied");
                }
            });
    }

    // Set initial theme
    try {
        zenith::ui::ViewTheme::getThemeManager().applyBuiltInTheme(
            zenith::ui::ViewTheme::Theme::BuiltIn::NeonNoir);
    } catch (const std::exception& e) {
        // Fallback to default theme if initialization fails
        zenith::ui::ViewTheme::getThemeManager().reportError(
            "SkiaSessionView", std::string("Theme initialization failed: ") + e.what());
    }
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
    return layout_.sceneLauncherWidth + trackIndex * layout_.trackWidth - scrollX_;
}

float SkiaSessionView::getSceneY(int sceneIndex) const {
    return layout_.trackHeaderHeight + sceneIndex * layout_.clipSlotHeight - scrollY_;
}

int SkiaSessionView::getVisibleTrackCount() const {
    return static_cast<int>((getWidth() - layout_.sceneLauncherWidth) / layout_.trackWidth) + 2;
}

int SkiaSessionView::getVisibleSceneCount() const {
    return static_cast<int>((getHeight() - layout_.trackHeaderHeight - layout_.mixerHeight) / layout_.clipSlotHeight) + 2;
}

//==============================================================================
// Drawing
//==============================================================================

void SkiaSessionView::drawSkia(SkCanvas* canvas) {
    try {
        SkAutoCanvasRestore acr(canvas, true);

        // 1. Background
        drawBackground(canvas);

        // 2. Clip grid (main content area, scrolled)
        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(
            layout_.sceneLauncherWidth, layout_.trackHeaderHeight,
            getWidth() - layout_.sceneLauncherWidth,
            getHeight() - layout_.trackHeaderHeight - layout_.mixerHeight - layout_.stopRowHeight));
        drawClipGrid(canvas);
        canvas->restore();

        // 3. Stop row
        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(
            layout_.sceneLauncherWidth, getHeight() - layout_.mixerHeight - layout_.stopRowHeight,
            getWidth() - layout_.sceneLauncherWidth, layout_.stopRowHeight));
        drawStopRow(canvas);
        canvas->restore();

        // 4. Scene launcher (left column, scrolled Y only)
        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(
            0, layout_.trackHeaderHeight,
            layout_.sceneLauncherWidth, getHeight() - layout_.trackHeaderHeight - layout_.mixerHeight));
        drawSceneLauncher(canvas);
        canvas->restore();

        // 5. Track headers (top row, scrolled X only)
        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(
            layout_.sceneLauncherWidth, 0,
            getWidth() - layout_.sceneLauncherWidth, layout_.trackHeaderHeight));
        drawTrackHeaders(canvas);
        canvas->restore();

        // 6. Mixer strips (bottom, scrolled X only)
        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(
            layout_.sceneLauncherWidth, getHeight() - layout_.mixerHeight,
            getWidth() - layout_.sceneLauncherWidth, layout_.mixerHeight));
        drawMixerStrips(canvas);
        canvas->restore();

        // 7. Master track (bottom right corner)
        drawMasterTrack(canvas);

        // 8. Top-left corner decoration with theme colors
        SkPaint cornerPaint;
        cornerPaint.setColor(colors_.sceneLauncherBg.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(0, 0, layout_.sceneLauncherWidth, layout_.trackHeaderHeight), cornerPaint);

        // "Session" label with theme typography
        SkPaint textPaint;
        textPaint.setColor(colors_.sceneText.getARGB());
        textPaint.setAntiAlias(true);
        SkFont font = design::typography::getSkFont(12.0f);
        canvas->drawString("SESSION", layout_.scenePadding + 2, layout_.trackHeaderHeight/2 + 4, font, textPaint);

    } catch (const std::exception& e) {
        handleError("drawSkia", e.what());
        drawFallbackBackground(canvas);
    }
}

void SkiaSessionView::drawBackground(SkCanvas* canvas) {
    try {
        // Create gradient background from theme
        SkPoint points[2] = {SkPoint::Make(0, 0), SkPoint::Make(0, getHeight())};
        SkColor colors[2] = {
            colors_.backgroundGradientStart.getARGB(),
            colors_.backgroundGradientEnd.getARGB()
        };

        SkGradientShader* gradient = SkGradientShader::CreateLinear(
            points, colors, nullptr, 2, SkShader::kClamp_TileMode);

        SkPaint bgPaint;
        bgPaint.setShader(gradient);
        canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);

        // Add noise texture for depth if enabled
        if (colors_.backgroundNoise.getAlpha() > 0) {
            SkBitmap noise;
            noise.allocN32Pixels(getWidth(), getHeight());

            uint8_t noiseAlpha = static_cast<uint8_t>(colors_.backgroundNoise.getAlpha());
            for (int y = 0; y < getHeight(); y += 2) {
                for (int x = 0; x < getWidth(); x += 2) {
                    uint8_t noiseValue = (x * y) % noiseAlpha + (noiseAlpha / 3);
                    SkColor noiseColor = SkColorSetARGB(noiseValue, 255, 255, 255);
                    noise.setPixel(x, y, noiseColor);
                }
            }

            SkPaint noisePaint;
            noisePaint.setAlpha(static_cast<uint8_t>(noiseAlpha * 0.6f));
            noisePaint.setShader(SkShader::CreateBitmapShader(noise, SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode));
            canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), noisePaint);
        }

        gradient->unref();

    } catch (const std::exception& e) {
        handleError("drawBackground", e.what());
        drawFallbackBackground(canvas);
    }
}

void SkiaSessionView::drawFallbackBackground(SkCanvas* canvas) {
    // Simple fallback background when theme system fails
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(255, 20, 20, 25));
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
}

void SkiaSessionView::drawSceneLauncher(SkCanvas* canvas) {
    try {
        // Theme-based gradient background
        SkPoint points[2] = {SkPoint::Make(0, 0), SkPoint::Make(0, getHeight())};
        SkColor colors[2] = {
            colors_.sceneLauncherBg.getARGB(),
            colors_.sceneLauncherBg.darker(0.1f).getARGB()
        };

        SkGradientShader* gradient = SkGradientShader::CreateLinear(
            points, colors, nullptr, 2, SkShader::kClamp_TileMode);

        SkPaint bgPaint;
        bgPaint.setShader(gradient);
        canvas->drawRect(SkRect::MakeXYWH(0, 0, layout_.sceneLauncherWidth, getHeight()), bgPaint);
        gradient->unref();

    for (size_t i = 0; i < scenes_.size(); ++i) {
        float y = getSceneY(static_cast<int>(i));
        if (y > getHeight() || y + layout_.clipSlotHeight < layout_.trackHeaderHeight) continue;

        float padding = layout_.scenePadding;
        SkRect sceneRect = SkRect::MakeXYWH(padding, y + padding,
                                            layout_.sceneLauncherWidth - padding * 2, layout_.clipSlotHeight - padding * 2);
        SkRRect sceneRRect = SkRRect::MakeRectXY(sceneRect, layout_.sceneRadius, layout_.sceneRadius);

        // Dynamic background with playing state using theme colors
        SkPaint scenePaint;
        scenePaint.setAntiAlias(true);

        if (scenes_[i].isPlaying) {
            // Animated gradient for playing scene
            float pulse = 0.3f + 0.2f * std::sin(animPhase_ * animation_.pulseSpeed);
            SkColor playingColors[2] = {
                colors_.accentButtonColor.withAlpha(0.4f + pulse * 0.2f).getARGB(),
                colors_.accentButtonColor.withAlpha(0.2f).getARGB()
            };

            SkGradientShader* playingGradient = SkGradientShader::CreateLinear(
                points, playingColors, nullptr, 2, SkShader::kClamp_TileMode);
            scenePaint.setShader(playingGradient);
            canvas->drawRRect(sceneRRect, scenePaint);
            playingGradient->unref();
        } else {
            // Theme-based gradient for regular scene
            SkColor bgColors[2] = {
                ZenithTheme::Colors::bg_03.getARGB(),
                ZenithTheme::Colors::bg_03.darker(0.1f).getARGB()
            };

            SkGradientShader* bgGradient = SkGradientShader::CreateLinear(
                points, bgColors, nullptr, 2, SkShader::kClamp_TileMode);
            scenePaint.setShader(bgGradient);
            canvas->drawRRect(sceneRRect, scenePaint);
            bgGradient->unref();
        }

        // Scene name with theme typography
        SkPaint textPaint;
        textPaint.setColor(colors_.sceneText.getARGB());
        textPaint.setAntiAlias(true);
        textPaint.setSubpixelText(true);
        SkFont font = design::typography::getSkFont(12.0f);
        font.setEdging(SkFont::Edging::kAntiAlias);

        // Text shadow with theme colors
        SkPaint shadowPaint;
        shadowPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
        shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 1.0f));
        canvas->save();
        canvas->translate(1, 1);
        canvas->drawString(scenes_[i].name.toRawUTF8(), layout_.scenePadding + 2, y + layout_.trackHeaderHeight/2 + 4, font, shadowPaint);
        canvas->restore();

        canvas->drawString(scenes_[i].name.toRawUTF8(), layout_.scenePadding + 2, y + layout_.trackHeaderHeight/2 + 4, font, textPaint);

        // Enhanced play all button
        float buttonY = y + kClipSlotHeight - 32;
        float buttonCx = kSceneLauncherWidth / 2;

        // Button background
        SkRect buttonRect = SkRect::MakeXYWH(buttonCx - 12, buttonY, 24, 24);
        SkRRect buttonRRect = SkRRect::MakeRectXY(buttonRect, 12, 12);

        SkPaint buttonBg;
        buttonBg.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.2f).getARGB());
        canvas->drawRRect(buttonRRect, buttonBg);

        // Play triangle with glow
        SkPaint glowPaint;
        glowPaint.setColor(ZenithTheme::Colors::accent_primary.withAlpha(0.4f).getARGB());
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
        canvas->drawPath(playTrianglePath(buttonCx, buttonY + 12, 8), glowPaint);

        SkPaint buttonPaint;
        buttonPaint.setColor(ZenithTheme::Colors::accent_primary.getARGB());
        canvas->drawPath(playTrianglePath(buttonCx, buttonY + 12, 8), buttonPaint);
    }

    // Enhanced right border with gradient
    SkPoint borderPoints[2] = {SkPoint::Make(kSceneLauncherWidth - 1, 0),
                               SkPoint::Make(kSceneLauncherWidth - 1, getHeight())};
    SkColor borderColors[2] = {
        ZenithTheme::Colors::border_default.getARGB(),
        ZenithTheme::Colors::border_subtle.getARGB()
    };

    SkGradientShader* borderGradient = SkGradientShader::CreateLinear(
        borderPoints, borderColors, nullptr, 2, SkShader::kClamp_TileMode);

    SkPaint borderPaint;
    borderPaint.setShader(borderGradient);
    canvas->drawLine(kSceneLauncherWidth - 1, 0, kSceneLauncherWidth - 1, getHeight(), borderPaint);
    borderGradient->unref();
}

void SkiaSessionView::drawTrackHeaders(SkCanvas* canvas) {
    try {
        for (size_t t = 0; t < tracks_.size(); ++t) {
            float x = getTrackX(static_cast<int>(t));
            if (x > getWidth() || x + layout_.trackWidth < layout_.sceneLauncherWidth) continue;

            // Background with theme colors
            SkPaint bgPaint;
            bgPaint.setColor(colors_.trackHeaderBg.getARGB());
            canvas->drawRect(SkRect::MakeXYWH(x, 0, layout_.trackWidth, layout_.trackHeaderHeight), bgPaint);

            // Color bar with theme colors
            SkPaint colorPaint;
            colorPaint.setColor(tracks_[t].color.getARGB());
            canvas->drawRect(SkRect::MakeXYWH(x, 0, layout_.trackWidth, 3), colorPaint);

            // Track name with theme typography
            SkPaint textPaint;
            textPaint.setColor(colors_.trackHeaderText.getARGB());
            textPaint.setAntiAlias(true);
            SkFont font = design::typography::getSkFont(12.0f);
            canvas->drawString(tracks_[t].name.toRawUTF8(), x + layout_.headerPadding, layout_.trackHeaderHeight/2 + 4, font, textPaint);

            // Divider with theme colors
            SkPaint divPaint;
            divPaint.setColor(colors_.trackDividerBorder.getARGB());
            canvas->drawLine(x + layout_.trackWidth - 1, 0, x + layout_.trackWidth - 1, layout_.trackHeaderHeight, divPaint);
        }

        // Bottom border with theme colors
        SkPaint borderPaint;
        borderPaint.setColor(colors_.sectionDividerBorder.getARGB());
        canvas->drawLine(0, layout_.trackHeaderHeight - 1, getWidth(), layout_.trackHeaderHeight - 1, borderPaint);

    } catch (const std::exception& e) {
        handleError("drawTrackHeaders", e.what());
    }
}

void SkiaSessionView::drawClipGrid(SkCanvas* canvas) {
    try {
        for (size_t t = 0; t < tracks_.size(); ++t) {
            float x = getTrackX(static_cast<int>(t));
            if (x > getWidth() || x + layout_.trackWidth < layout_.sceneLauncherWidth) continue;
        
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
    float padding = layout_.clipPadding;
    SkRect slotRect = SkRect::MakeXYWH(x + padding, y + padding,
                                        w - padding * 2, h - padding * 2);
    SkRRect slotRRect = SkRRect::MakeRectXY(slotRect, layout_.clipSlotRadius, layout_.clipSlotRadius);

    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    float cx = x + w / 2;
    float cy = y + h / 2 - 8;
    
    switch (data.state) {
        case ClipSlotState::Empty: {
            // Empty slot with theme gradient
            SkPoint points[2] = {SkPoint::Make(x, y), SkPoint::Make(x, y + h)};
            SkColor colors[2] = {
                colors_.emptySlotColor.getARGB(),
                colors_.emptySlotColor.darker(0.1f).getARGB()
            };

            SkGradientShader* gradient = SkGradientShader::CreateLinear(
                points, colors, nullptr, 2, SkShader::kClamp_TileMode);

            bgPaint.setShader(gradient);
            canvas->drawRRect(slotRRect, bgPaint);

            // Gradient shader no longer needed
            gradient->unref();

            // Theme-based empty circle indicator
            SkPaint circlePaint;
            circlePaint.setStyle(SkPaint::kStroke_Style);
            circlePaint.setStrokeWidth(2.0f);
            circlePaint.setColor(colors_.hoverColor.getARGB());
            circlePaint.setAntiAlias(true);

            // Add inner glow with theme colors
            SkPaint glowPaint;
            glowPaint.setColor(colors_.withAlpha(colors_.glowColor, 0.2f).getARGB());
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
            canvas->drawCircle(cx, cy, 10, glowPaint);

            canvas->drawCircle(cx, cy, 10, circlePaint);
            break;
        }
        
        case ClipSlotState::Stopped: {
            // Clip loaded but stopped with theme gradient
            SkPoint points[2] = {SkPoint::Make(x, y), SkPoint::Make(x + w, y + h)};
            SkColor colors[2] = {
                data.color.withAlpha(0.8f).getARGB(),
                data.color.withAlpha(0.6f).getARGB()
            };

            SkGradientShader* gradient = SkGradientShader::CreateLinear(
                points, colors, nullptr, 2, SkShader::kClamp_TileMode);

            bgPaint.setShader(gradient);
            canvas->drawRRect(slotRRect, bgPaint);
            gradient->unref();

            // Theme-based play button with shadow
            SkPaint shadowPaint;
            shadowPaint.setColor(colors_.withAlpha(colors_.hoverColor, 0.4f).getARGB());
            shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
            canvas->save();
            canvas->translate(2, 2);
            canvas->drawPath(playTrianglePath(cx, cy, 12), shadowPaint);
            canvas->restore();

            // Play triangle with theme color
            SkPaint playPaint;
            playPaint.setColor(colors_.clipText.getARGB());
            playPaint.setAntiAlias(true);
            playPaint.setFilterQuality(kLow_SkFilterQuality);
            canvas->drawPath(playTrianglePath(cx, cy, 12), playPaint);

            // Clip name with theme typography
            SkPaint textPaint;
            textPaint.setColor(colors_.clipText.getARGB());
            textPaint.setAntiAlias(true);
            textPaint.setSubpixelText(true);
            SkFont font = design::typography::getSkFont(10.0f);
            font.setEdging(SkFont::Edging::kAntiAlias);

            canvas->save();
            canvas->clipRRect(slotRRect);
            canvas->drawString(data.name.toRawUTF8(), x + 12, y + h - 10, font, textPaint);
            canvas->restore();
            break;
        }
        
        case ClipSlotState::Playing: {
            // Playing clip with theme-based dynamic gradient
            SkPoint points[2] = {SkPoint::Make(x, y), SkPoint::Make(x + w, y + h)};
            SkColor colors[2] = {
                data.color.getARGB(),
                data.color.lighten(0.1f).getARGB()
            };

            SkGradientShader* gradient = SkGradientShader::CreateLinear(
                points, colors, nullptr, 2, SkShader::kClamp_TileMode);

            bgPaint.setShader(gradient);
            canvas->drawRRect(slotRRect, bgPaint);
            gradient->unref();

            // Animated multi-layered glow effect with theme settings
            float glowIntensity = animation_.glowIntensity + (1.0f - animation_.glowIntensity) *
                                 std::sin(animPhase_ * animation_.glowSpeed);

            // Outer glow with theme colors
            SkPaint outerGlow;
            outerGlow.setColor(colors_.withAlpha(data.color, glowIntensity * animation_.glowIntensity).getARGB());
            outerGlow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 20.0f));
            canvas->drawRRect(slotRRect, outerGlow);

            // Inner glow with theme colors
            SkPaint innerGlow;
            innerGlow.setColor(colors_.withAlpha(colors_.playingIndicatorColor, glowIntensity * 0.4f).getARGB());
            innerGlow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
            canvas->drawRRect(slotRRect, innerGlow);

            // Playing indicator with theme colors
            drawPlayingIndicator(canvas, cx, cy, 12, data.playProgress);

            // Pulsing border with theme colors
            SkPaint borderPaint;
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(3.0f);
            borderPaint.setColor(colors_.withAlpha(colors_.selectionColor, glowIntensity).getARGB());
            borderPaint.setPathEffect(SkDashPathEffect::Make(2, 4));
            canvas->drawRRect(slotRRect, borderPaint);

            // Clip name with theme typography
            SkPaint textPaint;
            textPaint.setColor(colors_.clipText.getARGB());
            textPaint.setAntiAlias(true);
            textPaint.setSubpixelText(true);
            SkFont font = design::typography::getSkFont(10.0f);
            font.setEdging(SkFont::Edging::kAntiAlias);

            // Text glow with theme colors
            SkPaint textGlow;
            textGlow.setColor(colors_.withAlpha(colors_.hoverColor, 0.2f).getARGB());
            textGlow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
            canvas->save();
            canvas->translate(1, 1);
            canvas->clipRRect(slotRRect);
            canvas->drawString(data.name.toRawUTF8(), x + 12, y + h - 10, font, textGlow);
            canvas->restore();

            canvas->save();
            canvas->clipRRect(slotRRect);
            canvas->drawString(data.name.toRawUTF8(), x + 12, y + h - 10, font, textPaint);
            canvas->restore();
            break;
        }
        
        case ClipSlotState::Queued: {
            // Queued - sophisticated pulsing effect with theme settings
            float pulsePhase = std::fmod(animPhase_ * animation_.pulseSpeed, juce::MathConstants<float>::twoPi);
            float pulseAlpha = 0.3f + 0.7f * (std::sin(pulsePhase) * 0.5f + 0.5f);
            float scale = 1.0f + 0.05f * std::sin(pulsePhase);

            // Animated background with theme colors
            SkPoint points[2] = {SkPoint::Make(x, y), SkPoint::Make(x + w, y + h)};
            SkColor colors[2] = {
                data.color.withAlpha(pulseAlpha * 0.8f).getARGB(),
                data.color.withAlpha(pulseAlpha * 0.6f).getARGB()
            };

            SkGradientShader* gradient = SkGradientShader::CreateLinear(
                points, colors, nullptr, 2, SkShader::kClamp_TileMode);

            bgPaint.setShader(gradient);
            canvas->drawRRect(slotRRect, bgPaint);
            gradient->unref();

            // Animated ring with rotation effect and theme colors
            canvas->save();
            canvas->translate(cx, cy);
            canvas->scale(scale, scale);

            // Ring glow with theme colors
            SkPaint ringGlow;
            ringGlow.setColor(colors_.withAlpha(data.color, pulseAlpha * 0.4f).getARGB());
            ringGlow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
            canvas->drawCircle(0, 0, 14, ringGlow);

            // Main ring with theme colors
            SkPaint ringPaint;
            ringPaint.setStyle(SkPaint::kStroke_Style);
            ringPaint.setStrokeWidth(3.0f);
            ringPaint.setColor(colors_.queuedIndicatorColor.getARGB());
            ringPaint.setAntiAlias(true);
            canvas->drawCircle(0, 0, 12, ringPaint);

            canvas->restore();

            // Clip name with theme typography
            SkPaint textPaint;
            textPaint.setColor(colors_.clipText.getARGB());
            textPaint.setAntiAlias(true);
            textPaint.setSubpixelText(true);
            SkFont font = design::typography::getSkFont(10.0f);
            font.setEdging(SkFont::Edging::kAntiAlias);

            canvas->save();
            canvas->clipRRect(slotRRect);
            canvas->drawString(data.name.toRawUTF8(), x + 12, y + h - 10, font, textPaint);
            canvas->restore();
            break;
        }

        case ClipSlotState::Recording: {
            // Recording - with theme colors and pulse
            float pulse = 0.7f + 0.3f * std::sin(animPhase_ * animation_.pulseSpeed);

            bgPaint.setColor(errorHandling_.errorColor.withAlpha(pulse).getARGB());
            canvas->drawRRect(slotRRect, bgPaint);

            // Glow with theme colors
            SkPaint glowPaint;
            glowPaint.setColor(errorHandling_.errorColor.withAlpha(0.5f * pulse).getARGB());
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
            canvas->drawRRect(slotRRect, glowPaint);

            // Record circle with theme colors
            drawRecordingIndicator(canvas, cx, cy, 10);
            break;
        }

        case ClipSlotState::Stopping: {
            // Stopping - fading with theme colors
            float fade = 0.5f + 0.3f * std::sin(animPhase_ * animation_.glowSpeed);
            bgPaint.setColor(data.color.withAlpha(fade).getARGB());
            canvas->drawRRect(slotRRect, bgPaint);
            break;
        }
    }
    
    // Enhanced selection ring with theme colors
    if (isSelected) {
        // Outer glow with theme settings
        SkPaint selGlow;
        selGlow.setColor(colors_.selectionGlowColor.withAlpha(animation_.selectionIntensity).getARGB());
        selGlow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, errorHandling_.errorGlowRadius));
        canvas->drawRRect(slotRRect, selGlow);

        // Main selection ring with theme colors
        SkPaint selPaint;
        selPaint.setStyle(SkPaint::kStroke_Style);
        selPaint.setStrokeWidth(3.0f);
        selPaint.setColor(colors_.selectionColor.getARGB());
        selPaint.setPathEffect(SkDashPathEffect::Make(2, 4));
        canvas->drawRRect(slotRRect, selPaint);
    }

    // Enhanced hover effect with theme colors
    if (isHovered && !isSelected) {
        // Subtle glow with theme settings
        SkPaint hoverGlow;
        hoverGlow.setColor(colors_.withAlpha(colors_.hoverColor, colors_.hoverOverlayOpacity).getARGB());
        hoverGlow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
        canvas->drawRRect(slotRRect, hoverGlow);

        // Highlight with theme colors
        SkPaint hoverPaint;
        hoverPaint.setColor(colors_.withAlpha(colors_.hoverColor, colors_.hoverOverlayOpacity * 2).getARGB());
        canvas->drawRRect(slotRRect, hoverPaint);
    }
}

void SkiaSessionView::drawPlayingIndicator(SkCanvas* canvas, float cx, float cy,
                                            float radius, float progress) {
    try {
        // Background circle with theme colors
        SkPaint bgPaint;
        bgPaint.setColor(colors_.withAlpha(colors_.playingIndicatorColor, 0.3f).getARGB());
        bgPaint.setAntiAlias(true);
        canvas->drawCircle(cx, cy, radius, bgPaint);

        // Progress arc with theme colors
        SkPaint arcPaint;
        arcPaint.setStyle(SkPaint::kStroke_Style);
        arcPaint.setStrokeWidth(3.0f);
        arcPaint.setColor(colors_.playingIndicatorColor.getARGB());
        arcPaint.setAntiAlias(true);
        arcPaint.setStrokeCap(SkPaint::kRound_Cap);

        SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);
        float sweepAngle = progress * 360.0f;
        canvas->drawArc(arcRect, -90, sweepAngle, false, arcPaint);

        // Center filled circle with theme colors
        SkPaint centerPaint;
        centerPaint.setColor(colors_.playingIndicatorColor.getARGB());
        centerPaint.setAntiAlias(true);
        canvas->drawCircle(cx, cy, radius - 4, centerPaint);

    } catch (const std::exception& e) {
        handleError("drawPlayingIndicator", e.what());
    }
}

void SkiaSessionView::drawQueuedIndicator(SkCanvas* canvas, float cx, float cy, float radius) {
    try {
        // Pulsing ring with theme animation
        float scale = 1.0f + 0.1f * std::sin(animPhase_ * animation_.pulseSpeed);

        SkPaint ringPaint;
        ringPaint.setStyle(SkPaint::kStroke_Style);
        ringPaint.setStrokeWidth(3.0f);
        ringPaint.setColor(colors_.queuedIndicatorColor.getARGB());
        ringPaint.setAntiAlias(true);
        canvas->drawCircle(cx, cy, radius * scale, ringPaint);

        // Inner dot with theme colors
        SkPaint dotPaint;
        dotPaint.setColor(colors_.queuedIndicatorColor.getARGB());
        dotPaint.setAntiAlias(true);
        canvas->drawCircle(cx, cy, 3, dotPaint);

    } catch (const std::exception& e) {
        handleError("drawQueuedIndicator", e.what());
    }
}

void SkiaSessionView::drawRecordingIndicator(SkCanvas* canvas, float cx, float cy, float radius) {
    try {
        // Theme-based glow effect
        SkPaint glowPaint;
        glowPaint.setColor(colors_.withAlpha(errorHandling_.errorColor, 0.4f).getARGB());
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 12.0f));
        canvas->drawCircle(cx, cy, radius + 4, glowPaint);

        // Main circle with theme colors
        SkPaint recPaint;
        recPaint.setColor(errorHandling_.errorColor.getARGB());
        recPaint.setAntiAlias(true);
        canvas->drawCircle(cx, cy, radius, recPaint);

    } catch (const std::exception& e) {
        handleError("drawRecordingIndicator", e.what());
    }
}

SkPath SkiaSessionView::playTrianglePath(float cx, float cy, float size) {
    SkPath path;
    path.moveTo(cx - size/2, cy - size/2);
    path.lineTo(cx + size/2, cy);
    path.lineTo(cx - size/2, cy + size/2);
    path.close();
    return path;
}

void SkiaSessionView::drawStopRow(SkCanvas* canvas) {
    try {
        float y = getHeight() - layout_.mixerHeight - layout_.stopRowHeight;

        SkPaint bgPaint;
        bgPaint.setColor(colors_.stopRowBg.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(layout_.sceneLauncherWidth, y, getWidth(), layout_.stopRowHeight), bgPaint);

        for (size_t t = 0; t < tracks_.size(); ++t) {
            float x = getTrackX(static_cast<int>(t));
            if (x > getWidth() || x + layout_.trackWidth < layout_.sceneLauncherWidth) continue;

            // Stop button with theme colors
            float btnX = x + layout_.trackWidth / 2;
            float btnY = y + layout_.stopRowHeight / 2;

            SkPaint stopPaint;
            stopPaint.setStyle(SkPaint::kStroke_Style);
            stopPaint.setStrokeWidth(2.0f);
            stopPaint.setColor(colors_.stopButtonColor.getARGB());
            stopPaint.setAntiAlias(true);

            SkRect stopRect = SkRect::MakeXYWH(btnX - layout_.stopButtonSize/2,
                                            btnY - layout_.stopButtonSize/2,
                                            layout_.stopButtonSize, layout_.stopButtonSize);
            canvas->drawRect(stopRect, stopPaint);
        }

        // Top border with theme colors
        SkPaint borderPaint;
        borderPaint.setColor(colors_.sectionDividerBorder.getARGB());
        canvas->drawLine(layout_.sceneLauncherWidth, y, getWidth(), y, borderPaint);

    } catch (const std::exception& e) {
        handleError("drawStopRow", e.what());
    }
}

void SkiaSessionView::drawMixerStrips(SkCanvas* canvas) {
    try {
        float y = getHeight() - layout_.mixerHeight;

        SkPaint bgPaint;
        bgPaint.setColor(colors_.mixerBg.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(layout_.sceneLauncherWidth, y, getWidth(), layout_.mixerHeight), bgPaint);

        for (size_t t = 0; t < tracks_.size(); ++t) {
            float x = getTrackX(static_cast<int>(t));
            if (x > getWidth() || x + layout_.trackWidth < layout_.sceneLauncherWidth) continue;

            // S/M buttons with theme colors
            float btnY = y + layout_.mixerPadding;

            // Solo
            SkPaint soloPaint;
            soloPaint.setStyle(tracks_[t].isSolo ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
            soloPaint.setStrokeWidth(1.5f);
            soloPaint.setColor(colors_.soloButtonColor.getARGB());
            soloPaint.setAntiAlias(true);

            SkRect soloRect = SkRect::MakeXYWH(x + layout_.mixerPadding, btnY,
                                              layout_.soloMuteButtonSize, layout_.soloMuteButtonSize);
            canvas->drawRoundRect(soloRect, layout_.panelRadius, layout_.panelRadius, soloPaint);

            SkPaint soloTextPaint;
            soloTextPaint.setColor(tracks_[t].isSolo ? SK_ColorBLACK : colors_.soloButtonColor.getARGB());
            soloTextPaint.setAntiAlias(true);
            SkFont smallFont = design::typography::getSkFont(10.0f);
            canvas->drawString("S", x + layout_.mixerPadding + 7, btnY + layout_.soloMuteButtonSize/2 + 4, smallFont, soloTextPaint);

            // Mute
            SkPaint mutePaint;
            mutePaint.setStyle(tracks_[t].isMuted ? SkPaint::kFill_Style : SkPaint::kStroke_Style);
            mutePaint.setStrokeWidth(1.5f);
            mutePaint.setColor(colors_.muteButtonColor.getARGB());
            mutePaint.setAntiAlias(true);

            SkRect muteRect = SkRect::MakeXYWH(x + layout_.mixerPadding * 3 + layout_.soloMuteButtonSize, btnY,
                                              layout_.soloMuteButtonSize, layout_.soloMuteButtonSize);
            canvas->drawRoundRect(muteRect, layout_.panelRadius, layout_.panelRadius, mutePaint);

            SkPaint muteTextPaint;
            muteTextPaint.setColor(tracks_[t].isMuted ? SK_ColorWHITE : colors_.muteButtonColor.getARGB());
            muteTextPaint.setAntiAlias(true);
            canvas->drawString("M", x + layout_.mixerPadding * 3 + layout_.soloMuteButtonSize + 7, btnY + layout_.soloMuteButtonSize/2 + 4, smallFont, muteTextPaint);

            // Volume fader with theme dimensions
            float faderX = x + layout_.mixerPadding * 4 + layout_.soloMuteButtonSize * 2;
            float faderY = y + layout_.mixerPadding;
            float faderW = layout_.faderWidth;
            float faderH = layout_.mixerHeight - layout_.mixerPadding * 2;

            // Fader track
            SkPaint trackPaint;
            trackPaint.setColor(colors_.mixerBg.darker(0.2f).getARGB());
            canvas->drawRect(SkRect::MakeXYWH(faderX, faderY, faderW, faderH), trackPaint);

            // Fader position
            float faderLevel = tracks_[t].volume;
            float faderFillH = faderH * faderLevel;

            SkPaint fillPaint;
            fillPaint(colors_.accentButtonColor.getARGB());
            canvas->drawRect(SkRect::MakeXYWH(faderX, faderY + faderH - faderFillH, faderW, faderFillH), fillPaint);

            // Meter
            float meterX = x + layout_.mixerPadding * 5 + layout_.soloMuteButtonSize * 2 + layout_.faderWidth + 4;
            float meterLevel = tracks_[t].meterLevel;
            float meterFillH = faderH * meterLevel;

            // Meter background
            SkPaint meterBgPaint;
            meterBgPaint.setColor(colors_.mixerBg.darker(0.2f).getARGB());
            canvas->drawRect(SkRect::MakeXYWH(meterX, faderY, layout_.meterWidth, faderH), meterBgPaint);

            // Meter fill using theme colors
            if (meterLevel > 0) {
                SkColor meterColor;
                if (meterLevel < 0.7f) {
                    meterColor = colors_.meterSafeColor.getARGB();
                } else if (meterLevel < 0.9f) {
                    meterColor = colors_.meterWarningColor.getARGB();
                } else {
                    meterColor = colors_.meterHotColor.getARGB();
                }

                SkPaint meterPaint;
                meterPaint.setColor(meterColor);
                canvas->drawRect(SkRect::MakeXYWH(meterX, faderY + faderH - meterFillH, layout_.meterWidth, meterFillH), meterPaint);
            }

            // dB label
            float dbValue = 20.0f * std::log10(std::max(0.001f, tracks_[t].volume));
            juce::String dbStr = juce::String(dbValue, 1) + " dB";

            SkPaint dbPaint;
            dbPaint.setColor(colors_.meterText.getARGB());
            dbPaint.setAntiAlias(true);
            canvas->drawString(dbStr.toRawUTF8(), x + layout_.mixerPadding * 6 + layout_.soloMuteButtonSize * 2 + layout_.faderWidth + 12,
                              y + layout_.mixerHeight - 8, smallFont, dbPaint);

            // Divider with theme colors
            SkPaint divPaint;
            divPaint.setColor(colors_.trackDividerBorder.getARGB());
            canvas->drawLine(x + layout_.trackWidth - 1, y, x + layout_.trackWidth - 1, y + layout_.mixerHeight, divPaint);
        }

        // Top border with theme colors
        SkPaint borderPaint;
        borderPaint.setColor(colors_.sectionDividerBorder.getARGB());
        canvas->drawLine(layout_.sceneLauncherWidth, y, getWidth(), y, borderPaint);

    } catch (const std::exception& e) {
        handleError("drawMixerStrips", e.what());
    }
}

void SkiaSessionView::drawMasterTrack(SkCanvas* canvas) {
    try {
        // Master track is always visible in bottom-right corner
        float x = getWidth() - 100; // Wider to accommodate theme spacing
        float y = getHeight() - layout_.mixerHeight;
        float w = 100;
        float h = layout_.mixerHeight;

        // Background with theme colors
        SkPaint bgPaint;
        bgPaint.setColor(colors_.trackHeaderBg.getARGB());
        canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), bgPaint);

        // "Master" label with theme typography
        SkPaint textPaint;
        textPaint.setColor(colors_.trackHeaderText.getARGB());
        textPaint.setAntiAlias(true);
        SkFont font = design::typography::getSkFont(10.0f);
        canvas->drawString("MASTER", x + layout_.headerPadding, y + layout_.headerPadding + 12, font, textPaint);

        // Master meter (stereo) with theme dimensions
        float meterX = x + 20;
        float meterY = y + 24;
        float meterW = layout_.meterWidth * 4; // Wider for dual meters
        float meterH = h - 40;

        SkPaint meterBgPaint;
        meterBgPaint.setColor(colors_.mixerBg.darker(0.2f).getARGB());
        canvas->drawRect(SkRect::MakeXYWH(meterX, meterY, meterW, meterH), meterBgPaint);

        // Real master levels (updated via setMasterMeterLevel)
        float masterL = masterMeterLeft_;
        float masterR = masterMeterRight_;

        // Theme-based meter colors
        SkPaint meterPaint;
        meterPaint.setColor(colors_.meterSafeColor.getARGB());

        // Left channel
        float lH = meterH * masterL;
        canvas->drawRect(SkRect::MakeXYWH(meterX, meterY + meterH - lH, 6, lH), meterPaint);

        // Right channel
        float rH = meterH * masterR;
        canvas->drawRect(SkRect::MakeXYWH(meterX + 8, meterY + meterH - rH, 6, rH), meterPaint);

        // Left border with theme colors
        SkPaint borderPaint;
        borderPaint.setColor(colors_.trackDividerBorder.getARGB());
        canvas->drawLine(x, y, x, y + h, borderPaint);

    } catch (const std::exception& e) {
        handleError("drawMasterTrack", e.what());
    }
}

//==============================================================================
// Animation
//==============================================================================

void SkiaSessionView::onAnimationTick(float deltaMs) {
    // Use theme animation speed
    animPhase_ += deltaMs * 0.003f * animation_.pulseSpeed;
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

    // Only redraw if animations are enabled
    if (needsRedraw && !errorHandling_.useSimpleFallback) {
        markDirty();
    }
}

//==============================================================================
// Hit Testing
//==============================================================================

std::pair<int, int> SkiaSessionView::hitTestSlot(float x, float y) const {
    if (x < layout_.sceneLauncherWidth || y < layout_.trackHeaderHeight) {
        return {-1, -1};
    }
    if (y > getHeight() - layout_.mixerHeight - layout_.stopRowHeight) {
        return {-1, -1};
    }

    int trackIdx = static_cast<int>((x - layout_.sceneLauncherWidth + scrollX_) / layout_.trackWidth);
    int sceneIdx = static_cast<int>((y - layout_.trackHeaderHeight + scrollY_) / layout_.clipSlotHeight);

    if (trackIdx < 0 || trackIdx >= static_cast<int>(tracks_.size())) return {-1, -1};
    if (sceneIdx < 0 || sceneIdx >= static_cast<int>(scenes_.size())) return {-1, -1};

    return {trackIdx, sceneIdx};
}

int SkiaSessionView::hitTestScene(float y) const {
    if (y < layout_.trackHeaderHeight || y > getHeight() - layout_.mixerHeight) return -1;
    return static_cast<int>((y - layout_.trackHeaderHeight + scrollY_) / layout_.clipSlotHeight);
}

int SkiaSessionView::hitTestTrack(float x) const {
    if (x < layout_.sceneLauncherWidth) return -1;
    return static_cast<int>((x - layout_.sceneLauncherWidth + scrollX_) / layout_.trackWidth);
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
    float stopRowY = getHeight() - layout_.mixerHeight - layout_.stopRowHeight;

    // Check if y is in stop row
    if (y < stopRowY || y > stopRowY + layout_.stopRowHeight) {
        return -1;
    }

    // Check if x is in track area (excluding scene launcher)
    if (x < layout_.sceneLauncherWidth) {
        return -1;
    }

    // Find which track's stop button
    int trackIdx = static_cast<int>((x - layout_.sceneLauncherWidth + scrollX_) / layout_.trackWidth);
    if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks_.size())) {
        return trackIdx;
    }

    return -1;
}

void SkiaSessionView::mouseWheelMove(const juce::MouseEvent& e,
                                      const juce::MouseWheelDetails& wheel) {
    juce::ignoreUnused(e);

    float scrollSpeed = 30.0f * layout_.animationSpeed;
    setScrollPosition(scrollX_ - wheel.deltaX * scrollSpeed,
                      scrollY_ - wheel.deltaY * scrollSpeed);
}

void SkiaSessionView::handleError(const std::string& operation, const std::string& error) {
    if (errorHandling_.logErrors) {
        zenith::ui::ViewTheme::getThemeManager().reportError("SkiaSessionView",
            operation + ": " + error);
    }

    if (errorHandling_.enableAutoRecovery) {
        // Auto-recovery attempt
        if (errorState_.errorCount < errorHandling_.maxRetries) {
            // Apply fallback theme
            try {
                zenith::ui::ViewTheme::getThemeManager().applyBuiltInTheme(
                    zenith::ui::ViewTheme::Theme::BuiltIn::DarkModern);
                errorState_.errorCount++;
            } catch (...) {
                // If recovery fails, log and continue
                if (errorHandling_.logErrors) {
                    zenith::ui::ViewTheme::getThemeManager().reportError(
                        "SkiaSessionView", "Recovery failed for: " + operation);
                }
            }
        }
    }

    // Force immediate redraw to show error state
    markDirty();
}

} // namespace zenith::ui
