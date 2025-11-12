/**
 * @file CrashpadInit.cpp
 * @brief Implementation of Crashpad initialization
 *
 * IMPORTANT: This file requires Crashpad headers and libraries to compile.
 *
 * To enable Crashpad:
 * 1. Build or download crashpad_handler.exe (see scripts/windows/crashpad/README.md)
 * 2. Add Crashpad include path to CMake:
 *    target_include_directories(ZenithDAW PRIVATE /path/to/crashpad/include)
 * 3. Link Crashpad libraries (client, util, base):
 *    target_link_libraries(ZenithDAW PRIVATE crashpad::client crashpad::util crashpad::base)
 * 4. Build with: cmake -DZENITH_USE_CRASHPAD=ON -DCRAS HPAD_HANDLER_PATH="..."
 *
 * If Crashpad headers are not available, this file will not compile.
 * Build with -DZENITH_USE_CRASHPAD=OFF to exclude this file entirely.
 */

#if defined(ZENITH_USE_CRASHPAD) && defined(JUCE_WINDOWS)

#include "CrashpadInit.h"

// Crashpad headers (user must provide when ZENITH_USE_CRASHPAD=ON)
// These are NOT bundled with Zenith - build from source or use pre-built binaries
#if __has_include("client/crashpad_client.h")
    #include "client/crashpad_client.h"
    #include "client/crash_report_database.h"
    #include "client/settings.h"
    #define ZENITH_CRASHPAD_HEADERS_AVAILABLE 1
#else
    #define ZENITH_CRASHPAD_HEADERS_AVAILABLE 0
    #pragma message("Crashpad headers not found. Add Crashpad include path or build with -DZENITH_USE_CRASHPAD=OFF")
#endif

namespace zenith {
namespace diag {

#if ZENITH_CRASHPAD_HEADERS_AVAILABLE

// Global Crashpad client (lives for app lifetime)
static std::unique_ptr<crashpad::CrashpadClient> gCrashpadClient;

bool initCrashpad(const juce::File& handlerExe,
                  const juce::File& dbDir,
                  const juce::StringPairArray& annotations)
{
    // Verify handler exists
    if (!handlerExe.existsAsFile())
    {
        DBG("Crashpad: handler not found at " + handlerExe.getFullPathName());
        return false;
    }

    // Create database directory
    if (!dbDir.exists())
    {
        juce::Result result = dbDir.createDirectory();
        if (result.failed())
        {
            DBG("Crashpad: failed to create db directory: " + result.getErrorMessage());
            return false;
        }
    }

    // Initialize crash report database
    std::unique_ptr<crashpad::CrashReportDatabase> database =
        crashpad::CrashReportDatabase::Initialize(
            base::FilePath(dbDir.getFullPathName().toWideCharPointer()));

    if (!database)
    {
        DBG("Crashpad: failed to initialize database");
        return false;
    }

    // Disable uploads by default (local-only crash reports)
    crashpad::Settings* settings = database->GetSettings();
    if (settings)
    {
        settings->SetUploadsEnabled(false);
        DBG("Crashpad: uploads disabled (local-only mode)");
    }

    // Convert annotations to std::map
    std::map<std::string, std::string> annotationsMap;
    for (int i = 0; i < annotations.size(); ++i)
    {
        juce::String key = annotations.getAllKeys()[i];
        juce::String value = annotations.getAllValues()[i];
        annotationsMap[key.toStdString()] = value.toStdString();
    }

    // Handler arguments (optional flags)
    std::vector<std::string> arguments;
    // arguments.push_back("--no-rate-limit"); // Disable crash rate limiting

    // Create Crashpad client
    gCrashpadClient = std::make_unique<crashpad::CrashpadClient>();

    // Start handler process
    bool success = gCrashpadClient->StartHandler(
        base::FilePath(handlerExe.getFullPathName().toWideCharPointer()),  // handler path
        base::FilePath(dbDir.getFullPathName().toWideCharPointer()),       // database path
        base::FilePath(),                                                   // metrics dir (unused)
        std::string(),                                                      // upload URL (none)
        annotationsMap,                                                     // annotations
        arguments,                                                          // handler args
        true,                                                               // restartable
        true                                                                // asynchronous_start
    );

    if (success)
    {
        DBG("Crashpad: initialized successfully");
        DBG("  Handler: " + handlerExe.getFullPathName());
        DBG("  Database: " + dbDir.getFullPathName());
        DBG("  Annotations: " + juce::String(annotationsMap.size()) + " keys");
    }
    else
    {
        DBG("Crashpad: failed to start handler");
        gCrashpadClient.reset();
    }

    return success;
}

#else // !ZENITH_CRASHPAD_HEADERS_AVAILABLE

// Stub implementation when Crashpad headers are not available
bool initCrashpad(const juce::File& handlerExe,
                  const juce::File& dbDir,
                  const juce::StringPairArray& annotations)
{
    juce::ignoreUnused(handlerExe, dbDir, annotations);

    DBG("Crashpad: headers not available - crash reporting disabled");
    DBG("  To enable: add Crashpad include path and rebuild");
    DBG("  See: scripts/windows/crashpad/README.md");

    return false;
}

#endif // ZENITH_CRASHPAD_HEADERS_AVAILABLE

#if JUCE_DEBUG
void triggerTestCrash()
{
    DBG("Crashpad: triggering test crash (null pointer dereference)");
    DBG("  Expected: application crashes, .dmp file created in database");

    // Intentional null pointer dereference to trigger crash
    // This will be caught by Crashpad and generate a minidump
    volatile int* nullPtr = nullptr;
    *nullPtr = 42;

    // Never reached
    DBG("Crashpad: ERROR - test crash did not occur!");
}
#endif

} // namespace diag
} // namespace zenith

#endif // ZENITH_USE_CRASHPAD && JUCE_WINDOWS
