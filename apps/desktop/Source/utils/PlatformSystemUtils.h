/*
  ==============================================================================

    PlatformSystemUtils.h
    Created: 2025-12-22

    Interface for platform-specific system utility functions.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

class PlatformSystemUtils {
public:
    /**
     * Performs many platform-specific system logging and initialization.
     */
    static void logSystemInfo();
    
    /**
     * Gets the name of the system storage location for app data.
     */
    static juce::String getSystemInfoString();
};

} // namespace zenith
