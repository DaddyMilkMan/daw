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

  ==============================================================================

    PresetGenerator.h
    Created: 2025-11-29


    AI Preset Generation Helper
    Validates and generates synth presets based on descriptions


  ==============================================================================
*/

#pragma once

#include "../instruments/InstrumentPreset.h"
#include "../instruments/ZenithPresetManager.h"
#include <juce_core/juce_core.h>


namespace zenith {

/**
    Helper class for AI preset generation

    Provides validation, templates, and utilities for generating
    synthesizer presets from natural language descriptions.
*/
class PresetGenerator {
public:
  //==========================================================================
  /**
      Validate preset parameters against instrument schema

      @param instrumentId The instrument type
      @param parameters The preset parameters to validate
      @return true if valid, false if any parameters are out of range
  */
  static bool validatePresetParameters(const juce::String &instrumentId,
                                       const juce::var &parameters);

  /**
      Get parameter schema for an instrument

      @param instrumentId The instrument type
      @return JSON schema describing all parameters
  */
  static juce::var getParameterSchema(const juce::String &instrumentId);

  /**
      Generate a preset template based on sound type

      @param soundType Type of sound (pad, bass, lead, pluck, etc.)
      @return Template parameters for that sound type
  */
  static juce::var generateTemplate(const juce::String &soundType);

  /**
      Clamp parameter value to valid range

      @param paramName Parameter name
      @param value Input value
      @param instrumentId Instrument type
      @return Clamped value within valid range
  */
  static juce::var clampParameter(const juce::String &paramName,
                                  const juce::var &value,
                                  const juce::String &instrumentId);

  /**
      Get sound type from description

      @param description Natural language description
      @return Detected sound type (pad, bass, lead, etc.)
  */
  static juce::String detectSoundType(const juce::String &description);

  /**
      Create a complete preset from AI-generated parameters

      @param instrumentId Instrument type
      @param presetName Preset name
      @param description Description
      @param parameters AI-generated parameters
      @param genre Optional genre tag
      @return Complete InstrumentPreset object
  */
  static ZenithInstrumentPreset createPresetFromParameters(
      const juce::String &instrumentId, const juce::String &presetName,
      const juce::String &description, const juce::var &parameters,
      const juce::String &genre = juce::String());

  /**
      Mutate a preset effectively for genetic algorithms.
      Handles discrete parameters (waveforms, modes) by snapping,
      and continuous parameters with scaled offsets.

      @param preset The preset to mutate in-place
      @param mutationAmount 0.0 to 1.0 (0.1 = slight drift, 1.0 = chaos)
  */
  static void mutatePreset(ZenithInstrumentPreset &preset,
                           float mutationAmount);

  /**
      Mutate a lightweight Preset (for PresetGeneticistAgent).
  */
  static void mutatePreset(Preset &preset, float mutationAmount);

private:
  //==========================================================================
  // Parameter validation helpers

  static bool isValidWaveform(const juce::String &waveform);
  static bool isValidFilterType(const juce::String &filterType);
  static bool isValidLFOTarget(const juce::String &target);
  static bool isValidModSource(const juce::String &source);
  static bool isValidModDestination(const juce::String &destination);

  static float clampFloat(float value, float min, float max);
  static int clampInt(int value, int min, int max);

  JUCE_DECLARE_NON_COPYABLE(PresetGenerator)
};

} // namespace zenith
