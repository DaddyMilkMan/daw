/*
  ==============================================================================

    PlatformLogUtils_Windows.cpp
    Created: 2026-02-17

    Windows-specific logging utilities: Allocates a debug console window
    when running in debug mode or when explicitly requested.

  ==============================================================================
*/

#ifdef _WIN32
#include "../../../utils/PlatformLogUtils.h"
#include "../../../engine/ZenithLogger.h"
#include <juce_core/juce_core.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#include <iostream>

namespace zenith {

void PlatformLogUtils::showDebugConsole() {
    if (AllocConsole()) {
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$",  "r", stdin);

        SetConsoleTitleA("Zenith Debug Console");
        
        // Use Zenith's high-performance logger
        ZenithLogger::makeGlobal();
        ZenithLogger::getInstance().setLogToConsole(true);

        ZENITH_LOG_INFO("========================================");
        ZENITH_LOG_INFO("Zenith DAW Professional Debug Console");
        ZENITH_LOG_INFO("OS: Windows x64 | JUCE 8.0.0");
        ZENITH_LOG_INFO("========================================");
    }
}

void PlatformLogUtils::freeDebugConsole() {
    FreeConsole();
}

} // namespace zenith
#endif // _WIN32
