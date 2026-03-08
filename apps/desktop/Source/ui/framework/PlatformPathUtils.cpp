/*
  ==============================================================================

    PlatformPathUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-specific path utility functions.

  ==============================================================================
*/

#include "PlatformPathUtils.h"

namespace zenith {

juce::File PlatformPathUtils::getDefaultConfigurationFile()
{
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    
    #if JUCE_MAC
        return appDataDir.getChildFile("Application Support")
                        .getChildFile("ZenithDAW")
                        .getChildFile("ZenithConfig.json");
    #else
        return appDataDir.getChildFile("ZenithDAW")
                        .getChildFile("ZenithConfig.json");
    #endif
}

} // namespace zenith
