/*
  ==============================================================================

    TakeFolderComponent.h
    Created: 2025-12-24
    Author:  Zenith DAW

    Component for rendering and interacting with a Take Folder.
    Displays stacked take lanes and handles comp region selection.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "SkiaComponent.h"
#include "../../engine/ProjectState.h"
#include "../../engine/TakeFolder.h"

namespace zenith {

class ProjectState;
class ArrangerGridUtils;

class TakeFolderComponent : public SkiaComponent,
                            public juce::ValueTree::Listener {
public:
  //==========================================================================
  // Construction
  //==========================================================================

  TakeFolderComponent(ProjectState &projectState, ArrangerGridUtils &gridUtils,
                      juce::ValueTree folderNode);
  ~TakeFolderComponent() override;

  //==========================================================================
  // Component Overrides
  //==========================================================================

  void paint(juce::Graphics &g) override; // Fallback paint
  void drawSkia(SkCanvas *canvas) override;   // Skia paint
  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;

  //==========================================================================
  // State Management
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent,
                             juce::ValueTree &child, int) override;

  //==========================================================================
  // Setup
  //==========================================================================

  void setZoomLevel(double pixelsPerBeat);
  void setHeightPerLane(int height);

  juce::ValueTree getValueTree() const { return folderNode_; }
  void updateBounds(double pixelsPerBeat, int y, int height) {
      pixelsPerBeat_ = pixelsPerBeat;
      // Bounds handled by setBounds in ArrangerTrackComponent
      (void)y; (void)height;
  }

private:
  ProjectState &projectState_;
  ArrangerGridUtils &gridUtils_;
  juce::ValueTree folderNode_;
  
  // Cached state
  bool isExpanded_{false};
  int activeTakeIndex_{-1};
  double startBeats_{0};
  double lengthBeats_{0};
  
  double pixelsPerBeat_{100.0};
  int laneHeight_{60};
  int headerHeight_{24};

  // Interaction
  struct DragState {
    bool active{false};
    juce::Point<int> startPos;
    double startBeat{0};
    int takeIndex{-1};
  };
  DragState dragState_;

  // Helpers
  int getTakeIndexAtY(int y) const;
  double getBeatAtX(int x) const;
  juce::Rectangle<int> getCompRegionBounds(const CompRegion& region, int laneIndex) const;
  
  void updateState();
  void drawCollapsed(SkCanvas* canvas);
  void drawExpanded(SkCanvas* canvas);
  void drawTakeWaveform(SkCanvas* canvas, const juce::ValueTree& clipNode, SkRect bounds, SkColor color);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TakeFolderComponent)
};

} // namespace zenith
