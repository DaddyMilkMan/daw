/**
 * @file ArrangerRenderer.cpp
 * @brief Implementation of Skia rendering for ArrangerComponent
 */

#ifdef ZENITH_USE_SKIA

#include "ArrangerRenderer.h"
#include "ArrangerComponent.h"
#include "ArrangerClipManager.h"
#include "ArrangerGridUtils.h"
#include "ArrangerInputHandler.h"
#include "ArrangerTrackComponent.h"
#include "Engine.h"
#include "ProjectState.h"
#include "ZenithDesignSystem.h"
#include "ColorBridge.h"

// Skia Includes
#include "ZenithSkia.h"
#include "../arranger/ArrangerRenderer.h"
#include "../design-system/ColorBridge.h"
#include <core/SkMaskFilter.h>
#include <core/SkSpan.h>
#include <effects/SkDashPathEffect.h>
#include <effects/SkGradientShader.h>

#ifdef kNormal_SkBlurStyle
#undef kNormal_SkBlurStyle
#endif

#include <cmath>
#include <algorithm>

namespace zenith {
using namespace design;

//==============================================================================
// Layout Constants 
//==============================================================================
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float SECTION_HEIGHT = 24.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT = 80.0f;
static constexpr float TOP_MARGIN = SECTION_HEIGHT + RULER_HEIGHT;

// Grid Visibility Constants (float 0.0-1.0 for withAlpha())
static constexpr float kBarHighlightAlphaTop = 0.06f;
static constexpr float kBarHighlightAlphaBottom = 0.03f;
static constexpr float kBarLineAlpha = 0.4f;
static constexpr float kBarLineWidth = 1.5f;
static constexpr float kBeatLineAlpha = 0.2f;
static constexpr float kBeatLineWidth = 1.0f;
static constexpr float kMinGridSpacing = 10.0f;

//==============================================================================
// Constructor
//==============================================================================

ArrangerRenderer::ArrangerRenderer(ArrangerComponent& owner, Engine& engine, ProjectState& projectState,
                                   ArrangerClipManager& clipManager, ArrangerGridUtils& gridUtils)
    : owner_(owner)
    , engine_(engine)
    , projectState_(projectState)
    , clipManager_(clipManager)
    , gridUtils_(gridUtils)
{
}

ArrangerRenderer::~ArrangerRenderer() = default;

//==============================================================================
// Main Drawing Entry Point
//==============================================================================

void ArrangerRenderer::drawSkia(SkCanvas* canvas) {
    using namespace zenith::design;
    
    auto bounds = owner_.getLocalBounds();
    float width = static_cast<float>(bounds.getWidth());
    float height = static_cast<float>(bounds.getHeight());

    // 1. Background
    drawBackground(canvas, width, height);
    
    // 2. Grid & Timeline
    drawGrid(canvas, width, height);
    
    // 3. Loop Region
    drawLoopRegion(canvas, width, height);
    
    // 4. Section Highlight
    drawSectionHighlight(canvas, height);
    
    // 5. Tracks (disabled - handled by TrackComponents)
    // drawTracks(canvas, width, height);
    
    // 6. Clips
    drawClips(canvas, width, height);
    
    // 7. Marquee Selection
    drawMarquee(canvas);
    
    // 8. Section Track
    drawSectionTrack(canvas);
    
    // 9. Playhead
    drawPlayhead(canvas, width, height);
    
    // 10. Insertion Guide
    drawInsertionGuide(canvas, height);
}

//==============================================================================
// Background Drawing
//==============================================================================

void ArrangerRenderer::drawBackground(SkCanvas* canvas, float /* width */, float /* height */) {
    using namespace zenith::design;
    canvas->drawColor(colors::BG_DARKEST);
}

//==============================================================================
// Grid Drawing
//==============================================================================

void ArrangerRenderer::drawGrid(SkCanvas* canvas, float width, float height) {
    using namespace zenith::design;
    
    // Optimization: Get clip bounds to only draw visible grid lines
    SkRect clipBounds = SkRect::MakeWH(width, height);
    if (!canvas->getLocalClipBounds(&clipBounds)) {
        clipBounds = SkRect::MakeWH(width, height);
    }

    int beatsPerBar = gridUtils_.getBeatsPerBar();
    
    // Calculate start/end based on CLIP BOUNDS, not just view bounds
    // x = HEADER_WIDTH + (beat - viewStart) * ppb
    // beat = viewStart + (x - HEADER_WIDTH) / ppb
    
    float clipLeft = std::max(HEADER_WIDTH, clipBounds.fLeft);
    float clipRight = std::min(width, clipBounds.fRight);
    
    if (clipLeft >= clipRight) return;

    double startBeat = owner_.viewStartBeats + (clipLeft - HEADER_WIDTH) / owner_.pixelsPerBeat;
    double endBeat = owner_.viewStartBeats + (clipRight - HEADER_WIDTH) / owner_.pixelsPerBeat;
    
    // Buffer for edge cases
    startBeat = std::max(0.0, startBeat - 1.0);
    endBeat += 1.0;

    // Grid Step Calculation
    double gridStep = owner_.gridSnapBeats;
    if (gridStep <= 0.0) gridStep = 1.0;
    
    // Adaptive density: ensure lines aren't too close
    while (gridStep * owner_.pixelsPerBeat < kMinGridSpacing) {
        gridStep *= 2.0;
    }
    
    // SMART GRID LOD: Calculate fade-in alpha for finer subdivisions
    float currentPpb = owner_.pixelsPerBeat;
    float nextGridStep = gridStep / 2.0f;
    float fadeFactor = juce::jlimit(0.0f, 1.0f, (currentPpb * nextGridStep - kMinGridSpacing) / kMinGridSpacing);
    SkAlpha subBeatAlpha = static_cast<SkAlpha>(fadeFactor * kBeatLineAlpha);

    // Draw grid only within the timeline area
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(HEADER_WIDTH, SECTION_HEIGHT,
                                      width - HEADER_WIDTH, height - SECTION_HEIGHT));
    
    // A. Alternating BAR HIGHLIGHTING (subtle zebra striping)
    SkPaint barHighlightPaint;
    barHighlightPaint.setStyle(SkPaint::kFill_Style);
    barHighlightPaint.setAntiAlias(false);
    
    double alignedStart = std::floor(startBeat / gridStep) * gridStep;
    
    int startBar = static_cast<int>(std::floor(startBeat / beatsPerBar));
    int endBar = static_cast<int>(std::ceil(endBeat / beatsPerBar));
    
    for (int bar = startBar; bar <= endBar; ++bar) {
        if (bar % 2 == 0) {
            float barStartX = gridUtils_.beatsToX(bar * beatsPerBar);
            float barEndX = gridUtils_.beatsToX((bar + 1) * beatsPerBar);
            
            // Skip if out of horizontal clip
            if (barEndX < clipLeft || barStartX > clipRight) continue;

            SkPoint pts[2] = {{barStartX, SECTION_HEIGHT}, {barStartX, height}};
            SkColor colors[2] = {
            withAlpha(SK_ColorWHITE, kBarHighlightAlphaTop),
            withAlpha(SK_ColorWHITE, kBarHighlightAlphaBottom)
        };
            barHighlightPaint.setShader(SkGradientShader::MakeLinear(
                pts, colors, nullptr, 2, SkTileMode::kClamp));
            
            canvas->drawRect(SkRect::MakeXYWH(barStartX, SECTION_HEIGHT,
                                               barEndX - barStartX, height - SECTION_HEIGHT),
                             barHighlightPaint);
            barHighlightPaint.setShader(nullptr);
        }
    }
    
    // B. GRID LINES with hierarchy
    for (double beat = alignedStart; beat <= endBeat + 0.001; beat += gridStep) {
        float x = gridUtils_.beatsToX(beat);
        
        // Skip if outside view or clip
        if (x < clipLeft || x > clipRight) continue;

        // Determine hierarchy
        bool isBarLine = (std::abs(std::fmod(beat, (double)beatsPerBar)) < 0.001);
        bool isBeatLine = (std::abs(std::fmod(beat, 1.0)) < 0.001);
        
        SkPaint gridPaint;
        gridPaint.setAntiAlias(true);
        
        if (isBarLine) {
            gridPaint.setColor(withAlpha(colors::ACCENT_PRIMARY, 0.25f)); // Neon primary for bars
            gridPaint.setStrokeWidth(kBarLineWidth);
        } else if (isBeatLine) {
            gridPaint.setColor(withAlpha(SK_ColorWHITE, kBeatLineAlpha));
            gridPaint.setStrokeWidth(kBeatLineWidth);
        } else {
            // Sub-beat (Smart Grid LOD Fade)
            if (subBeatAlpha > 5) {
                gridPaint.setColor(withAlpha(SK_ColorWHITE, subBeatAlpha));
                gridPaint.setStrokeWidth(0.5f);
            } else {
                continue; // Skip invisible lines
            }
        }
        
        canvas->drawLine(x, SECTION_HEIGHT, x, height, gridPaint);
    }
    
    canvas->restore();
    
    // C. HEADER/TIMELINE BOUNDARY GLOW
    // Only draw if within clip
    if (clipBounds.intersects(SkRect::MakeXYWH(HEADER_WIDTH, SECTION_HEIGHT, 40, height - SECTION_HEIGHT))) {
        SkPaint boundaryGlowPaint;
        SkPoint glowPts[2] = {{HEADER_WIDTH, 0}, {HEADER_WIDTH + 40, 0}}; // Wider glow
        SkColor glowColors[2] = {
            withAlpha(colors::ACCENT_PRIMARY, 0.25f),
            withAlpha(colors::ACCENT_PRIMARY, 0.0f)
        };
        boundaryGlowPaint.setShader(SkGradientShader::MakeLinear(
            glowPts, glowColors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRect(SkRect::MakeXYWH(HEADER_WIDTH, SECTION_HEIGHT, 40,
                                           height - SECTION_HEIGHT), boundaryGlowPaint);
    }
}

//==============================================================================
// Section Highlight Drawing
//==============================================================================

void ArrangerRenderer::drawSectionHighlight(SkCanvas* canvas, float height) {
    if (!owner_.sectionTrack)
        return;
        
    const auto* section = owner_.sectionTrack->getHoveredSection();
    if (!section)
        section = owner_.sectionTrack->getDraggingSection();
    
    if (!section)
        return;
        
    float sx = gridUtils_.beatsToX(section->startBeats);
    float sl = static_cast<float>(section->lengthBeats * owner_.pixelsPerBeat);
    
    if (sl <= 0)
        return;
    
    // Optimization: Check clip intersection
    SkRect r = SkRect::MakeXYWH(sx, SECTION_HEIGHT, sl, height - SECTION_HEIGHT);
    SkRect clip;
    if (canvas->getLocalClipBounds(&clip)) {
        if (!clip.intersects(r)) return;
    }

    SkPaint highlightPaint;
    juce::Colour c = section->color;
    if (c.isTransparent())
        c = juce::Colours::cyan;
    SkColor sc = design::withAlpha(design::toSkColor(c), 0.15f);
    
    highlightPaint.setColor(sc);
    highlightPaint.setStyle(SkPaint::kFill_Style);
    
    canvas->drawRect(r, highlightPaint);
}

//==============================================================================
// Clips Drawing
//==============================================================================

void ArrangerRenderer::drawClips(SkCanvas* canvas, float width, float height) {
    using namespace zenith::design;
    
    // Optimization: Get clip bounds
    SkRect clipBounds = SkRect::MakeWH(width, height);
    if (!canvas->getLocalClipBounds(&clipBounds)) {
        clipBounds = SkRect::MakeWH(width, height);
    }
    
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(HEADER_WIDTH, RULER_HEIGHT, 
                                       width - HEADER_WIDTH, height - RULER_HEIGHT));
    
    for (const auto& clipView : clipManager_.getClipViews()) {
        // Check visibility against global view AND dirty rect
        SkRect clipRect = SkRect::MakeXYWH(clipView.bounds.getX(), clipView.bounds.getY(),
                                           clipView.bounds.getWidth(), clipView.bounds.getHeight());
                                           
        if (!clipRect.intersects(clipBounds)) {
             continue; // Skip clips outside dirty region
        }
        
        if (clipView.bounds.getRight() < HEADER_WIDTH || clipView.bounds.getX() > width)
            continue;
            
        drawSingleClip(canvas, clipView);
    }
    
    canvas->restore();
}

void ArrangerRenderer::drawSingleClip(SkCanvas* canvas, const ClipView& clipView) {
    using namespace zenith::design;
    
    SkRect r = SkRect::MakeXYWH(clipView.bounds.getX(), clipView.bounds.getY(),
                                clipView.bounds.getWidth(), clipView.bounds.getHeight());
    
    float clipRadius = 6.0f;
    SkRRect rr = SkRRect::MakeRectXY(r, clipRadius, clipRadius);
    
    // Determine base color based on clip type
    SkColor baseColor = clipView.isMidi ? colors::MAGENTA : colors::CYAN;
    if (clipView.isSelected) {
        baseColor = lighten(baseColor, 0.15f);
    }
    
    // 1. DROP SHADOW
    {
        SkPaint shadowPaint;
        shadowPaint.setAntiAlias(true);
        shadowPaint.setColor(withAlpha(SK_ColorBLACK, 0.25f));
        shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
        SkRect shadowRect = r;
        shadowRect.offset(0, 2);
        canvas->drawRRect(SkRRect::MakeRectXY(shadowRect, clipRadius, clipRadius), shadowPaint);
    }
    
    // 2. GLASSMORPHIC BACKGROUND
    {
        SkPaint bgPaint;
        bgPaint.setAntiAlias(true);
        
        SkPoint pts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
        SkColor bgColors[3] = {
            withAlpha(lighten(baseColor, 0.1f), 0.5f),
            withAlpha(baseColor, 0.35f),
            withAlpha(darken(baseColor, 0.2f), 0.25f)
        };
        float bgPositions[3] = {0.0f, 0.4f, 1.0f};
        
        bgPaint.setShader(SkGradientShader::MakeLinear(pts, bgColors, bgPositions, 3, SkTileMode::kClamp));
        canvas->drawRRect(rr, bgPaint);
    }
    
    // 3. INNER GLASSMORPHIC TEXTURE
    {
        SkPaint texturePaint;
        texturePaint.setAntiAlias(true);
        texturePaint.setColor(withAlpha(SK_ColorWHITE, 0.03f));
        texturePaint.setBlendMode(SkBlendMode::kOverlay);
        canvas->drawRRect(rr, texturePaint);
    }
    
    // 4. TOP RIM LIGHT
    {
        SkPaint rimPaint;
        rimPaint.setAntiAlias(true);
        rimPaint.setStyle(SkPaint::kStroke_Style);
        rimPaint.setStrokeWidth(1.0f);
        
        SkPoint rimPts[2] = {{r.left(), r.top()}, {r.right() * 0.6f, r.top() + r.height() * 0.3f}};
        SkColor rimColors[2] = {
            withAlpha(SK_ColorWHITE, 0.45f),
            withAlpha(SK_ColorWHITE, 0.0f)
        };
        rimPaint.setShader(SkGradientShader::MakeLinear(rimPts, rimColors, nullptr, 2, SkTileMode::kClamp));
        
        SkRRect innerRR = rr;
        innerRR.inset(0.5f, 0.5f);
        canvas->drawRRect(innerRR, rimPaint);
    }
    
    // 5. CLIP BORDER
    {
        SkPaint borderPaint;
        borderPaint.setAntiAlias(true);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(1.0f);
        
        SkPoint borderPts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
        SkColor borderColors[2] = {withAlpha(baseColor, 0.8f), withAlpha(baseColor, 0.4f)};
        borderPaint.setShader(SkGradientShader::MakeLinear(borderPts, borderColors, nullptr, 2, SkTileMode::kClamp));
        
        canvas->drawRRect(rr, borderPaint);
    }
    
    // 6. SELECTION GLOW
    if (clipView.isSelected) {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(4.0f); // Slightly thicker
        glowPaint.setColor(withAlpha(baseColor, 0.7f)); // Match clip base color
        glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f)); // More blur
        canvas->drawRRect(rr, glowPaint);
        
        SkPaint corePaint;
        corePaint.setAntiAlias(true);
        corePaint.setStyle(SkPaint::kStroke_Style);
        corePaint.setStrokeWidth(2.0f); // Crisper core
        corePaint.setColor(withAlpha(SK_ColorWHITE, 0.9f)); // Bright core line
        canvas->drawRRect(rr, corePaint);
    }
    
    // 7. CONTENT (Waveform or MIDI)
    {
        SkRect contentRect = r;
        contentRect.inset(4.0f, 16.0f);
        contentRect.fTop += 4.0f;
        
        canvas->save();
        canvas->clipRRect(rr, true);
        
        if (clipView.isMidi) {
            drawClipMidiBlobs(canvas, clipView, contentRect);
        } else {
            drawClipWaveform(canvas, clipView, contentRect);
        }
        
        canvas->restore();
    }
    
    
    // 7.5. FADE OVERLAY & HANDLES
    {
        float fadeInPx = static_cast<float>(clipView.fadeInBeats * owner_.pixelsPerBeat);
        float fadeOutPx = static_cast<float>(clipView.fadeOutBeats * owner_.pixelsPerBeat);
        
        SkPaint fadeCurvePaint;
        fadeCurvePaint.setAntiAlias(true);
        fadeCurvePaint.setStyle(SkPaint::kStroke_Style);
        fadeCurvePaint.setStrokeWidth(1.5f);
        fadeCurvePaint.setColor(withAlpha(SK_ColorWHITE, 0.7f));
        
        SkPaint handlePaint;
        handlePaint.setAntiAlias(true);
        handlePaint.setColor(withAlpha(SK_ColorWHITE, 0.5f));
        
        if (fadeInPx > 0) {
            // Draw Linear Fade In
            canvas->drawLine(r.left(), r.bottom(), r.left() + fadeInPx, r.top(), fadeCurvePaint);
            
            // Draw Handle (Triangle)
            SkPath handle;
            float hx = r.left() + fadeInPx;
            float hy = r.top();
            handle.moveTo(hx, hy);
            handle.lineTo(hx - 4, hy + 8);
            handle.lineTo(hx + 4, hy + 8);
            handle.close();
            canvas->drawPath(handle, handlePaint);
        } else {
             // Draw Handle at start (Optional, hidden if 0 or always visible?)
             // Usually visible at corners to allow dragging from 0.
             // Corner handle:
             SkPath handle;
             float hx = r.left();
             float hy = r.top();
             handle.moveTo(hx, hy);
             handle.lineTo(hx, hy + 8);
             handle.lineTo(hx + 8, hy);
             handle.close();
             handlePaint.setColor(withAlpha(SK_ColorWHITE, 0.3f));
             canvas->drawPath(handle, handlePaint);
        }
        
        if (fadeOutPx > 0) {
            // Draw Linear Fade Out
            canvas->drawLine(r.right() - fadeOutPx, r.top(), r.right(), r.bottom(), fadeCurvePaint);
            
            // Draw Handle
            SkPath handle;
            float hx = r.right() - fadeOutPx;
            float hy = r.top();
            handle.moveTo(hx, hy);
            handle.lineTo(hx - 4, hy + 8);
            handle.lineTo(hx + 4, hy + 8);
            handle.close();
            handlePaint.setColor(withAlpha(SK_ColorWHITE, 0.5f));
            canvas->drawPath(handle, handlePaint);
        } else {
             // Corner Handle
             SkPath handle;
             float hx = r.right();
             float hy = r.top();
             handle.moveTo(hx, hy);
             handle.lineTo(hx, hy + 8);
             handle.lineTo(hx - 8, hy);
             handle.close();
             handlePaint.setColor(withAlpha(SK_ColorWHITE, 0.3f));
             canvas->drawPath(handle, handlePaint);
        }
    }

    // 8. CLIP LABEL
    {
        SkPaint pillPaint;
        pillPaint.setAntiAlias(true);
        pillPaint.setColor(withAlpha(SK_ColorBLACK, 0.55f));
        
        SkRect pillRect = SkRect::MakeXYWH(r.left() + 4, r.top() + 4,
                                           std::min(r.width() - 8, 100.0f), 14);
        canvas->drawRRect(SkRRect::MakeRectXY(pillRect, 3, 3), pillPaint);
        
        SkFont labelFont = typography::getSkFont(typography::FONT_XS, FontWeight::Medium);
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SK_ColorWHITE);
        
        juce::String displayName = clipView.clipId;
        if (displayName.length() > 12) {
            displayName = displayName.substring(0, 10) + "...";
        }
        
        canvas->drawString(displayName.toStdString().c_str(), r.left() + 8, r.top() + 14, labelFont, textPaint);
    }
    
    // 9. CLIP TYPE INDICATOR
    {
        float badgeSize = 14.0f;
        SkRect badgeRect = SkRect::MakeXYWH(r.right() - badgeSize - 4,
                                            r.bottom() - badgeSize - 4, badgeSize, badgeSize);
        
        SkPaint badgePaint;
        badgePaint.setAntiAlias(true);
        badgePaint.setColor(withAlpha(baseColor, 0.8f));
        canvas->drawRRect(SkRRect::MakeRectXY(badgeRect, 3, 3), badgePaint);
        
        SkPaint iconPaint;
        iconPaint.setAntiAlias(true);
        iconPaint.setColor(SK_ColorWHITE);
        iconPaint.setStyle(SkPaint::kStroke_Style);
        iconPaint.setStrokeWidth(1.5f);
        
        float cx = badgeRect.centerX();
        float cy = badgeRect.centerY();
        
        if (clipView.isMidi) {
            canvas->drawLine(cx - 2, cy + 3, cx - 2, cy - 2, iconPaint);
            canvas->drawCircle(cx - 3, cy + 2, 2, iconPaint);
        } else {
            canvas->drawLine(cx - 3, cy, cx - 1, cy - 2, iconPaint);
            canvas->drawLine(cx - 1, cy - 2, cx + 1, cy + 2, iconPaint);
            canvas->drawLine(cx + 1, cy + 2, cx + 3, cy, iconPaint);
        }
    }
}

//==============================================================================
// Waveform Drawing
//==============================================================================

void ArrangerRenderer::drawClipWaveform(SkCanvas* canvas, const ClipView& clip, const SkRect& clipRect) {
    using namespace zenith::design;
    
    const WaveformCache* cache = gridUtils_.getWaveformCache(clip.audioFilePath);
    
    if (!cache || !cache->isValid || cache->minPeaks.empty()) {
        // Draw "Loading..." indicator while waveform is being processed
        SkPaint loadingPaint;
        loadingPaint.setColor(design::withAlpha(design::colors::TEXT_TERTIARY, 0.6f));
        loadingPaint.setAntiAlias(true);
        
        SkFont loadingFont = typography::getSkFont(typography::FONT_XS, FontWeight::Regular);
        
        // Draw loading text centered
        const char* loadingText = "Loading waveform...";
        canvas->drawString(loadingText, clipRect.centerX() - 40.0f, clipRect.centerY() + 4.0f, 
                          loadingFont, loadingPaint);
        
        // Draw subtle horizontal line as placeholder
        loadingPaint.setColor(design::withAlpha(design::colors::TEXT_TERTIARY, 0.2f));
        canvas->drawLine(clipRect.left() + 4.0f, clipRect.centerY(), 
                        clipRect.right() - 4.0f, clipRect.centerY(), loadingPaint);
        return;
    }
    
    SkPaint wavePaint;
    wavePaint.setColor(withAlpha(SK_ColorWHITE, 0.85f)); // Crisp white waveform
    wavePaint.setAntiAlias(true);
    
    // Add subtle neon glow to waveform if selected
    if (clip.isSelected) {
        wavePaint.setColor(withAlpha(colors::CYAN, 0.9f));
        wavePaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 1.5f));
    }
    
    float midY = clipRect.centerY();
    float heightScale = clipRect.height() * 0.45f;
    
    size_t numPeaks = std::min(cache->minPeaks.size(), static_cast<size_t>(clipRect.width()));
    float barWidth = clipRect.width() / static_cast<float>(numPeaks);
    
    for (size_t i = 0; i < numPeaks; ++i) {
        float x = clipRect.left() + i * barWidth;
        float top = midY - (cache->maxPeaks[i] * heightScale);
        float bottom = midY - (cache->minPeaks[i] * heightScale);
        
        float barHeight = std::max(2.0f, bottom - top);
        
        SkRect barRect = SkRect::MakeXYWH(x, top, std::max(1.0f, barWidth - 0.5f), barHeight);
        canvas->drawRect(barRect, wavePaint);
    }
    
    // Center line for reference
    SkPaint centerLinePaint;
    centerLinePaint.setColor(withAlpha(SK_ColorWHITE, 0.15f));
    centerLinePaint.setStrokeWidth(0.5f);
    canvas->drawLine(clipRect.left(), midY, clipRect.right(), midY, centerLinePaint);
}

//==============================================================================
// MIDI Blob Drawing
//==============================================================================

void ArrangerRenderer::drawClipMidiBlobs(SkCanvas* canvas, const ClipView& clip, const SkRect& clipRect) {
    using namespace zenith::design;
    
    if (clip.noteBlobs.empty() || clip.lengthBeats <= 0.001) {
        // Draw "Loading..." indicator while MIDI is being processed
        SkPaint loadingPaint;
        loadingPaint.setColor(design::withAlpha(design::colors::TEXT_TERTIARY, 0.6f));
        loadingPaint.setAntiAlias(true);
        
        SkFont loadingFont = typography::getSkFont(typography::FONT_XS, FontWeight::Regular);
        
        // Draw loading text centered
        const char* loadingText = "Loading MIDI...";
        canvas->drawString(loadingText, clipRect.centerX() - 35.0f, clipRect.centerY() + 4.0f, 
                          loadingFont, loadingPaint);
        
        // Draw subtle horizontal line as placeholder
        loadingPaint.setColor(design::withAlpha(design::colors::TEXT_TERTIARY, 0.2f));
        canvas->drawLine(clipRect.left() + 4.0f, clipRect.centerY(), 
                        clipRect.right() - 4.0f, clipRect.centerY(), loadingPaint);
        return;
    }
    
    // Find pitch range
    int minPitch = 127, maxPitch = 0;
    for (const auto& note : clip.noteBlobs) {
        minPitch = std::min(minPitch, note.pitch);
        maxPitch = std::max(maxPitch, note.pitch);
    }
    
    if (maxPitch <= minPitch) {
        maxPitch = minPitch + 12;
    }
    
    float pitchRange = static_cast<float>(maxPitch - minPitch + 2);
    float noteHeight = std::max(2.0f, clipRect.height() / pitchRange);
    float pixelsPerBeat = owner_.pixelsPerBeat;
    
    SkPaint notePaint;
    notePaint.setAntiAlias(true);
    
    for (const auto& note : clip.noteBlobs) {
        float x = clipRect.left() + static_cast<float>(note.startBeats) * pixelsPerBeat;
        float y = clipRect.bottom() - ((note.pitch - minPitch + 1) / pitchRange) * clipRect.height();
        float w = static_cast<float>(note.lengthBeats) * pixelsPerBeat;
        
        w = std::max(2.0f, w);
        
        if (x + w < clipRect.left() || x > clipRect.right())
            continue;
            
        SkRect noteRect = SkRect::MakeXYWH(x, y - noteHeight * 0.5f, w, noteHeight);
        
        // Gradient for 3D effect
        SkPoint pts[2] = {{noteRect.left(), noteRect.top()}, {noteRect.left(), noteRect.bottom()}};
        SkColor noteColors[2] = {
            withAlpha(colors::MAGENTA, 0.9f),
            withAlpha(colors::VIOLET, 0.8f)
        };
        notePaint.setShader(SkGradientShader::MakeLinear(pts, noteColors, nullptr, 2, SkTileMode::kClamp));
        
        canvas->drawRRect(SkRRect::MakeRectXY(noteRect, 2.0f, 2.0f), notePaint);
    }
}

//==============================================================================
// Marquee Drawing
//==============================================================================

void ArrangerRenderer::drawMarquee(SkCanvas* canvas) {
    using namespace zenith::design;
    
    if (!owner_.inputHandler_)
        return;
        
    auto marqueeRect = owner_.inputHandler_->getMarqueeRect();
    if (owner_.inputHandler_->getCurrentDragMode() != DragMode::Marquee || marqueeRect.isEmpty())
        return;
        
    SkRect mRect = SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                                     marqueeRect.getWidth(), marqueeRect.getHeight());
    
    SkPaint marqueePaint;
    marqueePaint.setColor(withAlpha(colors::CYAN, 0.2f));
    marqueePaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(mRect, marqueePaint);
    
    marqueePaint.setColor(colors::CYAN);
    marqueePaint.setStyle(SkPaint::kStroke_Style);
    static const SkScalar dashIntervals[] = {2.0f, 4.0f};
    marqueePaint.setPathEffect(SkDashPathEffect::Make(SkSpan<const SkScalar>(dashIntervals, 2), 0.0f));
    canvas->drawRect(mRect, marqueePaint);
}

//==============================================================================
// Section Track Drawing
//==============================================================================

void ArrangerRenderer::drawSectionTrack(SkCanvas* canvas) {
    if (!owner_.sectionTrack)
        return;
        
    canvas->save();
    canvas->translate(owner_.sectionTrack->getX(), owner_.sectionTrack->getY());
    
    owner_.sectionTrack->setViewContext(owner_.pixelsPerBeat, owner_.viewStartBeats);
    
    SkRect sectionClip = SkRect::MakeWH(owner_.sectionTrack->getWidth(), owner_.sectionTrack->getHeight());
    canvas->clipRect(sectionClip);
    
    owner_.sectionTrack->drawSkia(canvas);
    canvas->restore();
}

//==============================================================================
// Playhead Drawing
//==============================================================================

void ArrangerRenderer::drawPlayhead(SkCanvas* canvas, float width, float height) {
    using namespace zenith::design;
    
    float playheadX = gridUtils_.beatsToX(owner_.playheadBeats_);
    
    if (playheadX < HEADER_WIDTH || playheadX > width)
        return;
        
    SkPaint playheadPaint;
    playheadPaint.setColor(colors::NEON_RED);
    playheadPaint.setStrokeWidth(3.0f); // Wider glow stroke
    playheadPaint.setAntiAlias(true);
    
    // Outer Glow
    playheadPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
    playheadPaint.setAlphaf(0.6f);
    canvas->drawLine(playheadX, 0, playheadX, height, playheadPaint);
    
    // Core Line
    playheadPaint.setMaskFilter(nullptr);
    playheadPaint.setColor(SK_ColorWHITE);
    playheadPaint.setStrokeWidth(1.0f);
    playheadPaint.setAlphaf(1.0f);
    canvas->drawLine(playheadX, 0, playheadX, height, playheadPaint);
    
    // Triangle Cap
    SkPath cap;
    cap.moveTo(playheadX - 6, RULER_HEIGHT);
    cap.lineTo(playheadX + 6, RULER_HEIGHT);
    cap.lineTo(playheadX, RULER_HEIGHT + 8);
    cap.close();
    
    SkPaint capPaint;
    capPaint.setColor(colors::NEON_RED);
    capPaint.setStyle(SkPaint::kFill_Style);
    capPaint.setAntiAlias(true);
    canvas->drawPath(cap, capPaint);
}

//==============================================================================
// Loop Region Drawing
//==============================================================================

//==============================================================================
// Loop Region Drawing
//==============================================================================

void ArrangerRenderer::drawLoopRegion(SkCanvas* canvas, float width, float height) {
    using namespace zenith::design;
    
    if (!owner_.loopEnabled_)
        return;
        
    float loopStartX = gridUtils_.beatsToX(owner_.loopStartBeats_);
    float loopEndX = gridUtils_.beatsToX(owner_.loopEndBeats_);
    
    // Don't draw if loop region is outside visible area
    if (loopEndX < HEADER_WIDTH || loopStartX > width)
        return;
        
    // Clamp to visible area
    float drawStart = std::max(loopStartX, HEADER_WIDTH);
    float drawEnd = std::min(loopEndX, width);
    float drawWidth = drawEnd - drawStart;
    
    if (drawWidth <= 0)
        return;
    
    // 1. LOOP REGION FILL (subtle green tint)
    // Only draw below the ruler (in the track area)
    {
        SkPaint loopFillPaint;
        loopFillPaint.setAntiAlias(true);
        
        // Vertical gradient for depth
        SkPoint pts[2] = {{drawStart, TOP_MARGIN}, {drawStart, height}};
        SkColor fillColors[2] = {
            withAlpha(colors::NEON_GREEN, 0.08f),
            withAlpha(colors::NEON_GREEN, 0.04f)
        };
        loopFillPaint.setShader(SkGradientShader::MakeLinear(
            pts, fillColors, nullptr, 2, SkTileMode::kClamp));
        
        canvas->drawRect(SkRect::MakeXYWH(drawStart, TOP_MARGIN, 
                                          drawWidth, height - TOP_MARGIN), 
                        loopFillPaint);
    }
    
    // 2. LOOP EDGE LINES (bright green)
    {
        SkPaint edgePaint;
        edgePaint.setAntiAlias(true);
        edgePaint.setColor(withAlpha(colors::NEON_GREEN, 0.5f));
        edgePaint.setStrokeWidth(1.5f);
        
        // Left edge (if visible)
        if (loopStartX >= HEADER_WIDTH)
            canvas->drawLine(loopStartX, TOP_MARGIN, loopStartX, height, edgePaint);
        
        // Right edge (if visible)
        if (loopEndX <= width)
            canvas->drawLine(loopEndX, TOP_MARGIN, loopEndX, height, edgePaint);
    }
}

//==============================================================================
// Insertion Guide Drawing
//==============================================================================

void ArrangerRenderer::drawInsertionGuide(SkCanvas* canvas, float height) {
    using namespace zenith::design;
    
    if (!owner_.inputHandler_)
        return;
        
    float insertionGuideX = owner_.inputHandler_->getInsertionGuideX();
    auto editMode = owner_.inputHandler_->getCurrentEditMode();
    auto dragMode = owner_.inputHandler_->getCurrentDragMode();
    
    if (dragMode != DragMode::MoveClips || insertionGuideX < 0.0f)
        return;
        
    if (editMode != EditMode::Ripple && editMode != EditMode::Insert)
        return;
        
    SkPaint guidePaint;
    guidePaint.setColor(editMode == EditMode::Ripple ? colors::NEON_PINK : colors::NEON_GREEN);
    guidePaint.setStrokeWidth(2.0f);
    guidePaint.setAntiAlias(true);
    
    // Neon Glow
    guidePaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
    canvas->drawLine(insertionGuideX, RULER_HEIGHT, insertionGuideX, height, guidePaint);
    
    // Core bright line
    guidePaint.setMaskFilter(nullptr);
    guidePaint.setColor(SK_ColorWHITE);
    guidePaint.setStrokeWidth(1.0f);
    canvas->drawLine(insertionGuideX, RULER_HEIGHT, insertionGuideX, height, guidePaint);
    
    // Mode Label
    SkFont labelFont = typography::getSkFont(typography::FONT_SM, FontWeight::Bold);
    SkPaint labelPaint;
    labelPaint.setColor(SK_ColorWHITE);
    labelPaint.setAntiAlias(true);
    
    juce::String label = (editMode == EditMode::Ripple) ? "RIPPLE" : "INSERT";
    canvas->drawString(label.toStdString().c_str(), insertionGuideX + 5.0f,
                       RULER_HEIGHT + 20.0f, labelFont, labelPaint);
}

} // namespace zenith

#endif // ZENITH_USE_SKIA
