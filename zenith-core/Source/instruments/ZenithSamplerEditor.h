/**
 * @file ZenithSamplerEditor.h
 * @brief Custom editor for ZenithSampler with macro knobs
 */

#pragma once

#include <JuceHeader.h>
#include "ZenithSampler.h"

namespace zenith {

//==============================================================================
/**
 * @brief Custom editor for ZenithSampler
 *
 * Features:
 * - Sample load button
 * - Parameter controls grouped by category
 * - 4 macro knobs with labels at the bottom
 */
class ZenithSamplerEditor : public juce::Component,
                           public juce::FileDragAndDropTarget
{
public:
    explicit ZenithSamplerEditor(ZenithSampler& processor);
    ~ZenithSamplerEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    ZenithSampler& processor_;

    // Load sample button
    juce::TextButton loadSampleButton_;

    // Parameter sliders
    juce::Slider sampleStartSlider_;
    juce::Slider sampleEndSlider_;
    juce::Slider filterCutoffSlider_;
    juce::Slider filterResonanceSlider_;
    juce::Slider attackSlider_;
    juce::Slider decaySlider_;
    juce::Slider sustainSlider_;
    juce::Slider releaseSlider_;
    juce::Slider volumeSlider_;

    // Labels
    juce::Label sampleStartLabel_;
    juce::Label sampleEndLabel_;
    juce::Label filterCutoffLabel_;
    juce::Label filterResonanceLabel_;
    juce::Label attackLabel_;
    juce::Label decayLabel_;
    juce::Label sustainLabel_;
    juce::Label releaseLabel_;
    juce::Label volumeLabel_;

    // Macro knobs
    std::vector<std::unique_ptr<juce::Slider>> macroKnobs_;
    std::vector<std::unique_ptr<juce::Label>> macroLabels_;

    // Attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments_;

    void setupSlider(juce::Slider& slider, juce::Label& label,
                    const juce::String& labelText, const juce::String& paramId);
    void setupMacroKnobs();
    void loadSampleFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerEditor)
};

} // namespace zenith
