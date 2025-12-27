/*
  ==============================================================================

    SpectraAnalyzerComponent.h
    Created: 2025-12-13
    Author:  Zenith DAW

    Global Visualizer: Spectrum, Scope, and Vectorscope using Skia.
    Decouples analysis (FIFO) from rendering (VSync).

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>

#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkShader.h>
#include <effects/SkGradientShader.h>

#endif

#include "../../dsp/StereoAudioFifo.h"
#include "../framework/SkiaComponent.h"

namespace zenith {

class Engine;

class SpectraAnalyzerComponent : public SkiaComponent {
public:
  enum class AnalysisMode { Spectrum, Scope, StereoField };

  explicit SpectraAnalyzerComponent(Engine &engine);
  ~SpectraAnalyzerComponent() override;

  // SkiaComponent overrides
  void drawSkia(SkCanvas *canvas) override;
  void timerCallback() override;
  void resized() override;

  // AI Vision Hook
  std::vector<AIElementInfo> getInspectableElements() override {
    // Report interactive elements for AI
    // (Currently just buttons, which are standard components, but we could
    // report graph areas)
    return {};
  }

  void setMode(AnalysisMode mode);
  AnalysisMode getMode() const { return currentMode_; }

private:
  Engine &engine_;
  AnalysisMode currentMode_ = AnalysisMode::Spectrum;

  // Analysis
  static constexpr int fftOrder = 11; // 2048 points
  static constexpr int fftSize = 1 << fftOrder;
  static constexpr int scopeSize = 1024;

  juce::dsp::FFT fft;
  juce::dsp::WindowingFunction<float> window;

  std::vector<float> fftData;
  std::vector<float> scopeDataL;
  std::vector<float> scopeDataR;

  // Smooth decay for FFT
  std::vector<float> fftSmoothData;

  // Temporary buffer for FIFO pop
  juce::AudioBuffer<float> tempBuffer;

  // UI/Rendering
  float animationPhase_ = 0.0f;
  float decayRate_ = 0.85f; // Persistence

  // Mode Buttons
  juce::TextButton spectrumBtn{"FFT"};
  juce::TextButton scopeBtn{"WAVE"};
  juce::TextButton stereoBtn{"FIELD"};

  void processAudioData();
  void renderSpectrum(SkCanvas *canvas, const SkRect &bounds);
  void renderScope(SkCanvas *canvas, const SkRect &bounds);
  void renderStereoField(SkCanvas *canvas, const SkRect &bounds);

  // Helper to generate fluid path
  void createCurvedPath(SkPath &path, const std::vector<float> &data,
                        const SkRect &bounds, bool fill);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectraAnalyzerComponent)
};

} // namespace zenith
