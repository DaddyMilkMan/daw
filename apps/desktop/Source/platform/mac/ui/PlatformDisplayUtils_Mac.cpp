/*
  ==============================================================================

    PlatformDisplayUtils_Mac.cpp
    Created: 2025-12-22

    Mac implementation of display utilities.
    (Empty/Default implementation)

  ==============================================================================
*/

#include "../../../ui/framework/PlatformDisplayUtils.h"

namespace zenith {

int PlatformDisplayUtils::getSystemRefreshRate() {
    return 60; // Default fallback
}

} // namespace zenith
