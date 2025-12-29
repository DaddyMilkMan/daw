/*
  ==============================================================================

    PlatformPathUtils.h
    Created: 2025-12-22

    Interface for platform-specific path utility functions.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

class PlatformPathUtils {
public:
    /**
     * Gets the default location for the application configuration file.
     */
    static juce::File getDefaultConfigurationFile();
};

} // namespace zenith
