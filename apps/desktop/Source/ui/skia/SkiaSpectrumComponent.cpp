/*
  ==============================================================================

    SkiaSpectrumComponent.cpp
    Created: 2025-12-08
    Author:  Zenith DAW

    Implementation of the real-time Spectrum Analyzer.

  ==============================================================================
*/

#include "SkiaSpectrumComponent.h"
#include <cmath>

namespace zenith {

//==============================================================================
// Construction/Destruction
//==============================================================================

SkiaSpectrumComponent::SkiaSpectrumComponent(FFTSize fftSize)
    : fftSize_(static_cast<int>(fftSize))
    , fft_(static_cast<int>(std::log2(fftSize_)))
    , fftInput_(static_cast<size_t>(fftSize_ * 2), 0.0f)
    , fftOutput_(static_cast<size_t>(fftSize_ * 2), 0.0f)
    , magnitudes_(static_cast<size_t>(fftSize_ / 2), kMinDb)
    , smoothedMagnitudes_(static_cast<size_t>(fftSize_ / 2), kMinDb)
    , window_(static_cast<size_t>(fftSize_), juce::dsp::WindowingFunction<float>::hann)
    , audioFifo_(fftSize_ * 4)  // 4x FFT size for buffering
    , inputBuffer_(static_cast<size_t>(fftSize_), 0.0f)
    , peakMagnitudes_(static_cast<size_t>(fftSize_ / 2), kMinDb)
    , peakHoldCounters_(static_cast<size_t>(fftSize_ / 2), 0)
{
    // Start timer for continuous updates at 60 FPS
    startTimerHz(kTargetFPS);
    
    // Set default size
    setSize(200, 80);
}

SkiaSpectrumComponent::~SkiaSpectrumComponent()
{
    stopTimer();
}

//==============================================================================
// Timer Callback - Process FFT and trigger repaint
//==============================================================================

void SkiaSpectrumComponent::timerCallback()
{
    // Pop samples from audio FIFO and process FFT
    processFFT();
    
    // Update peak hold values
    updatePeaks();
    
    // Request repaint
    repaint();
}

//==============================================================================
// FFT Processing (UI Thread - NOT Audio Thread!)
//==============================================================================

void SkiaSpectrumComponent::processFFT()
{
    // Read available samples from FIFO
    int numAvailable = audioFifo_.getNumReady();
    
    if (numAvailable <= 0) {
        // No new samples, apply decay to smoothed magnitudes
        for (size_t i = 0; i < smoothedMagnitudes_.size(); ++i) {
            smoothedMagnitudes_[i] = smoothedMagnitudes_[i] * decaySpeed_ + kMinDb * (1.0f - decaySpeed_);
        }
        return;
    }

    // Read samples into input buffer (circular write)
    std::vector<float> tempBuffer(static_cast<size_t>(numAvailable));
    int numRead = audioFifo_.popSamples(tempBuffer.data(), numAvailable);

    // Copy to input buffer (overwrite oldest samples)
    for (int i = 0; i < numRead; ++i) {
        inputBuffer_[static_cast<size_t>(inputWritePos_)] = tempBuffer[static_cast<size_t>(i)];
        inputWritePos_ = (inputWritePos_ + 1) % fftSize_;
    }

    // Copy input buffer to FFT input (unwrap circular buffer)
    for (int i = 0; i < fftSize_; ++i) {
        int readPos = (inputWritePos_ + i) % fftSize_;
        fftInput_[static_cast<size_t>(i)] = inputBuffer_[static_cast<size_t>(readPos)];
    }

    // Apply window function
    window_.multiplyWithWindowingTable(fftInput_.data(), static_cast<size_t>(fftSize_));

    // Zero the imaginary parts
    for (int i = fftSize_; i < fftSize_ * 2; ++i) {
        fftInput_[static_cast<size_t>(i)] = 0.0f;
    }

    // Perform FFT
    fft_.performFrequencyOnlyForwardTransform(fftInput_.data());

    // Convert to magnitude (dB)
    const int numBins = fftSize_ / 2;
    for (int i = 0; i < numBins; ++i) {
        float magnitude = fftInput_[static_cast<size_t>(i)];
        
        // Convert to dB with safety check
        float db = (magnitude > 1e-10f) 
            ? juce::Decibels::gainToDecibels(magnitude) 
            : kMinDb;
        
        // Clamp to valid range
        db = juce::jlimit(kMinDb, kMaxDb, db);
        
        magnitudes_[static_cast<size_t>(i)] = db;

        // Smooth the display values (attack fast, decay slow)
        float current = smoothedMagnitudes_[static_cast<size_t>(i)];
        if (db > current) {
            // Fast attack
            smoothedMagnitudes_[static_cast<size_t>(i)] = db;
        } else {
            // Slow decay
            smoothedMagnitudes_[static_cast<size_t>(i)] = current * decaySpeed_ + db * (1.0f - decaySpeed_);
        }
    }
}

void SkiaSpectrumComponent::updatePeaks()
{
    if (!peakHoldEnabled_) return;

    const int numBins = fftSize_ / 2;
    const int holdFrames = (peakHoldTimeMs_ * kTargetFPS) / 1000;

    for (int i = 0; i < numBins; ++i) {
        float current = smoothedMagnitudes_[static_cast<size_t>(i)];
        
        if (current >= peakMagnitudes_[static_cast<size_t>(i)]) {
            // New peak
            peakMagnitudes_[static_cast<size_t>(i)] = current;
            peakHoldCounters_[static_cast<size_t>(i)] = holdFrames;
        } else if (peakHoldCounters_[static_cast<size_t>(i)] > 0) {
            // Hold peak
            peakHoldCounters_[static_cast<size_t>(i)]--;
        } else {
            // Decay peak
            peakMagnitudes_[static_cast<size_t>(i)] *= 0.95f;
            if (peakMagnitudes_[static_cast<size_t>(i)] < kMinDb) {
                peakMagnitudes_[static_cast<size_t>(i)] = kMinDb;
            }
        }
    }
}

//==============================================================================
// Frequency Mapping
//==============================================================================

float SkiaSpectrumComponent::frequencyToX(float frequency) const
{
    // Logarithmic frequency mapping
    float logMin = std::log10(minFrequency_);
    float logMax = std::log10(maxFrequency_);
    float logFreq = std::log10(std::max(frequency, minFrequency_));
    
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return normalized * static_cast<float>(getWidth());
}

float SkiaSpectrumComponent::binToFrequency(int bin) const
{
    return static_cast<float>(bin) * static_cast<float>(sampleRate_) / static_cast<float>(fftSize_);
}

//==============================================================================
// Skia Drawing
//==============================================================================

void SkiaSpectrumComponent::drawSkia(SkCanvas* canvas)
{
    if (!canvas) return;

    // Draw background
    drawBackground(canvas);

    // Draw spectrum based on display mode
    switch (displayMode_) {
        case DisplayMode::Bars:
            drawBars(canvas);
            break;
        case DisplayMode::Curve:
            drawCurve(canvas, false);
            break;
        case DisplayMode::FilledCurve:
            drawCurve(canvas, true);
            break;
    }

    // Draw peak hold indicators
    if (peakHoldEnabled_) {
        drawPeaks(canvas);
    }
}

void SkiaSpectrumComponent::drawBackground(SkCanvas* canvas)
{
    SkPaint paint;
    paint.setColor(backgroundColor_);
    paint.setAntiAlias(true);
    
    auto bounds = getLocalBounds();
    SkRect rect = SkRect::MakeXYWH(0, 0, 
        static_cast<float>(bounds.getWidth()), 
        static_cast<float>(bounds.getHeight()));
    
    canvas->drawRoundRect(rect, 4.0f, 4.0f, paint);
}

void SkiaSpectrumComponent::drawBars(SkCanvas* canvas)
{
    const int numBins = fftSize_ / 2;
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());
    
    const int numBars = 32; // Number of frequency bands to display
    const float barWidth = width / static_cast<float>(numBars) - 2.0f;

    SkPaint paint;
    paint.setAntiAlias(true);

    for (int bar = 0; bar < numBars; ++bar) {
        // Calculate frequency range for this bar (logarithmic)
        float t0 = static_cast<float>(bar) / static_cast<float>(numBars);
        float t1 = static_cast<float>(bar + 1) / static_cast<float>(numBars);
        
        float freq0 = minFrequency_ * std::pow(maxFrequency_ / minFrequency_, t0);
        float freq1 = minFrequency_ * std::pow(maxFrequency_ / minFrequency_, t1);
        
        int bin0 = static_cast<int>(freq0 * static_cast<float>(fftSize_) / static_cast<float>(sampleRate_));
        int bin1 = static_cast<int>(freq1 * static_cast<float>(fftSize_) / static_cast<float>(sampleRate_));
        
        bin0 = juce::jlimit(0, numBins - 1, bin0);
        bin1 = juce::jlimit(bin0 + 1, numBins, bin1);

        // Average magnitude for this bar
        float avgMag = kMinDb;
        for (int b = bin0; b < bin1; ++b) {
            avgMag = std::max(avgMag, smoothedMagnitudes_[static_cast<size_t>(b)]);
        }

        // Normalize to 0-1
        float normalized = (avgMag - kMinDb) / (kMaxDb - kMinDb);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        float barHeight = normalized * height;
        float x = static_cast<float>(bar) * (barWidth + 2.0f) + 1.0f;
        float y = height - barHeight;

        // Color based on level
        SkColor color;
        if (normalized > 0.8f) {
            color = gradientColorHigh_;
        } else if (normalized > 0.4f) {
            color = gradientColorMid_;
        } else {
            color = gradientColorLow_;
        }

        paint.setColor(color);
        SkRect barRect = SkRect::MakeXYWH(x, y, barWidth, barHeight);
        canvas->drawRoundRect(barRect, 2.0f, 2.0f, paint);
    }
}

void SkiaSpectrumComponent::drawCurve(SkCanvas* canvas, bool filled)
{
    const int numBins = fftSize_ / 2;
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());

    SkPath path;
    bool pathStarted = false;

    // Sample points along the frequency spectrum
    const int numPoints = 128;
    std::vector<SkPoint> points;
    points.reserve(static_cast<size_t>(numPoints));

    for (int i = 0; i < numPoints; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numPoints - 1);
        
        // Logarithmic frequency
        float freq = minFrequency_ * std::pow(maxFrequency_ / minFrequency_, t);
        int bin = static_cast<int>(freq * static_cast<float>(fftSize_) / static_cast<float>(sampleRate_));
        bin = juce::jlimit(0, numBins - 1, bin);

        float magnitude = smoothedMagnitudes_[static_cast<size_t>(bin)];
        float normalized = (magnitude - kMinDb) / (kMaxDb - kMinDb);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        float x = t * width;
        float y = height - (normalized * height);

        points.push_back(SkPoint::Make(x, y));
    }

    // Build smooth curve path
    if (!points.empty()) {
        path.moveTo(points[0]);
        
        for (size_t i = 1; i < points.size(); ++i) {
            // Use quadratic bezier for smoothness
            if (i < points.size() - 1) {
                float midX = (points[i].x() + points[i + 1].x()) / 2.0f;
                float midY = (points[i].y() + points[i + 1].y()) / 2.0f;
                path.quadTo(points[i].x(), points[i].y(), midX, midY);
            } else {
                path.lineTo(points[i]);
            }
        }
    }

    if (filled) {
        // Close path for fill
        SkPath fillPath = path;
        fillPath.lineTo(width, height);
        fillPath.lineTo(0, height);
        fillPath.close();

        // Create gradient
        SkPoint gradientPoints[2] = {
            SkPoint::Make(width / 2.0f, 0),
            SkPoint::Make(width / 2.0f, height)
        };
        SkColor gradientColors[3] = {
            gradientColorHigh_,
            gradientColorMid_,
            gradientColorLow_
        };
        float gradientPositions[3] = { 0.0f, 0.5f, 1.0f };

        SkPaint fillPaint;
        fillPaint.setAntiAlias(true);
        fillPaint.setStyle(SkPaint::kFill_Style);
        fillPaint.setShader(SkGradientShader::MakeLinear(
            gradientPoints,
            gradientColors,
            gradientPositions,
            3,
            SkTileMode::kClamp
        ));
        fillPaint.setAlphaf(0.6f);

        canvas->drawPath(fillPath, fillPaint);
    }

    // Draw stroke
    SkPaint strokePaint;
    strokePaint.setAntiAlias(true);
    strokePaint.setStyle(SkPaint::kStroke_Style);
    strokePaint.setColor(lineColor_);
    strokePaint.setStrokeWidth(lineWidth_);
    strokePaint.setStrokeCap(SkPaint::kRound_Cap);
    strokePaint.setStrokeJoin(SkPaint::kRound_Join);

    canvas->drawPath(path, strokePaint);
}

void SkiaSpectrumComponent::drawPeaks(SkCanvas* canvas)
{
    const int numBins = fftSize_ / 2;
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SkColorSetA(design::colors::TEXT_PRIMARY, 200));
    paint.setStrokeWidth(1.5f);
    paint.setStyle(SkPaint::kStroke_Style);

    SkPath peakPath;
    bool started = false;

    const int numPoints = 64;
    for (int i = 0; i < numPoints; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numPoints - 1);
        float freq = minFrequency_ * std::pow(maxFrequency_ / minFrequency_, t);
        int bin = static_cast<int>(freq * static_cast<float>(fftSize_) / static_cast<float>(sampleRate_));
        bin = juce::jlimit(0, numBins - 1, bin);

        float magnitude = peakMagnitudes_[static_cast<size_t>(bin)];
        float normalized = (magnitude - kMinDb) / (kMaxDb - kMinDb);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);

        float x = t * width;
        float y = height - (normalized * height);

        if (!started) {
            peakPath.moveTo(x, y);
            started = true;
        } else {
            peakPath.lineTo(x, y);
        }
    }

    canvas->drawPath(peakPath, paint);
}

} // namespace zenith
