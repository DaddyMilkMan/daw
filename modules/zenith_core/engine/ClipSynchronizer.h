/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// File: ClipSynchronizer.h
// Brief: Synchronizes clips between Engine and ProjectState

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <map>

namespace zenith {

// Forward declarations
class ProjectState;
class Engine;

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
  bool isModifyingState = false;
  std::map<int, int> engineClipCounts;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipSynchronizer)
};

} // namespace zenith
