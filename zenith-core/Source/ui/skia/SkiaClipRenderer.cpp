/**
 * @file SkiaClipRenderer.cpp
 * @brief Implementation of GPU-accelerated clip/arranger rendering
 */

#include "SkiaClipRenderer.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkPaint.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkRRect.h>
    #include <include/core/SkShader.h>
    #include <include/core/SkGradientShader.h>
    #include <include/core/SkMaskFilter.h>
    #include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

//==============================================================================
// Constructor/Destructor
//==============================================================================

SkiaClipRenderer::SkiaClipRenderer()
{
    ownedWaveformRenderer_ = std::make_unique<SkiaWaveformRenderer>();
    waveformRenderer_ = ownedWaveformRenderer_.get();

    ownedTextRenderer_ = std::make_unique<SkiaTextRenderer>();
    textRenderer_ = ownedTextRenderer_.get();
}

SkiaClipRenderer::~SkiaClipRenderer() = default;

//==============================================================================
// Main rendering
//==============================================================================

void SkiaClipRenderer::renderArrangerView(SkCanvas* canvas,
                                         const std::vector<ClipVisual>& clips,
                                         const std::vector<TrackLaneVisual>& tracks,
                                         const SkRect& bounds,
                                         float pixelsPerBeat,
                                         int trackHeight,
                                         double viewStartBeats,
                                         int firstVisibleTrack,
                                         const ClipRenderOptions& options)
{
    if (!canvas)
        return;

    // Layer 1: Track lanes background
    renderTrackLanes(canvas, tracks, bounds, trackHeight, firstVisibleTrack, options);

    // Layer 2: Timeline grid
    if (options.showGrid)
    {
        renderTimelineGrid(canvas, bounds, pixelsPerBeat, viewStartBeats, options);
    }

    // Layer 3: Clips
    renderClips(canvas, clips, bounds, options);
}

//==============================================================================
// Track lanes
//==============================================================================

void SkiaClipRenderer::renderTrackLanes(SkCanvas* canvas,
                                       const std::vector<TrackLaneVisual>& tracks,
                                       const SkRect& bounds,
                                       int trackHeight,
                                       int firstVisibleTrack,
                                       const ClipRenderOptions& options)
{
    if (!canvas)
        return;

    const auto& theme = SkiaTheme::getInstance();

    for (size_t i = firstVisibleTrack; i < tracks.size(); ++i)
    {
        const auto& track = tracks[i];

        if (!track.bounds.intersects(bounds))
            continue;

        // Track lane background
        SkPaint lanePaint;
        lanePaint.setAntiAlias(true);
        lanePaint.setColor(options.trackLaneColor);
        canvas->drawRect(track.bounds, lanePaint);

        // Track divider
        SkPaint dividerPaint;
        dividerPaint.setAntiAlias(true);
        dividerPaint.setStyle(SkPaint::kStroke_Style);
        dividerPaint.setStrokeWidth(1.0f);
        dividerPaint.setColor(options.trackDividerColor);
        canvas->drawLine(track.bounds.left(), track.bounds.bottom(),
                        track.bounds.right(), track.bounds.bottom(),
                        dividerPaint);

        // Track name (left side)
        if (textRenderer_ && !track.trackName.isEmpty())
        {
            SkRect nameRect = SkRect::MakeXYWH(track.bounds.left(), track.bounds.top(),
                                              150.0f, track.bounds.height());

            // Background for name
            SkPaint nameBgPaint;
            nameBgPaint.setColor(options.trackNameBgColor);
            canvas->drawRect(nameRect, nameBgPaint);

            // Text
            TextRenderOptions textOpts;
            textOpts.color = theme.getColors().textPrimary;
            textOpts.effects = TextEffect::Shadow;

            textRenderer_->drawTextCentered(canvas, track.trackName, nameRect,
                                           TextStyle::Regular, textOpts);
        }
    }
}

//==============================================================================
// Timeline grid
//==============================================================================

void SkiaClipRenderer::renderTimelineGrid(SkCanvas* canvas,
                                         const SkRect& bounds,
                                         float pixelsPerBeat,
                                         double viewStartBeats,
                                         const ClipRenderOptions& options)
{
    if (!canvas)
        return;

    SkPaint gridPaint;
    gridPaint.setAntiAlias(true);
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setStrokeWidth(0.5f);

    // Calculate visible beat range
    int startBeat = static_cast<int>(viewStartBeats);
    int endBeat = static_cast<int>(viewStartBeats + bounds.width() / pixelsPerBeat) + 1;

    // Vertical grid lines (beats)
    for (int beat = startBeat; beat <= endBeat; ++beat)
    {
        float x = bounds.left() + (beat - viewStartBeats) * pixelsPerBeat;

        if (x < bounds.left() || x > bounds.right())
            continue;

        // Bar lines (every 4 beats) are more prominent
        if (beat % 4 == 0)
            gridPaint.setColor(options.barLineColor);
        else
            gridPaint.setColor(options.gridLineColor);

        canvas->drawLine(x, bounds.top(), x, bounds.bottom(), gridPaint);
    }
}

//==============================================================================
// Clip rendering
//==============================================================================

void SkiaClipRenderer::renderClips(SkCanvas* canvas,
                                  const std::vector<ClipVisual>& clips,
                                  const SkRect& bounds,
                                  const ClipRenderOptions& options)
{
    if (!canvas)
        return;

    for (const auto& clip : clips)
    {
        renderClip(canvas, clip, bounds, options);
    }
}

void SkiaClipRenderer::renderClip(SkCanvas* canvas,
                                 const ClipVisual& clip,
                                 const SkRect& bounds,
                                 const ClipRenderOptions& options)
{
    if (!canvas || !clip.bounds.intersects(bounds))
        return;

    // Apply hover scale animation if enabled
    SkRect clipRect = clip.bounds;
    if (options.enableHoverScale && clip.isHovered)
    {
        float scale = 1.0f + (clip.hoverProgress * 0.05f);  // Scale up to 5%
        float offsetY = (clipRect.height() * (scale - 1.0f)) / 2.0f;
        clipRect.outset(0, offsetY);
    }

    clipRect.inset(1.0f, 1.0f);  // Visual separation

    // Layer 1: Glow (for selected/hovered)
    if (options.enableGlow && (clip.isSelected || clip.isHovered))
    {
        SkColor glowColor = clip.isSelected ? options.selectedClipColor : getClipColor(clip, options);
        float glowIntensity = clip.isSelected ? clip.selectProgress : clip.hoverProgress;
        renderClipGlow(canvas, clipRect, glowColor, glowIntensity);
    }

    // Layer 2: Background/waveform
    renderClipBackground(canvas, clipRect, clip, options);

    if (options.showWaveforms && !clip.isMidi && clip.waveformData && clip.waveformData->isValid())
    {
        renderClipWaveform(canvas, clipRect, clip, options);
    }

    // Layer 3: Clip name
    if (options.showClipNames)
    {
        renderClipName(canvas, clipRect, clip, options);
    }

    // Layer 4: Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(clip.isSelected ? 2.5f : 1.0f);

    if (clip.isSelected)
    {
        // Animated selection border
        float pulseIntensity = 0.6f + 0.4f * clip.selectProgress;
        uint8_t alpha = static_cast<uint8_t>(255 * pulseIntensity);
        borderPaint.setColor(SkColorSetA(options.selectedClipColor, alpha));
    }
    else
    {
        borderPaint.setColor(SK_ColorBLACK);
    }

    SkRRect roundRect = SkRRect::MakeRectXY(clipRect, 4.0f, 4.0f);
    canvas->drawRRect(roundRect, borderPaint);
}

void SkiaClipRenderer::renderClipBackground(SkCanvas* canvas,
                                           const SkRect& clipRect,
                                           const ClipVisual& clip,
                                           const ClipRenderOptions& options)
{
    const auto& theme = SkiaTheme::getInstance();
    const auto& depthStyle = theme.getDepthStyle();

    // Shadow for 3D depth
    if (options.use3DDepth)
    {
        SkPaint shadowPaint;
        shadowPaint.setAntiAlias(true);
        uint8_t alpha = static_cast<uint8_t>(255 * depthStyle.shadowOpacity);
        shadowPaint.setColor(SkColorSetARGB(alpha, 0, 0, 0));

        if (depthStyle.shadowBlur > 0.0f)
        {
            shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, depthStyle.shadowBlur));
        }

        SkRect shadowRect = clipRect;
        shadowRect.offset(0, depthStyle.shadowOffsetY);
        SkRRect shadowRRect = SkRRect::MakeRectXY(shadowRect, 4.0f, 4.0f);
        canvas->drawRRect(shadowRRect, shadowPaint);
    }

    // Main clip background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);

    SkColor clipColor = getClipColor(clip, options);

    if (options.useGradients && depthStyle.useGradients)
    {
        // Gradient for 3D effect
        SkColor topColor = clipColor;
        SkColor bottomColor = SkColorSetARGB(
            SkColorGetA(clipColor),
            static_cast<uint8_t>(SkColorGetR(clipColor) * 0.75f),
            static_cast<uint8_t>(SkColorGetG(clipColor) * 0.75f),
            static_cast<uint8_t>(SkColorGetB(clipColor) * 0.75f)
        );

        SkPoint points[2] = {
            SkPoint::Make(clipRect.centerX(), clipRect.top()),
            SkPoint::Make(clipRect.centerX(), clipRect.bottom())
        };
        SkColor colors[2] = {topColor, bottomColor};
        SkScalar positions[2] = {0.0f, 1.0f};

        sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
            points, colors, positions, 2, SkTileMode::kClamp
        );
        bgPaint.setShader(shader);
    }
    else
    {
        bgPaint.setColor(clipColor);
    }

    SkRRect roundRect = SkRRect::MakeRectXY(clipRect, 4.0f, 4.0f);
    canvas->drawRRect(roundRect, bgPaint);

    // Top highlight for 3D
    if (options.use3DDepth && depthStyle.highlightOpacity > 0.0f)
    {
        SkPaint highlightPaint;
        highlightPaint.setAntiAlias(true);
        highlightPaint.setStyle(SkPaint::kStroke_Style);
        highlightPaint.setStrokeWidth(1.5f);
        uint8_t alpha = static_cast<uint8_t>(255 * depthStyle.highlightOpacity);
        highlightPaint.setColor(SkColorSetARGB(alpha, 255, 255, 255));

        SkPath highlightPath;
        highlightPath.moveTo(clipRect.left() + 4.0f, clipRect.top() + 1.5f);
        highlightPath.lineTo(clipRect.right() - 4.0f, clipRect.top() + 1.5f);
        canvas->drawPath(highlightPath, highlightPaint);
    }
}

void SkiaClipRenderer::renderClipWaveform(SkCanvas* canvas,
                                         const SkRect& clipRect,
                                         const ClipVisual& clip,
                                         const ClipRenderOptions& options)
{
    juce::ignoreUnused(options);

    if (!waveformRenderer_ || !clip.waveformData)
        return;

    // Clip waveform to clip bounds
    canvas->save();
    SkRRect clipRRect = SkRRect::MakeRectXY(clipRect, 4.0f, 4.0f);
    canvas->clipRRect(clipRRect, SkClipOp::kIntersect, true);

    // Render waveform
    WaveformRenderOptions waveOpts = WaveformRenderOptions::producer();
    waveOpts.fillColor = SkColorSetARGB(120, 255, 255, 255);  // Semi-transparent white
    waveOpts.useGradient = false;
    waveOpts.showCenterLine = false;

    waveformRenderer_->render(canvas, *clip.waveformData, clipRect, waveOpts);

    canvas->restore();
}

void SkiaClipRenderer::renderClipGlow(SkCanvas* canvas,
                                     const SkRect& clipRect,
                                     SkColor glowColor,
                                     float intensity)
{
    const int glowLayers = 2;

    for (int i = 0; i < glowLayers; ++i)
    {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setStyle(SkPaint::kStroke_Style);

        float layerIntensity = intensity * (1.0f - i * 0.3f);
        float layerBlur = 6.0f + i * 4.0f;
        float layerStroke = 3.0f + i * 2.0f;

        uint8_t alpha = static_cast<uint8_t>(180 * layerIntensity);
        glowPaint.setColor(SkColorSetA(glowColor, alpha));
        glowPaint.setStrokeWidth(layerStroke);

        if (layerBlur > 0.0f)
        {
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, layerBlur));
        }

        SkRRect roundRect = SkRRect::MakeRectXY(clipRect, 4.0f, 4.0f);
        canvas->drawRRect(roundRect, glowPaint);
    }
}

void SkiaClipRenderer::renderClipName(SkCanvas* canvas,
                                     const SkRect& clipRect,
                                     const ClipVisual& clip,
                                     const ClipRenderOptions& options)
{
    juce::ignoreUnused(options);

    if (!textRenderer_ || clip.name.isEmpty())
        return;

    // Only show name if clip is wide enough
    if (clipRect.width() < 40.0f)
        return;

    SkRect textRect = SkRect::MakeLTRB(clipRect.left() + 6.0f, clipRect.top() + 4.0f,
                                       clipRect.right() - 6.0f, clipRect.top() + 20.0f);

    TextRenderOptions textOpts;
    textOpts.color = SK_ColorWHITE;
    textOpts.effects = TextEffect::Shadow;
    textOpts.shadowBlur = 2.0f;
    textOpts.shadowOpacity = 0.6f;

    textRenderer_->drawText(canvas, clip.name, textRect.left(), textRect.centerY() + 4.0f,
                           TextStyle::Small, textOpts);
}

SkColor SkiaClipRenderer::getClipColor(const ClipVisual& clip,
                                      const ClipRenderOptions& options) const
{
    if (clip.isMuted)
        return options.mutedClipColor;

    if (clip.isSelected)
        return options.selectedClipColor;

    if (clip.isMidi)
        return options.midiClipColor;
    else
        return options.audioClipColor;
}

//==============================================================================
// Playhead and loop region
//==============================================================================

void SkiaClipRenderer::renderPlayhead(SkCanvas* canvas,
                                     const SkRect& bounds,
                                     double playheadBeats,
                                     float pixelsPerBeat,
                                     double viewStartBeats,
                                     const ClipRenderOptions& options)
{
    if (!canvas)
        return;

    float x = bounds.left() + static_cast<float>((playheadBeats - viewStartBeats) * pixelsPerBeat);

    if (x < bounds.left() || x > bounds.right())
        return;

    SkPaint playheadPaint;
    playheadPaint.setAntiAlias(true);
    playheadPaint.setStyle(SkPaint::kStroke_Style);
    playheadPaint.setStrokeWidth(2.0f);
    playheadPaint.setColor(options.playheadColor);

    canvas->drawLine(x, bounds.top(), x, bounds.bottom(), playheadPaint);

    // Playhead triangle at top
    SkPath trianglePath;
    trianglePath.moveTo(x, bounds.top());
    trianglePath.lineTo(x - 6.0f, bounds.top() + 10.0f);
    trianglePath.lineTo(x + 6.0f, bounds.top() + 10.0f);
    trianglePath.close();

    SkPaint trianglePaint;
    trianglePaint.setAntiAlias(true);
    trianglePaint.setColor(options.playheadColor);
    canvas->drawPath(trianglePath, trianglePaint);
}

void SkiaClipRenderer::renderLoopRegion(SkCanvas* canvas,
                                       const SkRect& bounds,
                                       double loopStartBeats,
                                       double loopEndBeats,
                                       float pixelsPerBeat,
                                       double viewStartBeats,
                                       const ClipRenderOptions& options)
{
    if (!canvas || loopEndBeats <= loopStartBeats)
        return;

    float x1 = bounds.left() + static_cast<float>((loopStartBeats - viewStartBeats) * pixelsPerBeat);
    float x2 = bounds.left() + static_cast<float>((loopEndBeats - viewStartBeats) * pixelsPerBeat);

    SkRect loopRect = SkRect::MakeLTRB(x1, bounds.top(), x2, bounds.bottom());

    if (!loopRect.intersects(bounds))
        return;

    SkPaint loopPaint;
    loopPaint.setAntiAlias(true);
    loopPaint.setColor(options.loopRegionColor);
    canvas->drawRect(loopRect, loopPaint);

    // Loop region borders
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(2.0f);
    borderPaint.setColor(SkColorSetA(options.loopRegionColor, 200));

    canvas->drawLine(x1, bounds.top(), x1, bounds.bottom(), borderPaint);
    canvas->drawLine(x2, bounds.top(), x2, bounds.bottom(), borderPaint);
}

//==============================================================================
// Animation
//==============================================================================

void SkiaClipRenderer::updateAnimations(std::vector<ClipVisual>& clips, float deltaTime)
{
    const auto& physics = SkiaTheme::getInstance().getTimelinePhysics();

    for (auto& clip : clips)
    {
        // Hover animation
        float hoverTarget = clip.isHovered ? 1.0f : 0.0f;
        updateSpringAnimation(clip.hoverProgress, clip.hoverVelocity, hoverTarget,
                            deltaTime, physics.stiffness, physics.damping);

        // Selection animation with pulse
        if (clip.isSelected)
        {
            float pulseTarget = 0.8f + 0.2f * std::sin(juce::Time::getCurrentTime().toMilliseconds() / 400.0f);
            updateSpringAnimation(clip.selectProgress, clip.selectVelocity, pulseTarget,
                                deltaTime, physics.stiffness * 0.6f, physics.damping * 0.7f);
        }
        else
        {
            updateSpringAnimation(clip.selectProgress, clip.selectVelocity, 0.0f,
                                deltaTime, physics.stiffness, physics.damping);
        }

        // Scale animation
        float scaleTarget = clip.isHovered ? 1.05f : 1.0f;
        updateSpringAnimation(clip.scaleProgress, clip.scaleVelocity, scaleTarget,
                            deltaTime, physics.stiffness * 1.2f, physics.damping);
    }
}

void SkiaClipRenderer::updateSpringAnimation(float& progress, float& velocity,
                                             float target, float deltaTime,
                                             float stiffness, float damping)
{
    float displacement = progress - target;
    float force = -stiffness * displacement - damping * velocity;

    velocity += force * deltaTime;
    progress += velocity * deltaTime;
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
