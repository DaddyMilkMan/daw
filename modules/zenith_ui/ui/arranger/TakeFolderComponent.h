/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

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
#include <core/SkFont.h>
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

  // Cached Resources
  SkFont cachedFont_;
  SkFont cachedFontSmall_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TakeFolderComponent)
};

} // namespace zenith
