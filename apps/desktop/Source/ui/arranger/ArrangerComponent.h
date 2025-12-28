<<<<<<< HEAD
=======
/**
 * @file ArrangerComponent.h
 * @brief Timeline/Arranger view component for Zenith DAW
 *
 * The ArrangerComponent is the main timeline view that displays tracks and
 * clips. It delegates to specialized helper classes for different concerns:
 * - ArrangerGridUtils: Coordinate conversion and waveform caching
 * - ArrangerClipManager: Clip lifecycle and selection
 * - ArrangerInputHandler: Mouse and keyboard input
 * - ArrangerRenderer: Skia drawing (when ZENITH_USE_SKIA is defined)
 */
>>>>>>> origin/master
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

<<<<<<< HEAD
class ArrangerTrackComponent; // Forward declaration

//==============================================================================
// Grid resolution options for snapping
enum class GridResolution {
  Bar_1 = 0, // 4 beats (in 4/4)
  Beat_1,    // 1 beat (quarter note)
  Beat_1_2,  // 1/2 beat (eighth note)
  Beat_1_4,  // 1/4 beat (sixteenth note)
  Beat_1_8,  // 1/8 beat (thirty-second)
  Beat_1_3,  // 1/3 beat (triplet eighth)
  Beat_1_6,  // 1/6 beat (triplet sixteenth)
  Off        // No snap
};

// Convert grid resolution to beat value
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

=======
// Forward declarations for helper classes

class ArrangerGridUtils;
class ArrangerClipManager;
class ArrangerInputHandler;
class ArrangerTrackComponent;
class GridResolutionDropdown;
struct ClipView;

#ifdef ZENITH_USE_SKIA
class ArrangerRenderer;
#endif



//==============================================================================

//==============================================================================
/**
 * @class ArrangerComponent
 * @brief Main timeline/arranger view for the DAW
 *
 * Displays tracks and clips in a horizontal timeline. Supports:
 * - Clip selection, movement, and resizing
 * - Zoom and scroll navigation
 * - Drag-and-drop from browser
 * - Keyboard shortcuts for editing
 * - Premium glassmorphic Skia rendering
 */
>>>>>>> origin/master
class ArrangerComponent : public SkiaComponent,
                          public juce::ValueTree::Listener,
                          public juce::DragAndDropTarget {
public:
<<<<<<< HEAD
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

  //==========================================================================
  // Waveform Cache Entry (pre-computed peak data for fast rendering)
  //==========================================================================
  struct WaveformCache {
    juce::String audioFilePath;
    std::vector<float> minPeaks; // Downsampled min peaks
    std::vector<float> maxPeaks; // Downsampled max peaks
    int samplesPerPixel = 512;   // Resolution
    bool isValid = false;
  };

  //==========================================================================
  // MIDI Note Blob (for clip thumbnail rendering)
  //==========================================================================
  struct MidiNoteBlob {
    int pitch;
    double startBeats;
    double lengthBeats;
  };

  struct ClipView {
    juce::String clipId;
    juce::String trackId;
    int trackIndex =
        0; // Cached track index for O(1) lookups (avoids O(N²) searches)
    double startBeats;
    double lengthBeats;
    bool isMidi;
    bool isSelected;
    juce::Rectangle<float> bounds;

    // Cached content for rendering
    juce::String audioFilePath;          // For audio clips
    std::vector<MidiNoteBlob> noteBlobs; // For MIDI clips

    bool isInLeftResizeZone(juce::Point<float> p) const {
      return p.x >= bounds.getX() && p.x <= bounds.getX() + 5.0f;
    }

    bool isInRightResizeZone(juce::Point<float> p) const {
      return p.x >= bounds.getRight() - 5.0f && p.x <= bounds.getRight();
    }
  };

  bool keyPressed(const juce::KeyPress &key) override;

  juce::Array<ClipView> clipViews;
  juce::StringArray selectedClipIds;

  // MiniMap
  MiniMapComponent miniMap;

  // View state
  double pixelsPerBeat = 50.0;
  double viewStartBeats = 0.0;
  int firstVisibleTrackIndex = 0;

  double gridSnapBeats = 1.0;
  GridResolution gridResolution_ = GridResolution::Beat_1;

  // Playhead state (updated from Engine via timer)
  double playheadBeats_ = 0.0;
  bool isPlaying_ = false;
  bool followPlayhead_ = true; // Auto-scroll to follow playhead

  // Loop region state
  bool loopEnabled_ = false;
  double loopStartBeats_ = 0.0;
  double loopEndBeats_ = 8.0;

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

  // Drag optimization state
  double lastDragDeltaBeats_ = -99999.0;
  int lastDragDeltaTrack_ = -99999;

  void drawClips(SkCanvas *canvas);
  void drawTracks(SkCanvas *canvas);

  // Drop zone state (for browser drag-and-drop)
  bool isDropTargetActive_ = false;
  int dropTargetTrackIndex_ = -1;
  double dropTargetBeats_ = 0.0;

  // Edit Mode state
  enum class EditMode {
    Overwrite, // Default: Move clips freely, overlapping if needed
    Insert,    // Push content to the right to make room (Splicing)
    Ripple     // Push subsequent content by the exact same delta (Ripple Edit)
  };
  EditMode currentEditMode = EditMode::Overwrite;

  // Visuals for Insert/Ripple
  float insertionGuideX = -1.0f;

  // Store initial positions of ALL clips during drag for robust Ripple/Insert
  // logic
  std::map<juce::String, double> initialClipStarts;

  //==========================================================================
  // Clip Content Rendering Helpers (Skia)
  //==========================================================================
#ifdef ZENITH_USE_SKIA
  void drawClipWaveform(SkCanvas *canvas, const ClipView &clip,
                        const SkRect &clipRect);
  void drawClipMidiBlobs(SkCanvas *canvas, const ClipView &clip,
                         const SkRect &clipRect);
#endif

  // Bar.Beat.Tick formatting
  juce::String formatBarBeatTick(double beats) const;
  int getBeatsPerBar() const;

  // Waveform cache (file path -> cached peaks)
  std::unordered_map<juce::String, WaveformCache> waveformCache_;
  void buildWaveformCache(const juce::String &audioFilePath);
  const WaveformCache *
  getWaveformCache(const juce::String &audioFilePath) const;

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

  // Utility
  void updatePlayheadFromEngine();
  double samplesToBeats(juce::int64 samples) const;

=======
  /**
   * @brief Construct arranger component
   * @param engine Reference to the audio engine
   * @param engine Reference to the audio engine
   * @param ps Reference to the project state
   * @param api Reference to the command API
   */
  ArrangerComponent(Engine &engine, ProjectState &ps, CommandAPI &api);


  ~ArrangerComponent() override;

  //==========================================================================
  // Component Interface
  //==========================================================================

  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseWheelMove(const juce::MouseEvent &e,
                      const juce::MouseWheelDetails &wheel) override;

  //==========================================================================
  // Callbacks
  //==========================================================================

  /** @brief Callback when a clip is double-clicked (for opening editor) */
  std::function<void(const juce::String &trackId, const juce::String &clipId)>
      onClipDoubleClicked;

  //==========================================================================
  // ValueTree::Listener Interface
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;

  //==========================================================================
  // Skia Rendering
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override;

  //==========================================================================
  // Tooltip
  //==========================================================================

  juce::String getTooltip();

  //==========================================================================
  // DragAndDropTarget Interface
  //==========================================================================

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

  //==========================================================================
  // Grid Resolution Control
  //==========================================================================

  void setGridResolution(GridResolution res);
  GridResolution getGridResolution() const { return gridResolution_; }

  void setTool(ArrangerTool t) { currentTool_ = t; }
  ArrangerTool getTool() const { return currentTool_; }

  /** @brief Get access to the clip manager for collaboration features */
  ArrangerClipManager* getClipManager() { return clipManager_.get(); }
  const ArrangerClipManager* getClipManager() const { return clipManager_.get(); }

  //==========================================================================
  // Timer Interface
  //==========================================================================

  void timerCallback() override;

  CommandAPI& getCommandAPI() { return commandAPI; }

private:

  // Allow helper classes to access private members
  friend class ArrangerGridUtils;
  friend class ArrangerClipManager;
  friend class ArrangerInputHandler;
#ifdef ZENITH_USE_SKIA
  friend class ArrangerRenderer;
#endif

  //==========================================================================
  // Core References
  //==========================================================================

  std::unique_ptr<GridResolutionDropdown> gridDropdown;

  Engine &engine_;
  ProjectState &projectState;
  CommandAPI &commandAPI;


  //==========================================================================
  // Helper Module Objects
  //==========================================================================

  std::unique_ptr<ArrangerGridUtils> gridUtils_;
  std::unique_ptr<ArrangerClipManager> clipManager_;
  std::unique_ptr<ArrangerInputHandler> inputHandler_;
#ifdef ZENITH_USE_SKIA
  std::unique_ptr<ArrangerRenderer> renderer_;
#endif

  //==========================================================================
  // Child Components
  //==========================================================================

  MiniMapComponent miniMap;
  TimelineRuler timelineRuler;
>>>>>>> origin/master
  std::unique_ptr<MacroToolbar> macroToolbar;
  std::unique_ptr<FreezeProgressOverlay> freezeOverlay;
  std::unique_ptr<ArrangerTrackComponent> sectionTrack;
  std::vector<std::unique_ptr<ArrangerTrackComponent>> trackComponents;
<<<<<<< HEAD
  void rebuildTrackComponents();

=======

  //==========================================================================
  // View State
  //==========================================================================

  double pixelsPerBeat = 50.0;
  double viewStartBeats = 0.0;
  int firstVisibleTrackIndex = 0;

  double gridSnapBeats = 1.0;
  GridResolution gridResolution_ = GridResolution::Beat_1;
  ArrangerTool currentTool_ = ArrangerTool::Select;

  //==========================================================================
  // Playhead State
  //==========================================================================

  double playheadBeats_ = 0.0;
  bool isPlaying_ = false;
  bool followPlayhead_ = true;

  //==========================================================================
  // Loop Region State
  //==========================================================================

  bool loopEnabled_ = false;
  double loopStartBeats_ = 0.0;
  double loopEndBeats_ = 8.0;

  //==========================================================================
  // Drop Zone State
  //==========================================================================

  bool isDropTargetActive_ = false;
  int dropTargetTrackIndex_ = -1;
  double dropTargetBeats_ = 0.0;

  //==========================================================================
  // Private Methods
  //==========================================================================

  bool keyPressed(const juce::KeyPress &key) override;
  void updatePlayheadFromEngine();

>>>>>>> origin/master
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerComponent)
};

} // namespace zenith
