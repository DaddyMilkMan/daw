/*
  ==============================================================================

    ModulationVisualizer.h
    Created: 2026-02-01
    Author:  Zenith DAW

    Real-time modulation visualization for ZenithPolySynth.
    Shows LFO waveforms, envelopes, and modulation matrix connections.

  ==============================================================================
*/

#pragma once

#include "../../instruments/ZenithPolySynthDefs.h"
#include "../framework/SkiaComponent.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include <complex>

#ifdef ZENITH_USE_SKIA
extern "C++" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wattributes"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRect.h>
#include <core/SkFont.h>
#include <core/SkShader.h>
#include <effects/SkGradientShader.h>
#include <core/SkMaskFilter.h>
#pragma clang diagnostic pop
}
#endif

namespace zenith {

//==============================================================================
/**
    LFO state for visualization
*/
struct LFOVisualState {
    float phase = 0.0f;           // Current phase (0-1)
    float rate = 1.0f;            // Rate in Hz
    float amount = 0.5f;          // Modulation amount
    LFOWaveform waveform = LFOWaveform::Sine;
    bool bpmSynced = false;
    int syncRate = static_cast<int>(SyncRate::_1_4);

    // For visualization path
    std::vector<float> waveformBuffer;  // Pre-calculated waveform
};

//==============================================================================
/**
    Envelope state for visualization
*/
struct EnvelopeVisualState {
    float attack = 0.01f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.2f;
    float currentValue = 0.0f;     // Current output level
    int currentStage = 0;          // 0=attack, 1=decay, 2=sustain, 3=release, 4=idle
    float stageProgress = 0.0f;    // Progress through current stage (0-1)
    bool isActive = false;         // Is envelope currently running
};

//==============================================================================
/**
    Modulation routing visualization
*/
struct ModulationRouting {
    ModulationSource source;
    ModulationDestination dest;
    float amount = 0.0f;
    juce::String sourceName;
    juce::String destName;
    SkColor color;

    bool isActive() const {
        return source != ModulationSource::None && dest != ModulationDestination::None;
    }
};

//==============================================================================
/**
    Real-time modulation visualization component

    Features:
    - LFO waveform display with animated phase indicator
    - Envelope display with current stage indicator
    - Modulation matrix with animated connection lines
    - Real-time modulation value display
*/
class ModulationVisualizer : public SkiaComponent {
public:
    ModulationVisualizer();
    ~ModulationVisualizer() override;

    //==========================================================================
    // LFO Configuration
    //==========================================================================

    /** Set LFO 1 state */
    void setLFO1State(const LFOVisualState& state);

    /** Set LFO 2 state */
    void setLFO2State(const LFOVisualState& state);

    /** Update LFO 1 phase (called from audio thread or timer) */
    void setLFO1Phase(float phase) { lfo1Phase_.store(phase); }

    /** Update LFO 2 phase (called from audio thread or timer) */
    void setLFO2Phase(float phase) { lfo2Phase_.store(phase); }

    //==========================================================================
    // Envelope Configuration
    //==========================================================================

    /** Set Amp Envelope state */
    void setAmpEnvelopeState(const EnvelopeVisualState& state);

    /** Set Mod Envelope state */
    void setModEnvelopeState(const EnvelopeVisualState& state);

    /** Update envelope current values */
    void setAmpEnvelopeValue(float value) { ampEnvValue_.store(value); }
    void setModEnvelopeValue(float value) { modEnvValue_.store(value); }

    //==========================================================================
    // Modulation Matrix Configuration
    //==========================================================================

    /** Set active modulation routings */
    void setModulationRoutings(const std::vector<ModulationRouting>& routings);

    /** Add a modulation routing */
    void addModulationRouting(const ModulationRouting& routing);

    /** Clear all routings */
    void clearModulationRoutings();

    /** Update current modulation values for display */
    void setModulationValue(ModulationDestination dest, float value);

    //==========================================================================
    // Display Options
    //==========================================================================

    /** Show/hide LFO section */
    void setShowLFO(bool show) { showLFO_ = show; markDirty(); }

    /** Show/hide Envelope section */
    void setShowEnvelopes(bool show) { showEnvelopes_ = show; markDirty(); }

    /** Show/hide Modulation Matrix */
    void setShowMatrix(bool show) { showMatrix_ = show; markDirty(); }

    /** Set display mode (compact, full, matrix-only) */
    enum class DisplayMode {
        Compact,   // Just LFOs and envelopes
        Full,      // Everything
        MatrixOnly // Just modulation matrix
    };
    void setDisplayMode(DisplayMode mode) { displayMode_ = mode; markDirty(); }

    //==========================================================================
    // Component overrides
    //==========================================================================

#ifdef ZENITH_USE_SKIA
    void drawSkia(SkCanvas* canvas) override;
#endif

    void timerCallback() override;

private:
#ifdef ZENITH_USE_SKIA
    //==========================================================================
    // Drawing methods
    //==========================================================================

    /** Draw all sections */
    void drawAllSections(SkCanvas* canvas);

    /** Draw LFO waveform with phase indicator */
    void drawLFOWaveform(SkCanvas* canvas, const SkRect& bounds,
                        LFOWaveform waveform, float phase, SkColor color,
                        const juce::String& label);

    /** Draw envelope with current stage indicator */
    void drawEnvelope(SkCanvas* canvas, const SkRect& bounds,
                     const EnvelopeVisualState& state, SkColor color,
                     const juce::String& label);

    /** Draw modulation matrix */
    void drawModulationMatrix(SkCanvas* canvas, const SkRect& bounds);

    /** Draw animated modulation connection line */
    void drawModulationConnection(SkCanvas* canvas, const SkRect& sourceRect,
                                 const SkRect& destRect, float amount,
                                 float currentValue, SkColor color);

    /** Draw a single modulation source node */
    void drawSourceNode(SkCanvas* canvas, const SkRect& bounds,
                       ModulationSource source, SkColor color);

    /** Draw a single modulation destination node */
    void drawDestNode(SkCanvas* canvas, const SkRect& bounds,
                     ModulationDestination dest, float currentValue);

    /** Generate LFO waveform path */
    void generateLFOPath(LFOWaveform waveform, SkPath& path,
                        const SkRect& bounds, float phase);

    /** Generate envelope path */
    void generateEnvelopePath(const EnvelopeVisualState& state, SkPath& path,
                             const SkRect& bounds);

    /** Get color for modulation source */
    SkColor getSourceColor(ModulationSource source);

    /** Get color for modulation destination */
    SkColor getDestColor(ModulationDestination dest);

    /** Get name for modulation source */
    juce::String getSourceName(ModulationSource source);

    /** Get name for modulation destination */
    juce::String getDestName(ModulationDestination dest);

    /** Calculate LFO value at given phase */
    float calculateLFOValue(LFOWaveform waveform, float phase);
#endif

    //==========================================================================
    // Member variables
    //==========================================================================

    // LFO state
    LFOVisualState lfo1State_;
    LFOVisualState lfo2State_;
    std::atomic<float> lfo1Phase_{0.0f};
    std::atomic<float> lfo2Phase_{0.0f};

    // Envelope state
    EnvelopeVisualState ampEnvState_;
    EnvelopeVisualState modEnvState_;
    std::atomic<float> ampEnvValue_{0.0f};
    std::atomic<float> modEnvValue_{0.0f};

    // Modulation matrix
    std::vector<ModulationRouting> routings_;
    std::array<float, static_cast<size_t>(ModulationDestination::NumDestinations)>
        currentModValues_{};

    // Display settings
    DisplayMode displayMode_ = DisplayMode::Full;
    bool showLFO_ = true;
    bool showEnvelopes_ = true;
    bool showMatrix_ = true;

    // Animation state
    float animationPhase_ = 0.0f;
    float pulsePhase_ = 0.0f;
    std::vector<std::pair<float, float>> pulsePositions_; // For each routing

    // Premium colors
    SkColor backgroundColor_ = SkColorSetARGB(255, 12, 12, 18);
    SkColor gridColor_ = SkColorSetARGB(80, 45, 45, 55);
    SkColor textColor_ = SkColorSetARGB(220, 165, 165, 175);

    // LFO colors with gradients
    SkColor lfo1Color_ = SkColorSetARGB(255, 0, 220, 255);     // Bright Cyan
    SkColor lfo1GradientEnd_ = SkColorSetARGB(100, 0, 150, 200);
    SkColor lfo2Color_ = SkColorSetARGB(255, 255, 0, 150);     // Magenta
    SkColor lfo2GradientEnd_ = SkColorSetARGB(100, 180, 0, 100);

    // Envelope colors with gradients
    SkColor ampEnvColor_ = SkColorSetARGB(255, 120, 255, 120);  // Green
    SkColor ampEnvGradientEnd_ = SkColorSetARGB(80, 80, 180, 80);
    SkColor modEnvColor_ = SkColorSetARGB(255, 255, 220, 0);    // Yellow
    SkColor modEnvGradientEnd_ = SkColorSetARGB(100, 100, 60, 0);

    // Matrix colors with glow variants
    std::array<SkColor, static_cast<size_t>(ModulationSource::NumSources)> sourceColors_ = {{
        SkColorSetARGB(255, 60, 60, 60),     // None
        SkColorSetARGB(255, 0, 220, 255),    // LFO1
        SkColorSetARGB(255, 255, 0, 150),    // LFO2
        SkColorSetARGB(255, 120, 255, 120),  // Env1
        SkColorSetARGB(255, 255, 220, 0),    // Env2
        SkColorSetARGB(255, 255, 120, 120),  // Velocity
        SkColorSetARGB(255, 220, 120, 255),  // ModWheel
        SkColorSetARGB(255, 255, 170, 70),   // Aftertouch
        SkColorSetARGB(255, 120, 220, 255)   // Timbre
    }};

    std::array<SkColor, static_cast<size_t>(ModulationSource::NumSources)> glowColors_ = {{
        SkColorSetARGB(100, 40, 40, 40),     // None
        SkColorSetARGB(150, 0, 180, 220),    // LFO1
        SkColorSetARGB(150, 180, 0, 100),    // LFO2
        SkColorSetARGB(150, 80, 180, 80),    // Env1
        SkColorSetARGB(150, 180, 160, 0),    // Env2
        SkColorSetARGB(150, 180, 80, 80),    // Velocity
        SkColorSetARGB(150, 160, 80, 180),   // ModWheel
        SkColorSetARGB(150, 180, 120, 50),   // Aftertouch
        SkColorSetARGB(150, 80, 160, 180)    // Timbre
    }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationVisualizer)
};

} // namespace zenith
