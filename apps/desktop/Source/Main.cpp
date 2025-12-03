/**
 * @file Main.cpp
 * @brief Zenith DAW - Main application entry point
 *
 * This file initializes the JUCE application and creates the main window.
 */

#include "../include/MainWindow.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * @class ZenithApplication
 * @brief Main application class for Zenith DAW
 *
 * Handles application lifecycle events:
 * - Initialization
 * - Shutdown
 * - System commands
 */
class ZenithApplication : public juce::JUCEApplication {
public:
  //==========================================================================
  ZenithApplication() = default;

  //==========================================================================
  // JUCEApplication interface
  //==========================================================================

  const juce::String getApplicationName() override { return "Zenith DAW"; }
  const juce::String getApplicationVersion() override { return "0.1.0"; }
  bool moreThanOneInstanceAllowed() override { return false; }

  //==========================================================================
  //==========================================================================
  void initialise(const juce::String &commandLine) override {
    // SECURITY: Input Validation
    // Although currently unused, we explicitly document that this input
    // is untrusted. Future implementations must validate this against
    // an allow-list of known flags before processing.
    juce::ignoreUnused(commandLine);

    // Log startup
    DBG("Zenith DAW starting...");
    DBG("Version: " + getApplicationVersion());
    DBG("JUCE Version: " + juce::SystemStats::getJUCEVersion());

    // SECURITY: Information Disclosure Prevention
    // Only log detailed system info in debug builds to prevent leaking
    // user environment details in production logs.
#if JUCE_DEBUG
    logSystemInfo();
#endif

    // Create main window
    mainWindow = std::make_unique<MainWindow>(getApplicationName());

    DBG("Zenith DAW initialized successfully!");
  }

  void shutdown() override {
    DBG("Zenith DAW shutting down...");

    // Close main window (releases all resources)
    mainWindow.reset();

    DBG("Zenith DAW shutdown complete.");
  }

  //==========================================================================
  void systemRequestedQuit() override {
    if (mainWindow != nullptr) {
      auto *projectState = mainWindow->getProjectState();
      if (projectState != nullptr && projectState->hasUnsavedChanges()) {
        int result = juce::NativeMessageBox::showYesNoCancelBox(
            juce::AlertWindow::WarningIcon, "Unsaved Changes",
            "You have unsaved changes. Do you want to save before quitting?",
            mainWindow.get(), nullptr);

        // JUCE NativeMessageBox return values:
        // 1 = Yes, 2 = No, 0 = Cancel
        const int RESULT_YES = 1;
        const int RESULT_NO = 2;
        const int RESULT_CANCEL = 0;

        if (result == RESULT_YES) // Yes
        {
          // SECURITY FIX: Prevention of Data Loss
          // Previously, this path would log a TODO and then QUIT, destroying user data.
          // We now enforce a "Fail-Safe" default: save and then quit.
          mainWindow->saveProject();
          quit();
        } else if (result == RESULT_NO) // No
        {
          // User explicitly consented to data loss (discard changes).
          quit();
        }
        // Cancel (result == RESULT_CANCEL) -> do nothing
      } else {
        quit();
      }
    } else {
      quit();
    }
  }

  void anotherInstanceStarted(const juce::String &commandLine) override {
    juce::ignoreUnused(commandLine);
  }

private:
  //==========================================================================
  void logSystemInfo() {
    DBG("========================================");
    DBG("System Information (DEBUG ONLY)");
    DBG("========================================");
    DBG("OS: " + juce::SystemStats::getOperatingSystemName());
    DBG("CPU: " + juce::String(juce::SystemStats::getCpuSpeedInMegahertz()) + " MHz");
    DBG("CPU Cores: " + juce::String(juce::SystemStats::getNumCpus()));
    DBG("Memory: " + juce::String(juce::SystemStats::getMemorySizeInMegabytes()) + " MB");
    DBG("========================================");
  }

  //==========================================================================
  std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(ZenithApplication)
