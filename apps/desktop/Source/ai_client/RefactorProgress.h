/*
  ==============================================================================

    RefactorProgress.h
    Progress tracking for project refactoring operations

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {
namespace ai {

//==============================================================================
/**
    Refactoring progress tracking
*/
struct RefactorProgress {
  int currentStep = 0;
  int totalSteps = 0;
  juce::String currentOperation;
  float progressPercent = 0.0f;
  juce::String error;
  bool isComplete = false;

  // Helper methods
  void reset();
  void advance(const juce::String& operation);
  void setOperation(const juce::String& operation);
  void updateProgress();
  float getProgress() const;
  juce::String getProgressString() const;
  juce::String getProgressPercentString() const;
  bool isInProgress() const;
  void setError(const juce::String& errorMessage);
  juce::String getError() const;
  bool hasError() const;
  void complete();
  bool isFinished() const;
  juce::var toJSON() const;
  bool fromJSON(const juce::var& json);
};

} // namespace ai
} // namespace zenith
