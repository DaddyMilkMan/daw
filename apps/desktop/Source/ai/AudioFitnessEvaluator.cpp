/*
  ==============================================================================

    AudioFitnessEvaluator.cpp
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Implementation of audio-based fitness evaluation.

  ==============================================================================
*/

#include "AudioFitnessEvaluator.h"
#include "PresetGeneticistAgent.h"
#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace zenith {
namespace ai {

//==============================================================================
// Constructor
//==============================================================================

AudioFitnessEvaluator::AudioFitnessEvaluator()
    : currentRole_(TargetRole::General) {}

//==============================================================================
// Configuration
//==============================================================================

void AudioFitnessEvaluator::setTargetRole(TargetRole role) {
  currentRole_ = role;
  adjustWeightsForRole(role);
}

void AudioFitnessEvaluator::adjustWeightsForRole(TargetRole role) {
  // Reset to default weights
  config_ = FitnessConfig();

  switch (role) {
  case TargetRole::Bass:
    // Bass sounds: emphasize warmth, sub frequencies, less brightness
    config_.warmthWeight = 0.30f;
    config_.spectralBalanceWeight = 0.25f;
    config_.dynamicRangeWeight = 0.20f;
    config_.clarityWeight = 0.10f;
    config_.brightnessWeight = 0.05f;
    config_.stereoWidthWeight = 0.05f; // Bass should be centered
    config_.noiseFloorWeight = 0.05f;
    break;

  case TargetRole::Lead:
    // Lead sounds: emphasize brightness, clarity, presence
    config_.brightnessWeight = 0.25f;
    config_.clarityWeight = 0.25f;
    config_.dynamicRangeWeight = 0.20f;
    config_.spectralBalanceWeight = 0.15f;
    config_.warmthWeight = 0.05f;
    config_.stereoWidthWeight = 0.05f;
    config_.noiseFloorWeight = 0.05f;
    break;

  case TargetRole::Pad:
    // Pad sounds: emphasize stereo width, warmth, spectral balance
    config_.stereoWidthWeight = 0.25f;
    config_.warmthWeight = 0.25f;
    config_.spectralBalanceWeight = 0.20f;
    config_.dynamicRangeWeight = 0.10f; // Pads can be more compressed
    config_.clarityWeight = 0.10f;
    config_.brightnessWeight = 0.05f;
    config_.noiseFloorWeight = 0.05f;
    break;

  case TargetRole::FX:
    // FX sounds: unusual spectral content is good, high dynamic range
    config_.dynamicRangeWeight = 0.30f;
    config_.spectralBalanceWeight = 0.10f; // Don't penalize weird spectra
    config_.clarityWeight = 0.10f;
    config_.warmthWeight = 0.10f;
    config_.brightnessWeight = 0.20f;
    config_.stereoWidthWeight = 0.15f;
    config_.noiseFloorWeight = 0.05f;
    break;

  case TargetRole::General:
  default:
    // Balanced defaults (already set by FitnessConfig())
    break;
  }
}

//==============================================================================
// Main Evaluation
//==============================================================================

FitnessResult
AudioFitnessEvaluator::evaluate(const juce::AudioBuffer<float> &buffer,
                                double sampleRate) {
  FitnessResult result;

  // Check for death conditions first
  if (isSilent(buffer)) {
    result.isDead = true;
    result.deathReason = "Silent audio (below " +
                         juce::String(config_.silenceThresholdDb) + " dB)";
    result.totalScore = 0.0f;
    return result;
  }

  if (isClipping(buffer)) {
    result.isDead = true;
    result.deathReason = "Clipping detected (peak > " +
                         juce::String(config_.clippingThreshold) + ")";
    result.totalScore = 0.0f;
    return result;
  }

  // Calculate metrics
  float dynamicRangeDb = calculateDynamicRange(buffer);
  if (dynamicRangeDb < config_.minDynamicRangeDb) {
    result.isDead = true;
    result.deathReason = "Insufficient dynamic range (" +
                         juce::String(dynamicRangeDb, 1) + " dB)";
    result.totalScore = 0.0f;
    return result;
  }

  // Analyze spectrum
  auto bands = analyzeSpectrum(buffer, sampleRate);

  // Compute individual metrics (all 0.0 - 1.0)
  result.spectralBalance = computeSpectralBalance(bands);
  result.dynamicRange = juce::jlimit(
      0.0f, 1.0f,
      (dynamicRangeDb - config_.minDynamicRangeDb) / 30.0f); // Normalize to 0-1
  result.clarity = computeClarity(bands);
  result.warmth = computeWarmth(bands);
  result.brightness = computeBrightness(bands);
  result.stereoWidth = calculateStereoWidth(buffer);
  result.noiseFloor =
      1.0f - juce::jlimit(0.0f, 1.0f,
                          (calculateRmsDb(buffer) + 60.0f) /
                              60.0f); // Invert: quieter = less noise

  // Calculate weighted total
  result.totalScore = (result.spectralBalance * config_.spectralBalanceWeight +
                       result.dynamicRange * config_.dynamicRangeWeight +
                       result.clarity * config_.clarityWeight +
                       result.warmth * config_.warmthWeight +
                       result.brightness * config_.brightnessWeight +
                       result.stereoWidth * config_.stereoWidthWeight +
                       result.noiseFloor * config_.noiseFloorWeight);

  // Clamp to valid range
  result.totalScore = juce::jlimit(0.0f, 1.0f, result.totalScore);

  return result;
}

//==============================================================================
// Analysis Helpers
//==============================================================================

bool AudioFitnessEvaluator::isClipping(
    const juce::AudioBuffer<float> &buffer) const {
  return calculatePeak(buffer) >= config_.clippingThreshold;
}

bool AudioFitnessEvaluator::isSilent(
    const juce::AudioBuffer<float> &buffer) const {
  return calculateRmsDb(buffer) < config_.silenceThresholdDb;
}

float AudioFitnessEvaluator::calculateRmsDb(
    const juce::AudioBuffer<float> &buffer) const {
  float sumSquares = 0.0f;
  int totalSamples = 0;

  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    const float *data = buffer.getReadPointer(ch);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      sumSquares += data[i] * data[i];
      totalSamples++;
    }
  }

  if (totalSamples == 0)
    return -100.0f;

  float rms = std::sqrt(sumSquares / totalSamples);
  if (rms <= 0.0f)
    return -100.0f;

  return 20.0f * std::log10(rms);
}

float AudioFitnessEvaluator::calculatePeak(
    const juce::AudioBuffer<float> &buffer) const {
  float peak = 0.0f;

  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    const float *data = buffer.getReadPointer(ch);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      peak = std::max(peak, std::abs(data[i]));
    }
  }

  return peak;
}

float AudioFitnessEvaluator::calculateDynamicRange(
    const juce::AudioBuffer<float> &buffer) const {
  // Simple crest factor approach: Peak dB - RMS dB
  float peakDb = 20.0f * std::log10(std::max(0.0001f, calculatePeak(buffer)));
  float rmsDb = calculateRmsDb(buffer);

  return peakDb - rmsDb;
}

std::array<float, 8>
AudioFitnessEvaluator::analyzeSpectrum(const juce::AudioBuffer<float> &buffer,
                                       double sampleRate) const {
  std::array<float, 8> bands = {0.0f};

  const int numSamples = buffer.getNumSamples();
  if (numSamples < 512)
    return bands;

  // Use 1024 points for FFT
  const int fftOrder = 10;
  const int fftSize = 1 << fftOrder;
  juce::dsp::FFT fft(fftOrder);
  juce::dsp::WindowingFunction<float> window(
      fftSize, juce::dsp::WindowingFunction<float>::hann);

  std::vector<float> avgMagnitudes(fftSize / 2, 0.0f);
  const float *data = buffer.getReadPointer(0); // Analyze mono/left channel

  int numWindows = 0;
  for (int pos = 0; pos + fftSize <= numSamples; pos += fftSize / 2) {
    std::vector<float> fftData(fftSize * 2, 0.0f);
    for (int i = 0; i < fftSize; ++i)
      fftData[i] = data[pos + i];

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    for (int i = 0; i < fftSize / 2; ++i)
      avgMagnitudes[i] += fftData[i];

    numWindows++;
  }

  if (numWindows == 0)
    return bands;

  // Average magnitudes
  for (int i = 0; i < fftSize / 2; ++i)
    avgMagnitudes[i] /= static_cast<float>(numWindows);

  // Frequency band boundaries (Hz)
  // Sub: 20-60, Bass: 60-250, Low-Mid: 250-500, Mid: 500-2k
  // High-Mid: 2k-4k, Presence: 4k-6k, Brilliance: 6k-10k, Air: 10k-20k
  constexpr std::array<float, 9> freqBounds = {20.0f,   60.0f,    250.0f,
                                               500.0f,  2000.0f,  4000.0f,
                                               6000.0f, 10000.0f, 20000.0f};

  float binWidth = static_cast<float>(sampleRate) / fftSize;

  for (int b = 0; b < 8; ++b) {
    float lowFreq = freqBounds[b];
    float highFreq = freqBounds[b + 1];

    int flow = static_cast<int>(std::floor(lowFreq / binWidth));
    int fhigh = static_cast<int>(std::ceil(highFreq / binWidth));

    flow = juce::jlimit(0, fftSize / 2 - 1, flow);
    fhigh = juce::jlimit(0, fftSize / 2 - 1, fhigh);

    float energy = 0.0f;
    for (int i = flow; i <= fhigh; ++i) {
      energy += avgMagnitudes[i];
    }

    bands[b] = energy / std::max(1, fhigh - flow + 1);
  }

  // Normalize bands
  float maxBand = 0.0001f;
  for (float b : bands) {
    maxBand = std::max(maxBand, b);
  }
  for (float &b : bands) {
    b /= maxBand;
  }

  return bands;
}

float AudioFitnessEvaluator::calculateStereoWidth(
    const juce::AudioBuffer<float> &buffer) const {
  if (buffer.getNumChannels() < 2) {
    return 0.0f; // Mono = no width
  }

  const float *left = buffer.getReadPointer(0);
  const float *right = buffer.getReadPointer(1);
  int numSamples = buffer.getNumSamples();

  // Mid-Side analysis
  float midEnergy = 0.0f;
  float sideEnergy = 0.0f;

  for (int i = 0; i < numSamples; ++i) {
    float mid = (left[i] + right[i]) * 0.5f;
    float side = (left[i] - right[i]) * 0.5f;

    midEnergy += mid * mid;
    sideEnergy += side * side;
  }

  // Width = ratio of side to total
  float totalEnergy = midEnergy + sideEnergy;
  if (totalEnergy < 0.0001f)
    return 0.0f;

  return sideEnergy / totalEnergy;
}

//==============================================================================
// Metric Computation
//==============================================================================

float AudioFitnessEvaluator::computeSpectralBalance(
    const std::array<float, 8> &bands) const {
  // Good spectral balance = energy distributed across bands
  // Calculate variance of band energies

  float mean = 0.0f;
  for (float b : bands) {
    mean += b;
  }
  mean /= 8.0f;

  float variance = 0.0f;
  for (float b : bands) {
    float diff = b - mean;
    variance += diff * diff;
  }
  variance /= 8.0f;

  // Lower variance = better balance = higher score
  // Normalize: variance of 0.25 (max possible) = 0, variance of 0 = 1
  return 1.0f - juce::jlimit(0.0f, 1.0f, variance * 4.0f);
}

float AudioFitnessEvaluator::computeClarity(
    const std::array<float, 8> &bands) const {
  // Clarity = presence of mid and high-mid frequencies
  // Bands: 3 = Mid (500-2k), 4 = High-Mid (2k-4k), 5 = Presence (4k-6k)

  float clarityBands = bands[3] + bands[4] + bands[5];
  return juce::jlimit(0.0f, 1.0f, clarityBands / 2.0f);
}

float AudioFitnessEvaluator::computeWarmth(
    const std::array<float, 8> &bands) const {
  // Warmth = presence of bass and low-mid frequencies
  // Bands: 0 = Sub, 1 = Bass, 2 = Low-Mid

  float warmBands = bands[0] * 0.5f + bands[1] + bands[2];
  return juce::jlimit(0.0f, 1.0f, warmBands / 2.0f);
}

float AudioFitnessEvaluator::computeBrightness(
    const std::array<float, 8> &bands) const {
  // Brightness = presence of high frequencies
  // Bands: 5 = Presence, 6 = Brilliance, 7 = Air

  float brightBands = bands[5] + bands[6] + bands[7] * 0.5f;
  return juce::jlimit(0.0f, 1.0f, brightBands / 2.0f);
}

} // namespace ai
} // namespace zenith
