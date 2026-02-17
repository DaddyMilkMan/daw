/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include "../engine/AudioFilePool.h"
#include "ContentPaths.h"
#include "Instrument.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {

// Forward declarations for voice/sound classes
  struct SampleBankData;
  void loadBankAsync(const juce::File &bankFile);
  void loadBankFromJsonAsync(const juce::String &jsonString,
                             const juce::String &bankName);
  bool parseBankFile(const juce::File &bankFile, SampleBankData &outData);
  bool parseBankJson(const juce::var &json, const juce::File &baseDir,
                     SampleBankData &outData);
  void applyBankData(std::shared_ptr<SampleBankData> bankData);

  //==========================================================================
  // Member variables
  //==========================================================================

  // Parameters (managed by APVTS)
  juce::AudioProcessorValueTreeState parameters;

  // Synthesiser engine
  juce::Synthesiser synth;

  // AudioFilePool integration (optional, can be nullptr)
  AudioFilePool *audioFilePool_ = nullptr;

  // Current patch
  juce::String currentPatchName;
  std::atomic<bool> isLoadingPatch{false};

  // Background loading
  std::unique_ptr<juce::Thread> loadingThread;

  //==========================================================================
  JUCE_DECLARE_WEAK_REFERENCEABLE(ZenithSamplerProcessor)
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerProcessor)
};

//==============================================================================
/**
    Zenith Sampler instrument wrapper

    Wraps ZenithSamplerProcessor with InstrumentBase to provide:
    - Metadata (parameters, macros)
    - Preset management
    - CommandAPI integration
*/

} // namespace
