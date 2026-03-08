/*
  ==============================================================================

    PlatformDisplayUtils_Windows.cpp
    Created: 2026-02-17

    Windows implementation of display utilities.

  ==============================================================================
*/

#ifdef _WIN32
#include "../../../ui/framework/PlatformDisplayUtils.h"
#include <juce_core/juce_core.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace zenith {

int PlatformDisplayUtils::getSystemRefreshRate() {
    // Query the primary display's refresh rate via Win32 API.
    // DEVMODE contains dmDisplayFrequency for the current display settings.
    DEVMODEA devMode = {};
    devMode.dmSize = sizeof(DEVMODEA);

    if (EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &devMode)) {
        int rate = static_cast<int>(devMode.dmDisplayFrequency);
        // dmDisplayFrequency of 0 or 1 means "default" (hardware default, treat as 60)
        if (rate > 1) {
            return rate;
        }
    }

    // Safe universal default
    return 60;
}

} // namespace zenith
#endif // _WIN32
