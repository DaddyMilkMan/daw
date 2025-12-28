/*
  ==============================================================================

    PlatformPathUtils_Mac.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "ui/framework/PlatformPathUtils.h"

#ifdef __APPLE__
namespace zenith {

juce::File PlatformPathUtils::getDefaultConfigurationFile() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW/config.json");
}

} // namespace zenith
#endif
