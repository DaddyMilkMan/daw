/*
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
