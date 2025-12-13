/**
 * @file ArrangerTrackComponent.h
 * @brief Component for managing song sections (Verse, Chorus, etc.)
 */

#pragma once

#include "../../Source/ui/skia/SkiaComponent.h"
#include "../ProjectState.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

struct ArrangementSection {
  juce::String name;
  double startBeats;
  double lengthBeats;
  juce::Colour color;
};

class ArrangerTrackComponent : public SkiaComponent {
public:
  ArrangerTrackComponent(ProjectState &ps);
  ~ArrangerTrackComponent() override;

  void drawSkia(SkCanvas *canvas) override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;

  // Set the view parameters for rendering
  void setViewContext(double pixelsPerBeat, double viewStartBeats);

  // Command to re-order sections (The "Magic" of this feature)
  void moveSection(int index, double newStartBeats);

private:
  ProjectState &projectState;
  std::vector<ArrangementSection> sections_; // Cache

  // View State
  double pixelsPerBeat_ = 50.0;
  double viewStartBeats_ = 0.0;

  // Interaction
  int draggingSectionIndex_ = -1;
  double dragStartBeats_ = 0.0;
  double initialSectionStart_ = 0.0;

  void rebuildSections(); // Pull from ProjectState

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerTrackComponent)
};

} // namespace zenith
