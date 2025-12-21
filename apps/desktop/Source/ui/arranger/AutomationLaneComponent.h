/**
 * @file AutomationLaneComponent.h
 * @brief UI component for editing track automation envelopes
 *
 * Phase U5: Automation Lanes UI
 *
 * Features:
 * - Displays automation curves (volume/pan/mute)
 * - Visual editing of automation points:
 *   - Click empty space: add point
 *   - Drag point: move point
 *   - Double-click point: delete point
 * - Integrates with ProjectState and UndoManager
 * - RT-safe: all edits on message thread only
 */

#pragma once

#include "ProjectState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include "SkiaTheme.h"
#include <include/core/SkCanvas.h>

#endif

//==============================================================================
/**
 * @class AutomationLaneComponent
 * @brief Visual editor for a single automation envelope (e.g. track volume)
 *
 * Data Flow:
 * - Query: Reads from ProjectState ValueTree
 * - Edit: Calls ProjectState methods (with undo support)
 * - Update: Listens to ValueTree changes, repaints automatically
 *
 * RT-Safety:
 * - All operations on message thread only
 * - No audio thread interaction
 * - TrackAutomationSynchronizer independently updates audio engine atomics
 */
class AutomationLaneComponent : public juce::Component,
                                public juce::ValueTree::Listener {
public:
  //==========================================================================
  /**
   * @brief Constructor
   * @param state Reference to project state
   * @param trackId Track identifier
   * @param paramId Parameter identifier ("volume", "pan", or "mute")
   */
  AutomationLaneComponent(zenith::ProjectState &state,
                          const juce::String &trackId,
                          const juce::String &paramId);

  /**
   * @brief Destructor
   */
  ~AutomationLaneComponent() override;

  //==========================================================================
  // Visual Settings
  //==========================================================================

  /**
   * @brief Set horizontal zoom (pixels per beat)
   * @param ppb Pixels per beat (e.g. 100.0)
   */
  void setPixelsPerBeat(double ppb);

  /**
   * @brief Set horizontal scroll offset
   * @param offset Offset in beats
   */
  void setScrollOffsetBeats(double offset);

  /**
   * @brief Enable/disable grid snapping
   * @param enabled Enable grid snap
   * @param gridBeats Grid size in beats (e.g. 1.0 for quarter notes)
   */
  void setGridSnapEnabled(bool enabled, double gridBeats = 1.0);

  //==========================================================================
  // Parameter Info
  //==========================================================================

  /**
   * @brief Parameter display information
   */
  struct ParamInfo {
    juce::String displayName; // "Volume", "Pan", "Mute"
    double minValue;          // e.g. 0.0 for volume
    double maxValue;          // e.g. 1.0 for volume
    juce::String units;       // "dB", "%", ""

    /**
     * @brief Convert value to display string
     */
    std::function<juce::String(double)> valueToString;
  };

  /**
   * @brief Set parameter display info
   * @param info Parameter info structure
   */
  void setParameterInfo(const ParamInfo &info);

  /**
   * @brief Get default parameter info for volume
   */
  static ParamInfo getDefaultVolumeInfo();

  //==========================================================================
  // Component Overrides
  //==========================================================================

  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

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

private:
  //==========================================================================
  // Data Binding
  //==========================================================================

  zenith::ProjectState &projectState;
  juce::String trackId;
  juce::String paramId;
  juce::ValueTree envelopeNode; // Cached reference to automation envelope

  //==========================================================================
  // Visual State
  //==========================================================================

  double pixelsPerBeat = 100.0;
  double scrollOffsetBeats = 0.0;
  bool gridSnapEnabled = false;
  double gridBeats = 1.0;
  ParamInfo paramInfo;

  //==========================================================================
  // Hit Testing
  //==========================================================================

  /**
   * @brief Screen-space representation of automation point
   */
  struct PointHandle {
    juce::String pointId;
    juce::Point<float> screenPos;
    float radius = 6.0f;

    bool hitTest(juce::Point<float> pos) const {
      return screenPos.getDistanceFrom(pos) <= radius;
    }

    // Added for tension support
    float tension = 0.0f;
  };

  std::vector<PointHandle> pointHandles; // Rebuilt each paint

  //==========================================================================
  // Drag State
  //==========================================================================

  juce::String draggedPointId;
  juce::Point<float> dragStartMousePos;
  double dragStartTimeBeats = 0.0;
  double dragStartValue = 0.0;
  bool isDragging = false;

  // Tension Dragging
  bool isDraggingTension = false;
  juce::String tensionPointId; // The point whose tension determines the curve
                               // (start point of segment)
  float tensionStartValue = 0.0f;
  juce::Point<float> tensionStartMousePos;

  //==========================================================================
  // Hover State (for tooltips and visual feedback)
  //==========================================================================

  juce::String hoveredPointId;
  juce::Point<float> hoveredPointScreenPos;
  double hoveredPointTime = 0.0;
  double hoveredPointValue = 0.0;

  // Cached render image
  juce::Image cachedImage_;

  //==========================================================================
  // Coordinate Conversion
  //==========================================================================

  /**
   * @brief Convert beat time to horizontal pixel position
   * @param timeBeats Time in beats
   * @return X coordinate in pixels
   */
  float beatsToPixels(double timeBeats) const;

  /**
   * @brief Convert horizontal pixel position to beat time
   * @param pixelX X coordinate in pixels
   * @return Time in beats
   */
  double pixelsToBeats(float pixelX) const;

  /**
   * @brief Convert parameter value to vertical pixel position
   * @param value Parameter value (normalized)
   * @return Y coordinate in pixels (inverted: 0 at top)
   */
  float valueToPixelY(double value) const;

  /**
   * @brief Convert vertical pixel position to parameter value
   * @param pixelY Y coordinate in pixels
   * @return Parameter value (normalized, clamped)
   */
  double pixelYToValue(float pixelY) const;

  //==========================================================================
  // Hit Testing
  //==========================================================================

  /**
   * @brief Find automation point at screen position
   * @param pos Screen position
   * @return Point ID if hit, empty string otherwise
   */
  juce::String findPointAtPosition(juce::Point<float> pos) const;

  /**
   * @brief Find automation segment at screen position
   * @param pos Screen position
   * @return Start Point ID of the segment if hit, empty string otherwise
   */
  juce::String findSegmentAtPosition(juce::Point<float> pos) const;

  //==========================================================================
  // Drawing
  //==========================================================================

  /**
   * @brief Draw background grid (beat lines)
   */
    void drawGridSkia(SkCanvas &canvas);

  /**
   * @brief Draw automation envelope curve
   */
  void drawEnvelopeCurve(juce::Graphics &g);

  /**
   * @brief Draw control points (handles)
   */
  void drawControlPoints(juce::Graphics &g);

  /**
   * @brief Rebuild point handles cache (called from paint)
   */
  void rebuildPointHandles();

  //==========================================================================
  // Editing
  //==========================================================================

  /**
   * @brief Add automation point at screen position
   * @param pos Screen position
   */
  void addPointAt(juce::Point<float> pos);

  /**
   * @brief Delete automation point
   * @param pointId Point ID
   */
  void deletePoint(const juce::String &pointId);

  /**
   * @brief Move automation point to new screen position
   * @param pointId Point ID
   * @param newPos New screen position
   */
  void movePoint(const juce::String &pointId, juce::Point<float> newPos);

  //==========================================================================
  // Snap to Grid
  //==========================================================================

  /**
   * @brief Quantize time to grid
   * @param timeBeats Time in beats
   * @return Quantized time in beats
   */
  double quantizeToGrid(double timeBeats) const;

  //==========================================================================

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLaneComponent)
};
