/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <vector>
#include <array>

extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <effects/SkGradientShader.h>
#pragma clang diagnostic pop
}

namespace zenith {

//==============================================================================
// Synth UI Parameter
//==============================================================================

struct SynthParameter {
    juce::String id;
    juce::String name;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.5f;
    float currentValue = 0.5f;

    // Display
    juce::String unit;
    int decimalPlaces = 2;

    // MIDI learn
    int midiCC = -1;
    bool midiLearnActive = false;

    // Modulation
    std::vector<float> modulationAmounts;  // Per-modulator amounts
};

//==============================================================================
// Skia Rotary Knob
//==============================================================================

class SkiaRotaryKnob : public SkiaComponent {
public:
    SkiaRotaryKnob();
    ~SkiaRotaryKnob() override = default;

    void drawSkia(SkCanvas* canvas) override;
    std::vector<AIElementInfo> getInspectableElements() override;

    void setParameter(SynthParameter* param) { parameter_ = param; }
    SynthParameter* getParameter() const { return parameter_; }

    void setValue(float value);
    float getValue() const { return value_; }

    void setRange(float min, float max);
    void setDefaultValue(float def) { defaultValue_ = def; }

    void setText(const juce::String& text) { text_ = text; }
    juce::String getText() const { return text_; }

    void setShowValue(bool show) { showValue_ = show; }
    bool isShowingValue() const { return showValue_; }

    // Styling
    void setTrackWidth(float width) { trackWidth_ = width; }
    void setIndicatorSize(float size) { indicatorSize_ = size; }
    void setArcAngle(float startAngle, float endAngle) {
        arcStartAngle_ = startAngle;
        arcEndAngle_ = endAngle;
    }

    // Callbacks
    using ValueChangedCallback = std::function<void(float)>;
    void setValueChangedCallback(ValueChangedCallback callback) {
        valueChangedCallback_ = std::move(callback);
    }

    using DragStartedCallback = std::function<void()>;
    void setDragStartedCallback(DragStartedCallback callback) {
        dragStartedCallback_ = std::move(callback);
    }

    using DragEndedCallback = std::function<void()>;
    void setDragEndedCallback(DragEndedCallback callback) {
        dragEndedCallback_ = std::move(callback);
    }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void onMouseDown(const juce::MouseEvent& e) override;
    void onMouseDrag(const juce::MouseEvent& e) override;
    void onMouseUp(const juce::MouseEvent& e) override;

private:
    float value_ = 0.5f;
    float defaultValue_ = 0.5f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    juce::String text_;
    bool showValue_ = true;

    // Styling
    float trackWidth_ = 4.0f;
    float indicatorSize_ = 12.0f;
    float arcStartAngle_ = 0.7f;  // Radians
    float arcEndAngle_ = 2.0f * juce::MathConstants<float>::pi - 0.7f;

    // Drag state
    bool isDragging_ = false;
    float lastDragY_ = 0.0f;
    float dragStartValue_ = 0.0f;

    // Animation
    float currentAngle_ = 0.0f;
    float targetAngle_ = 0.0f;

    SynthParameter* parameter_ = nullptr;
    ValueChangedCallback valueChangedCallback_;
    DragStartedCallback dragStartedCallback_;
    DragEndedCallback dragEndedCallback_;

    void updateAngle();
    void drawTrack(SkCanvas* canvas, const SkRect& bounds);
    void drawFill(SkCanvas* canvas, const SkRect& bounds, float fillAmount);
    void drawIndicator(SkCanvas* canvas, const SkRect& bounds, float angle);
    void drawValueText(SkCanvas* canvas, const SkRect& bounds);
    void drawLabel(SkCanvas* canvas, const SkRect& bounds);
    SkColor getColorForValue(float value) const;
};

//==============================================================================
// Skia Linear Fader
//==============================================================================

class SkiaLinearFader : public SkiaComponent {
public:
    enum class Orientation {
        Horizontal,
        Vertical
    };

    SkiaLinearFader();
    ~SkiaLinearFader() override = default;

    void drawSkia(SkCanvas* canvas) override;

    void setParameter(SynthParameter* param) { parameter_ = param; }
    SynthParameter* getParameter() const { return parameter_; }

    void setValue(float value);
    float getValue() const { return value_; }

    void setRange(float min, float max);
    void setDefaultValue(float def) { defaultValue_ = def; }

    void setOrientation(Orientation orientation) { orientation_ = orientation; }
    Orientation getOrientation() const { return orientation_; }

    void setShowValue(bool show) { showValue_ = show; }
    void setReverse(bool reverse) { reverse_ = reverse; }

    // Styling
    void setTrackThickness(float thickness) { trackThickness_ = thickness; }
    void setThumbSize(float size) { thumbSize_ = size; }

    // Callbacks
    using ValueChangedCallback = std::function<void(float)>;
    void setValueChangedCallback(ValueChangedCallback callback) {
        valueChangedCallback_ = std::move(callback);
    }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    float value_ = 0.5f;
    float defaultValue_ = 0.5f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    bool showValue_ = true;
    bool reverse_ = false;
    Orientation orientation_ = Orientation::Vertical;

    // Styling
    float trackThickness_ = 4.0f;
    float thumbSize_ = 16.0f;

    // Drag state
    bool isDragging_ = false;
    float dragStartPos_ = 0.0f;
    float dragStartValue_ = 0.0f;

    SynthParameter* parameter_ = nullptr;
    ValueChangedCallback valueChangedCallback_;

    void drawTrack(SkCanvas* canvas, const SkRect& bounds);
    void drawFill(SkCanvas* canvas, const SkRect& bounds);
    void drawThumb(SkCanvas* canvas, const SkRect& bounds, float thumbPos);
    void drawValueText(SkCanvas* canvas, const SkRect& bounds);
    void drawTickMarks(SkCanvas* canvas, const SkRect& bounds);
    float getThumbPosition() const;
};

//==============================================================================
// Skia Waveform Display
//==============================================================================

class SkiaWaveformDisplay : public SkiaComponent {
public:
    enum class WaveformType {
        Sine,
        Saw,
        Square,
        Triangle,
        Noise,
        Wavetable
    };

    SkiaWaveformDisplay();
    ~SkiaWaveformDisplay() override = default;

    void drawSkia(SkCanvas* canvas) override;

    void setWaveformType(WaveformType type) { waveformType_ = type; }
    WaveformType getWaveformType() const { return waveformType_; }

    void setPhase(float phase) { phase_ = phase; markDirty(); }
    void setPulseWidth(float pw) { pulseWidth_ = pw; markDirty(); }

    // For wavetable display
    void setWavetableData(const float* samples, int numSamples);

    // Animation
    void setAnimate(bool animate) { animate_ = animate; }
    void setAnimationSpeed(float speed) { animationSpeed_ = speed; }

    // Styling
    void setLineWidth(float width) { lineWidth_ = width; }
    void setGlowIntensity(float intensity) { glowIntensity_ = intensity; }
    void setGradientStart(SkColor color) { gradientStart_ = color; }
    void setGradientEnd(SkColor color) { gradientEnd_ = color; }

protected:
    void timerCallback() override;

private:
    WaveformType waveformType_ = WaveformType::Sine;
    float phase_ = 0.0f;
    float pulseWidth_ = 0.5f;
    bool animate_ = true;
    float animationSpeed_ = 1.0f;
    float animationPhase_ = 0.0f;

    // Styling
    float lineWidth_ = 2.0f;
    float glowIntensity_ = 0.5f;
    SkColor gradientStart_ = SkColorSetRGB(0, 255, 255);
    SkColor gradientEnd_ = SkColorSetRGB(255, 0, 255);

    // Wavetable data
    std::vector<float> wavetableSamples_;

    void drawSineWave(SkCanvas* canvas, const SkRect& bounds);
    void drawSawWave(SkCanvas* canvas, const SkRect& bounds);
    void drawSquareWave(SkCanvas* canvas, const SkRect& bounds);
    void drawTriangleWave(SkCanvas* canvas, const SkRect& bounds);
    void drawWavetable(SkCanvas* canvas, const SkRect& bounds);
    void drawWave(SkCanvas* canvas, const SkRect& bounds,
                  std::function<float(float)> waveFunc);
};

//==============================================================================
// Skia Envelope Editor
//==============================================================================

class SkiaEnvelopeEditor : public SkiaComponent {
public:
    struct EnvelopePoint {
        float x;           // 0-1 (time)
        float y;           // 0-1 (level)
        bool curved = false;
        float curveAmount = 0.5f;  // -1 to +1
        bool fixed = false;  // Cannot be moved

        EnvelopePoint() : x(0), y(0) {}
        EnvelopePoint(float x_, float y_) : x(x_), y(y_) {}
    };

    enum class EnvelopeStage {
        Attack,
        Decay,
        Sustain,
        Release
    };

    SkiaEnvelopeEditor();
    ~SkiaEnvelopeEditor() override = default;

    void drawSkia(SkCanvas* canvas) override;

    void setAttackTime(float seconds);
    void setDecayTime(float seconds);
    void setSustainLevel(float level);
    void setReleaseTime(float seconds);

    float getAttackTime() const { return attackTime_; }
    float getDecayTime() const { return decayTime_; }
    float getSustainLevel() const { return sustainLevel_; }
    float getReleaseTime() const { return releaseTime_; }

    void setTimeScale(float maxSeconds) { maxTimeSeconds_ = maxSeconds; }

    // Styling
    void setNodeSize(float size) { nodeSize_ = size; }
    void setLineWidth(float width) { lineWidth_ = width; }
    void setShowGrid(bool show) { showGrid_ = show; }
    void setShowValues(bool show) { showValues_ = show; }

    // Callbacks
    using EnvelopeChangedCallback = std::function<void()>;
    void setEnvelopeChangedCallback(EnvelopeChangedCallback callback) {
        envelopeChangedCallback_ = std::move(callback);
    }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    float attackTime_ = 0.01f;
    float decayTime_ = 0.3f;
    float sustainLevel_ = 0.7f;
    float releaseTime_ = 0.5f;
    float maxTimeSeconds_ = 5.0f;

    // Styling
    float nodeSize_ = 10.0f;
    float lineWidth_ = 2.0f;
    bool showGrid_ = true;
    bool showValues_ = true;

    // Interaction
    int selectedNode_ = -1;
    bool isDragging_ = false;
    float dragOffsetX_ = 0.0f;
    float dragOffsetY_ = 0.0f;

    std::vector<EnvelopePoint> points_;
    EnvelopeChangedCallback envelopeChangedCallback_;

    void initializePoints();
    void updatePointsFromParameters();
    void updateParametersFromPoints();
    int findNodeAt(float x, float y);
    void drawGrid(SkCanvas* canvas, const SkRect& bounds);
    void drawEnvelope(SkCanvas* canvas, const SkRect& bounds);
    void drawNodes(SkCanvas* canvas, const SkRect& bounds);
    void drawNodeLabels(SkCanvas* canvas, const SkRect& bounds);
    SkPoint valueToPoint(float time, float level, const SkRect& bounds);
    void pointToValue(const SkPoint& point, const SkRect& bounds,
                     float& time, float& level);
};

//==============================================================================
// Skia Wavetable Editor
//==============================================================================

class SkiaWavetableEditor : public SkiaComponent {
public:
    SkiaWavetableEditor();
    ~SkiaWavetableEditor() override = default;

    void drawSkia(SkCanvas* canvas) override;

    void setWavetableData(const float** frames, int numFrames, int numSamplesPerFrame);
    void setCurrentFrame(int frame);

    void setInterpolationEnabled(bool enabled) { interpolationEnabled_ = enabled; }
    void set3DMode(bool enabled) { is3DMode_ = enabled; }

    // Editing
    void setEditMode(bool enabled) { editMode_ = enabled; }
    void setTool(Tool tool) { currentTool_ = tool; }

    enum class Tool {
        Select,
        Draw,
        Smooth,
        Sharpen,
        Morph
    };

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    // Wavetable data
    std::vector<std::vector<float>> frames_;
    int currentFrame_ = 0;
    bool interpolationEnabled_ = true;
    bool is3DMode_ = false;
    bool editMode_ = false;
    Tool currentTool_ = Tool::Select;

    // Display
    float zoomLevel_ = 1.0f;
    float scrollOffset_ = 0.0f;
    int hoveredSample_ = -1;
    int selectedSample_ = -1;

    // Drawing
    std::vector<float> drawBuffer_;

    void drawWavetable3D(SkCanvas* canvas, const SkRect& bounds);
    void drawSingleFrame(SkCanvas* canvas, const SkRect& bounds);
    void drawFrameMarkers(SkCanvas* canvas, const SkRect& bounds);
    void drawCrosshair(SkCanvas* canvas, const SkRect& bounds);
    SkPoint getSamplePosition(int sampleIndex, const SkRect& bounds);
    int getSampleAtPosition(const SkPoint& pos, const SkRect& bounds);
};

//==============================================================================
// Skia Modulation Matrix
//==============================================================================

class SkiaModulationMatrix : public SkiaComponent {
public:
    struct ModulationConnection {
        int sourceIndex;
        int destinationIndex;
        float amount;
        SkColor color;

        // Position for bezier curve control points
        float controlOffsetX = 0.0f;
        float controlOffsetY = 0.0f;
    };

    SkiaModulationMatrix();
    ~SkiaModulationMatrix() override = default;

    void drawSkia(SkCanvas* canvas) override;

    void setNumSources(int num) { numSources_ = num; }
    void setNumDestinations(int num) { numDestinations_ = num; }

    void setSourceName(int index, const juce::String& name);
    void setDestinationName(int index, const juce::String& name);

    void addConnection(const ModulationConnection& connection);
    void removeConnection(int sourceIndex, int destIndex);
    void clearConnections();

    void setConnectionAmount(int sourceIndex, int destIndex, float amount);

    // Callbacks
    using ConnectionChangedCallback = std::function<void(int, int, float)>;
    void setConnectionChangedCallback(ConnectionChangedCallback callback) {
        connectionChangedCallback_ = std::move(callback);
    }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

private:
    int numSources_ = 8;
    int numDestinations_ = 16;
    std::vector<juce::String> sourceNames_;
    std::vector<juce::String> destinationNames_;
    std::vector<ModulationConnection> connections_;

    // Interaction
    bool isCreatingConnection_ = false;
    int tempSourceIndex_ = -1;
    int hoveredSource_ = -1;
    int hoveredDestination_ = -1;
    int draggedConnection_ = -1;
    juce::Point<float> mousePos_;

    ConnectionChangedCallback connectionChangedCallback_;

    void drawSources(SkCanvas* canvas, const SkRect& bounds);
    void drawDestinations(SkCanvas* canvas, const SkRect& bounds);
    void drawConnections(SkCanvas* canvas, const SkRect& bounds);
    void drawTempConnection(SkCanvas* canvas, const SkRect& bounds);
    SkPoint getSourcePosition(int index, const SkRect& bounds);
    SkPoint getDestinationPosition(int index, const SkRect& bounds);
    int findSourceAt(const SkPoint& pos, const SkRect& bounds);
    int findDestinationAt(const SkPoint& pos, const SkRect& bounds);
    int findConnectionAt(const SkPoint& pos, const SkRect& bounds);
};

//==============================================================================
// Skia Macro Control
//==============================================================================

class SkiaMacroControl : public SkiaComponent {
public:
    SkiaMacroControl();
    ~SkiaMacroControl() override = default;

    void drawSkia(SkCanvas* canvas) override;

    void setMacroIndex(int index) { macroIndex_ = index; }
    int getMacroIndex() const { return macroIndex_; }

    void setName(const juce::String& name) { name_ = name; }
    void setColor(SkColor color) { color_ = color; }

    void setValue(float value) { value_ = value; markDirty(); }
    float getValue() const { return value_; }

    void setNumAssignments(int num) { numAssignments_ = num; }

    // MIDI learn
    void setMidiLearning(bool learning) { midiLearning_ = learning; }
    bool isMidiLearning() const { return midiLearning_; }
    void setMidiCC(int cc) { midiCC_ = cc; }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    int macroIndex_ = 0;
    juce::String name_ = "Macro 1";
    SkColor color_ = SkColorSetRGB(33, 150, 243);
    float value_ = 0.5f;
    int numAssignments_ = 0;
    bool midiLearning_ = false;
    int midiCC_ = -1;

    bool isDragging_ = false;
    float dragStartValue_ = 0.0f;

    void drawKnob(SkCanvas* canvas, const SkRect& bounds);
    void drawName(SkCanvas* canvas, const SkRect& bounds);
    void drawMidiIndicator(SkCanvas* canvas, const SkRect& bounds);
    void drawAssignmentCount(SkCanvas* canvas, const SkRect& bounds);
};

//==============================================================================
// Skia Synth Main Panel
//==============================================================================

class SkiaSynthPanel : public SkiaComponent {
public:
    SkiaSynthPanel();
    ~SkiaSynthPanel() override = default;

    void drawSkia(SkCanvas* canvas) override;

    void setSynthName(const juce::String& name) { synthName_ = name; }

    // Oscillator section
    void addOscillatorControl(std::unique_ptr<SkiaRotaryKnob> knob);
    void setOscillatorWaveformDisplay(std::unique_ptr<SkiaWaveformDisplay> display);

    // Filter section
    void addFilterControl(std::unique_ptr<SkiaRotaryKnob> knob);
    void setFilterCutoffKnob(std::unique_ptr<SkiaRotaryKnob> knob);

    // Envelope section
    void setEnvelopeEditor(std::unique_ptr<SkiaEnvelopeEditor> editor);

    // Effects section
    void addEffectControl(std::unique_ptr<SkiaLinearFader> fader);

    // Macro section
    void addMacroControl(std::unique_ptr<SkiaMacroControl> macro);

    // Layout
    void resized() override;

private:
    juce::String synthName_ = "Zenith Ultra Synth";

    // Sections
    std::vector<std::unique_ptr<SkiaRotaryKnob>> oscillatorKnobs_;
    std::vector<std::unique_ptr<SkiaRotaryKnob>> filterKnobs_;
    std::vector<std::unique_ptr<SkiaLinearFader>> effectFaders_;
    std::vector<std::unique_ptr<SkiaMacroControl>> macroControls_;

    std::unique_ptr<SkiaWaveformDisplay> oscillatorWaveform_;
    std::unique_ptr<SkiaEnvelopeEditor> ampEnvelope_;

    // Layout rects
    SkRect oscillatorSection_;
    SkRect filterSection_;
    SkRect envelopeSection_;
    SkRect effectsSection_;
    SkRect macroSection_;

    void drawBackground(SkCanvas* canvas, const SkRect& bounds);
    void drawSectionLabels(SkCanvas* canvas, const SkRect& bounds);
    void drawSectionBackgrounds(SkCanvas* canvas, const SkRect& bounds);
};

} // namespace zenith
