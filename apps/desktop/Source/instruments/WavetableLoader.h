/*
  ==============================================================================

    WavetableLoader.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Wavetable file loader supporting:
    - WAV files (split into frames of N samples)
    - WT files (Serum-compatible format)
    - Bundled wavetables from Content directory

  ==============================================================================
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
      Generate a basic wavetable from waveform type.
      @param type Waveform type (sine, saw, square, etc.)
      @param numFrames Number of frames (1 for static, >1 for morphing)
      @return Wavetable with procedural waveforms
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
};

} // namespace zenith
