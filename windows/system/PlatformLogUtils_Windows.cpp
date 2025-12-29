/*
  ==============================================================================

    PlatformLogUtils_Windows.cpp
    Created: 2025-12-23

    Windows-specific logging utilities: Debug console allocation

  ==============================================================================
*/

#ifdef _WIN32
#include "../../apps/desktop/Source/utils/PlatformLogUtils.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <iostream>
#include <windows.h>

namespace zenith {

void PlatformLogUtils::showDebugConsole() {
  if (AllocConsole()) {
    SetConsoleTitleW(L"Zenith Debug Console");

    FILE *fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();

    std::ios::sync_with_stdio(true);

    std::cout << "========================================" << std::endl;
    std::cout << "Zenith Professional Debug Console" << std::endl;
    std::cout << "========================================" << std::endl;
  }
}

void PlatformLogUtils::freeDebugConsole() { FreeConsole(); }

} // namespace zenith
#endif
