/**
 * @file TrackHeaderComponent.h
 * @brief Track header UI component with name, color, and M/S/R controls
 *
 * Displays and controls track properties:
 * - Color stripe (left edge)
 * - Track name (editable label)
 * - Mute button (M)
 * - Solo button (S)
 * - Record arm button (R)
 *
 * Observes ProjectState via ValueTree listener and updates Engine via
 * ProjectState APIs.
 */

#pragma once

#include "../framework/SkiaComponent.h"
#include "../widgets/SkiaButton.h"
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


//==============================================================================
namespace zenith {

/**
 * @class TrackHeaderComponent
 * @brief UI component for a single track header
 *
 * Layout (~160px wide):
 * ┌─┬──────────────────────┬───┬───┬───┐
 * │C│ Track Name           │ M │ S │ R │
 * └─┴──────────────────────┴───┴───┴───┘
 *
 * C = Color stripe (8px)
 * Track Name = Editable label
 * M = Mute button
 * S = Solo button
 * R = Record arm button
 */
class TrackHeaderComponent : public zenith::SkiaComponent,
                             private juce::ValueTree::Listener {
public:
  //==========================================================================
  /**
   * @brief Constructor
   * @param projectState Reference to project state
   * @param trackId Track ID in ProjectState
   */
  TrackHeaderComponent(ProjectState &projectState, const juce::String &trackId);

  /**
   * @brief Destructor
   */
  ~TrackHeaderComponent() override;

  //==========================================================================
  // Component interface
  //==========================================================================

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void timerCallback() override;

  //==========================================================================
  /**
   * @brief Get track ID
   */
  const juce::String &getTrackId() const { return trackId_; }

private:
  //==========================================================================
  // ValueTree::Listener (MESSAGE THREAD)
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override {}
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) [[maybe_unused]] override {}
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) [[maybe_unused]] override {}
  void valueTreeParentChanged(juce::ValueTree &tree) override {}

  //==========================================================================
  // UI Callbacks
  //==========================================================================

  void onNameChanged();
  void onMuteClicked();
  void onSoloClicked();
  void onArmClicked();

  //==========================================================================
  // Helper Methods
  //==========================================================================

  void updateFromState();

  //==========================================================================
  // Member Variables
  //==========================================================================

  ProjectState &projectState_;
  juce::String trackId_;
  juce::ValueTree trackNode_;

  // UI Components
  juce::Label nameLabel_;
  zenith::SkiaButton muteButton_;
  zenith::SkiaButton soloButton_;
  zenith::SkiaButton armButton_;

  // Current state (cached from ValueTree)
  juce::Colour trackColour_{juce::Colours::grey};
  bool isMuted_ = false;
  bool isSoloed_ = false;
  bool isArmed_ = false;

  // Animation state
  float nameFocusAnim_ = 0.0f;
  bool nameHasFocus_ = false;
  bool isHovered_ = false;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeaderComponent)
};

} // namespace zenith
