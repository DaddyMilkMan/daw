/*
  ==============================================================================

    AIPrompts.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Helper for building AI system prompts with DAW context.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <functional>

namespace zenith {

class AIPrompts {
public:
    static juce::String buildSystemPrompt(const std::function<juce::var()>& contextProvider,
                                         const juce::String& recentAgentActivity = "");
};

} // namespace zenith
