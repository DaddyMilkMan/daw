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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    Metronome.h
    Created: 2025-12-18
    Author:  Zenith DAW

    Synthesized metronome for click track generation.

  ==============================================================================

*/

#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>


namespace zenith {

class TempoMap;

class Metronome {
public:
  Metronome();
  ~Metronome() = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock);
  void releaseResources();

  /**
   * @brief Generates the click track audio.
   * @param bufferToFill The buffer to mix the click into.
   * @param currentTransportSample The current absolute sample position of the
   * transport.
   * @param isPlaying Whether the transport is currently playing.
   * @param tempoMap The TempoMap to calculate beat positions.
   */
  void getNextAudioBlock(juce::AudioBuffer<float> &bufferToFill,
                         int64_t currentTransportSample, bool isPlaying,
                         const TempoMap &tempoMap);

  void setEnabled(bool shouldBeEnabled);
  bool isEnabled() const;

  void setLevel(float newLevel);
  float getLevel() const;

  void setCountInBars(int bars);
  int getCountInBars() const;

private:
  double sampleRate_ = 44100.0;
  std::atomic<bool> enabled_{false};
  std::atomic<float> level_{0.5f}; // -6dB default
  std::atomic<int> countInBars_{0};

  // Synthesis state
  int currentNoteSamplesRemaining_ = 0;
  float currentFrequency_ = 0.0f;
  float currentPhase_ = 0.0f;
  float phaseIncrement_ = 0.0f;

  // Constants
  static constexpr float kHighClickFreq = 1600.0f;
  static constexpr float kLowClickFreq = 800.0f;
  static constexpr float kClickDurationSec = 0.1f;
  static constexpr float kReleaseTimeSec = 0.05f; // Short decay

  double lastBeat_ = -1.0;

  // Helper to trigger a click
  void triggerClick(float frequency);
};

} // namespace zenith
