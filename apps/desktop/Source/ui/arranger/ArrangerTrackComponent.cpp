/**
 * @file ArrangerTrackComponent.cpp
 * @brief Implementation of the 'Arranger Track' for global section editing.
 */

#include "../../include/ui/ArrangerTrackComponent.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"
#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

ArrangerTrackComponent::ArrangerTrackComponent(ProjectState &ps)
    : projectState(ps) {
  // Hardcoded demo sections if state is empty, to verify visuals immediately
  // In a real flow, this would read from a ProjectState::ID_ARRANGER_TRACK
  // node.
  sections_ = {{"Intro", 0.0, 8.0, juce::Colours::cyan},
               {"Verse 1", 8.0, 16.0, juce::Colours::purple},
               {"Chorus", 24.0, 16.0, juce::Colours::orange},
               {"Outro", 40.0, 8.0, juce::Colours::lightblue}};
}

ArrangerTrackComponent::~ArrangerTrackComponent() = default;

void ArrangerTrackComponent::setViewContext(double pixelsPerBeat,
                                            double viewStartBeats) {
  if (std::abs(pixelsPerBeat - pixelsPerBeat_) > 0.001 ||
      std::abs(viewStartBeats - viewStartBeats_) > 0.001) {
    pixelsPerBeat_ = pixelsPerBeat;
    viewStartBeats_ = viewStartBeats;
    repaint();
  }
}

void ArrangerTrackComponent::drawSkia(SkCanvas *canvas) {
  using namespace design;

  auto bounds = getLocalBounds();

  // Premium Background - subtle gradient
  {
    SkPoint bgPts[2] = {{0, 0}, {0, (float)bounds.getHeight()}};
    SkColor bgColors[2] = {SkColorSetRGB(28, 28, 35),
                           SkColorSetRGB(22, 22, 28)};
    SkPaint bgPaint;
    bgPaint.setShader(SkGradientShader::MakeLinear(bgPts, bgColors, nullptr, 2,
                                                   SkTileMode::kClamp));
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                     bgPaint);
  }

  // Bottom border
  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
  borderPaint.setStrokeWidth(1.0f);
  canvas->drawLine(0, bounds.getHeight() - 0.5f, bounds.getWidth(),
                   bounds.getHeight() - 0.5f, borderPaint);

  // Draw Sections as glassmorphic pills
  SkFont font =
      typography::getSkFont(typography::FONT_XS, FontWeight::SemiBold);

  for (size_t idx = 0; idx < sections_.size(); ++idx) {
    const auto &section = sections_[idx];

    // Calculate X position
    double relStart = section.startBeats - viewStartBeats_;
    float x = static_cast<float>(relStart * pixelsPerBeat_);
    float w = static_cast<float>(section.lengthBeats * pixelsPerBeat_);

    // Bounds check
    if (x + w < 0 || x > bounds.getWidth())
      continue;

    // Make pills slightly inset from top/bottom
    float padding = 3.0f;
    SkRect rect =
        SkRect::MakeXYWH(x + 2, padding, w - 4,
                         static_cast<float>(bounds.getHeight()) - padding * 2);

    if (rect.width() < 8)
      continue; // Too small to render

    SkRRect rrect = SkRRect::MakeRectXY(rect, 6.0f, 6.0f);

    // Get section color
    SkColor c =
        SkColorSetARGB(255, section.color.getRed(), section.color.getGreen(),
                       section.color.getBlue());

    bool isDragging = (draggingSectionIndex_ == static_cast<int>(idx));

    // ========================================
    // 1. DROP SHADOW
    // ========================================
    {
      SkPaint shadowPaint;
      shadowPaint.setAntiAlias(true);
      shadowPaint.setColor(SkColorSetARGB(40, 0, 0, 0));
      shadowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
      SkRect shadowRect = rect;
      shadowRect.offset(0, 1);
      canvas->drawRRect(SkRRect::MakeRectXY(shadowRect, 6.0f, 6.0f),
                        shadowPaint);
    }

    // ========================================
    // 2. GLASSMORPHIC FILL
    // ========================================
    {
      SkPaint fillPaint;
      fillPaint.setAntiAlias(true);

      SkPoint pts[2] = {{rect.left(), rect.top()},
                        {rect.left(), rect.bottom()}};
      SkColor gradColors[3] = {
          withAlpha(lighten(c, 0.2f), isDragging ? 0.9f : 0.7f),
          withAlpha(c, isDragging ? 0.7f : 0.5f),
          withAlpha(darken(c, 0.2f), isDragging ? 0.6f : 0.4f)};
      float positions[3] = {0.0f, 0.4f, 1.0f};

      fillPaint.setShader(SkGradientShader::MakeLinear(
          pts, gradColors, positions, 3, SkTileMode::kClamp));
      canvas->drawRRect(rrect, fillPaint);
    }

    // ========================================
    // 3. GLOW (if dragging or active)
    // ========================================
    if (isDragging) {
      SkPaint glowPaint;
      glowPaint.setAntiAlias(true);
      glowPaint.setStyle(SkPaint::kStroke_Style);
      glowPaint.setStrokeWidth(3.0f);
      glowPaint.setColor(withAlpha(c, 0.6f));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
      canvas->drawRRect(rrect, glowPaint);
    }

    // ========================================
    // 4. BORDER
    // ========================================
    {
      SkPaint borderPaint;
      borderPaint.setAntiAlias(true);
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(1.0f);

      SkPoint borderPts[2] = {{rect.left(), rect.top()},
                              {rect.left(), rect.bottom()}};
      SkColor borderColors[2] = {withAlpha(lighten(c, 0.3f), 0.8f),
                                 withAlpha(c, 0.4f)};
      borderPaint.setShader(SkGradientShader::MakeLinear(
          borderPts, borderColors, nullptr, 2, SkTileMode::kClamp));
      canvas->drawRRect(rrect, borderPaint);
    }

    // ========================================
    // 5. TOP RIM HIGHLIGHT
    // ========================================
    {
      SkPaint rimPaint;
      rimPaint.setAntiAlias(true);
      rimPaint.setStyle(SkPaint::kStroke_Style);
      rimPaint.setStrokeWidth(1.0f);

      SkPoint rimPts[2] = {{rect.left(), rect.top()},
                           {rect.left() + rect.width() * 0.5f, rect.top() + 6}};
      SkColor rimColors[2] = {SkColorSetARGB(100, 255, 255, 255),
                              SkColorSetARGB(0, 255, 255, 255)};
      rimPaint.setShader(SkGradientShader::MakeLinear(
          rimPts, rimColors, nullptr, 2, SkTileMode::kClamp));

      SkRRect innerRR = rrect;
      innerRR.inset(0.5f, 0.5f);
      canvas->drawRRect(innerRR, rimPaint);
    }

    // ========================================
    // 6. LABEL with pill background
    // ========================================
    if (w > 30) {
      juce::String label = section.name;

      // Text shadow
      SkPaint shadowPaint;
      shadowPaint.setAntiAlias(true);
      shadowPaint.setColor(SkColorSetARGB(120, 0, 0, 0));
      canvas->drawSimpleText(label.toRawUTF8(), label.length(),
                             SkTextEncoding::kUTF8, x + 9,
                             rect.centerY() + 4.5f, font, shadowPaint);

      // Main text
      SkPaint textPaint;
      textPaint.setColor(SK_ColorWHITE);
      textPaint.setAntiAlias(true);
      canvas->drawSimpleText(label.toRawUTF8(), label.length(),
                             SkTextEncoding::kUTF8, x + 8,
                             rect.centerY() + 4.0f, font, textPaint);
    }
  }
}

void ArrangerTrackComponent::mouseDown(const juce::MouseEvent &e) {
  // Hit test
  double clickBeats = (e.position.x / pixelsPerBeat_) + viewStartBeats_;

  draggingSectionIndex_ = -1;
  for (size_t i = 0; i < sections_.size(); ++i) {
    if (clickBeats >= sections_[i].startBeats &&
        clickBeats < sections_[i].startBeats + sections_[i].lengthBeats) {
      draggingSectionIndex_ = static_cast<int>(i);
      dragStartBeats_ = clickBeats;
      initialSectionStart_ = sections_[i].startBeats;
      repaint();
      return;
    }
  }
}

void ArrangerTrackComponent::mouseDrag(const juce::MouseEvent &e) {
  if (draggingSectionIndex_ >= 0) {
    double currentBeats = (e.position.x / pixelsPerBeat_) + viewStartBeats_;
    double delta = currentBeats - dragStartBeats_;

    // Update visual position temporarily
    sections_[draggingSectionIndex_].startBeats = initialSectionStart_ + delta;
    repaint();

    // In a real implementation, this would show a "Global Slice" guide line
    // down the entire ArrangerComponent to show what will be moved.
  }
}

void ArrangerTrackComponent::mouseUp(const juce::MouseEvent &e) {
  if (draggingSectionIndex_ >= 0) {
    // Commit Move
    double finalStart = sections_[draggingSectionIndex_].startBeats;

    // Snap to grid (simplified)
    finalStart = std::round(finalStart);

    moveSection(draggingSectionIndex_, finalStart);
  }
  draggingSectionIndex_ = -1;
}

void ArrangerTrackComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  // ADD NEW SECTION logic would go here
}

void ArrangerTrackComponent::moveSection(int index, double newStartBeats) {
  // THIS IS THE "PROMPT 4" CORE LOGIC
  // Moving a section block should logically move all clips inside that time
  // range.

  if (index < 0 || index >= sections_.size())
    return;

  // 1. Calculate Delta
  double originalStart = initialSectionStart_;
  double delta = newStartBeats - originalStart;

  if (std::abs(delta) < 0.001)
    return;

  auto &section = sections_[index];
  double sectionEnd = originalStart + section.lengthBeats;

  projectState.getUndoManager().beginNewTransaction(
      "Arranger Track: Move Section");

  // 2. Iterate ALL clips in the project
  // We need to access the raw ValueTree for the track list
  auto tracksNode =
      projectState.getState().getChildWithName(ProjectState::ID_TRACKS);

  for (auto track : tracksNode) {
    auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
    if (!clipsNode.isValid()) {
      continue;
    }

    for (auto clip : clipsNode) {
      double clipStart = clip[ProjectState::PROP_START_BEATS];

      // check if clip starts INSIDE the section
      if (clipStart >= originalStart && clipStart < sectionEnd) {
        double newClipStart = clipStart + delta;

        // Apply move
        clip.setProperty(ProjectState::PROP_START_BEATS, newClipStart,
                         &projectState.getUndoManager());
      }
    }
  }

  // Update local model
  section.startBeats = newStartBeats;
  repaint();
}

void ArrangerTrackComponent::setVisibleRange(double startBeats,
                                             double endBeats) {
  juce::ignoreUnused(endBeats);
  viewStartBeats_ = startBeats;
  repaint();
}

const ArrangementSection *ArrangerTrackComponent::getHoveredSection() const {
  return nullptr;
}

const ArrangementSection *ArrangerTrackComponent::getDraggingSection() const {
  if (draggingSectionIndex_ >= 0 &&
      draggingSectionIndex_ < (int)sections_.size()) {
    return &sections_[draggingSectionIndex_];
  }
  return nullptr;
}

} // namespace zenith
