/*
  ==============================================================================

    MCPServer.h
    Created: 2025-12-24
    Author:  Zenith DAW

    Implementation of Model Context Protocol Server for AI agent integration.
    Allows inspection of UI tree and command execution via JSON-RPC over stdio.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <string>
#include <thread>

namespace zenith {
class CommandAPI;
class ProjectState;
class Engine;

namespace mcp {

/**
 * @class MCPServer
 * @brief Provides a Model Context Protocol interface for AI agents.
 *
 * This server implements the MCP spec over standard input/output.
 * It allows agents to discover capabilities, inspect the DAW state,
 * and execute commands.
 */
class MCPServer {
public:
  static constexpr const char *PROTOCOL_VERSION = "2024-11-05";
  static constexpr const char *SERVER_NAME = "zenith-mcp";
  static constexpr const char *SERVER_VERSION = "1.1.0";

  enum class ErrorCode : int {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ResourceNotFound = -32001,
    ToolNotFound = -32002,
    ToolExecutionFailed = -32003
  };

  MCPServer(CommandAPI &api, ProjectState &state, Engine &engine,
            juce::Component *mainWindow = nullptr);
  ~MCPServer();

  /** Starts the server in a background thread */
  void start();

  /** Stops the server */
  void stop();

  bool isRunning() const { return running_.load(); }

  /** Callback called when the server stops or encounters a fatal error */
  std::function<void()> onStop;

private:
  void run();
  void processMessage(const juce::String &line);
  juce::var routeRequest(const juce::String &method, const juce::var &params);

  // JSON-RPC Helpers
  void sendResponse(const juce::var &id, const juce::var &result);
  void sendError(const juce::var &id, ErrorCode code,
                 const juce::String &message,
                 const juce::var &data = juce::var());
  void sendNotification(const juce::String &method, const juce::var &params);
  void writeMessage(const juce::var &message);

  // MCP Handlers
  juce::var handleInitialize(const juce::var &params);
  void handleInitialized();
  juce::var handleToolsList();
  juce::var handleToolsCall(const juce::String &name,
                            const juce::var &arguments);
  juce::var handleResourcesList();
  juce::var handleResourcesRead(const juce::String &uri);

  // Tool Implementations
  juce::var toolGetUITree();
  juce::var toolClickComponent(const juce::String &componentId);
  juce::var toolSetText(const juce::String &componentId,
                        const juce::String &text);
  juce::var toolExecuteCommand(const juce::String &command);

  // Tree Helpers
  juce::var serializeComponent(juce::Component *comp);

  CommandAPI &commandAPI_;
  ProjectState &projectState_;
  Engine &engine_;
  juce::Component *mainWindow_;

  std::atomic<bool> running_{false};
  bool initialized_ = false;
  std::thread serverThread_;
  juce::CriticalSection outputLock_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MCPServer)
};

} // namespace mcp
} // namespace zenith
