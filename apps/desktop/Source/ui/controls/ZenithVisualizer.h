/*
  ==============================================================================

    ZenithVisualizer.h
    Created: 2025-12-12
    Author:  Zenith DAW

    Visualizer widget for ZenithPolySynth.
    Consumes audio data from the processor and renders it using Skia.

  ==============================================================================
*/

#pragma once

#include "../../instruments/ZenithPolySynth.h"
#include "SkiaComponent.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>

#ifdef ZENITH_USE_SKIA
#include "ZenithSkia.h"
#include <core/SkPath.h>
#include <effects/SkGradientShader.h>
#endif

namespace zenith {

class ZenithVisualizer : public SkiaComponent {
public:
  explicit ZenithVisualizer(ZenithPolySynthProcessor &processor);
  ~ZenithVisualizer() override = default;

  // ----- Rendering -----
  void drawSkia(SkCanvas *canvas) override;

  // ----- Audio -----
  void updateAudioData();
  void timerCallback() override;

private:
#ifdef ZENITH_USE_SKIA
  void drawWaveform(SkCanvas *canvas);
  void drawSpectrum(SkCanvas *canvas);
#endif

  ZenithPolySynthProcessor &processor_;
  std::vector<float> audioBuffer_;
  std::vector<float> displayBuffer_;

  // Visualization settings (waveform)
  SkColor waveformColor_ = SkColorSetRGB(0, 255, 255);
  SkColor fillColor_ = SkColorSetARGB(50, 0, 255, 255);

  // FFT processing for spectrum analysis
  static constexpr int fftOrder_ = 11;             // 2048-point FFT
  static constexpr int fftSize_ = 1 << fftOrder_; // 2048
  static constexpr int numBands_ = 64;             // Frequency bars
  juce::dsp::FFT forwardFFT_{fftOrder_};
  juce::dsp::WindowingFunction<float> window_{
      static_cast<size_t>(fftSize_), juce::dsp::WindowingFunction<float>::hann};

  // FFT buffers
  std::vector<float> fftData_;      // Complex FFT output
  std::vector<float> spectrumData_; // Processed magnitude spectrum (64 bands)
  std::vector<float> spectrumPeakData_; // Peak hold values

  // Spectrum settings
  float spectrumDecaySpeed_ = 0.85f;
  SkColor spectrumColorTop_ = SkColorSetRGB(255, 0, 128); // Magenta
  SkColor spectrumColorBottom_ = SkColorSetARGB(80, 255, 0, 128);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithVisualizer)
};

} // namespace zenith
