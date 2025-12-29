/*
  ==============================================================================

    ZenithVisualizer.cpp
    Created: 2025-12-12
    Author:  Zenith DAW

    Implementation of the Zenith visualizer.

  ==============================================================================
*/

#include "ZenithVisualizer.h"
// Forced compilation check

namespace zenith {

ZenithVisualizer::ZenithVisualizer(ZenithPolySynthProcessor &processor)
    : processor_(processor) {
  audioBuffer_.resize(fftSize_, 0.0f);
  displayBuffer_.resize(fftSize_, 0.0f);
  fftData_.resize(fftSize_ * 2, 0.0f); // 2x for complex output
  spectrumData_.assign(numBands_, 0.0f);
  spectrumPeakData_.assign(numBands_, 0.0f);

  // Updates are driven by parent (ZenithPolySynthUI)
  // if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60); 
}

void ZenithVisualizer::timerCallback() {
  // Driven externally
  // updateAudioData();
  // repaint();
}

void ZenithVisualizer::updateAudioData() {
  // Read new samples from processor's FIFO
  int needed = audioBuffer_.size();
  int read = processor_.readFromVisualizer(audioBuffer_.data(), needed);

  if (read > 0) {
    // Copy valid samples
    std::copy(audioBuffer_.begin(), audioBuffer_.begin() + read,
              displayBuffer_.begin());

    // Zero the rest if less than needed
    if (static_cast<size_t>(read) < displayBuffer_.size()) {
       std::fill(displayBuffer_.begin() + read, displayBuffer_.end(), 0.0f);
    }

    // --- FFT Processing ---
    // 1. Prepare data (copy to FFT buffer)
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    std::copy(displayBuffer_.begin(), displayBuffer_.end(), fftData_.begin());

    // 2. Apply Windowing
    window_.multiplyWithWindowingTable(fftData_.data(), fftSize_);

    // 3. Perform FFT
    forwardFFT_.performFrequencyOnlyForwardTransform(fftData_.data());

    // 4. Map FFT bins to logarithmic frequency bands
    const float sampleRate = 44100.0f; // Assume standard, or get from processor
    const float minHz = 20.0f;
    const float maxHz = 20000.0f;

    for (int i = 0; i < numBands_; ++i) {
        float fLower = minHz * std::pow(maxHz / minHz, (float)i / numBands_);
        float fUpper = minHz * std::pow(maxHz / minHz, (float)(i + 1) / numBands_);

        int binLower = static_cast<int>(fLower * fftSize_ / sampleRate);
        int binUpper = static_cast<int>(fUpper * fftSize_ / sampleRate);
        
        // Ensure at least one bin per band
        binUpper = std::max(binUpper, binLower + 1);
        binUpper = std::min(binUpper, fftSize_ / 2);

        float magnitudeSum = 0.0f;
        for (int b = binLower; b < binUpper; ++b) {
            magnitudeSum += fftData_[b];
        }
        
        float averageMagnitude = (binUpper > binLower) ? magnitudeSum / (binUpper - binLower) : 0.0f;
        
        // Logarithmic amplitude (dB-ish)
        float level = 20.0f * std::log10(averageMagnitude + 1e-6f);
        level = (level + 80.0f) / 80.0f; // Normalize -80dB..0dB to 0..1
        level = std::clamp(level, 0.0f, 1.0f);

        // Temporal smoothing
        spectrumData_[i] = spectrumData_[i] * 0.4f + level * 0.6f;
        
        // Peak hold with decay
        if (spectrumData_[i] > spectrumPeakData_[i])
            spectrumPeakData_[i] = spectrumData_[i];
        else
            spectrumPeakData_[i] *= spectrumDecaySpeed_;
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

  drawSpectrum(canvas);
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
  auto bounds = getLocalBounds().toFloat();
  float width = bounds.getWidth();
  float height = bounds.getHeight();

  float padding = 2.0f;
  float barWidth = (width / (float)numBands_) - padding;

  for (int i = 0; i < numBands_; ++i) {
    float x = i * (barWidth + padding);
    float val = spectrumData_[i];
    float peakVal = spectrumPeakData_[i];

    float barHeight = val * height;
    float peakY = height - (peakVal * height);

    // Draw main bar
    SkRect rect = SkRect::MakeXYWH(x, height - barHeight, barWidth, barHeight);
    
    SkPaint paint;
    paint.setAntiAlias(true);

    SkPoint pts[2] = {{x, height - barHeight}, {x, height}};
    SkColor colors[2] = {spectrumColorTop_, spectrumColorBottom_};
    paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));

    canvas->drawRect(rect, paint);

    // Draw peak line
    SkPaint peakPaint;
    peakPaint.setColor(spectrumColorTop_);
    peakPaint.setAlpha(180);
    peakPaint.setStrokeWidth(1.5f);
    canvas->drawLine(x, peakY, x + barWidth, peakY, peakPaint);
  }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith
