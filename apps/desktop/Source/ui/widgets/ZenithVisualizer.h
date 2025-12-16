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
#include <vector>


#ifdef ZENITH_USE_SKIA
#include <core/SkCanvas.h>
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

  // Visualization settings
  SkColor waveformColor_ = SkColorSetRGB(0, 255, 255);
  SkColor fillColor_ = SkColorSetARGB(50, 0, 255, 255);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithVisualizer)
};

} // namespace zenith
