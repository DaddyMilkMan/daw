/*
  ==============================================================================

    PlatformDisplayUtils_Windows.cpp
    Created: 2025-12-22

    Windows implementation of display utilities.

  ==============================================================================
*/

#include "../../../ui/framework/PlatformDisplayUtils.h"

#if defined(_WIN32)
#include <windows.h>

namespace zenith {

int PlatformDisplayUtils::getSystemRefreshRate() {
  DEVMODE devMode;
  devMode.dmSize = sizeof(DEVMODE);
  devMode.dmDriverExtra = 0;

  if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode)) {
    int rate = devMode.dmDisplayFrequency;
    // Ensure reasonable bounds (e.g., 30Hz to 360Hz)
    if (rate < 30)
      rate = 30;
    if (rate > 360)
      rate = 360;

    return rate;
  }
  return 60;
}

} // namespace zenith
#endif
