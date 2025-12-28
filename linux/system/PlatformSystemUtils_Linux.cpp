/*
  ==============================================================================

    PlatformSystemUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "utils/PlatformSystemUtils.h"

#ifdef __linux__
namespace zenith {

void PlatformSystemUtils::logSystemInfo() {
    DBG("System: Linux " + getSystemInfoString());
}

juce::String PlatformSystemUtils::getSystemInfoString() {
    return juce::SystemStats::getOperatingSystemName();
}

} // namespace zenith
#endif
