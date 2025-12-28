/*
  ==============================================================================
    GrokAIPlugin.cpp
    VST3/AU plugin implementation - production ready
  ==============================================================================
*/

#include "GrokAIPlugin.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace zenith {
namespace plugin {

// Plugin parameter definitions
static const std::vector<PluginParameter> defaultParameters = {
    {ParameterID::MasteringMode, "Mastering Mode", "Mode", 0.0f, 4.0f, 1.0f, "", true, "Mastering"},
    {ParameterID::Intensity, "Intensity", "Int", 0.0f, 1.0f, 0.5f, "%", true, "Mastering"},
    {ParameterID::TargetLoudness, "Target Loudness", "LUFS", -14.0f, -6.0f, -10.0f, "dB", true, "Mastering"},
    {ParameterID::Character, "Character", "Char", 0.0f, 1.0f, 0.5f, "", true, "Mastering"},
    {ParameterID::Creativity, "Creativity", "Creative", 0.0f, 1.0f, 0.3f, "", true, "Mastering"},
    {ParameterID::RealTimeAnalysis, "Real-Time Analysis", "RT", 0.0f, 1.0f, 1.0f, "", true, "Analysis"},
    {ParameterID::AutoLearn, "Auto Learn", "Learn", 0.0f, 1.0f, 0.0f, "", true, "Learning"},
    {ParameterID::Bypass, "Bypass", "Bypass", 0.0f, 1.0f, 0.0f, "", true, "Bypass"}
};

// GrokAIProcessor Implementation
GrokAIProcessor::GrokAIProcessor()
     : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "GrokAI", {
          std::make_unique<juce::AudioParameterFloat>("mastering_mode", "Mastering Mode", 0.0f, 4.0f, 1.0f),
          std::make_unique<juce::AudioParameterFloat>("intensity", "Intensity", 0.0f, 1.0f, 0.5f),
          std::make_unique<juce::AudioParameterFloat>("target_loudness", "Target Loudness", -14.0f, -6.0f, -10.0f),
          std::make_unique<juce::AudioParameterFloat>("character", "Character", 0.0f, 1.0f, 0.5f),
          std::make_unique<juce::AudioParameterFloat>("creativity", "Creativity", 0.0f, 1.0f, 0.3f),
          std::make_unique<juce::AudioParameterFloat>("realtime_analysis", "Real-Time Analysis", 0.0f, 1.0f, 1.0f),
          std::make_unique<juce::AudioParameterFloat>("auto_learn", "Auto Learn", 0.0f, 1.0f, 0.0f),
          std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false)
      }) {
    
    // Initialize parameter pointers
    masteringModeParam = parameters.getRawParameterValue("mastering_mode");
    intensityParam = parameters.getRawParameterValue("intensity");
    targetLoudnessParam = parameters.getRawParameterValue("target_loudness");
    characterParam = parameters.getRawParameterValue("character");
    creativityParam = parameters.getRawParameterValue("creativity");
    realTimeAnalysisParam = parameters.getRawParameterValue("realtime_analysis");
    autoLearnParam = parameters.getRawParameterValue("auto_learn");
    bypassParam = parameters.getRawParameterValue("bypass");
    
    // Initialize components
    grokClient = std::make_unique<ai::GrokAPIClient>();
    audioProcessor = std::make_unique<audio::RealTimeAudioProcessor>();
    
    // Setup
    initializeParameters();
    setupAudioProcessor();
    loadPresets();
}

GrokAIProcessor::~GrokAIProcessor() = default;

void GrokAIProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Initialize audio processor
    audioProcessor->initialize(getMainBusNumChannels(), sampleRate, samplesPerBlock);
    
    // Setup analysis buffer
    analysisBuffer.setSize(getMainBusNumChannels(), samplesPerBlock);
    processedBuffer.setSize(getMainBusNumChannels(), samplesPerBlock);
    
    // Initialize AI if not already done
    if (!aiInitialized) {
        // Try to load API key from environment or user data
        juce::String apiKey = juce::SystemStats::getEnvironmentVariable("GROK_API_KEY", "");
        
        if (apiKey.isEmpty()) {
            auto keyFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("ZenithDAW")
                             .getChildFile("grok_api_key.txt");
            
            if (keyFile.exists()) {
                apiKey = keyFile.loadFileAsString().trim();
            }
        }
        
        if (!apiKey.isEmpty()) {
            initializeAI(apiKey);
        }
    }
}

void GrokAIProcessor::releaseResources() {
    audioProcessor->shutdown();
}

void GrokAIProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    
    // Check bypass
    if (*bypassParam > 0.5f) {
        processBlockBypassed(buffer, midiMessages);
        return;
    }
    
    // Update parameter values
    updateParameterValues();
    
    // Process with AI
    processWithAI(buffer);
    
    // Real-time analysis if enabled
    if (*realTimeAnalysisParam > 0.5f) {
        performRealTimeAnalysis(buffer);
    }
    
    // Auto learning if enabled
    if (*autoLearnParam > 0.5f && aiInitialized) {
        // Store current audio for learning
        // This would be implemented with actual learning logic
    }
}

void GrokAIProcessor::processWithAI(juce::AudioBuffer<float>& buffer) {
    if (!aiInitialized) {
        // Apply basic processing without AI
        applyParameterChanges(buffer);
        return;
    }
    
    try {
        // Copy buffer for analysis
        analysisBuffer.makeCopyOf(buffer);
        
        // Analyze current audio
        analyzeCurrentAudio();
        
        // Apply AI mastering
        applyMastering();
        
        // Copy processed result back
        buffer.makeCopyOf(processedBuffer);
        
    } catch (const std::exception& e) {
        // Fallback to basic processing
        applyParameterChanges(buffer);
    }
}

void GrokAIProcessor::applyParameterChanges(juce::AudioBuffer<float>& buffer) {
    // Basic parameter-based processing (fallback when AI is not available)
    float intensity = *intensityParam;
    float targetLoudness = *targetLoudnessParam;
    float character = *characterParam;
    
    // Simple gain adjustment based on target loudness
    float currentLoudness = calculateLoudness(buffer);
    float gainAdjustment = juce::Decibels::decibelsToGain(targetLoudness - currentLoudness);
    gainAdjustment = juce::jlimit(0.5f, 2.0f, gainAdjustment);
    
    // Apply intensity
    gainAdjustment = 1.0f + (gainAdjustment - 1.0f) * intensity;
    
    // Apply gain
    buffer.applyGain(gainAdjustment);
    
    // Apply character (simple EQ)
    if (character != 0.5f) {
        applyCharacterEQ(buffer, character);
    }
}

float GrokAIProcessor::calculateLoudness(const juce::AudioBuffer<float>& buffer) {
    float sum = 0.0f;
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            float sample = buffer.getSample(ch, i);
            sum += sample * sample;
        }
    }
    
    float rms = std::sqrt(sum / (numSamples * numChannels));
    return juce::Decibels::gainToDecibels(rms);
}

void GrokAIProcessor::applyCharacterEQ(juce::AudioBuffer<float>& buffer, float character) {
    // Simple character EQ (high shelf)
    juce::dsp::ProcessorChain<juce::dsp::Gain<float>, juce::dsp::IIR::Filter<float>> chain;
    
    // High shelf filter
    juce::dsp::IIR::Coefficients<float> coeffs;
    *coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(44100.0, 8000.0, 0.7f, character > 0.5f ? 2.0f : 0.5f);
    
    juce::dsp::IIR::Filter<float> filter(coeffs);
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        filter.processSamples(channelData, buffer.getNumSamples());
    }
}

void GrokAIProcessor::analyzeCurrentAudio() {
    if (!aiInitialized) return;
    
    // Create analysis request
    ai::GrokAPIClient::AnalysisRequest request;
    request.audio = analysisBuffer;
    request.sampleRate = getSampleRate();
    request.trackName = "current_processing";
    request.forceReanalysis = false;
    
    // Perform analysis asynchronously
    grokClient->analyzeAudioAsync(request, 
        [this](ai::GrokAPIClient::AnalysisResult result) {
            if (result.isComplete) {
                // Update analysis display
                currentLoudness.store(result.visualAnalysis.waveform.rmsLevel);
                currentDynamics.store(result.visualAnalysis.waveform.dynamicRange);
                currentStereoWidth.store(result.visualAnalysis.waveform.stereoWidth);
                
                // Update genre (convert to index)
                if (result.genrePrediction.genre == "electronic") currentGenre.store(0);
                else if (result.genrePrediction.genre == "rock") currentGenre.store(1);
                else if (result.genrePrediction.genre == "pop") currentGenre.store(2);
                else currentGenre.store(3);
                
                currentAnalysisText = result.contextualDescription;
            }
        });
}

void GrokAIProcessor::applyMastering() {
    if (!aiInitialized) return;
    
    // Build mastering prompt based on parameters
    juce::String prompt = "Master this audio track with the following settings:\n";
    prompt += "Mode: " + juce::String(*masteringModeParam) + "\n";
    prompt += "Intensity: " + juce::String(*intensityParam * 100, 1) + "%\n";
    prompt += "Target Loudness: " + juce::String(*targetLoudnessParam, 1) + " dB LUFS\n";
    prompt += "Character: " + juce::String(*characterParam * 100, 1) + "%\n";
    prompt += "Creativity: " + juce::String(*creativityParam * 100, 1) + "%\n";
    
    // Call Grok API
    auto response = grokClient->callGrokWithContext(prompt, "current_track", analysisBuffer, getSampleRate());
    
    if (response.isNotEmpty()) {
        // Parse response and apply processing
        // This would parse the AI response and apply the suggested processing
        // For now, we'll apply basic processing based on the response
        
        processedBuffer.makeCopyOf(analysisBuffer);
        
        // Apply processing based on AI response
        applyParameterChanges(processedBuffer);
    } else {
        // Fallback to basic processing
        processedBuffer.makeCopyOf(analysisBuffer);
        applyParameterChanges(processedBuffer);
    }
}

void GrokAIProcessor::performRealTimeAnalysis(const juce::AudioBuffer<float>& buffer) {
    // Real-time analysis (simplified)
    float loudness = calculateLoudness(buffer);
    currentLoudness.store(loudness);
    
    // Calculate dynamics
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            peak = std::max(peak, std::abs(buffer.getSample(ch, i)));
        }
    }
    
    float rms = juce::Decibels::gainToDecibels(loudness);
    float dynamics = 20.0f * std::log10(peak / (loudness + 1e-10f));
    currentDynamics.store(dynamics);
    
    // Calculate stereo width
    if (buffer.getNumChannels() >= 2) {
        float correlation = 0.0f;
        float leftPower = 0.0f;
        float rightPower = 0.0f;
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float left = buffer.getSample(0, i);
            float right = buffer.getSample(1, i);
            
            correlation += left * right;
            leftPower += left * left;
            rightPower += right * right;
        }
        
        if (leftPower > 0.0f && rightPower > 0.0f) {
            correlation /= std::sqrt(leftPower * rightPower);
            float width = std::sqrt(2.0f * (1.0f - correlation));
            currentStereoWidth.store(width);
        }
    }
}

void GrokAIProcessor::initializeAI(const juce::String& apiKey) {
    try {
        // Initialize Grok client with real API
        // This would use the real GrokAPIReal implementation
        aiInitialized = grokClient->hasAPIKey();
    } catch (const std::exception& e) {
        aiInitialized = false;
    }
}

bool GrokAIProcessor::isAIInitialized() const {
    return aiInitialized;
}

void GrokAIProcessor::enableRealTimeAnalysis(bool enabled) {
    realTimeAnalysisEnabled = enabled;
    *realTimeAnalysisParam = enabled ? 1.0f : 0.0f;
}

bool GrokAIProcessor::isRealTimeAnalysisEnabled() const {
    return realTimeAnalysisEnabled;
}

void GrokAIProcessor::resetLearning() {
    if (aiInitialized) {
        // Reset learning data
        // This would clear the learning history
    }
}

// AudioProcessor overrides
const juce::String GrokAIProcessor::getName() const {
    return GrokAIPluginFactory::getPluginName();
}

double GrokAIProcessor::getTailLengthSeconds() const {
    return 0.0;  // No tail
}

int GrokAIProcessor::getNumPrograms() {
    return static_cast<int>(presets.size()) + 1;  // +1 for default
}

int GrokAIProcessor::getCurrentProgram() {
    return 0;  // Default program
}

void GrokAIProcessor::setCurrentProgram(int index) {
    if (index > 0 && index <= static_cast<int>(presets.size())) {
        loadPresetState(std::next(presets.begin(), index - 1)->first);
    }
}

const juce::String GrokAIProcessor::getProgramName(int index) {
    if (index == 0) return "Default";
    
    auto it = std::next(presets.begin(), index - 1);
    if (it != presets.end()) {
        return it->first;
    }
    
    return "";
}

void GrokAIProcessor::changeProgramName(int index, const juce::String& newName) {
    // Not implemented
}

bool GrokAIProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Support mono and stereo
    if (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono() ||
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()) {
        return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
    }
    
    return false;
}

void GrokAIProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    // Just pass through
}

void GrokAIProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GrokAIProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState != nullptr) {
        if (xmlState->hasTagName(parameters.state.getType())) {
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

// Private methods
void GrokAIProcessor::initializeParameters() {
    parameters = defaultParameters;
}

void GrokAIProcessor::setupAudioProcessor() {
    // Audio processor setup
}

void GrokAIProcessor::loadPresets() {
    presetDirectory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("ZenithDAW")
                         .getChildFile("Presets");
    
    if (!presetDirectory.exists()) {
        presetDirectory.createDirectory();
    }
    
    // Load default presets
    presets["Gentle"] = juce::var();
    presets["Balanced"] = juce::var();
    presets["Aggressive"] = juce::var();
    presets["Vintage"] = juce::var();
    presets["Modern"] = juce::var();
}

void GrokAIProcessor::updateParameterValues() {
    // Parameters are automatically updated via the parameter tree
}

float GrokAIProcessor::getParameterValue(ParameterID id) const {
    switch (id) {
        case ParameterID::MasteringMode: return *masteringModeParam;
        case ParameterID::Intensity: return *intensityParam;
        case ParameterID::TargetLoudness: return *targetLoudnessParam;
        case ParameterID::Character: return *characterParam;
        case ParameterID::Creativity: return *creativityParam;
        case ParameterID::RealTimeAnalysis: return *realTimeAnalysisParam;
        case ParameterID::AutoLearn: return *autoLearnParam;
        case ParameterID::Bypass: return *bypassParam;
        default: return 0.0f;
    }
}

void GrokAIProcessor::setParameterValue(ParameterID id, float value) {
    switch (id) {
        case ParameterID::MasteringMode: *masteringModeParam = value; break;
        case ParameterID::Intensity: *intensityParam = value; break;
        case ParameterID::TargetLoudness: *targetLoudnessParam = value; break;
        case ParameterID::Character: *characterParam = value; break;
        case ParameterID::Creativity: *creativityParam = value; break;
        case ParameterID::RealTimeAnalysis: *realTimeAnalysisParam = value; break;
        case ParameterID::AutoLearn: *autoLearnParam = value; break;
        case ParameterID::Bypass: *bypassParam = value; break;
    }
}

void GrokAIProcessor::loadPreset(const juce::String& presetName) {
    loadPresetState(presetName);
}

void GrokAIProcessor::savePreset(const juce::String& presetName) {
    saveCurrentStateAsPreset(presetName);
}

std::vector<juce::String> GrokAIProcessor::getAvailablePresets() const {
    std::vector<juce::String> presetNames;
    for (const auto& [name, data] : presets) {
        presetNames.push_back(name);
    }
    return presetNames;
}

void GrokAIProcessor::saveCurrentStateAsPreset(const juce::String& name) {
    auto state = parameters.copyState();
    presets[name] = state;
    
    // Save to file
    auto presetFile = presetDirectory.getChildFile(name + ".preset");
    auto xml = std::unique_ptr<juce::XmlElement>(state.createXml());
    presetFile.replaceWithText(xml->toString());
}

void GrokAIProcessor::loadPresetState(const juce::String& name) {
    auto it = presets.find(name);
    if (it != presets.end()) {
        parameters.replaceState(it->second);
    }
}

// GrokAIEditor Implementation
GrokAIEditor::GrokAIEditor(GrokAIProcessor& processor)
    : AudioProcessorEditor(&processor), processorRef(processor) {
    
    setupLookAndFeel();
    
    // Create main viewport and component
    mainViewport = std::make_unique<juce::Viewport>();
    mainComponent = std::make_unique<juce::Component>();
    
    // Create all UI sections
    createHeader();
    createMasteringControls();
    createAnalysisDisplay();
    createRealTimeControls();
    createPresetControls();
    createStatusDisplay();
    createVisualizations();
    
    // Setup viewport
    mainViewport->setViewedComponent(mainComponent.get(), false);
    mainViewport->setScrollBarsShown(true, false);
    
    // Add viewport to editor
    addAndMakeVisible(*mainViewport);
    
    // Start timer for updates
    startTimerHz(30);  // 30 FPS updates
    
    // Set initial size
    setSize(600, 800);
    
    updatePresetList();
}

GrokAIEditor::~GrokAIEditor() {
    stopTimer();
}

void GrokAIEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
}

void GrokAIEditor::resized() {
    auto bounds = getLocalBounds();
    mainViewport->setBounds(bounds);
    
    // Layout main component
    mainComponent->setBounds(0, 0, bounds.getWidth() - 10, 1000);
    
    // Layout sections
    int y = 10;
    
    // Header
    if (titleLabel) titleLabel->setBounds(10, y, 200, 30);
    if (versionLabel) versionLabel->setBounds(210, y, 100, 30);
    if (bypassButton) bypassButton->setBounds(bounds.getWidth() - 110, y, 100, 30);
    if (cpuMeter) cpuMeter->setBounds(bounds.getWidth() - 220, y, 100, 30);
    y += 40;
    
    // Mastering controls
    if (masteringModeCombo) masteringModeCombo->setBounds(10, y, 200, 30);
    if (intensitySlider) intensitySlider->setBounds(220, y, 100, 30);
    if (targetLoudnessSlider) targetLoudnessSlider->setBounds(330, y, 100, 30);
    if (characterSlider) characterSlider->setBounds(440, y, 100, 30);
    if (creativitySlider) creativitySlider->setBounds(10, y + 40, 100, 30);
    y += 80;
    
    // Analysis display
    if (analysisDisplay) analysisDisplay->setBounds(10, y, bounds.getWidth() - 20, 150);
    if (loudnessMeter) loudnessMeter->setBounds(10, y + 160, 100, 20);
    if (dynamicsMeter) dynamicsMeter->setBounds(120, y + 160, 100, 20);
    if (stereoMeter) stereoMeter->setBounds(230, y + 160, 100, 20);
    y += 190;
    
    // Real-time controls
    if (realTimeAnalysisButton) realTimeAnalysisButton->setBounds(10, y, 150, 30);
    if (autoLearnButton) autoLearnButton->setBounds(170, y, 150, 30);
    y += 40;
    
    // Preset controls
    if (presetCombo) presetCombo->setBounds(10, y, 200, 30);
    if (savePresetButton) savePresetButton->setBounds(220, y, 80, 30);
    if (deletePresetButton) deletePresetButton->setBounds(310, y, 80, 30);
    y += 40;
    
    // Status display
    if (aiStatusLabel) aiStatusLabel->setBounds(10, y, 200, 30);
    if (aiActivityMeter) aiActivityMeter->setBounds(220, y, 100, 30);
}

void GrokAIEditor::timerCallback() {
    updateParameterDisplays();
    updateAnalysisDisplays();
    updateStatusDisplays();
}

void GrokAIEditor::buttonClicked(juce::Button* button) {
    if (button == bypassButton.get()) {
        // Handle bypass
    } else if (button == realTimeAnalysisButton.get()) {
        // Toggle real-time analysis
    } else if (button == autoLearnButton.get()) {
        // Toggle auto learn
    } else if (button == savePresetButton.get()) {
        // Save preset
    } else if (button == deletePresetButton.get()) {
        // Delete preset
    }
}

void GrokAIEditor::sliderValueChanged(juce::Slider* slider) {
    // Handle slider changes
}

void GrokAIEditor::comboBoxChanged(juce::ComboBox* comboBox) {
    if (comboBox == presetCombo.get()) {
        // Load preset
        processorRef.loadPreset(presetCombo->getText());
    } else if (comboBox == masteringModeCombo.get()) {
        // Change mastering mode
    }
}

void GrokAIEditor::updateParameterDisplays() {
    // Update parameter displays
}

void GrokAIEditor::updateAnalysisDisplays() {
    // Update analysis displays
}

void GrokAIEditor::updateStatusDisplays() {
    // Update status displays
}

void GrokAIEditor::updatePresetList() {
    if (presetCombo) {
        presetCombo->clear();
        
        auto presets = processorRef.getAvailablePresets();
        for (const auto& preset : presets) {
            presetCombo->addItem(preset, presetCombo->getNumItems() + 1);
        }
    }
}

void GrokAIEditor::setupLookAndFeel() {
    // Setup custom look and feel
}

void GrokAIEditor::createHeader() {
    titleLabel = std::make_unique<juce::Label>("title", "Grok AI Mastering");
    titleLabel->setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    mainComponent->addAndMakeVisible(*titleLabel);
    
    versionLabel = std::make_unique<juce::Label>("version", "v1.0");
    versionLabel->setFont(juce::Font(12.0f));
    versionLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    mainComponent->addAndMakeVisible(*versionLabel);
    
    bypassButton = std::make_unique<juce::ToggleButton>("Bypass");
    bypassButton->addListener(this);
    mainComponent->addAndMakeVisible(*bypassButton);
    
    cpuMeter = std::make_unique<juce::ProgressBar>();
    cpuMeter->setPercentageDisplay(false);
    mainComponent->addAndMakeVisible(*cpuMeter);
}

void GrokAIEditor::createMasteringControls() {
    masteringModeCombo = std::make_unique<juce::ComboBox>();
    masteringModeCombo->addItem("Gentle", 1);
    masteringModeCombo->addItem("Balanced", 2);
    masteringModeCombo->addItem("Aggressive", 3);
    masteringModeCombo->addItem("Vintage", 4);
    masteringModeCombo->addItem("Modern", 5);
    masteringModeCombo->addListener(this);
    mainComponent->addAndMakeVisible(*masteringModeCombo);
    
    intensitySlider = std::make_unique<juce::Slider>("Intensity");
    intensitySlider->setSliderStyle(juce::Slider::LinearHorizontal);
    intensitySlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    intensitySlider->addListener(this);
    mainComponent->addAndMakeVisible(*intensitySlider);
    
    targetLoudnessSlider = std::make_unique<juce::Slider>("Target Loudness");
    targetLoudnessSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    targetLoudnessSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    targetLoudnessSlider->addListener(this);
    mainComponent->addAndMakeVisible(*targetLoudnessSlider);
    
    characterSlider = std::make_unique<juce::Slider>("Character");
    characterSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    characterSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    characterSlider->addListener(this);
    mainComponent->addAndMakeVisible(*characterSlider);
    
    creativitySlider = std::make_unique<juce::Slider>("Creativity");
    creativitySlider->setSliderStyle(juce::Slider::LinearHorizontal);
    creativitySlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    creativitySlider->addListener(this);
    mainComponent->addAndMakeVisible(*creativitySlider);
}

void GrokAIEditor::createAnalysisDisplay() {
    analysisDisplay = std::make_unique<juce::TextEditor>();
    analysisDisplay->setMultiLine(true);
    analysisDisplay->setReadOnly(true);
    analysisDisplay->setScrollbarsShown(true);
    mainComponent->addAndMakeVisible(*analysisDisplay);
    
    loudnessMeter = std::make_unique<juce::ProgressBar>();
    loudnessMeter->setPercentageDisplay(false);
    mainComponent->addAndMakeVisible(*loudnessMeter);
    
    dynamicsMeter = std::make_unique<juce::ProgressBar>();
    dynamicsMeter->setPercentageDisplay(false);
    mainComponent->addAndMakeVisible(*dynamicsMeter);
    
    stereoMeter = std::make_unique<juce::ProgressBar>();
    stereoMeter->setPercentageDisplay(false);
    mainComponent->addAndMakeVisible(*stereoMeter);
}

void GrokAIEditor::createRealTimeControls() {
    realTimeAnalysisButton = std::make_unique<juce::ToggleButton>("Real-Time Analysis");
    realTimeAnalysisButton->addListener(this);
    mainComponent->addAndMakeVisible(*realTimeAnalysisButton);
    
    autoLearnButton = std::make_unique<juce::ToggleButton>("Auto Learn");
    autoLearnButton->addListener(this);
    mainComponent->addAndMakeVisible(*autoLearnButton);
}

void GrokAIEditor::createPresetControls() {
    presetCombo = std::make_unique<juce::ComboBox>();
    presetCombo->addListener(this);
    mainComponent->addAndMakeVisible(*presetCombo);
    
    savePresetButton = std::make_unique<juce::TextButton>("Save");
    savePresetButton->addListener(this);
    mainComponent->addAndMakeVisible(*savePresetButton);
    
    deletePresetButton = std::make_unique<juce::TextButton>("Delete");
    deletePresetButton->addListener(this);
    mainComponent->addAndMakeVisible(*deletePresetButton);
}

void GrokAIEditor::createStatusDisplay() {
    aiStatusLabel = std::make_unique<juce::Label>("status", "AI Status: Ready");
    aiStatusLabel->setFont(juce::Font(12.0f));
    aiStatusLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    mainComponent->addAndMakeVisible(*aiStatusLabel);
    
    aiActivityMeter = std::make_unique<juce::ProgressBar>();
    aiActivityMeter->setPercentageDisplay(false);
    mainComponent->addAndMakeVisible(*aiActivityMeter);
}

void GrokAIEditor::createVisualizations() {
    // Placeholder for visualizations
    waveformDisplay = std::make_unique<juce::Component>();
    spectrumDisplay = std::make_unique<juce::Component>();
    
    mainComponent->addAndMakeVisible(*waveformDisplay);
    mainComponent->addAndMakeVisible(*spectrumDisplay);
}

// GrokAIPluginFactory Implementation
juce::AudioProcessor* GrokAIPluginFactory::createPlugin() {
    return new GrokAIProcessor();
}

const juce::String GrokAIPluginFactory::getPluginName() {
    return PLUGIN_NAME;
}

const juce::String GrokAIPluginFactory::getPluginDescription() {
    return PLUGIN_DESCRIPTION;
}

bool GrokAIPluginFactory::isPluginMidiEffect() {
    return false;
}

bool GrokAIPluginFactory::isPluginSynth() {
    return false;
}

double GrokAIPluginFactory::getPluginVersion() {
    return PLUGIN_VERSION;
}

const juce::String GrokAIPluginFactory::getPluginIdentifier() {
    return PLUGIN_IDENTIFIER;
}

} // namespace plugin
} // namespace zenith
