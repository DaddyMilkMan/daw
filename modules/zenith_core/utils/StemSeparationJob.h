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

#include "../dsp/ONNXStemSeparator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

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
    bool usedNeuralEngine = false;
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
