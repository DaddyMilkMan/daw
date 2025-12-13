/**
 * @file ArrangerTrackComponent.cpp
 * @brief Implementation of the 'Arranger Track' for global section editing.
 */

#include "../../include/ui/ArrangerTrackComponent.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>


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

  // Background
  canvas->clear(SkColorSetARGB(255, 30, 30, 30));

  // Draw Sections
  SkPaint paint;
  paint.setAntiAlias(true);

  SkFont font = typography::getSkFont(typography::FONT_XS, FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(SK_ColorWHITE);
  textPaint.setAntiAlias(true);

  for (const auto &section : sections_) {
    // Calculate X position
    double relStart = section.startBeats - viewStartBeats_;
    float x = static_cast<float>(relStart * pixelsPerBeat_);
    float w = static_cast<float>(section.lengthBeats * pixelsPerBeat_);

    // Bounds check
    if (x + w < 0 || x > bounds.getWidth())
      continue;

    SkRect rect = SkRect::MakeXYWH(
        x, 2.0f, w, static_cast<float>(bounds.getHeight()) - 4.0f);
    SkRRect rrect;
    rrect.setRectXY(rect, 4.0f, 4.0f);

    // Fill
    // Convert JUCE color to SkColor
    SkColor c =
        SkColorSetARGB(section.color.getAlpha(), section.color.getRed(),
                       section.color.getGreen(), section.color.getBlue());
    paint.setColor(SkColorSetA(c, 180)); // Semi-transparent
    canvas->drawRRect(rrect, paint);

    // Border
    paint.setColor(c);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.0f);
    canvas->drawRRect(rrect, paint);
    paint.setStyle(SkPaint::kFill_Style); // Reset

    // Label
    if (w > 20) {
      juce::String label = section.name;
      canvas->save();
      canvas->clipRRect(rrect, true);
      canvas->drawSimpleText(label.toRawUTF8(), label.length(),
                             SkTextEncoding::kUTF8, x + 5.0f,
                             rect.centerY() + 4.0f, font, textPaint);
      canvas->restore();
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
    auto clipListId = ProjectState::ID_CLIPS; // or whatever the child ID is
    auto clipsNode = track.getChildWithName(
        clipListId); // In real engine this structure varies

    // Iterate clips (mock loop for structure)
    for (int i = 0; i < track.getNumChildren(); ++i) {
      auto child = track.getChild(i);
      if (child.hasType(ProjectState::ID_CLIPS)) {
        for (auto clip : child) {
          double clipStart = clip[ProjectState::PROP_START_BEATS];
          double clipLen = clip[ProjectState::PROP_LENGTH_BEATS];

          // check if clip starts INSIDE the section
          if (clipStart >= originalStart && clipStart < sectionEnd) {
            double newClipStart = clipStart + delta;

            // Apply move
            clip.setProperty(ProjectState::PROP_START_BEATS, newClipStart,
                             &projectState.getUndoManager());
          }
        }
      }
    }
  }

  // Update local model
  section.startBeats = newStartBeats;
  repaint();
}

} // namespace zenith
