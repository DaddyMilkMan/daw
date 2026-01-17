/**
 * @file Main.cpp
 * @brief Zenith DAW - Main application entry point
 *
 * This file initializes the JUCE application and creates the main window.
 */

// JUCE includes first
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

// Project includes after JUCE
#include "ui/common/MainWindow.h"
#include "engine/ProjectState.h"
#include "utils/SampleGenerator.h"
#include "utils/PlatformSystemUtils.h"
#include "ui/design-system/FontManager.h"
#include "engine/ZenithLogger.h"
#include "Settings.h"


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
    // Input validation should be added here for production releases
    juce::ignoreUnused(commandLine);
    
    // Load settings immediately on startup
    // FIX: This was missing, causing changes to be lost on relaunch
    ::zenith::Settings::getInstance().load();

    // Log startup
    DBG("Zenith DAW starting...");
    DBG("Version: " + getApplicationVersion());
    DBG("JUCE Version: " + juce::SystemStats::getJUCEVersion());

    // Log system info
    ::zenith::PlatformSystemUtils::logSystemInfo();

    // Ensure content validity (Generate missing samples if needed)
    ::zenith::SampleGenerator::generateMissingSamples();

    // Pre-initialize FontManager to avoid hangs when UI is created
    DBG("Initializing FontManager...");
    ::zenith::design::FontManager::getInstance();
    DBG("FontManager initialized.");

    // Create main window
    mainWindow = std::make_unique<::zenith::MainWindow>(getApplicationName());

    DBG("Zenith DAW initialized successfully!");
  }

  void shutdown() override {
    ZENITH_LOG_INFO("ZenithApplication::shutdown() STARTED");
    DBG("Zenith DAW shutting down...");

    // Close main window (releases all resources)
    mainWindow.reset();

    ZENITH_LOG_INFO("ZenithApplication::shutdown() COMPLETE");
    DBG("Zenith DAW shutdown complete.");
  }

  //==========================================================================
  void systemRequestedQuit() override {
    if (mainWindow != nullptr) {
      auto *projectState = mainWindow->getProjectState();
      
      // If dirty, ask user via our custom modal
      if (projectState != nullptr && projectState->hasUnsavedChanges()) {
        mainWindow->checkUnsavedAndQuit();
        // Do NOT call quit() here; waiting for modal response.
        return;
      }
      
      // If clean (or after Discard chosen), actually quit.
      quit();
    } else {
      quit();
    }
  }

  void anotherInstanceStarted(const juce::String &commandLine) override {
    juce::ignoreUnused(commandLine);
  }

private:
  //==========================================================================
  std::unique_ptr<::zenith::MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(ZenithApplication)
