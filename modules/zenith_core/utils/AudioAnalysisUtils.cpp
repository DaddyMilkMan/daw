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

    AudioAnalysisUtils.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/


#include "AudioAnalysisUtils.h"
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>

namespace zenith {

double AudioAnalysisUtils::detectBpm(const juce::AudioBuffer<float> &buffer, double sampleRate) {
  if (buffer.getNumSamples() == 0 || sampleRate <= 0.0) return 0.0;

  // 1. Compute Energy Envelope
  // Downsample to simpler rate for analysis (e.g., ~100Hz is roughly enough for beat detection envelopes, 
  // but let's go with 4410Hz (downsample by 10) for better resolution if input is 44.1k)
  const int downsampleFactor = 10; 
  const int stepSize = downsampleFactor; 
  
  std::vector<float> envelope = computeEnvelope(buffer, stepSize);
  double envelopeSampleRate = sampleRate / stepSize;

  // 2. Autocorrelation over plausible BPM range
  // Range: 60 BPM to 200 BPM
  // Lag in samples = (60 / BPM) * envelopeSampleRate
  
  int minBpm = 60;
  int maxBpm = 200;

  int minLag = static_cast<int>((60.0 / maxBpm) * envelopeSampleRate);
  int maxLag = static_cast<int>((60.0 / minBpm) * envelopeSampleRate);
  
  // Ensure lags are within envelope bounds
  if (maxLag >= envelope.size()) {
      return 0.0; // Audio too short
  }

  std::vector<float> correlations;
  correlations.reserve(maxLag - minLag + 1);

  // Naive autocorrelation (sufficient for this purpose)
  // Optimization: use FFT for faster correlation if meaningful, but for short clips loop is fine.
  float bestCorrelation = 0.0f;
  int bestLag = 0;

  // Simple normalization factor
  // To verify beats, we check multiple harmonics usually, but let's stick to finding the strongest peak.
  
  for (int lag = minLag; lag <= maxLag; ++lag) {
    float corr = computeAutocorrelation(envelope, lag);
    if (corr > bestCorrelation) {
      bestCorrelation = corr;
      bestLag = lag;
    }
  }

  if (bestLag == 0) return 0.0;

  // 3. Convert best Lag to BPM
  double detectedBpm = 60.0 * envelopeSampleRate / bestLag;
  
  // Sanity check refinement
  // If we detected 180 but it might be 90, checking harmonics is complex.
  // We will return the raw detected peak for now.
  
  return detectedBpm;
}

std::vector<float> AudioAnalysisUtils::computeEnvelope(const juce::AudioBuffer<float>& buffer, int stepSize) {
  int numSamples = buffer.getNumSamples();
  int numChannels = buffer.getNumChannels();
  int envelopeSize = numSamples / stepSize;
  std::vector<float> envelope(envelopeSize, 0.0f);

  auto* readPointers = buffer.getArrayOfReadPointers();

  for (int i = 0; i < envelopeSize; ++i) {
    float sumEng = 0.0f;
    int start = i * stepSize;
    int end = std::min(start + stepSize, numSamples);
    
    for (int ch = 0; ch < numChannels; ++ch) {
      for (int s = start; s < end; ++s) {
        float sample = readPointers[ch][s];
        sumEng += sample * sample;
      }
    }
    // RMS-like
    envelope[i] = std::sqrt(sumEng / (numChannels * (end - start)));
  }
  
  return envelope;
}

float AudioAnalysisUtils::computeAutocorrelation(const std::vector<float>& signal, int lag) {
  // Pearson correlation or simple dot product
  // Let's use simple dot product of overlapping regions
  
  size_t N = signal.size();
  if (lag >= static_cast<int>(N)) return 0.0f;

  size_t count = N - lag;
  float sum = 0.0f;

  for (size_t i = 0; i < count; ++i) {
    sum += signal[i] * signal[i + lag];
  }
  
  return sum; // Unnormalized is fine for peak finding
}

} // namespace zenith
