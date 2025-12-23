/*
  ==============================================================================

    PlatformPathUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../ui/framework/PlatformPathUtils.h"

#ifdef __linux__
namespace zenith {

juce::File PlatformPathUtils::getDefaultConfigurationFile() {
    juce::String xdgConfigHome = juce::Process::getEnvironmentVariable("XDG_CONFIG_HOME");
    if (xdgConfigHome.isNotEmpty()) {
        return juce::File(xdgConfigHome).getChildFile("ZenithDAW").getChildFile("config.json");
    }
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile(".config/ZenithDAW/config.json");
}

} // namespace zenith
#endif
