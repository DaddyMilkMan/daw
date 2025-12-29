/*
  ==============================================================================

    PlatformDisplayUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-specific display utilities.

  ==============================================================================
*/

#include "PlatformDisplayUtils.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

int PlatformDisplayUtils::getSystemRefreshRate()
{
    // Getting exact refresh rate from JUCE is not always direct in older versions or without VBlankListener.
    // For now, returning a safe default of 60Hz.
    // Ideally this would query the OS (EnumDisplaySettings on Windows, CGDisplayMode on Mac).
    
    // Future improvement: Implement OS-specific queries.
    return 60;
}

} // namespace zenith
