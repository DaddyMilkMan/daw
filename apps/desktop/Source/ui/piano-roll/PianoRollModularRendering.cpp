/*
  ==============================================================================
    PianoRollModularRendering.cpp
    Modular rendering functions for the Piano Roll
  ==============================================================================
*/

#include "Engine.h"
#include "PianoRollComponent.h"
#include "ZenithDesignSystem.h"
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <effects/SkDashPathEffect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

void PianoRollComponent::drawRuler(SkCanvas *canvas, float width) {
    using namespace design;
    SkRect rulerRect = SkRect::MakeXYWH(0, 0, width, RULER_HEIGHT);

    // Ruler background with gradient
    SkPoint gradPts[] = {{0, 0}, {0, RULER_HEIGHT}};
    SkColor gradColors[] = {colors::BG_DARK, colors::BG_DARKER};
    generalPaint_.setShader(SkGradientShader::MakeLinear(
        gradPts, gradColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(rulerRect, generalPaint_);
    generalPaint_.setShader(nullptr);

    // Bottom border
    borderPaint_.setColor(colors::BORDER_DEFAULT);
    borderPaint_.setStrokeWidth(1.0f);
    canvas->drawLine(0, RULER_HEIGHT - 1, width, RULER_HEIGHT - 1, borderPaint_);

    // Calculate visible beat range
    double visibleStartBeat = viewStartBeats;
    double visibleEndBeat = pixelsToBeats(width);

    // Determine grid density based on zoom
    double beatsPerBar = 4.0;
    double barStep = 1.0;
    double beatSubdiv = 1.0;

    if (pixelsPerBeat < 10.0) {
      barStep = 4.0;
      beatSubdiv = 4.0;
    } else if (pixelsPerBeat < 30.0) {
      barStep = 1.0;
      beatSubdiv = 1.0;
    } else if (pixelsPerBeat < 80.0) {
      beatSubdiv = 0.5;
    } else {
      beatSubdiv = 0.25;
    }

    double startBar = std::floor(visibleStartBeat / beatsPerBar) * beatsPerBar;

    for (double beat = startBar; beat <= visibleEndBeat; beat += beatSubdiv) {
      float x = PIANO_WIDTH + beatsToPixels(beat);
      if (x < PIANO_WIDTH) continue;

      int barNum = static_cast<int>(beat / beatsPerBar) + 1;
      double beatInBar = std::fmod(beat, beatsPerBar);
      bool isBarStart = std::abs(beatInBar) < 0.001;
      bool isDownbeat = std::fmod(beat, 1.0) < 0.001;

      if (isBarStart) {
        generalPaint_.setColor(colors::TEXT_SECONDARY);
        generalPaint_.setStrokeWidth(1.5f);
        canvas->drawLine(x, 4, x, RULER_HEIGHT - 4, generalPaint_);

        textPaint_.setColor(colors::TEXT_PRIMARY);
        juce::String barStr = juce::String(barNum);
        canvas->drawString(barStr.toStdString().c_str(), x + 4, 18, rulerBarFont_, textPaint_);
      } else if (isDownbeat && pixelsPerBeat >= 30.0) {
        generalPaint_.setColor(colors::BORDER_SUBTLE);
        generalPaint_.setStrokeWidth(1.0f);
        canvas->drawLine(x, RULER_HEIGHT - 12, x, RULER_HEIGHT - 4, generalPaint_);

        if (pixelsPerBeat >= 50.0) {
          textPaint_.setColor(colors::TEXT_TERTIARY);
          int beatInBarNum = static_cast<int>(beatInBar) + 1;
          juce::String label = juce::String(barNum) + "." + juce::String(beatInBarNum);
          canvas->drawString(label.toStdString().c_str(), x + 2, RULER_HEIGHT - 6, rulerBeatFont_, textPaint_);
        }
      } else if (pixelsPerBeat >= 80.0) {
        generalPaint_.setColor(SkColorSetARGB(60, 255, 255, 255));
        generalPaint_.setStrokeWidth(0.5f);
        canvas->drawLine(x, RULER_HEIGHT - 6, x, RULER_HEIGHT - 2, generalPaint_);
      }
    }

    // Clip name badge
    if (currentClip.isValid()) {
      SkRect badge = SkRect::MakeXYWH(4, 4, juce::jmin(150.0f, static_cast<float>(PIANO_WIDTH - 8)), 22);
      generalPaint_.setColor(withAlpha(colors::VIOLET, opacity::GLASS_SOLID));
      canvas->drawRoundRect(badge, 4, 4, generalPaint_);

      borderPaint_.setColor(withAlpha(colors::VIOLET, opacity::GLOW_STRONG));
      borderPaint_.setStrokeWidth(1.0f);
      canvas->drawRoundRect(badge, 4, 4, borderPaint_);

      textPaint_.setColor(colors::TEXT_PRIMARY);
      juce::String clipName = currentClip.clipName.isEmpty() ? "MIDI Clip" : currentClip.clipName;
      if (clipName.length() > 18) clipName = clipName.substring(0, 17) + "...";
      canvas->drawString(clipName.toStdString().c_str(), 10, 19, clipNameFont_, textPaint_);
    }
}

void PianoRollComponent::drawPitchBackground(SkCanvas *canvas, float notesHeight, float width) {
    using namespace design;
    
    // 1. Piano Keys Area Background
    SkRect pianoRect = SkRect::MakeXYWH(0, RULER_HEIGHT, PIANO_WIDTH, notesHeight);
    canvas->drawRect(pianoRect, generalPaint_);

    // 3. Draw Keys and Horizontal Grid Lines
    int topPitch = pixelsToPitch(RULER_HEIGHT);
    int bottomPitch = pixelsToPitch(RULER_HEIGHT + notesHeight);

    topPitch = juce::jlimit(0, 127, topPitch);
    bottomPitch = juce::jlimit(0, 127, bottomPitch);

    for (int p = bottomPitch; p <= topPitch; ++p) {
        float y = pitchToPixels(p) + RULER_HEIGHT;
        float h = pixelsPerPitch;

        if (y < RULER_HEIGHT - h || y >= RULER_HEIGHT + notesHeight) continue;

        int noteInOctave = p % 12;
        bool black = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                      noteInOctave == 8 || noteInOctave == 10);

        SkRect keyRect = SkRect::MakeXYWH(0, y, PIANO_WIDTH, h);
        generalPaint_.setStyle(SkPaint::kFill_Style);

        bool isHovered = (p == hoveredPianoKey);
        bool isPlaying = (p == playingPianoKey);

        if (black) {
            if (isPlaying) generalPaint_.setColor(colors::MAGENTA);
            else if (isHovered) generalPaint_.setColor(colors::BG_MEDIUM);
            else generalPaint_.setColor(colors::BG_DARKEST);
            canvas->drawRect(keyRect, generalPaint_);
        } else {
            if (isPlaying) generalPaint_.setColor(colors::CYAN);
            else if (isHovered) generalPaint_.setColor(colors::TEXT_PRIMARY);
            else generalPaint_.setColor(colors::TEXT_SECONDARY);
            canvas->drawRect(keyRect, generalPaint_);

            SkPaint shadow;
            shadow.setColor(SkColorSetARGB(50, 0, 0, 0));
            canvas->drawRect(SkRect::MakeXYWH(0, y + h - 1, PIANO_WIDTH, 1), shadow);
        }

        // Key Label
        if (pixelsPerPitch > 12.0f) {
            static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F", "F#", "G",  "G#", "A",  "A#", "B"};
            if (noteInOctave == 0) {
                textPaint_.setColor(black ? colors::TEXT_SECONDARY : colors::BG_DARKEST);
                SkFont font = getMonoFont(juce::jmin(11.0f, (float)(pixelsPerPitch * 0.7f)), FontWeight::Bold);
                juce::String label = "C" + juce::String(p / 12 - 2);
                canvas->drawString(label.toStdString().c_str(), PIANO_WIDTH - 24.0f, y + h * 0.7f, font, textPaint_);
            } else if (pixelsPerPitch > 18.0f) {
                textPaint_.setColor(colors::TEXT_TERTIARY);
                SkFont font = getMonoFont(juce::jmin(9.0f, (float)(pixelsPerPitch * 0.5f)), FontWeight::Regular);
                canvas->drawString(noteNames[noteInOctave], PIANO_WIDTH - 18.0f, y + h * 0.7f, font, textPaint_);
            }
        }

        generalPaint_.setColor(colors::BORDER_SUBTLE);
        generalPaint_.setStrokeWidth(1.0f);
        canvas->drawLine(PIANO_WIDTH, y + h, width, y + h, generalPaint_);
    }
}

void PianoRollComponent::drawNoteEditor(SkCanvas *canvas, float notesHeight, float width) {
    using namespace design;
    SkRect noteAreaRect = SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT, width - PIANO_WIDTH, notesHeight);
    canvas->save();
    canvas->clipRect(noteAreaRect);

    // Grid Lines (Vertical)
    generalPaint_.setColor(colors::BORDER_SUBTLE);
    generalPaint_.setStrokeWidth(1.0f);
    double startBeat = std::floor(pixelsToBeats(PIANO_WIDTH));
    double endBeat = pixelsToBeats(width);
    double beatStep = (pixelsPerBeat < 15.0) ? 4.0 : 1.0;

    for (double b = startBeat; b <= endBeat; b += beatStep) {
        float x = PIANO_WIDTH + beatsToPixels(b);
        if (x >= PIANO_WIDTH) {
            canvas->drawLine(x, RULER_HEIGHT, x, RULER_HEIGHT + notesHeight, generalPaint_);
        }
    }

    // Notes
    SkPaint selectedGlowPaint;
    selectedGlowPaint.setColor(colors::CYAN);
    selectedGlowPaint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kSolid_SkBlurStyle, 4.0f));

    SkPaint collisionGlowPaint;
    collisionGlowPaint.setColor(colors::AMBER);
    collisionGlowPaint.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 3.0f));

    for (const auto &note : noteRects) {
        if (note.bounds.getY() > (RULER_HEIGHT + notesHeight) || note.bounds.getBottom() < RULER_HEIGHT) continue;
        if (note.bounds.getX() > width || note.bounds.getRight() < PIANO_WIDTH) continue;

        SkRect r = SkRect::MakeXYWH(note.bounds.getX(), note.bounds.getY(), note.bounds.getWidth(), note.bounds.getHeight());
        SkRRect rr = SkRRect::MakeRectXY(r.makeInset(1.0f, 1.0f), dimensions::RADIUS_XS, dimensions::RADIUS_XS);

        if (note.hasCollision && !note.selected) {
            canvas->drawRect(rr.rect().makeOutset(3.0f, 3.0f), collisionGlowPaint);
        }

        SkColor noteColor;
        if (note.selected) {
            canvas->drawRect(rr.rect().makeOutset(2.0f, 2.0f), selectedGlowPaint);
            noteColor = colors::CYAN;
        } else if (note.hasCollision) {
            noteColor = interpolateColor(colors::AMBER, colors::VIOLET, 0.4f);
        } else {
            float velocityFactor = note.velocity / 127.0f;
            noteColor = interpolateColor(darken(colors::VIOLET, 0.3f), lighten(colors::VIOLET, 0.15f), velocityFactor);
        }

        if (note.muted) noteColor = withAlpha(noteColor, design::opacity::GLASS_MEDIUM);

        generalPaint_.setColor(noteColor);
        SkPoint pts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
        SkColor nColors[2] = {lighten(noteColor, 0.12f), darken(noteColor, 0.08f)};
        generalPaint_.setShader(SkGradientShader::MakeLinear(pts, nColors, nullptr, 2, SkTileMode::kClamp));
        canvas->drawRRect(rr, generalPaint_);
        generalPaint_.setShader(nullptr);

        if (r.height() > 6.0f && r.width() > 10.0f) {
            SkRect stripe = SkRect::MakeXYWH(rr.rect().left() + 1, rr.rect().top() + 1, rr.rect().width() - 2, 2.0f);
            SkPaint stripePaint;
            stripePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.3f + (note.velocity / 127.0f) * 0.4f));
            canvas->drawRect(stripe, stripePaint);
        }

        borderPaint_.setStyle(SkPaint::kStroke_Style);
        if (note.hasCollision) {
            borderPaint_.setColor(withAlpha(colors::AMBER, opacity::GLOW_INTENSE));
            borderPaint_.setStrokeWidth(1.5f);
        } else {
            borderPaint_.setColor(SkColorSetARGB(80, 0, 0, 0));
            borderPaint_.setStrokeWidth(1.0f);
        }
        canvas->drawRRect(rr, borderPaint_);

        if (note.isHovered && !note.selected) {
            SkPaint hoverPaint;
            hoverPaint.setStyle(SkPaint::kStroke_Style);
            hoverPaint.setColor(withAlpha(colors::TEXT_PRIMARY, opacity::GLOW_STRONG));
            hoverPaint.setStrokeWidth(1.5f);
            canvas->drawRRect(rr, hoverPaint);
        }

        if (note.probability < 0.99f && r.width() > 20.0f && r.height() > 12.0f) {
            float probSize = juce::jmin(r.height() - 4, 10.0f);
            SkRect probBg = SkRect::MakeXYWH(r.right() - probSize - 3, r.bottom() - 6, probSize, 3);
            SkRect probFill = SkRect::MakeXYWH(r.right() - probSize - 3, r.bottom() - 6, probSize * note.probability, 3);
            SkPaint p;
            p.setColor(SkColorSetARGB(100, 0, 0, 0));
            canvas->drawRoundRect(probBg, 1, 1, p);
            p.setColor(colors::AMBER);
            canvas->drawRoundRect(probFill, 1, 1, p);
        }

        if (note.muted && r.width() > 14.0f) {
            textPaint_.setColor(withAlpha(colors::TEXT_TERTIARY, opacity::SECONDARY));
            SkFont muteFont = typography::getMonoFont(8.0f, FontWeight::Bold);
            canvas->drawString("M", r.left() + 3, r.bottom() - 3, muteFont, textPaint_);
        }
    }
    canvas->restore();
}

void PianoRollComponent::drawVelocityEditor(SkCanvas *canvas, float notesHeight, float width) {
    using namespace design;
    SkRect velocityRect = SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT + notesHeight, width - PIANO_WIDTH, velocityLaneHeight);
    generalPaint_.setColor(colors::BG_DARK);
    generalPaint_.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(velocityRect, generalPaint_);

    canvas->save();
    canvas->clipRect(velocityRect);
    borderPaint_.setColor(colors::BORDER_DEFAULT);
    canvas->drawLine(0, RULER_HEIGHT + notesHeight, width, RULER_HEIGHT + notesHeight, borderPaint_);

    float laneBottom = RULER_HEIGHT + notesHeight + velocityLaneHeight;

    for (const auto &note : noteRects) {
        if (note.bounds.getX() > width || note.bounds.getRight() < PIANO_WIDTH) continue;

        float barWidth = std::max(2.0f, std::min(8.0f, note.bounds.getWidth()));
        float barX = note.bounds.getX() + (note.bounds.getWidth() - barWidth) * 0.5f;
        float normalizedVel = note.velocity / 127.0f;
        float barHeight = normalizedVel * (velocityLaneHeight - 4.0f);
        float barY = laneBottom - barHeight - 2.0f;

        SkRect barRect = SkRect::MakeXYWH(barX, barY, barWidth, barHeight);
        SkRRect barRR = SkRRect::MakeRectXY(barRect, dimensions::RADIUS_XXS, dimensions::RADIUS_XXS);

        SkColor barColor = note.selected ? colors::CYAN : interpolateColor(darken(colors::VIOLET, 0.2f), colors::VIOLET, normalizedVel);
        generalPaint_.setColor(barColor);
        canvas->drawRRect(barRR, generalPaint_);

        if (note.selected) {
            SkPaint glow;
            glow.setColor(colors::CYAN);
            glow.setMaskFilter(SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 2.0f));
            canvas->drawRRect(barRR, glow);
        }
    }
    canvas->restore();
}

void PianoRollComponent::drawPlayheadPosition(SkCanvas *canvas, float height, float width) {
    using namespace design;
    float x = PIANO_WIDTH + beatsToPixels(currentPlayheadBeats);
    if (x >= PIANO_WIDTH && x <= width) {
      SkPaint p;
      p.setColor(colors::TEXT_PRIMARY);
      p.setStrokeWidth(2.0f);
      p.setAntiAlias(true);
      canvas->drawLine(x, RULER_HEIGHT, x, height, p);

      SkPath triangle;
      float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT; // Fix: use appropriate top
      triangle.moveTo(x, contentTop);
      triangle.lineTo(x - 5.0f, contentTop - 8.0f);
      triangle.lineTo(x + 5.0f, contentTop - 8.0f);
      triangle.close();
      canvas->drawPath(triangle, p);
    }
}

} // namespace zenith
