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

    ProCompressor.h
    Extracted from MixerChannel.h
    
    Professional-grade compressor with RMS detection, lookahead, and soft knee.

  ==============================================================================
*/

#pragma once

#include <atomic>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include "EngineConstants.h"

namespace zenith {

//==============================================================================
/**
    Professional-grade compressor with RMS detection, lookahead, and soft knee.

    Features:
    - RMS envelope detection (more musical than peak)
    - Lookahead for transparent limiting
    - Soft knee option
    - Auto makeup gain
*/
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

} // namespace zenith
