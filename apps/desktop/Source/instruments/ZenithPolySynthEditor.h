/**
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

#include "PresetBrowserComponent.h"
#include "InstrumentPreset.h"
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


namespace zenith {

//==============================================================================
/**
 * @brief Comprehensive editor for ZenithPolySynth
 *
 * DAW-grade UI with:
 * - Organized sections with group boxes
 * - Preset browser integration
 * - Parameter tooltips
 * - Collapsible sections (future enhancement)
 */
class ZenithPolySynthEditor : public juce::Component {
public:
  explicit ZenithPolySynthEditor(ZenithPolySynth &instrument,
                                 ZenithPresetManager &presetManager);
  ~ZenithPolySynthEditor() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  //==========================================================================
  // Setup Helpers
  //==========================================================================

  void setupSlider(juce::Slider &slider, juce::Label &label,
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
  juce::TextButton togglePresetBrowserButton_;
  bool presetBrowserVisible_ = true;

  // Group components for sections
  juce::GroupComponent oscGroup_;
  juce::GroupComponent filterGroup_;
  juce::GroupComponent envGroup_;
  juce::GroupComponent lfoGroup_;
  juce::GroupComponent globalGroup_;

  //==========================================================================
  // Oscillator Section
  //==========================================================================

  // Osc 1
  juce::Slider osc1WaveSlider_;
  juce::Label osc1WaveLabel_;
  juce::ComboBox osc1WaveCombo_;

  juce::Slider osc1DetuneSlider_;
  juce::Label osc1DetuneLabel_;

  juce::Slider osc1LevelSlider_;
  juce::Label osc1LevelLabel_;

  // Osc 2 (ready for processor integration)
  juce::Slider osc2WaveSlider_;
  juce::Label osc2WaveLabel_;
  juce::ComboBox osc2WaveCombo_;
  juce::Slider osc2DetuneSlider_;
  juce::Label osc2DetuneLabel_;
  juce::Slider osc2LevelSlider_;
  juce::Label osc2LevelLabel_;

  // Osc 3 (ready for processor integration)
  juce::Slider osc3WaveSlider_;
  juce::Label osc3WaveLabel_;
  juce::ComboBox osc3WaveCombo_;
  juce::Slider osc3DetuneSlider_;
  juce::Label osc3DetuneLabel_;
  juce::Slider osc3LevelSlider_;
  juce::Label osc3LevelLabel_;

  // Mix/Unison
  juce::Slider unisonVoicesSlider_;
  juce::Label unisonVoicesLabel_;

  juce::Slider unisonDetuneSlider_;
  juce::Label unisonDetuneLabel_;

  //==========================================================================
  // Filter Section
  //==========================================================================

  juce::ComboBox filterTypeCombo_;
  juce::Label filterTypeLabel_;

  juce::Slider filterCutoffSlider_;
  juce::Label filterCutoffLabel_;

  juce::Slider filterResonanceSlider_;
  juce::Label filterResonanceLabel_;

  juce::Slider filterDriveSlider_;
  juce::Label filterDriveLabel_;

  //==========================================================================
  // Envelope Section (2 ADSRs)
  //==========================================================================

  // Amp Envelope
  juce::Label ampEnvLabel_;
  juce::Slider ampAttackSlider_;
  juce::Label ampAttackLabel_;
  juce::Slider ampDecaySlider_;
  juce::Label ampDecayLabel_;
  juce::Slider ampSustainSlider_;
  juce::Label ampSustainLabel_;
  juce::Slider ampReleaseSlider_;
  juce::Label ampReleaseLabel_;

  // Filter Envelope
  juce::Label filterEnvLabel_;
  juce::Slider filterEnvAttackSlider_;
  juce::Label filterEnvAttackLabel_;
  juce::Slider filterEnvDecaySlider_;
  juce::Label filterEnvDecayLabel_;
  juce::Slider filterEnvSustainSlider_;
  juce::Label filterEnvSustainLabel_;
  juce::Slider filterEnvReleaseSlider_;
  juce::Label filterEnvReleaseLabel_;
  juce::Slider filterEnvAmountSlider_;
  juce::Label filterEnvAmountLabel_;

  //==========================================================================
  // LFO Section (2 LFOs)
  //==========================================================================

  // LFO 1
  juce::Label lfo1Label_;
  juce::ComboBox lfo1WaveCombo_;
  juce::Label lfo1WaveLabel_;
  juce::Slider lfo1RateSlider_;
  juce::Label lfo1RateLabel_;
  juce::ComboBox lfo1TargetCombo_;
  juce::Label lfo1TargetLabel_;
  juce::Slider lfo1AmountSlider_;
  juce::Label lfo1AmountLabel_;

  // LFO 2
  juce::Label lfo2Label_;
  juce::ComboBox lfo2WaveCombo_;
  juce::Label lfo2WaveLabel_;
  juce::Slider lfo2RateSlider_;
  juce::Label lfo2RateLabel_;
  juce::ComboBox lfo2TargetCombo_;
  juce::Label lfo2TargetLabel_;
  juce::Slider lfo2AmountSlider_;
  juce::Label lfo2AmountLabel_;

  //==========================================================================
  // Global Section
  //==========================================================================

  juce::Slider masterGainSlider_;
  juce::Label masterGainLabel_;

  juce::ToggleButton monoModeButton_;
  juce::Label monoModeLabel_;

  juce::Slider glideSlider_;
  juce::Label glideLabel_;

  juce::Slider voicesSlider_;
  juce::Label voicesLabel_;

  //==========================================================================
  // Macro Section (Smart Macros)
  //==========================================================================

  juce::Label macroSectionLabel_;
  std::vector<std::unique_ptr<juce::Slider>> macroKnobs_;
  std::vector<std::unique_ptr<juce::Label>> macroLabels_;

  //==========================================================================
  // Parameter Attachments
  //==========================================================================

  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>
      sliderAttachments_;
  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>>
      comboAttachments_;
  std::vector<
      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>>
      buttonAttachments_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithPolySynthEditor)
};

} // namespace zenith
