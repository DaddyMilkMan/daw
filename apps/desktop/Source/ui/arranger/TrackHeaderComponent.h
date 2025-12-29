/*
  ==============================================================================

    TrackHeaderComponent.h
    Created: 2025-12-29
    Author:  Zenith DAW

    Premium track header with glassmorphic styling, volume fader, and controls.

  ==============================================================================
*/

#pragma once

#include "../../engine/ProjectState.h"
#include "../framework/SkiaComponent.h"
#include "../controls/SkiaButton.h"
#include "../controls/ZenithSlider.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

class TrackHeaderComponent : public SkiaComponent,
                             public juce::ValueTree::Listener {
public:
  TrackHeaderComponent(ProjectState &projectState, const juce::String &trackId);
  ~TrackHeaderComponent() override;

  void resized() override;
  void drawSkia(SkCanvas *canvas) override;
  void timerCallback() override;

  // Mouse interaction for drag reordering
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  // ValueTree::Listener
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;

private:
  ProjectState &projectState_;
  juce::String trackId_;
  juce::ValueTree trackNode_;

  // UI Controls
  juce::Label nameLabel_;
  SkiaButton muteButton_;
  SkiaButton soloButton_;
  SkiaButton armButton_;
  std::unique_ptr<ZenithSlider> volumeFader_;

  // State
  juce::Colour trackColour_ = juce::Colours::grey;
  bool isMuted_ = false;
  bool isSoloed_ = false;
  bool isArmed_ = false;
  
  // Animation state
  float nameFocusAnim_ = 0.0f;
  bool nameHasFocus_ = false;
  bool isHovered_ = false;

  void onNameChanged();
  void onMuteClicked();
  void onSoloClicked();
  void onArmClicked();
  void onVolumeChanged();

  void updateFromState();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeaderComponent)
};

} // namespace zenith
