/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

// MixerChannel.h - Mixer channel strip with EQ, dynamics, and send/return processing

#include <array>
#include <span>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../effects/ConsoleEmulation.h"
#include "AudioConstants.h"
#include "MeteringSystem.h"

namespace zenith {

class ProCompressor {
public:
  ProCompressor() = default;

  void prepare(double sampleRate, int maxBlockSize);
  void reset();

  void setThreshold(float thresholdDb);
  void setRatio(float ratio);
  void setAttack(float attackMs);
  void setRelease(float releaseMs);
  void setMakeup(float makeupDb);
  void setKnee(float kneeDb);
  void setAutoMakeup(bool enabled);
  void setLookaheadEnabled(bool enabled);
  void setRmsEnabled(bool enabled);

  float getGainReduction() const;

  void process(juce::AudioBuffer<float> &buffer);

private:
  double sampleRate_ = ::zenith::constants::kDefaultSampleRate;

  // Parameters (initialized from EngineConstants)
  float threshold_ = ::zenith::constants::kDefaultCompThresholdDb;
  float ratio_ = ::zenith::constants::kDefaultCompRatio;
  float attackMs_ = ::zenith::constants::kDefaultCompAttackMs;
  float releaseMs_ = ::zenith::constants::kDefaultCompReleaseMs;
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

  void updateCoefficients();
  void updateAutoMakeup();
  float computeGainReduction(float inputDb) const;
};

//==============================================================================
/**
    Pre-calculated filter coefficients for RT-safe coefficient swapping.
*/

} // namespace
