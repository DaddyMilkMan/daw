/*
  ==============================================================================

    SessionViewComponent.h
    Created: 2025-12-12
    Author:  Zenith DAW Team

    Session View (Clip Launcher) - Ableton-style grid layout

    Features:
    - Grid layout: Columns = Tracks, Rows = Scenes
    - Clip slots with waveform/MIDI thumbnails
    - Play/Stop buttons on hover
    - Recording state indicator
    - Color-coded by clip type
    - Scene launch column
    - Track headers with arm/solo/mute
    - Drag-and-drop support
    - Playing clip animation

  ==============================================================================
*/

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include "SkiaComponent.h"
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkMaskFilter.h>
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>
#endif

#include <unordered_map>
#include <vector>

namespace zenith {

/**
 * @class SessionViewComponent
 * @brief Ableton-style clip launcher grid view
 *
 * Provides a session/clip launcher view with:
 * - Grid layout (tracks as columns, scenes as rows)
 * - Clip slots with visual feedback
 * - Scene launch controls
 * - Track controls (arm, solo, mute)
 * - Drag-and-drop support for clip rearrangement
 */
class SessionViewComponent : public SkiaComponent,
                             public juce::ValueTree::Listener,
                             public juce::DragAndDropTarget {
public:
  //==========================================================================
  // Construction/Destruction
  //==========================================================================

  SessionViewComponent(Engine &engine, ProjectState &state);
  ~SessionViewComponent() override;

  //==========================================================================
  // Component Interface
  //==========================================================================

  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;

  //==========================================================================
  // ValueTree::Listener
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
  // Timer
  //==========================================================================

  void timerCallback() override;

  //==========================================================================
  // DragAndDropTarget
  //==========================================================================

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

  void drawSkia(SkCanvas *canvas) override;

  //==========================================================================
  // Session Control
  //==========================================================================

  /**
   * @brief Set the number of visible scenes (rows)
   */
  void setNumScenes(int numScenes);
  int getNumScenes() const { return numScenes_; }

  /**
   * @brief Launch a specific clip
   */
  void launchClip(int trackIndex, int sceneIndex);

  /**
   * @brief Stop a specific clip
   */
  void stopClip(int trackIndex, int sceneIndex);

  /**
   * @brief Launch an entire scene (row)
   */
  void launchScene(int sceneIndex);

  /**
   * @brief Stop all clips
   */
  void stopAllClips();

private:
  //==========================================================================
  // Internal Structures
  //==========================================================================

  struct ClipSlot {
    juce::String clipId;
    juce::String trackId;
    juce::String name;
    bool hasClip = false;
    bool isMidi = false;
    bool isPlaying = false;
    bool isRecording = false;
    bool isQueued = false; // Queued for launch
    juce::Colour clipColor;
    juce::Rectangle<float> bounds;

    // Waveform/MIDI preview data
    std::vector<float> waveformPeaks;
    std::vector<std::pair<int, float>> midiNotes; // pitch, position
  };

  struct TrackHeader {
    juce::String trackId;
    juce::String name;
    bool isArmed = false;
    bool isSoloed = false;
    bool isMuted = false;
    juce::Colour trackColor;
    juce::Rectangle<float> bounds;
    juce::Rectangle<float> armButtonBounds;
    juce::Rectangle<float> soloButtonBounds;
    juce::Rectangle<float> muteButtonBounds;
  };

  struct SceneRow {
    int sceneIndex;
    juce::String name;
    juce::Rectangle<float> launchButtonBounds;
  };

  enum class HoverState {
    None,
    ClipSlot,
    ClipPlayButton,
    ClipStopButton,
    TrackArm,
    TrackSolo,
    TrackMute,
    SceneLaunch
  };

  //==========================================================================
  // Layout Constants
  //==========================================================================

  static constexpr float TRACK_HEADER_HEIGHT = 80.0f;
  static constexpr float CLIP_SLOT_WIDTH = 140.0f;
  static constexpr float CLIP_SLOT_HEIGHT = 90.0f;
  static constexpr float SCENE_LAUNCH_WIDTH = 60.0f;
  static constexpr float SLOT_SPACING = 4.0f;
  static constexpr float MARGIN = 12.0f;
  static constexpr float CORNER_RADIUS = 6.0f;
  static constexpr float BUTTON_SIZE = 24.0f;

  //==========================================================================
  // Drawing Methods
  //==========================================================================

#ifdef ZENITH_USE_SKIA
  void drawBackground(SkCanvas *canvas);
  void drawTrackHeaders(SkCanvas *canvas);
  void drawClipGrid(SkCanvas *canvas);
  void drawSceneLaunchColumn(SkCanvas *canvas);
  void drawClipSlot(SkCanvas *canvas, const ClipSlot &slot, bool isHovered);
  void drawEmptySlot(SkCanvas *canvas, const juce::Rectangle<float> &bounds,
                     bool isHovered, bool isRecordArmed);
  void drawWaveformPreview(SkCanvas *canvas, const ClipSlot &slot,
                           const SkRect &contentRect);
  void drawMidiPreview(SkCanvas *canvas, const ClipSlot &slot,
                       const SkRect &contentRect);
  void drawPlayingIndicator(SkCanvas *canvas,
                            const juce::Rectangle<float> &bounds,
                            float animPhase);
  void drawQueuedIndicator(SkCanvas *canvas,
                           const juce::Rectangle<float> &bounds,
                           float animPhase);
  void drawTrackControlButtons(SkCanvas *canvas, const TrackHeader &header);
#endif

  //==========================================================================
  // Layout Methods
  //==========================================================================

  void rebuildLayout();
  void rebuildClipSlots();
  void updateClipSlotBounds();

  //==========================================================================
  // Hit Testing
  //==========================================================================

  ClipSlot *findSlotAt(juce::Point<float> pos);
  TrackHeader *findTrackHeaderAt(juce::Point<float> pos);
  int findSceneAt(juce::Point<float> pos);
  bool isPointInPlayButton(const ClipSlot &slot, juce::Point<float> pos);
  bool isPointInStopButton(const ClipSlot &slot, juce::Point<float> pos);
  bool isPointInArmButton(const TrackHeader &header, juce::Point<float> pos);
  bool isPointInSoloButton(const TrackHeader &header, juce::Point<float> pos);
  bool isPointInMuteButton(const TrackHeader &header, juce::Point<float> pos);
  bool isPointInSceneLaunch(int sceneIndex, juce::Point<float> pos);

  //==========================================================================
  // State Management
  //==========================================================================

  void updateHoverState(juce::Point<float> pos);
  void toggleTrackArm(const juce::String &trackId);
  void toggleTrackSolo(const juce::String &trackId);
  void toggleTrackMute(const juce::String &trackId);

  //==========================================================================
  // Data Building
  //==========================================================================

  void buildWaveformPreview(ClipSlot &slot, const juce::String &audioFilePath);
  void buildMidiPreview(ClipSlot &slot, const juce::ValueTree &clipTree);

  //==========================================================================
  // Member Variables
  //==========================================================================

  Engine &engine_;
  ProjectState &projectState_;

  // Layout data
  std::vector<TrackHeader> trackHeaders_;
  std::vector<std::vector<ClipSlot>> clipGrid_; // [trackIndex][sceneIndex]
  std::vector<SceneRow> sceneRows_;

  int numScenes_ = 8;

  // Scroll state
  float scrollX_ = 0.0f;
  float scrollY_ = 0.0f;

  // Hover/interaction state
  HoverState currentHoverState_ = HoverState::None;
  int hoveredTrackIndex_ = -1;
  int hoveredSceneIndex_ = -1;
  ClipSlot *hoveredSlot_ = nullptr;

  // Drag state
  bool isDragging_ = false;
  juce::Point<float> dragStartPos_;
  ClipSlot *draggedSlot_ = nullptr;
  int dropTargetTrack_ = -1;
  int dropTargetScene_ = -1;

  // Animation
  float animationPhase_ = 0.0f;

  // Drop target highlight
  bool isDropTargetActive_ = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionViewComponent)
};

} // namespace zenith
