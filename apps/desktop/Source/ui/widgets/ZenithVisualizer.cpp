/*
  ==============================================================================

    ZenithVisualizer.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the Zenith visualizer.

  ==============================================================================
*/

#include "ZenithVisualizer.h"

namespace zenith {

ZenithVisualizer::ZenithVisualizer(ZenithPolySynthProcessor &processor)
    : processor_(processor) {
  audioBuffer_.resize(512, 0.0f);
  displayBuffer_.resize(512, 0.0f);

  // Updates are driven by parent or own timer
  startTimerHz(60);
}

void ZenithVisualizer::timerCallback() {
  updateAudioData();
  repaint();
}

void ZenithVisualizer::updateAudioData() {
  // Read new samples from processor's FIFO
  int needed = audioBuffer_.size();
  int read = processor_.readFromVisualizer(audioBuffer_.data(), needed);

  if (read > 0) {
    // Copy valid samples
    std::copy(audioBuffer_.begin(), audioBuffer_.begin() + read,
              displayBuffer_.begin());

    // Zero the rest to avoid artifacts
    if (static_cast<size_t>(read) < displayBuffer_.size()) {
      std::fill(displayBuffer_.begin() + read, displayBuffer_.end(), 0.0f);
    }
  }
}

void ZenithVisualizer::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  if (canvas == nullptr)
    return;

  auto bounds = getLocalBounds().toFloat();

  SkPaint bgPaint;
  bgPaint.setColor(SkColorSetARGB(50, 0, 0, 0));
  canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()),
                   bgPaint);

  drawWaveform(canvas);
#else
  juce::ignoreUnused(canvas);
#endif
}

#ifdef ZENITH_USE_SKIA

void ZenithVisualizer::drawWaveform(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  float width = bounds.getWidth();
  float height = bounds.getHeight();
  float midY = height / 2.0f;

  SkPath path;
  path.moveTo(0, midY);

  float xStep = width / (float)displayBuffer_.size();

  for (size_t i = 0; i < displayBuffer_.size(); ++i) {
    float sample = displayBuffer_[i];
    float x = i * xStep;
    float y = midY - (sample * height * 0.5f); // Scale amplitude
    path.lineTo(x, y);
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(2.0f);
  paint.setColor(waveformColor_);

  canvas->drawPath(path, paint);

  // Fill below
  path.lineTo(width, height);
  path.lineTo(0, height);
  path.close();

  SkPaint fillPaint;
  fillPaint.setStyle(SkPaint::kFill_Style);

  SkPoint pts[2] = {{0, 0}, {0, height}};
  SkColor colors[2] = {fillColor_, SkColorSetARGB(0, 0, 0, 0)};

  fillPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                   SkTileMode::kClamp));
  canvas->drawPath(path, fillPaint);
}

void ZenithVisualizer::drawSpectrum(SkCanvas *canvas) {
  // Placeholder for spectrum
  juce::ignoreUnused(canvas);
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
