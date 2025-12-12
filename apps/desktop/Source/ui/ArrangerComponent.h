#pragma once

#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include "skia/SkiaComponent.h"
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <core/SkCanvas.h>
#include <unordered_map>
#include <vector>

namespace zenith {

class ArrangerComponent : public SkiaComponent,
                          public juce::ValueTree::Listener,
                          public juce::Timer,
                          public juce::DragAndDropTarget {
public:
  ArrangerComponent(Engine &eng, ProjectState &ps);
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

  // ValueTree::Listener
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;

  // Timer
  void timerCallback() override;

  // DragAndDropTarget
  bool isInterestedInDragSource(
      const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragEnter(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragMove(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDragExit(const juce::DragAndDropTarget::SourceDetails &details) override;
  void
  itemDropped(const juce::DragAndDropTarget::SourceDetails &details) override;

#ifdef ZENITH_USE_SKIA
  void drawSkia(SkCanvas *canvas) override;
#endif

  juce::String getTooltip();

  // Callbacks
  std::function<void(const juce::String &trackId, const juce::String &clipId)>
      onClipDoubleClicked;

  // Grid control
  enum class GridResolution { Bar, HalfBar, Beat, HalfBeat, Quarter };
  void setGridResolution(GridResolution res);
  GridResolution getGridResolution() const { return gridResolution_; }

private:
  Engine &engine_;
  ProjectState &projectState;

  // MIDI Note Blob for thumbnails
  struct MidiNoteBlob {
    int pitch;
    double startBeats;
    double lengthBeats;
  };

  struct ClipView {
    juce::String clipId;
    juce::String trackId;
    double startBeats = 0.0;
    double lengthBeats = 0.0;
    bool isMidi = false;
    bool isSelected = false;
    juce::Rectangle<float> bounds;
    juce::String audioFilePath;
    std::vector<MidiNoteBlob> noteBlobs;

    bool isInLeftResizeZone(juce::Point<float> p) const {
      return p.x >= bounds.getX() && p.x <= bounds.getX() + 5.0f;
    }

    bool isInRightResizeZone(juce::Point<float> p) const {
      return p.x >= bounds.getRight() - 5.0f && p.x <= bounds.getRight();
    }
  };

  juce::Array<ClipView> clipViews;
  juce::StringArray selectedClipIds;

  // View state
  double pixelsPerBeat = 50.0;
  double viewStartBeats = 0.0;
  int firstVisibleTrackIndex = 0;
  double gridSnapBeats = 1.0;
  GridResolution gridResolution_ = GridResolution::Beat;

  // Playhead state
  double playheadBeats_ = 0.0;
  bool isPlaying_ = false;
  bool followPlayhead_ = true;

  // Loop state
  bool loopEnabled_ = false;
  double loopStartBeats_ = 0.0;
  double loopEndBeats_ = 0.0;

  // Drag state
  enum class DragMode {
    None,
    MoveClips,
    ResizeClipLeft,
    ResizeClipRight,
    Marquee
  };
  DragMode currentDragMode = DragMode::None;
  juce::Point<float> dragStartPoint;

  struct ClipDragState {
    juce::String clipId;
    double originalStartBeats;
    int originalTrackIndex;
  };
  juce::Array<ClipDragState> clipDragStates;

  juce::String resizingClipId;
  double resizeOriginalStart = 0.0;
  double resizeOriginalLength = 0.0;

  juce::Rectangle<float> marqueeRect;

  // Drag and drop state
  bool isDropTargetActive_ = false;
  int dropTargetTrackIndex_ = -1;
  double dropTargetBeats_ = 0.0;

  // Waveform cache for audio clips
  struct WaveformCache {
    std::vector<float> minPeaks;
    std::vector<float> maxPeaks;
    bool isValid = false;
  };
  mutable std::unordered_map<juce::String, WaveformCache> waveformCache_;

  // Methods
  void rebuildClipViews();
  void recomputeClipBounds();
  ClipView *findClipView(const juce::String &clipId);
  ClipView *findClipAtPoint(juce::Point<float> point);

  float beatsToX(double beats) const;
  double xToBeats(float x) const;
  float trackIndexToY(int trackIndex) const;
  int yToTrackIndex(float y) const;
  double snapToGrid(double beats) const;

  void clearSelection();
  void selectClip(const juce::String &clipId, bool addToSelection);
  void selectClipsInRect(juce::Rectangle<float> rect);
  bool isClipSelected(const juce::String &clipId) const;

  void createClipAtPoint(juce::Point<float> point);
  void deleteSelectedClips();
  void duplicateSelectedClips();

  void updatePlayheadFromEngine();
  double samplesToBeats(juce::int64 samples) const;
  int getBeatsPerBar() const;
  juce::String formatBarBeatTick(double beats) const;
  double gridResolutionToBeats(GridResolution res) const;

  void buildWaveformCache(const juce::String &audioFilePath);
  const WaveformCache *
  getWaveformCache(const juce::String &audioFilePath) const;

#ifdef ZENITH_USE_SKIA
  void drawClipWaveform(SkCanvas *canvas, const ClipView &clip,
                        const SkRect &clipRect);
  void drawClipMidiBlobs(SkCanvas *canvas, const ClipView &clip,
                         const SkRect &clipRect);
#endif

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
