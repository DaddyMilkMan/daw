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

// SampleEditorRendering.cpp

#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#include <effects/SkDashPathEffect.h>

namespace zenith {

void SampleEditorComponent::drawSkia(SkCanvas* canvas) {
    if (!canvas) return;
    
    // Using tokens for design
    using namespace design;
    
    float w = (float)getWidth();
    float h = (float)getHeight();
    float radius = dimensions::RADIUS_SM;
    
    drawBackground(canvas, w, h, radius);
    
    if (!audioHandle_ && !isRecording_) {
        drawEmptyState(canvas, w, h);
        drawBorder(canvas, w, h, radius);
        return;
    }

    SkRect fullBounds = SkRect::MakeWH(w, h);
    
    // Layout
    SkRect toolbarArea = SkRect::MakeXYWH(0, 0, w, toolbarHeight_);
    SkRect overviewArea = SkRect::MakeXYWH(0, toolbarHeight_, w, overviewHeight_);
    SkRect rulerArea = SkRect::MakeXYWH(0, toolbarHeight_ + overviewHeight_, w, rulerHeight_);
    SkRect mainArea = SkRect::MakeXYWH(0, toolbarHeight_ + overviewHeight_ + rulerHeight_, w, h - toolbarHeight_ - overviewHeight_ - rulerHeight_ - scrollbarHeight_);
    SkRect scrollbarArea = SkRect::MakeXYWH(0, h - scrollbarHeight_, w, scrollbarHeight_);

    drawToolbar(canvas, toolbarArea);
    drawOverview(canvas, overviewArea);
    drawRuler(canvas, rulerArea);
    
    // Main Content
    canvas->save();
    canvas->clipRect(mainArea);
    
    drawGrid(canvas, mainArea);
    
    if (viewMode_ == WaveformViewMode::Waveform || viewMode_ == WaveformViewMode::Combined)
        drawWaveform(canvas, mainArea);
        
    if (viewMode_ == WaveformViewMode::Spectrogram || viewMode_ == WaveformViewMode::Combined)
        drawSpectrogram(canvas, mainArea);
        
    drawSelection(canvas, mainArea);
    drawMarkers(canvas, mainArea);
    drawRegions(canvas, mainArea);
    drawWarpMarkers(canvas, mainArea);
    drawPlayhead(canvas, mainArea);
    
    canvas->restore();
    
    drawScrollbar(canvas, scrollbarArea);
    drawBorder(canvas, w, h, radius);
}

void SampleEditorComponent::drawBackground(SkCanvas* canvas, float w, float h, float radius) {
    SkPaint paint;
    paint.setColor(design::colors::BG_DARK);
    paint.setAntiAlias(true);
    
    SkRRect rrect = SkRRect::MakeRectXY(SkRect::MakeWH(w, h), radius, radius);
    canvas->drawRRect(rrect, paint);
}

void SampleEditorComponent::drawBorder(SkCanvas* canvas, float w, float h, float radius) {
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    paint.setColor(design::colors::BORDER_SUBTLE);
    paint.setAntiAlias(true);
    
    SkRRect rrect = SkRRect::MakeRectXY(SkRect::MakeWH(w, h).makeInset(0.5f, 0.5f), radius, radius);
    canvas->drawRRect(rrect, paint);
}

void SampleEditorComponent::drawEmptyState(SkCanvas* canvas, float w, float h) {
    using namespace design;
    SkFont font = typography::getSkFont(typography::FONT_MD, FontWeight::Medium);
    SkPaint paint;
    paint.setColor(withAlpha(colors::TEXT_PRIMARY, opacity::SECONDARY));
    paint.setAntiAlias(true);
    
    const char* text = "Drag and drop a sample or Click to Record";
    float textW = font.measureText(text, strlen(text), SkTextEncoding::kUTF8);
    canvas->drawString(text, (w - textW) * 0.5f, h * 0.5f, font, paint);
}

void SampleEditorComponent::drawToolbar(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;
    
    // Toolbar bg
    SkPaint bgPaint;
    bgPaint.setColor(colors::BG_DARKER);
    canvas->drawRect(bounds, bgPaint);
    
    // Bottom border
    SkPaint border;
    border.setColor(colors::BORDER_DEFAULT);
    canvas->drawLine(bounds.left(), bounds.bottom(), bounds.right(), bounds.bottom(), border);
    
    float x = 10.0f;
    float btnW = 32.0f;
    float btnH = 24.0f;
    float y = (bounds.height() - btnH) * 0.5f;

    // Logic for drawing buttons would go here (delegated to drawToolbarButton)
    // For now we assume they are drawn in a loop or specifically
}

void SampleEditorComponent::drawToolbarButton(SkCanvas* canvas, const SkRect& bounds, 
                                            const char* icon, const char* tooltip, 
                                            bool isActive, bool isEnabled) {
    using namespace design;
    
    SkPaint paint;
    if (isActive) paint.setColor(withAlpha(colors::ACCENT_PRIMARY, opacity::ACTIVE));
    else if (!isEnabled) paint.setColor(withAlpha(colors::BG_LIGHT, opacity::DISABLED));
    else paint.setColor(colors::BG_LIGHT);
    
    canvas->drawRoundRect(bounds, dimensions::RADIUS_XXS, dimensions::RADIUS_XXS, paint);
    
    // Icon rendering...
}

void SampleEditorComponent::drawOverview(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;
    
    SkPaint bg;
    bg.setColor(colors::BG_DARKEST);
    canvas->drawRect(bounds, bg);
    
    // Draw simplified waveform in overview
    // Draw viewport rectangle based on zoom/scroll
    double totalDuration = audioHandle_ ? audioHandle_->buffer.getNumSamples() / audioHandle_->sampleRate : 1.0;
    float viewportX = (float)(timeOffset_ / totalDuration * bounds.width());
    float viewportW = (float)(viewWidthSeconds_ / totalDuration * bounds.width());
    
    SkRect viewport = SkRect::MakeXYWH(bounds.left() + viewportX, bounds.top(), viewportW, bounds.height());
    SkPaint vpPaint;
    vpPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.1f));
    canvas->drawRect(viewport, vpPaint);
}

void SampleEditorComponent::drawRuler(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;
    SkPaint paint;
    paint.setColor(colors::BORDER_DEFAULT);
    canvas->drawLine(bounds.left(), bounds.bottom() - 1, bounds.right(), bounds.bottom() - 1, paint);
    
    // Draw time markings...
}

void SampleEditorComponent::drawGrid(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;
    SkPaint paint;
    paint.setColor(withAlpha(colors::BORDER_SUBTLE, 0.05f));
    // Grid logic...
}

void SampleEditorComponent::drawWaveform(SkCanvas* canvas, const SkRect& bounds) {
    if (!audioHandle_ && !editBuffer_ && !isRecording_) return;
    
    using namespace design;
    SkPaint paint;
    paint.setColor(colors::ACCENT_PRIMARY);
    paint.setAntiAlias(true);
    paint.setStrokeWidth(1.0f);
    paint.setStyle(SkPaint::kStroke_Style);
    
    // Core waveform rendering logic...
}

void SampleEditorComponent::drawSpectrogram(SkCanvas* canvas, const SkRect& bounds) {
    // Spectral rendering logic...
}

void SampleEditorComponent::drawSelection(SkCanvas* canvas, const SkRect& bounds) {
    if (selection_.isEmpty()) return;
    
    using namespace design;
    float x1 = timeToPixels(selection_.getStart(), bounds.width());
    float x2 = timeToPixels(selection_.getEnd(), bounds.width());
    
    SkRect selRect = SkRect::MakeLTRB(bounds.left() + x1, bounds.top(), bounds.left() + x2, bounds.bottom());
    SkPaint paint;
    paint.setColor(withAlpha(colors::ACCENT_PRIMARY, 0.2f));
    canvas->drawRect(selRect, paint);
}

void SampleEditorComponent::drawPlayhead(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;
    float px = timeToPixels(playheadPosition_, bounds.width());
    if (px < 0 || px > bounds.width()) return;
    
    SkPaint paint;
    paint.setColor(colors::TEXT_PRIMARY);
    paint.setStrokeWidth(2.0f);
    canvas->drawLine(bounds.left() + px, bounds.top(), bounds.left() + px, bounds.bottom(), paint);
}

void SampleEditorComponent::drawMarkers(SkCanvas* canvas, const SkRect& bounds) {
    // Marker drawing...
}

void SampleEditorComponent::drawRegions(SkCanvas* canvas, const SkRect& bounds) {
    // Region drawing...
}

void SampleEditorComponent::drawWarpMarkers(SkCanvas* canvas, const SkRect& bounds) {
    // Warp marker drawing...
}

void SampleEditorComponent::drawScrollbar(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;
    SkPaint bg;
    bg.setColor(colors::BG_DARKEST);
    canvas->drawRect(bounds, bg);
    
    // Scrollbar thumb drawing...
}

juce::Colour SampleEditorComponent::getColorForVelocity(int velocity) const {
    // Dummy for compatibility if needed, though SampleEditor usually doesn't use velocity colors the same way
    return juce::Colours::white;
}

SkColor SampleEditorComponent::getSkiaColorForVelocity(int velocity) const {
    return SkColorSetRGB(255, 255, 255);
}

} // namespace zenith
