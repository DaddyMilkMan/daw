/*
  ==============================================================================

    AITools.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Defines the available tools (functions) for the AI agent.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include "GrokDAWClient.h" // Authoritative source for GrokFunction

namespace zenith {

class AITools {
public:
    static juce::Array<GrokFunction> getAvailableFunctions();
};

} // namespace zenith
