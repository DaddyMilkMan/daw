#include "instruments/ZenithSamplerEditor.h"

namespace zenith {
namespace instruments {

//==============================================================================
// Constants
//==============================================================================

namespace
{
    constexpr int EDITOR_WIDTH = 800;
    constexpr int EDITOR_HEIGHT = 400;
    constexpr int MARGIN = 10;
    constexpr int LABEL_HEIGHT = 20;
    constexpr int SLIDER_HEIGHT = 60;

    const juce::Colour BACKGROUND_COLOUR = juce::Colour(0xff2a2a2a);
    const juce::Colour PANEL_COLOUR = juce::Colour(0xff353535);
    const juce::Colour TEXT_COLOUR = juce::Colour(0xffe0e0e0);
    const juce::Colour ACCENT_COLOUR = juce::Colour(0xff4a9eff);
}

//==============================================================================
// ZenithSamplerEditor
//==============================================================================

ZenithSamplerEditor::ZenithSamplerEditor(ZenithSampler& processor)
    : AudioProcessorEditor(&processor), sampler(processor)
{
    setSize(EDITOR_WIDTH, EDITOR_HEIGHT);

    // Setup look and feel
    getLookAndFeel().setColour(juce::Slider::thumbColourId, ACCENT_COLOUR);
    getLookAndFeel().setColour(juce::Slider::trackColourId, ACCENT_COLOUR.withAlpha(0.3f));
    getLookAndFeel().setColour(juce::Label::textColourId, TEXT_COLOUR);
    getLookAndFeel().setColour(juce::ComboBox::textColourId, TEXT_COLOUR);

    //==========================================================================
    // Preset Section
    //==========================================================================

    presetGroup.setText("Preset");
    presetGroup.setColour(juce::GroupComponent::outlineColourId, PANEL_COLOUR.brighter(0.2f));
    presetGroup.setColour(juce::GroupComponent::textColourId, TEXT_COLOUR);
    addAndMakeVisible(presetGroup);

    presetLabel.setText("Patch:", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    presetComboBox.onChange = [this] { onPatchSelected(); };
    addAndMakeVisible(presetComboBox);

    statusLabel.setText("No patch loaded", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, TEXT_COLOUR.withAlpha(0.7f));
    addAndMakeVisible(statusLabel);

    //==========================================================================
    // Envelope Section
    //==========================================================================

    envelopeGroup.setText("Amp Envelope");
    envelopeGroup.setColour(juce::GroupComponent::outlineColourId, PANEL_COLOUR.brighter(0.2f));
    envelopeGroup.setColour(juce::GroupComponent::textColourId, TEXT_COLOUR);
    addAndMakeVisible(envelopeGroup);

    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        slider.setColour(juce::Slider::textBoxTextColourId, TEXT_COLOUR);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);
        addAndMakeVisible(label);
    };

    setupSlider(attackSlider, attackLabel, "Attack");
    setupSlider(decaySlider, decayLabel, "Decay");
    setupSlider(sustainSlider, sustainLabel, "Sustain");
    setupSlider(releaseSlider, releaseLabel, "Release");

    //==========================================================================
    // Filter Section
    //==========================================================================

    filterGroup.setText("Filter");
    filterGroup.setColour(juce::GroupComponent::outlineColourId, PANEL_COLOUR.brighter(0.2f));
    filterGroup.setColour(juce::GroupComponent::textColourId, TEXT_COLOUR);
    addAndMakeVisible(filterGroup);

    setupSlider(filterCutoffSlider, filterCutoffLabel, "Cutoff");
    setupSlider(filterResonanceSlider, filterResonanceLabel, "Resonance");

    //==========================================================================
    // Global Controls Section
    //==========================================================================

    globalGroup.setText("Global");
    globalGroup.setColour(juce::GroupComponent::outlineColourId, PANEL_COLOUR.brighter(0.2f));
    globalGroup.setColour(juce::GroupComponent::textColourId, TEXT_COLOUR);
    addAndMakeVisible(globalGroup);

    setupSlider(tuneSlider, tuneLabel, "Tune");
    setupSlider(gainSlider, gainLabel, "Gain");
    setupSlider(characterSlider, characterLabel, "Character");

    //==========================================================================
    // Parameter Attachments
    //==========================================================================

    auto& params = sampler.getParameters();

    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "attack", attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "decay", decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "sustain", sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "release", releaseSlider);
    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "filterCutoff", filterCutoffSlider);
    filterResonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "filterResonance", filterResonanceSlider);
    tuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "tune", tuneSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "gain", gainSlider);
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "character", characterSlider);

    //==========================================================================
    // Initialize
    //==========================================================================

    updatePatchList();
    startTimerHz(10); // Update status at 10Hz
}

ZenithSamplerEditor::~ZenithSamplerEditor()
{
    stopTimer();
}

//==============================================================================
// Layout
//==============================================================================

void ZenithSamplerEditor::paint(juce::Graphics& g)
{
    g.fillAll(BACKGROUND_COLOUR);
}

void ZenithSamplerEditor::resized()
{
    auto bounds = getLocalBounds().reduced(MARGIN);

    // Top section: Preset selector
    auto presetArea = bounds.removeFromTop(80);
    presetGroup.setBounds(presetArea);

    auto presetContent = presetArea.reduced(MARGIN);
    presetContent.removeFromTop(20); // For group label

    auto presetRow = presetContent.removeFromTop(30);
    presetLabel.setBounds(presetRow.removeFromLeft(60));
    presetRow.removeFromLeft(5);
    presetComboBox.setBounds(presetRow.removeFromLeft(200));

    statusLabel.setBounds(presetContent);

    bounds.removeFromTop(MARGIN);

    // Middle section: Envelope and Filter
    auto middleArea = bounds.removeFromTop(160);

    auto envelopeArea = middleArea.removeFromLeft(400);
    envelopeGroup.setBounds(envelopeArea);

    auto envelopeContent = envelopeArea.reduced(MARGIN);
    envelopeContent.removeFromTop(20); // For group label

    auto envelopeWidth = envelopeContent.getWidth() / 4;
    attackSlider.setBounds(envelopeContent.removeFromLeft(envelopeWidth).reduced(5));
    decaySlider.setBounds(envelopeContent.removeFromLeft(envelopeWidth).reduced(5));
    sustainSlider.setBounds(envelopeContent.removeFromLeft(envelopeWidth).reduced(5));
    releaseSlider.setBounds(envelopeContent.removeFromLeft(envelopeWidth).reduced(5));

    middleArea.removeFromLeft(MARGIN);

    filterGroup.setBounds(middleArea);
    auto filterContent = middleArea.reduced(MARGIN);
    filterContent.removeFromTop(20); // For group label

    auto filterWidth = filterContent.getWidth() / 2;
    filterCutoffSlider.setBounds(filterContent.removeFromLeft(filterWidth).reduced(5));
    filterResonanceSlider.setBounds(filterContent.removeFromLeft(filterWidth).reduced(5));

    bounds.removeFromTop(MARGIN);

    // Bottom section: Global controls
    globalGroup.setBounds(bounds);
    auto globalContent = bounds.reduced(MARGIN);
    globalContent.removeFromTop(20); // For group label

    auto globalWidth = globalContent.getWidth() / 3;
    tuneSlider.setBounds(globalContent.removeFromLeft(globalWidth).reduced(5));
    gainSlider.setBounds(globalContent.removeFromLeft(globalWidth).reduced(5));
    characterSlider.setBounds(globalContent.removeFromLeft(globalWidth).reduced(5));
}

//==============================================================================
// Patch Management
//==============================================================================

void ZenithSamplerEditor::updatePatchList()
{
    presetComboBox.clear(juce::dontSendNotification);

    auto patches = sampler.getAvailablePatches();

    if (patches.isEmpty())
    {
        presetComboBox.addItem("No patches found", 1);
        presetComboBox.setSelectedId(1, juce::dontSendNotification);
        statusLabel.setText("No patches available. See documentation for setup.", juce::dontSendNotification);
        return;
    }

    for (int i = 0; i < patches.size(); ++i)
    {
        presetComboBox.addItem(patches[i], i + 1);
    }

    // Select current patch if loaded
    auto currentPatch = sampler.getCurrentPatchName();
    if (currentPatch.isNotEmpty())
    {
        int index = patches.indexOf(currentPatch);
        if (index >= 0)
        {
            presetComboBox.setSelectedId(index + 1, juce::dontSendNotification);
        }
    }
}

void ZenithSamplerEditor::onPatchSelected()
{
    auto selectedId = presetComboBox.getSelectedId();
    if (selectedId == 0)
        return;

    auto patchName = presetComboBox.getText();
    if (patchName.isNotEmpty() && patchName != "No patches found")
    {
        sampler.loadPatchByName(patchName);
    }
}

void ZenithSamplerEditor::timerCallback()
{
    // Update status
    if (sampler.isLoading())
    {
        statusLabel.setText("Loading patch...", juce::dontSendNotification);
    }
    else
    {
        auto patchName = sampler.getCurrentPatchName();
        if (patchName.isNotEmpty())
        {
            statusLabel.setText("Patch loaded: " + patchName, juce::dontSendNotification);
        }
        else
        {
            statusLabel.setText("No patch loaded", juce::dontSendNotification);
        }
    }
}

} // namespace instruments
} // namespace zenith
