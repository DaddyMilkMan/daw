/*
  ==============================================================================

    ZenithEffects.cpp
    Refactored: 2025-12-21 (Pro Audio Upgrade)
    Author:  Zenith DAW

    Implementation of ZenithEffects with High-Fidelity DSP.
    - Interpolated Chorus (Lush)
    - Interpolated Ping-Pong Delay (Smooth)
    - True Stereo Reverb (Wide)

  ==============================================================================
*/

#include "ZenithEffects.h"
#include <algorithm>
#include <cmath>


namespace zenith {

static float getDelaySecondsForSyncRate(SyncRate rate, double bpm) {
  if (bpm <= 0.0)
    bpm = 120.0;
  double beats = 0.25;
  switch (rate) {
  case SyncRate::_1_64:
    beats = 0.0625;
    break;
  case SyncRate::_1_32:
    beats = 0.125;
    break;
  case SyncRate::_1_16:
    beats = 0.25;
    break;
  case SyncRate::_1_8:
    beats = 0.5;
    break;
  case SyncRate::_1_4:
    beats = 1.0;
    break;
  case SyncRate::_1_2:
    beats = 2.0;
    break;
  case SyncRate::_1_1:
    beats = 4.0;
    break;
  case SyncRate::_2_1:
    beats = 8.0;
    break;
  case SyncRate::_4_1:
    beats = 16.0;
    break;
  default:
    beats = 1.0;
    break;
  }
  return static_cast<float>(beats * (60.0 / bpm));
}

void ZenithEffects::setSampleRate(double sampleRate) {
  sampleRate_ = sampleRate;
  initDelay();
  reverbInit_ = false;
  initReverb();
}

void ZenithEffects::initDelay() {
  // Pre-allocate 2 seconds of buffer
  size_t size = static_cast<size_t>(sampleRate_ * 2.0);
  if (size == 0)
    size = 88200; // Fallback

  // Only resize if significantly different to avoid thrashing
  if (echoBufferL_.size() != size) {
    echoBufferL_.resize(size, 0.0f);
    echoBufferR_.resize(size, 0.0f);
  }
  std::fill(echoBufferL_.begin(), echoBufferL_.end(), 0.0f);
  std::fill(echoBufferR_.begin(), echoBufferR_.end(), 0.0f);
}

void ZenithEffects::initReverb() {
  if (reverbInit_)
    return;

  float scale = static_cast<float>(sampleRate_) / 44100.0f;

  // Standard Schroeder/Freeverb tunings
  int tunings[] = {1116, 1188, 1277, 1356};

  // Initialize Left Channel
  for (int i = 0; i < 4; ++i) {
    combsL_[i].resize(static_cast<int>(tunings[i] * scale));
    // Stereo Spread: Right channel has offset (+23 samples is standard trick)
    combsR_[i].resize(static_cast<int>((tunings[i] + 23) * scale));
  }

  int allpassTunings[] = {225, 556};
  for (int i = 0; i < 2; ++i) {
    allpassesL_[i].resize(static_cast<int>(allpassTunings[i] * scale));
    allpassesR_[i].resize(static_cast<int>((allpassTunings[i] + 23) * scale));
  }

  reverbInit_ = true;
}

void ZenithEffects::process(float &left, float &right) {
  // 1. Distortion (Waveshaping)
  // ---------------------------------------------------------
  if (distortionAmount_ > 0.0f) {
    float drive = 1.0f + distortionAmount_ * 9.0f;
    // Simple tanh soft clipping (could be upgraded to oversampled folding later)
    left = std::tanh(left * drive) / drive;
    right = std::tanh(right * drive) / drive;
  }

  // 2. Chorus (Interpolated Delay Line)
  // ---------------------------------------------------------
  if (chorusAmount_ > 0.0f) {
    // Calculate LFO
    float lfoValue = std::sin(chorusPhase_ * juce::MathConstants<float>::twoPi);

    // Modulate delay time (5ms base +/- 3ms depth)
    // Using fractional delay for smooth modulation (no zipper noise)
    float delaySamples = (5.0f + lfoValue * 3.0f) * (static_cast<float>(sampleRate_) / 1000.0f);

    // Read Position (Floating Point)
    float readPos = static_cast<float>(delayPos_) - delaySamples;

    // Stereo Spread: Right channel uses inverted LFO for width
    // Or just offset phase. Here we use the same delay buffer but same read pos logic?
    // Wait, original code read L/R from same pos.
    // To make it wider, let's invert the LFO for Right channel slightly if we had 2 LFOs.
    // But sticking to original logic + interpolation for fidelity:

    float chorusL = getInterpolatedSample(delayBufferL_, readPos);
    float chorusR = getInterpolatedSample(delayBufferR_, readPos);

    left = left * (1.0f - chorusAmount_) + chorusL * chorusAmount_;
    right = right * (1.0f - chorusAmount_) + chorusR * chorusAmount_;

    // Write input to delay line
    delayBufferL_[delayPos_] = left;
    delayBufferR_[delayPos_] = right;

    // Advance write head
    delayPos_ = (delayPos_ + 1) % delayBufferL_.size();

    // Advance LFO
    chorusPhase_ += 1.0f / static_cast<float>(sampleRate_); // 1Hz approx for now?
    // Original was: chorusPhase_ += 2.0f / ... (2Hz)
    // Let's stick to 2Hz or make it parameterizable later.
    chorusPhase_ += 2.0f / static_cast<float>(sampleRate_);
    if (chorusPhase_ >= 1.0f)
      chorusPhase_ -= 1.0f;
  }

  // 3. Stereo Ping-Pong Delay (Interpolated)
  // ---------------------------------------------------------
  if (delayMix_ > 0.0f && !echoBufferL_.empty()) {
    float time = delaySync_ ? getDelaySecondsForSyncRate(delaySyncRate_, bpm_)
                            : delayTime_;
    float delaySamples = time * sampleRate_;

    // Clamp to buffer size
    delaySamples = std::max(1.0f, std::min(delaySamples, static_cast<float>(echoBufferL_.size() - 1)));

    // Fractional Read Position
    float rPos = static_cast<float>(echoPos_) - delaySamples;

    // High-Quality Interpolation
    float dL = getInterpolatedSample(echoBufferL_, rPos);
    float dR = getInterpolatedSample(echoBufferR_, rPos);

    // Denormal protection
    if (std::abs(dL) < 1.0e-15f) dL = 0.0f;
    if (std::abs(dR) < 1.0e-15f) dR = 0.0f;

    // Ping-Pong Feedback (Cross-channel)
    float feedL = dR * delayFeedback_;
    float feedR = dL * delayFeedback_;

    float inL = left + feedL;
    float inR = right + feedR;

    // Saturation in feedback loop
    if (delayFeedback_ > 0.8f) {
      inL = std::tanh(inL);
      inR = std::tanh(inR);
    }

    echoBufferL_[echoPos_] = inL;
    echoBufferR_[echoPos_] = inR;

    // Mix Output
    left = left * (1.0f - delayMix_) + dL * delayMix_;
    right = right * (1.0f - delayMix_) + dR * delayMix_;

    echoPos_ = (echoPos_ + 1) % echoBufferL_.size();
  }

  // 4. True Stereo Reverb
  // ---------------------------------------------------------
  if (reverbAmount_ > 0.0f) {
    // Process Left
    float inputL = left * reverbAmount_ * 0.5f; // Attenuate into reverb
    float combOutL = 0.0f;
    for (auto &comb : combsL_)
      combOutL += comb.process(inputL);
    float allpassOutL = combOutL;
    for (auto &allpass : allpassesL_)
      allpassOutL = allpass.process(allpassOutL);

    // Process Right (Independent tank with spread)
    float inputR = right * reverbAmount_ * 0.5f;
    float combOutR = 0.0f;
    for (auto &comb : combsR_)
      combOutR += comb.process(inputR);
    float allpassOutR = combOutR;
    for (auto &allpass : allpassesR_)
      allpassOutR = allpass.process(allpassOutR);

    // Mix Wet Signal
    left += allpassOutL;
    right += allpassOutR;
  }
}

} // namespace zenith
