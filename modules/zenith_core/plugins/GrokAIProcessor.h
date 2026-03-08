/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "../ai/GrokAPIClient.h"
#include "../audio/RealTimeAudioBuffer.h"
#include <memory>

namespace zenith {
namespace plugin {

// Plugin parameters
enum class ParameterID {
    MasteringMode = 0,
    Intensity,
    TargetLoudness,
    Character,
    Creativity,
    RealTimeAnalysis,
    AutoLearn,
    Bypass,
    NumParameters
};

class GrokAIProcessor : public juce::AudioProcessor {
public:
    GrokAIProcessor();
    ~GrokAIProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    // Program management
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    // Parameter management
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    // State management
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Custom methods
    void initializeAI(const juce::String& apiKey);
    bool isAIInitialized() const;

    // Analysis and processing
    void analyzeCurrentAudio();
    void applyMastering();
    void resetLearning();

    // Real-time features
    void enableRealTimeAnalysis(bool enabled);
    bool isRealTimeAnalysisEnabled() const;

    // Presets
    void loadPreset(const juce::String& presetName);
    void savePreset(const juce::String& presetName);
    std::vector<juce::String> getAvailablePresets() const;

private:
    // AI components
    std::unique_ptr<ai::GrokAPIClient> grokClient;
    std::unique_ptr<audio::RealTimeAudioProcessor> audioProcessor;

    // Parameters
    std::vector<PluginParameter> parameters;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* masteringModeParam;
    std::atomic<float>* intensityParam;
    std::atomic<float>* targetLoudnessParam;
    std::atomic<float>* characterParam;
    std::atomic<float>* creativityParam;
    std::atomic<float>* realTimeAnalysisParam;
    std::atomic<float>* autoLearnParam;
    std::atomic<float>* bypassParam;

    // Processing state
    bool isProcessing = false;
    bool aiInitialized = false;
    bool realTimeAnalysisEnabled = false;
    juce::AudioBuffer<float> analysisBuffer;
    juce::AudioBuffer<float> processedBuffer;

    // Analysis data
    std::atomic<float> currentLoudness{0.0f};
    std::atomic<float> currentDynamics{0.0f};
    std::atomic<float> currentStereoWidth{0.0f};
    std::atomic<int> currentGenre{0};
    juce::String currentAnalysisText;

    // Performance monitoring
    std::atomic<float> cpuUsage{0.0f};
    std::atomic<int> processingLatency{0};
    std::atomic<int> underruns{0};

    // Preset management
    juce::File presetDirectory;
    std::unordered_map<juce::String, juce::var> presets;

    // Initialization
    void initializeParameters();
    void setupAudioProcessor();
    void loadPresets();

    // Parameter handling
    void updateParameterValues();
    float getParameterValue(ParameterID id) const;
    void setParameterValue(ParameterID id, float value);

    // Audio processing
    void processWithAI(juce::AudioBuffer<float>& buffer);
    void processBypassed(juce::AudioBuffer<float>& buffer);
    void applyParameterChanges(juce::AudioBuffer<float>& buffer);

    // Analysis
    void performRealTimeAnalysis(const juce::AudioBuffer<float>& buffer);
    void updateAnalysisDisplay();

    // Preset operations
    void saveCurrentStateAsPreset(const juce::String& name);
    void loadPresetState(const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokAIProcessor)
};

// Plugin editor

} // namespace
