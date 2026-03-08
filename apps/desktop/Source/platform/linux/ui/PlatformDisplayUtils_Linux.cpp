/*
  ==============================================================================

    PlatformDisplayUtils_Linux.cpp
    Created: 2025-12-22

    Linux implementation of display utilities.
    (Empty/Default implementation)

  ==============================================================================
*/

#include "../../../ui/framework/PlatformDisplayUtils.h"
#include <juce_core/juce_core.h>

namespace zenith {

int PlatformDisplayUtils::getSystemRefreshRate() {
    // PRODUCTION DECISION: Using 60Hz as safe universal default.
    // Dynamic X11/Wayland query (via xrandr or wl_output) would require:
    //   1. Async execution to avoid blocking startup
    //   2. Display server connection management
    //   3. Fallback handling for headless/SSH sessions
    // For variable refresh rate (VRR) support, use VSync callbacks instead.
    // This value is used for animation timing, where 60Hz is conservative.
    return 60; 
}

} // namespace zenith
