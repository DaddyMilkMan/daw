/*
  ==============================================================================

    PlatformSystemUtils_Mac.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "utils/PlatformSystemUtils.h"

#ifdef __APPLE__
namespace zenith {

void PlatformSystemUtils::logSystemInfo() {
    DBG("System: macOS " + getSystemInfoString());
}

juce::String PlatformSystemUtils::getSystemInfoString() {
    return juce::SystemStats::getOperatingSystemName();
}

} // namespace zenith
#endif
