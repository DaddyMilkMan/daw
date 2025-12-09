/**
 * @file PianoRollRendering.cpp
 * @brief Skia Rendering Implementation for PianoRollComponent
 */

#include "../../include/ui/PianoRollComponent.h"
#include "skia/ZenithDesignSystem.h"
#include <cmath>

using namespace zenith;

constexpr float RULER_HEIGHT = 30.0f;
constexpr float PIANO_WIDTH = 80.0f;
constexpr float TOOLBAR_HEIGHT = 40.0f;

//==============================================================================
// Skia Rendering Implementation
//==============================================================================

void PianoRollComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect fullRect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Define areas
  float rulerH = RULER_HEIGHT;
  float pianoW = PIANO_WIDTH;
  float velH = (float)velocityLaneHeight;

  SkRect contentArea = SkRect::MakeLTRB(pianoW, rulerH, bounds.getWidth(),
                                        bounds.getHeight() - velH);
  SkRect pianoArea =
      SkRect::MakeLTRB(0, rulerH, pianoW, bounds.getHeight() - velH);
  SkRect velocityArea = SkRect::MakeLTRB(pianoW, bounds.getHeight() - velH,
                                         bounds.getWidth(), bounds.getHeight());
  SkRect rulerArea = SkRect::MakeLTRB(pianoW, 0, bounds.getWidth(), rulerH);

  // 1. Background
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(18, 18, 22));
  canvas->drawRect(fullRect, bgPaint);

  // 2. Grid & Content
  canvas->save();
  canvas->clipRect(contentArea);

  if (stepSequencerMode) {
    drawStepSequencer(canvas, contentArea);
  } else {
    drawGrid(canvas, contentArea);

    if (ghostNotesEnabled) {
      drawGhostNotes(canvas, contentArea);
    }

    drawExpressionLanes(canvas, contentArea);
    drawNotes(canvas, contentArea);

    if (arpPreviewEnabled) {
      drawArpPreview(canvas, contentArea);
    }

    // Selection marquee
    if (currentDragMode == DragMode::MarqueeSelect) {
      SkPaint marqueePaint;
      marqueePaint.setColor(SkColorSetARGB(40, 0, 255, 255));
      marqueePaint.setStyle(SkPaint::kFill_Style);
      SkRect mRect =
          SkRect::MakeXYWH(marqueeRect.getX(), marqueeRect.getY(),
                           marqueeRect.getWidth(), marqueeRect.getHeight());
      canvas->drawRect(mRect, marqueePaint);

      marqueePaint.setColor(SkColorSetRGB(0, 255, 255));
      marqueePaint.setStyle(SkPaint::kStroke_Style);
      canvas->drawRect(mRect, marqueePaint);
    }
  }
  canvas->restore();

  // 3. Sidebars & Overlays
  drawPianoKeys(canvas, pianoArea);
  drawVelocityLane(canvas, velocityArea);

  // Ruler Background
  SkPaint rulerPaint;
  rulerPaint.setColor(SkColorSetRGB(25, 25, 30));
  canvas->drawRect(rulerArea, rulerPaint);

  // 4. Toolbar
  drawModernToolbar(canvas, fullRect);

  // 5. Overlays
  drawChordName(canvas);

  // Clip Name Overlay
  if (currentClip.isValid()) {
    SkFont titleFont;
    titleFont.setSize(12.0f);
    SkPaint titlePaint;
    titlePaint.setColor(SkColorSetARGB(200, 255, 255, 255));
    canvas->drawString(currentClip.clipName.toRawUTF8(), PIANO_WIDTH + 10.0f,
                       20.0f, titleFont, titlePaint);
  }
}

void PianoRollComponent::drawPianoKeys(SkCanvas *canvas, const SkRect &area) {
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(10, 10, 12));
  canvas->drawRect(area, bgPaint);

  SkPaint keyPaint;
  SkPaint textPaint;
  textPaint.setColor(SkColorSetARGB(180, 255, 255, 255));
  SkFont font;
  font.setSize(10.0f);
  font.setSubpixel(true);

  int startPitch = pixelsToPitch(area.top());
  int endPitch = pixelsToPitch(area.bottom());
  startPitch = juce::jlimit(0, 127, startPitch + 1);
  endPitch = juce::jlimit(0, 127, endPitch - 1);

  for (int p = endPitch; p <= startPitch; ++p) {
    float y = pitchToPixels(p) + RULER_HEIGHT;
    float h = pixelsPerPitch;

    bool isBlackKey = (p % 12 == 1 || p % 12 == 3 || p % 12 == 6 ||
                       p % 12 == 8 || p % 12 == 10);
    SkRect keyRect = SkRect::MakeXYWH(area.left(), y, area.width(), h);

    if (isBlackKey) {
      keyPaint.setColor(SkColorSetRGB(30, 30, 35));
      keyPaint.setStyle(SkPaint::kFill_Style);
      canvas->drawRect(keyRect, keyPaint);

      SkPaint bevel;
      bevel.setColor(SkColorSetRGB(50, 50, 60));
      canvas->drawLine(area.left(), y, area.right(), y, bevel);
    } else {
      keyPaint.setColor(SkColorSetRGB(60, 60, 70));
      keyPaint.setStyle(SkPaint::kFill_Style);
      canvas->drawRect(keyRect, keyPaint);

      SkPaint sep;
      sep.setColor(SkColorSetRGB(20, 20, 25));
      sep.setStrokeWidth(1.0f);
      canvas->drawLine(area.left(), y + h, area.right(), y + h,
                       sep); // Bottom separator

      if (p % 12 == 0) {
        juce::String label = "C" + juce::String(p / 12 - 2);
        canvas->drawString(label.toRawUTF8(), area.left() + 35.0f, y + h - 3.0f,
                           font, textPaint);
      }
    }
  }
}

void PianoRollComponent::drawGrid(SkCanvas *canvas, const SkRect &area) {
  SkPaint gridPaint;
  gridPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
  gridPaint.setStrokeWidth(1.0f);

  int startPitch = pixelsToPitch(area.top() - RULER_HEIGHT);
  int endPitch = pixelsToPitch(area.bottom() - RULER_HEIGHT);

  for (int p = endPitch; p <= startPitch; ++p) {
    float y = pitchToPixels(p) + RULER_HEIGHT;

    if (p % 12 == 0)
      gridPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
    else
      gridPaint.setColor(SkColorSetARGB(20, 255, 255, 255));

    if (scaleLockEnabled && scaleHighlight.enabled && !isNoteInScale(p)) {
      SkPaint dimPaint;
      dimPaint.setColor(SkColorSetARGB(100, 10, 10, 12));
      canvas->drawRect(
          SkRect::MakeXYWH(area.left(), y, area.width(), pixelsPerPitch),
          dimPaint);
    }

    canvas->drawLine(area.left(), y, area.right(), y, gridPaint);
  }

  double startBeat = pixelsToBeats(area.left() - PIANO_WIDTH);
  double endBeat = pixelsToBeats(area.right() - PIANO_WIDTH);

  double beatStep = 1.0;
  if (pixelsPerBeat > 100)
    beatStep = 0.25;
  else if (pixelsPerBeat < 20)
    beatStep = 4.0;

  for (double b = std::floor(startBeat); b <= endBeat; b += beatStep) {
    float x = beatsToPixels(b) + PIANO_WIDTH;
    if (std::abs(std::fmod(b, 4.0)) < 0.001)
      gridPaint.setColor(SkColorSetARGB(60, 255, 255, 255));
    else if (std::abs(std::fmod(b, 1.0)) < 0.001)
      gridPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
    else
      gridPaint.setColor(SkColorSetARGB(20, 255, 255, 255));

    canvas->drawLine(x, area.top(), x, area.bottom(), gridPaint);
  }
}

//==============================================================================
// MPE Expression Lane Visualization
//==============================================================================

void PianoRollComponent::drawExpressionLanes(SkCanvas *canvas,
                                             const SkRect &area) {
  // Calculate total height of visible lanes
  int visibleLanes = 0;
  for (int i = 0; i < 4; ++i) {
    if (expressionLaneVisible[i])
      visibleLanes++;
  }

  if (visibleLanes == 0)
    return;

  float startY = area.bottom() - (visibleLanes * expressionLaneHeight);

  SkPaint laneBgPaint;
  laneBgPaint.setColor(SkColorSetARGB(255, 30, 30, 30));

  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(255, 60, 60, 60));
  borderPaint.setStyle(SkPaint::kStroke_Style);

  SkPaint textPaint;
  textPaint.setColor(SkColorSetARGB(255, 150, 150, 150));
  SkFont font;
  font.setSize(10.0f);

  const char *laneNames[] = {"Pitch Bend", "Pressure", "Slide", "Expression"};
  const SkColor laneColors[] = {
      SkColorSetRGB(255, 150, 150), // Red for Pitch
      SkColorSetRGB(150, 255, 150), // Green for Pressure
      SkColorSetRGB(150, 150, 255), // Blue for Slide
      SkColorSetRGB(255, 255, 150)  // Yellow for Expression
  };

  int laneIndex = 0;
  for (int i = 0; i < 4; ++i) {
    if (!expressionLaneVisible[i])
      continue;

    SkRect laneRect = SkRect::MakeXYWH(
        area.left(), startY + (laneIndex * expressionLaneHeight), area.width(),
        (float)expressionLaneHeight);

    // Draw background
    canvas->drawRect(laneRect, laneBgPaint);
    canvas->drawRect(laneRect, borderPaint);

    // Draw label
    canvas->drawString(laneNames[i], laneRect.left() + 5, laneRect.top() + 15,
                       font, textPaint);

    // Draw curves for each note
    SkPaint curvePaint;
    curvePaint.setColor(laneColors[i]);
    curvePaint.setStyle(SkPaint::kStroke_Style);
    curvePaint.setStrokeWidth(2.0f);
    curvePaint.setAntiAlias(true);

    // Only draw for visible notes to optimize
    for (const auto &note : noteRects) {
      auto it = noteExpressions.find(note.id);
      if (it == noteExpressions.end())
        continue;

      const std::vector<ExpressionPoint> *points = nullptr;
      switch ((ExpressionType)i) {
      case ExpressionType::PitchBend:
        points = &it->second.pitchBend;
        break;
      case ExpressionType::Pressure:
        points = &it->second.pressure;
        break;
      case ExpressionType::Slide:
        points = &it->second.slide;
        break;
      case ExpressionType::Expression:
        points = &it->second.expression;
        break;
      }

      if (!points || points->empty())
        continue;

      SkPath path;
      bool first = true;

      for (const auto &pt : *points) {
        float x = area.left() + beatsToPixels(note.startBeats + pt.timeOffset);
        float y = laneRect.bottom() - (pt.value * expressionLaneHeight);

        if (first) {
          path.moveTo(x, y);
          first = false;
        } else {
          path.lineTo(x, y);
        }
      }

      // Dim non-selected notes
      if (note.selected) {
        curvePaint.setAlpha(255);
        curvePaint.setStrokeWidth(2.5f);
      } else {
        curvePaint.setAlpha(100);
        curvePaint.setStrokeWidth(1.0f);
      }

      canvas->drawPath(path, curvePaint);

      // distinct points
      SkPaint pointPaint;
      pointPaint.setColor(laneColors[i]);
      pointPaint.setAntiAlias(true);
      if (!note.selected)
        pointPaint.setAlpha(100);

      for (const auto &pt : *points) {
        float x = area.left() + beatsToPixels(note.startBeats + pt.timeOffset);
        float y = laneRect.bottom() - (pt.value * expressionLaneHeight);
        canvas->drawCircle(x, y, 2.5f, pointPaint);
      }
    }

    laneIndex++;
  }
}

void PianoRollComponent::drawNotes(SkCanvas *canvas, const SkRect &area) {
  SkPaint notePaint;
  notePaint.setAntiAlias(true);
  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(SK_ColorBLACK);
  borderPaint.setAntiAlias(true);

  for (const auto &note : noteRects) {
    if (note.bounds.getBottom() < area.top() ||
        note.bounds.getY() > area.bottom() ||
        note.bounds.getRight() < area.left() ||
        note.bounds.getX() > area.right())
      continue;

    SkRect rect =
        SkRect::MakeXYWH(note.bounds.getX(), note.bounds.getY(),
                         note.bounds.getWidth(), note.bounds.getHeight());
    SkRRect rrect = SkRRect::MakeRectXY(rect, 3.0f, 3.0f);

    SkColor baseColor =
        note.selected ? SK_ColorWHITE : getSkiaColorForVelocity(note.velocity);
    if (note.muted)
      baseColor = SkColorSetA(baseColor, 100);
    else if (note.probability < 1.0f)
      // Probability transparency (min 30%)
      baseColor =
          SkColorSetA(baseColor, (int)(255 * (0.3f + 0.7f * note.probability)));

    notePaint.setColor(baseColor);

    // Draw chance indicator if < 1.0
    if (note.probability < 1.0f && !note.muted) {
      SkPaint chancePaint;
      chancePaint.setColor(SK_ColorWHITE);
      chancePaint.setAlpha(150);
      canvas->drawCircle(rect.right() - 4, rect.bottom() - 4, 3, chancePaint);
    }

    // Draw Articulation / Keyswitch ID
    if (note.articulationId > 0 && note.lengthBeats > 0.5) {
      SkFont artFont;
      artFont.setSize(10.0f);
      SkPaint artTextPaint;
      artTextPaint.setColor(SK_ColorLTGRAY);
      juce::String artText = "Art: " + juce::String(note.articulationId);
      canvas->drawString(artText.toRawUTF8(), rect.left() + 4, rect.top() + 10,
                         artFont, artTextPaint);
    }

    // Draw Logic Condition / Recurrence
    if ((note.condition.isNotEmpty() || note.recurrence.isNotEmpty()) &&
        note.lengthBeats > 1.0) {
      SkFont logicFont;
      logicFont.setSize(9.0f);
      SkPaint logicPaint;
      logicPaint.setColor(SkColorSetRGB(255, 200, 100)); // Gold text for logic
      juce::String logicText = note.condition + " " + note.recurrence;
      canvas->drawString(logicText.toRawUTF8(), rect.left() + 4,
                         rect.bottom() - 8, logicFont, logicPaint);
    }

    canvas->drawRRect(rrect, notePaint);
    canvas->drawRRect(rrect, borderPaint);
  }
}

void PianoRollComponent::drawVelocityLane(SkCanvas *canvas,
                                          const SkRect &area) {
  SkPaint divPaint;
  divPaint.setColor(SkColorSetRGB(50, 50, 60));
  divPaint.setStrokeWidth(2.0f);
  canvas->drawLine(area.left(), area.top(), area.right(), area.top(), divPaint);

  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(15, 15, 18));
  canvas->drawRect(area, bgPaint);

  SkPaint stalkPaint;
  stalkPaint.setAntiAlias(true);

  for (const auto &note : noteRects) {
    if (note.bounds.getRight() < area.left() ||
        note.bounds.getX() > area.right())
      continue;
    SkRect vRect = SkRect::MakeXYWH(
        note.velocityBounds.getX(), note.velocityBounds.getY(),
        note.velocityBounds.getWidth(), note.velocityBounds.getHeight());
    if (vRect.bottom() > area.bottom())
      vRect.fBottom = area.bottom();
    if (vRect.top() < area.top())
      vRect.fTop = area.top();

    stalkPaint.setColor(note.selected ? SK_ColorWHITE
                                      : getSkiaColorForVelocity(note.velocity));
    float cx = vRect.centerX();
    canvas->drawLine(cx, vRect.top(), cx, area.bottom(), stalkPaint);
    canvas->drawCircle(cx, vRect.top(), 4.0f, stalkPaint);
  }
}

static juce::String getNoteName(int pitchClass) {
  static const char *names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};
  return names[std::abs(pitchClass) % 12];
}

static juce::String getScaleTypeName(PianoRollComponent::ScaleType type) {
  using T = PianoRollComponent::ScaleType;
  switch (type) {
  case T::Chromatic:
    return "Chromatic";
  case T::Major:
    return "Major";
  case T::Minor:
    return "Minor";
  case T::HarmonicMinor:
    return "Harm. Min";
  case T::MelodicMinor:
    return "Mel. Min";
  case T::Dorian:
    return "Dorian";
  case T::Phrygian:
    return "Phrygian";
  case T::Lydian:
    return "Lydian";
  case T::Mixolydian:
    return "Mixolydian";
  case T::Aeolian:
    return "Aeolian";
  case T::Locrian:
    return "Locrian";
  case T::MajorPentatonic:
    return "Maj Pent";
  case T::MinorPentatonic:
    return "Min Pent";
  case T::MajorBlues:
    return "Maj Blues";
  case T::MinorBlues:
    return "Min Blues";
  case T::Bhairav:
    return "Bhairav";
  case T::Byzantine:
    return "Byzantine";
  case T::ManGong:
    return "Man Gong";
  default:
    return "Exotic";
  }
}

void PianoRollComponent::drawModernToolbar(SkCanvas *canvas,
                                           const SkRect &fullRect) {
  SkRect toolbarRect =
      SkRect::MakeXYWH(fullRect.right() - 600.0f, 4.0f, 590.0f, 32.0f);
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(200, 30, 30, 35));
  canvas->drawRoundRect(toolbarRect, 16.0f, 16.0f, bgPaint);

  auto drawBtn = [&](float x, float w, const char *label, bool active) {
    SkRect btnRect = SkRect::MakeXYWH(x, toolbarRect.top() + 4.0f, w, 24.0f);
    SkPaint p;
    p.setColor(active ? SkColorSetRGB(0, 255, 100) : SkColorSetRGB(60, 60, 70));
    p.setAntiAlias(true);
    canvas->drawRoundRect(btnRect, 4.0f, 4.0f, p);

    SkFont f;
    f.setSize(10.0f);
    SkPaint tp;
    tp.setColor(SK_ColorWHITE);
    float tw = f.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawString(label, x + (w - tw) / 2, btnRect.centerY() + 3.0f, f,
                       tp);
  };

  float x = toolbarRect.left() + 8.0f;
  float gap = 4.0f;
  drawBtn(x, 40.0f, "SPRAY", sprayCanMode);
  x += 40 + gap;
  drawBtn(x, 35.0f, "RIFF", false);
  x += 35 + gap;
  drawBtn(x, 35.0f, "STEP", stepSequencerMode);
  x += 35 + gap;
  drawBtn(x, 35.0f, "LOCK", scaleLockEnabled);
  x += 35 + gap;
  drawBtn(x, 35.0f, "FOLD", foldMode);
  x += 35 + gap;
  x += 8.0f;

  SkFont f;
  f.setSize(12.0f);
  SkPaint tp;
  tp.setColor(SK_ColorLTGRAY);
  juce::String grooveName = "Straight";
  switch (currentGroove) {
  case GrooveTemplate::Straight:
    grooveName = "Straight";
    break;
  case GrooveTemplate::Swing8th:
    grooveName = "Swing 8th";
    break;
  case GrooveTemplate::Swing16th:
    grooveName = "Swing 16th";
    break;
  case GrooveTemplate::Shuffle:
    grooveName = "Shuffle";
    break;
  case GrooveTemplate::MPC:
    grooveName = "MPC";
    break;
  case GrooveTemplate::JDilla:
    grooveName = "J Dilla";
    break;
  case GrooveTemplate::HipHop:
    grooveName = "Hip Hop";
    break;
  case GrooveTemplate::Funk:
    grooveName = "Funk";
    break;
  }
  canvas->drawString(("Groove: " + grooveName).toRawUTF8(), x,
                     toolbarRect.centerY() + 4.0f, f, tp);
  x += 100.0f + gap;

  // Scale Display
  juce::String rootName = getNoteName(scaleHighlight.rootNote);
  juce::String typeName = getScaleTypeName(scaleHighlight.scale);
  juce::String scaleText = "Key: " + rootName + " " + typeName;
  SkPaint pScale = tp;
  if (scaleLockEnabled)
    pScale.setColor(SkColorSetRGB(0, 255, 100));

  canvas->drawString(scaleText.toRawUTF8(), x, toolbarRect.centerY() + 4.0f, f,
                     pScale);
  x += 120.0f + gap;

  drawBtn(x, 35.0f, "ECHO", false);
  x += 35 + gap;
  drawBtn(x, 40.0f, "STRUM", false);
  x += 40 + gap;
  drawBtn(x, 30.0f, "ARP", arpPreviewEnabled);
  x += 30 + gap;
  drawBtn(x, 42.0f, "CHORD", false);
  x += 42 + gap;

  x += 8.0f;
  drawBtn(x, 24.0f, "<", false);
  x += 24 + gap;
  drawBtn(x, 24.0f, "I", false);
  x += 24 + gap;
  drawBtn(x, 24.0f, "2x", false);
  x += 24 + gap;
}

SkColor PianoRollComponent::getSkiaColorForVelocity(int velocity) const {
  float t = velocity / 127.0f;
  if (t < 0.5f)
    return design::interpolateColor(SkColorSetRGB(0, 50, 200),
                                    SkColorSetRGB(0, 255, 255), t * 2.0f);
  return design::interpolateColor(SkColorSetRGB(0, 255, 255),
                                  SkColorSetRGB(255, 255, 255),
                                  (t - 0.5f) * 2.0f);
}

void PianoRollComponent::drawStepSequencer(SkCanvas *canvas,
                                           const SkRect &area) {
  if (stepSequencerRows.empty())
    return;

  // Draw Dark Background
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetRGB(25, 27, 33));
  canvas->drawRect(area, bgPaint);

  float cellW = area.width() / (float)stepSequencerSteps;
  float cellH = area.height() / (float)stepSequencerRows.size();

  SkPaint strokePaint;
  strokePaint.setColor(SkColorSetARGB(30, 255, 255, 255));
  strokePaint.setStyle(SkPaint::kStroke_Style);
  strokePaint.setStrokeWidth(1.0f);

  SkPaint activePaint;
  activePaint.setColor(SkColorSetRGB(0, 255, 180)); // Neon Cyan
  activePaint.setStyle(SkPaint::kFill_Style);

  // OPTIMIZATION: Pre-calculate active steps to avoid O(N) lookup in drawing
  // loop
  std::map<int, std::set<int>> activeSteps;
  for (const auto &note : noteRects) {
    // Quantize to 16th notes (0.25)
    int stepIdx = (int)std::floor(note.startBeats / 0.25 + 0.5);
    if (stepIdx >= 0 && stepIdx < stepSequencerSteps) {
      activeSteps[note.pitch].insert(stepIdx);
    }
  }

  for (int row = 0; row < (int)stepSequencerRows.size(); ++row) {
    int pitch = stepSequencerRows[row];
    // Highlight C notes
    if (pitch % 12 == 0) {
      SkRect rowRect = SkRect::MakeXYWH(area.left(), area.top() + row * cellH,
                                        area.width(), cellH);
      SkPaint cLabelPaint;
      cLabelPaint.setColor(SkColorSetARGB(20, 255, 255, 255));
      canvas->drawRect(rowRect, cLabelPaint);
    }

    bool rowHasNotes = activeSteps.count(pitch);

    for (int step = 0; step < stepSequencerSteps; ++step) {
      SkRect cell = SkRect::MakeXYWH(area.left() + step * cellW,
                                     area.top() + row * cellH, cellW, cellH);

      // Draw active if present in map
      if (rowHasNotes && activeSteps[pitch].count(step)) {
        canvas->drawRect(cell.makeInset(2, 2), activePaint);
      } else {
        canvas->drawRect(cell, strokePaint);
      }

      // beat markers (every 4 steps)
      if (step % 4 == 0) {
        SkPaint beatMarker;
        beatMarker.setColor(SkColorSetARGB(50, 255, 255, 255));
        beatMarker.setStrokeWidth(2.0f);
        canvas->drawLine(cell.left(), cell.top(), cell.left(), cell.bottom(),
                         beatMarker);
      }
    }
  }
}

void PianoRollComponent::drawArpPreview(SkCanvas *canvas, const SkRect &area) {
  if (!arpPreviewEnabled || getSelectedNoteCount() < 1)
    return;

  // Visualize a ghostly UP pattern based on selected chords
  SkPaint previewPaint;
  previewPaint.setColor(SkColorSetARGB(80, 255, 100, 200)); // Pink ghost
  previewPaint.setStyle(SkPaint::kFill_Style);

  for (const auto &note : noteRects) {
    if (!note.selected)
      continue;

    // Draw 3 ghost notes ascending from this note
    for (int i = 1; i <= 3; ++i) {
      // Simple Up Arp Logic simulation
      float x = beatsToPixels(note.startBeats + i * 0.25) + PIANO_WIDTH;
      // Shift pitch up by scale degrees? Just octaves or 3rds for visual
      float y = pitchToPixels(note.pitch + (i * 4)) +
                RULER_HEIGHT; // +Major 3rds stacking

      if (x > area.right())
        continue;

      SkRect ghost =
          SkRect::MakeXYWH(x, y, beatsToPixels(0.25), pixelsPerPitch);
      canvas->drawRect(ghost, previewPaint);

      // Connector line
      SkPaint linePaint;
      linePaint.setColor(SkColorSetARGB(50, 255, 100, 200));
      linePaint.setStrokeWidth(1.0f);

      float prevX =
          beatsToPixels(note.startBeats + (i - 1) * 0.25) + PIANO_WIDTH;
      float prevY = pitchToPixels(note.pitch + ((i - 1) * 4)) + RULER_HEIGHT;
      if (i == 1) { // Connect to original
        prevY = pitchToPixels(note.pitch) + RULER_HEIGHT;
        prevX = beatsToPixels(note.startBeats) + PIANO_WIDTH;
      }
      canvas->drawLine(prevX + 10, prevY + pixelsPerPitch / 2, x + 10,
                       y + pixelsPerPitch / 2, linePaint);
    }
  }
}

void PianoRollComponent::drawGhostNotes(SkCanvas *canvas, const SkRect &area) {
  if (ghostNotes.empty())
    return;

  SkPaint paint;
  paint.setColor(SkColorSetA(SK_ColorLTGRAY, (int)(ghostNoteOpacity * 255)));
  paint.setAntiAlias(true);

  for (const auto &gn : ghostNotes) {
    if (gn.startBeats < pixelsToBeats(area.left() - PIANO_WIDTH))
      continue;
    if (gn.startBeats > pixelsToBeats(area.right() - PIANO_WIDTH))
      continue;

    float x = beatsToPixels(gn.startBeats) + PIANO_WIDTH;
    float y = pitchToPixels(gn.pitch) + RULER_HEIGHT;
    float w = beatsToPixels(gn.lengthBeats);
    float h = pixelsPerPitch;

    canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), paint);
  }
}

void PianoRollComponent::drawChordName(SkCanvas *canvas) {
  juce::String chord = getCurrentChordName();
  if (chord.isEmpty())
    return;

  SkPaint paint;
  paint.setColor(SkColorSetARGB(180, 255, 255, 255));
  paint.setAntiAlias(true);
  SkFont font;
  font.setSize(14.0f);

  canvas->drawString(chord.toRawUTF8(), PIANO_WIDTH + 150.0f, 24.0f, font,
                     paint);
}

} // namespace zenith
