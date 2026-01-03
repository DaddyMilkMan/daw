#include "ZenithSamplerEditor.h"
#include "ZenithSampler.h"

namespace zenith {

ZenithSamplerEditor::ZenithSamplerEditor(ZenithSamplerProcessor &processor,
                                         ZenithSampler &instrument,
                                         ZenithPresetManager &presetManager)
    : AudioProcessorEditor(&processor), sampler(processor), 
      instrument_(instrument), presetManager_(presetManager) 
{
    // Setup Slider helper lambda
    auto setupSlider = [this](ZenithKnob& knob, SkiaLabel& label, const juce::String& text)
    {
        addAndMakeVisible(knob);
        knob.setStyle(ZenithKnob::Style::Standard);
        knob.setShowLabel(true); 

        addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
        label.setJustification(SkiaLabel::Justification::Center);
    };

    // ADSR Envelopes
    setupSlider(attackSlider, attackLabel, "Attack");
    setupSlider(decaySlider, decayLabel, "Decay");
    setupSlider(sustainSlider, sustainLabel, "Sustain");
    setupSlider(releaseSlider, releaseLabel, "Release");

    // Filter
    setupSlider(filterCutoffSlider, filterCutoffLabel, "Cutoff");
    setupSlider(filterResonanceSlider, filterResonanceLabel, "Reson");

    // Global
    setupSlider(tuneSlider, tuneLabel, "Tune");
    setupSlider(gainSlider, gainLabel, "Gain");
    setupSlider(characterSlider, characterLabel, "Char");

    // Attachments
    auto& apvts = sampler.getAPVTS();
    
    attackAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("attack"), attackSlider);
    decayAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("decay"), decaySlider);
    sustainAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("sustain"), sustainSlider);
    releaseAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("release"), releaseSlider);
    
    filterCutoffAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("filterCutoff"), filterCutoffSlider);
    filterResonanceAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("filterResonance"), filterResonanceSlider);
    
    tuneAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("tune"), tuneSlider);
    gainAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("globalGain"), gainSlider);
    characterAttachment = std::make_unique<ZenithParameterAttachment>(*apvts.getParameter("character"), characterSlider);

    // Preset Browser
    addAndMakeVisible(presetLabel);
    addAndMakeVisible(presetComboBox);
    addAndMakeVisible(statusLabel);
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustification(SkiaLabel::Justification::Center);

    // Groups
    addAndMakeVisible(envelopeGroup);
    envelopeGroup.setText("Envelope");

    addAndMakeVisible(filterGroup);
    filterGroup.setText("Filter");

    setSize(800, 600);
}

ZenithSamplerEditor::~ZenithSamplerEditor()
{
}

void ZenithSamplerEditor::paint(juce::Graphics& g)
{
    // Rendering handled by Skia in parent window integration
    juce::ignoreUnused(g);
}

void ZenithSamplerEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Header
    auto headerArea = area.removeFromTop(40);
    presetComboBox.setBounds(headerArea.removeFromRight(200));
    statusLabel.setBounds(headerArea);

    area.removeFromTop(10);

    // Controls layout
    auto controlArea = area.removeFromRight(300);

    controlArea.removeFromLeft(10);

    // Controls layout
    auto envelopeArea = controlArea.removeFromTop(180);
    envelopeGroup.setBounds(envelopeArea);
    envelopeArea.reduce(5, 20);
    
    auto adsrRow1 = envelopeArea.removeFromTop(80);
    attackSlider.setBounds(adsrRow1.removeFromLeft(envelopeArea.getWidth() / 2));
    decaySlider.setBounds(adsrRow1);
    
    auto adsrRow2 = envelopeArea;
    sustainSlider.setBounds(adsrRow2.removeFromLeft(envelopeArea.getWidth() / 2));
    releaseSlider.setBounds(adsrRow2);

    controlArea.removeFromTop(10);

    auto filterArea = controlArea.removeFromTop(120);
    filterGroup.setBounds(filterArea);
    filterArea.reduce(5, 20);
    filterCutoffSlider.setBounds(filterArea.removeFromLeft(filterArea.getWidth() / 2));
    filterResonanceSlider.setBounds(filterArea);

    controlArea.removeFromTop(10);

    auto globalArea = controlArea;
    int knobWidth = globalArea.getWidth() / 3;
    tuneSlider.setBounds(globalArea.removeFromLeft(knobWidth));
    gainSlider.setBounds(globalArea.removeFromLeft(knobWidth));
    characterSlider.setBounds(globalArea);
}

void ZenithSamplerEditor::timerCallback() {}
void ZenithSamplerEditor::updatePatchList() {}
void ZenithSamplerEditor::onPatchSelected() {}
void ZenithSamplerEditor::onPresetLoaded(const Preset &preset) { juce::ignoreUnused(preset); }
Preset ZenithSamplerEditor::captureCurrentState() const { return {}; }
void ZenithSamplerEditor::loadSampleMapData() {}

ZenithSamplerEditor::SampleMapTableModel::SampleMapTableModel(ZenithSamplerEditor &owner) : owner_(owner) {}
int ZenithSamplerEditor::SampleMapTableModel::getNumRows() { return 0; }
void ZenithSamplerEditor::SampleMapTableModel::paintRowBackground(juce::Graphics &, int, int, int, bool) {}
void ZenithSamplerEditor::SampleMapTableModel::paintCell(juce::Graphics &, int, int, int, int, bool) {}

} // namespace zenith
