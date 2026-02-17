/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
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
#pragma clang diagnostic pop
}

namespace zenith {

//==============================================================================
// Synth State Snapshot
//==============================================================================

class SynthStateSnapshot {
public:
    // All synth parameters stored as key-value pairs
    juce::StringPairArray parameters;

    // Modulation matrix state
    juce::StringArray modulationRoutes;

    // Macro states
    std::array<float, 8> macroValues;
    juce::String macroNames[8];
    juce::uint32 macroColors[8];

    // Waveform states
    struct OscillatorState {
        int waveformType;
        float detune;
        float mix;
        float phase;
        float shape;
    };
    std::array<OscillatorState, 3> oscillatorStates;

    // Filter states
    struct FilterState {
        int type;
        float cutoff;
        float resonance;
        float drive;
        float envAmount;
    };
    std::array<FilterState, 2> filterStates;

    // Envelope states (4 ADSR envelopes)
    struct EnvelopeState {
        float attack, decay, sustain, release;
        float attackCurve, decayCurve, releaseCurve;
    };
    std::array<EnvelopeState, 4> envelopeStates;

    // Effects states
    juce::StringArray effectsSettings;

    void clear() {
        parameters.clear();
        modulationRoutes.clear();
        macroValues.fill(0.5f);
        for (auto& name : macroNames) name = "Macro " + juce::String(&name - macroNames + 1);
        for (auto& color : macroColors) color = 0xFF2196F3;
    }

    bool isEmpty() const {
        return parameters.size() == 0;
    }

    // Serialization
    juce::String toJSON() const;
    bool fromJSON(const juce::String& json);

    // Compare
    bool operator==(const SynthStateSnapshot& other) const;
    bool operator!=(const SynthStateSnapshot& other) const {
        return !(*this == other);
    }
};

//==============================================================================
// A/B Comparison Manager
//==============================================================================

class ABComparisonManager {
public:
    enum class State {
        A,      // Currently viewing state A
        B,      // Currently viewing state B
        Comparing  // Currently morphing between A and B
    };

    ABComparisonManager();

    // State management
    void captureStateA();
    void captureStateB();
    void switchToA();
    void switchToB();
    void toggleAB();          // Switch between A and B

    State getCurrentState() const { return currentState_; }
    bool isShowingA() const { return currentState_ == State::A; }
    bool isShowingB() const { return currentState_ == State::B; }
    bool isComparing() const { return currentState_ == State::Comparing; }

    // Morphing
    void setMorphAmount(float amount);  // 0.0 = full A, 1.0 = full B
    float getMorphAmount() const { return morphAmount_; }
    void morph(float amount);

    // Comparison
    float calculateDifference() const;  // How different are A and B?

    // Preset management
    juce::String getPresetNameA() const { return presetNameA_; }
    juce::String getPresetNameB() const { return presetNameB_; }
    void setPresetNameA(const juce::String& name) { presetNameA_ = name; }
    void setPresetNameB(const juce::String& name) { presetNameB_ = name; }

    // Copy A to B or B to A
    void copyAToB();
    void copyBToA();

    // Clear
    void clearA();
    void clearB();
    bool hasStateA() const { return !stateA_.isEmpty(); }
    bool hasStateB() const { return !stateB_.isEmpty(); }

    // History
    void saveToA(const SynthStateSnapshot& state);
    void saveToB(const SynthStateSnapshot& state);
    SynthStateSnapshot getStateA() const { return stateA_; }
    SynthStateSnapshot getStateB() const { return stateB_; }

private:
    SynthStateSnapshot stateA_;
    SynthStateSnapshot stateB_;
    SynthStateSnapshot displayState_;

    juce::String presetNameA_ = "State A";
    juce::String presetNameB_ = "State B";

    State currentState_ = State::A;
    float morphAmount_ = 0.0f;
    float morphSmoothing_ = 0.0f;  // For animated morphing

    juce::Random random_;
};

//==============================================================================
// SkiaABComparisonComponent
//==============================================================================

class SkiaABComparisonComponent : public SkiaComponent {
public:
    SkiaABComparisonComponent();
    ~SkiaABComparisonComponent() override = default;

    void drawSkia(SkCanvas* canvas) override;

    // Manager access
    ABComparisonManager& getManager() { return manager_; }
    const ABComparisonManager& getManager() const { return manager_; }

    // Display mode
    void setShowMorphSlider(bool show) { showMorphSlider_ = show; markDirty(); }
    void setShowDifferenceMeter(bool show) { showDifferenceMeter_ = show; markDirty(); }
    void setCompactMode(bool compact) { compactMode_ = compact; markDirty(); }

    // Style
    void setButtonAColor(SkColor color) { buttonAColor_ = color; markDirty(); }
    void setButtonBColor(SkColor color) { buttonBColor_ = color; markDirty(); }
    void setMorphColor(SkColor color) { morphColor_ = color; markDirty(); }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    ABComparisonManager manager_;

    // Display options
    bool showMorphSlider_ = true;
    bool showDifferenceMeter_ = true;
    bool compactMode_ = false;

    // Colors
    SkColor buttonAColor_ = SkColorSetRGB(100, 150, 255);
    SkColor buttonBColor_ = SkColorSetRGB(255, 100, 150);
    SkColor morphColor_ = SkColorSetRGB(150, 255, 200);

    // Layout
    SkRect buttonARect_;
    SkRect buttonBRect_;
    SkRect copyAToBRect_;
    SkRect copyBToARect_;
    SkRect morphSliderRect_;
    SkRect differenceMeterRect_;

    // Interaction
    bool isDraggingMorph_ = false;
    float dragStartAmount_ = 0.0f;
    float dragStartY_ = 0.0f;

    // Animation
    float morphAnimationTarget_ = 0.0f;
    float morphAnimationCurrent_ = 0.0f;

    // Drawing helpers
    void drawBackground(SkCanvas* canvas, const SkRect& bounds);
    void drawButton(SkCanvas* canvas, const SkRect& bounds, const juce::String& label,
                  SkColor color, bool isActive, bool isHovered);
    void drawMorphSlider(SkCanvas* canvas, const SkRect& bounds, float amount,
                       bool isDragging, bool isHovered);
    void drawDifferenceMeter(SkCanvas* canvas, const SkRect& bounds, float difference);
    void drawCopyButton(SkCanvas* canvas, const SkRect& bounds, bool copyTo, bool isHovered);

    void updateLayout();
};

//==============================================================================
// Quick AB Button (for toolbar)
//==============================================================================

class SkiaABQuickButton : public SkiaComponent {
public:
    SkiaABQuickButton();
    ~SkiaABQuickButton() override = default;

    void drawSkia(SkCanvas* canvas) override;

    void setManager(std::shared_ptr<ABComparisonManager> manager) {
        manager_ = manager;
    }

    void setDisplayMode(bool showState, bool showMorph) {
        showState_ = showState;
        showMorph_ = showMorph;
        markDirty();
    }

    void setShowName(bool show) { showName_ = show; markDirty(); }

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    std::shared_ptr<ABComparisonManager> manager_;
    bool showState_ = true;      // Show "A" or "B"
    bool showMorph_ = true;      // Show morph position
    bool showName_ = true;        // Show preset name

    ABComparisonManager::State lastKnownState_;

    SkRect stateRect_;           // A/B indicator
    SkRect nameRect_;            // Preset name
    SkRect morphRect_;           // Morph position indicator

    void updateLayout();
    SkColor getCurrentColor() const;
};

} // namespace zenith
