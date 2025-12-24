/**
 * @file ArrangerComponent.h
 * @brief Timeline/Arranger view component for Zenith DAW
 */
#pragma once

#include "../../engine/Engine.h"
#include "../../engine/ProjectState.h"
#include "../framework/SkiaComponent.h"
#include "MiniMapComponent.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
#include "ZenithSkia.h"
#endif
#include <map>
#include <unordered_map>

#include "MacroToolbar.h"

namespace zenith {
class BrowserDragData;
class ArrangerGridUtils;
class ArrangerClipManager;
class ArrangerInputHandler;
class ArrangerTrackComponent;
struct ClipView;

#ifdef ZENITH_USE_SKIA
class ArrangerRenderer;
#endif

//==============================================================================
enum class GridResolution {
  Bar_1 = 0, ///< 4 beats (in 4/4)
  Beat_1,    ///< 1 beat (quarter note)
  Beat_1_2,  ///< 1/2 beat (eighth note)
  Beat_1_4,  ///< 1/4 beat (sixteenth note)
  Beat_1_8,  ///< 1/8 beat (thirty-second)
  Beat_1_3,  ///< 1/3 beat (triplet eighth)
  Beat_1_6,  ///< 1/6 beat (triplet sixteenth)
  Off        ///< No snap
};

inline double gridResolutionToBeats(GridResolution res) {
  switch (res) {
  case GridResolution::Bar_1:
    return 4.0;
  case GridResolution::Beat_1:
    return 1.0;
  case GridResolution::Beat_1_2:
    return 0.5;
  case GridResolution::Beat_1_4:
    return 0.25;
  case GridResolution::Beat_1_8:
    return 0.125;
  case GridResolution::Beat_1_3:
    return 1.0 / 3.0;
  case GridResolution::Beat_1_6:
    return 1.0 / 6.0;
  case GridResolution::Off:
    return 0.0;
  default:
    return 1.0;
  }
}

//==============================================================================
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

  void setGridResolution(GridResolution res);
  GridResolution getGridResolution() const { return gridResolution_; }

  void timerCallback() override;

  void rebuildTrackComponents();
  void syncTrackComponents();

private:
  friend class ArrangerGridUtils;
  friend class ArrangerClipManager;
  friend class ArrangerInputHandler;
#ifdef ZENITH_USE_SKIA
  friend class ArrangerRenderer;
#endif

  Engine &engine_;
  zenith::ProjectState &projectState;

  std::unique_ptr<ArrangerGridUtils> gridUtils_;
  std::unique_ptr<ArrangerClipManager> clipManager_;
  std::unique_ptr<ArrangerInputHandler> inputHandler_;
#ifdef ZENITH_USE_SKIA
  std::unique_ptr<ArrangerRenderer> renderer_;
#endif

  MiniMapComponent miniMap;
  std::unique_ptr<MacroToolbar> macroToolbar;
  std::unique_ptr<ArrangerTrackComponent> sectionTrack;
  std::vector<std::unique_ptr<ArrangerTrackComponent>> trackComponents;

  double pixelsPerBeat = 50.0;
  double viewStartBeats = 0.0;
  int firstVisibleTrackIndex = 0;

  double gridSnapBeats = 1.0;
  GridResolution gridResolution_ = GridResolution::Beat_1;

  double playheadBeats_ = 0.0;
  bool isPlaying_ = false;
  bool followPlayhead_ = true;

  bool loopEnabled_ = false;
  double loopStartBeats_ = 0.0;
  double loopEndBeats_ = 8.0;

  double lastEngineBeats_ = 0.0;
  double lastEngineTime_ = 0.0;
  std::unique_ptr<juce::VBlankAttachment> vBlankAttachment_;

  bool isDropTargetActive_ = false;
  int dropTargetTrackIndex_ = -1;
  double dropTargetBeats_ = 0.0;

  bool keyPressed(const juce::KeyPress &key) override;
  void updatePlayheadFromEngine();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
