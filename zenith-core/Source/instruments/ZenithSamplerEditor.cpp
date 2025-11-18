#include "instruments/ZenithSamplerEditor.h"
#include "instruments/ZenithSampler.h"

namespace zenith {

//==============================================================================
ZenithSamplerEditor::ZenithSamplerEditor(ZenithSamplerProcessor& proc)
    : juce::AudioProcessorEditor(&proc),
      sampler(proc)
{
    setSize(700, 450);

    // Preset group
    addAndMakeVisible(presetGroup);
    presetGroup.setText("Patch");
    presetGroup.setTextLabelPosition(juce::Justification::centredTop);

    addAndMakeVisible(presetLabel);
    presetLabel.setText("Preset:", juce::dontSendNotification);

    addAndMakeVisible(presetComboBox);
    presetComboBox.onChange = [this] { onPatchSelected(); };

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);

    // Envelope group
    addAndMakeVisible(envelopeGroup);
    envelopeGroup.setText("Envelope");
    envelopeGroup.setTextLabelPosition(juce::Justification::centredTop);

    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        addAndMakeVisible(slider);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);

        addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.attachToComponent(&slider, false);
    };

    setupSlider(attackSlider, attackLabel, "Attack");
    setupSlider(decaySlider, decayLabel, "Decay");
    setupSlider(sustainSlider, sustainLabel, "Sustain");
    setupSlider(releaseSlider, releaseLabel, "Release");

    // Filter group
    addAndMakeVisible(filterGroup);
    filterGroup.setText("Filter");
    filterGroup.setTextLabelPosition(juce::Justification::centredTop);

    setupSlider(filterCutoffSlider, filterCutoffLabel, "Cutoff");
    setupSlider(filterResonanceSlider, filterResonanceLabel, "Resonance");

    // Global group
    addAndMakeVisible(globalGroup);
    globalGroup.setText("Global");
    globalGroup.setTextLabelPosition(juce::Justification::centredTop);

    setupSlider(tuneSlider, tuneLabel, "Tune");
    setupSlider(gainSlider, gainLabel, "Gain");
    setupSlider(characterSlider, characterLabel, "Character");

    // Create parameter attachments
    auto& apvts = sampler.getAPVTS();
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "attack", attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "decay", decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "sustain", sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "release", releaseSlider);
    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "filterCutoff", filterCutoffSlider);
    filterResonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "filterResonance", filterResonanceSlider);
    tuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "tune", tuneSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gain", gainSlider);
    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "character", characterSlider);

    // Update patch list
    updatePatchList();

    // Start timer for status updates
    startTimer(100);
}

ZenithSamplerEditor::~ZenithSamplerEditor()
{
    stopTimer();
}

//==============================================================================
void ZenithSamplerEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("Zenith Sampler", 0, 10, getWidth(), 30, juce::Justification::centred);
}

void ZenithSamplerEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(40); // Title space

    // Preset section (left)
    auto presetBounds = bounds.removeFromLeft(150);
    presetGroup.setBounds(presetBounds);
    
    auto presetContent = presetBounds.reduced(10);
    presetContent.removeFromTop(20); // Group label
    presetLabel.setBounds(presetContent.removeFromTop(25));
    presetComboBox.setBounds(presetContent.removeFromTop(30));
    presetContent.removeFromTop(10);
    statusLabel.setBounds(presetContent.removeFromTop(25));

    bounds.removeFromLeft(10); // Spacing

    // Envelope section (middle-left)
    auto envBounds = bounds.removeFromLeft(240);
    envelopeGroup.setBounds(envBounds);
    
    auto envContent = envBounds.reduced(10);
    envContent.removeFromTop(20); // Group label
    
    auto envTop = envContent.removeFromTop(100);
    attackSlider.setBounds(envTop.removeFromLeft(60));
    envTop.removeFromLeft(10);
    decaySlider.setBounds(envTop.removeFromLeft(60));
    envTop.removeFromLeft(10);
    sustainSlider.setBounds(envTop.removeFromLeft(60));
    
    envContent.removeFromTop(10);
    releaseSlider.setBounds(envContent.removeFromTop(100).removeFromLeft(60));

    bounds.removeFromLeft(10); // Spacing

    // Filter section (middle-right)
    auto filterBounds = bounds.removeFromLeft(130);
    filterGroup.setBounds(filterBounds);
    
    auto filterContent = filterBounds.reduced(10);
    filterContent.removeFromTop(20); // Group label
    
    filterCutoffSlider.setBounds(filterContent.removeFromTop(100).removeFromLeft(60));
    filterContent.removeFromTop(10);
    filterResonanceSlider.setBounds(filterContent.removeFromTop(100).removeFromLeft(60));

    bounds.removeFromLeft(10); // Spacing

    // Global section (right)
    globalGroup.setBounds(bounds);
    
    auto globalContent = bounds.reduced(10);
    globalContent.removeFromTop(20); // Group label
    
    tuneSlider.setBounds(globalContent.removeFromTop(100).removeFromLeft(60));
    globalContent.removeFromTop(10);
    gainSlider.setBounds(globalContent.removeFromTop(100).removeFromLeft(60));
    globalContent.removeFromTop(10);
    characterSlider.setBounds(globalContent.removeFromTop(100).removeFromLeft(60));
}

//==============================================================================
void ZenithSamplerEditor::timerCallback()
{
    if (sampler.isLoading())
    {
        statusLabel.setText("Loading...", juce::dontSendNotification);
    }
    else
    {
        auto patchName = sampler.getCurrentPatchName();
        if (patchName.isEmpty())
            statusLabel.setText("No patch loaded", juce::dontSendNotification);
        else
            statusLabel.setText("Loaded: " + patchName, juce::dontSendNotification);
    }
}

void ZenithSamplerEditor::updatePatchList()
{
    presetComboBox.clear();
    
    auto patches = sampler.getAvailablePatches();
    for (int i = 0; i < patches.size(); ++i)
    {
        presetComboBox.addItem(patches[i], i + 1);
    }

    // Select current patch
    auto currentPatch = sampler.getCurrentPatchName();
    if (currentPatch.isNotEmpty())
    {
        int index = patches.indexOf(currentPatch);
        if (index >= 0)
            presetComboBox.setSelectedId(index + 1, juce::dontSendNotification);
    }
}

void ZenithSamplerEditor::onPatchSelected()
{
    int selectedId = presetComboBox.getSelectedId();
    if (selectedId > 0)
    {
        juce::String patchName = presetComboBox.getItemText(selectedId - 1);
        sampler.loadPatchByName(patchName);
    }
}

} // namespace zenith
