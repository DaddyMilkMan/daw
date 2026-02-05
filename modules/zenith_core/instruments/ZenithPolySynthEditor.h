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

 * @file ZenithPolySynthEditor.h
 * @brief Comprehensive editor for ZenithPolySynth with all sections
 *
 * Layout:
 * - Top: Preset browser
 * - Oscillator section (waveform, detune, mix, unison)
 * - Filter section (type, cutoff, resonance, drive)
 * - Envelope section (2 ADSRs: amp + filter)
 * - LFO section (2 LFOs with routing)

 * - Global section (master gain, mono/poly, glide)
 * - Bottom: Macro knobs
 */

#pragma once

#include "InstrumentPreset.h"
#include "../ui/panels/PresetBrowserComponent.h"
#include "ZenithPolySynth.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <core/SkFont.h>

#include "../ui/framework/SkiaComponent.h"
#include "../ui/design-system/ZenithDesignSystem.h"
#include "../ui/controls/SkiaComboBox.h"
#include "../ui/controls/SkiaLabel.h"
#include "../ui/controls/ZenithButton.h"
#include "../ui/controls/ZenithKnob.h"
#include "../ui/controls/ZenithToggle.h"
#include "../ui/utils/ZenithParameterAttachment.h"

namespace zenith {

//==============================================================================
/**
 * @brief Comprehensive editor for ZenithPolySynth
 *
 * DAW-grade UI with:
 * - Organized sections with Skia-rendered group boxes
 * - Preset browser integration
 * - Parameter tooltips
 * - Collapsible sections (future enhancement)
 */
class ZenithPolySynthEditor : public SkiaComponent {
public:
  explicit ZenithPolySynthEditor(ZenithPolySynth &instrument,
                                 ZenithPresetManager &presetManager);
  ~ZenithPolySynthEditor() override = default;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

private:
  //==========================================================================
  // Setup Helpers
  //==========================================================================

  void setupSlider(ZenithKnob &knob, SkiaLabel &label,
                   const juce::String &labelText,
                   const juce::String &tooltip = "");
  void setupOscillatorSection();
  void setupFilterSection();
  void setupEnvelopeSection();
  void setupLFOSection();
  void setupGlobalSection();
  void setupMacroSection();
  void setupPresetBrowser();

  //==========================================================================
  // Preset Management
  //==========================================================================

  void onPresetLoaded(const zenith::Preset &preset);
  zenith::Preset captureCurrentState() const;

  //==========================================================================
  // Member Variables
  //==========================================================================

  ZenithPolySynth &instrument_;
  ZenithPresetManager &presetManager_;

  //==========================================================================
  // UI Sections
  //==========================================================================

  // Preset browser
  std::unique_ptr<PresetBrowserComponent> presetBrowser_;
  ZenithButton togglePresetBrowserButton_{"Hide Presets"};
  bool presetBrowserVisible_ = true;

  // Section bounds for Skia rendering (replaces juce::GroupComponent)
  juce::Rectangle<int> oscGroupBounds_;
  juce::Rectangle<int> filterGroupBounds_;
  juce::Rectangle<int> envGroupBounds_;
  juce::Rectangle<int> lfoGroupBounds_;
  juce::Rectangle<int> globalGroupBounds_;
  
  void drawSectionBox(SkCanvas *canvas, const juce::Rectangle<int> &bounds,
                      const juce::String &title);

  //==========================================================================
  // Oscillator Section
  //==========================================================================

  // Osc 1
  ZenithKnob osc1WaveSlider_;
  SkiaLabel osc1WaveLabel_;
  SkiaComboBox osc1WaveCombo_;

  ZenithKnob osc1DetuneSlider_;
  SkiaLabel osc1DetuneLabel_;

  ZenithKnob osc1LevelSlider_;
  SkiaLabel osc1LevelLabel_;

  // Osc 2 (ready for processor integration)
  ZenithKnob osc2WaveSlider_;
  SkiaLabel osc2WaveLabel_;
  SkiaComboBox osc2WaveCombo_;
  ZenithKnob osc2DetuneSlider_;
  SkiaLabel osc2DetuneLabel_;
  ZenithKnob osc2LevelSlider_;
  SkiaLabel osc2LevelLabel_;

  // Osc 3 (ready for processor integration)
  ZenithKnob osc3WaveSlider_;
  SkiaLabel osc3WaveLabel_;
  SkiaComboBox osc3WaveCombo_;
  ZenithKnob osc3DetuneSlider_;
  SkiaLabel osc3DetuneLabel_;
  ZenithKnob osc3LevelSlider_;
  SkiaLabel osc3LevelLabel_;

  // Mix/Unison
  ZenithKnob unisonVoicesSlider_;
  SkiaLabel unisonVoicesLabel_;

  ZenithKnob unisonDetuneSlider_;
  SkiaLabel unisonDetuneLabel_;

  //==========================================================================
  // Filter Section
  //==========================================================================

  SkiaComboBox filterTypeCombo_;
  SkiaLabel filterTypeLabel_;

  ZenithKnob filterCutoffSlider_;
  SkiaLabel filterCutoffLabel_;

  ZenithKnob filterResonanceSlider_;
  SkiaLabel filterResonanceLabel_;

  ZenithKnob filterDriveSlider_;
  SkiaLabel filterDriveLabel_;

  //==========================================================================
  // Envelope Section (2 ADSRs)
  //==========================================================================

  // Amp Envelope
  SkiaLabel ampEnvLabel_;
  ZenithKnob ampAttackSlider_;
  SkiaLabel ampAttackLabel_;
  ZenithKnob ampDecaySlider_;
  SkiaLabel ampDecayLabel_;
  ZenithKnob ampSustainSlider_;
  SkiaLabel ampSustainLabel_;
  ZenithKnob ampReleaseSlider_;
  SkiaLabel ampReleaseLabel_;

  // Filter Envelope
  SkiaLabel filterEnvLabel_;
  ZenithKnob filterEnvAttackSlider_;
  SkiaLabel filterEnvAttackLabel_;
  ZenithKnob filterEnvDecaySlider_;
  SkiaLabel filterEnvDecayLabel_;
  ZenithKnob filterEnvSustainSlider_;
  SkiaLabel filterEnvSustainLabel_;
  ZenithKnob filterEnvReleaseSlider_;
  SkiaLabel filterEnvReleaseLabel_;
  ZenithKnob filterEnvAmountSlider_;
  SkiaLabel filterEnvAmountLabel_;

  //==========================================================================
  // LFO Section (2 LFOs)
  //==========================================================================

  // LFO 1
  SkiaLabel lfo1Label_;
  SkiaComboBox lfo1WaveCombo_;
  SkiaLabel lfo1WaveLabel_;
  ZenithKnob lfo1RateSlider_;
  SkiaLabel lfo1RateLabel_;
  SkiaComboBox lfo1TargetCombo_;
  SkiaLabel lfo1TargetLabel_;
  ZenithKnob lfo1AmountSlider_;
  SkiaLabel lfo1AmountLabel_;

  // LFO 2
  SkiaLabel lfo2Label_;
  SkiaComboBox lfo2WaveCombo_;
  SkiaLabel lfo2WaveLabel_;
  ZenithKnob lfo2RateSlider_;
  SkiaLabel lfo2RateLabel_;
  SkiaComboBox lfo2TargetCombo_;
  SkiaLabel lfo2TargetLabel_;
  ZenithKnob lfo2AmountSlider_;
  SkiaLabel lfo2AmountLabel_;

  //==========================================================================
  // Global Section
  //==========================================================================

  ZenithKnob masterGainSlider_;
  SkiaLabel masterGainLabel_;

  ZenithToggle monoModeButton_;
  SkiaLabel monoModeLabel_;

  ZenithKnob glideSlider_;
  SkiaLabel glideLabel_;

  ZenithKnob voicesSlider_;
  SkiaLabel voicesLabel_;

  //==========================================================================
  // Macro Section (Smart Macros)
  //==========================================================================

  SkiaLabel macroSectionLabel_;
  std::vector<std::unique_ptr<ZenithKnob>> macroKnobs_;
  std::vector<std::unique_ptr<SkiaLabel>> macroLabels_;

  //==========================================================================
  // Parameter Attachments
  //==========================================================================

  std::vector<std::unique_ptr<ZenithParameterAttachment>> sliderAttachments_;
  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>>
      comboAttachments_;
  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>
      buttonAttachments_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthEditor)
};

} // namespace zenith
