#pragma once
#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
#include <windows.h>
#endif

#include <iostream>

/**
 * @brief Simple debug console helper (Legacy)
 * @note Prefer zenith::ScopedDebugConsole in new code
 */
inline void showDebugConsole() {
#if JUCE_WINDOWS
    if (AllocConsole()) {
        SetConsoleTitle(TEXT("Zenith Debug Console"));
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
        std::ios::sync_with_stdio(true);
    }
#endif
}
