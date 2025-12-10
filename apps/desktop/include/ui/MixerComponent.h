/**
 * @file MixerComponent.h
 * @brief Mixer panel component for Zenith DAW
 *
 * Displays track mixer controls using the Zenith Design System.
 * - Volume fader
 * - Pan knob
 * - Mute/Solo/Arm buttons
 * - Track name label
 *
 * This component acts as a container for MixerChannelComponents.
 */

#pragma once

#include "../Engine.h"
#include "../Source/ui/skia/SkiaComponent.h"
#include "../Source/ui/skia/ZenithDesignSystem.h" // Import ZenithDesignSystem
#include "MixerChannelComponent.h"
#include "ProjectState.h"


#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <memory>
#include <vector>


namespace zenith {

class MixerComponent : public SkiaComponent, public juce::ValueTree::Listener {
public:
  //==========================================================================
  /**
   * @brief Constructor
   * @param engine Reference to the audio engine (for Track access)
   * @param state Reference to project state (for listeners/order)
   */
  MixerComponent(Engine &engine, ProjectState &state);
  ~MixerComponent() override;

  //==========================================================================
  // Component interface
  //==========================================================================

  void paint(juce::Graphics &g) override; // JUCE fallback
  void resized() override;
  void drawSkia(SkCanvas *canvas) override; // Skia rendering

  //==========================================================================
  // ValueTree::Listener interface
  //==========================================================================

  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreeChildOrderChanged(juce::ValueTree &parent, int oldIndex,
                                  int newIndex) override;
  void valueTreeParentChanged(juce::ValueTree &tree) override;

private:
  //==========================================================================
  // Internal methods
  //==========================================================================

  void rebuildChannels();
  Track *findTrackById(const juce::String &trackId);

  //==========================================================================
  // Member variables
  //==========================================================================

  Engine &engine_;
  ProjectState &projectState_;

  // List of channel strips
  std::vector<std::unique_ptr<MixerChannelComponent>> channels_;

  // Layout constants
  static constexpr int stripWidth = 100;
  static constexpr int stripSpacing = 4;
  static constexpr int topMargin = 0;
  static constexpr int bottomMargin = 0;
  static constexpr int sideMargin = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};

} // namespace zenith
