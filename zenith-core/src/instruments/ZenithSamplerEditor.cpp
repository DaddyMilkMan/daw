/**
 * @file ZenithSamplerEditor.cpp
 * @brief Implementation of ZenithSamplerEditor
 */

#include "../../include/instruments/ZenithSamplerEditor.h"

namespace zenith {

//==============================================================================
ZenithSamplerEditor::ZenithSamplerEditor(ZenithSampler& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    // Set editor size
    setSize(600, 500);

    // Setup load sample button
    addAndMakeVisible(loadSampleButton_);
    loadSampleButton_.setButtonText("Load Sample...");
    loadSampleButton_.onClick = [this] { loadSampleFile(); };

    // Setup regular parameter controls
    setupSlider(sampleStartSlider_, sampleStartLabel_, "Start", "sample_start");
    setupSlider(sampleEndSlider_, sampleEndLabel_, "End", "sample_end");
    setupSlider(filterCutoffSlider_, filterCutoffLabel_, "Cutoff", "filter_cutoff");
    setupSlider(filterResonanceSlider_, filterResonanceLabel_, "Resonance", "filter_resonance");
    setupSlider(attackSlider_, attackLabel_, "Attack", "env_attack");
    setupSlider(decaySlider_, decayLabel_, "Decay", "env_decay");
    setupSlider(sustainSlider_, sustainLabel_, "Sustain", "env_sustain");
    setupSlider(releaseSlider_, releaseLabel_, "Release", "env_release");
    setupSlider(volumeSlider_, volumeLabel_, "Volume", "master_volume");

    // Setup macro knobs
    setupMacroKnobs();
}

//==============================================================================
void ZenithSamplerEditor::setupSlider(juce::Slider& slider, juce::Label& label,
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
    attachments_.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor_.getParameters(), paramId, slider));
}

//==============================================================================
void ZenithSamplerEditor::setupMacroKnobs()
{
    const auto& metadata = processor_.getInstrumentMetadata();

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

        // Create attachment
        auto macroParamId = "macro_" + std::to_string(i);
        attachments_.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor_.getParameters(), macroParamId, *knob));

        // Store knob and label
        macroKnobs_.push_back(std::move(knob));
        macroLabels_.push_back(std::move(label));
    }
}

//==============================================================================
void ZenithSamplerEditor::loadSampleFile()
{
    juce::FileChooser chooser("Select a sample file...",
                             juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                             "*.wav;*.aif;*.aiff;*.mp3;*.ogg;*.flac");

    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        processor_.loadSample(file);
    }
}

//==============================================================================
bool ZenithSamplerEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        if (file.endsWithIgnoreCase(".wav") ||
            file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff") ||
            file.endsWithIgnoreCase(".mp3") ||
            file.endsWithIgnoreCase(".ogg") ||
            file.endsWithIgnoreCase(".flac"))
        {
            return true;
        }
    }
    return false;
}

void ZenithSamplerEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.size() > 0)
    {
        juce::File file(files[0]);
        processor_.loadSample(file);
    }
}

//==============================================================================
void ZenithSamplerEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("Zenith Sampler", 0, 10, getWidth(), 30, juce::Justification::centred);

    // Section labels
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xffaaaaaa));
    g.drawText("SAMPLE", 20, 90, 160, 20, juce::Justification::centredLeft);
    g.drawText("FILTER", 200, 90, 160, 20, juce::Justification::centredLeft);
    g.drawText("ENVELOPE", 20, 230, 200, 20, juce::Justification::centredLeft);

    // Macro section
    g.setColour(juce::Colour(0xff4a9eff));
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("SMART MACROS", 0, 360, getWidth(), 25, juce::Justification::centred);

    // Separator line
    g.setColour(juce::Colour(0xff333333));
    g.fillRect(20, 355, getWidth() - 40, 2);

    // Drag and drop hint
    g.setColour(juce::Colour(0xff666666));
    g.setFont(juce::Font(12.0f));
    g.drawText("Drag & drop audio files here", 0, 50, getWidth(), 20, juce::Justification::centred);
}

//==============================================================================
void ZenithSamplerEditor::resized()
{
    const int knobSize = 80;
    const int labelHeight = 20;
    const int spacing = 20;

    // Load sample button
    loadSampleButton_.setBounds(20, 50, 120, 30);

    // Sample section
    sampleStartSlider_.setBounds(20, 130, knobSize, knobSize);
    sampleEndSlider_.setBounds(120, 130, knobSize, knobSize);

    // Filter section
    filterCutoffSlider_.setBounds(220, 130, knobSize, knobSize);
    filterResonanceSlider_.setBounds(320, 130, knobSize, knobSize);

    // Envelope section
    attackSlider_.setBounds(20, 270, knobSize, knobSize);
    decaySlider_.setBounds(120, 270, knobSize, knobSize);
    sustainSlider_.setBounds(220, 270, knobSize, knobSize);
    releaseSlider_.setBounds(320, 270, knobSize, knobSize);

    // Volume
    volumeSlider_.setBounds(500, 130, knobSize, knobSize);

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
