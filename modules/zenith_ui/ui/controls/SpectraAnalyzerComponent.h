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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    SpectraAnalyzerComponent.h
    Created: 2025-12-13
    Author:  Zenith DAW

    Global Visualizer: Spectrum, Scope, and Vectorscope using Skia.
    Decouples analysis (FIFO) from rendering (VSync).


  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"

#include <core/SkPath.h>
#include <core/SkShader.h>
#include <effects/SkGradientShader.h>

#endif

#include "../../dsp/StereoAudioFifo.h"
#include "../controls/ZenithButton.h"
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
  zenith::ZenithButton spectrumBtn{"FFT"};
  zenith::ZenithButton scopeBtn{"WAVE"};
  zenith::ZenithButton stereoBtn{"FIELD"};

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
