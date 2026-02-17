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
class ZenithSamplerProcessor : public juce::AudioProcessor {
public:
  //==========================================================================
  // Parameter indices
  //==========================================================================
  enum Parameters {
    Attack = 0,
    Decay,
    Sustain,
    Release,
    FilterCutoff,
    FilterResonance,
    SampleStartOffset,
    PitchFine,
    PitchSemitones,
    GlobalPan,
    GlobalGain,
    Character,
    NumParameters
  };

  //==========================================================================
  // Constructor / Destructor
  //==========================================================================

  ZenithSamplerProcessor();
  ~ZenithSamplerProcessor() override;

  /**
   * @brief Get the AudioProcessorValueTreeState for parameter attachments
   */
  juce::AudioProcessorValueTreeState &getParameters() { return parameters; }

  /**
   * @brief Set the AudioFilePool for RT-safe sample loading
   *
   * @param pool Pointer to the audio file pool (can be nullptr for standalone
   * mode)
   */
  void setAudioFilePool(AudioFilePool *pool) { audioFilePool_ = pool; }

  //==========================================================================
  // AudioProcessor overrides
  //==========================================================================

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  //==========================================================================
  // Editor
  //==========================================================================

  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor *createEditor() override;

  //==========================================================================
  // Plugin description
  //==========================================================================

  const juce::String getName() const override { return "Zenith Sampler"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 2.0; }

  //==========================================================================
  // Programs (presets)
  //==========================================================================

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
  const juce::String getProgramName(int index) override {
    juce::ignoreUnused(index);
    return currentPatchName.isEmpty() ? "Empty" : currentPatchName;
  }
  void changeProgramName(int index, const juce::String &newName) override {
    juce::ignoreUnused(index, newName);
  }

  //==========================================================================
  // State save/load
  //==========================================================================

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==========================================================================
  // Patch management
  //==========================================================================

  /**
   * @brief Load a sample bank from a .zpatch file
   *
   * This operation happens asynchronously on a background thread.
   * The audio thread continues processing with the old patch until
   * the new one is ready.
   *
   * @param patchFile Path to .zpatch file
   * @return bool True if loading started successfully
   */
  bool loadSampleBank(const juce::File &patchFile);

  /**
   * @brief Load a sample bank by name
   *
   * Looks for the patch in the standard content directory.
   *
   * @param bankName Name of the bank to load
   * @return bool True if loading started successfully
   */
  bool loadSampleBankByName(const juce::String &bankName);

  /**
   * @brief Synchronous version of loadSampleBankByName for testing
   */
  bool loadSampleBankByNameSync(const juce::String &bankName);

  /**
   * @brief Load a sample bank from JSON string
   *
   * For built-in banks embedded in the binary.
   *
   * @param jsonString JSON description of the sample bank
   * @param bankName Name for the bank
   * @return bool True if loading started successfully
   */
  bool loadSampleBankFromJson(const juce::String &jsonString,
                              const juce::String &bankName);

  /**
   * @brief Get the name of the currently loaded sample bank
   */
  juce::String getCurrentBankName() const { return currentPatchName; }

  /**
   * @brief Alias for getCurrentBankName() - compatibility with editor
   */
  juce::String getCurrentPatchName() const { return currentPatchName; }

  /**
   * @brief Check if a bank is currently loading
   */
  bool isLoading() const { return isLoadingPatch.load(); }

  /**
   * @brief Get available sample banks from the content directory
   */
  juce::StringArray getAvailableBanks() const;

  /**
   * @brief Alias for getAvailableBanks() - compatibility with editor
   */
  juce::StringArray getAvailablePatches() const { return getAvailableBanks(); }

  /**
   * @brief Alias for getCurrentBankName() but with different name for editor
   * compatibility For now, just returns the current bank name (implementation
   * in .cpp)
   */
  bool loadPatchByName(const juce::String &patchName);

  //==========================================================================
  // Parameter access (for UI)
  //==========================================================================

  juce::AudioProcessorValueTreeState &getAPVTS() { return parameters; }
  juce::Synthesiser &getSynth() { return synth; }

private:
  //==========================================================================
  // Parameter creation
  //==========================================================================

  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  //==========================================================================
  // Sample bank loading (background thread)
  //==========================================================================

} // namespace
