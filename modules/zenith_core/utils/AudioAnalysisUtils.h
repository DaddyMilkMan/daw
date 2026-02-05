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

    AudioAnalysisUtils.h
    Created: 2025-12-26
    Author:  Zenith DAW

    Utilities for analyzing audio content (BPM detection, Pitch detection, etc.)

  ==============================================================================

*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {

class AudioAnalysisUtils {
public:
  /**
   * @brief Analyze an audio buffer to detect its Tempo (BPM)
   * @param buffer The audio buffer to analyze
   * @param sampleRate The sample rate of the buffer
   * @return Detected BPM, or 0.0 if detection failed/uncertain
   */
  static double detectBpm(const juce::AudioBuffer<float> &buffer, double sampleRate);

private:
  // Helpers
  static std::vector<float> computeEnvelope(const juce::AudioBuffer<float>& buffer, int stepSize);
  static float computeAutocorrelation(const std::vector<float>& signal, int lag);
};

} // namespace zenith
