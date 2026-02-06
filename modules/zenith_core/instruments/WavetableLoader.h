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

#include "WavetableData.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {

//==============================================================================
/**
    Result of a wavetable loading operation
*/
struct WavetableLoadResult {
  std::unique_ptr<Wavetable> wavetable;
  juce::String errorMessage;
  bool success = false;

  static WavetableLoadResult ok(std::unique_ptr<Wavetable> wt) {
    return {std::move(wt), {}, true};
  }

  static WavetableLoadResult error(const juce::String &msg) {
    return {nullptr, msg, false};
  }
};

//==============================================================================
/**
    Loads wavetables from various file formats.

    Supported formats:
    - WAV (mono, 32-bit float or 16-bit): Split into frames of 2048 samples
    - WT (Serum format): Header + raw frame data
    - Single-cycle WAV: Resampled to 2048 samples

    Thread Safety:
    - generateBasicWavetable() is SAFE from any thread (pure function, allocates locally)
    - All file I/O methods (loadFromFile, loadWavFile, loadWtFile) are MESSAGE THREAD ONLY
    - setContentDirectory() and getBundledWavetableNames() are MESSAGE THREAD ONLY
    - WARNING: generateBasicWavetable() is NOT real-time safe (allocates memory)
*/
class WavetableLoader {
public:
  WavetableLoader();
  ~WavetableLoader() = default;

  /**
      Load a wavetable from file.
      @param file Path to .wav or .wt file
      @return Result containing wavetable or error message
  */
  WavetableLoadResult loadFromFile(const juce::File &file);

  /**
      Load a wavetable from memory buffer.
      @param data Raw audio data
      @param numSamples Total number of samples
      @param samplesPerFrame Samples per wavetable frame (default: 2048)
      @return Result containing wavetable or error message
  */
  WavetableLoadResult
  loadFromBuffer(const float *data, int numSamples,
                 int samplesPerFrame = WAVETABLE_FRAME_SIZE);

  /**
      Generate a basic wavetable from waveform type using additive synthesis.
      
      Uses trigonometric recurrence relations to optimize harmonic generation:
      - Saw: 64 harmonics with step-1 recurrence (sin((n+1)θ) = sin(nθ)cos(θ) + cos(nθ)sin(θ))
      - Square/Triangle: 32 odd harmonics with step-2 recurrence (θ' = 2θ)
      - PWM: 32 harmonics with precomputed phase-shifted coefficients
      
      @param type Waveform type (0=Sine, 1=Saw, 2=Square, 3=Triangle, 4=PWM, 5=Formant)
      @param numFrames Number of frames (1 for static, >1 for morphing wavetables)
      @return Wavetable with procedurally generated waveforms
      
      @note Maximum numerical error: <2e-6 compared to direct std::sin
      @warning Not real-time safe (allocates memory for frame buffers)
      @warning Thread-safe (no shared state), but not suitable for audio thread
  */
  std::unique_ptr<Wavetable> generateBasicWavetable(int type,
                                                    int numFrames = 1);

  /**
      Get list of bundled wavetable names.
      @return Array of wavetable names available in Content directory
  */
  juce::StringArray getBundledWavetableNames() const;

  /**
      Load a bundled wavetable by name.
      @param name Wavetable name (from getBundledWavetableNames())
      @return Result containing wavetable or error message
  */
  WavetableLoadResult loadBundledWavetable(const juce::String &name);

  /**
      Set the content directory path for bundled wavetables.
      @param path Path to Content/Wavetables directory
  */
  void setContentDirectory(const juce::File &path);

private:
  juce::AudioFormatManager formatManager_;
  juce::File contentDirectory_;

  WavetableLoadResult loadWavFile(const juce::File &file);
  WavetableLoadResult loadWtFile(const juce::File &file);

  /**
      Resample audio data to match wavetable frame size.
      Used for single-cycle waveforms that aren't exactly 2048 samples.
  */
  std::vector<float> resampleToFrameSize(const float *data, int numSamples);

  /**
      Resample audio data to match wavetable frame size.
      Writes directly to output buffer to avoid allocation.
  */
  void resampleToFrameSize(const float *data, float *dst, int numSamples);
};

} // namespace zenith
