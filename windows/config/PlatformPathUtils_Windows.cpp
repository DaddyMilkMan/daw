/*
  ==============================================================================

    PlatformPathUtils_Windows.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "ui/framework/PlatformPathUtils.h"

#ifdef _WIN32
namespace zenith {

juce::File PlatformPathUtils::getDefaultConfigurationFile() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ZenithDAW/config.json");
}

} // namespace zenith
#endif
