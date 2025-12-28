/*
  ==============================================================================

    ZenithSampler.h
    Created: 2025-11-18
    Author:  Zenith DAW

    Comprehensive sampler instrument with multi-sample playback,
    velocity layers, envelope, filter, and async patch loading.

  ==============================================================================
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
class ZenithSamplerSound;
class ZenithSamplerVoice;

/**
 * @brief Sample-based AudioProcessor for Zenith DAW
 *
 * Features:
 * - Multi-sample playback with key ranges and velocity layers
 * - Amp envelope (ADSR)
 * - Low-pass filter with envelope
 * - Global tuning, gain, and character control
 * - Async patch loading (off audio thread)
 *
 * Patch format: .zpatch (JSON-based)
 */
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
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerProcessor)
  JUCE_DECLARE_WEAK_REFERENCEABLE(ZenithSamplerProcessor)
};

//==============================================================================
/**
    Zenith Sampler instrument wrapper

    Wraps ZenithSamplerProcessor with InstrumentBase to provide:
    - Metadata (parameters, macros)
    - Preset management
    - CommandAPI integration
*/
class ZenithSampler : public InstrumentBase {
public:
  ZenithSampler();
  ~ZenithSampler() override = default;

  /**
   * @brief Create metadata for this instrument
   */
  static InstrumentMetadata createMetadata();

  /**
   * @brief Get the AudioProcessorValueTreeState for parameter attachments in
   * editor
   */
  juce::AudioProcessorValueTreeState *getParameterState();

private:
  void registerPresets();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSampler)
};

//==============================================================================
/**
 * @brief Custom sampler sound with key range, velocity range, and loop mode
 */
class ZenithSamplerSound : public juce::SynthesiserSound {
public:
  enum class LoopMode { None, Forward, PingPong };

  ZenithSamplerSound(const juce::String &name, juce::AudioFormatReader &source,
                     const juce::BigInteger &midiNotes,
                     int midiNoteForNormalPitch, int lowVelocity,
                     int highVelocity, double attackTimeSecs,
                     double releaseTimeSecs, double maxSampleLengthSeconds,
                     LoopMode loopMode = LoopMode::None, float gain = 1.0f,
                     float tune = 0.0f, int chokeGroup = 0);

  // Constructor that uses AudioFilePool handle
  ZenithSamplerSound(const juce::String &name,
                     AudioFilePool::HandlePtr audioHandle,
                     const juce::BigInteger &midiNotes,
                     int midiNoteForNormalPitch, int lowVelocity,
                     int highVelocity, LoopMode loopMode = LoopMode::None,
                     float gain = 1.0f, float tune = 0.0f, int chokeGroup = 0);

  ~ZenithSamplerSound() override;

  bool appliesToNote(int midiNoteNumber) override;
  bool appliesToChannel(int midiChannel) override;

  juce::String getName() const { return soundName; }
  const juce::AudioBuffer<float> *getAudioData() const { return data; }
  double getSampleRate() const { return sourceSampleRate; }
  int getRootNote() const { return rootNote; }
  LoopMode getLoopMode() const { return loopMode; }
  float getGain() const { return gain; }
  float getTune() const { return tune; }
  int getChokeGroup() const { return chokeGroup; }

  bool appliesToVelocity(int midiVelocity) const {
    return midiVelocity >= lowVelocity && midiVelocity <= highVelocity;
  }

  int getLowKey() const {
    for (int i = 0; i < 128; ++i)
      if (midiNotes[i])
        return i;
    return -1;
  }
  int getHighKey() const {
    for (int i = 127; i >= 0; --i)
      if (midiNotes[i])
        return i;
    return -1;
  }
  int getLowVelocity() const { return lowVelocity; }
  int getHighVelocity() const { return highVelocity; }

private:
  juce::String soundName;

  // Can hold audio data directly or via pool handle
  std::unique_ptr<juce::AudioBuffer<float>> ownedData;
  AudioFilePool::HandlePtr poolHandle;
  const juce::AudioBuffer<float> *data =
      nullptr; // Points to either ownedData or poolHandle->buffer

  double sourceSampleRate;
  juce::BigInteger midiNotes;
  int rootNote;
  int lowVelocity, highVelocity;
  LoopMode loopMode;
  float gain;
  float tune;
  int chokeGroup;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerSound)
};

//==============================================================================
/**
 * @brief Custom sampler voice with filter and envelope
 */
class ZenithSamplerVoice : public juce::SynthesiserVoice {
public:
  ZenithSamplerVoice();
  ~ZenithSamplerVoice() override;

  bool canPlaySound(juce::SynthesiserSound *sound) override;

  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;

  void stopNote(float velocity, bool allowTailOff) override;

  void pitchWheelMoved(int newPitchWheelValue) override;
  void controllerMoved(int controllerNumber, int newControllerValue) override;

  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

  void setParameters(std::atomic<float> *attack, std::atomic<float> *decay,
                     std::atomic<float> *sustain, std::atomic<float> *release,
                     std::atomic<float> *filterCutoff,
                     std::atomic<float> *filterResonance,
                     std::atomic<float> *sampleStartOffset,
                     std::atomic<float> *pitchFine,
                     std::atomic<float> *pitchSemitones,
                     std::atomic<float> *globalPan,
                     std::atomic<float> *globalGain);

private:
  // Envelope
  juce::ADSR ampEnvelope;
  juce::ADSR::Parameters ampEnvParams;

  // Filter
  juce::dsp::StateVariableTPTFilter<float> filter;

  // Playback state
  double pitchRatio = 0.0;
  double sourceSamplePosition = 0.0;
  float velocity = 0.0f;
  bool loopDirection = true; // true = forward, false = backward (for ping-pong)

  // Parameter pointers (from APVTS)
  std::atomic<float> *attackParam = nullptr;
  std::atomic<float> *decayParam = nullptr;
  std::atomic<float> *sustainParam = nullptr;
  std::atomic<float> *releaseParam = nullptr;
  std::atomic<float> *filterCutoffParam = nullptr;
  std::atomic<float> *filterResonanceParam = nullptr;
  std::atomic<float> *sampleStartOffsetParam = nullptr;
  std::atomic<float> *pitchFineParam = nullptr;
  std::atomic<float> *pitchSemitonesParam = nullptr;
  std::atomic<float> *globalPanParam = nullptr;
  std::atomic<float> *globalGainParam = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerVoice)
};

} // namespace zenith
