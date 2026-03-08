/*
  ==============================================================================

    MacroController.h
    Created: 16 Feb 2025
    Author:  Zenith DAW

    4 macro controls with assignable parameters for quick performance control.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include "ZenithPolySynthDefs.h"

namespace zenith {

enum class MacroCurve : int {
  Linear = 0,
  Soft = 1,
  Hard = 2
};

/**
    Single macro assignment - maps a macro to a parameter
*/
struct MacroAssignment {
  juce::String parameterId;  // ID of parameter to control
  float amount = 1.0f;        // Amount of influence (-2.0 to 2.0)
  bool enabled = true;        // Whether this assignment is active
  MacroCurve curve = MacroCurve::Linear;

  MacroAssignment() = default;
  MacroAssignment(const juce::String& id, float amt = 1.0f,
                  MacroCurve cv = MacroCurve::Linear)
    : parameterId(id), amount(amt), curve(cv) {}
};

/**
    Macro Controller - 4 performance macros with parameter assignments

    Each macro (0-3) can control up to 8 parameters simultaneously.
    Macros are stored as normalized values (0.0 to 1.0) but can be
    scaled and inverted per assignment.
*/
class MacroController {
public:
  static constexpr int NUM_MACROS = 4;
  static constexpr int MAX_ASSIGNMENTS_PER_MACRO = 8;

  MacroController();
  ~MacroController() = default;

  //==========================================================================
  // Macro value control
  //==========================================================================

  /** Set a macro value (normalized 0.0 to 1.0) */
  void setMacroValue(int macroIndex, float value);

  /** Get current macro value */
  float getMacroValue(int macroIndex) const { return macroValues_[macroIndex]; }

  /** Get smoothed macro value (for modulation) */
  float getSmoothedMacroValue(int macroIndex) const {
    return smoothedValues_[macroIndex].getTargetValue();
  }

  //==========================================================================
  // Parameter assignment
  //==========================================================================

  /** Assign a macro to control a parameter */
  void assignMacroToParameter(int macroIndex, int assignmentSlot,
                              const juce::String& parameterId,
                              float amount = 1.0f,
                              MacroCurve curve = MacroCurve::Linear);

  /** Remove a macro assignment */
  void removeAssignment(int macroIndex, int assignmentSlot);

  /** Get all assignments for a macro */
  const juce::Array<MacroAssignment>& getAssignments(int macroIndex) const {
    return assignments_[macroIndex];
  }

  /** Clear all assignments for a macro */
  void clearAssignments(int macroIndex);

  //==========================================================================
  // Processing
  //==========================================================================

  /** Process macro smoothing (call once per audio block) */
  void processSmoothing(int numSamples);

  /** Apply macro values to assigned parameters
      Called from audio thread - must be RT safe!
  */
  void applyToParameters(juce::AudioProcessorValueTreeState& params);

  //==========================================================================
  // Preset save/load
  //==========================================================================

  /** Save macro state to XML */
  juce::String saveToXml() const;

  /** Load macro state from XML */
  bool loadFromXml(const juce::String& xml);

private:
  // Current macro values (set from UI/MIDI)
  std::array<float, NUM_MACROS> macroValues_;
  std::array<float, NUM_MACROS> previousAppliedValues_;

  // Smoothed values for audio processing
  std::array<juce::SmoothedValue<float>, NUM_MACROS> smoothedValues_;

  // Parameter assignments for each macro
  std::array<juce::Array<MacroAssignment>, NUM_MACROS> assignments_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroController)
};

} // namespace zenith
