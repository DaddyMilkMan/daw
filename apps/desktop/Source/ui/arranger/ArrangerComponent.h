#pragma once

#include "../../engine/Engine.h"
#include "MiniMapComponent.h"
#include "../../engine/ProjectState.h"
#include "../framework/SkiaComponent.h"
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
#include "../common/MacroToolbar.h"
#include "ArrangerTypes.h"

// Forward declarations
namespace zenith {
class BrowserDragData;
class ArrangerGridUtils;
class ArrangerClipManager;
class ArrangerInputHandler;
class ArrangerRenderer;
class GridResolutionDropdown;
class CommandAPI;
class ArrangerTrackComponent;
}

namespace zenith {

class ArrangerComponent : public SkiaComponent,
                          public juce::ValueTree::Listener,
                          public juce::DragAndDropTarget,
                          public juce::DragAndDropContainer {
public:
  ArrangerComponent(Engine &engine, ProjectState &ps, CommandAPI &api);
  ~ArrangerComponent() override;

  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

  bool keyPressed(const juce::KeyPress &key) override;

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

  // Accessors for helper classes
  ArrangerGridUtils* getGridUtils() { return gridUtils_.get(); }
  ArrangerClipManager* getClipManager() { return clipManager_.get(); }
  Engine& getEngine() { return engine_; }
  CommandAPI& getCommandAPI() { return commandAPI; }

  // Tool management
  ArrangerTool getTool() const { return currentTool_; }
  void setTool(ArrangerTool tool) { currentTool_ = tool; repaint(); }

  // UI Verification
  void runVerificationRender();

private:
  friend class ArrangerGridUtils;
  friend class ArrangerClipManager;
  friend class ArrangerInputHandler;
  friend class ArrangerRenderer;

  Engine &engine_;
  zenith::ProjectState &projectState;
  CommandAPI &commandAPI;

  // Helper modules
  std::unique_ptr<ArrangerGridUtils> gridUtils_;
  std::unique_ptr<ArrangerClipManager> clipManager_;
  std::unique_ptr<ArrangerInputHandler> inputHandler_;
  std::unique_ptr<ArrangerRenderer> renderer_;

  // UI Components
  std::unique_ptr<MacroToolbar> macroToolbar;
  std::unique_ptr<FreezeProgressOverlay> freezeOverlay;
  std::unique_ptr<ArrangerTrackComponent> sectionTrack;
  std::unique_ptr<GridResolutionDropdown> gridDropdown;
  std::vector<std::unique_ptr<ArrangerTrackComponent>> trackComponents;
  TimelineRuler timelineRuler;
  MiniMapComponent miniMap;

  void rebuildTrackComponents();
  void updatePlayheadFromEngine();

  GridResolution gridResolution_ = GridResolution::Bar_1;
  double gridSnapBeats = 4.0; // Beats per bar for 4/4 time

  // View state
  double pixelsPerBeat = 40.0;
  double viewStartBeats = 0.0;
  double playheadBeats_ = 0.0;
  bool isPlaying_ = false;
  bool followPlayhead_ = true;
  int firstVisibleTrackIndex = 0;

  // Loop state
  bool loopEnabled_ = false;
  double loopStartBeats_ = 0.0;
  double loopEndBeats_ = 4.0;

  // Drag and drop state
  bool isDropTargetActive_ = false;
  int dropTargetTrackIndex_ = -1;
  double dropTargetBeats_ = 0.0;

  ArrangerTool currentTool_ = ArrangerTool::Select;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
