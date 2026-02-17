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

} // namespace
