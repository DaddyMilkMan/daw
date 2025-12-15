/**
 * @file ClipComponent.cpp
 * @brief Flat clip component with theme colors and clean typography
 *
 * Clean DAW design:
 * - Track-colored fills (muted, from theme)
 * - Typography.body for clip names
 * - Simple 1-2px selection border
 * - Rounded corners (4px, 8px grid)
 */

// POLISH: spacing normalized to 8px grid (rounded corners 4px)
// POLISH: typography now uses ZenithDesignSystem
// POLISH: flattened visuals (track colors, no gradients)

#include "ClipComponent.h"
#include "../../engine/ProjectState.h"
#include <JuceHeader.h>

#include "../ZenithTheme.h"
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <effects/SkDashPathEffect.h>
#include <effects/SkGradientShader.h>

using namespace zenith;

ClipComponent::ClipComponent(juce::ValueTree clipNode) : clip(clipNode) {
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  startTimerHz(60); // 60 Hz for smooth animations
}

ClipComponent::~ClipComponent() { stopTimer(); }

juce::String ClipComponent::getClipId() const {
  return clip[ProjectState::PROP_ID].toString();
}

double ClipComponent::getStartBeats() const {
  return clip[ProjectState::PROP_START_BEATS];
}

double ClipComponent::getLengthBeats() const {
  return clip[ProjectState::PROP_LENGTH_BEATS];
}

double ClipComponent::getLoopLength() const {
  // If explicitly set, use it. Otherwise default to full length (no loop)
  if (clip.hasProperty(ProjectState::PROP_LOOP_LENGTH)) {
    return clip[ProjectState::PROP_LOOP_LENGTH];
  }
  return getLengthBeats();
}

ClipComponent::NeighborhoodState ClipComponent::getNeighborhoodState() const {
  NeighborhoodState state;
  auto clipsNode = clip.getParent();
  if (!clipsNode.isValid())
    return state;

  double myStart = getStartBeats();
  double myEnd = myStart + getLengthBeats();
  juce::String myName = clip[ProjectState::PROP_NAME].toString();
  juce::String myType = clip[ProjectState::PROP_TYPE].toString();

  // Tolerance for float comparison
  const double kEpsilon = 0.001;

  for (const auto &other : clipsNode) {
    if (other == clip)
      continue; // Skip self

    // Determine if "same identity" (Name + Type is best proxy for non-unique-ID
    // systems) Strictly speaking user asked for "Same ID", but IDs are unique.
    // We use Name+Type to approximate "Split Clip" behavior.
    bool sameIdentity = (other[ProjectState::PROP_NAME].toString() == myName) &&
                        (other[ProjectState::PROP_TYPE].toString() == myType);

    if (!sameIdentity)
      continue;

    double otherStart = other[ProjectState::PROP_START_BEATS];
    double otherEnd =
        otherStart + (double)other[ProjectState::PROP_LENGTH_BEATS];

    // Check Left Adjacency (Other End == My Start)
    if (std::abs(otherEnd - myStart) < kEpsilon) {
      state.leftConnected = true;
    }

    // Check Right Adjacency (Other Start == My End)
    if (std::abs(otherStart - myEnd) < kEpsilon) {
      state.rightConnected = true;
    }
  }
  return state;
}

void ClipComponent::updateBounds(double pixelsPerBeat, int yPosition,
                                 int height) {
  int x = static_cast<int>(getStartBeats() * pixelsPerBeat);
  int width = static_cast<int>(getLengthBeats() * pixelsPerBeat);
  setBounds(x, yPosition, width, height);
}

void ClipComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  using namespace zenith::design;

  float fWidth = (float)bounds.getWidth();
  float fHeight = (float)bounds.getHeight();

  // Early exit for zero-size clips (edge case safety)
  if (fWidth <= 0 || fHeight <= 0)
    return;

  // 1. Calculate Smart Corners
  NeighborhoodState neighbors = getNeighborhoodState();
  float radius = 8.0f; // As requested

  // radii order: TopLeft, TopRight, BottomRight, BottomLeft
  SkVector radii[4];

  // Left side
  if (neighbors.leftConnected) {
    radii[0].set(0, 0); // TopLeft flat
    radii[3].set(0, 0); // BottomLeft flat
  } else {
    radii[0].set(radius, radius);
    radii[3].set(radius, radius);
  }

  // Right side
  if (neighbors.rightConnected) {
    radii[1].set(0, 0); // TopRight flat
    radii[2].set(0, 0); // BottomRight flat
  } else {
    radii[1].set(radius, radius);
    radii[2].set(radius, radius);
  }

  SkRRect clipRRect;
  clipRRect.setRectRadii(SkRect::MakeWH(fWidth, fHeight), radii);

  // 2. Setup Colors
  juce::String clipType = clip[ProjectState::PROP_TYPE].toString();
  SkColor baseColor =
      (clipType == "midi") ? colors.waveformMidi : colors.waveformAudio;

  // Header Color (Solid)
  SkColor headerColor = baseColor;

  // Body Color (Muted/Transparent)
  SkColor bodyColor = SkColorSetARGB(
      40, // Very transparent for body
      SkColorGetR(baseColor), SkColorGetG(baseColor), SkColorGetB(baseColor));

  // 3. Clipping for Rounded Structure
  canvas->save();
  canvas->clipRRect(clipRRect, true);

  // 4. Draw Backgrounds (Ghost Loop Support)
  double loopBeats = getLoopLength();
  double totalBeats = getLengthBeats();
  double pixelsPerBeat = (totalBeats > 0) ? (fWidth / totalBeats) : 0;

  // Guard against divide by zero or negative loops
  if (loopBeats <= 0)
    loopBeats = totalBeats;

  // Guard against zero-length clips
  if (totalBeats <= 0)
    totalBeats = 1.0; // Minimum 1 beat to prevent division issues

  float loopWidthPx = (float)(loopBeats * pixelsPerBeat);
  int numLoops = (int)std::ceil(totalBeats / loopBeats);

  // Performance safety: Cap max loop iterations to prevent runaway rendering
  static constexpr int kMaxClipLoopIterations = 100;
  numLoops = std::min(numLoops, kMaxClipLoopIterations);

  // Constants
  const float headerHeight = 24.0f;
  const float contentAreaTop = headerHeight;

  for (int i = 0; i < numLoops; ++i) {
    float xOffset = i * loopWidthPx;
    float loopRight = std::min(xOffset + loopWidthPx, fWidth);
    float loopW = loopRight - xOffset;

    if (loopW <= 0)
      continue;

    SkRect loopRect = SkRect::MakeXYWH(xOffset, 0, loopW, fHeight);

    bool isGhost = (i > 0);

    // Draw Body
    SkPaint bodyPaint;
    if (isGhost) {
      // Ghost: Faded
      bodyPaint.setColor(SkColorSetA(bodyColor, 20)); // Fainter
    } else {
      // Main: Regular
      bodyPaint.setColor(bodyColor);
    }
    canvas->drawRect(loopRect, bodyPaint);

    // Draw Separator for Ghost Loops (Dashed Line at boundary)
    if (isGhost) {
      SkPaint dividerPaint;
      dividerPaint.setColor(SkColorSetA(headerColor, 128));
      dividerPaint.setStrokeWidth(1.0f);
      SkScalar dashes[] = {4.0f, 4.0f};
      dividerPaint.setPathEffect(
          SkDashPathEffect::Make(SkSpan<const SkScalar>(dashes, 2), 0));
      canvas->drawLine(xOffset, 0, xOffset, fHeight, dividerPaint);
    }

    // Draw Header Area for this loop iteration?
    // User says "Clip Header strip at the top... separate".
    // Usually Header is only on the Clip Container, NOT repeated per loop.
    // "Clip Header strip... separate from the waveform/MIDI content area".
    // So I will draw the Header ONCE on top of everything.

    // Ghost visual: "Original loop iteration is opaque, and subsequent loops...
    // have a dashed outline" Dashed outline usually refers to the content or
    // the loop boundary.
  }

  // 5. Draw Header (Overlay on top of loops)
  // The header background
  SkRect headerRect = SkRect::MakeXYWH(0, 0, fWidth, headerHeight);
  SkPaint headerPaint;
  headerPaint.setColor(headerColor);
  canvas->drawRect(headerRect, headerPaint);

  // Header Name (A+ Typography)
  if (fWidth > 20) {
    SkFont font =
        design::typography::getSkFont(12.0f, design::FontWeight::Bold);

    juce::String clipName = clip[ProjectState::PROP_NAME].toString();
    if (clipName.isEmpty())
      clipName = "Clip";

    // Text Paint
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorWHITE);

    // Text Shadow for contrast
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(SkColorSetA(SK_ColorBLACK, 128));

    float textX = 8.0f;
    float textY = headerHeight / 2.0f + 5.0f; // Adjusted vertical center

    // Simple text clipping
    canvas->save();
    canvas->clipRect(headerRect);

    // Draw Shadow
    canvas->drawSimpleText(clipName.toRawUTF8(), clipName.length(),
                           SkTextEncoding::kUTF8, textX + 1.0f, textY + 1.0f,
                           font, shadowPaint);

    // Draw Text
    canvas->drawSimpleText(clipName.toRawUTF8(), clipName.length(),
                           SkTextEncoding::kUTF8, textX, textY, font,
                           textPaint);
    canvas->restore();
  }

  // 6. Content Area (Waveform/MIDI Blobs)
  // The content area starts below the header and extends to the clip bottom.
  SkRect contentRect =
      SkRect::MakeXYWH(0, contentAreaTop, fWidth, fHeight - contentAreaTop);

  canvas->save();
  canvas->clipRect(contentRect);

  // Use Clip ID to seed random for consistent visualization
  // (In proper implementation, this would read actual note/audio data)
  juce::String cid = clip[ProjectState::PROP_ID].toString();
  juce::Random rng(cid.hashCode());

  if (clipType == "midi") {
    // Draw Neon MIDI bars
    SkPaint notePaint;
    notePaint.setAntiAlias(true);

    int numNotes = (int)(fWidth / 15) + 2;
    for (int i = 0; i < numNotes; ++i) {
      if (rng.nextFloat() > 0.7f)
        continue; // Sparsity

      float x = i * 15.0f + rng.nextFloat() * 5.0f;
      float pitchNorm = rng.nextFloat();
      float y = contentAreaTop + pitchNorm * (fHeight - contentAreaTop - 6);
      float w = 8.0f + rng.nextFloat() * 20.0f;
      float h = 4.0f;

      SkRect noteRect = SkRect::MakeXYWH(x, y, w, h);

      // Glowy Note
      notePaint.setColor(SkColorSetA(baseColor, 255));
      canvas->drawRRect(SkRRect::MakeRectXY(noteRect, 2, 2), notePaint);

      // Subtle tail
      SkPaint tailPaint;
      tailPaint.setColor(SkColorSetA(baseColor, 50));
      canvas->drawRect(SkRect::MakeXYWH(x + w, y, 10, h), tailPaint);
    }
  } else {
    // Draw Waveform (Generative approximation)
    SkPath wavePath;
    float midY = contentAreaTop + (fHeight - contentAreaTop) * 0.5f;
    float amp = (fHeight - contentAreaTop) * 0.4f;

    wavePath.moveTo(0, midY);
    int steps = (int)(fWidth / 2.0f);
    for (int i = 0; i < steps; ++i) {
      float x = i * 2.0f;
      float noise = (rng.nextFloat() * 2.0f - 1.0f);
      // Mix with sine for structure
      float val = std::sin(x * 0.1f) * noise * noise; // Squared for peaks
      wavePath.lineTo(x, midY + val * amp);
    }

    SkPaint wavePaint;
    wavePaint.setStyle(SkPaint::kStroke_Style);
    wavePaint.setStrokeWidth(1.5f);
    wavePaint.setAntiAlias(true);
    wavePaint.setColor(SkColorSetA(baseColor, 200));
    canvas->drawPath(wavePath, wavePaint);

    // Fill
    wavePath.lineTo(fWidth, midY); // Close path loosely
    wavePath.lineTo(0, midY);

    SkPaint fillPaint;
    fillPaint.setStyle(SkPaint::kFill_Style);
    fillPaint.setColor(SkColorSetA(baseColor, 50));
    canvas->drawPath(wavePath, fillPaint);
  }

  canvas->restore(); // Restore Clipping (Ends Content Area)
  canvas->restore(); // Restore Clipping (Ends Smart Rounded Corner Mask)

  // 7. Draw Borders (Selection or Outline)
  if (isSelected) {
    SkPaint selectionPaint;
    selectionPaint.setAntiAlias(true);
    selectionPaint.setColor(SkColorSetA(colors.primary, 255)); // Bright accent
    selectionPaint.setStyle(SkPaint::kStroke_Style);
    selectionPaint.setStrokeWidth(2.0f);
    canvas->drawRRect(clipRRect, selectionPaint);
  } else {
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setColor(SkColorSetA(colors.borderSubtle, 100));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    canvas->drawRRect(clipRRect, borderPaint);
  }
}

void ClipComponent::mouseEnter(const juce::MouseEvent &event) {
  juce::ignoreUnused(event);
  isHovered = true;
  repaint();
}

void ClipComponent::mouseExit(const juce::MouseEvent &event) {
  juce::ignoreUnused(event);
  isHovered = false;
  repaint();
}

void ClipComponent::mouseDown(const juce::MouseEvent &event) {
  dragStartPos = event.getPosition();
  dragStartBeats = getStartBeats();

  // Toggle selection on click (Ctrl/Cmd for multi-select)
  if (!event.mods.isCommandDown()) {
    isSelected = !isSelected;
  }

  repaint();
}

void ClipComponent::mouseDrag(const juce::MouseEvent &event) {
  // Simple drag visualization (actual state changes would go through
  // ProjectState)
  auto delta = event.getPosition() - dragStartPos;
  setTopLeftPosition(getX() + delta.x, getY());
}

void ClipComponent::timerCallback() {
  // Smooth animation updates
  const float animationSpeed = 0.1f;

  // Hover animation (smooth ease in/out)
  float targetHover = isHovered ? 1.0f : 0.0f;
  hoverAnimation += (targetHover - hoverAnimation) * animationSpeed;

  // Selection pulse animation
  if (isSelected) {
    selectionPulse += 0.02f;
    if (selectionPulse > 1.0f) {
      selectionPulse -= 1.0f;
    }
  }

  // Repaint only if animation is active
  if (std::abs(hoverAnimation - targetHover) > 0.01f || isSelected) {
    repaint();
  }
}
