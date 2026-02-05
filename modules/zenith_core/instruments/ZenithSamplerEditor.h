/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

#pragma once

#include "../ui/controls/SkiaComboBox.h"
#include "../ui/controls/SkiaLabel.h"
#include "../ui/controls/ZenithButton.h"
#include "../ui/controls/ZenithKnob.h"
#include "../ui/utils/ZenithParameterAttachment.h"
#include "InstrumentPreset.h"
#include "../ui/panels/PresetBrowserComponent.h"
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

  // Parameter attachments
  std::unique_ptr<ZenithParameterAttachment> attackAttachment, decayAttachment, sustainAttachment, releaseAttachment;
  std::unique_ptr<ZenithParameterAttachment> filterCutoffAttachment, filterResonanceAttachment;
  std::unique_ptr<ZenithParameterAttachment> tuneAttachment, gainAttachment, characterAttachment;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithSamplerEditor)
};

} // namespace zenith
