/*
  ==============================================================================
    PlatformDisplayUtils_Mac.mm
    macOS implementation of display utilities.
  ==============================================================================
*/

#ifdef __APPLE__
#include "../../../ui/framework/PlatformDisplayUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

int PlatformDisplayUtils::getSystemRefreshRate() {
    // JUCE's Display::refreshRate is often the most reliable way 
    // to query macOS ProMotion/Retina refresh rates without complex 
    // CVDisplayLink setup.
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()) {
        if (display->refreshRate > 0) {
            return static_cast<int>(display->refreshRate);
        }
    }

    // Standard fallback
    return 60;
}

} // namespace zenith
#endif // __APPLE__
