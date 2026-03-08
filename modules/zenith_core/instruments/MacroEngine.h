/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace zenith {

//==============================================================================
/**
    Parameter metadata for an instrument parameter
*/
class MacroEngine {
public:
  MacroEngine() = default;

  /**
   * @brief Initialize with instrument metadata
   * @param metadata Instrument metadata containing macro definitions
   * @note Must be called before use, from message thread
   */
  void initialize(const InstrumentMetadata &metadata) {
    metadata_ = &metadata;

    // Allocate storage for macro values
    macroValues_.resize(metadata.macros.size(), 0.5f);

    // Build lookup map for fast macro value access
    macroIdToIndex_.clear();
    for (size_t i = 0; i < metadata.macros.size(); ++i) {
      macroIdToIndex_[metadata.macros[i].id] = (int)i;
    }
  }

  /**
   * @brief Set macro value
   * @param macroId Macro ID
   * @param value Normalized value [0..1]
   * @note Thread-safe
   */
  void setMacroValue(const juce::String &macroId, float value) {
    auto it = macroIdToIndex_.find(macroId);
    if (it != macroIdToIndex_.end()) {
      macroValues_[it->second] = juce::jlimit(0.0f, 1.0f, value);
    }
  }

  /**
   * @brief Get macro value
   * @param macroId Macro ID
   * @return Normalized value [0..1], or 0.5 if not found
   * @note Thread-safe
   */
  float getMacroValue(const juce::String &macroId) const {
    auto it = macroIdToIndex_.find(macroId);
    if (it != macroIdToIndex_.end()) {
      return macroValues_[it->second];
    }
    return 0.5f; // Default centered value
  }

  /**
   * @brief Compute macro contribution to a parameter
   * @param parameterId Parameter ID
   * @param baseValue Base parameter value (before macro application)
   * @param paramRange Parameter range (max - min)
   * @return Contribution to add to base value
   * @note Audio-thread safe (no allocations)
   */
  float computeMacroContribution(const juce::String &parameterId,
                                 float baseValue, float paramRange) const {
    if (!metadata_)
      return 0.0f;

    float contribution = 0.0f;

    // Iterate through all macros and their targets
    for (size_t macroIdx = 0; macroIdx < metadata_->macros.size(); ++macroIdx) {
      const auto &macro = metadata_->macros[macroIdx];
      float macroValue = macroValues_[macroIdx];

      // Check if this macro targets our parameter
      for (const auto &target : macro.targets) {
        if (target.parameterId == parameterId) {
          // Macro value is [0..1], centered at 0.5
          // Convert to [-1..+1] range
          float normalizedMacro = (macroValue - 0.5f) * 2.0f;

          // Contribution = macro value * amount * parameter range
          contribution += normalizedMacro * target.amount * paramRange;
        }
      }
    }

    return contribution;
  }

  /**
   * @brief Get number of macros
   */
  size_t getNumMacros() const { return macroValues_.size(); }

  /**
   * @brief Get macro value by index
   */
  float getMacroValue(size_t index) const {
    if (index < macroValues_.size())
      return macroValues_[index];
    return 0.5f;
  }

  /**
   * @brief Set macro value by index
   */
  void setMacroValue(size_t index, float value) {
    if (index < macroValues_.size())
      macroValues_[index] = juce::jlimit(0.0f, 1.0f, value);
  }

private:
  const InstrumentMetadata *metadata_ = nullptr;
  std::vector<float> macroValues_;             ///< Macro values [0..1]
  std::map<juce::String, int> macroIdToIndex_; ///< Fast macro ID lookup

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroEngine)
};

} // namespace zenith
