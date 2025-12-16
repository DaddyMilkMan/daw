#pragma once

#include "SkiaComponent.h"
#include "ProjectState.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <map>
#include <vector>

namespace zenith {
class Engine;
}

//==============================================================================
/**
 * @class DrumPadComponent
 * @brief APC/Maschine-style Drum Pad View with integrated Step Sequencer
 *
 * Features:
 * - 4x4 Pad Grid (16 Pads)
 * - Per-pad Step Sequencer (16 steps)
 * - Visual Feedback (Hit animations)
 * - Zero-latency triggering
 */
class DrumPadComponent : public zenith::SkiaComponent,
                         private juce::ValueTree::Listener {
public:
  //==============================================================================
  struct PadData {
    int noteNumber;
    juce::String name;
    juce::Colour color;
    bool isPlaying = false;
    float flashLevel = 0.0f; // 0.0 to 1.0 for hit animation

    // Sequencer state for this pad (cached from ProjectState)
    std::vector<bool> steps; // typically 16 steps

    // UI Layout (calculated in resized)
    juce::Rectangle<float> padBounds;
    juce::Rectangle<float> sequencerBounds;
    std::vector<juce::Rectangle<float>> stepBounds;
  };

  //==============================================================================
  explicit DrumPadComponent(zenith::Engine &engine,
                            zenith::ProjectState &state);
  ~DrumPadComponent() override;

  //==============================================================================
  void setClipContext(const juce::String &clipId);

  // Helper used by MainWindow/Layout logic to determine visibility/state
  bool hasValidClip() const { return currentClipId.isNotEmpty(); }

  //==============================================================================
  void resized() override;
  void drawSkia(SkCanvas *canvas) override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  //==============================================================================
  // ValueTree::Listener overrides
  void valueTreeChildAdded(juce::ValueTree &parent,
                           juce::ValueTree &child) override;
  void valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child,
                             int index) override;
  void valueTreePropertyChanged(juce::ValueTree &tree,
                                const juce::Identifier &property) override;

  // External trigger (e.g. from MIDI input or playback)
  void triggerPad(int noteNumber, float velocity);

private:
  //==============================================================================
  zenith::Engine &engine;
  zenith::ProjectState &projectState;
  juce::String currentClipId;
  juce::ValueTree midiNotesNode;

  // Grid Configuration
  static constexpr int kRows = 4;
  static constexpr int kCols = 4;
  static constexpr int kNumPads = 16;
  static constexpr int kSequencerSteps = 16;

  // State
  std::vector<PadData> pads;
  int baseNote = 36; // C1

  // Internal methods
  void updatePadLayout();
  void refreshData();
  void hitPad(int index, float velocity);
  void toggleStep(int padIndex, int stepIndex);

  int getPadIndexAt(float x, float y) const;
  std::pair<int, int>
  getSequencerStepAt(float x, float y) const; // returns {padIndex, stepIndex}

  // Animation
  void updateAnimations();
  void timerCallback() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumPadComponent)
};
