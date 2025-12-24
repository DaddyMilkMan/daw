/**
 * @file Main.cpp
 * @brief Zenith DAW - Main application entry point
 *
 * This file initializes the JUCE application and creates the main window.
 */

#include "MainWindow.h"
#include "mcp/MCPServer.h"
#include "utils/PlatformSystemUtils.h"
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
    // Check for MCP server mode
    if (commandLine.contains("--mcp-server")) {
      DBG("Zenith DAW starting in MCP server mode...");
      runMCPServer();
      return;
    }

    // Log startup
    DBG("Zenith DAW starting...");
    DBG("Version: " + getApplicationVersion());
    DBG("JUCE Version: " + juce::SystemStats::getJUCEVersion());

    // Log system info
    zenith::PlatformSystemUtils::logSystemInfo();

    // Ensure content validity (Generate missing samples if needed)
    zenith::SampleGenerator::generateMissingSamples();

    // Create main window
    mainWindow = std::make_unique<MainWindow>(getApplicationName());

    DBG("Zenith DAW initialized successfully!");
  }

  /**
   * @brief Run as a headless MCP server
   *
   * This mode allows AI models (Claude, Gemini, etc.) to control the DAW
   * via the Model Context Protocol over stdin/stdout.
   */
  void runMCPServer() {
    // Create engine and project state for headless mode
    engine = std::make_unique<zenith::Engine>();
    projectState = std::make_unique<zenith::ProjectState>();

    // Initialize engine
    engine->initialize();
    engine->setProjectState(projectState.get());

    // Create CommandAPI
    commandAPI = std::make_unique<zenith::CommandAPI>(*projectState, *engine);

    // Create and run MCP server
    mcpServer = std::make_unique<zenith::mcp::MCPServer>(
        *commandAPI, *projectState, *engine);

    // Set callback to quit when server stops (EOF)
    mcpServer->onStop = [this]() {
      DBG("MCP Server stopped (EOF). Quitting...");
      quit();
    };

    DBG("MCP Server ready - listening on stdin");
    mcpServer->startBackground();

    // Do NOT quit here. Return to let message loop run.
  }

  void shutdown() override {
    DBG("Zenith DAW shutting down...");

    if (mcpServer) {
      mcpServer->stop();
      mcpServer.reset();
    }

    // Close main window (releases all resources)
    mainWindow.reset();

    commandAPI.reset();
    engine.reset();
    projectState.reset();

    DBG("Zenith DAW shutdown complete.");
  }

  // ... (systemRequestedQuit implementation remains same) ...
  void systemRequestedQuit() override {
    if (mcpServer) {
      // If headless, just quit
      quit();
      return;
    }

    if (mainWindow != nullptr) {
      auto *ps = mainWindow->getProjectState();
      if (ps != nullptr && ps->hasUnsavedChanges()) {
        int result = juce::NativeMessageBox::showYesNoCancelBox(
            juce::AlertWindow::WarningIcon, "Unsaved Changes",
            "You have unsaved changes. Do you want to save before quitting?",
            mainWindow.get(), nullptr);

        const int RESULT_YES = 1;
        const int RESULT_NO = 2;

        if (result == RESULT_YES) {
          mainWindow->saveProject();
          quit();
        } else if (result == RESULT_NO) {
          quit();
        }
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
  std::unique_ptr<zenith::Engine> engine;
  std::unique_ptr<zenith::ProjectState> projectState;
  std::unique_ptr<zenith::CommandAPI> commandAPI;
  std::unique_ptr<zenith::mcp::MCPServer> mcpServer;
  std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(ZenithApplication)
