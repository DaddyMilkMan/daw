/*
  ==============================================================================

    StemSeparationJob.h
    Created: 2025-12-09
    Author:  Zenith DAW AI Team

    Background job for splitting audio files into 4 stems using ONNX Runtime.

  ==============================================================================
*/

#pragma once

#include "../dsp/ONNXStemSeparator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

namespace zenith {
namespace utils {

class StemSeparationJob : public juce::ThreadPoolJob {
public:
  struct StemFiles {
    juce::File vocals;
    juce::File drums;
    juce::File bass;
    juce::File other;
    bool success = false;
    juce::String error;
  };

  using CompletionCallback = std::function<void(const StemFiles &)>;

  StemSeparationJob(const juce::File &inputFile,
                    const juce::File &outputDirectory,
                    CompletionCallback callback);

  ~StemSeparationJob() override;

  juce::ThreadPoolJob::JobStatus runJob() override;

private:
  juce::File inputFile_;
  juce::File outputDirectory_;
  CompletionCallback callback_;

  // Helper to write buffer to file
  juce::File writeStemToFile(const juce::AudioBuffer<float> &buffer,
                             const juce::String &stemName, double sampleRate);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemSeparationJob)
};

} // namespace utils
} // namespace zenith
