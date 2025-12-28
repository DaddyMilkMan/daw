#pragma once

#include "Engine.h"
#include "MiniMapComponent.h"
#include "ProjectState.h"
#include "SkiaComponent.h"
#include "TimelineRuler.h"
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#include <core/SkCanvas.h>
#include <map>
#include <unordered_map>
#include <vector>

#include "../controls/FreezeProgressOverlay.h"
#include "MacroToolbar.h"
#include "ArrangerTypes.h"

// Forward declaration for browser drag
namespace zenith {
class BrowserDragData;
}

namespace zenith {
class CommandAPI;

class ArrangerTrackComponent; // Forward declaration

class ArrangerComponent : public SkiaComponent,
                          public juce::ValueTree::Listener,
                          public juce::DragAndDropTarget {
public:
  ArrangerComponent(Engine &engine, ProjectState &ps);
  ~ArrangerComponent() override;

  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

  std::function<void(const juce::String &trackId, const juce::String &clipId)>
      onClipDoubleClicked;

  // ValueTree::Listener
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;

  void drawSkia(SkCanvas *canvas) override;

  juce::String getTooltip();

  // DragAndDropTarget interface
  bool isInterestedInDragSource(
      const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDropped(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragEnter(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragExit(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragMove(const juce::DragAndDropTarget::SourceDetails &details) override;

  // Grid resolution control
  void setGridResolution(GridResolution res);
  GridResolution getGridResolution() const { return gridResolution_; }

  // Timer callback for playhead updates
  void timerCallback() override;

private:
  Engine &engine_;
  zenith::ProjectState &projectState;

  std::unique_ptr<MacroToolbar> macroToolbar;
  std::unique_ptr<FreezeProgressOverlay> freezeOverlay;
  std::unique_ptr<ArrangerTrackComponent> sectionTrack;
  std::vector<std::unique_ptr<ArrangerTrackComponent>> trackComponents;
  void rebuildTrackComponents();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
