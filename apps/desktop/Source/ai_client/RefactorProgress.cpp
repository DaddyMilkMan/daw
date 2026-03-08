/*
  ==============================================================================

    RefactorProgress.cpp
    Implementation of refactoring progress tracking for AI-driven cleanup

  ==============================================================================
*/

#include "RefactorProgress.h"

namespace zenith {
namespace ai {

//==============================================================================
// RefactorProgress helper methods
//==============================================================================

void RefactorProgress::reset()
{
    currentStep = 0;
    totalSteps = 0;
    currentOperation.clear();
    progressPercent = 0.0f;
    error.clear();
    isComplete = false;
}

void RefactorProgress::advance(const juce::String& operation)
{
    if (totalSteps > 0)
        currentStep++;

    currentOperation = operation;
    updateProgress();
}

void RefactorProgress::setOperation(const juce::String& operation)
{
    currentOperation = operation;
}

void RefactorProgress::updateProgress()
{
    if (totalSteps > 0)
        progressPercent = static_cast<float>(currentStep) / static_cast<float>(totalSteps);
    else
        progressPercent = 0.0f;
}

float RefactorProgress::getProgress() const
{
    return progressPercent;
}

juce::String RefactorProgress::getProgressString() const
{
    if (isComplete)
        return "Complete";

    if (totalSteps > 0)
        return "Step " + juce::String(currentStep) + " of " + juce::String(totalSteps) +
               ": " + currentOperation;
    else
        return currentOperation;
}

juce::String RefactorProgress::getProgressPercentString() const
{
    return juce::String(static_cast<int>(progressPercent * 100.0f)) + "%";
}

bool RefactorProgress::isInProgress() const
{
    return !isComplete && totalSteps > 0 && currentStep >= 0;
}

void RefactorProgress::setError(const juce::String& errorMessage)
{
    error = errorMessage;
    isComplete = true;  // Stop on error
}

juce::String RefactorProgress::getError() const
{
    return error;
}

bool RefactorProgress::hasError() const
{
    return error.isNotEmpty();
}

void RefactorProgress::complete()
{
    isComplete = true;
    progressPercent = 1.0f;
    if (totalSteps > 0)
        currentStep = totalSteps;
}

bool RefactorProgress::isFinished() const
{
    return isComplete;
}

juce::var RefactorProgress::toJSON() const
{
    auto obj = new juce::DynamicObject();
    obj->setProperty("currentStep", currentStep);
    obj->setProperty("totalSteps", totalSteps);
    obj->setProperty("currentOperation", currentOperation);
    obj->setProperty("progressPercent", progressPercent);
    obj->setProperty("isComplete", isComplete);
    obj->setProperty("error", error);
    return juce::var(obj);
}

bool RefactorProgress::fromJSON(const juce::var& json)
{
    if (!json.isObject())
        return false;

    auto obj = json.getDynamicObject();
    currentStep = obj->getProperty("currentStep");
    totalSteps = obj->getProperty("totalSteps");
    currentOperation = obj->getProperty("currentOperation").toString();
    progressPercent = static_cast<float>(
        obj->getProperty("progressPercent"));
    isComplete = obj->getProperty("isComplete");
    error = obj->getProperty("error").toString();

    return true;
}

} // namespace ai
} // namespace zenith
