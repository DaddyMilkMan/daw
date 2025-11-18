#include "instruments/ZenithPolySynthEditor.h"

namespace zenith {
namespace instruments {

ZenithPolySynthEditor::ZenithPolySynthEditor(ZenithPolySynth& p)
    : AudioProcessorEditor(&p)
    , processor_(p)
{
    // Set editor size
    setSize(700, 500);

    auto& params = processor_.getParameters();

    //==============================================================================
    // Oscillator section

    setupLabel(osc1WaveLabel_, "Osc 1 Wave");
    addAndMakeVisible(osc1WaveLabel_);
    setupComboBox(osc1WaveCombo_);
    osc1WaveCombo_.addItemList(juce::StringArray{"Sine", "Saw", "Square", "Triangle"}, 1);
    addAndMakeVisible(osc1WaveCombo_);
    osc1WaveAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, "osc1_wave", osc1WaveCombo_);

    setupLabel(osc2WaveLabel_, "Osc 2 Wave");
    addAndMakeVisible(osc2WaveLabel_);
    setupComboBox(osc2WaveCombo_);
    osc2WaveCombo_.addItemList(juce::StringArray{"Sine", "Saw", "Square", "Triangle"}, 1);
    addAndMakeVisible(osc2WaveCombo_);
    osc2WaveAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, "osc2_wave", osc2WaveCombo_);

    setupLabel(osc2DetuneLabel_, "Osc 2 Detune");
    addAndMakeVisible(osc2DetuneLabel_);
    setupSlider(osc2DetuneSlider_);
    osc2DetuneSlider_.setTextValueSuffix(" cents");
    addAndMakeVisible(osc2DetuneSlider_);
    osc2DetuneAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "osc2_detune", osc2DetuneSlider_);

    setupLabel(oscMixLabel_, "Osc Mix");
    addAndMakeVisible(oscMixLabel_);
    setupSlider(oscMixSlider_);
    addAndMakeVisible(oscMixSlider_);
    oscMixAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "osc_mix", oscMixSlider_);

    setupLabel(noiseLevelLabel_, "Noise");
    addAndMakeVisible(noiseLevelLabel_);
    setupSlider(noiseLevelSlider_);
    addAndMakeVisible(noiseLevelSlider_);
    noiseLevelAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "noise_level", noiseLevelSlider_);

    //==============================================================================
    // Filter section

    setupLabel(filterTypeLabel_, "Filter Type");
    addAndMakeVisible(filterTypeLabel_);
    setupComboBox(filterTypeCombo_);
    filterTypeCombo_.addItemList(juce::StringArray{"Low Pass", "Band Pass", "High Pass"}, 1);
    addAndMakeVisible(filterTypeCombo_);
    filterTypeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        params, "filter_type", filterTypeCombo_);

    setupLabel(filterCutoffLabel_, "Cutoff");
    addAndMakeVisible(filterCutoffLabel_);
    setupSlider(filterCutoffSlider_);
    filterCutoffSlider_.setTextValueSuffix(" Hz");
    addAndMakeVisible(filterCutoffSlider_);
    filterCutoffAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "filter_cutoff", filterCutoffSlider_);

    setupLabel(filterResonanceLabel_, "Resonance");
    addAndMakeVisible(filterResonanceLabel_);
    setupSlider(filterResonanceSlider_);
    addAndMakeVisible(filterResonanceSlider_);
    filterResonanceAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "filter_resonance", filterResonanceSlider_);

    //==============================================================================
    // Envelope section

    setupLabel(envAttackLabel_, "Attack");
    addAndMakeVisible(envAttackLabel_);
    setupSlider(envAttackSlider_);
    envAttackSlider_.setTextValueSuffix(" s");
    addAndMakeVisible(envAttackSlider_);
    envAttackAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "env_attack", envAttackSlider_);

    setupLabel(envDecayLabel_, "Decay");
    addAndMakeVisible(envDecayLabel_);
    setupSlider(envDecaySlider_);
    envDecaySlider_.setTextValueSuffix(" s");
    addAndMakeVisible(envDecaySlider_);
    envDecayAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "env_decay", envDecaySlider_);

    setupLabel(envSustainLabel_, "Sustain");
    addAndMakeVisible(envSustainLabel_);
    setupSlider(envSustainSlider_);
    addAndMakeVisible(envSustainSlider_);
    envSustainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "env_sustain", envSustainSlider_);

    setupLabel(envReleaseLabel_, "Release");
    addAndMakeVisible(envReleaseLabel_);
    setupSlider(envReleaseSlider_);
    envReleaseSlider_.setTextValueSuffix(" s");
    addAndMakeVisible(envReleaseSlider_);
    envReleaseAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "env_release", envReleaseSlider_);

    //==============================================================================
    // Master section

    setupLabel(masterGainLabel_, "Master Gain");
    addAndMakeVisible(masterGainLabel_);
    setupSlider(masterGainSlider_);
    masterGainSlider_.setTextValueSuffix(" dB");
    addAndMakeVisible(masterGainSlider_);
    masterGainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        params, "master_gain", masterGainSlider_);
}

ZenithPolySynthEditor::~ZenithPolySynthEditor()
{
}

//==============================================================================
void ZenithPolySynthEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff2a2a2a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("Zenith Poly Synth", 20, 15, 300, 30, juce::Justification::left);

    // Section backgrounds and labels
    auto sectionColor = juce::Colour(0xff353535);
    auto labelColor = juce::Colour(0xff00a8ff);

    // Oscillator section
    g.setColour(sectionColor);
    g.fillRoundedRectangle(20, 60, 330, 180, 5);
    g.setColour(labelColor);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("OSCILLATORS", 30, 65, 200, 20, juce::Justification::left);

    // Filter section
    g.setColour(sectionColor);
    g.fillRoundedRectangle(360, 60, 320, 180, 5);
    g.setColour(labelColor);
    g.drawText("FILTER", 370, 65, 200, 20, juce::Justification::left);

    // Envelope section
    g.setColour(sectionColor);
    g.fillRoundedRectangle(20, 250, 660, 180, 5);
    g.setColour(labelColor);
    g.drawText("AMPLITUDE ENVELOPE", 30, 255, 300, 20, juce::Justification::left);

    // Master section
    g.setColour(sectionColor);
    g.fillRoundedRectangle(20, 440, 160, 45, 5);
    g.setColour(labelColor);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("MASTER", 30, 443, 100, 20, juce::Justification::left);
}

void ZenithPolySynthEditor::resized()
{
    const int knobSize = 80;
    const int knobSpacing = 90;
    const int comboHeight = 25;
    const int labelHeight = 20;

    //==============================================================================
    // Oscillator section (20, 60, 330, 180)

    int oscX = 30;
    int oscY = 95;

    // Row 1: Waveform selectors
    osc1WaveLabel_.setBounds(oscX, oscY, 90, labelHeight);
    osc1WaveCombo_.setBounds(oscX, oscY + labelHeight, 90, comboHeight);

    osc2WaveLabel_.setBounds(oscX + 110, oscY, 90, labelHeight);
    osc2WaveCombo_.setBounds(oscX + 110, oscY + labelHeight, 90, comboHeight);

    // Row 2: Knobs
    int oscKnobY = oscY + 60;

    osc2DetuneLabel_.setBounds(oscX, oscKnobY + knobSize, knobSize, labelHeight);
    osc2DetuneSlider_.setBounds(oscX, oscKnobY, knobSize, knobSize);

    oscMixLabel_.setBounds(oscX + knobSpacing, oscKnobY + knobSize, knobSize, labelHeight);
    oscMixSlider_.setBounds(oscX + knobSpacing, oscKnobY, knobSize, knobSize);

    noiseLevelLabel_.setBounds(oscX + knobSpacing * 2, oscKnobY + knobSize, knobSize, labelHeight);
    noiseLevelSlider_.setBounds(oscX + knobSpacing * 2, oscKnobY, knobSize, knobSize);

    //==============================================================================
    // Filter section (360, 60, 320, 180)

    int filterX = 370;
    int filterY = 95;

    // Filter type selector
    filterTypeLabel_.setBounds(filterX, filterY, 120, labelHeight);
    filterTypeCombo_.setBounds(filterX, filterY + labelHeight, 120, comboHeight);

    // Filter knobs
    int filterKnobY = filterY + 60;

    filterCutoffLabel_.setBounds(filterX, filterKnobY + knobSize, knobSize, labelHeight);
    filterCutoffSlider_.setBounds(filterX, filterKnobY, knobSize, knobSize);

    filterResonanceLabel_.setBounds(filterX + knobSpacing, filterKnobY + knobSize, knobSize, labelHeight);
    filterResonanceSlider_.setBounds(filterX + knobSpacing, filterKnobY, knobSize, knobSize);

    //==============================================================================
    // Envelope section (20, 250, 660, 180)

    int envX = 90;
    int envY = 295;

    envAttackLabel_.setBounds(envX, envY + knobSize, knobSize, labelHeight);
    envAttackSlider_.setBounds(envX, envY, knobSize, knobSize);

    envDecayLabel_.setBounds(envX + knobSpacing + 40, envY + knobSize, knobSize, labelHeight);
    envDecaySlider_.setBounds(envX + knobSpacing + 40, envY, knobSize, knobSize);

    envSustainLabel_.setBounds(envX + (knobSpacing + 40) * 2, envY + knobSize, knobSize, labelHeight);
    envSustainSlider_.setBounds(envX + (knobSpacing + 40) * 2, envY, knobSize, knobSize);

    envReleaseLabel_.setBounds(envX + (knobSpacing + 40) * 3, envY + knobSize, knobSize, labelHeight);
    envReleaseSlider_.setBounds(envX + (knobSpacing + 40) * 3, envY, knobSize, knobSize);

    //==============================================================================
    // Master section (20, 440, 160, 45)

    int masterX = 100;
    int masterY = 445;

    masterGainLabel_.setBounds(masterX, masterY + 28, 70, labelHeight);
    masterGainSlider_.setBounds(masterX, masterY, 70, 28);
}

//==============================================================================
// Helper methods

void ZenithPolySynthEditor::setupLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setFont(juce::Font(12.0f));
}

void ZenithPolySynthEditor::setupSlider(juce::Slider& slider, juce::Slider::SliderStyle style)
{
    slider.setSliderStyle(style);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff00a8ff));
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00a8ff));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1a1a1a));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void ZenithPolySynthEditor::setupComboBox(juce::ComboBox& combo)
{
    combo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    combo.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    combo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff00a8ff));
    combo.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff00a8ff));
}

} // namespace instruments
} // namespace zenith
