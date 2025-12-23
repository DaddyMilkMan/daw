/*
  ==============================================================================

    AudioFitnessEvaluator.h
    Created: 2025-12-19
    Author:  Zenith DAW AI Team

    Real audio-based fitness evaluation for genetic preset evolution.
    Analyzes synthesized audio to score presets based on perceptual qualities.

  ==============================================================================
*/

#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>


namespace zenith {
namespace ai {

// Forward declare from PresetGeneticistAgent
enum class TargetRole;

//==============================================================================
/**
    Fitness result containing multiple quality metrics
*/
struct FitnessResult {
  float totalScore = 0.0f;      // Weighted combination (0.0 - 1.0)
  float spectralBalance = 0.0f; // How well-distributed the frequency content is
  float dynamicRange = 0.0f;    // Healthy dynamics (not clipping, not dead)
  float clarity = 0.0f;         // Presence of clear fundamental, not muddy
  float warmth = 0.0f;          // Low-mid content, pleasant harmonics
  float brightness = 0.0f;      // High frequency content
  float stereoWidth = 0.0f;     // Stereo spread
  float noiseFloor = 0.0f;      // Absence of unwanted noise

  bool isDead = false;      // True if audio is silent or clipping
  juce::String deathReason; // Why it was marked dead

  juce::String toString() const {
    return juce::String::formatted(
        "Score: %.2f (Balance: %.2f, Dynamics: %.2f, Clarity: %.2f, "
        "Warmth: %.2f, Bright: %.2f, Width: %.2f)",
        totalScore, spectralBalance, dynamicRange, clarity, warmth, brightness,
        stereoWidth);
  }
};

//==============================================================================
/**
    Configuration for fitness evaluation weights
*/
struct FitnessConfig {
  // Weight multipliers for each metric (should sum to 1.0)
  float spectralBalanceWeight = 0.20f;
  float dynamicRangeWeight = 0.20f;
  float clarityWeight = 0.15f;
  float warmthWeight = 0.15f;
  float brightnessWeight = 0.15f;
  float stereoWidthWeight = 0.10f;
  float noiseFloorWeight = 0.05f;

  // Thresholds
  float clippingThreshold = 0.99f;   // Peak above this = death
  float silenceThresholdDb = -60.0f; // Below this = silent = death
  float minDynamicRangeDb = 6.0f;    // Minimum acceptable dynamic range
};

//==============================================================================
/**
    Audio-based fitness evaluator for genetic preset evolution.

    This class analyzes short audio snippets and produces a fitness score
    based on perceptual audio qualities appropriate for the target role.

    Usage:
      AudioFitnessEvaluator evaluator;
      evaluator.setTargetRole(TargetRole::Bass);

      // Render audio from preset...
      juce::AudioBuffer<float> buffer = renderPreset(preset);

      // Evaluate
      FitnessResult result = evaluator.evaluate(buffer, sampleRate);
      if (!result.isDead) {
        preset.fitness = result.totalScore;
      }
*/
class AudioFitnessEvaluator {
public:
  AudioFitnessEvaluator();
  ~AudioFitnessEvaluator() = default;

  //============================================================================
  // Configuration
  //============================================================================

  /**
   * Set the target role for evaluation.
   * This adjusts the weight of various metrics to favor sounds
   * appropriate for the role.
   */
  void setTargetRole(TargetRole role);

  /**
   * Set custom fitness configuration.
   */
  void setConfig(const FitnessConfig &config) { config_ = config; }

  //============================================================================
  // Evaluation
  //============================================================================

  /**
   * Evaluate an audio buffer and return fitness score.
   *
   * @param buffer The audio to analyze (should be at least 100ms)
   * @param sampleRate Sample rate of the audio
   * @return FitnessResult with scores and death status
   */
  FitnessResult evaluate(const juce::AudioBuffer<float> &buffer,
                         double sampleRate);

  //============================================================================
  // Analysis Helpers (exposed for testing)
  //============================================================================

  /**
   * Check if audio is clipping.
   */
  bool isClipping(const juce::AudioBuffer<float> &buffer) const;

  /**
   * Check if audio is silent.
   */
  bool isSilent(const juce::AudioBuffer<float> &buffer) const;

  /**
   * Calculate RMS level in dB.
   */
  float calculateRmsDb(const juce::AudioBuffer<float> &buffer) const;

  /**
   * Calculate peak level (0.0 - 1.0).
   */
  float calculatePeak(const juce::AudioBuffer<float> &buffer) const;

  /**
   * Calculate dynamic range in dB.
   */
  float calculateDynamicRange(const juce::AudioBuffer<float> &buffer) const;

  /**
   * Analyze frequency spectrum and return band energies.
   * Returns 8 bands: sub, bass, low-mid, mid, high-mid, presence, brilliance,
   * air
   */
  std::array<float, 8> analyzeSpectrum(const juce::AudioBuffer<float> &buffer,
                                       double sampleRate) const;

  /**
   * Calculate stereo width (0.0 = mono, 1.0 = full stereo).
   */
  float calculateStereoWidth(const juce::AudioBuffer<float> &buffer) const;

private:
  FitnessConfig config_;
  TargetRole currentRole_;

  // Compute individual metrics
  float computeSpectralBalance(const std::array<float, 8> &bands) const;
  float computeClarity(const std::array<float, 8> &bands) const;
  float computeWarmth(const std::array<float, 8> &bands) const;
  float computeBrightness(const std::array<float, 8> &bands) const;

  // Adjust weights based on target role
  void adjustWeightsForRole(TargetRole role);
};

} // namespace ai
} // namespace zenith
