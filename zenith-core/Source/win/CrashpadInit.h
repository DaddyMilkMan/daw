/**
 * @file CrashpadInit.h
 * @brief Crashpad crash reporting initialization (W8: Windows-only, opt-in)
 *
 * W8 Features:
 * - Out-of-process crash handler (crashpad_handler.exe)
 * - Local minidump generation (no uploads by default)
 * - Application annotations (version, build info)
 * - DEBUG-only test crash trigger
 *
 * Compilation:
 * - Only compiled when ZENITH_USE_CRASHPAD=ON in CMake
 * - Requires Crashpad headers/libraries (user-provided)
 * - Zero overhead when ZENITH_USE_CRASHPAD=OFF (compiled out)
 *
 * Privacy:
 * - Uploads disabled by default (SetUploadsEnabled(false))
 * - Minidumps stored locally in %APPDATA%\ZenithDAW\crashpad_db\
 * - User opt-in required (diagnostics.crashReportsEnabled setting)
 */

#pragma once

#if defined(ZENITH_USE_CRASHPAD) && defined(JUCE_WINDOWS)

#include <JuceHeader.h>

namespace zenith {
namespace diag {

/**
 * @brief Initialize Crashpad crash reporting
 *
 * Call once during app startup, after paths are available but before
 * worker threads start.
 *
 * @param handlerExe Path to crashpad_handler.exe (must exist)
 * @param dbDir Database directory for minidumps (created if missing)
 * @param annotations Key-value pairs added to crash reports (product, version, etc.)
 * @return true if handler started successfully, false otherwise
 *
 * Example:
 * @code
 * auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
 *                   .getChildFile("ZenithDAW");
 * auto dbDir = appData.getChildFile("crashpad_db");
 * juce::File handler("C:\\path\\to\\crashpad_handler.exe");
 *
 * juce::StringPairArray annotations;
 * annotations.set("product", "Zenith DAW");
 * annotations.set("version", "0.1.0");
 * annotations.set("build", "Debug");
 *
 * bool ok = zenith::diag::initCrashpad(handler, dbDir, annotations);
 * @endcode
 */
bool initCrashpad(const juce::File& handlerExe,
                  const juce::File& dbDir,
                  const juce::StringPairArray& annotations);

#if JUCE_DEBUG
/**
 * @brief Trigger intentional crash for testing (DEBUG-only)
 *
 * Causes immediate null pointer dereference to test crash reporting.
 * Should generate a .dmp file in dbDir/completed/ if Crashpad is active.
 *
 * **WARNING**: This WILL crash the application. Only call when testing.
 *
 * Usage:
 * - Debug menu: "Help → Diagnostics → Trigger Test Crash"
 * - Verify .dmp appears in %APPDATA%\ZenithDAW\crashpad_db\completed\
 */
void triggerTestCrash();
#endif

} // namespace diag
} // namespace zenith

#endif // ZENITH_USE_CRASHPAD && JUCE_WINDOWS
