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

    SkiaSpectrumComponent.cpp
    Created: 2025-12-08
    Author:  Zenith DAW

  ==============================================================================
*/


#include "SkiaSpectrumComponent.h"
#include <effects/SkGradientShader.h>

namespace zenith {

SkiaSpectrumComponent::SkiaSpectrumComponent(FFTSize fftSize)
    : forwardFFT_((int)fftSize),
      window_((size_t)(1 << (int)fftSize), juce::dsp::WindowingFunction<float>::hann),
      audioFifo_(4096) // Larger buffer to handle jitter
{
    // Set target FPS to 60 for smooth visualization
    setTargetFPS(60);
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60);
    
    int numPoints = 1 << (int)fftSize;
    fftData_.assign(numPoints * 2, 0.0f);
    scopeData_.assign(numPoints, 0.0f);
    
    // Set up visualization buckets (logarithmic)
    // 64 bands is usually enough for visual analysis
    logFrequencyData_.assign(64, 0.0f);
    peakData_.assign(64, 0.0f);
    peakHoldCounters_.assign(64, 0);
}

SkiaSpectrumComponent::~SkiaSpectrumComponent() {
    stopTimer();
}

void SkiaSpectrumComponent::pushSamples(const juce::AudioBuffer<float>& buffer) {
    audioFifo_.pushStereoAsMono(buffer, buffer.getNumSamples());
}

void SkiaSpectrumComponent::timerCallback() {
    // Process audio data if enough samples available
    if (audioFifo_.getNumReady() >= (int)scopeData_.size()) {
        processFFT();
        updateVisualData();
        markDirty(); // Request repaint
    } else {
        // Fallback decay if no audio
        bool changed = false;
        for (auto& val : logFrequencyData_) {
            if (val > 0.001f) {
                val *= decaySpeed_;
                changed = true;
            }
        }
        if (changed) {
            updateVisualData(); // Process peaks
            markDirty();
        }
    }
}

void SkiaSpectrumComponent::processFFT() {
    // Pull from FIFO
    audioFifo_.pop(scopeData_);
    
    // Apply windowing
    window_.multiplyWithWindowingTable(scopeData_.data(), scopeData_.size());
    
    // Prepare for FFT (copy to complex buffer)
    std::fill(fftData_.begin(), fftData_.end(), 0.0f);
    std::copy(scopeData_.begin(), scopeData_.end(), fftData_.begin());
    
    // Perform FFT
    forwardFFT_.performFrequencyOnlyForwardTransform(fftData_.data());
    
    // fftData_ now holds magnitude in first half
}

void SkiaSpectrumComponent::updateVisualData() {
    int fftSize = (int)scopeData_.size();
    int numBins = fftSize / 2;
    int numBuckets = (int)logFrequencyData_.size();
    
    // Map FFT bins to logarithmic buckets
    // Simple mapping: 20Hz to 20kHz
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    
    for (int i = 0; i < numBuckets; ++i) {
        float normX = (float)i / (float)numBuckets;
        
        // Logarithmic frequency
        float freqStart = minFreq * std::pow(maxFreq / minFreq, normX);
        float freqEnd = minFreq * std::pow(maxFreq / minFreq, (float)(i + 1) / numBuckets);
        
        // Find corresponding bins
        int binStart = getIndexForFrequency(freqStart);
        int binEnd = getIndexForFrequency(freqEnd);
        
        // Average or Max magnitude in this range
        float maxMag = 0.0f;
        int count = 0;
        
        for (int bin = binStart; bin <= binEnd && bin < numBins; ++bin) {
            float mag = fftData_[bin];
            if (mag > maxMag) maxMag = mag;
            count++;
        }
        
        // Normalize magnitude (semi-logarithmic amplitude)
        // FFT output depends on window size.
        // Approx scaling for visual range.
        float rawLevel = maxMag / (float)fftSize; 
        float db = juce::Decibels::gainToDecibels(rawLevel + 0.00001f);
        float normLevel = juce::jmap(db, -100.0f, 0.0f, 0.0f, 1.0f);
        normLevel = juce::jlimit(0.0f, 1.0f, normLevel);
        
        // Smooth transitions
        float current = logFrequencyData_[i];
        if (normLevel > current) {
            logFrequencyData_[i] = normLevel; // Attack instant
        } else {
            logFrequencyData_[i] = current * decaySpeed_; // Decay
        }
        
        // Peak hold
        if (logFrequencyData_[i] > peakData_[i]) {
            peakData_[i] = logFrequencyData_[i];
            peakHoldCounters_[i] = peakHoldTime_;
        } else {
            if (peakHoldCounters_[i] > 0) {
                peakHoldCounters_[i]--;
            } else {
                peakData_[i] *= 0.98f; // Slow decay
            }
        }
    }
}

int SkiaSpectrumComponent::getIndexForFrequency(float freq) const {
    // bin = freq * size / sampleRate
    return (int)(freq * (float)scopeData_.size() / sampleRate_);
}

void SkiaSpectrumComponent::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
    
    int numBuckets = (int)logFrequencyData_.size();
    float binWidth = rect.width() / (float)numBuckets;
    
    if (displayMode_ == DisplayMode::Bars) {
        SkPaint paint;
        paint.setColor(colorTop_);
        paint.setAntiAlias(false); // Sharp bars
        
        for (int i = 0; i < numBuckets; ++i) {
            float height = logFrequencyData_[i] * rect.height();
            if (height < 1.0f) height = 1.0f; // Minimum visibility
            
            SkRect bar = SkRect::MakeXYWH(
                rect.x() + i * binWidth,
                rect.bottom() - height,
                binWidth - 1.0f, // 1px gap
                height
            );
            canvas->drawRect(bar, paint);
        }
    } 
    else if (displayMode_ == DisplayMode::FilledCurve) {
        SkPath path;
        path.moveTo(rect.x(), rect.bottom());
        
        for (int i = 0; i < numBuckets; ++i) {
            float x = rect.x() + i * binWidth + (binWidth * 0.5f);
            float height = logFrequencyData_[i] * rect.height();
            float y = rect.bottom() - height;
            
            // Simple line to point
            path.lineTo(x, y);
        }
        
        path.lineTo(rect.right(), rect.bottom());
        path.close();
        
        // Gradient fill
        SkPoint pts[2] = { {rect.centerX(), rect.top()}, {rect.centerX(), rect.bottom()} };
        SkColor colors[2] = { colorTop_, colorBottom_ };
        auto shader = SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp);
        
        SkPaint paint;
        paint.setShader(shader);
        paint.setAntiAlias(true);
        canvas->drawPath(path, paint);
        
        // Stroke on top
        SkPaint stroke;
        stroke.setColor(colorTop_);
        stroke.setStyle(SkPaint::kStroke_Style);
        stroke.setStrokeWidth(1.5f);
        stroke.setAntiAlias(true);
        
        // Rebuild path for stroke (no close/bottom)
        SkPath strokePath;
        strokePath.moveTo(rect.x(), rect.bottom()); // Start left bottom? or first point?
        
        // Improved: Curve smoothing could be added here later (Catmull-Rom)
        for (int i = 0; i < numBuckets; ++i) {
            float x = rect.x() + i * binWidth + (binWidth * 0.5f);
            float height = logFrequencyData_[i] * rect.height();
            float y = rect.bottom() - height;
            if (i==0) strokePath.moveTo(x, y);
            else strokePath.lineTo(x, y);
        }
        canvas->drawPath(strokePath, stroke);
    }
    
    // Draw Peak Hold
    SkPaint peakPaint;
    peakPaint.setColor(SkColorSetARGB(180, 255, 255, 255));
    peakPaint.setAntiAlias(true);
    
    if (displayMode_ == DisplayMode::Bars) {
        for (int i = 0; i < numBuckets; ++i) {
            float height = peakData_[i] * rect.height();
            SkRect bar = SkRect::MakeXYWH(
                rect.x() + i * binWidth,
                rect.bottom() - height,
                binWidth - 1.0f,
                2.0f // Thick line
            );
            canvas->drawRect(bar, peakPaint);
        }
    }
}

void SkiaSpectrumComponent::setGradientColors(SkColor top, SkColor bottom) {
    colorTop_ = top;
    colorBottom_ = bottom;
    markDirty();
}

void SkiaSpectrumComponent::setDecaySpeed(float speed) {
    decaySpeed_ = juce::jlimit(0.6f, 0.99f, speed);
}

void SkiaSpectrumComponent::resized() {
    // Rebuild cache if needed
}

} // namespace zenith
