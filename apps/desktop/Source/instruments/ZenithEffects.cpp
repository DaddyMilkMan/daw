/*
  ==============================================================================

    ZenithEffects.cpp
    Refactored: 2025-12-09
    Author:  Zenith DAW

    Implementation of ZenithEffects with Stereo Ping-Pong Delay.
    Fixes: Audio Thread Allocation Optimization.

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
  int tunings[] = {1116, 1188, 1277, 1356};
  for (int i = 0; i < 4; ++i)
    combs_[i].resize(static_cast<int>(tunings[i] * scale));
  int allpassTunings[] = {225, 556};
  for (int i = 0; i < 2; ++i)
    allpasses_[i].resize(static_cast<int>(allpassTunings[i] * scale));
  reverbInit_ = true;
}

void ZenithEffects::process(float &left, float &right) {
  // Distortion
  if (distortionAmount_ > 0.0f) {
    float drive = 1.0f + distortionAmount_ * 9.0f;
    left = std::tanh(left * drive) / drive;
    right = std::tanh(right * drive) / drive;
  }

  // Chorus
  if (chorusAmount_ > 0.0f) {
    float lfoValue = std::sin(chorusPhase_ * juce::MathConstants<float>::twoPi);
    int delayTime = static_cast<int>(5.0f + lfoValue * 3.0f);

    int readPos =
        (delayPos_ - delayTime + delayBufferL_.size()) % delayBufferL_.size();
    float chorusL = delayBufferL_[readPos];
    float chorusR = delayBufferR_[readPos];

    left = left * (1.0f - chorusAmount_) + chorusL * chorusAmount_;
    right = right * (1.0f - chorusAmount_) + chorusR * chorusAmount_;

    delayBufferL_[delayPos_] = left;
    delayBufferR_[delayPos_] = right;
    delayPos_ = (delayPos_ + 1) % delayBufferL_.size();

    chorusPhase_ += 2.0f / static_cast<float>(sampleRate_);
    if (chorusPhase_ >= 1.0f)
      chorusPhase_ -= 1.0f;
  }

  // Stereo Ping-Pong Delay
  if (delayMix_ > 0.0f && !echoBufferL_.empty()) {
    float time = delaySync_ ? getDelaySecondsForSyncRate(delaySyncRate_, bpm_)
                            : delayTime_;
    float delaySamples = time * sampleRate_;
    // Use static_cast directly or floor, but clamping is safe
    delaySamples =
        std::max(1.0f, std::min(delaySamples,
                                static_cast<float>(echoBufferL_.size() - 1)));

    int rPos =
        (echoPos_ - static_cast<int>(delaySamples) + echoBufferL_.size()) %
        echoBufferL_.size();

    float dL = echoBufferL_[rPos];
    float dR = echoBufferR_[rPos];

    // Denormal protection
    if (std::abs(dL) < 1.0e-15f)
      dL = 0.0f;
    if (std::abs(dR) < 1.0e-15f)
      dR = 0.0f;

    float feedL = dR * delayFeedback_;
    float feedR = dL * delayFeedback_;

    float inL = left + feedL;
    float inR = right + feedR;

    if (delayFeedback_ > 0.8f) {
      inL = std::tanh(inL);
      inR = std::tanh(inR);
    }

    echoBufferL_[echoPos_] = inL;
    echoBufferR_[echoPos_] = inR;

    left = left * (1.0f - delayMix_) + dL * delayMix_;
    right = right * (1.0f - delayMix_) + dR * delayMix_;

    echoPos_ = (echoPos_ + 1) % echoBufferL_.size();
  }

  // Reverb
  if (reverbAmount_ > 0.0f) {
    initReverb(); // Still lazy, but standard for reverb combs
    float input = (left + right) * 0.5f * reverbAmount_;
    float combOut = 0.0f;
    for (auto &comb : combs_)
      combOut += comb.process(input);
    float allpassOut = combOut;
    for (auto &allpass : allpasses_)
      allpassOut = allpass.process(allpassOut);

    left += allpassOut * 0.2f;
    right += allpassOut * 0.2f;
  }
}

} // namespace zenith
