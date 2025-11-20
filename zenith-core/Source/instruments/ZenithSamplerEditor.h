#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "InstrumentPreset.h"
#include "../ui/PresetBrowserComponent.h"

namespace zenith {

// Forward declarations
class ZenithSamplerProcessor;
class ZenithSampler;

/**
 * @brief Comprehensive editor for ZenithSamplerProcessor
 *
 * Layout:
 *   [Top]    Preset browser (toggleable)
 *   [Left]   Sample map table
 *   [Middle] Envelope + Filter controls
 *   [Right]  Global controls (tune, gain, character)
 */
class ZenithSamplerEditor : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    ZenithSamplerEditor(ZenithSamplerProcessor& processor,
                       ZenithSampler& instrument,
                       ZenithPresetManager& presetManager);
    ~ZenithSamplerEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==========================================================================
    // Timer & Update
    //==========================================================================

    void timerCallback() override;
    void updatePatchList();
    void onPatchSelected();

    //==========================================================================
    // Preset Management
    //==========================================================================

    void onPresetLoaded(const ZenithInstrumentPreset& preset);
    std::map<std::string, float> captureCurrentState();
    void loadSampleMapData();

    //==========================================================================
    // Member Variables
    //==========================================================================

    ZenithSamplerProcessor& sampler;
    ZenithSampler& instrument_;
    ZenithPresetManager& presetManager_;

    //==========================================================================
    // Preset Browser
    //==========================================================================

    std::unique_ptr<PresetBrowserComponent> presetBrowser_;
    juce::TextButton togglePresetBrowserButton_;
    bool presetBrowserVisible_ = false;

    //==========================================================================
    // UI sections
    //==========================================================================

    juce::GroupComponent sampleMapGroup;
    juce::GroupComponent envelopeGroup;
    juce::GroupComponent filterGroup;
    juce::GroupComponent globalGroup;

    //==========================================================================
    // Sample Map Table
    //==========================================================================

    juce::TableListBox sampleMapTable_;
    juce::TextButton refreshSamplesButton_;

    // Sample map data
    struct SampleInfo
    {
        juce::String fileName;
        int lowKey = 0;
        int highKey = 127;
        int lowVelocity = 0;
        int highVelocity = 127;
        int rootNote = 60;
    };

    std::vector<SampleInfo> sampleMapData_;

    //==========================================================================
    // Sample Map Table Model
    //==========================================================================

    class SampleMapTableModel : public juce::TableListBoxModel
    {
    public:
        SampleMapTableModel(ZenithSamplerEditor& owner);

        int getNumRows() override;
        void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height,
                               bool rowIsSelected) override;
        void paintCell(juce::Graphics& g, int rowNumber, int columnId,
                      int width, int height, bool rowIsSelected) override;

    private:
        ZenithSamplerEditor& owner_;
    };

    std::unique_ptr<SampleMapTableModel> sampleMapTableModel_;

    //==========================================================================
    // Legacy preset selector
    //==========================================================================

    juce::GroupComponent presetGroup;

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

} // namespace zenith
