/**
 * @file PianoRollRendering.cpp
 * @brief Professional-grade MIDI Piano Roll Editor - Rendering Modules
 */

#include "Engine.h"
#include "PianoRollComponent.h"
#include "ZenithDesignSystem.h"
#include <algorithm>
#include <cmath>
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <effects/SkDashPathEffect.h>
#include <effects/SkGradientShader.h>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

using namespace zenith;

// Magic numbers moved to constants/theme
static constexpr float NOTE_CORNER_RADIUS = 3.0f;
static constexpr float SELECTION_STROKE_WIDTH = 2.0f;
static constexpr float HOVER_STROKE_WIDTH = 2.0f;
constexpr float RULER_HEIGHT = 30.0f;
constexpr float TOOLBAR_HEIGHT = 40.0f;
constexpr float PIANO_WIDTH = 80.0f;

// Toolbar Layout (Agent 1 Constants)
constexpr float TOOLBAR_BUTTON_START_X = 10.0f;
constexpr float TOOLBAR_BUTTON_WIDTH = 60.0f;
constexpr float TOOLBAR_BUTTON_HEIGHT = 30.0f;
constexpr float TOOLBAR_BUTTON_MARGIN = 5.0f;
constexpr int DEFAULT_PIANO_KEY_VELOCITY = 100;

//==============================================================================
// Main Rendering Loop
//==============================================================================

void PianoRollComponent::drawSkia(SkCanvas *canvas) {
  if (!canvas)
    return;
  using namespace zenith::design;

  // Background (Deep Slate)
  canvas->clear(colors::BG_DARKEST);

  generalPaint_.setColor(colors::BG_DARKER);
  generalPaint_.setStyle(SkPaint::kFill_Style);

  // Alias for legacy code
  SkPaint &paint = generalPaint_;

  auto localBounds = getLocalBounds();
  float width = (float)localBounds.getWidth();
  float height = (float)localBounds.getHeight();
  float notesHeight = height - RULER_HEIGHT - velocityLaneHeight;

  // 1. Piano Keys Area Background
  SkRect pianoRect =
      SkRect::MakeXYWH(0, RULER_HEIGHT, PIANO_WIDTH, notesHeight);
  canvas->drawRect(pianoRect, generalPaint_);

  //==========================================================================
  // PROFESSIONAL TIMELINE RULER (Ableton/Logic style)
  //==========================================================================
  {
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
    canvas->drawLine(0, RULER_HEIGHT - 1, width, RULER_HEIGHT - 1,
                     borderPaint_);

    // Calculate visible beat range
    double visibleStartBeat = viewStartBeats;
    double visibleEndBeat = pixelsToBeats(width);

    // Determine grid density based on zoom
    double beatsPerBar = 4.0; // Assume 4/4 time
    double barStep = 1.0;     // Every bar
    double beatSubdiv = 1.0;  // Beat subdivision

    if (pixelsPerBeat < 10.0) {
      barStep = 4.0; // Every 4 bars
      beatSubdiv = 4.0;
    } else if (pixelsPerBeat < 30.0) {
      barStep = 1.0; // Every bar
      beatSubdiv = 1.0;
    } else if (pixelsPerBeat < 80.0) {
      beatSubdiv = 0.5; // Half beats
    } else {
      beatSubdiv = 0.25; // Sixteenth notes
    }

    // Draw bar numbers and markers
    // Fonts are now members: rulerBarFont_, rulerBeatFont_

    double startBar = std::floor(visibleStartBeat / beatsPerBar) * beatsPerBar;

    for (double beat = startBar; beat <= visibleEndBeat; beat += beatSubdiv) {
      float x = PIANO_WIDTH + beatsToPixels(beat);
      if (x < PIANO_WIDTH)
        continue;

      int barNum = static_cast<int>(beat / beatsPerBar) + 1;
      double beatInBar = std::fmod(beat, beatsPerBar);
      bool isBarStart = std::abs(beatInBar) < 0.001;
      bool isDownbeat = std::fmod(beat, 1.0) < 0.001;

      if (isBarStart) {
        // Bar marker - tall line + number
        generalPaint_.setColor(colors::TEXT_SECONDARY);
        generalPaint_.setStrokeWidth(1.5f);
        canvas->drawLine(x, 4, x, RULER_HEIGHT - 4, generalPaint_);

        // Bar number with subtle glow
        textPaint_.setColor(colors::TEXT_PRIMARY);
        juce::String barStr = juce::String(barNum);
        canvas->drawString(barStr.toStdString().c_str(), x + 4, 18,
                           rulerBarFont_, textPaint_);

      } else if (isDownbeat && pixelsPerBeat >= 30.0) {
        // Beat marker - medium line
        generalPaint_.setColor(colors::BORDER_SUBTLE);
        generalPaint_.setStrokeWidth(1.0f);
        canvas->drawLine(x, RULER_HEIGHT - 12, x, RULER_HEIGHT - 4,
                         generalPaint_);

        // Beat number (1.2, 1.3, etc)
        if (pixelsPerBeat >= 50.0) {
          textPaint_.setColor(colors::TEXT_TERTIARY);
          int beatInBarNum = static_cast<int>(beatInBar) + 1;
          juce::String label =
              juce::String(barNum) + "." + juce::String(beatInBarNum);
          canvas->drawString(label.toStdString().c_str(), x + 2,
                             RULER_HEIGHT - 6, rulerBeatFont_, textPaint_);
        }
      } else if (pixelsPerBeat >= 80.0) {
        // Subdivision tick - short line
        generalPaint_.setColor(SkColorSetARGB(60, 255, 255, 255));
        generalPaint_.setStrokeWidth(0.5f);
        canvas->drawLine(x, RULER_HEIGHT - 6, x, RULER_HEIGHT - 2,
                         generalPaint_);
      }
    }

    // Clip name badge (top-left)
    if (currentClip.isValid()) {
      SkRect badge = SkRect::MakeXYWH(
          4, 4, juce::jmin(150.0f, static_cast<float>(PIANO_WIDTH - 8)), 22);
      generalPaint_.setColor(withAlpha(colors::VIOLET, 0.3f));
      canvas->drawRoundRect(badge, 4, 4, generalPaint_);

      // Border glow
      borderPaint_.setColor(withAlpha(colors::VIOLET, 0.6f));
      borderPaint_.setStrokeWidth(1.0f);
      canvas->drawRoundRect(badge, 4, 4, borderPaint_);

      // Clip name
      textPaint_.setColor(colors::TEXT_PRIMARY);
      // font is clipNameFont_

      juce::String clipName =
          currentClip.clipName.isEmpty() ? "MIDI Clip" : currentClip.clipName;
      if (clipName.length() > 18)
        clipName = clipName.substring(0, 17) + "...";
      canvas->drawString(clipName.toStdString().c_str(), 10, 19, clipNameFont_,
                         textPaint_);
    }
  }

  // 2. Grid Lines (Vertical) - Added for "Real" feel
  canvas->save();
  SkRect noteAreaRect = SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT,
                                         width - PIANO_WIDTH, notesHeight);
  canvas->clipRect(noteAreaRect);

  paint.setColor(colors::BORDER_SUBTLE);
  paint.setStrokeWidth(1.0f);
  // Draw vertical lines for beats
  // We iterate visible beat range
  double startBeat = std::floor(pixelsToBeats(PIANO_WIDTH));
  double endBeat = pixelsToBeats(width);

  // Optimization: Don't draw too many lines if zoomed out
  double beatStep = (pixelsPerBeat < 15.0) ? 4.0 : 1.0;

  for (double b = startBeat; b <= endBeat; b += beatStep) {
    float x = PIANO_WIDTH + beatsToPixels(b);
    if (x >= PIANO_WIDTH) {
      canvas->drawLine(x, RULER_HEIGHT, x, RULER_HEIGHT + notesHeight, paint);
    }
  }
  canvas->restore();

  // 3. Draw Keys and Horizontal Grid Lines
  int topPitch = pixelsToPitch(RULER_HEIGHT);
  int bottomPitch = pixelsToPitch(RULER_HEIGHT + notesHeight);

  topPitch = juce::jlimit(0, 127, topPitch);
  bottomPitch = juce::jlimit(0, 127, bottomPitch);

  for (int p = bottomPitch; p <= topPitch; ++p) {
    float y = pitchToPixels(p) + RULER_HEIGHT;
    float h = pixelsPerPitch;

    if (y < RULER_HEIGHT - h || y >= RULER_HEIGHT + notesHeight)
      continue;

    int noteInOctave = p % 12;
    bool black = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                  noteInOctave == 8 || noteInOctave == 10);

    // Draw Key
    static constexpr float kKeyLabelMinZoom = 12.0f;
    static constexpr float kKeyLabelDetailZoom = 18.0f;
    static constexpr float kKeyLabelMaxFontSize = 11.0f;
    static constexpr float kKeyLabelDetailMaxFontSize = 9.0f;
    static constexpr float kKeyLabelOffset = -24.0f;
    static constexpr float kKeyLabelDetailOffset = -18.0f;

    SkRect keyRect = SkRect::MakeXYWH(0, y, PIANO_WIDTH, h);
    paint.setStyle(SkPaint::kFill_Style);

    // Check if this key is hovered or playing
    bool isHovered = (p == hoveredPianoKey);
    bool isPlaying = (p == playingPianoKey);

    if (black) {
      // Black key
      if (isPlaying) {
        paint.setColor(colors::MAGENTA);
      } else if (isHovered) {
        paint.setColor(colors::BG_MEDIUM);
      } else {
        paint.setColor(colors::BG_DARKEST);
      }
      canvas->drawRect(keyRect, paint);
    } else {
      // White key (actually light grey)
      if (isPlaying) {
        paint.setColor(colors::CYAN);
      } else if (isHovered) {
        paint.setColor(colors::TEXT_PRIMARY);
      } else {
        paint.setColor(colors::TEXT_SECONDARY); // #A1A1AA
      }
      canvas->drawRect(keyRect, paint);

      // Shadow for depth
      SkPaint shadow;
      shadow.setColor(SkColorSetARGB(50, 0, 0, 0));
      canvas->drawRect(SkRect::MakeXYWH(0, y + h - 1, PIANO_WIDTH, 1), shadow);
    }

    // Key Label
    if (pixelsPerPitch > kKeyLabelMinZoom) {
      static const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                        "F#", "G",  "G#", "A",  "A#", "B"};
      SkPaint textPaint;
      textPaint.setAntiAlias(true);

      if (noteInOctave == 0) {
        // C notes get octave number
        textPaint.setColor(black ? colors::TEXT_SECONDARY : colors::BG_DARKEST);
        SkFont font = getMonoFont(
            juce::jmin(kKeyLabelMaxFontSize, (float)(pixelsPerPitch * 0.7f)),
            FontWeight::Bold);
        juce::String label = "C" + juce::String(p / 12 - 2);
        canvas->drawString(label.toStdString().c_str(),
                           PIANO_WIDTH + kKeyLabelOffset, y + h * 0.7f, font,
                           textPaint);
      } else if (pixelsPerPitch > kKeyLabelDetailZoom) {
        textPaint.setColor(colors::TEXT_TERTIARY);
        SkFont font = getMonoFont(juce::jmin(kKeyLabelDetailMaxFontSize,
                                             (float)(pixelsPerPitch * 0.5f)),
                                  FontWeight::Regular);
        canvas->drawString(noteNames[noteInOctave],
                           PIANO_WIDTH + kKeyLabelDetailOffset, y + h * 0.7f,
                           font, textPaint);
      }
    }

    // Horizontal Grid Line
    paint.setColor(colors::BORDER_SUBTLE);
    paint.setStrokeWidth(1.0f);
    canvas->drawLine(PIANO_WIDTH, y + h, width, y + h, paint);
  }

  // 4. Notes - Professional rendering with collision detection, mute,
  // probability
  canvas->save();
  canvas->clipRect(noteAreaRect);

  SkPaint selectedGlowPaint;
  selectedGlowPaint.setColor(colors::CYAN);
  selectedGlowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(SkBlurStyle::kSolid_SkBlurStyle, 4.0f));

  SkPaint collisionGlowPaint;
  collisionGlowPaint.setColor(colors::AMBER);
  collisionGlowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 3.0f));

  for (const auto &note : noteRects) {
    // Culling
    if (note.bounds.getY() > height || note.bounds.getBottom() < 0)
      continue;
    if (note.bounds.getX() > width || note.bounds.getRight() < PIANO_WIDTH)
      continue;

    SkRect r =
        SkRect::MakeXYWH(note.bounds.getX(), note.bounds.getY(),
                         note.bounds.getWidth(), note.bounds.getHeight());

    // Inner Rect for pseudo-3D
    SkRect inner = r.makeInset(1.0f, 1.0f);
    SkRRect rr = SkRRect::MakeRectXY(inner, 3.0f, 3.0f);

    // COLLISION WARNING: Amber glow for overlapping notes
    if (note.hasCollision && !note.selected) {
      SkRect collisionRect = rr.rect().makeOutset(3.0f, 3.0f);
      canvas->drawRect(collisionRect, collisionGlowPaint);
    }

    // Color based on selection, mute, and velocity
    SkColor noteColor;
    float alpha = note.muted ? 0.4f : 1.0f; // Dim muted notes

    if (note.selected) {
      // Glow for selected notes
      SkRect outsetRect = rr.rect().makeOutset(2.0f, 2.0f);
      canvas->drawRect(outsetRect, selectedGlowPaint);
      noteColor = colors::CYAN;
    } else if (note.hasCollision) {
      // Collision: tinted amber
      noteColor = interpolateColor(colors::AMBER, colors::VIOLET, 0.4f);
    } else {
      // Normal: VIOLET with velocity intensity
      float velocityFactor = note.velocity / 127.0f;
      noteColor =
          interpolateColor(darken(colors::VIOLET, 0.3f),
                           lighten(colors::VIOLET, 0.15f), velocityFactor);
    }

    // Apply mute dimming
    if (note.muted) {
      noteColor = withAlpha(noteColor, 0.35f);
    }

    paint.setColor(noteColor);

    // Gradient for note depth
    SkPoint pts[2] = {{r.left(), r.top()}, {r.left(), r.bottom()}};
    SkColor nColors[2] = {lighten(noteColor, 0.12f), darken(noteColor, 0.08f)};
    paint.setShader(SkGradientShader::MakeLinear(pts, nColors, nullptr, 2,
                                                 SkTileMode::kClamp));

    canvas->drawRRect(rr, paint);
    paint.setShader(nullptr);

    // Velocity indicator stripe at top (like Ableton)
    if (r.height() > 6.0f && r.width() > 10.0f) {
      float stripeHeight = 2.0f;
      SkRect stripe = SkRect::MakeXYWH(inner.left() + 1, inner.top() + 1,
                                       inner.width() - 2, stripeHeight);
      SkPaint stripePaint;
      stripePaint.setColor(withAlpha(colors::TEXT_PRIMARY,
                                     0.3f + (note.velocity / 127.0f) * 0.4f));
      stripePaint.setAntiAlias(true);
      canvas->drawRect(stripe, stripePaint);
    }

    // Border
    SkPaint border;
    border.setStyle(SkPaint::kStroke_Style);
    border.setAntiAlias(true);
    if (note.hasCollision) {
      border.setColor(withAlpha(colors::AMBER, 0.8f));
      border.setStrokeWidth(1.5f);
    } else {
      border.setColor(SkColorSetARGB(80, 0, 0, 0));
      border.setStrokeWidth(1.0f);
    }
    canvas->drawRRect(rr, border);

    // Hover highlight
    if (note.isHovered && !note.selected) {
      SkPaint hoverPaint;
      hoverPaint.setStyle(SkPaint::kStroke_Style);
      hoverPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.6f));
      hoverPaint.setStrokeWidth(1.5f);
      hoverPaint.setAntiAlias(true);
      canvas->drawRRect(rr, hoverPaint);
    }

    // PROBABILITY INDICATOR (dice icon position - bottom right)
    if (note.probability < 0.99f && r.width() > 20.0f && r.height() > 12.0f) {
      float probSize = juce::jmin(r.height() - 4, 10.0f);
      float probWidth = probSize * note.probability;
      SkRect probBg = SkRect::MakeXYWH(r.right() - probSize - 3, r.bottom() - 6,
                                       probSize, 3);
      SkRect probFill = SkRect::MakeXYWH(r.right() - probSize - 3,
                                         r.bottom() - 6, probWidth, 3);

      SkPaint probBgPaint;
      probBgPaint.setColor(SkColorSetARGB(100, 0, 0, 0));
      canvas->drawRoundRect(probBg, 1, 1, probBgPaint);

      SkPaint probFillPaint;
      probFillPaint.setColor(colors::AMBER);
      canvas->drawRoundRect(probFill, 1, 1, probFillPaint);
    }

    // MUTED INDICATOR (diagonal stripes or "M")
    if (note.muted && r.width() > 14.0f) {
      SkPaint mutePaint;
      mutePaint.setColor(withAlpha(colors::TEXT_TERTIARY, 0.5f));
      mutePaint.setAntiAlias(true);
      SkFont muteFont = typography::getMonoFont(8.0f, FontWeight::Bold);
      canvas->drawString("M", r.left() + 3, r.bottom() - 3, muteFont,
                         mutePaint);
    }
  }
  canvas->restore();

  // 5. Velocity Lane Background
  SkRect velocityRect =
      SkRect::MakeXYWH(PIANO_WIDTH, RULER_HEIGHT + notesHeight,
                       width - PIANO_WIDTH, velocityLaneHeight);
  paint.setColor(colors::BG_DARK);
  paint.setStyle(SkPaint::kFill_Style);
  canvas->drawRect(velocityRect, paint);

  // Velocity Bars - Draw vertical bars for each note
  canvas->save();
  canvas->clipRect(velocityRect);

  // Separator line
  paint.setColor(colors::BORDER_DEFAULT);
  canvas->drawLine(0, RULER_HEIGHT + notesHeight, width,
                   RULER_HEIGHT + notesHeight, paint);

  float velocityLaneTop = RULER_HEIGHT + notesHeight;
  float velocityLaneBottom = velocityLaneTop + velocityLaneHeight;

  for (const auto &note : noteRects) {
    // Skip notes outside visible horizontal range
    if (note.bounds.getX() > width || note.bounds.getRight() < PIANO_WIDTH)
      continue;

    float barWidth = std::max(2.0f, std::min(8.0f, note.bounds.getWidth()));
    float barX =
        note.bounds.getX() + (note.bounds.getWidth() - barWidth) * 0.5f;

    // Height based on velocity (0-127 maps to 0 to velocityLaneHeight)
    float normalizedVelocity = note.velocity / 127.0f;
    float barHeight = normalizedVelocity * (velocityLaneHeight - 4.0f);
    float barY = velocityLaneBottom - barHeight - 2.0f;

    // Color - selected uses CYAN, unselected uses VIOLET with velocity
    // intensity
    SkColor barColor;
    if (note.selected) {
      barColor = colors::CYAN;
    } else {
      barColor = interpolateColor(darken(colors::VIOLET, 0.2f), colors::VIOLET,
                                  normalizedVelocity);
    }

    // Draw velocity bar
    SkRect barRect = SkRect::MakeXYWH(barX, barY, barWidth, barHeight);
    SkRRect barRR = SkRRect::MakeRectXY(barRect, 1.0f, 1.0f);

    paint.setColor(barColor);
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRRect(barRR, paint);

    // Glow for selected
    if (note.selected) {
      SkPaint glowPaint;
      glowPaint.setColor(colors::CYAN);
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 2.0f));
      canvas->drawRRect(barRR, glowPaint);
    }
  }
  canvas->restore();

  // 6. Playhead - follows transport
  {
    float playheadX = PIANO_WIDTH + beatsToPixels(currentPlayheadBeats);
    if (playheadX >= PIANO_WIDTH && playheadX <= width) {
      // Playhead line
      SkPaint playheadPaint;
      playheadPaint.setColor(colors::TEXT_PRIMARY);
      playheadPaint.setStrokeWidth(2.0f);
      playheadPaint.setAntiAlias(true);
      canvas->drawLine(playheadX, RULER_HEIGHT, playheadX, (float)height,
                       playheadPaint);

      // Triangle marker in ruler/toolbar area
      static constexpr float kPlayheadMarkerHalfWidth = 5.0f;
      static constexpr float kPlayheadMarkerHeight = 8.0f;

      const float contentTop = TOOLBAR_HEIGHT + RULER_HEIGHT;

      SkPath trianglePath;
      trianglePath.moveTo(playheadX, contentTop);
      trianglePath.lineTo(playheadX - kPlayheadMarkerHalfWidth,
                          contentTop - kPlayheadMarkerHeight);
      trianglePath.lineTo(playheadX + kPlayheadMarkerHalfWidth,
                          contentTop - kPlayheadMarkerHeight);
      trianglePath.close();
      canvas->drawPath(trianglePath, playheadPaint);
    }
  }

  // 7. Marquee Selection
  if (!marqueeRect.isEmpty()) {
    SkRect m =
        SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                         marqueeRect.getWidth(), marqueeRect.getHeight());
    paint.setColor(withAlpha(colors::CYAN, 0.2f));
    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(m, paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(colors::CYAN);
    canvas->drawRect(m, paint);
  }
}

//==============================================================================
// Rendering Modules
//==============================================================================

juce::Colour PianoRollComponent::getColorForVelocity(int velocity) const {
  // Clamp velocity to valid MIDI range
  velocity = juce::jlimit(0, 127, velocity);

  // Generate a color gradient from blue (soft) to red (loud)
  // Low velocity: cooler colors (blue/purple)
  // High velocity: warmer colors (orange/red)
  float normalized = velocity / 127.0f;

  // Use HSL color space for smooth gradient
  // Hue: 240 (blue) -> 0 (red) as velocity increases
  float hue = (1.0f - normalized) * 0.66f; // From blue to red
  float saturation =
      0.7f + normalized * 0.3f; // More saturated at high velocity
  float brightness = 0.6f + normalized * 0.4f; // Brighter at high velocity

  return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
}

SkColor PianoRollComponent::getSkiaColorForVelocity(int velocity) const {
  juce::Colour c = getColorForVelocity(velocity);
  return SkColorSetARGB(c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue());
}

//==============================================================================
// Rendering Modules
//==============================================================================

void PianoRollComponent::drawModernToolbar(SkCanvas *canvas,
                                           const SkRect &fullRect) {
  using namespace zenith::design;

  SkRect toolbarRect = SkRect::MakeXYWH(0, 0, fullRect.width(), TOOLBAR_HEIGHT);

  // Glassmorphism Background
  SkPaint bgPaint;
  bgPaint.setColor(colors::BG_DARK);
  canvas->drawRect(toolbarRect, bgPaint);

  SkPaint border;
  border.setColor(colors::BORDER_SUBTLE);
  canvas->drawLine(0, TOOLBAR_HEIGHT - 1, fullRect.width(), TOOLBAR_HEIGHT - 1,
                   border);

  // Tools
  float x = 10;
  const char *toolNames[] = {"SEL", "DRW", "ERS", "CUT"};
  Tool tools[] = {Tool::Select, Tool::Draw, Tool::Erase, Tool::Slice};

  for (int i = 0; i < 4; ++i) {
    SkRect btnRect = SkRect::MakeXYWH(x, 6, 40, 28);
    SkPaint btnPaint;
    bool isActive = (currentTool == tools[i]);

    if (isActive)
      btnPaint.setColor(colors::CYAN);
    else
      btnPaint.setColor(colors::BG_LIGHT);

    canvas->drawRoundRect(btnRect, 4, 4, btnPaint);

    SkPaint textPaint;
    textPaint.setColor(isActive ? colors::BG_DARKEST : colors::TEXT_PRIMARY);
    SkFont font = typography::getMonoFont(10, FontWeight::Bold);
    canvas->drawString(toolNames[i], x + 8, 24, font, textPaint);

    x += 46;
  }

  // Scale Lock
  x += 20;
  SkRect lockBtn = SkRect::MakeXYWH(x, 6, 80, 28);
  SkPaint lockPaint;
  lockPaint.setColor(scaleHighlight.enabled ? colors::VIOLET
                                            : colors::BG_LIGHT);
  canvas->drawRoundRect(lockBtn, 4, 4, lockPaint);

  SkPaint lockText;
  lockText.setColor(colors::TEXT_PRIMARY);
  SkFont font = typography::getSkFont(10, FontWeight::Bold);
  juce::String label = scaleHighlight.enabled ? "SCALE ON" : "SCALE OFF";
  canvas->drawString(label.toStdString().c_str(), x + 8, 24, font, lockText);
}

void PianoRollComponent::drawPianoKeys(SkCanvas *canvas, const SkRect &area) {
  using namespace zenith::design;

  SkPaint bg;
  bg.setColor(colors::BG_DARKER);
  canvas->drawRect(area, bg);

  for (int p = 0; p < 128; ++p) {
    float y = area.top() + pitchToPixels(p);
    float h = pixelsPerPitch;

    if (y > area.bottom() || y + h < area.top())
      continue;

    int noteInOctave = p % 12;
    bool black = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                  noteInOctave == 8 || noteInOctave == 10);

    SkRect keyRect = SkRect::MakeXYWH(area.left(), y, area.width(), h);
    SkPaint keyPaint;
    keyPaint.setStyle(SkPaint::kFill_Style);

    bool isRoot =
        scaleHighlight.enabled && (noteInOctave == scaleHighlight.rootNote);
    bool inScale = isNoteInScale(p);
    bool isHovered = (p == hoveredPianoKey);
    bool isPlaying = (p == playingPianoKey);

    if (black) {
      if (isPlaying)
        keyPaint.setColor(colors::MAGENTA);
      else if (isHovered)
        keyPaint.setColor(colors::BG_MEDIUM);
      else if (scaleHighlight.enabled && !inScale)
        keyPaint.setColor(darken(colors::BG_DARKEST, 0.5f));
      else if (isRoot)
        keyPaint.setColor(withAlpha(colors::VIOLET, 0.3f));
      else
        keyPaint.setColor(colors::BG_DARKEST);
    } else {
      if (isPlaying)
        keyPaint.setColor(colors::CYAN);
      else if (isHovered)
        keyPaint.setColor(colors::TEXT_PRIMARY);
      else if (scaleHighlight.enabled && !inScale)
        keyPaint.setColor(darken(colors::TEXT_SECONDARY, 0.3f));
      else if (isRoot)
        keyPaint.setColor(withAlpha(colors::VIOLET, 0.2f));
      else
        keyPaint.setColor(colors::TEXT_SECONDARY);
    }

    canvas->drawRect(keyRect, keyPaint);

    SkPaint linePaint;
    linePaint.setColor(colors::BG_DARKEST);
    canvas->drawLine(area.left(), y + h, area.right(), y + h, linePaint);
  }
}

void PianoRollComponent::drawGrid(SkCanvas *canvas, const SkRect &area) {
  using namespace zenith::design;
  SkPaint vLine;
  vLine.setColor(colors::BORDER_SUBTLE);
  vLine.setStrokeWidth(1.0f);

  double minBeat = pixelsToBeats(0);
  double maxBeat = pixelsToBeats(area.width());

  for (double b = std::floor(minBeat); b <= maxBeat; b += 1.0) {
    float x = area.left() + beatsToPixels(b);
    canvas->drawLine(x, area.top(), x, area.bottom(), vLine);
  }
}

void PianoRollComponent::drawNotes(SkCanvas *canvas, const SkRect &area) {
  using namespace zenith::design;

  SkPaint collisionGlowPaint;
  collisionGlowPaint.setColor(colors::AMBER);
  collisionGlowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(SkBlurStyle::kOuter_SkBlurStyle, 3.0f));

  for (const auto &note : noteRects) {
    float x = area.left() + beatsToPixels(note.startBeats);
    float w = beatsToPixels(note.lengthBeats);
    float y = area.top() + pitchToPixels(note.pitch);
    float h = pixelsPerPitch;

    if (x > area.right() || x + w < area.left() || y > area.bottom() ||
        y + h < area.top())
      continue;

    SkRect r = SkRect::MakeXYWH(x, y, w, h);
    SkRect inner = r.makeInset(1.0f, 1.0f);
    SkRRect rr = SkRRect::MakeRectXY(inner, 3.0f, 3.0f);

    SkColor noteColor = note.selected ? colors::CYAN : colors::VIOLET;
    if (note.hasCollision && !note.selected)
      noteColor = colors::AMBER;
    if (note.muted)
      noteColor = withAlpha(noteColor, 0.4f);

    SkPaint p;
    p.setColor(noteColor);
    canvas->drawRRect(rr, p);

    if (note.hasCollision && !note.selected) {
      canvas->drawRect(rr.rect().makeOutset(2, 2), collisionGlowPaint);
    }
  }
}

void PianoRollComponent::drawVelocityLane(SkCanvas *canvas,
                                          const SkRect &area) {
  using namespace zenith::design;

  SkPaint bg;
  bg.setColor(colors::BG_DARK);
  canvas->drawRect(area, bg);

  SkPaint line;
  line.setColor(colors::BORDER_DEFAULT);
  canvas->drawLine(area.left(), area.top(), area.right(), area.top(), line);

  for (const auto &note : noteRects) {
    float x = area.left() + beatsToPixels(note.startBeats);
    if (x > area.right())
      continue;

    float h = (note.velocity / 127.0f) * area.height();
    SkRect bar = SkRect::MakeXYWH(x, area.bottom() - h, 4, h);

    SkPaint p;
    p.setColor(note.selected ? colors::CYAN : withAlpha(colors::VIOLET, 0.7f));
    canvas->drawRect(bar, p);
  }
}

void PianoRollComponent::drawChordName(SkCanvas *canvas) {
  using namespace zenith::design;
  juce::String chord = getCurrentChordName();
  if (chord.isEmpty())
    return;

  float cx = getLocalBounds().getWidth() / 2.0f;
  float cy = TOOLBAR_HEIGHT + 20.0f;

  SkFont font = typography::getDisplayFont(16.0f);
  SkPaint textP;
  textP.setColor(colors::TEXT_PRIMARY);
  textP.setAntiAlias(true);

  float textW = font.measureText(chord.toStdString().c_str(), chord.length(),
                                 SkTextEncoding::kUTF8);

  SkRect pill = SkRect::MakeXYWH(cx - textW / 2 - 10, cy - 12, textW + 20, 24);
  SkPaint pillBg;
  pillBg.setColor(withAlpha(colors::BG_DARKEST, 0.8f));
  canvas->drawRoundRect(pill, 12, 12, pillBg);

  SkPaint border;
  border.setStyle(SkPaint::kStroke_Style);
  border.setColor(withAlpha(colors::CYAN, 0.5f));
  canvas->drawRoundRect(pill, 12, 12, border);

  canvas->drawString(chord.toStdString().c_str(), cx - textW / 2, cy + 6, font,
                     textP);
}
