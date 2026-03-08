/*
  ==============================================================================
    PlatformPathUtils_Mac.mm
    macOS implementation for platform-specific path utility functions.
  ==============================================================================
*/

#ifdef __APPLE__
#include "../../../ui/framework/PlatformPathUtils.h"
#import <Foundation/Foundation.h>

namespace zenith {

juce::File PlatformPathUtils::getDefaultConfigurationFile() {
    // On macOS, use ~/Library/Application Support/zenith
    auto configDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("Application Support")
                         .getChildFile("zenith");

    if (!configDir.exists()) {
        configDir.createDirectory();
    }

    return configDir.getChildFile("zenith.settings");
}

} // namespace zenith
#endif // __APPLE__
