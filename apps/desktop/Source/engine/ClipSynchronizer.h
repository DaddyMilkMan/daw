/**
 * @file ClipSynchronizer.h
 * @brief Synchronizes clips between Engine and ProjectState
 */

#pragma once

#include "Engine.h"
#include "ProjectState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <map>

namespace zenith {

class ClipSynchronizer : public juce::Timer, public juce::ValueTree::Listener {
public:
  ClipSynchronizer(ProjectState &projectState, Engine &engine);
  ~ClipSynchronizer() override;

  void start(int updateRateHz = 30);
  void stop();
  bool isRunning() const { return isTimerRunning(); }

  // ValueTree::Listener overrides
  void valueTreeChildAdded(juce::ValueTree &parentTree,
                           juce::ValueTree &childWhichHasBeenAdded) override;
  void valueTreeChildRemoved(juce::ValueTree &parentTree,
                             juce::ValueTree &childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;
  void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged,
                                const juce::Identifier &property) override;

  juce::String createClip(const juce::String &trackId, double startBeats,
                          double lengthBeats, const juce::String &clipType);

private:
  void timerCallback() override;
  void syncEngineToProjectState();
  int64_t beatsToSamples(double beats, double tempo, double sampleRate) const;
  double samplesToBeats(int64_t samples, double tempo, double sampleRate) const;

  ProjectState &projectState;
  Engine &engine;
  std::atomic<bool> isModifyingState{false};
  std::map<int, int> engineClipCounts;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSynchronizer)
};

} // namespace zenith
