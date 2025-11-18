/**
 * @file ZenithPolySynthEditor.h
 * @brief Custom editor for ZenithPolySynth with macro knobs
 */

#pragma once

#include <JuceHeader.h>
#include "ZenithPolySynth.h"

namespace zenith {

//==============================================================================
/**
 * @brief Custom editor for ZenithPolySynth
 *
 * Features:
 * - Parameter controls grouped by category
 * - 4 macro knobs with labels at the bottom
 * - Preset browser
 */
class ZenithPolySynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit ZenithPolySynthEditor(ZenithPolySynth& processor);
    ~ZenithPolySynthEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ZenithPolySynth& processor_;

    // Parameter sliders
    juce::Slider waveformSlider_;
    juce::Slider detuneSlider_;
    juce::Slider filterCutoffSlider_;
    juce::Slider filterResonanceSlider_;
    juce::Slider attackSlider_;
    juce::Slider decaySlider_;
    juce::Slider sustainSlider_;
    juce::Slider releaseSlider_;
    juce::Slider volumeSlider_;

    // Labels
    juce::Label waveformLabel_;
    juce::Label detuneLabel_;
    juce::Label filterCutoffLabel_;
    juce::Label filterResonanceLabel_;
    juce::Label attackLabel_;
    juce::Label decayLabel_;
    juce::Label sustainLabel_;
    juce::Label releaseLabel_;
    juce::Label volumeLabel_;

    // Macro knobs (displayed prominently at bottom)
    std::vector<std::unique_ptr<juce::Slider>> macroKnobs_;
    std::vector<std::unique_ptr<juce::Label>> macroLabels_;

    // Attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments_;

    void setupSlider(juce::Slider& slider, juce::Label& label,
                    const juce::String& labelText, const juce::String& paramId);
    void setupMacroKnobs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthEditor)
};

} // namespace zenith
