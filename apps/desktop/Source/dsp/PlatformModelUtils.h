/*
  ==============================================================================

    PlatformModelUtils.h
    Created: 2025-12-22

    Interface for platform-specific AI model utility functions.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

class PlatformModelUtils {
public:
    /**
     * Searches for the default AI model file (e.g., htdemucs.onnx) in standard 
     * platform locations.
     */
    static juce::File findDefaultModel();
};

} // namespace zenith
