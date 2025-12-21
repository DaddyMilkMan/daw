/*
  ==============================================================================

    ProCompressor.h
    Created with MixerChannel refactor (2025-12-20)
    Author:  Zenith DAW

    Professional-grade compressor with RMS detection, lookahead, and soft knee.

  ==============================================================================
*/

#pragma once

#include <cmath>
#include <atomic>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "AudioConstants.h"

namespace zenith {

//==============================================================================
/**
    Professional-grade compressor with RMS detection, lookahead, and soft knee.
*/
class ProCompressor {
public:
  ProCompressor() = default;

  void prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;

    // Lookahead buffer (5ms - use constant)
    lookaheadSamples_ =
        static_cast<int>(sampleRate * constants::kCompLookaheadMs / 1000.0);
    lookaheadBuffer_.setSize(2, lookaheadSamples_ + maxBlockSize);
    lookaheadBuffer_.clear();
    lookaheadWritePos_ = 0;

    // RMS buffer (10ms window - use constant)
    rmsWindowSamples_ =
        static_cast<int>(sampleRate * constants::kCompRmsWindowMs / 1000.0);
    rmsBuffer_.resize(rmsWindowSamples_, 0.0f);
    rmsWritePos_ = 0;
    rmsSum_ = 0.0f;

    // Initialize envelopes
    envL_ = 0.0f;
    envR_ = 0.0f;
    gainSmooth_ = 1.0f;

    updateCoefficients();
  }

  void reset() {
    lookaheadBuffer_.clear();
    lookaheadWritePos_ = 0;
    std::fill(rmsBuffer_.begin(), rmsBuffer_.end(), 0.0f);
    rmsWritePos_ = 0;
    rmsSum_ = 0.0f;
    envL_ = 0.0f;
    envR_ = 0.0f;
    gainSmooth_ = 1.0f;
  }

  void setThreshold(float thresholdDb) {
    threshold_ = thresholdDb;
    updateAutoMakeup();
  }
  void setRatio(float ratio) {
    ratio_ = ratio;
    updateAutoMakeup();
  }
  void setAttack(float attackMs) {
    attackMs_ = attackMs;
    updateCoefficients();
  }
  void setRelease(float releaseMs) {
    releaseMs_ = releaseMs;
    updateCoefficients();
  }
  void setMakeup(float makeupDb) {
    makeup_ = makeupDb;
    autoMakeupEnabled_ = false;
  }
  void setKnee(float kneeDb) { knee_ = kneeDb; }
  void setAutoMakeup(bool enabled) {
    autoMakeupEnabled_ = enabled;
    if (enabled)
      updateAutoMakeup();
  }
  void setLookaheadEnabled(bool enabled) { lookaheadEnabled_ = enabled; }
  void setRmsEnabled(bool enabled) { useRms_ = enabled; }

  float getGainReduction() const { return gainReduction_.load(); }

  void process(juce::AudioBuffer<float> &buffer) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels < 1)
      return;

    float maxGR = 0.0f;

    for (int i = 0; i < numSamples; ++i) {
      // Get input samples
      float inputL = buffer.getSample(0, i);
      float inputR = (numChannels >= 2) ? buffer.getSample(1, i) : inputL;

      // Compute detector signal (summed mono)
      float detector = (std::abs(inputL) + std::abs(inputR)) * 0.5f;

      // RMS or Peak detection
      float level;
      if (useRms_) {
        // Update RMS buffer
        float oldValue = rmsBuffer_[rmsWritePos_];
        rmsSum_ -= oldValue * oldValue;
        rmsSum_ += detector * detector;
        rmsBuffer_[rmsWritePos_] = detector;
        rmsWritePos_ = (rmsWritePos_ + 1) % rmsWindowSamples_;

        // RMS value
        level = std::sqrt(rmsSum_ / static_cast<float>(rmsWindowSamples_));
      } else {
        level = detector;
      }

      // Convert to dB
      float levelDb = juce::Decibels::gainToDecibels(level, -100.0f);

      // Compute gain reduction with soft knee
      float gr = computeGainReduction(levelDb);

      // Smooth the gain change (envelope follower)
      if (gr < gainSmooth_) {
        gainSmooth_ += attackCoeff_ * (gr - gainSmooth_);
      } else {
        gainSmooth_ += releaseCoeff_ * (gr - gainSmooth_);
      }

      // Track max gain reduction for metering
      float grDb = juce::Decibels::gainToDecibels(gainSmooth_, -100.0f);
      if (-grDb > maxGR)
        maxGR = -grDb;

      // Apply makeup gain
      float totalGain =
          gainSmooth_ * juce::Decibels::decibelsToGain(
                            autoMakeupEnabled_ ? autoMakeup_ : makeup_);

      // Apply with lookahead
      if (lookaheadEnabled_ && lookaheadSamples_ > 0) {
        // Write current samples to lookahead buffer
        lookaheadBuffer_.setSample(0, lookaheadWritePos_, inputL);
        if (numChannels >= 2) {
          lookaheadBuffer_.setSample(1, lookaheadWritePos_, inputR);
        }

        // Read delayed samples
        int readPos = (lookaheadWritePos_ - lookaheadSamples_ +
                       lookaheadBuffer_.getNumSamples()) %
                      lookaheadBuffer_.getNumSamples();
        float delayedL = lookaheadBuffer_.getSample(0, readPos);
        float delayedR = (numChannels >= 2)
                             ? lookaheadBuffer_.getSample(1, readPos)
                             : delayedL;

        lookaheadWritePos_ =
            (lookaheadWritePos_ + 1) % lookaheadBuffer_.getNumSamples();

        buffer.setSample(0, i, delayedL * totalGain);
        if (numChannels >= 2) {
          buffer.setSample(1, i, delayedR * totalGain);
        }
      } else {
        buffer.setSample(0, i, inputL * totalGain);
        if (numChannels >= 2) {
          buffer.setSample(1, i, inputR * totalGain);
        }
      }
    }

    gainReduction_.store(maxGR);
  }

private:
  double sampleRate_ = constants::kDefaultSampleRate;

  // Parameters (initialized from EngineConstants)
  float threshold_ = constants::kDefaultCompThresholdDb;
  float ratio_ = constants::kDefaultCompRatio;
  float attackMs_ = constants::kDefaultCompAttackMs;
  float releaseMs_ = constants::kDefaultCompReleaseMs;
  float makeup_ = 0.0f;
  float knee_ = 6.0f; // Soft knee width in dB
  float autoMakeup_ = 0.0f;
  bool autoMakeupEnabled_ = false;
  bool lookaheadEnabled_ = true;
  bool useRms_ = true;

  // State
  float attackCoeff_ = 0.0f;
  float releaseCoeff_ = 0.0f;
  float envL_ = 0.0f;
  float envR_ = 0.0f;
  float gainSmooth_ = 1.0f;
  std::atomic<float> gainReduction_{0.0f};

  // Lookahead
  juce::AudioBuffer<float> lookaheadBuffer_;
  int lookaheadSamples_ = 0;
  int lookaheadWritePos_ = 0;

  // RMS detection
  std::vector<float> rmsBuffer_;
  int rmsWindowSamples_ = 0;
  int rmsWritePos_ = 0;
  float rmsSum_ = 0.0f;

  void updateCoefficients() {
    if (sampleRate_ <= 0)
      return;
    attackCoeff_ = 1.0f - std::exp(-1.0f / (attackMs_ * 0.001f *
                                            static_cast<float>(sampleRate_)));
    releaseCoeff_ = 1.0f - std::exp(-1.0f / (releaseMs_ * 0.001f *
                                             static_cast<float>(sampleRate_)));
  }

  void updateAutoMakeup() {
    // Calculate auto makeup based on threshold and ratio
    // Approximates the average gain reduction at -20dB input
    float overshoot = -20.0f - threshold_;
    if (overshoot > 0.0f && ratio_ > 1.0f) {
      autoMakeup_ = overshoot * (1.0f - 1.0f / ratio_) * 0.5f;
    } else {
      autoMakeup_ = 0.0f;
    }
  }

  float computeGainReduction(float inputDb) const {
    // Soft knee implementation
    float halfKnee = knee_ * 0.5f;
    float output;

    if (inputDb < threshold_ - halfKnee) {
      // Below knee - no compression
      output = inputDb;
    } else if (inputDb > threshold_ + halfKnee) {
      // Above knee - full compression
      output = threshold_ + (inputDb - threshold_) / ratio_;
    } else {
      // In knee region - smooth transition
      float x = inputDb - threshold_ + halfKnee;
      float kneeGain = (1.0f / ratio_ - 1.0f) / (2.0f * knee_);
      output = inputDb + kneeGain * x * x;
    }

    float gr = output - inputDb;
    return juce::Decibels::decibelsToGain(gr);
  }
};

} // namespace zenith
