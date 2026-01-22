/*
  ==============================================================================

    WavetableData.h
    Created: 2025-12-20
    Author:  Zenith DAW

    Core data structures for wavetable synthesis.
    Supports multi-frame wavetables with MIP-mapping for anti-aliased playback.

  ==============================================================================
*/

#pragma once

#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
// Constants
//==============================================================================

/** Standard wavetable size (Serum uses 2048) */
constexpr int WAVETABLE_FRAME_SIZE = 2048;

constexpr int WAVETABLE_FRAME_MASK = WAVETABLE_FRAME_SIZE - 1;

/** Maximum frames per wavetable (Serum uses 256) */
constexpr int MAX_WAVETABLE_FRAMES = 256;

/** Number of MIP levels for anti-aliasing */
constexpr int WAVETABLE_MIP_LEVELS = 10;

//==============================================================================
/**
    A single frame of wavetable data with MIP-map levels.

    MIP-mapping provides band-limited versions of the wavetable at different
    octaves to prevent aliasing at high frequencies.
*/
class WavetableFrame {
public:
  WavetableFrame() {
    // Initialize all MIP levels to silence
    for (auto &level : mipLevels_) {
      level.resize(WAVETABLE_FRAME_SIZE, 0.0f);
    }
  }

  /**
      Set the base waveform data (level 0).
      Automatically generates all MIP levels.
      @param data Pointer to WAVETABLE_FRAME_SIZE samples
  */
  void setData(const float *data) {
    std::copy(data, data + WAVETABLE_FRAME_SIZE, mipLevels_[0].begin());
    generateMipLevels();
  }

  /**
      Get a sample with linear interpolation at a given phase.
      @param phase Normalized phase [0, 1)
      @param mipLevel MIP level for anti-aliasing (0 = full, higher = filtered)
      @return Interpolated sample value
  */
  float getSample(float phase, int mipLevel) const {
    mipLevel = juce::jlimit(0, WAVETABLE_MIP_LEVELS - 1, mipLevel);
    const auto &data = mipLevels_[mipLevel];

    // Wrap phase
    phase = phase - std::floor(phase);
    if (phase < 0.0f)
      phase += 1.0f;

    // Convert to index
    float indexFloat = phase * WAVETABLE_FRAME_SIZE;
    int index0 = static_cast<int>(indexFloat) & WAVETABLE_FRAME_MASK;
    int index1 = (index0 + 1) & WAVETABLE_FRAME_MASK;
    float frac = indexFloat - std::floor(indexFloat);

    // Linear interpolation
    return data[index0] * (1.0f - frac) + data[index1] * frac;
  }

  /**
      Get a sample with cubic interpolation (higher quality).
  */
  float getSampleCubic(float phase, int mipLevel) const {
    mipLevel = juce::jlimit(0, WAVETABLE_MIP_LEVELS - 1, mipLevel);
    const auto &data = mipLevels_[mipLevel];

    phase = phase - std::floor(phase);
    if (phase < 0.0f)
      phase += 1.0f;

    float indexFloat = phase * WAVETABLE_FRAME_SIZE;
    int i1 = static_cast<int>(indexFloat) % WAVETABLE_FRAME_SIZE;
    int i0 = (i1 - 1 + WAVETABLE_FRAME_SIZE) % WAVETABLE_FRAME_SIZE;
    int i2 = (i1 + 1) % WAVETABLE_FRAME_SIZE;
    int i3 = (i1 + 2) % WAVETABLE_FRAME_SIZE;
    float t = indexFloat - std::floor(indexFloat);

    // Catmull-Rom spline
    float y0 = data[i0], y1 = data[i1], y2 = data[i2], y3 = data[i3];
    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;

    return ((a0 * t + a1) * t + a2) * t + a3;
  }

  /** Direct access to raw data for level 0 */
  const std::vector<float> &getData() const { return mipLevels_[0]; }

private:
  std::array<std::vector<float>, WAVETABLE_MIP_LEVELS> mipLevels_;

  /**
      Generate MIP levels by progressively filtering high frequencies.
      Each level removes one octave of content.
  */
  void generateMipLevels() {
    // Level 0 is already set, generate 1..N-1
    for (int level = 1; level < WAVETABLE_MIP_LEVELS; ++level) {
      auto &prev = mipLevels_[level - 1];
      auto &curr = mipLevels_[level];

      // Improved filter: 3-point window (0.25, 0.5, 0.25)
      // This provides better anti-aliasing than a simple box filter
      for (int i = 0; i < WAVETABLE_FRAME_SIZE; ++i) {
        int i_prev = (i - 1 + WAVETABLE_FRAME_SIZE) % WAVETABLE_FRAME_SIZE;
        int i_next = (i + 1) % WAVETABLE_FRAME_SIZE;
        
        curr[i] = 0.25f * prev[i_prev] + 0.5f * prev[i] + 0.25f * prev[i_next];
      }

      // Normalize to maintain peak amplitude
      float maxVal = 0.0f;
      for (float s : curr)
        maxVal = std::max(maxVal, std::abs(s));
      if (maxVal > 1e-6f) {
        float scale = 1.0f / maxVal;
        for (float &s : curr)
          s *= scale;
      }
    }
  }
};

//==============================================================================
/**
    A complete wavetable containing multiple frames.

    Supports:
    - 1-256 frames (1 = static waveform, 256 = Serum-style animated)
    - Frame interpolation for smooth morphing
    - MIP-mapped anti-aliasing
*/
class Wavetable {
public:
  Wavetable() = default;

  /**
      Create a wavetable with specified number of frames.
      @param numFrames Number of frames (1-256)
  */
  explicit Wavetable(int numFrames) {
    frames_.resize(juce::jlimit(1, MAX_WAVETABLE_FRAMES, numFrames));
  }

  /**
      Set frame data.
      @param frameIndex Frame index (0 to numFrames-1)
      @param data Pointer to WAVETABLE_FRAME_SIZE samples
  */
  void setFrameData(int frameIndex, const float *data) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames_.size())) {
      frames_[frameIndex].setData(data);
    }
  }

  /**
      Get interpolated sample across frames.
      @param phase Phase within waveform [0, 1)
      @param framePosition Normalized position within table [0, 1]
      @param mipLevel MIP level for anti-aliasing
      @return Sample with cross-frame interpolation
  */
  float getSample(float phase, float framePosition, int mipLevel) const {
    if (frames_.empty())
      return 0.0f;

    int numFrames = static_cast<int>(frames_.size());
    if (numFrames == 1) {
      return frames_[0].getSample(phase, mipLevel);
    }

    // Interpolate between frames
    framePosition = juce::jlimit(0.0f, 1.0f, framePosition);
    float frameFloat = framePosition * (numFrames - 1);
    int frame0 = static_cast<int>(frameFloat);
    int frame1 = std::min(frame0 + 1, numFrames - 1);
    float frameFrac = frameFloat - frame0;

    float s0 = frames_[frame0].getSample(phase, mipLevel);
    float s1 = frames_[frame1].getSample(phase, mipLevel);

    return s0 * (1.0f - frameFrac) + s1 * frameFrac;
  }

  /** Get number of frames in this wavetable */
  int getNumFrames() const { return static_cast<int>(frames_.size()); }

  /** Set table name (for UI display) */
  void setName(const juce::String &name) { name_ = name; }
  const juce::String &getName() const { return name_; }

  /** Check if wavetable has been initialized */
  bool isValid() const { return !frames_.empty(); }

  /** Direct frame access for loading */
  WavetableFrame &getFrame(int index) {
    jassert(index >= 0 && index < static_cast<int>(frames_.size()));
    return frames_[index];
  }

  /** Resize the wavetable to a new number of frames */
  void resize(int numFrames) {
    frames_.resize(juce::jlimit(1, MAX_WAVETABLE_FRAMES, numFrames));
  }

private:
  std::vector<WavetableFrame> frames_;
  juce::String name_{"Init"};
};

//==============================================================================
/**
    Calculate appropriate MIP level based on playback frequency.
    Higher frequencies need more filtering to prevent aliasing.

    @param frequency Playback frequency in Hz
    @param sampleRate Current sample rate
    @return MIP level (0 = full bandwidth, higher = more filtered)
*/
inline int calculateMipLevel(float frequency, double sampleRate) {
  if (frequency <= 0.0f || sampleRate <= 0.0)
    return 0;

  // The phase increment determines how many table entries we step over per output
  // sample. A larger increment means higher frequency playback, which requires
  // more anti-aliasing.
  const float phaseIncrement =
      frequency * (static_cast<float>(WAVETABLE_FRAME_SIZE) /
                   static_cast<float>(sampleRate));

  // The MIP level is the base-2 logarithm of the phase increment.
  // This effectively selects a pre-filtered table that matches the required
  // bandwidth.
  const int level = static_cast<int>(std::log2(std::max(1.0f, phaseIncrement)));

  return juce::jlimit(0, WAVETABLE_MIP_LEVELS - 1, level);
}

} // namespace zenith
