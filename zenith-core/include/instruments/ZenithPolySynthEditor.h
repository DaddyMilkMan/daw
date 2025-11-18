#pragma once

#include "instruments/ZenithPolySynth.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith {
namespace instruments {

/**
 * @brief UI Editor for ZenithPolySynth
 *
 * Clean, minimal editor with organized sections:
 * - Oscillator controls
 * - Filter controls
 * - Envelope controls
 * - Master controls
 */
class ZenithPolySynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit ZenithPolySynthEditor(ZenithPolySynth& processor);
    ~ZenithPolySynthEditor() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Reference to the processor
    ZenithPolySynth& processor_;

    //==============================================================================
    // Oscillator section

    juce::Label osc1WaveLabel_;
    juce::ComboBox osc1WaveCombo_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveAttachment_;

    juce::Label osc2WaveLabel_;
    juce::ComboBox osc2WaveCombo_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc2WaveAttachment_;

    juce::Label osc2DetuneLabel_;
    juce::Slider osc2DetuneSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2DetuneAttachment_;

    juce::Label oscMixLabel_;
    juce::Slider oscMixSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscMixAttachment_;

    juce::Label noiseLevelLabel_;
    juce::Slider noiseLevelSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseLevelAttachment_;

    //==============================================================================
    // Filter section

    juce::Label filterTypeLabel_;
    juce::ComboBox filterTypeCombo_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment_;

    juce::Label filterCutoffLabel_;
    juce::Slider filterCutoffSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment_;

    juce::Label filterResonanceLabel_;
    juce::Slider filterResonanceSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResonanceAttachment_;

    //==============================================================================
    // Envelope section

    juce::Label envAttackLabel_;
    juce::Slider envAttackSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envAttackAttachment_;

    juce::Label envDecayLabel_;
    juce::Slider envDecaySlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envDecayAttachment_;

    juce::Label envSustainLabel_;
    juce::Slider envSustainSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envSustainAttachment_;

    juce::Label envReleaseLabel_;
    juce::Slider envReleaseSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envReleaseAttachment_;

    //==============================================================================
    // Master section

    juce::Label masterGainLabel_;
    juce::Slider masterGainSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment_;

    //==============================================================================
    // Helper methods

    void setupLabel(juce::Label& label, const juce::String& text);
    void setupSlider(juce::Slider& slider, juce::Slider::SliderStyle style = juce::Slider::RotaryHorizontalVerticalDrag);
    void setupComboBox(juce::ComboBox& combo);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthEditor)
};

} // namespace instruments
} // namespace zenith
