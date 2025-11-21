/**
 * @file ZenithPolySynthEditor.cpp
 * @brief Implementation of comprehensive ZenithPolySynthEditor
 */

#include "ZenithPolySynthEditor.h"

namespace zenith {

//==============================================================================
// Constants for layout
//==============================================================================

namespace LayoutConstants
{
    constexpr int WINDOW_WIDTH = 900;
    constexpr int WINDOW_HEIGHT = 700;
    constexpr int MARGIN = 10;
    constexpr int GROUP_MARGIN = 8;
    constexpr int KNOB_SIZE = 60;
    constexpr int SLIDER_WIDTH = 80;
    constexpr int SLIDER_HEIGHT = 20;
    constexpr int LABEL_HEIGHT = 18;
    constexpr int BUTTON_HEIGHT = 25;
}

//==============================================================================
// Constructor
//==============================================================================

ZenithPolySynthEditor::ZenithPolySynthEditor(
    ZenithPolySynth& instrument,
    ZenithPresetManager& presetManager)
    : instrument_(instrument)
    , presetManager_(presetManager)
{
    setSize(LayoutConstants::WINDOW_WIDTH, LayoutConstants::WINDOW_HEIGHT);

    // Setup all sections
    setupPresetBrowser();
    setupOscillatorSection();
    setupFilterSection();
    setupEnvelopeSection();
    setupLFOSection();
    setupGlobalSection();
    setupMacroSection();
}

//==============================================================================
// Setup Methods
//==============================================================================

void ZenithPolySynthEditor::setupPresetBrowser()
{
    // Toggle button
    addAndMakeVisible(togglePresetBrowserButton_);
    togglePresetBrowserButton_.setButtonText(presetBrowserVisible_ ? "Hide Presets" : "Show Presets");
    togglePresetBrowserButton_.onClick = [this]
    {
        presetBrowserVisible_ = !presetBrowserVisible_;
        togglePresetBrowserButton_.setButtonText(presetBrowserVisible_ ? "Hide Presets" : "Show Presets");

        if (presetBrowser_)
            presetBrowser_->setVisible(presetBrowserVisible_);

        resized();
    };

    // Create preset browser
    presetBrowser_ = std::make_unique<PresetBrowserComponent>(
        instrument_.getMetadata().instrumentId,
        presetManager_);

    presetBrowser_->setLoadPresetCallback([this](const ZenithInstrumentPreset& preset)
    {
        onPresetLoaded(preset);
    });

    presetBrowser_->setCaptureStateCallback([this]
    {
        return captureCurrentState();
    });

    addAndMakeVisible(*presetBrowser_);
    presetBrowser_->setVisible(presetBrowserVisible_);
}

void ZenithPolySynthEditor::setupOscillatorSection()
{
    addAndMakeVisible(oscGroup_);
    oscGroup_.setText("Oscillator");
    oscGroup_.setTextLabelPosition(juce::Justification::centredTop);

    // Osc 1 Waveform (using existing parameter if available)
    addAndMakeVisible(osc1WaveCombo_);
    osc1WaveCombo_.addItem("Sine", 1);
    osc1WaveCombo_.addItem("Saw", 2);
    osc1WaveCombo_.addItem("Square", 3);
    osc1WaveCombo_.addItem("Triangle", 4);
    osc1WaveCombo_.setSelectedId(1);

    addAndMakeVisible(osc1WaveLabel_);
    osc1WaveLabel_.setText("Wave", juce::dontSendNotification);
    osc1WaveLabel_.setJustificationType(juce::Justification::centred);

    // Detune
    setupSlider(osc1DetuneSlider_, osc1DetuneLabel_, "Detune", "Fine pitch adjustment");
    osc1DetuneSlider_.setRange(0.0, 1.0, 0.01);

    // Level
    setupSlider(osc1LevelSlider_, osc1LevelLabel_, "Level", "Oscillator volume");
    osc1LevelSlider_.setRange(0.0, 1.0, 0.01);
    osc1LevelSlider_.setValue(1.0);

    // Unison
    setupSlider(unisonVoicesSlider_, unisonVoicesLabel_, "Unison", "Number of unison voices");
    unisonVoicesSlider_.setRange(1.0, 8.0, 1.0);
    unisonVoicesSlider_.setValue(1.0);

    setupSlider(unisonDetuneSlider_, unisonDetuneLabel_, "Spread", "Unison detune amount");
    unisonDetuneSlider_.setRange(0.0, 1.0, 0.01);

    // Placeholders for Osc 2 & 3
    addAndMakeVisible(osc2Label_);
    osc2Label_.setText("Osc 2/3: TODO", juce::dontSendNotification);
    osc2Label_.setJustificationType(juce::Justification::centred);
    osc2Label_.setColour(juce::Label::textColourId, juce::Colours::grey);
}

void ZenithPolySynthEditor::setupFilterSection()
{
    addAndMakeVisible(filterGroup_);
    filterGroup_.setText("Filter");
    filterGroup_.setTextLabelPosition(juce::Justification::centredTop);

    // Filter Type
    addAndMakeVisible(filterTypeCombo_);
    filterTypeCombo_.addItem("Low Pass", 1);
    filterTypeCombo_.addItem("High Pass", 2);
    filterTypeCombo_.addItem("Band Pass", 3);
    filterTypeCombo_.addItem("Notch", 4);
    filterTypeCombo_.setSelectedId(1);

    addAndMakeVisible(filterTypeLabel_);
    filterTypeLabel_.setText("Type", juce::dontSendNotification);
    filterTypeLabel_.setJustificationType(juce::Justification::centred);

    // Cutoff
    setupSlider(filterCutoffSlider_, filterCutoffLabel_, "Cutoff", "Filter cutoff frequency");
    filterCutoffSlider_.setRange(0.0, 1.0, 0.01);
    filterCutoffSlider_.setValue(0.8);

    // Resonance
    setupSlider(filterResonanceSlider_, filterResonanceLabel_, "Resonance", "Filter resonance");
    filterResonanceSlider_.setRange(0.0, 1.0, 0.01);
    filterResonanceSlider_.setValue(0.5);

    // Drive
    setupSlider(filterDriveSlider_, filterDriveLabel_, "Drive", "Pre-filter drive/saturation");
    filterDriveSlider_.setRange(0.0, 1.0, 0.01);
}

void ZenithPolySynthEditor::setupEnvelopeSection()
{
    addAndMakeVisible(envGroup_);
    envGroup_.setText("Envelopes");
    envGroup_.setTextLabelPosition(juce::Justification::centredTop);

    // Amp Envelope Label
    addAndMakeVisible(ampEnvLabel_);
    ampEnvLabel_.setText("Amp ADSR", juce::dontSendNotification);
    ampEnvLabel_.setJustificationType(juce::Justification::centred);
    ampEnvLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    // Amp ADSR
    setupSlider(ampAttackSlider_, ampAttackLabel_, "A", "Amp attack time");
    ampAttackSlider_.setRange(0.0, 1.0, 0.01);
    ampAttackSlider_.setValue(0.1);

    setupSlider(ampDecaySlider_, ampDecayLabel_, "D", "Amp decay time");
    ampDecaySlider_.setRange(0.0, 1.0, 0.01);
    ampDecaySlider_.setValue(0.2);

    setupSlider(ampSustainSlider_, ampSustainLabel_, "S", "Amp sustain level");
    ampSustainSlider_.setRange(0.0, 1.0, 0.01);
    ampSustainSlider_.setValue(0.7);

    setupSlider(ampReleaseSlider_, ampReleaseLabel_, "R", "Amp release time");
    ampReleaseSlider_.setRange(0.0, 1.0, 0.01);
    ampReleaseSlider_.setValue(0.3);

    // Filter Envelope Label
    addAndMakeVisible(filterEnvLabel_);
    filterEnvLabel_.setText("Filter ADSR", juce::dontSendNotification);
    filterEnvLabel_.setJustificationType(juce::Justification::centred);
    filterEnvLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    // Filter ADSR
    setupSlider(filterEnvAttackSlider_, filterEnvAttackLabel_, "A", "Filter envelope attack");
    filterEnvAttackSlider_.setRange(0.0, 1.0, 0.01);

    setupSlider(filterEnvDecaySlider_, filterEnvDecayLabel_, "D", "Filter envelope decay");
    filterEnvDecaySlider_.setRange(0.0, 1.0, 0.01);

    setupSlider(filterEnvSustainSlider_, filterEnvSustainLabel_, "S", "Filter envelope sustain");
    filterEnvSustainSlider_.setRange(0.0, 1.0, 0.01);

    setupSlider(filterEnvReleaseSlider_, filterEnvReleaseLabel_, "R", "Filter envelope release");
    filterEnvReleaseSlider_.setRange(0.0, 1.0, 0.01);

    setupSlider(filterEnvAmountSlider_, filterEnvAmountLabel_, "Amt", "Filter envelope amount");
    filterEnvAmountSlider_.setRange(0.0, 1.0, 0.01);
}

void ZenithPolySynthEditor::setupLFOSection()
{
    addAndMakeVisible(lfoGroup_);
    lfoGroup_.setText("LFOs");
    lfoGroup_.setTextLabelPosition(juce::Justification::centredTop);

    // LFO 1
    addAndMakeVisible(lfo1Label_);
    lfo1Label_.setText("LFO 1", juce::dontSendNotification);
    lfo1Label_.setJustificationType(juce::Justification::centred);
    lfo1Label_.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    addAndMakeVisible(lfo1WaveCombo_);
    lfo1WaveCombo_.addItem("Sine", 1);
    lfo1WaveCombo_.addItem("Triangle", 2);
    lfo1WaveCombo_.addItem("Saw", 3);
    lfo1WaveCombo_.addItem("Square", 4);
    lfo1WaveCombo_.setSelectedId(1);

    addAndMakeVisible(lfo1WaveLabel_);
    lfo1WaveLabel_.setText("Wave", juce::dontSendNotification);
    lfo1WaveLabel_.setJustificationType(juce::Justification::centred);

    setupSlider(lfo1RateSlider_, lfo1RateLabel_, "Rate", "LFO 1 frequency");
    lfo1RateSlider_.setRange(0.0, 1.0, 0.01);

    addAndMakeVisible(lfo1TargetCombo_);
    lfo1TargetCombo_.addItem("Cutoff", 1);
    lfo1TargetCombo_.addItem("Resonance", 2);
    lfo1TargetCombo_.addItem("Pitch", 3);
    lfo1TargetCombo_.addItem("Amp", 4);
    lfo1TargetCombo_.setSelectedId(1);

    addAndMakeVisible(lfo1TargetLabel_);
    lfo1TargetLabel_.setText("Target", juce::dontSendNotification);
    lfo1TargetLabel_.setJustificationType(juce::Justification::centred);

    setupSlider(lfo1AmountSlider_, lfo1AmountLabel_, "Amt", "LFO 1 modulation amount");
    lfo1AmountSlider_.setRange(0.0, 1.0, 0.01);

    // LFO 2
    addAndMakeVisible(lfo2Label_);
    lfo2Label_.setText("LFO 2", juce::dontSendNotification);
    lfo2Label_.setJustificationType(juce::Justification::centred);
    lfo2Label_.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    addAndMakeVisible(lfo2WaveCombo_);
    lfo2WaveCombo_.addItem("Sine", 1);
    lfo2WaveCombo_.addItem("Triangle", 2);
    lfo2WaveCombo_.addItem("Saw", 3);
    lfo2WaveCombo_.addItem("Square", 4);
    lfo2WaveCombo_.setSelectedId(1);

    addAndMakeVisible(lfo2WaveLabel_);
    lfo2WaveLabel_.setText("Wave", juce::dontSendNotification);
    lfo2WaveLabel_.setJustificationType(juce::Justification::centred);

    setupSlider(lfo2RateSlider_, lfo2RateLabel_, "Rate", "LFO 2 frequency");
    lfo2RateSlider_.setRange(0.0, 1.0, 0.01);

    addAndMakeVisible(lfo2TargetCombo_);
    lfo2TargetCombo_.addItem("Cutoff", 1);
    lfo2TargetCombo_.addItem("Resonance", 2);
    lfo2TargetCombo_.addItem("Pitch", 3);
    lfo2TargetCombo_.addItem("Amp", 4);
    lfo2TargetCombo_.setSelectedId(2);

    addAndMakeVisible(lfo2TargetLabel_);
    lfo2TargetLabel_.setText("Target", juce::dontSendNotification);
    lfo2TargetLabel_.setJustificationType(juce::Justification::centred);

    setupSlider(lfo2AmountSlider_, lfo2AmountLabel_, "Amt", "LFO 2 modulation amount");
    lfo2AmountSlider_.setRange(0.0, 1.0, 0.01);
}

void ZenithPolySynthEditor::setupGlobalSection()
{
    addAndMakeVisible(globalGroup_);
    globalGroup_.setText("Global");
    globalGroup_.setTextLabelPosition(juce::Justification::centredTop);

    // Master Gain
    setupSlider(masterGainSlider_, masterGainLabel_, "Gain", "Master output level");
    masterGainSlider_.setRange(0.0, 1.0, 0.01);
    masterGainSlider_.setValue(0.8);

    // Mono Mode
    addAndMakeVisible(monoModeButton_);
    monoModeButton_.setButtonText("Mono");
    monoModeButton_.setTooltip("Enable monophonic mode");

    addAndMakeVisible(monoModeLabel_);
    monoModeLabel_.setText("Mode", juce::dontSendNotification);
    monoModeLabel_.setJustificationType(juce::Justification::centred);

    // Glide
    setupSlider(glideSlider_, glideLabel_, "Glide", "Portamento time");
    glideSlider_.setRange(0.0, 1.0, 0.01);

    // Voices
    setupSlider(voicesSlider_, voicesLabel_, "Voices", "Maximum polyphony");
    voicesSlider_.setRange(1.0, 16.0, 1.0);
    voicesSlider_.setValue(8.0);
}

void ZenithPolySynthEditor::setupMacroSection()
{
    const auto& metadata = instrument_.getMetadata();

    for (const auto& macroInfo : metadata.macros)
    {
        // Create knob
        auto knob = std::make_unique<juce::Slider>();
        knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        knob->setRange(0.0, 1.0, 0.01);
        knob->setValue(0.5);
        knob->setTooltip(macroInfo.description);
        addAndMakeVisible(*knob);

        // Create label
        auto label = std::make_unique<juce::Label>();
        label->setText(macroInfo.name, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::FontOptions(12.0f, juce::Font::bold));
        addAndMakeVisible(*label);

        // Store knob and label
        macroKnobs_.push_back(std::move(knob));
        macroLabels_.push_back(std::move(label));
    }
}

void ZenithPolySynthEditor::setupSlider(
    juce::Slider& slider,
    juce::Label& label,
    const juce::String& labelText,
    const juce::String& tooltip)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);

    if (tooltip.isNotEmpty())
        slider.setTooltip(tooltip);

    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
}

//==============================================================================
// Preset Management
//==============================================================================

void ZenithPolySynthEditor::onPresetLoaded(const ZenithInstrumentPreset& preset)
{
    // Apply all parameters from preset
    for (const auto& [paramId, value] : preset.parameters)
    {
        instrument_.setParameter(juce::String(paramId), value);
    }

    // Apply macros
    for (const auto& [macroId, value] : preset.macros)
    {
        instrument_.setMacro(juce::String(macroId), value);
    }
}

std::map<std::string, float, std::less<>> ZenithPolySynthEditor::captureCurrentState() const
{
    std::map<std::string, float, std::less<>> state;

    // Capture all parameters from metadata
    const auto& metadata = instrument_.getMetadata();
    for (const auto& param : metadata.parameters)
    {
        float value = instrument_.getParameter(param.id);
        state[param.id.toStdString()] = value;
    }

    return state;
}

//==============================================================================
// Paint & Resized
//==============================================================================

void ZenithPolySynthEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("Zenith PolySynth", 0, 10, getWidth(), 35, juce::Justification::centred);

    // Macro section header
    if (!macroKnobs_.empty())
    {
        g.setColour(juce::Colour(0xff4a9eff));
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("SMART MACROS", 0, getHeight() - 140, getWidth(), 20, juce::Justification::centred);

        // Separator line
        g.setColour(juce::Colour(0xff333333));
        g.fillRect(20, getHeight() - 145, getWidth() - 40, 2);
    }
}

void ZenithPolySynthEditor::resized()
{
    auto bounds = getLocalBounds().reduced(LayoutConstants::MARGIN);

    // Top: Title + Toggle Button
    auto topRow = bounds.removeFromTop(40);
    topRow.removeFromTop(10); // Title space
    togglePresetBrowserButton_.setBounds(topRow.removeFromRight(120).reduced(2));

    bounds.removeFromTop(5);

    // Preset browser (left side, if visible)
    int presetBrowserWidth = 250;
    if (presetBrowserVisible_ && presetBrowser_)
    {
        presetBrowser_->setBounds(bounds.removeFromLeft(presetBrowserWidth));
        bounds.removeFromLeft(LayoutConstants::MARGIN);
    }

    // Main content area
    auto mainArea = bounds;

    // Reserve space for macros at bottom
    int macroHeight = 120;
    auto macroArea = mainArea.removeFromBottom(macroHeight);
    mainArea.removeFromBottom(10); // Spacing

    // Layout main sections in a grid
    int row1Height = 140;
    int row2Height = 180;
    // // int row3Height = mainArea.getHeight() - row1Height - row2Height - 20;  // Unused variable  // Unused

    // Row 1: Oscillator + Filter
    auto row1 = mainArea.removeFromTop(row1Height);
    {
        int oscWidth = row1.getWidth() / 2;
        auto oscBounds = row1.removeFromLeft(oscWidth);
        oscBounds.removeFromRight(5);
        oscGroup_.setBounds(oscBounds);

        auto oscContent = oscBounds.reduced(LayoutConstants::GROUP_MARGIN);
        oscContent.removeFromTop(20); // Group label

        // Layout oscillator controls in a row
        int knobW = LayoutConstants::KNOB_SIZE;
        osc1WaveCombo_.setBounds(oscContent.removeFromLeft(knobW).removeFromTop(25));
        osc1WaveLabel_.setBounds(oscContent.removeFromLeft(knobW).removeFromTop(20));
        oscContent.removeFromLeft(5);

        osc1DetuneSlider_.setBounds(oscContent.removeFromLeft(knobW).removeFromTop(knobW + 20));
        osc1DetuneLabel_.setBounds(osc1DetuneSlider_.getBounds().removeFromBottom(20));
        oscContent.removeFromLeft(5);

        osc1LevelSlider_.setBounds(oscContent.removeFromLeft(knobW).removeFromTop(knobW + 20));
        oscContent.removeFromLeft(5);

        unisonVoicesSlider_.setBounds(oscContent.removeFromLeft(knobW).removeFromTop(knobW + 20));
        oscContent.removeFromLeft(5);

        unisonDetuneSlider_.setBounds(oscContent.removeFromLeft(knobW).removeFromTop(knobW + 20));

        row1.removeFromLeft(5);
        auto filterBounds = row1;
        filterGroup_.setBounds(filterBounds);

        auto filterContent = filterBounds.reduced(LayoutConstants::GROUP_MARGIN);
        filterContent.removeFromTop(20); // Group label

        filterTypeCombo_.setBounds(filterContent.removeFromLeft(knobW).removeFromTop(25));
        filterTypeLabel_.setBounds(filterContent.removeFromLeft(knobW).removeFromTop(20));
        filterContent.removeFromLeft(5);

        filterCutoffSlider_.setBounds(filterContent.removeFromLeft(knobW).removeFromTop(knobW + 20));
        filterContent.removeFromLeft(5);

        filterResonanceSlider_.setBounds(filterContent.removeFromLeft(knobW).removeFromTop(knobW + 20));
        filterContent.removeFromLeft(5);

        filterDriveSlider_.setBounds(filterContent.removeFromLeft(knobW).removeFromTop(knobW + 20));
    }

    mainArea.removeFromTop(10);

    // Row 2: Envelopes
    auto row2 = mainArea.removeFromTop(row2Height);
    {
        envGroup_.setBounds(row2);

        auto envContent = row2.reduced(LayoutConstants::GROUP_MARGIN);
        envContent.removeFromTop(20); // Group label

        // Amp envelope
        auto ampEnvArea = envContent.removeFromTop(70);
        ampEnvLabel_.setBounds(ampEnvArea.removeFromTop(20));

        int knobW = LayoutConstants::KNOB_SIZE;
        ampAttackSlider_.setBounds(ampEnvArea.removeFromLeft(knobW).removeFromTop(knobW));
        ampEnvArea.removeFromLeft(5);
        ampDecaySlider_.setBounds(ampEnvArea.removeFromLeft(knobW).removeFromTop(knobW));
        ampEnvArea.removeFromLeft(5);
        ampSustainSlider_.setBounds(ampEnvArea.removeFromLeft(knobW).removeFromTop(knobW));
        ampEnvArea.removeFromLeft(5);
        ampReleaseSlider_.setBounds(ampEnvArea.removeFromLeft(knobW).removeFromTop(knobW));

        envContent.removeFromTop(10);

        // Filter envelope
        auto filterEnvArea = envContent;
        filterEnvLabel_.setBounds(filterEnvArea.removeFromTop(20));

        filterEnvAttackSlider_.setBounds(filterEnvArea.removeFromLeft(knobW).removeFromTop(knobW));
        filterEnvArea.removeFromLeft(5);
        filterEnvDecaySlider_.setBounds(filterEnvArea.removeFromLeft(knobW).removeFromTop(knobW));
        filterEnvArea.removeFromLeft(5);
        filterEnvSustainSlider_.setBounds(filterEnvArea.removeFromLeft(knobW).removeFromTop(knobW));
        filterEnvArea.removeFromLeft(5);
        filterEnvReleaseSlider_.setBounds(filterEnvArea.removeFromLeft(knobW).removeFromTop(knobW));
        filterEnvArea.removeFromLeft(5);
        filterEnvAmountSlider_.setBounds(filterEnvArea.removeFromLeft(knobW).removeFromTop(knobW));
    }

    mainArea.removeFromTop(10);

    // Row 3: LFOs + Global
    auto row3 = mainArea;
    {
        int lfoWidth = row3.getWidth() * 2 / 3;
        auto lfoBounds = row3.removeFromLeft(lfoWidth);
        lfoBounds.removeFromRight(5);
        lfoGroup_.setBounds(lfoBounds);

        auto lfoContent = lfoBounds.reduced(LayoutConstants::GROUP_MARGIN);
        lfoContent.removeFromTop(20); // Group label

        // LFO 1
        auto lfo1Area = lfoContent.removeFromTop(lfoContent.getHeight() / 2);
        lfo1Label_.setBounds(lfo1Area.removeFromTop(20));

        int knobW = LayoutConstants::KNOB_SIZE;
        lfo1WaveCombo_.setBounds(lfo1Area.removeFromLeft(knobW).removeFromTop(25));
        lfo1WaveLabel_.setBounds(lfo1Area.removeFromLeft(knobW).removeFromTop(20));
        lfo1Area.removeFromLeft(5);

        lfo1RateSlider_.setBounds(lfo1Area.removeFromLeft(knobW).removeFromTop(knobW));
        lfo1Area.removeFromLeft(5);

        lfo1TargetCombo_.setBounds(lfo1Area.removeFromLeft(knobW).removeFromTop(25));
        lfo1TargetLabel_.setBounds(lfo1Area.removeFromLeft(knobW).removeFromTop(20));
        lfo1Area.removeFromLeft(5);

        lfo1AmountSlider_.setBounds(lfo1Area.removeFromLeft(knobW).removeFromTop(knobW));

        lfoContent.removeFromTop(10);

        // LFO 2
        auto lfo2Area = lfoContent;
        lfo2Label_.setBounds(lfo2Area.removeFromTop(20));

        lfo2WaveCombo_.setBounds(lfo2Area.removeFromLeft(knobW).removeFromTop(25));
        lfo2WaveLabel_.setBounds(lfo2Area.removeFromLeft(knobW).removeFromTop(20));
        lfo2Area.removeFromLeft(5);

        lfo2RateSlider_.setBounds(lfo2Area.removeFromLeft(knobW).removeFromTop(knobW));
        lfo2Area.removeFromLeft(5);

        lfo2TargetCombo_.setBounds(lfo2Area.removeFromLeft(knobW).removeFromTop(25));
        lfo2TargetLabel_.setBounds(lfo2Area.removeFromLeft(knobW).removeFromTop(20));
        lfo2Area.removeFromLeft(5);

        lfo2AmountSlider_.setBounds(lfo2Area.removeFromLeft(knobW).removeFromTop(knobW));

        // Global section
        row3.removeFromLeft(5);
        auto globalBounds = row3;
        globalGroup_.setBounds(globalBounds);

        auto globalContent = globalBounds.reduced(LayoutConstants::GROUP_MARGIN);
        globalContent.removeFromTop(20); // Group label

        masterGainSlider_.setBounds(globalContent.removeFromTop(knobW + 20));
        globalContent.removeFromTop(10);

        monoModeButton_.setBounds(globalContent.removeFromTop(25));
        monoModeLabel_.setBounds(globalContent.removeFromTop(20));
        globalContent.removeFromTop(10);

        glideSlider_.setBounds(globalContent.removeFromTop(knobW + 20));
        globalContent.removeFromTop(10);

        voicesSlider_.setBounds(globalContent.removeFromTop(knobW + 20));
    }

    // Macros at bottom (centered)
    if (!macroKnobs_.empty())
    {
        macroArea.removeFromTop(25); // Header space

        int macroKnobSize = 80;
        int macroSpacing = 20;
        int totalMacroWidth = (macroKnobSize * (int)macroKnobs_.size()) +
                             (macroSpacing * ((int)macroKnobs_.size() - 1));
        int macroStartX = (macroArea.getWidth() - totalMacroWidth) / 2;

        for (size_t i = 0; i < macroKnobs_.size(); ++i)
        {
            int x = macroStartX + (int)i * (macroKnobSize + macroSpacing);
            auto macroBounds = macroArea.withX(x).withWidth(macroKnobSize).withHeight(macroKnobSize);
            macroKnobs_[i]->setBounds(macroBounds);
            macroLabels_[i]->setBounds(macroBounds.removeFromBottom(20));
        }
    }
}

} // namespace zenith

