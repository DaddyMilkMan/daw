/**
 * @file Main.cpp
 * @brief Zenith DAW - Main application entry point
 *
 * This file initializes the JUCE application and the main window.
 */

// Prevent ProjectInfo redefinition when other headers include JuceHeader.h
#define JUCE_DONT_DECLARE_PROJECTINFO 1

// Include all JUCE modules using manual JuceHeader.h
#include "JuceHeader.h"
#include "engine/ProjectState.h"
#include "engine/Engine.h"
#include "commands/CommandAPI.h"
#include "utils/SampleGenerator.h"
#include "utils/PlatformSystemUtils.h"
#include "ui/design-system/FontManager.h"
#include "engine/ZenithLogger.h"
#include "Settings.h"
#include "ui/common/MainWindow.h"
#include "network/MCPServer.h"
#include <cstdlib>


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
    auto args = juce::StringArray::fromTokens(commandLine, true);
    const bool mcpServerMode = args.contains("--mcp-server");
    const bool mcpStdioGui = args.contains("--mcp-stdio");
    if (mcpStdioGui) {
#if JUCE_WINDOWS
      _putenv_s("MCP_STDIO", "1");
#else
      setenv("MCP_STDIO", "1", 1);
#endif
    }
    
    // Load settings immediately on startup
    // FIX: This was missing, causing changes to be lost on relaunch
    ::zenith::Settings::getInstance().load();

    // Log startup
    DBG("Zenith DAW starting...");
    DBG("Version: " + getApplicationVersion());
    DBG("JUCE Version: " + juce::SystemStats::getJUCEVersion());

    // Log system info
    ::zenith::PlatformSystemUtils::logSystemInfo();

    if (mcpServerMode) {
      mcpHeadless_ = true;

      mcpProjectState_ = std::make_unique<::zenith::ProjectState>();
      mcpEngine_ = std::make_unique<::zenith::Engine>();
      mcpCommandAPI_ = std::make_unique<::zenith::CommandAPI>(*mcpProjectState_, *mcpEngine_);

      mcpEngine_->setProjectState(mcpProjectState_.get());
      mcpEngine_->initialize();

      mcpServer_ = std::make_unique<::zenith::mcp::MCPServer>(
          *mcpCommandAPI_, *mcpProjectState_, *mcpEngine_, nullptr);
      mcpServer_->onStop = [this]() { quit(); };
      mcpServer_->start();

      ZENITH_LOG_INFO("[MCP STDIO] Headless MCP server running (--mcp-server)");
      return;
    }

    // Ensure content validity (Generate missing samples if needed)
    // Run asynchronously to unblock startup
    ::zenith::SampleGenerator::generateMissingSamples(&threadPool);

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

    // Stop background tasks
    threadPool.removeAllJobs(true, 4000);

    if (mcpHeadless_) {
      if (mcpServer_) {
        mcpServer_->stop();
        mcpServer_.reset();
      }
      if (mcpEngine_) {
        mcpEngine_->shutdown();
        mcpEngine_.reset();
      }
      mcpCommandAPI_.reset();
      mcpProjectState_.reset();
    }

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
  juce::ThreadPool threadPool;
  bool mcpHeadless_ = false;
  std::unique_ptr<::zenith::ProjectState> mcpProjectState_;
  std::unique_ptr<::zenith::Engine> mcpEngine_;
  std::unique_ptr<::zenith::CommandAPI> mcpCommandAPI_;
  std::unique_ptr<::zenith::mcp::MCPServer> mcpServer_;
};

//==============================================================================
START_JUCE_APPLICATION(ZenithApplication)
