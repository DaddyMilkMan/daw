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

struct PluginParameter {
    ParameterID id;
    juce::String name;
    juce::String shortName;
    float minValue;
    float maxValue;
    float defaultValue;
    juce::String unit;
    bool isAutomatable;
    juce::String category;
};

// Main plugin processor
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
class GrokAIEditor : public juce::AudioProcessorEditor,
                   public juce::Timer,
                   public juce::Button::Listener,
                   public juce::Slider::Listener,
                   public juce::ComboBox::Listener {
public:
    GrokAIEditor(GrokAIProcessor& processor);
    ~GrokAIEditor() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Timer callback
    void timerCallback() override;
    
    // Button/Slider listeners
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    
private:
    GrokAIProcessor& processorRef;
    
    // UI Components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;
    
    // Header
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Label> versionLabel;
    std::unique_ptr<juce::ToggleButton> bypassButton;
    std::unique_ptr<juce::ProgressBar> cpuMeter;
    
    // Mastering controls
    std::unique_ptr<juce::ComboBox> masteringModeCombo;
    std::unique_ptr<juce::Slider> intensitySlider;
    std::unique_ptr<juce::Slider> targetLoudnessSlider;
    std::unique_ptr<juce::Slider> characterSlider;
    std::unique_ptr<juce::Slider> creativitySlider;
    
    // Analysis display
    std::unique_ptr<juce::TextEditor> analysisDisplay;
    std::unique_ptr<juce::ProgressBar> loudnessMeter;
    std::unique_ptr<juce::ProgressBar> dynamicsMeter;
    std::unique_ptr<juce::ProgressBar> stereoMeter;
    
    // Real-time controls
    std::unique_ptr<juce::ToggleButton> realTimeAnalysisButton;
    std::unique_ptr<juce::ToggleButton> autoLearnButton;
    
    // Preset management
    std::unique_ptr<juce::ComboBox> presetCombo;
    std::unique_ptr<juce::TextButton> savePresetButton;
    std::unique_ptr<juce::TextButton> deletePresetButton;
    
    // AI status
    std::unique_ptr<juce::Label> aiStatusLabel;
    std::unique_ptr<juce::ProgressBar> aiActivityMeter;
    
    // Visualizations
    std::unique_ptr<juce::Component> waveformDisplay;
    std::unique_ptr<juce::Component> spectrumDisplay;
    
    // Layout
    void createHeader();
    void createMasteringControls();
    void createAnalysisDisplay();
    void createRealTimeControls();
    void createPresetControls();
    void createStatusDisplay();
    void createVisualizations();
    
    // Update methods
    void updateParameterDisplays();
    void updateAnalysisDisplays();
    void updateStatusDisplays();
    void updatePresetList();
    
    // Styling
    void setupLookAndFeel();
    juce::LookAndFeel customLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokAIEditor)
};

// Plugin factory
class GrokAIPluginFactory {
public:
    static juce::AudioProcessor* createPlugin();
    static const juce::String getPluginName();
    static const juce::String getPluginDescription();
    static bool isPluginMidiEffect();
    static bool isPluginSynth();
    static double getPluginVersion();
    static const juce::String getPluginIdentifier();
    
private:
    static constexpr double PLUGIN_VERSION = 1.0;
    static constexpr juce::String PLUGIN_NAME = "Grok AI Mastering";
    static constexpr juce::String PLUGIN_DESCRIPTION = "AI-powered mastering with Grok 4.1";
    static constexpr juce::String PLUGIN_IDENTIFIER = "zenith.grokai";
};

// Plugin entry points (for different formats)
#if JUCE_PLUGINHOST_VST3
    #define JUCE_VST3_CAN_REPLACE_VST2 0
    #include <juce_audio_processors/juce_audio_processors.h>
    
    class GrokAI_VST3Processor : public GrokAIProcessor {
    public:
        GrokAI_VST3Processor() : GrokAIProcessor() {}
        
        const juce::String getName() const override { return GrokAIPluginFactory::getPluginName(); }
        bool acceptsMidi() const override { return GrokAIPluginFactory::isPluginMidiEffect(); }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return GrokAIPluginFactory::isPluginMidiEffect(); }
        double getTailLengthSeconds() const override { return 0.0; }
    };
    
    juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
    {
        return new GrokAI_VST3Processor();
    }
#endif

#if JUCE_PLUGINHOST_AU
    #include <juce_audio_processors/juce_audio_processors.h>
    
    class GrokAI_AUProcessor : public GrokAIProcessor {
    public:
        GrokAI_AUProcessor() : GrokAIProcessor() {}
        
        const juce::String getName() const override { return GrokAIPluginFactory::getPluginName(); }
        bool acceptsMidi() const override { return GrokAIPluginFactory::isPluginMidiEffect(); }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return GrokAIPluginFactory::isPluginMidiEffect(); }
        double getTailLengthSeconds() const override { return 0.0; }
    };
    
    juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
    {
        return new GrokAI_AUProcessor();
    }
#endif

} // namespace plugin
} // namespace zenith
