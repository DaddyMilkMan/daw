/*
  ==============================================================================

    PlatformLogUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-agnostic logging utilities.

  ==============================================================================
*/

#include "PlatformLogUtils.h"
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
#include <windows.h>
#include <iostream>
#endif

namespace zenith {

void PlatformLogUtils::showDebugConsole()
{
#if JUCE_WINDOWS
    if (AllocConsole())
    {
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
        std::cout << "Debug Console Attached" << std::endl;
    }
#else
    // On macOS/Linux, output usually goes to stdout/terminal by default if launched from there
    // No explicit console allocation needed usually.
#endif
}

void PlatformLogUtils::freeDebugConsole()
{
#if JUCE_WINDOWS
    FreeConsole();
#endif
}

} // namespace zenith
