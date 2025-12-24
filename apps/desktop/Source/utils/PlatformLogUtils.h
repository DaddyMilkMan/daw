/*
  ==============================================================================

    PlatformLogUtils.h
    Created: 2025-12-23

    Platform-agnostic logging utilities interface

  ==============================================================================
*/

#pragma once

namespace zenith {

class PlatformLogUtils {
public:
  /**
   * @brief Show platform-specific debug console
   * @note On Windows, allocates a console window. On Unix, prints to terminal.
   */
  static void showDebugConsole();

  /**
   * @brief Free debug console resources
   * @note Only has effect on Windows (FreeConsole)
   */
  static void freeDebugConsole();
};

} // namespace zenith
