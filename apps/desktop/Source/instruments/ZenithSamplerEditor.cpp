#include "ZenithSamplerEditor.h"
#include "ZenithSampler.h"

namespace zenith {

//==============================================================================
ZenithSamplerEditor::ZenithSamplerEditor(
    ZenithSamplerProcessor& proc,
    ZenithSampler& instrument,
    ZenithPresetManager& presetManager)
    : juce::AudioProcessorEditor(&proc)
    , sampler(proc)
    , instrument_(instrument)
    , presetManager_(presetManager)
{
    setSize(900, 600);

    // Toggle preset browser button
    addAndMakeVisible(togglePresetBrowserButton_);
    togglePresetBrowserButton_.setButtonText("Show Presets");
    togglePresetBrowserButton_.onClick = [this]
    {
        presetBrowserVisible_ = !presetBrowserVisible_;
        togglePresetBrowserButton_.setButtonText(presetBrowserVisible_ ? "Hide Presets" : "Show Presets");

        if (presetBrowser_)
            presetBrowser_->setVisible(presetBrowserVisible_);

        resized();
    };

    // Create preset browser
    // Create preset browser
    presetBrowser_ = std::make_unique<PresetBrowserComponent>();
    presetBrowser_->setInstrumentId(instrument_.getMetadata().instrumentId);

    presetBrowser_->setLoadPresetCallback([this](const Preset& preset)
    {
        onPresetLoaded(preset);
    });

    presetBrowser_->setCaptureStateCallback([this]
    {
        return captureCurrentState();
    });

    addAndMakeVisible(*presetBrowser_);
    presetBrowser_->setVisible(presetBrowserVisible_);

    // Sample map section
    addAndMakeVisible(sampleMapGroup);
    sampleMapGroup.setText("Sample Map");
    sampleMapGroup.setTextLabelPosition(juce::Justification::centredTop);

    // Sample map table
    sampleMapTableModel_ = std::make_unique<SampleMapTableModel>(*this);
    sampleMapTable_.setModel(sampleMapTableModel_.get());
    sampleMapTable_.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1e1e1e));

    // Add columns
    sampleMapTable_.getHeader().addColumn("File", 1, 200, 50, 400);
    sampleMapTable_.getHeader().addColumn("Key Range", 2, 100, 60, 150);
    sampleMapTable_.getHeader().addColumn("Vel Range", 3, 100, 60, 150);
    sampleMapTable_.getHeader().addColumn("Root", 4, 60, 40, 80);

    addAndMakeVisible(sampleMapTable_);

    // Refresh button
    addAndMakeVisible(refreshSamplesButton_);
    refreshSamplesButton_.setButtonText("Refresh");
    refreshSamplesButton_.onClick = [this] { loadSampleMapData(); };

    // Legacy preset section (for patches)
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

    // Load sample map data
    loadSampleMapData();

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
    g.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
    g.drawText("Zenith Sampler", 0, 10, getWidth(), 30, juce::Justification::centred);
}

void ZenithSamplerEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(40); // Title space

    // Toggle button
    auto topRow = bounds.removeFromTop(30);
    togglePresetBrowserButton_.setBounds(topRow.removeFromRight(120).reduced(2));
    bounds.removeFromTop(5);

    // Preset browser (if visible)
    if (presetBrowserVisible_ && presetBrowser_)
    {
        auto presetBounds = bounds.removeFromTop(200);
        presetBrowser_->setBounds(presetBounds);
        bounds.removeFromTop(10);
    }

    // Main layout: Sample map on left, controls on right
    auto sampleMapBounds = bounds.removeFromLeft(380);
    sampleMapGroup.setBounds(sampleMapBounds);

    auto sampleMapContent = sampleMapBounds.reduced(10);
    sampleMapContent.removeFromTop(20); // Group label

    auto refreshButtonBounds = sampleMapContent.removeFromBottom(30);
    refreshSamplesButton_.setBounds(refreshButtonBounds);
    sampleMapContent.removeFromBottom(5);

    sampleMapTable_.setBounds(sampleMapContent);

    bounds.removeFromLeft(10); // Spacing

    // Right side: Preset selector + Envelope/Filter/Global
    auto presetSelectorBounds = bounds.removeFromTop(100);
    presetGroup.setBounds(presetSelectorBounds);

    auto presetContent = presetSelectorBounds.reduced(10);
    presetContent.removeFromTop(20); // Group label
    presetLabel.setBounds(presetContent.removeFromTop(25));
    presetComboBox.setBounds(presetContent.removeFromTop(30));
    presetContent.removeFromTop(5);
    statusLabel.setBounds(presetContent.removeFromTop(25));

    bounds.removeFromTop(10);

    // Envelope section
    auto envBounds = bounds.removeFromTop(140);
    envelopeGroup.setBounds(envBounds);

    auto envContent = envBounds.reduced(10);
    envContent.removeFromTop(20); // Group label

    int knobSize = 60;
    attackSlider.setBounds(envContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));
    envContent.removeFromLeft(5);
    decaySlider.setBounds(envContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));
    envContent.removeFromLeft(5);
    sustainSlider.setBounds(envContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));
    envContent.removeFromLeft(5);
    releaseSlider.setBounds(envContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));

    bounds.removeFromTop(10);

    // Filter and Global in a row
    auto bottomRow = bounds;

    auto filterBounds = bottomRow.removeFromLeft(bottomRow.getWidth() / 2);
    filterBounds.removeFromRight(5);
    filterGroup.setBounds(filterBounds);

    auto filterContent = filterBounds.reduced(10);
    filterContent.removeFromTop(20); // Group label

    filterCutoffSlider.setBounds(filterContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));
    filterContent.removeFromLeft(5);
    filterResonanceSlider.setBounds(filterContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));

    bottomRow.removeFromLeft(5);

    auto globalBounds = bottomRow;
    globalGroup.setBounds(globalBounds);

    auto globalContent = globalBounds.reduced(10);
    globalContent.removeFromTop(20); // Group label

    tuneSlider.setBounds(globalContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));
    globalContent.removeFromLeft(5);
    gainSlider.setBounds(globalContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));
    globalContent.removeFromLeft(5);
    characterSlider.setBounds(globalContent.removeFromLeft(knobSize + 20).removeFromTop(knobSize + 30));
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

        // Reload sample map
        loadSampleMapData();
    }
}

void ZenithSamplerEditor::onPresetLoaded(const Preset& preset)
{
    // Apply all parameters from preset
    for (const auto& [paramId, value] : preset.parameters)
    {
        instrument_.setParameter(juce::String(paramId), value);
    }
}

Preset ZenithSamplerEditor::captureCurrentState() const
{
    Preset preset;
    preset.instrumentId = instrument_.getMetadata().instrumentId;

    // Capture all parameters from metadata
    const auto& metadata = instrument_.getMetadata();
    for (const auto& param : metadata.parameters)
    {
        float value = instrument_.getParameter(param.id);
        preset.parameters[param.id] = value;
    }

    return preset;
}

void ZenithSamplerEditor::loadSampleMapData()
{
    // Clear existing data
    sampleMapData_.clear();

    auto& synth = sampler.getSynth();
    int numSounds = synth.getNumSounds();

    if (numSounds == 0)
    {
        // Show placeholder if no sounds
        SampleInfo placeholder;
        placeholder.fileName = "No samples loaded";
        placeholder.lowKey = 0;
        placeholder.highKey = 127;
        placeholder.lowVelocity = 0;
        placeholder.highVelocity = 127;
        placeholder.rootNote = 60;
        sampleMapData_.push_back(placeholder);
    }
    else
    {
        for (int i = 0; i < numSounds; ++i)
        {
            if (auto* sound = dynamic_cast<ZenithSamplerSound*>(synth.getSound(i).get()))
            {
                SampleInfo info;
                info.fileName = sound->getName();
                info.rootNote = sound->getRootNote();
                info.lowKey = sound->getLowKey();
                info.highKey = sound->getHighKey();
                info.lowVelocity = sound->getLowVelocity();
                info.highVelocity = sound->getHighVelocity();
                sampleMapData_.push_back(info);
            }
        }
    }

    sampleMapTable_.updateContent();
}

//==============================================================================
// SampleMapTableModel
//==============================================================================

ZenithSamplerEditor::SampleMapTableModel::SampleMapTableModel(ZenithSamplerEditor& owner)
    : owner_(owner)
{
}

int ZenithSamplerEditor::SampleMapTableModel::getNumRows()
{
    return (int)owner_.sampleMapData_.size();
}

void ZenithSamplerEditor::SampleMapTableModel::paintRowBackground(
    juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xff4a9eff));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xff242424));
    else
        g.fillAll(juce::Colour(0xff1e1e1e));
}

void ZenithSamplerEditor::SampleMapTableModel::paintCell(
    juce::Graphics& g, int rowNumber, int columnId,
    int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int)owner_.sampleMapData_.size())
        return;

    const auto& sample = owner_.sampleMapData_[rowNumber];


    // Display sample filename, key range, etc.
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(12.0f);

    switch (columnId)
    {
        case 1: // Filename
            g.drawText(sample.fileName, 4, 0, width - 8, height, juce::Justification::centredLeft, true);
            break;
        case 2: // Low Key
            g.drawText(juce::String(sample.lowKey), 4, 0, width - 8, height, juce::Justification::centred, true);
            break;
        case 3: // High Key
            g.drawText(juce::String(sample.highKey), 4, 0, width - 8, height, juce::Justification::centred, true);
            break;
        case 4: // Root Note
            g.drawText(juce::String(sample.rootNote), 4, 0, width - 8, height, juce::Justification::centred, true);
            break;
    }
}

} // namespace zenith
