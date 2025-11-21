/**
 * @file SkiaPianoRollRenderer.cpp
 * @brief Implementation of GPU-accelerated piano roll renderer
 */

#include "SkiaPianoRollRenderer.h"

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkPaint.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkShader.h>
    #include <include/core/SkGradientShader.h>
    #include <include/core/SkMaskFilter.h>
    #include <include/core/SkRRect.h>
    #include <include/effects/SkGradientShader.h>
#endif

#include <cmath>

namespace zenith {

#ifdef ZENITH_USE_SKIA

//==============================================================================
// Constructor/Destructor
//==============================================================================

SkiaPianoRollRenderer::SkiaPianoRollRenderer()
{
    // Create owned text renderer if none provided
    ownedTextRenderer_ = std::make_unique<SkiaTextRenderer>();
    textRenderer_ = ownedTextRenderer_.get();
}

SkiaPianoRollRenderer::~SkiaPianoRollRenderer() = default;

//==============================================================================
// Main rendering
//==============================================================================

void SkiaPianoRollRenderer::render(SkCanvas* canvas,
                                   const std::vector<MidiNoteVisual>& notes,
                                   const SkRect& bounds,
                                   float pixelsPerBeat,
                                   float pixelsPerPitch,
                                   float pianoKeysWidth,
                                   int lowestPitch,
                                   int highestPitch,
                                   const PianoRollRenderOptions& options)
{
    if (!canvas)
        return;

    // Split bounds into piano keys and note area
    SkRect pianoBounds = SkRect::MakeLTRB(bounds.left(), bounds.top(),
                                          bounds.left() + pianoKeysWidth, bounds.bottom());
    SkRect noteBounds = SkRect::MakeLTRB(bounds.left() + pianoKeysWidth, bounds.top(),
                                         bounds.right(), bounds.bottom());

    // Render piano keys
    renderPianoKeys(canvas, pianoBounds, pixelsPerPitch, lowestPitch, highestPitch, options);

    // Render grid
    if (options.showGrid)
    {
        renderGrid(canvas, noteBounds, pixelsPerBeat, pixelsPerPitch,
                  lowestPitch, highestPitch, options);
    }

    // Render notes
    renderNotes(canvas, notes, noteBounds, pixelsPerPitch, lowestPitch, options);
}

//==============================================================================
// Grid rendering
//==============================================================================

void SkiaPianoRollRenderer::renderGrid(SkCanvas* canvas,
                                      const SkRect& bounds,
                                      float pixelsPerBeat,
                                      float pixelsPerPitch,
                                      int lowestPitch,
                                      int highestPitch,
                                      const PianoRollRenderOptions& options)
{
    if (!canvas)
        return;

    SkPaint gridPaint;
    gridPaint.setAntiAlias(true);
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setStrokeWidth(0.5f);

    // Vertical grid lines (beats)
    int maxBeats = static_cast<int>(bounds.width() / pixelsPerBeat) + 1;
    for (int beat = 0; beat <= maxBeats; ++beat)
    {
        float x = bounds.left() + beat * pixelsPerBeat;

        if (x < bounds.left() || x > bounds.right())
            continue;

        // Bar lines (every 4 beats) are brighter
        if (beat % 4 == 0)
            gridPaint.setColor(options.barLineColor);
        else
            gridPaint.setColor(options.gridLineColor);

        canvas->drawLine(x, bounds.top(), x, bounds.bottom(), gridPaint);
    }

    // Horizontal grid lines (pitches)
    for (int pitch = lowestPitch; pitch <= highestPitch; ++pitch)
    {
        float y = bounds.top() + (highestPitch - pitch) * pixelsPerPitch;

        if (y < bounds.top() || y > bounds.bottom())
            continue;

        // Octave lines (C notes) are more visible
        if (options.showOctaveLines && (pitch % 12) == 0)
            gridPaint.setColor(options.octaveLineColor);
        else
            gridPaint.setColor(options.gridLineColor);

        canvas->drawLine(bounds.left(), y, bounds.right(), y, gridPaint);
    }
}

//==============================================================================
// Piano keys rendering
//==============================================================================

void SkiaPianoRollRenderer::renderPianoKeys(SkCanvas* canvas,
                                           const SkRect& bounds,
                                           float pixelsPerPitch,
                                           int lowestPitch,
                                           int highestPitch,
                                           const PianoRollRenderOptions& options)
{
    if (!canvas)
        return;

    const auto& theme = SkiaTheme::getInstance();
    const auto& depthStyle = theme.getDepthStyle();

    for (int pitch = lowestPitch; pitch <= highestPitch; ++pitch)
    {
        float y = bounds.top() + (highestPitch - pitch) * pixelsPerPitch;
        SkRect keyRect = SkRect::MakeXYWH(bounds.left(), y, bounds.width(), pixelsPerPitch);

        // Determine key color
        bool isBlack = isBlackKey(pitch);
        SkColor keyColor = isBlack ? options.blackKeyColor : options.whiteKeyColor;

        // Draw key background with 3D effect
        SkPaint keyPaint;
        keyPaint.setAntiAlias(true);

        if (depthStyle.useGradients)
        {
            // Gradient for 3D effect
            SkColor topColor = keyColor;
            SkColor bottomColor = SkColorSetARGB(
                SkColorGetA(keyColor),
                static_cast<uint8_t>(SkColorGetR(keyColor) * 0.85f),
                static_cast<uint8_t>(SkColorGetG(keyColor) * 0.85f),
                static_cast<uint8_t>(SkColorGetB(keyColor) * 0.85f)
            );

            SkPoint points[2] = {
                SkPoint::Make(keyRect.centerX(), keyRect.top()),
                SkPoint::Make(keyRect.centerX(), keyRect.bottom())
            };
            SkColor colors[2] = {topColor, bottomColor};
            SkScalar positions[2] = {0.0f, 1.0f};

            sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
                points, colors, positions, 2, SkTileMode::kClamp
            );
            keyPaint.setShader(shader);
        }
        else
        {
            keyPaint.setColor(keyColor);
        }

        canvas->drawRect(keyRect, keyPaint);

        // Draw border
        SkPaint borderPaint;
        borderPaint.setAntiAlias(true);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(0.5f);
        borderPaint.setColor(options.keyBorderColor);
        canvas->drawRect(keyRect, borderPaint);

        // Draw note name for C notes
        if ((pitch % 12) == 0 && textRenderer_ && options.useFlashyText)
        {
            juce::String noteName = getPitchName(pitch);

            TextRenderOptions textOpts;
            textOpts.color = theme.getColors().textSecondary;
            textOpts.effects = TextEffect::Shadow;
            textOpts.shadowOffsetY = 1.0f;
            textOpts.shadowBlur = 1.5f;
            textOpts.shadowOpacity = 0.4f;

            textRenderer_->drawTextCentered(canvas, noteName, keyRect,
                                           TextStyle::Small, textOpts);
        }
    }

    // Right border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(SK_ColorBLACK);
    canvas->drawLine(bounds.right(), bounds.top(), bounds.right(), bounds.bottom(), borderPaint);
}

//==============================================================================
// Note rendering
//==============================================================================

void SkiaPianoRollRenderer::renderNotes(SkCanvas* canvas,
                                       const std::vector<MidiNoteVisual>& notes,
                                       const SkRect& bounds,
                                       float pixelsPerPitch,
                                       int lowestPitch,
                                       const PianoRollRenderOptions& options)
{
    if (!canvas)
        return;

    // Render all notes
    for (const auto& note : notes)
    {
        renderNote(canvas, note, bounds, pixelsPerPitch, lowestPitch, options);
    }
}

void SkiaPianoRollRenderer::renderNote(SkCanvas* canvas,
                                      const MidiNoteVisual& note,
                                      const SkRect& bounds,
                                      float pixelsPerPitch,
                                      int lowestPitch,
                                      const PianoRollRenderOptions& options)
{
    // Skip if out of bounds
    if (!note.bounds.intersects(bounds))
        return;

    // Reduce note rect slightly for visual separation
    SkRect noteRect = note.bounds;
    noteRect.inset(1.0f, 1.0f);

    renderNoteWithEffects(canvas, noteRect, note, options);
}

void SkiaPianoRollRenderer::renderNoteWithEffects(SkCanvas* canvas,
                                                 const SkRect& noteRect,
                                                 const MidiNoteVisual& note,
                                                 const PianoRollRenderOptions& options)
{
    const auto& theme = SkiaTheme::getInstance();
    const auto& depthStyle = theme.getDepthStyle();

    // Layer 1: Shadow (3D depth)
    if (options.use3DDepth)
    {
        renderNoteShadow(canvas, noteRect, options);
    }

    // Layer 2: Glow (for selected/hovered notes)
    if (options.enableGlow && (note.isSelected || note.isHovered))
    {
        SkColor glowColor = note.isSelected ? options.noteColorSelected : options.noteColorHover;
        float glowIntensity = note.isSelected ? note.selectProgress : note.hoverProgress;
        renderNoteGlow(canvas, noteRect, glowColor, glowIntensity);
    }

    // Layer 3: Main note fill
    SkPaint notePaint;
    notePaint.setAntiAlias(true);

    SkColor baseColor = getNoteColor(note, options);

    // Apply velocity intensity if enabled
    if (options.showVelocityColors)
    {
        float velocityFactor = note.velocity / 127.0f;
        uint8_t alpha = static_cast<uint8_t>(SkColorGetA(baseColor) * velocityFactor);
        baseColor = SkColorSetA(baseColor, alpha);
    }

    if (options.useGradientNotes && depthStyle.useGradients)
    {
        // Gradient for 3D appearance
        SkColor topColor = baseColor;
        SkColor bottomColor = SkColorSetARGB(
            SkColorGetA(baseColor),
            static_cast<uint8_t>(SkColorGetR(baseColor) * 0.7f),
            static_cast<uint8_t>(SkColorGetG(baseColor) * 0.7f),
            static_cast<uint8_t>(SkColorGetB(baseColor) * 0.7f)
        );

        SkPoint points[2] = {
            SkPoint::Make(noteRect.centerX(), noteRect.top()),
            SkPoint::Make(noteRect.centerX(), noteRect.bottom())
        };
        SkColor colors[2] = {topColor, bottomColor};
        SkScalar positions[2] = {0.0f, 1.0f};

        sk_sp<SkShader> shader = SkGradientShader::MakeLinear(
            points, colors, positions, 2, SkTileMode::kClamp
        );
        notePaint.setShader(shader);
    }
    else
    {
        notePaint.setColor(baseColor);
    }

    // Draw rounded rectangle for notes
    SkRRect roundRect = SkRRect::MakeRectXY(noteRect, 2.0f, 2.0f);
    canvas->drawRRect(roundRect, notePaint);

    // Layer 4: Highlight (top edge for 3D effect)
    if (options.use3DDepth && depthStyle.highlightOpacity > 0.0f)
    {
        SkPaint highlightPaint;
        highlightPaint.setAntiAlias(true);
        highlightPaint.setStyle(SkPaint::kStroke_Style);
        highlightPaint.setStrokeWidth(1.0f);

        uint8_t alpha = static_cast<uint8_t>(255 * depthStyle.highlightOpacity);
        highlightPaint.setColor(SkColorSetARGB(alpha, 255, 255, 255));

        SkPath highlightPath;
        highlightPath.moveTo(noteRect.left() + 2.0f, noteRect.top() + 1.0f);
        highlightPath.lineTo(noteRect.right() - 2.0f, noteRect.top() + 1.0f);
        canvas->drawPath(highlightPath, highlightPaint);
    }

    // Layer 5: Border
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(note.isSelected ? 2.0f : 1.0f);

    if (note.isSelected)
    {
        // Animated border for selection
        float pulseIntensity = 0.5f + 0.5f * note.selectProgress;
        uint8_t alpha = static_cast<uint8_t>(255 * pulseIntensity);
        borderPaint.setColor(SkColorSetA(options.noteColorSelected, alpha));
    }
    else
    {
        borderPaint.setColor(SK_ColorBLACK);
    }

    canvas->drawRRect(roundRect, borderPaint);
}

//==============================================================================
// Effect helpers
//==============================================================================

void SkiaPianoRollRenderer::renderNoteShadow(SkCanvas* canvas,
                                            const SkRect& noteRect,
                                            const PianoRollRenderOptions& options)
{
    juce::ignoreUnused(options);

    const auto& depthStyle = SkiaTheme::getInstance().getDepthStyle();

    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);

    uint8_t alpha = static_cast<uint8_t>(255 * depthStyle.shadowOpacity);
    shadowPaint.setColor(SkColorSetARGB(alpha, 0, 0, 0));

    if (depthStyle.shadowBlur > 0.0f)
    {
        shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, depthStyle.shadowBlur));
    }

    SkRect shadowRect = noteRect;
    shadowRect.offset(0, depthStyle.shadowOffsetY);

    SkRRect roundRect = SkRRect::MakeRectXY(shadowRect, 2.0f, 2.0f);
    canvas->drawRRect(roundRect, shadowPaint);
}

void SkiaPianoRollRenderer::renderNoteGlow(SkCanvas* canvas,
                                          const SkRect& noteRect,
                                          SkColor glowColor,
                                          float intensity)
{
    // Multiple glow layers for flashy effect
    const int glowLayers = 3;

    for (int i = 0; i < glowLayers; ++i)
    {
        SkPaint glowPaint;
        glowPaint.setAntiAlias(true);
        glowPaint.setStyle(SkPaint::kStroke_Style);

        float layerIntensity = intensity * (1.0f - i * 0.25f);
        float layerBlur = 5.0f + i * 3.0f;
        float layerStroke = 2.0f + i * 1.5f;

        uint8_t alpha = static_cast<uint8_t>(200 * layerIntensity);
        glowPaint.setColor(SkColorSetA(glowColor, alpha));
        glowPaint.setStrokeWidth(layerStroke);

        if (layerBlur > 0.0f)
        {
            glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, layerBlur));
        }

        SkRRect roundRect = SkRRect::MakeRectXY(noteRect, 2.0f, 2.0f);
        canvas->drawRRect(roundRect, glowPaint);
    }
}

SkColor SkiaPianoRollRenderer::getNoteColor(const MidiNoteVisual& note,
                                           const PianoRollRenderOptions& options) const
{
    if (note.isMuted)
        return options.noteColorMuted;

    if (note.isSelected)
        return options.noteColorSelected;

    if (note.isHovered)
    {
        // Blend between default and hover color
        float t = note.hoverProgress;
        SkColor c1 = options.noteColorDefault;
        SkColor c2 = options.noteColorHover;

        uint8_t a = static_cast<uint8_t>(SkColorGetA(c1) * (1.0f - t) + SkColorGetA(c2) * t);
        uint8_t r = static_cast<uint8_t>(SkColorGetR(c1) * (1.0f - t) + SkColorGetR(c2) * t);
        uint8_t g = static_cast<uint8_t>(SkColorGetG(c1) * (1.0f - t) + SkColorGetG(c2) * t);
        uint8_t b = static_cast<uint8_t>(SkColorGetB(c1) * (1.0f - t) + SkColorGetB(c2) * t);

        return SkColorSetARGB(a, r, g, b);
    }

    return options.noteColorDefault;
}

//==============================================================================
// Animation
//==============================================================================

void SkiaPianoRollRenderer::updateAnimations(std::vector<MidiNoteVisual>& notes, float deltaTime)
{
    const auto& physics = SkiaTheme::getInstance().getWaveformPhysics(); // Use precise physics for MIDI

    for (auto& note : notes)
    {
        // Hover animation
        float hoverTarget = note.isHovered ? 1.0f : 0.0f;
        updateSpringAnimation(note.hoverProgress, note.hoverVelocity, hoverTarget,
                            deltaTime, physics.stiffness, physics.damping);

        // Selection animation (with pulse)
        if (note.isSelected)
        {
            // Pulse effect: oscillate between 0.7 and 1.0
            float pulseTarget = 0.85f + 0.15f * std::sin(juce::Time::getCurrentTime().toMilliseconds() / 300.0f);
            updateSpringAnimation(note.selectProgress, note.selectVelocity, pulseTarget,
                                deltaTime, physics.stiffness * 0.5f, physics.damping * 0.8f);
        }
        else
        {
            updateSpringAnimation(note.selectProgress, note.selectVelocity, 0.0f,
                                deltaTime, physics.stiffness, physics.damping);
        }
    }
}

void SkiaPianoRollRenderer::updateSpringAnimation(float& progress, float& velocity,
                                                  float target, float deltaTime,
                                                  float stiffness, float damping)
{
    // Damped spring physics: F = -k*x - b*v
    float displacement = progress - target;
    float force = -stiffness * displacement - damping * velocity;

    velocity += force * deltaTime;
    progress += velocity * deltaTime;

    // Clamp to valid range
    progress = juce::jlimit(0.0f, 1.0f, progress);
}

//==============================================================================
// Helper methods
//==============================================================================

bool SkiaPianoRollRenderer::isBlackKey(int pitch) const
{
    int noteInOctave = pitch % 12;
    return (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
            noteInOctave == 8 || noteInOctave == 10);
}

juce::String SkiaPianoRollRenderer::getPitchName(int pitch) const
{
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (pitch / 12) - 1;
    int noteInOctave = pitch % 12;

    return juce::String(noteNames[noteInOctave]) + juce::String(octave);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
