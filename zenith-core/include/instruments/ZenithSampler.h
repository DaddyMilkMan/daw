#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "ContentPaths.h"

namespace zenith {
namespace instruments {

/**
 * @brief Sample-based instrument for Zenith DAW
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
class ZenithSampler : public juce::AudioProcessor
{
public:
    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    ZenithSampler();
    ~ZenithSampler() override;

    //==========================================================================
    // AudioProcessor overrides
    //==========================================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midiMessages) override;

    //==========================================================================
    // Editor
    //==========================================================================

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

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
    const juce::String getProgramName(int index) override
    {
        juce::ignoreUnused(index);
        return currentPatchName.isEmpty() ? "Empty" : currentPatchName;
    }
    void changeProgramName(int index, const juce::String& newName) override
    {
        juce::ignoreUnused(index, newName);
    }

    //==========================================================================
    // State save/load
    //==========================================================================

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // Patch management
    //==========================================================================

    /**
     * @brief Load a patch from a .zpatch file
     *
     * This operation happens asynchronously on a background thread.
     * The audio thread continues processing with the old patch until
     * the new one is ready.
     *
     * @param patchFile Path to .zpatch file
     * @return bool True if loading started successfully
     */
    bool loadPatch(const juce::File& patchFile);

    /**
     * @brief Load a patch by name
     *
     * Looks for the patch in the standard content directory.
     *
     * @param patchName Name of the patch to load
     * @return bool True if loading started successfully
     */
    bool loadPatchByName(const juce::String& patchName);

    /**
     * @brief Get the name of the currently loaded patch
     */
    juce::String getCurrentPatchName() const { return currentPatchName; }

    /**
     * @brief Check if a patch is currently loading
     */
    bool isLoading() const { return isLoadingPatch.load(); }

    /**
     * @brief Get available patches from the content directory
     */
    juce::StringArray getAvailablePatches() const;

    //==========================================================================
    // Parameter access (for UI)
    //==========================================================================

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

private:
    //==========================================================================
    // Parameter creation
    //==========================================================================

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    // Patch loading (background thread)
    //==========================================================================

    struct PatchData;
    void loadPatchAsync(const juce::File& patchFile);
    bool parsePatchFile(const juce::File& patchFile, PatchData& outData);
    void applyPatchData(std::unique_ptr<PatchData> patchData);

    //==========================================================================
    // Member variables
    //==========================================================================

    // Parameters (managed by APVTS)
    juce::AudioProcessorValueTreeState parameters;

    // Synthesiser engine
    juce::Synthesiser synth;

    // Current patch
    juce::String currentPatchName;
    std::atomic<bool> isLoadingPatch{false};

    // Background loading
    std::unique_ptr<juce::Thread> loadingThread;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSampler)
};

/**
 * @brief Custom sampler sound with key range and velocity range
 */
class ZenithSamplerSound : public juce::SynthesiserSound
{
public:
    ZenithSamplerSound(const juce::String& name,
                       juce::AudioFormatReader& source,
                       const juce::BigInteger& midiNotes,
                       int midiNoteForNormalPitch,
                       double attackTimeSecs,
                       double releaseTimeSecs,
                       double maxSampleLengthSeconds);

    ~ZenithSamplerSound() override;

    bool appliesToNote(int midiNoteNumber) override;
    bool appliesToChannel(int midiChannel) override;

    juce::String getName() const { return soundName; }
    juce::AudioBuffer<float>* getAudioData() { return data.get(); }
    double getSampleRate() const { return sourceSampleRate; }

private:
    juce::String soundName;
    std::unique_ptr<juce::AudioBuffer<float>> data;
    double sourceSampleRate;
    juce::BigInteger midiNotes;
    int rootNote;
    double attackTime, releaseTime;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerSound)
};

/**
 * @brief Custom sampler voice with filter and envelope
 */
class ZenithSamplerVoice : public juce::SynthesiserVoice
{
public:
    ZenithSamplerVoice();
    ~ZenithSamplerVoice() override;

    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void startNote(int midiNoteNumber, float velocity,
                  juce::SynthesiserSound* sound,
                  int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

    void setParameters(float* attack, float* decay, float* sustain, float* release,
                      float* filterCutoff, float* filterResonance,
                      float* tune, float* gain);

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

    // Parameter pointers (from APVTS)
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* filterCutoffParam = nullptr;
    std::atomic<float>* filterResonanceParam = nullptr;
    std::atomic<float>* tuneParam = nullptr;
    std::atomic<float>* gainParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerVoice)
};

} // namespace instruments
} // namespace zenith
