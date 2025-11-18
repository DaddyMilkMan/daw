/**
 * @file ZenithPolySynthEditor.cpp
 * @brief Implementation of ZenithPolySynthEditor
 */

#include "ZenithPolySynthEditor.h"

namespace zenith {

//==============================================================================
ZenithPolySynthEditor::ZenithPolySynthEditor(ZenithPolySynth& p)
    : processor_(p)
{
    // Set editor size
    setSize(600, 500);

    // Setup regular parameter controls
    setupSlider(waveformSlider_, waveformLabel_, "Waveform", "osc_wave");
    setupSlider(detuneSlider_, detuneLabel_, "Detune", "osc_detune");
    setupSlider(filterCutoffSlider_, filterCutoffLabel_, "Cutoff", "filter_cutoff");
    setupSlider(filterResonanceSlider_, filterResonanceLabel_, "Resonance", "filter_resonance");
    setupSlider(attackSlider_, attackLabel_, "Attack", "amp_attack");
    setupSlider(decaySlider_, decayLabel_, "Decay", "amp_decay");
    setupSlider(sustainSlider_, sustainLabel_, "Sustain", "amp_sustain");
    setupSlider(releaseSlider_, releaseLabel_, "Release", "amp_release");
    setupSlider(volumeSlider_, volumeLabel_, "Volume", "master_volume");

    // Setup macro knobs
    setupMacroKnobs();
}

//==============================================================================
void ZenithPolySynthEditor::setupSlider(juce::Slider& slider, juce::Label& label,
                                       const juce::String& labelText, const juce::String& paramId)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);

    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);

    // Create attachment
    auto* audioProcessor = processor_.getAudioProcessor();
    if (auto* apvts = dynamic_cast<juce::AudioProcessorValueTreeState*>(audioProcessor))
    {
        attachments_.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            *apvts, paramId, slider));
    }
}

//==============================================================================
void ZenithPolySynthEditor::setupMacroKnobs()
{
    const auto& metadata = processor_.getMetadata();

    for (size_t i = 0; i < metadata.macros.size(); ++i)
    {
        const auto& macroInfo = metadata.macros[i];

        // Create knob
        auto knob = std::make_unique<juce::Slider>();
        knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(*knob);

        // Create label
        auto label = std::make_unique<juce::Label>();
        label->setText(macroInfo.name, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->attachToComponent(knob.get(), false);
        addAndMakeVisible(*label);

        // Create attachment using actual macro ID from metadata
        auto* audioProcessor = processor_.getAudioProcessor();
        if (auto* apvts = dynamic_cast<juce::AudioProcessorValueTreeState*>(audioProcessor))
        {
            attachments_.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                *apvts, macroInfo.id, *knob));
        }

        // Store knob and label
        macroKnobs_.push_back(std::move(knob));
        macroLabels_.push_back(std::move(label));
    }
}

//==============================================================================
void ZenithPolySynthEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("Zenith PolySynth", 0, 10, getWidth(), 30, juce::Justification::centred);

    // Section labels
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffaaaaaa));
    g.drawText("OSCILLATOR", 20, 60, 160, 20, juce::Justification::centredLeft);
    g.drawText("FILTER", 200, 60, 160, 20, juce::Justification::centredLeft);
    g.drawText("ENVELOPE", 20, 200, 200, 20, juce::Justification::centredLeft);

    // Macro section
    g.setColour(juce::Colour(0xff4a9eff));
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("SMART MACROS", 0, 360, getWidth(), 25, juce::Justification::centred);

    // Separator line
    g.setColour(juce::Colour(0xff333333));
    g.fillRect(20, 355, getWidth() - 40, 2);
}

//==============================================================================
void ZenithPolySynthEditor::resized()
{
    const int knobSize = 80;
    const int labelHeight = 20;
    const int spacing = 20;

    // Oscillator section
    waveformSlider_.setBounds(20, 100, knobSize, knobSize);
    detuneSlider_.setBounds(120, 100, knobSize, knobSize);

    // Filter section
    filterCutoffSlider_.setBounds(220, 100, knobSize, knobSize);
    filterResonanceSlider_.setBounds(320, 100, knobSize, knobSize);

    // Envelope section
    attackSlider_.setBounds(20, 240, knobSize, knobSize);
    decaySlider_.setBounds(120, 240, knobSize, knobSize);
    sustainSlider_.setBounds(220, 240, knobSize, knobSize);
    releaseSlider_.setBounds(320, 240, knobSize, knobSize);

    // Volume
    volumeSlider_.setBounds(500, 100, knobSize, knobSize);

    // Macro knobs at bottom - centered and evenly spaced
    const int macroKnobSize = 90;
    const int macroSpacing = 30;
    const int totalMacroWidth = (macroKnobSize * 4) + (macroSpacing * 3);
    const int macroStartX = (getWidth() - totalMacroWidth) / 2;
    const int macroY = 390;

    for (size_t i = 0; i < macroKnobs_.size(); ++i)
    {
        int x = macroStartX + i * (macroKnobSize + macroSpacing);
        macroKnobs_[i]->setBounds(x, macroY, macroKnobSize, macroKnobSize);
    }
}

} // namespace zenith
