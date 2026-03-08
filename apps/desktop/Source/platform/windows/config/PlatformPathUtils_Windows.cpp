/*
  ==============================================================================

    PlatformPathUtils_Windows.cpp
    Created: 2026-02-17

    Windows implementation for platform-specific path utility functions.
    Uses %APPDATA%\zenith for configuration storage, consistent with
    Windows application conventions.

  ==============================================================================
*/

#ifdef _WIN32
#include "../../../ui/framework/PlatformPathUtils.h"

namespace zenith {

juce::File PlatformPathUtils::getDefaultConfigurationFile() {
    // On Windows, use %APPDATA%\zenith (e.g. C:\Users\<user>\AppData\Roaming\zenith)
    auto configDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                         .getChildFile("zenith");

    if (!configDir.exists()) {
        configDir.createDirectory();
    }

    return configDir.getChildFile("zenith.settings");
}

} // namespace zenith
#endif // _WIN32
