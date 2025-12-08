/**
 * @file MixerComponent.h
 * @brief Mixer panel component for Zenith DAW
 *
 * Displays track mixer controls:
 * - Volume fader
 * - Pan knob
 * - Mute/Solo/Arm buttons
 * - Track name label
 *
 * Phase 10: Mixer MVP
 * - Basic mixer UI with track strips
 * - Integration with ProjectState
 * - Undo/redo support
 */

#pragma once

#include "../Source/ui/skia/ZenithUIComponents.h"
#include "../Source/ui/skia/SkiaComponent.h"

#include "ProjectState.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

class SkCanvas;
struct SkRect;

//==============================================================================
/**
 * @class MixerComponent
 * @brief Mixer panel with vertical track strips
 */
class MixerComponent : public zenith::SkiaComponent,
                       public juce::ValueTree::Listener {
public:
  MixerComponent(zenith::ProjectState &ps);
  ~MixerComponent() override;

  void resized() override;

  void drawSkia(SkCanvas *canvas) override;

  //==============================================================================
  struct TrackStrip {
    juce::String trackId;
    juce::String trackName;

    std::unique_ptr<zenith::ZenithSlider> volumeSlider;
    std::unique_ptr<zenith::ZenithKnob> panSlider; // Use Knob for Pan
    std::unique_ptr<zenith::ZenithButton> muteButton;
    std::unique_ptr<zenith::ZenithButton> soloButton;
    std::unique_ptr<zenith::ZenithButton> armButton;

    juce::Rectangle<int> bounds;

    TrackStrip() = default;
    ~TrackStrip() = default;

    // Delete copy and move to prevent issues with unique_ptr
    TrackStrip(const TrackStrip &) = delete;
    TrackStrip &operator=(const TrackStrip &) = delete;
    TrackStrip(TrackStrip &&) = default;
    TrackStrip &operator=(TrackStrip &&) = default;
  };

  //==========================================================================
  // ValueTree::Listener interface
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged,
                                const juce::Identifier &property) override;

  void valueTreeChildAdded(juce::ValueTree &parentTree,
                           juce::ValueTree &childWhichHasBeenAdded) override;

  void valueTreeChildRemoved(juce::ValueTree &parentTree,
                             juce::ValueTree &childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

  void
  valueTreeChildOrderChanged(juce::ValueTree &parentTreeWhoseChildrenHaveMoved,
                             int oldIndex, int newIndex) override;

  void
  valueTreeParentChanged(juce::ValueTree &treeWhoseParentHasChanged) override {}
  void valueTreeRedirected(juce::ValueTree &treeWhichHasBeenChanged) override {}

private:
  //==========================================================================
  // Helper methods
  //==========================================================================

  /**
   * @brief Rebuild all track strips from current ProjectState
   */
  void rebuildTrackStrips();

  /**
   * @brief Create a new track strip for a track
   */
  std::unique_ptr<TrackStrip>
  createTrackStrip(const juce::ValueTree &trackNode);

  /**
   * @brief Update a track strip from ProjectState
   */
  void updateTrackStripFromState(TrackStrip &strip,
                                 const juce::ValueTree &trackNode);

  /**
   * @brief Find track strip by track ID
   */
  TrackStrip *findTrackStrip(const juce::String &trackId);

  /**
   * @brief Draw a single track strip background using Skia
   * Child components (volumeSlider, panSlider, muteButton, soloButton,
   * armButton) render themselves to avoid double-rendering issues.
   */
  void drawTrackStripSkia(SkCanvas *canvas, SkRect stripBounds,
                          const TrackStrip &strip);

  //==========================================================================
  // Control callbacks
  //==========================================================================

  void onVolumeChanged(const juce::String &trackId, float value);
  void onPanChanged(const juce::String &trackId, float value);
  void onMuteClicked(const juce::String &trackId, bool state);
  void onSoloClicked(const juce::String &trackId, bool state);
  void onArmClicked(const juce::String &trackId, bool state);

  //==========================================================================
  // Member variables
  //==========================================================================

  zenith::ProjectState &projectState;

  std::vector<std::unique_ptr<TrackStrip>> trackStrips;

  // UI constants
  static constexpr int stripWidth = 80;
  static constexpr int stripSpacing = 4;
  static constexpr int topMargin = 10;
  static constexpr int bottomMargin = 10;
  static constexpr int sideMargin = 10;

  // Flag to prevent feedback loops
  bool updatingFromState = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
