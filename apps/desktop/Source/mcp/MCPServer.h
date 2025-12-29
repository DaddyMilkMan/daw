/*
  ==============================================================================

    MCPServer.h
    Created: 2025-12-23
    Author:  Zenith DAW Team

    MCP (Model Context Protocol) Server for AI integration.
    Allows AI models like Claude, Gemini, etc. to control Zenith DAW
    via the standardized MCP protocol over stdio.

  ==============================================================================
*/

#pragma once

#include "../commands/CommandAPI.h"
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <thread>

namespace zenith {
namespace mcp {

/**
 * @brief MCP Server implementation for Zenith DAW
 */
class MCPServer {
public:
  //==========================================================================
  // Protocol Constants
  //==========================================================================
  static constexpr const char *PROTOCOL_VERSION = "2024-11-05";
  static constexpr const char *SERVER_NAME = "zenith-daw";
  static constexpr const char *SERVER_VERSION = "1.0.0";

  //==========================================================================
  // JSON-RPC Error Codes
  //==========================================================================
  enum class ErrorCode : int {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    // MCP-specific errors
    ResourceNotFound = -32001,
    ToolNotFound = -32002,
    ToolExecutionFailed = -32003
  };

  //==========================================================================
  // Construction
  //==========================================================================
  /**
   * @brief Headless server constructor
   */
  MCPServer(CommandAPI &api, ProjectState &state, Engine &engine);

  /**
   * @brief GUI-embedded server constructor
   */
  MCPServer(CommandAPI &api, ProjectState &state, Engine &engine,
            juce::Component *mainWindow);

  ~MCPServer();

  //==========================================================================
  // Main Entry Points
  //==========================================================================

  /**
   * @brief Run the MCP server main loop (blocking)
   * Designed for headless mode (--mcp-server)
   */
  void run();

  /**
   * @brief Start the MCP server in a background thread
   * Designed for embedded mode (GUI app)
   */
  void startBackground();

  /**
   * @brief Signal the server to stop
   */
  void stop();

  /**
   * @brief Check if server is running
   */
  bool isRunning() const { return running_.load(); }

  /**
   * @brief Check if server is running in background mode
   */
  bool isBackgroundMode() const { return backgroundMode_; }

  //==========================================================================
  // Protocol Handlers
  //==========================================================================

  juce::var handleInitialize(const juce::var &params);
  void handleInitialized();
  juce::var handleToolsList(const juce::var &params);
  juce::var handleToolsCall(const juce::var &params);
  juce::var handleResourcesList(const juce::var &params);
  juce::var handleResourcesRead(const juce::var &params);
  juce::var handlePing(const juce::var &params);

  //==========================================================================
  // Callbacks
  //==========================================================================
  std::function<void()> onStop;

private:
  //==========================================================================
  // Message Processing
  //==========================================================================

  void processMessage(const juce::String &line);
  juce::var routeRequest(const juce::String &method, const juce::var &params);

  //==========================================================================
  // Response Helpers
  //==========================================================================

  void sendResponse(const juce::var &id, const juce::var &result);
  void sendError(const juce::var &id, ErrorCode code,
                 const juce::String &message,
                 const juce::var &data = juce::var());
  void sendNotification(const juce::String &method, const juce::var &params);
  void writeMessage(const juce::var &message);

  //==========================================================================
  // Tool Helpers
  //==========================================================================

  juce::var buildToolsList();
  juce::var executeTool(const juce::String &toolName,
                        const juce::var &arguments);

  //==========================================================================
  // Resource Helpers
  //==========================================================================

  juce::var buildResourcesList();
  juce::var readResource(const juce::String &uri);

  //==========================================================================
  // Logging
  //==========================================================================

  void log(const juce::String &message);
  void logError(const juce::String &message);

  //==========================================================================
  // Members
  //==========================================================================
  CommandAPI &commandAPI_;
  ProjectState &projectState_;
  Engine &engine_;
  juce::Component *mainWindow_ = nullptr;

  std::atomic<bool> running_{false};
  bool initialized_ = false;
  bool backgroundMode_ = false;

  std::thread backgroundThread_;

  // Cached tool list
  juce::var cachedToolsList_;
  bool toolsListCached_ = false;

  // Mutex for thread-safe output
  juce::CriticalSection outputLock_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MCPServer)
};

} // namespace mcp
} // namespace zenith
