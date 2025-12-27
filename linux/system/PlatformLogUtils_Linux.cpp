/*
  ==============================================================================

    PlatformLogUtils_Linux.cpp
    Created: 2025-12-23

    Linux-specific logging utilities: No-op debug console (uses terminal)

  ==============================================================================
*/

#ifdef __linux__
#include "../../apps/desktop/Source/utils/PlatformLogUtils.h"
#include <iostream>

namespace zenith {

void PlatformLogUtils::showDebugConsole() {
  // Linux uses terminal - no special allocation needed
  std::cout << "========================================" << std::endl;
  std::cout << "Zenith Debug Console (Linux Terminal)" << std::endl;
  std::cout << "========================================" << std::endl;
}

void PlatformLogUtils::freeDebugConsole() {
  // No-op on Linux
}

} // namespace zenith
#endif
