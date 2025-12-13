/*
  ==============================================================================

    SpectraAnalyzerComponent.cpp
    Created: 2025-12-13
    Author:  Zenith DAW

  ==============================================================================
*/

#include "SpectraAnalyzerComponent.h"
#include "../../include/Engine.h"
#include "../../ui/skia/ZenithDesignSystem.h"

#ifdef ZENITH_USE_SKIA
#include <core/SkMaskFilter.h>
#include <effects/SkGradientShader.h>

#endif

namespace zenith {

SpectraAnalyzerComponent::SpectraAnalyzerComponent(Engine &engine)
    : SkiaComponent(), engine_(engine), fft(fftOrder),
      window(fftSize, juce::dsp::WindowingFunction<float>::hann) {
  // Initialize buffers
  fftData.resize(fftSize * 2, 0.0f);
  fftSmoothData.resize(fftSize / 2, 0.0f); // Display half size (nyquist)

  // Scope buffers
  scopeDataL.resize(scopeSize, 0.0f);
  scopeDataR.resize(scopeSize, 0.0f);

  // Temp buffer for pop
  tempBuffer.setSize(2, 4096);

  // Setup Buttons
  addAndMakeVisible(spectrumBtn);
  addAndMakeVisible(scopeBtn);
  addAndMakeVisible(stereoBtn);

  spectrumBtn.onClick = [this] { setMode(AnalysisMode::Spectrum); };
  scopeBtn.onClick = [this] { setMode(AnalysisMode::Scope); };
  stereoBtn.onClick = [this] { setMode(AnalysisMode::StereoField); };

  // Basic styling
  auto styleBtn = [](juce::TextButton &btn) {
    btn.setColour(juce::TextButton::buttonColourId,
                  juce::Colours::black.withAlpha(0.5f));
    btn.setColour(juce::TextButton::textColourOffId,
                  juce::Colours::white.withAlpha(0.7f));
    btn.setColour(juce::TextButton::textColourOnId, juce::Colours::cyan);
  };
  styleBtn(spectrumBtn);
  styleBtn(scopeBtn);
  styleBtn(stereoBtn);

  setMode(AnalysisMode::Spectrum); // Default

  // Start rendering loop in UI thread (60Hz)
  // SkiaComponent also starts a timer if setTargetFPS is used, or we just rely
  // on ours. SkiaComponent usually defaults to 60fps.
  setTargetFPS(60);
}

SpectraAnalyzerComponent::~SpectraAnalyzerComponent() { stopTimer(); }

void SpectraAnalyzerComponent::resized() {
  SkiaComponent::resized(); // Base class (if needed for glow etc)

  auto area = getLocalBounds();

  // Mode switcher at top right
  // Pushing down slightly to avoid window controls if docked at very top
  auto btnArea = area.removeFromTop(20).removeFromRight(150);
  int w = btnArea.getWidth() / 3;
  spectrumBtn.setBounds(btnArea.removeFromLeft(w).reduced(1));
  scopeBtn.setBounds(btnArea.removeFromLeft(w).reduced(1));
  stereoBtn.setBounds(btnArea.removeFromLeft(w).reduced(1));
}

void SpectraAnalyzerComponent::setMode(AnalysisMode mode) {
  currentMode_ = mode;

  spectrumBtn.setToggleState(mode == AnalysisMode::Spectrum,
                             juce::dontSendNotification);
  scopeBtn.setToggleState(mode == AnalysisMode::Scope,
                          juce::dontSendNotification);
  stereoBtn.setToggleState(mode == AnalysisMode::StereoField,
                           juce::dontSendNotification);
}

void SpectraAnalyzerComponent::timerCallback() {
  SkiaComponent::timerCallback(); // Run base animations

  processAudioData();
  animationPhase_ += 0.02f;

  markDirty(); // Trigger Skia repaint
}

void SpectraAnalyzerComponent::processAudioData() {
  auto *fifo = engine_.getAnalysisFifo();
  if (!fifo)
    return;

  // Drain FIFO
  int ready = fifo->getNumReady();
  if (ready <= 0)
    return;

  // Pop loop
  while (fifo->getNumReady() > 0) {
    int toRead = juce::jmin(fifo->getNumReady(), tempBuffer.getNumSamples());
    juce::AudioBuffer<float> chunk(tempBuffer.getArrayOfWritePointers(),
                                   tempBuffer.getNumChannels(), toRead);

    fifo->pop(chunk);

    int numSamples = chunk.getNumSamples();
    const float *l = chunk.getReadPointer(0);
    const float *r = chunk.getReadPointer(1);

    // Scope Data
    if (numSamples < scopeSize) {
      std::copy(scopeDataL.begin() + numSamples, scopeDataL.end(),
                scopeDataL.begin());
      std::copy(l, l + numSamples, scopeDataL.end() - numSamples);

      std::copy(scopeDataR.begin() + numSamples, scopeDataR.end(),
                scopeDataR.begin());
      std::copy(r, r + numSamples, scopeDataR.end() - numSamples);
    } else {
      std::copy(l + numSamples - scopeSize, l + numSamples, scopeDataL.begin());
      std::copy(r + numSamples - scopeSize, r + numSamples, scopeDataR.begin());
    }
  }

  // FFT Processing
  if (currentMode_ == AnalysisMode::Spectrum) {
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    size_t len = juce::jmin((size_t)fftSize, scopeDataL.size());

    for (size_t i = 0; i < len; ++i) {
      fftData[i] =
          (scopeDataL[scopeSize - len + i] + scopeDataR[scopeSize - len + i]) *
          0.5f;
    }

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    for (size_t i = 0; i < fftSmoothData.size(); ++i) {
      float val = fftData[i];
      val = std::log10(val + 1.0f) * 2.0f;

      if (val > fftSmoothData[i]) {
        fftSmoothData[i] = val;
      } else {
        fftSmoothData[i] *= decayRate_;
      }
    }
  }
}

void SpectraAnalyzerComponent::drawSkia(SkCanvas *canvas) {
  if (!canvas)
    return;

  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Draw Background
  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(250, 10, 10, 14)); // Deep background
  canvas->drawRect(skBounds, bgPaint);

  // Render Mode
  switch (currentMode_) {
  case AnalysisMode::Spectrum:
    renderSpectrum(canvas, skBounds);
    break;
  case AnalysisMode::Scope:
    renderScope(canvas, skBounds);
    break;
  case AnalysisMode::StereoField:
    renderStereoField(canvas, skBounds);
    break;
  }

  // Header overlay (optional)
  SkFont font; // Default font
  font.setSize(10.0f);
  SkPaint textPaint;
  textPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
  // canvas->drawString("SPECTRA", 10, 15, font, textPaint);
}

void SpectraAnalyzerComponent::renderSpectrum(SkCanvas *canvas,
                                              const SkRect &bounds) {
  if (fftSmoothData.empty())
    return;

  SkPath path;
  path.moveTo(bounds.fLeft, bounds.fBottom);

  size_t numBins = fftSmoothData.size();
  size_t displayBins = numBins / 2; // Up to Nyquist/2
  if (displayBins < 2)
    return;

  float binWidth = bounds.width() / (float)displayBins;

  // Draw
  for (size_t i = 0; i < displayBins; ++i) {
    float val = fftSmoothData[i];
    float normalizedHeight = juce::jmin(val * 0.8f, 1.0f);
    float y = bounds.fBottom - (normalizedHeight * bounds.height());
    float x = bounds.fLeft + (i * binWidth);
    path.lineTo(x, y);
  }

  path.lineTo(bounds.fRight, bounds.fBottom);
  path.close();

  // Gradient
  SkPoint pts[2] = {{bounds.fLeft, bounds.fBottom},
                    {bounds.fLeft, bounds.fTop}};
  SkColor colors[3] = {
      SkColorSetRGB(0, 100, 255), // Blue/Cyan
      SkColorSetRGB(180, 0, 255), // Purple
      SkColorSetRGB(255, 0, 100)  // Pink
  };

  SkPaint paint;
  paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 3,
                                               SkTileMode::kClamp));
  paint.setStyle(SkPaint::kFill_Style);
  paint.setAntiAlias(true);

  canvas->drawPath(path, paint);

  // Stroke
  paint.setShader(nullptr);
  paint.setColor(SkColorSetARGB(200, 200, 200, 255));
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.5f);
  canvas->drawPath(path, paint);
}

void SpectraAnalyzerComponent::renderScope(SkCanvas *canvas,
                                           const SkRect &bounds) {
  auto drawChannel = [&](const std::vector<float> &data, SkColor color,
                         float yOffset) {
    SkPath path;
    float h = bounds.height();
    float midY = bounds.fTop + (h * 0.5f) + yOffset;
    float scaleY = h * 0.25f;

    float stepX = bounds.width() / (float)scopeSize;

    path.moveTo(bounds.fLeft, midY - (data[0] * scaleY));
    for (int i = 1; i < scopeSize; ++i) {
      float x = bounds.fLeft + (i * stepX);
      float y = midY - (data[i] * scaleY);
      path.lineTo(x, y);
    }

    SkPaint paint;
    paint.setColor(color);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.5f);
    paint.setAntiAlias(true);

    // Glow (Outer)
    paint.setStrokeWidth(3.0f);
    paint.setColor(SkColorSetA(color, 100));
    paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 3.0f));
    canvas->drawPath(path, paint);

    // Core (Inner)
    paint.setMaskFilter(nullptr);
    paint.setStrokeWidth(1.5f);
    paint.setColor(SkColorSetA(color, 255));
    canvas->drawPath(path, paint);
  };

  drawChannel(scopeDataL, SkColorSetRGB(0, 255, 255), -20.0f);
  drawChannel(scopeDataR, SkColorSetRGB(255, 0, 150), 20.0f);
}

void SpectraAnalyzerComponent::renderStereoField(SkCanvas *canvas,
                                                 const SkRect &bounds) {
  SkPath path;
  float cx = bounds.centerX();
  float cy = bounds.centerY();
  float scale = bounds.height() * 0.35f;

  // Vectorscope: X = L-R, Y = L+R (Rotated 45deg)
  // Actually standard goniometer is: X = L-R, Y = L+R.
  // L=1, R=0 -> X=1, Y=1 (Top Right)
  // L=0, R=1 -> X=-1, Y=1 (Top Left)
  // Wait, (L+R)/2 is Mid, (L-R)/2 is Side.
  // L only -> Diag.

  auto getPt = [&](int i) {
    float l = scopeDataL[i];
    float r = scopeDataR[i];
    // Standard Goniometer orientation:
    // Mid (L+R) is Vertical Y
    // Side (L-R) is Horizontal X
    float x = (l - r) * scale;  // Side
    float y = -(l + r) * scale; // Mid (Up is negative)
    return SkPoint::Make(cx + x, cy + y);
  };

  path.moveTo(getPt(0));

  // Decimate for performance?
  int step = 1;
  for (int i = step; i < scopeSize; i += step) {
    path.lineTo(getPt(i));
  }

  SkPaint paint;
  paint.setColor(SkColorSetARGB(180, 0, 255, 128)); // Spring green
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(1.0f);
  paint.setAntiAlias(true);

  // Glow
  SkPaint glow = paint;
  glow.setStrokeWidth(2.5f);
  glow.setColor(SkColorSetARGB(80, 0, 255, 128));
  glow.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
  canvas->drawPath(path, glow);

  canvas->drawPath(path, paint);
}

void SpectraAnalyzerComponent::createCurvedPath(SkPath &path,
                                                const std::vector<float> &data,
                                                const SkRect &bounds,
                                                bool fill) {
  // Unused helper for now, keeping signature
}

} // namespace zenith
