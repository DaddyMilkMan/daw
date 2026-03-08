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
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>

namespace zenith {

//==============================================================================
// WAVETABLE EDITOR
//==============================================================================
/**
 * Professional wavetable editor for creating and modifying wavetables
 *
 * FEATURES:
 * - Single-cycle waveform editor with 2048 samples
 * - Multi-frame wavetable creation (up to 256 frames)
 * - Morphing between frames
 * - FFT-based processing
 * - Import from audio file
 * - Export to .wav (Serum-compatible)
 * - Basic waveforms (saw, square, triangle, sine)
 * - Waveform processing (normalize, phase align, smooth, harmonics)
 */
class ZenithWavetableEditor {
public:
    static constexpr int SINGLE_CYCLE_SIZE = 2048;
    static constexpr int MAX_FRAMES = 256;

    ZenithWavetableEditor();

    //==========================================================================
    // Frame Management
    //==========================================================================

    /** Create new empty wavetable */
    void clear();

    /** Set number of frames */
    void setNumFrames(int numFrames);

    /** Get current frame index */
    int getCurrentFrame() const { return currentFrame_; }
    void setCurrentFrame(int frame);

    /** Get total frames */
    int getNumFrames() const { return numFrames_; }

    //==========================================================================
    // Waveform Generation
    //==========================================================================

    /** Generate basic waveform */
    enum class BasicWaveform {
        Sine,
        Square,
        Saw,
        Triangle,
        Noise
    };
    void generateWaveform(BasicWaveform type);

    /** Generate from harmonics (additive synthesis) */
    void generateFromHarmonics(const float* amplitudes, int numHarmonics);
    void generateFromHarmonics(const std::function<float(int)>& amplitudeFunc);

    //==========================================================================
    // Waveform Processing
    //==========================================================================

    /** Normalize waveform */
    void normalize(bool peak = true);

    /** Phase alignment */
    void alignPhase();

    /** Smooth waveform (moving average) */
    void smooth(int strength);

    /** Remove DC offset */
    void removeDC();

    /** Apply fade in/out at edges for seamless looping */
    void applyCrossfade(int samples);

    //==========================================================================
    // Import/Export
    //==========================================================================

    /** Import from audio file */
    juce::Result importFromFile(const juce::File& file);

    /** Import from buffer (resamples to 2048 samples) */
    void importFromBuffer(const float* data, int numSamples);

    /** Export current frame as .wav */
    juce::Result exportCurrentFrame(const juce::File& file) const;

    /** Export full wavetable as .wav (Serum-compatible) */
    juce::Result exportWavetable(const juce::File& file) const;

    /** Export to WavetableData structure */
    WavetableData exportWavetableData() const;

    //==========================================================================
    // Frame Editing
    //==========================================================================

    /** Get frame data for editing */
    const float* getFrameData(int frame) const;
    float* getFrameData(int frame);

    /** Get current frame data */
    const float* getCurrentFrameData() const;
    float* getCurrentFrameData();

    /** Set frame data */
    void setFrameData(int frame, const float* data);

    //==========================================================================
    // Morphing
    //==========================================================================

    /** Morph between two frames */
    static void morphFrames(const float* frameA, const float* frameB,
                          float* output, float position);

    /** Interpolate between frames for smooth morphing */
    float getInterpolatedSample(float phase, float framePosition) const;

    //==========================================================================
    // Analysis
    //==========================================================================

    /** Get waveform statistics */
    struct WaveformStats {
        float min = 0.0f;
        float max = 0.0f;
        float rms = 0.0f;
        float peakToPeak = 0.0f;
        float dcOffset = 0.0f;
        float zeroCrossings = 0.0f;
    };
    WaveformStats analyzeFrame(int frame) const;

    /** Get harmonic content (FFT) */
    void getHarmonics(int frame, float* amplitudes, int numHarmonics) const;

    //==========================================================================
    // Undo/Redo
    //==========================================================================

    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

    //==========================================================================
    // Callbacks
    //==========================================================================

    using ChangeCallback = std::function<void()>;
    void setChangeCallback(ChangeCallback callback) { changeCallback_ = callback; }

private:
    //==========================================================================
    // Data Storage
    //==========================================================================

    std::vector<std::array<float, SINGLE_CYCLE_SIZE>> frames_;
    int numFrames_ = 1;
    int currentFrame_ = 0;

    //==========================================================================
    // Undo/Redo
    //==========================================================================

    static constexpr int MAX_UNDO = 32;
    std::vector<std::array<float, SINGLE_CYCLE_SIZE>> undoStack_;
    std::vector<std::array<float, SINGLE_CYCLE_SIZE>> redoStack_;
    int undoIndex_ = -1;

    //==========================================================================
    // Callbacks
    //==========================================================================

    ChangeCallback changeCallback_;

    //==========================================================================
    // Internal Helpers
    //==========================================================================

    void saveUndoState();
    void notifyChanged();
    float lerp(float a, float b, float t) const;
};

//==============================================================================
// WAVETABLE PRESETS
//==============================================================================

/**
 * Factory wavetable presets
 */
class ZenithWavetablePresets {
public:
    /** Get all preset names */
    static juce::StringArray getPresetNames();

    /** Load preset into editor */
    static void loadPreset(const juce::String& name, ZenithWavetableEditor& editor);

    /** Create factory wavetable data */
    static WavetableData createPreset(const juce::String& name);

private:
    /** Serum-compatible wavetable factory */
    static void createSaw(WavetableData& wt);
    static void createSquare(WavetableData& wt);
    static void createTriangle(WavetableData& wt);
    static void createSine(WavetableData& wt);
    static void create supersaw(WavetableData& wt);
    static void create supersquare(WavetableData& wt);
    static void createMorbid(WavetableData& wt);  // Serum-style
    static void createVocal(WavetableData& wt);     // Formant
    static void createBells(WavetableData& wt);
};

} // namespace zenith
