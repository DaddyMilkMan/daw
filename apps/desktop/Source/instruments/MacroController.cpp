/*
  ==============================================================================

    MacroController.cpp
    Created: 16 Feb 2025
    Author:  Zenith DAW

  ==============================================================================
*/

#include "MacroController.h"
#include <cmath>
#include <sstream>

namespace zenith {

namespace {
float applyCurve(float normalizedValue, MacroCurve curve) {
  const float x = juce::jlimit(0.0f, 1.0f, normalizedValue);
  switch (curve) {
    case MacroCurve::Soft:
      return std::sqrt(x);
    case MacroCurve::Hard:
      return x * x;
    case MacroCurve::Linear:
    default:
      return x;
  }
}
} // namespace

MacroController::MacroController() {
  // Initialize macro values to center position
  macroValues_.fill(0.5f);
  previousAppliedValues_.fill(0.5f);

  // Initialize smoothed values
  for (auto& val : smoothedValues_) {
    val.setCurrentAndTargetValue(0.5f);
    val.reset(44100.0, 0.01); // 10ms smoothing at default sample rate
  }

  // Initialize empty assignments
  for (auto& assignments : assignments_) {
    assignments.ensureStorageAllocated(MAX_ASSIGNMENTS_PER_MACRO);
  }
}

//==============================================================================
void MacroController::setMacroValue(int macroIndex, float value) {
  jassert(macroIndex >= 0 && macroIndex < NUM_MACROS);
  jassert(value >= 0.0f && value <= 1.0f);

  macroValues_[macroIndex] = value;
  smoothedValues_[macroIndex].setTargetValue(value);
}

//==============================================================================
void MacroController::assignMacroToParameter(int macroIndex, int assignmentSlot,
                                           const juce::String& parameterId,
                                           float amount,
                                           MacroCurve curve) {
  jassert(macroIndex >= 0 && macroIndex < NUM_MACROS);
  jassert(assignmentSlot >= 0 && assignmentSlot < MAX_ASSIGNMENTS_PER_MACRO);

  auto& assignments = assignments_[macroIndex];

  // Ensure array is large enough
  while (assignments.size() <= assignmentSlot) {
    assignments.add(MacroAssignment{});
  }

  assignments.set(assignmentSlot, MacroAssignment(parameterId, amount, curve));
}

void MacroController::removeAssignment(int macroIndex, int assignmentSlot) {
  jassert(macroIndex >= 0 && macroIndex < NUM_MACROS);

  auto& assignments = assignments_[macroIndex];

  if (assignmentSlot >= 0 && assignmentSlot < assignments.size()) {
    assignments.set(assignmentSlot, MacroAssignment{}); // Reset to default
  }
}

void MacroController::clearAssignments(int macroIndex) {
  jassert(macroIndex >= 0 && macroIndex < NUM_MACROS);
  assignments_[macroIndex].clear();
}

//==============================================================================
void MacroController::processSmoothing(int numSamples) {
  // Process all smoothed values
  // juce::SmoothedValue handles smoothing automatically when getNextValue() is called
  // No need to manually skip - that would cause incorrect smoothing behavior
  // The smoothed values will naturally approach their target values over time
}

void MacroController::applyToParameters(juce::AudioProcessorValueTreeState& params) {
  // Apply assignment deltas to prevent cumulative runaway when macro value
  // remains static across blocks.
  for (int macroIdx = 0; macroIdx < NUM_MACROS; ++macroIdx) {
    const float macroValue = smoothedValues_[macroIdx].getNextValue();
    const float previousMacroValue = previousAppliedValues_[macroIdx];
    previousAppliedValues_[macroIdx] = macroValue;

    for (const auto& assignment : assignments_[macroIdx]) {
      if (!assignment.enabled || assignment.parameterId.isEmpty())
        continue;

      // Get the parameter
      auto* param = params.getParameter(assignment.parameterId);
      if (!param)
        continue;

      const float shapedCurrent = applyCurve(macroValue, assignment.curve);
      const float shapedPrevious = applyCurve(previousMacroValue, assignment.curve);
      const float macroDelta = shapedCurrent - shapedPrevious;
      if (std::abs(macroDelta) < 1.0e-6f) {
        continue;
      }

      const float normalised = param->getValue();
      const float nextNormalised =
          juce::jlimit(0.0f, 1.0f, normalised + macroDelta * assignment.amount);
      param->setValueNotifyingHost(nextNormalised);
    }
  }
}

//==============================================================================
juce::String MacroController::saveToXml() const {
  juce::XmlElement root("macros");

  for (int macroIdx = 0; macroIdx < NUM_MACROS; ++macroIdx) {
    auto* macroEl = root.createNewChildElement("macro");
    macroEl->setAttribute("index", macroIdx);
    macroEl->setAttribute("value", macroValues_[macroIdx]);

    auto* assignmentsEl = macroEl->createNewChildElement("assignments");

    int slot = 0;
    for (const auto& assignment : assignments_[macroIdx]) {
      if (!assignment.parameterId.isEmpty()) {
        auto* assignEl = assignmentsEl->createNewChildElement("assignment");
        assignEl->setAttribute("slot", slot);
        assignEl->setAttribute("param", assignment.parameterId);
        assignEl->setAttribute("amount", assignment.amount);
        assignEl->setAttribute("enabled", assignment.enabled);
        assignEl->setAttribute("curve", static_cast<int>(assignment.curve));
      }
      slot++;
    }
  }

  return root.toString();
}

bool MacroController::loadFromXml(const juce::String& xmlString) {
  auto xml = juce::XmlDocument::parse(xmlString);
  if (xml == nullptr)
    return false;

  auto* root = xml.get();
  if (!root || root->getTagName() != "macros")
    return false;

  for (auto* macroEl : root->getChildIterator()) {
    if (macroEl->getTagName() != "macro")
      continue;

    int macroIdx = macroEl->getIntAttribute("index");
    if (macroIdx < 0 || macroIdx >= NUM_MACROS)
      continue;

    macroValues_[macroIdx] = static_cast<float>(macroEl->getDoubleAttribute("value", 0.5));
    previousAppliedValues_[macroIdx] = macroValues_[macroIdx];
    smoothedValues_[macroIdx].setCurrentAndTargetValue(macroValues_[macroIdx]);

    clearAssignments(macroIdx);

    auto* assignmentsEl = macroEl->getChildByName("assignments");
    if (assignmentsEl) {
      for (auto* assignEl : assignmentsEl->getChildIterator()) {
        if (assignEl->getTagName() != "assignment")
          continue;

        int slot = assignEl->getIntAttribute("slot");
        juce::String paramId = assignEl->getStringAttribute("param");
        float amount = static_cast<float>(assignEl->getDoubleAttribute("amount", 1.0));
        bool enabled = assignEl->getBoolAttribute("enabled", true);
        MacroCurve curve = static_cast<MacroCurve>(
            assignEl->getIntAttribute("curve", static_cast<int>(MacroCurve::Linear)));
        if (curve != MacroCurve::Linear && curve != MacroCurve::Soft &&
            curve != MacroCurve::Hard) {
          curve = MacroCurve::Linear;
        }

        if (!paramId.isEmpty() && slot >= 0 && slot < MAX_ASSIGNMENTS_PER_MACRO) {
          assignMacroToParameter(macroIdx, slot, paramId, amount, curve);
          assignments_[macroIdx].getReference(slot).enabled = enabled;
        }
      }
    }
  }

  return true;
}

} // namespace zenith
