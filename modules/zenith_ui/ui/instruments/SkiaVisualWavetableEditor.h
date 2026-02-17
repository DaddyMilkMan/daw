/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include <array>

extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <core/SkImageInfo.h>
#include <core/SkSurface.h>
#pragma clang diagnostic pop
}

namespace zenith {

//==============================================================================
// Wavetable Frame Data
//==============================================================================

struct WavetableFrame {
    std::vector<float> samples;
    bool edited = false;

    WavetableFrame() = default;
    explicit WavetableFrame(int numSamples) : samples(numSamples, 0.0f) {}

    int size() const { return static_cast<int>(samples.size()); }

    void resize(int newSize) {
        samples.resize(newSize, 0.0f);
    }

    void normalize() {
        float max = 0.001f;
        for (float s : samples) {
            max = std::max(max, std::abs(s));
        }
        if (max > 0.001f) {
            for (float& s : samples) {
                s /= max;
            }
        }
    }

    void clear() {
        std::fill(samples.begin(), samples.end(), 0.0f);
    }
};

//==============================================================================
// Wavetable Data
//==============================================================================

class WavetableData {
public:
    static constexpr int maxFrames = 256;
    static constexpr int defaultFrameSize = 2048;

    WavetableData() = default;

    void resize(int numFrames, int frameSize);
    int getNumFrames() const { return static_cast<int>(frames_.size()); }
    int getFrameSize() const { return frameSize_; }

    WavetableFrame& getFrame(int index) {
        return frames_.at(juce::jlimit(0, static_cast<int>(frames_.size()) - 1, index));
    }

    const WavetableFrame& getFrame(int index) const {
        return frames_.at(juce::jlimit(0, static_cast<int>(frames_.size()) - 1, index));
    }

    void setFrames(const std::vector<WavetableFrame>& frames) {
        frames_ = frames;
        if (!frames_.empty()) {
            frameSize_ = frames_[0].size();
        }
    }

    void clear() {
        frames_.clear();
        frameSize_ = defaultFrameSize;
    }

    bool isEmpty() const { return frames_.empty(); }

    // Import/Export
    bool importWAV(const juce::File& file);
    bool importWAVStream(juce::InputStream& stream);
    bool exportWAV(const juce::File& file) const;
    bool exportWAVStream(juce::OutputStream& stream) const;

    // Processing
    void normalizeAllFrames();
    void interpolateFrames(int numInterpolatedFrames);
    void smoothAllFrames(float amount = 0.5f);

    // Analysis
    std::vector<float> analyzeSpectrum(int frameIndex) const;
    float estimateFundamental(int frameIndex) const;

private:
    std::vector<WavetableFrame> frames_;
    int frameSize_ = defaultFrameSize;
    juce::String name_;
};

//==============================================================================
// SkiaVisualWavetableEditor
//==============================================================================

class SkiaVisualWavetableEditor : public SkiaComponent {
public:
    enum class ViewMode {
        Waveform,        // 2D waveform view
        Spectrogram,     // Frequency heatmap
        3D,              // 3D perspective view
        Matrix,          // Frame matrix view
        Harmonic         // Harmonic content view
    };

    enum class EditTool {
        Select,          // Select samples/frames
        Draw,            // Freehand draw
        Line,            // Line tool
        Smooth,          // Smooth/blur
        Sharpen,         // Sharpen peaks
        Noise,           // Add noise
        Generate,        // Generate waveforms
        Morph            // Morph between frames
    };

    SkiaVisualWavetableEditor();
    ~SkiaVisualWavetableEditor() override = default;

    void drawSkia(SkCanvas* canvas) override;

    // Data management
    void setWavetableData(std::shared_ptr<WavetableData> data);
    std::shared_ptr<WavetableData> getWavetableData() const { return wavetableData_; }

    // View control
    void setViewMode(ViewMode mode) { viewMode_ = mode; markDirty(); }
    ViewMode getViewMode() const { return viewMode_; }

    void setCurrentFrame(int frame) { currentFrame_ = frame; markDirty(); }
    int getCurrentFrame() const { return currentFrame_; }

    void setZoomLevel(float zoom) { zoomLevel_ = juce::jlimit(0.1f, 10.0f, zoom); markDirty(); }
    float getZoomLevel() const { return zoomLevel_; }

    // Editing
    void setEditTool(EditTool tool) { editTool_ = tool; }
    EditTool getEditTool() const { return editTool_; }

    void setEditMode(bool enabled) { editMode_ = enabled; }
    bool isEditMode() const { return editMode_; }

    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

    // Selection
    void selectAll();
    void selectNone();
    void invertSelection();
    const std::vector<bool>& getSelection() const { return sampleSelection_; }

    // Display options
    void setShowGrid(bool show) { showGrid_ = show; markDirty(); }
    void setShowRulers(bool show) { showRulers_ = show; markDirty(); }
    void setShowCrosshair(bool show) { showCrosshair_ = show; markDirty(); }
    void setShowSpectrum(bool show) { showSpectrum_ = show; markDirty(); }

    void setGridDivisions(int divs) { gridDivisions_ = divs; markDirty(); }
    void setWaveformColor(SkColor color) { waveformColor_ = color; markDirty(); }

    // Playback
    void setPlaying(bool playing) { isPlaying_ = playing; }
    void setPlaybackPosition(float pos) { playbackPosition_ = pos; markDirty(); }

    // Callbacks
    using FrameChangedCallback = std::function<void(int)>;
    void setFrameChangedCallback(FrameChangedCallback callback) {
        frameChangedCallback_ = std::move(callback);
    }

    using DataModifiedCallback = std::function<void()>;
    void setDataModifiedCallback(DataModifiedCallback callback) {
        dataModifiedCallback_ = std::move(callback);
    }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    // Data
    std::shared_ptr<WavetableData> wavetableData_;
    int currentFrame_ = 0;
    float zoomLevel_ = 1.0f;
    float scrollOffsetX_ = 0.0f;
    float scrollOffsetY_ = 0.0f;

    // View state
    ViewMode viewMode_ = ViewMode::Waveform;
    EditTool editTool_ = EditTool::Select;
    bool editMode_ = false;
    bool isPlaying_ = false;
    float playbackPosition_ = 0.0f;

    // Display options
    bool showGrid_ = true;
    bool showRulers_ = true;
    bool showCrosshair_ = true;
    bool showSpectrum_ = false;
    int gridDivisions_ = 8;
    SkColor waveformColor_ = SkColorSetRGB(0, 255, 200);

    // Interaction
    bool isDragging_ = false;
    bool isSelecting_ = false;
    int hoveredSample_ = -1;
    int hoveredFrame_ = -1;
    int selectedSample_ = -1;
    juce::Point<float> dragStartPos_;
    juce::Point<float> selectionStart_;
    juce::Point<float> selectionEnd_;
    std::vector<bool> sampleSelection_;

    // Editing state
    std::vector<float> originalSamples_;  // For undo/redo
    juce::Point<float> lastDrawPos_;

    // Animation
    float animationPhase_ = 0.0f;
    float targetAnimationFrame_ = 0.0f;

    // History
    struct HistoryEntry {
        int frameIndex;
        std::vector<float> previousSamples;
    };
    std::vector<HistoryEntry> undoHistory_;
    std::vector<HistoryEntry> redoHistory_;
    size_t historyPosition_ = 0;

    // Callbacks
    FrameChangedCallback frameChangedCallback_;
    DataModifiedCallback dataModifiedCallback_;

    // Drawing helpers
    void drawWaveformView(SkCanvas* canvas, const SkRect& bounds);
    void drawSpectrogramView(SkCanvas* canvas, const SkRect& bounds);
    void draw3DView(SkCanvas* canvas, const SkRect& bounds);
    void drawMatrixView(SkCanvas* canvas, const SkRect& bounds);
    void drawHarmonicView(SkCanvas* canvas, const SkRect& bounds);

    void drawGrid(SkCanvas* canvas, const SkRect& bounds);
    void drawRulers(SkCanvas* canvas, const SkRect& bounds);
    void drawCrosshair(SkCanvas* canvas, const SkRect& bounds);
    void drawFrameMarkers(SkCanvas* canvas, const SkRect& bounds);
    void drawSelection(SkCanvas* canvas, const SkRect& bounds);
    void drawPlaybackIndicator(SkCanvas* canvas, const SkRect& bounds);

    // 3D rendering helpers
    void draw3DWaveform(SkCanvas* canvas, const SkRect& bounds,
                       const std::vector<std::vector<float>>& frames);

    // Editing helpers
    void applyDrawTool(float x, float y, const SkRect& bounds);
    void applyLineTool(float x, float y, const SkRect& bounds);
    void applySmoothTool(float x, float y, const SkRect& bounds);
    void applySharpenTool(float x, float y, const SkRect& bounds);
    void applyNoiseTool(float x, float y, const SkRect& bounds);
    void generateWaveform(float x, float y, const SkRect& bounds);

    // Utility
    SkPoint sampleToScreen(int sampleIndex, float value, const SkRect& bounds);
    int screenToSample(float x, const SkRect& bounds);
    float screenToValue(float y, const SkRect& bounds);
    int screenToFrame(float x, const SkRect& bounds);

    void saveState();
    void restoreState();

    // Colors
    SkColor getFrameColor(int frameIndex) const;
    SkColor getHarmonicColor(int harmonicIndex) const;
};

//==============================================================================
// WavetableGenerator
//==============================================================================

class WavetableGenerator {
public:
    // Classic waveforms
    static std::vector<float> generateSine(int size);
    static std::vector<float> generateSaw(int size, int harmonics = 32);
    static std::vector<float> generateSquare(int size, int harmonics = 32);
    static std::vector<float> generateTriangle(int size);
    static std::vector<float> generatePulse(int size, float pulseWidth);

    // Complex waveforms
    static std::vector<float> generateWavetable(int size, const std::vector<float>& amplitudes,
                                               const std::vector<float>& phases);
    static std::vector<float> generatePhaseDistortion(int size, float amount);
    static std::vector<float> generateChebyshev(int size, const std::vector<float>& coefficients);

    // Noise and random
    static std::vector<float> generateWhiteNoise(int size);
    static std::vector<float> generatePinkNoise(int size);
    static std::vector<float> generatePerlinNoise(int size, int octaves = 4);

    // Morphing
    static std::vector<float> morphFrames(const std::vector<float>& frameA,
                                         const std::vector<float>& frameB,
                                         float amount);

    // Windowing
    static void applyWindow(std::vector<float>& samples, const char* windowType = "hann");

    // Analysis
    static std::vector<float> getHarmonics(const std::vector<float>& samples, int numHarmonics);
    static float estimateFundamental(const std::vector<float>& samples);
};

//==============================================================================
// Preset Wavetables
//==============================================================================

class WavetablePresets {
public:
    static std::shared_ptr<WavetableData> createBasicWaves();
    static std::shared_ptr<WavetableData> createMorphingSaw();
    static std::shared_ptr<WavetableData> createMorphingSquare();
    static std::shared_ptr<WavetableData> formantFilter1();
    static std::shared_ptr<WavetableData> formantFilter2();
    static std::shared_ptr<WavetableData> vocalEee();
    static std::shared_ptr<WavetableData> vocalAhh();
    static std::shared_ptr<WavetableData> vocalOoo();

    static juce::StringArray getPresetNames();
    static std::shared_ptr<WavetableData> loadPreset(const juce::String& name);
};

} // namespace zenith
