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

#pragma once

#include "../dsp/StereoAudioFifo.h"
#include <atomic>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>


namespace zenith {

/**
 * @class MeteringSystem
 * @brief Manages audio metering and visualizer data.
 */
class MeteringSystem {
public:
  enum class MeterMode { Peak, RMS, VU, PPM, K12, K14, K20, LUFS_Momentary };

  MeteringSystem();
  ~MeteringSystem() = default;

  // Audio thread
  void prepare(const juce::dsp::ProcessSpec &spec);
  void process(const juce::AudioBuffer<float> &buffer);
  void reset();

  // Message thread
  float getLevel(MeterMode mode) const;
  float getMasterLevel() const { return rmsLevel.load(); }
  float getMasterPeak() const { return masterPeak.load(); }
  void resetMasterPeak() { masterPeak.store(0.0f); }
  float getPeak() const { return masterPeak.load(); }
  void resetPeak() { masterPeak.store(0.0f); }

  StereoAudioFifo &getAnalysisFifo() { return *analysisFifo; }

private:
  std::atomic<float> masterPeak{0.0f};
  std::atomic<float> rmsLevel{0.0f};
  std::atomic<float> vuLevel{0.0f};          // 300ms integration
  std::atomic<float> ppmLevel{0.0f};         // 10ms integration
  std::atomic<float> lufsMomentary{-100.0f}; // K-weighted momentary

  // Ballistics
  float vuEnvelope_ = 0.0f;
  float ppmEnvelope_ = 0.0f;

  // K-Weighting Filters for LUFS
  juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>,
                            juce::dsp::IIR::Filter<float>>
      kWeightingFilter_;

  std::unique_ptr<StereoAudioFifo> analysisFifo;
  double sampleRate_ = 44100.0;

  // Constants
  static constexpr float VU_RISE_TIME = 0.300f; // 300ms
  static constexpr float VU_FALL_TIME = 0.300f;
  static constexpr float PPM_RISE_TIME = 0.010f; // 10ms
  static constexpr float PPM_FALL_TIME = 1.500f; // Slow fallback

  // Scratch buffer for LUFS processing (RT-safe)
  juce::AudioBuffer<float> scratchBuffer_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeteringSystem)
};

} // namespace zenith
