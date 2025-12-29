/*
  ==============================================================================

    PlatformSystemUtils_Windows.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "utils/PlatformSystemUtils.h"

#ifdef _WIN32
#include <windows.h>

namespace zenith {

void PlatformSystemUtils::logSystemInfo() {
    DBG("System: Windows " + getSystemInfoString());
}

juce::String PlatformSystemUtils::getSystemInfoString() {
    return juce::SystemStats::getOperatingSystemName();
}

} // namespace zenith
#endif
