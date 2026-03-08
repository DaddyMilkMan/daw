/*
  ==============================================================================
    PlatformLogUtils_Mac.mm
    macOS-specific logging utilities implementation
  ==============================================================================
*/

#ifdef __APPLE__
#include "../../../utils/PlatformLogUtils.h"
#include "../../../engine/ZenithLogger.h"

namespace zenith {

void PlatformLogUtils::showDebugConsole() {
    // macOS apps usually log to stdout/stderr which are captured by Xcode
    // or Console.app. No need for a separate console window allocation.
    ZenithLogger::makeGlobal();
    ZenithLogger::getInstance().setLogToConsole(true);
    
    ZENITH_LOG_INFO("========================================");
    ZENITH_LOG_INFO("Zenith DAW Professional Logging (macOS)");
    ZENITH_LOG_INFO("OS: macOS | Architecture: Apple Silicon/Intel");
    ZENITH_LOG_INFO("========================================");
}

void PlatformLogUtils::freeDebugConsole() {
    // No explicit console to free on macOS
}

} // namespace zenith
#endif // __APPLE__
