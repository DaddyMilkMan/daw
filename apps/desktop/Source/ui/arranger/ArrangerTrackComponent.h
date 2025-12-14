/*
  ==============================================================================

    ArrangerTrackComponent.h
    Created: 2025-12-12
    Author:  Zenith AI

  ==============================================================================
*/

#pragma once

#include "../../include/ProjectState.h"
#include "skia/SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace zenith {

class ArrangerComponent; // Forward declaraton

class ArrangerTrackComponent : public SkiaComponent,
                               public juce::ValueTree::Listener {
public:
  ArrangerTrackComponent(ProjectState &state, ArrangerComponent &arranger);
  ~ArrangerTrackComponent() override;

  void drawSkia(SkCanvas *canvas) override;

  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;

  // ValueTree::Listener
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;

  // Render State
  void setVisibleRange(double startBeats, double pixelsPerBeat);

  // Interaction State
  struct SectionView {
    juce::String id;
    juce::String name;
    juce::String color; // Hex string
    double startBeats;
    double lengthBeats;
    juce::Rectangle<float> bounds;
  };

  const SectionView *getHoveredSection() const { return hoveredSection_; }
  const SectionView *getDraggingSection() const { return draggingSection_; }

private:
  ProjectState &projectState_;
  ArrangerComponent &arranger_;

  juce::Array<SectionView> sections_;
  double viewStartBeats_ = 0.0;
  double pixelsPerBeat_ = 50.0;

  const SectionView *hoveredSection_ = nullptr;
  const SectionView *draggingSection_ = nullptr;

  bool isDragging_ = false;
  double dragStartBeats_ = 0.0;
  double sectionOriginalStart_ = 0.0;

  void rebuildSections();
  const SectionView *findSectionAt(juce::Point<float> pos) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerTrackComponent)
};

} // namespace zenith
