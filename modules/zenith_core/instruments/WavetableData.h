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

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <string>
#include <vector>

namespace zenith {

//==============================================================================
// WAVETABLE FORMAT ENUMERATION
//==============================================================================
/**
 * Detected wavetable file format
 */
enum class WavetableFormat {
    Unknown,    ///< Unable to detect format
    RIFF,       ///< Standard RIFF/WAVE format
    Serum,      ///< Xfer Serum single-cycle wavetable
    Vital,      ///< Vital wavetable format
    RAW         ///< Raw sample data without header
};

//==============================================================================
// WAVETABLE FRAME DATA
//==============================================================================
/**
 * Single wavetable frame containing one cycle of a waveform
 * Uses fixed size for RT-safe access (2048 samples = Serum standard)
 */
struct WavetableFrame {
    static constexpr int SAMPLES_PER_FRAME = 2048;
    std::array<float, SAMPLES_PER_FRAME> samples;

    WavetableFrame() { samples.fill(0.0f); }

    /** Get sample at normalized position (0.0 - 1.0) with linear interpolation */
    float getSampleAt(float position) const {
        float pos = juce::jlimit(0.0f, 1.0f, position);
        float exactSample = pos * static_cast<float>(SAMPLES_PER_FRAME - 1);
        int index1 = static_cast<int>(exactSample);
        int index2 = juce::jmin(index1 + 1, SAMPLES_PER_FRAME - 1);
        float frac = exactSample - static_cast<float>(index1);
        return samples[index1] + frac * (samples[index2] - samples[index1]);
    }

    /** Get raw sample at index */
    inline float& operator[](size_t index) { return samples[index]; }
    inline const float& operator[](size_t index) const { return samples[index]; }
};

//==============================================================================
// WAVETABLE DATA STRUCTURE
//==============================================================================
/**
 * Complete wavetable with multiple frames for morphing
 * This is the main data structure for loaded wavetable files
 */
class WavetableData {
public:
    WavetableData() = default;

    //==========================================================================
    // Frame Management
    //==========================================================================

    /** Add a frame to the wavetable */
    void addFrame(const WavetableFrame& frame) {
        frames_.push_back(frame);
    }

    /** Get number of frames */
    int getNumFrames() const { return static_cast<int>(frames_.size()); }

    /** Get frame at index */
    const WavetableFrame& getFrame(int index) const {
        return frames_.at(juce::jlimit(0, static_cast<int>(frames_.size()) - 1, index));
    }

    /** Get interpolated sample across frames (for wavetable morphing) */
    float getSample(float phase, float framePosition) const {
        if (frames_.empty()) return 0.0f;

        float pos = juce::jlimit(0.0f, 1.0f, framePosition);
        float exactFrame = pos * static_cast<float>(frames_.size() - 1);
        int frame1 = static_cast<int>(exactFrame);
        int frame2 = juce::jmin(frame1 + 1, static_cast<int>(frames_.size()) - 1);
        float frameFrac = exactFrame - static_cast<float>(frame1);

        float sample1 = frames_[frame1].getSampleAt(phase);
        float sample2 = frames_[frame2].getSampleAt(phase);

        return sample1 + frameFrac * (sample2 - sample1);
    }

    //==========================================================================
    // Metadata
    //==========================================================================

    void setName(const juce::String& name) { name_ = name; }
    juce::String getName() const { return name_; }

    void setAuthor(const juce::String& author) { author_ = author; }
    juce::String getAuthor() const { return author_; }

    void setNumFrames(int num) { numFrames_ = num; }
    int getNumFramesValue() const { return numFrames_; }

    void setBitDepth(int depth) { bitDepth_ = depth; }
    int getBitDepth() const { return bitDepth_; }

    /** Check if wavetable is valid (has frames) */
    bool isValid() const { return !frames_.empty(); }

    //==========================================================================
    // MIP Mapping (anti-aliasing support)
    //==========================================================================

    /** Calculate optimal MIP level for given frequency */
    int calculateMipLevel(float frequency, float sampleRate) const {
        if (frames_.empty()) return 0;

        // Base MIP level based on Nyquist frequency
        float baseMip = std::log2(frequency / (sampleRate * 0.5f));
        int mipLevel = static_cast<int>(std::floor(-baseMip));

        // Clamp to available MIP levels (could generate these offline)
        return juce::jlimit(0, MAX_MIP_LEVELS - 1, mipLevel);
    }

    static constexpr int MAX_MIP_LEVELS = 6;

private:
    std::vector<WavetableFrame> frames_;
    juce::String name_;
    juce::String author_;
    int numFrames_ = 1;
    int bitDepth_ = 16;
};

//==============================================================================
// WAVETABLE CLASS (for runtime use)
//==============================================================================
/**
 * Runtime wavetable reference for oscillators
 * Non-owning pointer to WavetableData with position state
 */
class Wavetable {
public:
    Wavetable() = default;
    explicit Wavetable(const WavetableData* data) : data_(data) {}

    bool isValid() const { return data_ != nullptr && data_->isValid(); }

    /** Get sample at phase with current frame position */
    float getSample(float phase, float framePos = 0.0f) const {
        if (data_) {
            return data_->getSample(phase, framePos);
        }
        return 0.0f;
    }

    const WavetableData* getData() const { return data_; }

private:
    const WavetableData* data_ = nullptr;
};

} // namespace zenith
