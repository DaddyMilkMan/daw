/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux
    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>
    SPDX-License-Identifier: Apache-2.0 
*/
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <memory>
namespace zenith {
//==============================================================================
// CUSTOM LFO SHAPES
//==============================================================================
/**
 * @class CustomLFO
 * @brief Professional LFO with custom drawable shapes and morphing
 *
 * Features:
 * - User-drawable waveform shapes using spline interpolation
 * - Shape morphing between multiple custom shapes
 * - Smooth phase-continuous transitions
 * - Sync to host tempo with various subdivisions
 * - Unipolar/bipolar output modes
 * - One-shot, trigger, and loop modes
 * - Shape presets (S-curve, exponential, bounce, etc.)
 */
class CustomLFO {
public:
    enum class Mode {
        Loop,           // Continuous looping
        OneShot,        // Play once and hold
        Trigger,        // Reset phase on trigger
        PingPong,       // Forward then reverse
        Random          // Random selection of shapes
    };
    enum class SyncMode {
        Free,           // Free running (Hz based)
        Sync1,          // 1/1 notes
        Sync2,          // 1/2 notes
        Sync4,          // 1/4 notes
        Sync4T,         // 1/4 triplets
        Sync8,          // 1/8 notes
        Sync8T,         // 1/8 triplets
        Sync16,         // 1/16 notes
        Sync16T,        // 1/16 triplets
        Sync32          // 1/32 notes
    };
    struct ShapePoint {
        float x;        // Position 0-1
        float y;        // Amplitude -1 to 1
        float curve;    // Curve tension (negative = ease-in, positive = ease-out)
        ShapePoint() : x(0.0f), y(0.0f), curve(0.0f) {}
        ShapePoint(float x_, float y_, float curve_ = 0.0f) : x(x_), y(y_), curve(curve_) {}
    };
    CustomLFO();
    ~CustomLFO() = default;
    void prepare(double sampleRate);
    void reset();
    void process(float& output, int numSamples = 1);
    // Shape management
    void setShape(const std::vector<ShapePoint>& points);
    void addPoint(const ShapePoint& point);
    void removePoint(int index);
    void clearShape();
    const std::vector<ShapePoint>& getShape() const { return shape_; }
    // Shape presets
    void setPresetShape(const juce::String& name);
    static std::vector<ShapePoint> createPresetShape(const juce::String& name);
    // Morphing between shapes
    void setMorphShape(const std::vector<ShapePoint>& shape);
    void setMorphPosition(float position); // 0-1 between shapes
    void setMorphEnabled(bool enabled) { morphEnabled_ = enabled; }
    // Parameters
    void setRate(float rateHz) { rate_ = juce::jlimit(0.01f, 100.0f, rateHz); }
    void setDepth(float depth) { depth_ = juce::jlimit(0.0f, 1.0f, depth); }
    void setPhase(float phase) { phase_ = juce::jlimit(0.0f, 1.0f, phase); }
    void setMode(Mode mode) { mode_ = mode; }
    void setSyncMode(SyncMode mode) { syncMode_ = mode; }
    void setBpm(float bpm) { bpm_ = bpm; }
    void setUnipolar(bool unipolar) { unipolar_ = unipolar; }
    // Trigger
    void trigger() { shouldTrigger_ = true; }
    void resetPhase() { phase_ = 0.0f; direction_ = 1; }
    // Output
    float getCurrentOutput() const { return currentOutput_; }
    float getNormalizedPhase() const { return phase_; }
private:
    float processSingleSample();
    float evaluateShape(float position, const std::vector<ShapePoint>& points) const;
    float hermiteInterpolate(float x, float y0, float y1, float y2, float y3, float tension) const;
    std::vector<ShapePoint> shape_;
    std::vector<ShapePoint> morphShape_;
    std::array<float, 256> lookupTable_;
    bool tableDirty_ = true;
    float morphPosition_ = 0.0f;
    bool morphEnabled_ = false;
    // Parameters
    float rate_ = 1.0f;
    float depth_ = 1.0f;
    float phase_ = 0.0f;
    float bpm_ = 120.0f;
    bool unipolar_ = false;
    Mode mode_ = Mode::Loop;
    SyncMode syncMode_ = SyncMode::Free;
    bool shouldTrigger_ = false;
    // State
    int direction_ = 1;  // 1 = forward, -1 = reverse
    float currentOutput_ = 0.0f;
    double sampleRate_ = 44100.0;
    bool wasTriggered_ = false;
    void rebuildLookupTable();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLFO)
};
//==============================================================================
// 3D WAVETABLE MORPHING
//==============================================================================
/**
 * @class Wavetable3D
 * @brief 3D wavetable morphing grid (Serum-style)
 *
 * Features:
 * - 8-slot wavetable grid in 3D cube arrangement
 * - Smooth interpolation between all 8 wavetables
 * - X/Y/Z position control with morphing
 * - Visual grid representation
 * - Morph trajectory recording
 * - Per-position parameter automation
 */
class Wavetable3D {
public:
    static constexpr int GRID_SIZE = 2;  // 2x2x2 = 8 wavetables
    static constexpr int TOTAL_WAVETABLES = GRID_SIZE * GRID_SIZE * GRID_SIZE;
    static constexpr int FRAME_SIZE = 2048;
    struct Position3D {
