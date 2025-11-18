#pragma once

#include "ZenithSampler.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {
namespace instruments {

/**
 * @brief Custom editor for ZenithSampler
 *
 * Layout:
 *   [Left]   Preset selector
 *   [Middle] Envelope + Filter controls
 *   [Right]  Global controls (tune, gain, character)
 */
class ZenithSamplerEditor : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    ZenithSamplerEditor(ZenithSampler& processor);
    ~ZenithSamplerEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updatePatchList();
    void onPatchSelected();

    ZenithSampler& sampler;

    // UI sections
    juce::GroupComponent presetGroup;
    juce::GroupComponent envelopeGroup;
    juce::GroupComponent filterGroup;
    juce::GroupComponent globalGroup;

    // Preset selector
    juce::Label presetLabel;
    juce::ComboBox presetComboBox;
    juce::Label statusLabel;

    // Envelope controls
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;

    // Filter controls
    juce::Label filterCutoffLabel, filterResonanceLabel;
    juce::Slider filterCutoffSlider, filterResonanceSlider;

    // Global controls
    juce::Label tuneLabel, gainLabel, characterLabel;
    juce::Slider tuneSlider, gainSlider, characterSlider;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> characterAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerEditor)
};

} // namespace instruments
} // namespace zenith
