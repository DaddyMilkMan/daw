/*
  ==============================================================================

    PlatformPathUtils_Linux.cpp
    Created: 2025-12-28

    Linux implementation for platform-specific path utility functions.

  ==============================================================================
*/

#include "../../../ui/framework/PlatformPathUtils.h"

namespace zenith {

juce::File PlatformPathUtils::getDefaultConfigurationFile() {
    auto configDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("zenith");

    if (!configDir.exists()) {
        configDir.createDirectory();
    }

    return configDir.getChildFile("zenith.settings");
}

} // namespace zenith
