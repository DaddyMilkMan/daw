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

  struct EQBand {
    std::atomic<bool> enabled{false};
    std::atomic<float> frequency{1000.0f};
    std::atomic<float> gain{0.0f}; // In dB
    std::atomic<float> q{::zenith::constants::kDefaultEQQ};

    enum class Type { LowShelf, Peak, HighShelf };
    Type type = Type::Peak;
  };

  EQBand &getEQBand(int bandIndex);
  const EQBand &getEQBand(int bandIndex) const;

  // Mark EQ as needing coefficient recalculation (message thread safe)
  void markEQDirty(int bandIndex);

  //==============================================================================
  // Console Emulation
  void setConsoleMode(zenith::effects::ConsoleEmulation::Mode mode);
  zenith::effects::ConsoleEmulation::Mode getConsoleMode() const;

  void setConsoleDrive(float drive);
  float getConsoleDrive() const;

  void setConsoleCharacter(float character);
  float getConsoleCharacter() const;

  //==============================================================================
  // Dynamics (Compressor) - Now uses ProCompressor
  void setCompressorEnabled(bool enabled);
  bool isCompressorEnabled() const { return compressorEnabled.load(); }

  void setCompressorThreshold(float thresholdDb);
  float getCompressorThreshold() const { return compThreshold.load(); }

  void setCompressorRatio(float ratio);
  float getCompressorRatio() const { return compRatio.load(); }

  void setCompressorAttack(float attackMs);
  float getCompressorAttack() const { return compAttack.load(); }

  void setCompressorRelease(float releaseMs);
  float getCompressorRelease() const { return compRelease.load(); }

  void setCompressorMakeup(float makeupDb);
  float getCompressorMakeup() const { return compMakeup.load(); }

  void setCompressorKnee(float kneeDb);
  void setCompressorLookahead(bool enabled);
  void setCompressorRmsMode(bool useRms);
  void setCompressorAutoMakeup(bool enabled);

  float getGainReduction() const { return compressor_.getGainReduction(); }

  //==============================================================================
  // Send effects (4 aux sends)
  void setSendLevel(int sendIndex, float level);
  float getSendLevel(int sendIndex) const;

  void setSendPreFader(int sendIndex, bool preFader);
  bool isSendPreFader(int sendIndex) const;

  //==============================================================================
  // Output section
  void setVolume(float volume);
  float getVolume() const { return volume.load(); }

  void setPan(float pan);
  float getPan() const { return this->pan.load(); }

  void setMuted(bool shouldBeMuted);
  bool isMuted() const { return muted.load(); }

  void setSolo(bool shouldBeSolo);
  bool isSolo() const { return solo.load(); }

  // Sol-In-Place Logic
  void setSilencedBySolo(bool silenced) { silencedBySolo.store(silenced); }
  bool isSilencedBySolo() const { return silencedBySolo.load(); }

  //==============================================================================
  // Level metering
  void setMeterMode(MeteringSystem::MeterMode mode) { meterMode.store(mode); }
  MeteringSystem::MeterMode getMeterMode() const { return meterMode.load(); }

  float getInputLevel() const { return inputMeter.getLevel(meterMode.load()); }
  float getInputPeak() const { return inputMeter.getPeak(); }
  void resetInputPeak() { inputMeter.resetPeak(); }

  float getOutputLevel() const {
    return outputMeter.getLevel(meterMode.load());
  }
  float getOutputPeak() const { return outputMeter.getPeak(); }
  void resetOutputPeak() { outputMeter.resetPeak(); }

  // Direct access for visualizers
  MeteringSystem &getInputMeter() { return inputMeter; }
  MeteringSystem &getOutputMeter() { return outputMeter; }

  void resetPeaks() {
    inputMeter.resetPeak();
    outputMeter.resetPeak();
  }

  // State management
  juce::ValueTree getState() const;
  void loadState(const juce::ValueTree &state);

private:
  //==============================================================================
  // Input section
  std::atomic<float> inputGain{0.0f}; // In dB
  std::atomic<bool> phaseInvert{false};

  // Console Emulation
  std::unique_ptr<zenith::effects::ConsoleEmulation> consoleEmulation;
  std::atomic<zenith::effects::ConsoleEmulation::Mode> consoleMode{
      zenith::effects::ConsoleEmulation::Mode::Vintage};
  std::atomic<float> consoleDrive{0.1f};
  std::atomic<float> consoleCharacter{0.0f};

  // Output Gain
  juce::dsp::Gain<float> gain;

  std::atomic<AudioFifo *> spectrumFifo_{nullptr};

  //==============================================================================
  // High-pass filter
  std::atomic<bool> hpfEnabled{false};
  std::atomic<float> hpfFrequency{20.0f};
  juce::IIRFilter hpfFilterL, hpfFilterR;

  //==============================================================================
  // EQ section
  static constexpr int numEQBands = ::zenith::constants::kNumEQBands;
  EQBand eqBands[numEQBands];
  juce::IIRFilter eqFiltersL[numEQBands];
  juce::IIRFilter eqFiltersR[numEQBands];

  // RT-safe coefficient swapping using atomic pointer and RealTimeGarbageCollector
  std::atomic<FilterCoefficients*> activeCoeffs_{nullptr};

  // Pre-calculate coefficients on message thread
  void recalculateCoefficients();
  void updateFiltersFromCoefficients(); // Called from audio thread

  //==============================================================================
  // Dynamics section - Now using ProCompressor
  ProCompressor compressor_;
  std::atomic<bool> compressorEnabled{false};
  std::atomic<float> compThreshold{::zenith::constants::kDefaultCompThresholdDb};
  std::atomic<float> compRatio{::zenith::constants::kDefaultCompRatio};
  std::atomic<float> compAttack{::zenith::constants::kDefaultCompAttackMs};
  std::atomic<float> compRelease{::zenith::constants::kDefaultCompReleaseMs};
  std::atomic<float> compMakeup{0.0f};

  //==============================================================================
  // Send effects
  static constexpr int numSends = ::zenith::constants::kNumSends;
  std::atomic<float> sendLevels[numSends];
  std::atomic<bool> sendPreFader[numSends];

  //==============================================================================
  // Output section
  std::atomic<float> volume{0.8f};
  std::atomic<float> pan{0.0f};
  std::atomic<bool> muted{false};
  std::atomic<bool> solo{false};
  std::atomic<bool> silencedBySolo{false};

  //==============================================================================
  // Metering
  MeteringSystem inputMeter;
  MeteringSystem outputMeter;
  std::atomic<MeteringSystem::MeterMode> meterMode{
      MeteringSystem::MeterMode::Peak};

  // Legacy (kept to compile, but likely unused by getters)
  std::atomic<float> inputLevel{0.0f};
  std::atomic<float> outputLevel{0.0f};
  std::atomic<float> inputPeak{0.0f};
  std::atomic<float> outputPeak{0.0f};

  //==============================================================================
  // Processing state
  double currentSampleRate = ::zenith::constants::kDefaultSampleRate;
  int currentBlockSize = ::zenith::constants::kDefaultBufferSize;

  //==============================================================================
  // Helper methods
  void processInput(juce::AudioBuffer<float> &buffer);
  void processHighPass(juce::AudioBuffer<float> &buffer);
  void processEQ(juce::AudioBuffer<float> &buffer);
  void processCompressor(juce::AudioBuffer<float> &buffer);
  void processSends(const juce::AudioBuffer<float> &sourceBuffer,
                    std::span<juce::AudioBuffer<float> * const> sendBuffers,
                    bool matchPreFader);
  void processOutput(juce::AudioBuffer<float> &buffer);
  void updateMeters(const juce::AudioBuffer<float> &buffer, bool isInput);

  float dbToGain(float db) const { return juce::Decibels::decibelsToGain(db); }
  float gainToDb(float gain) const {
    return juce::Decibels::gainToDecibels(gain);
  }

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannel)
};

} // namespace zenith
