/*
  ==============================================================================

    ProjectState.h
    Created: 2025-11-11
    Author:  Vexel DAW

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ProjectState : public juce::ValueTree::Listener
{
public:
    ProjectState();
    ~ProjectState() override;

    juce::ValueTree& getState() { return state; }
    const juce::ValueTree& getState() const { return state; }

    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                 const juce::Identifier& property) override;

private:
    juce::ValueTree state;
    juce::UndoManager undoManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectState)
};
