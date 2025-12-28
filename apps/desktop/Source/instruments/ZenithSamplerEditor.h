#pragma once

#include "../ui/controls/SkiaComboBox.h"
#include "../ui/controls/SkiaLabel.h"
#include "../ui/controls/ZenithButton.h"
#include "../ui/controls/ZenithKnob.h"
#include "../ui/utils/ZenithParameterAttachment.h"
#include "InstrumentPreset.h"
#include "panels/PresetBrowserComponent.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

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
                            private juce::Timer {
public:
  ZenithSamplerEditor(ZenithSamplerProcessor &processor,
                      ZenithSampler &instrument,
                      ZenithPresetManager &presetManager);
  ~ZenithSamplerEditor() override;

  void paint(juce::Graphics &g) override;
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

  void onPresetLoaded(const Preset &preset);
  Preset captureCurrentState() const;
  void loadSampleMapData();

  //==========================================================================
  // Member Variables
  //==========================================================================

  ZenithSamplerProcessor &sampler;
  ZenithSampler &instrument_;
  ZenithPresetManager &presetManager_;

  //==========================================================================
  // Preset Browser
  //==========================================================================

  std::unique_ptr<PresetBrowserComponent> presetBrowser_;
  ZenithButton togglePresetBrowserButton_;
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
  ZenithButton refreshSamplesButton_;

  // Sample map data
  struct SampleInfo {
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

  class SampleMapTableModel : public juce::TableListBoxModel {
  public:
    SampleMapTableModel(ZenithSamplerEditor &owner);

    int getNumRows() override;
    void paintRowBackground(juce::Graphics &g, int rowNumber, int width,
                            int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics &g, int rowNumber, int columnId, int width,
                   int height, bool rowIsSelected) override;

  private:
    ZenithSamplerEditor &owner_;
  };

  std::unique_ptr<SampleMapTableModel> sampleMapTableModel_;

  //==========================================================================
  // Legacy preset selector
  //==========================================================================

  juce::GroupComponent presetGroup;

  // Preset selector
  SkiaLabel presetLabel;
  SkiaComboBox presetComboBox;
  SkiaLabel statusLabel;

  // Envelope controls
  SkiaLabel attackLabel, decayLabel, sustainLabel, releaseLabel;
  ZenithKnob attackSlider, decaySlider, sustainSlider, releaseSlider;

  // Filter controls
  SkiaLabel filterCutoffLabel, filterResonanceLabel;
  ZenithKnob filterCutoffSlider, filterResonanceSlider;

  // Global controls
  SkiaLabel tuneLabel, gainLabel, characterLabel;
  ZenithKnob tuneSlider, gainSlider, characterSlider;

  std::vector<std::unique_ptr<ZenithParameterAttachment>> attachments_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerEditor)
};

} // namespace zenith
