/**
 * @file Main.cpp
 * @brief Zenith DAW - Main application entry point
 *
 * This file initializes the JUCE application and creates the main window.
 */

#include "MainWindow.h"
#include "engine/ZenithLogger.h"
#include "utils/SampleGenerator.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>


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
    juce::ignoreUnused(commandLine);

    // RAII Console - allocated immediately
    debugConsole = std::make_unique<zenith::ScopedDebugConsole>();

    // Set ZenithLogger as the global JUCE logger
    zenith::ZenithLogger::makeGlobal();

    ZENITH_LOG_INFO("Zenith DAW starting...");
    ZENITH_LOG_INFO("Version: " + getApplicationVersion());
    ZENITH_LOG_INFO("JUCE Version: " + juce::SystemStats::getJUCEVersion());

    logSystemInfo();

    // Ensure content validity (Generate missing samples if needed)
    zenith::SampleGenerator::generateMissingSamples();

    // Create main window
    mainWindow = std::make_unique<MainWindow>(getApplicationName());

    ZENITH_LOG_INFO("Zenith DAW initialized successfully!");
  }

  void shutdown() override {
    ZENITH_LOG_INFO("Zenith DAW shutting down...");

    // Close main window (releases all resources)
    mainWindow.reset();

    // Console and Logger cleanup handled by RAII/Destructors
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
          // Save and quit
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
    ZENITH_LOG_INFO("========================================");
    ZENITH_LOG_INFO("System Information");
    ZENITH_LOG_INFO("========================================");
    ZENITH_LOG_INFO("OS: " + juce::SystemStats::getOperatingSystemName());
    ZENITH_LOG_INFO("CPU: " + juce::String(juce::SystemStats::getCpuSpeedInMegahertz()) + " MHz");
    ZENITH_LOG_INFO("CPU Cores: " + juce::String(juce::SystemStats::getNumCpus()));
    ZENITH_LOG_INFO("Memory: " + juce::String(juce::SystemStats::getMemorySizeInMegabytes()) + " MB");
    ZENITH_LOG_INFO("========================================");
  }

  //==========================================================================
  std::unique_ptr<MainWindow> mainWindow;
  std::unique_ptr<zenith::ScopedDebugConsole> debugConsole;
};

//==============================================================================
START_JUCE_APPLICATION(ZenithApplication)
