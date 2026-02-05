/*
  ==============================================================================

    MCPServer.cpp
    Created: 2025-12-23
    Author:  Zenith DAW Team

    MCP (Model Context Protocol) Server for AI integration.

  ==============================================================================
*/

#include "MCPServer.h"
#include "MCPResourceProviders.h"
#include "MCPToolSchemas.h"
#include "MCPUIInteractionTools.h"
#include "MCPVisionTools.h"
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace zenith {
namespace mcp {

//==============================================================================
// Construction
//==============================================================================

MCPServer::MCPServer(CommandAPI &api, ProjectState &state, Engine &engine)
    : commandAPI_(api), projectState_(state), engine_(engine),
      mainWindow_(nullptr) {}

MCPServer::MCPServer(CommandAPI &api, ProjectState &state, Engine &engine,
                     juce::Component *mainWindow)
    : commandAPI_(api), projectState_(state), engine_(engine),
      mainWindow_(mainWindow) {}

MCPServer::~MCPServer() { stop(); }

//==============================================================================
// Main Entry Points
//==============================================================================

void MCPServer::run() {
  if (running_.exchange(true))
    return;
  backgroundMode_ = false;

  log("[MCP] MCPServer: Starting main loop (blocking)");

  // Read lines from stdin
  std::string line;
  while (running_ && std::getline(std::cin, line)) {
    juce::String msg(line);
    if (msg.trim().isNotEmpty()) {
      processMessage(msg);
    }
  }

  log("[MCP] MCPServer: Main loop ended");
  running_ = false;
}

void MCPServer::startBackground() {
  if (running_.exchange(true))
    return;
  backgroundMode_ = true;

  log("[MCP] MCPServer: Starting background thread");

  backgroundThread_ = std::thread([this]() {
    std::string line;
    while (running_) {
      // Check for stdin data without blocking forever if possible,
      // but std::getline is standard.
      // In a real GUI app, stdin might not be connected if not launched from
      // CLI, but we assume it is or we'd handle it.
      if (std::getline(std::cin, line)) {
        juce::String msg(line);
        if (msg.trim().isNotEmpty()) {
          // Dispatch to message thread for processing!
          juce::MessageManager::callAsync(
              [this, msg]() { processMessage(msg); });
        }
      } else {
        // End of stream
        running_ = false;
        break;
      }
    }
    log("[MCP] MCPServer: Background thread ended");
    if (onStop) {
      juce::MessageManager::callAsync([this]() { onStop(); });
    }
  });

  backgroundThread_.detach(); // We manage lifetime via running_ flag
}

void MCPServer::stop() {
  running_ = false;
  // Note: std::getline is blocking, so we can't easily interrupt it
  // without platform-specific tricks or closing stdin.
}

//==============================================================================
// Message Messages
//==============================================================================

void MCPServer::processMessage(const juce::String &line) {
  juce::var json;
  juce::Result result = juce::JSON::parse(line, json);

  if (result.failed()) {
    logError("[MCP] JSON parse error: " + result.getErrorMessage());
    logError("[MCP] Message was: " + line);
    juce::var v;
    sendError(v, ErrorCode::ParseError, "Parse error");
    return;
  }

  // Debug incoming message
  log("[MCP] Received: " + line);

  if (json.isVoid()) {
    logError("[MCP] JSON is void");
    juce::var v;
    sendError(v, ErrorCode::ParseError, "Invalid JSON");
    return;
  }

  // JSON-RPC 2.0 Validation
  if (!json.hasProperty("jsonrpc") || json["jsonrpc"].toString() != "2.0") {
    logError("[MCP] Invalid JSON-RPC version");
    sendError(json["id"], ErrorCode::InvalidRequest,
              "Invalid JSON-RPC version");
    return;
  }

  if (!json.hasProperty("method")) {
    logError("[MCP] Missing method");
    sendError(json["id"], ErrorCode::InvalidRequest, "Missing method");
    return;
  }

  juce::String method = json["method"].toString();
  juce::var id = json["id"];
  juce::var params = json["params"];

  // LOG THE METHOD BEING DISPATCHED
  log("[MCP] Dispatching method: " + method);

  try {
    if (json.hasProperty("id")) {
      // Request (expects response)
      juce::var result = routeRequest(method, params);
      if (!result.isVoid()) {
        sendResponse(id, result);
      }
    } else {
      // Notification (no response)
      routeRequest(method, params);
    }
  } catch (const std::exception &e) {
    logError("[MCP] Exception in routeRequest: " + juce::String(e.what()));
    sendError(id, ErrorCode::InternalError, e.what());
  }
}

juce::var MCPServer::routeRequest(const juce::String &method,
                                  const juce::var &params) {
  if (method == "initialize")
    return handleInitialize(params);
  if (method == "initialized") {
    handleInitialized();
    return juce::var();
  }
  if (method == "tools/list")
    return handleToolsList(params);
  if (method == "tools/call")
    return handleToolsCall(params);
  if (method == "resources/list")
    return handleResourcesList(params);
  if (method == "resources/read")
    return handleResourcesRead(params);
  if (method == "ping")
    return handlePing(params);

  throw std::runtime_error("Method not found");
}

//==============================================================================
// Protocol Handlers
//==============================================================================

juce::var MCPServer::handleInitialize(const juce::var &params) {
  if (params.hasProperty("protocolVersion")) {
    log("[MCP] Client protocol version: " +
        params["protocolVersion"].toString());
  }

  auto *result = new juce::DynamicObject();
  result->setProperty("protocolVersion", PROTOCOL_VERSION);

  auto *capabilities = new juce::DynamicObject();

  // Tools capability
  auto *tools = new juce::DynamicObject();
  capabilities->setProperty("tools", tools);

  // Resources capability
  auto *resources = new juce::DynamicObject();
  capabilities->setProperty("resources", resources);

  // DynamicObject property assignment (avoiding deleted var constructor)
  juce::var capVar;
  capVar = capabilities;
  result->setProperty("capabilities", capVar);

  auto *serverInfo = new juce::DynamicObject();
  serverInfo->setProperty("name", SERVER_NAME);
  serverInfo->setProperty("version", SERVER_VERSION);

  juce::var infoVar;
  infoVar = serverInfo;
  result->setProperty("serverInfo", infoVar);

  initialized_ = true;
  log("[MCP] Initialization complete");

  juce::var v;
  v = result;
  return v;
}

void MCPServer::handleInitialized() {
  log("[MCP] Client initialized notification received");
}

juce::var MCPServer::handleToolsList(const juce::var &params) {
  juce::ignoreUnused(params);
  return buildToolsList();
}

juce::var MCPServer::handleToolsCall(const juce::var &params) {
  if (!params.hasProperty("name")) {
    throw std::runtime_error("Missing tool name");
  }

  juce::String toolName = params["name"].toString();
  juce::var arguments = params["arguments"];

  log("[MCP] Executing tool: " + toolName);

  try {
    // executeTool now delegates to CommandAPI AND new Vision/UI tools
    juce::var resultData = executeTool(toolName, arguments);

    auto *result = new juce::DynamicObject();

    juce::var content;
    auto *textItem = new juce::DynamicObject();

    if (toolName == "screenshot") {
      // Screenshot returns image_data object directly
      // MCP expects content list
      textItem->setProperty("type", "image");
      textItem->setProperty("data", resultData["image_data"]);
      textItem->setProperty("mimeType", "image/png");
    } else if (toolName == "get_ui_tree") {
      textItem->setProperty("type", "text");
      textItem->setProperty("text", juce::JSON::toString(resultData));
    } else {
      textItem->setProperty("type", "text");
      textItem->setProperty("text", juce::JSON::toString(resultData));
    }

    juce::var textItemVar;
    textItemVar = textItem;
    content.append(textItemVar);

    // DynamicObject property assignment
    result->setProperty("content", content);

    juce::var v;
    v = result;
    return v;
  } catch (const std::exception &e) {
    logError("Tool execution failed: " + juce::String(e.what()));

    auto *result = new juce::DynamicObject();
    result->setProperty("isError", true);

    juce::var content;
    auto *textItem = new juce::DynamicObject();
    textItem->setProperty("type", "text");
    textItem->setProperty("text", "Error: " + juce::String(e.what()));

    juce::var textItemVar;
    textItemVar = textItem;
    content.append(textItemVar);

    // DynamicObject property assignment
    result->setProperty("content", content);

    juce::var v;
    v = result;
    return v;
  }
}

juce::var MCPServer::handleResourcesList(const juce::var &params) {
  juce::ignoreUnused(params);

  auto *result = new juce::DynamicObject();
  result->setProperty("resources", buildResourcesList());

  juce::var v;
  v = result;
  return v;
}

juce::var MCPServer::handleResourcesRead(const juce::var &params) {
  if (!params.hasProperty("uri")) {
    throw std::runtime_error("Missing resource URI");
  }

  juce::String uri = params["uri"].toString();
  log("[MCP] Reading resource: " + uri);

  // Call member function readResource
  juce::var data = readResource(uri);

  auto *result = new juce::DynamicObject();

  juce::var contents;
  auto *item = new juce::DynamicObject();
  item->setProperty("uri", uri);
  item->setProperty("mimeType", "application/json");
  item->setProperty("text", juce::JSON::toString(data));

  juce::var itemVar;
  itemVar = item;
  contents.append(itemVar);

  // DynamicObject property assignment
  result->setProperty("contents", contents);

  juce::var v;
  v = result;
  return v;
}

juce::var MCPServer::handlePing(const juce::var &params) {
  juce::ignoreUnused(params);
  return juce::var();
}

//==============================================================================
// Tool Helpers
//==============================================================================

juce::var MCPServer::buildToolsList() {
  if (toolsListCached_)
    return cachedToolsList_;

  auto list = new juce::DynamicObject();

  // Use getAllToolSchemas from schemas namespace
  juce::var tools = zenith::mcp::schemas::getAllToolSchemas();

  // Add Vision Tools
  tools.append(zenith::mcp::vision::screenshotSchema());
  tools.append(zenith::mcp::vision::getUITreeSchema());

  // Add UI Interaction Tools
  tools.append(zenith::mcp::ui::clickSchema());
  tools.append(zenith::mcp::ui::clickComponentSchema());
  tools.append(zenith::mcp::ui::typeTextSchema());

  // Assign tools array
  list->setProperty("tools", tools);

  juce::var listVar;
  listVar = list;
  cachedToolsList_ = listVar;
  toolsListCached_ = true;
  return cachedToolsList_;
}

juce::var MCPServer::executeTool(const juce::String &toolName,
                                 const juce::var &arguments) {
  // 1. Vision Tools
  if (toolName == "screenshot") {
    return zenith::mcp::vision::executeScreenshot(mainWindow_, arguments);
  }
  if (toolName == "get_ui_tree") {
    float depth =
        arguments.hasProperty("depth") ? (float)arguments["depth"] : 5.0f;
    if (!mainWindow_)
      return juce::var();
    return zenith::mcp::vision::buildComponentTree(mainWindow_, 0, (int)depth);
  }

  // 2. UI Interaction Tools
  if (toolName == "click") {
    int x = (int)arguments["x"];
    int y = (int)arguments["y"];
    juce::String mods = arguments["modifiers"].toString();
    return zenith::mcp::ui::executeClick(mainWindow_, x, y, mods);
  }
  if (toolName == "click_component") {
    juce::String id = arguments["componentId"].toString();
    return zenith::mcp::ui::executeClickComponent(mainWindow_, id);
  }
  if (toolName == "type_text") {
    juce::String text = arguments["text"].toString();
    return zenith::mcp::ui::executeTypeText(mainWindow_, text);
  }

  // 3. CommandAPI Tools
  // Wrap the request in a JSON object with "command" and "params"
  auto *req = new juce::DynamicObject();
  req->setProperty("command", toolName);
  if (!arguments.isVoid()) {
    req->setProperty("params", arguments);
  }

  juce::var reqVar;
  reqVar = req;
  return commandAPI_.executeCommand(reqVar);
}

//==============================================================================
// Resource Helpers
//==============================================================================

juce::var MCPServer::buildResourcesList() {
  return zenith::mcp::resources::getResourcesList();
}

juce::var MCPServer::readResource(const juce::String &uri) {
  return zenith::mcp::resources::readResource(uri, commandAPI_, engine_,
                                              projectState_);
}

//==============================================================================
// Response Helpers
//==============================================================================

void MCPServer::sendResponse(const juce::var &id, const juce::var &result) {
  auto *response = new juce::DynamicObject();
  response->setProperty("jsonrpc", "2.0");
  response->setProperty("id", id);
  response->setProperty("result", result);

  juce::var v;
  v = response;
  writeMessage(v);
}

void MCPServer::sendError(const juce::var &id, ErrorCode code,
                          const juce::String &message, const juce::var &data) {
  auto *response = new juce::DynamicObject();
  response->setProperty("jsonrpc", "2.0");
  response->setProperty("id", id);

  auto *error = new juce::DynamicObject();
  error->setProperty("code", (int)code);
  error->setProperty("message", message);
  if (!data.isVoid()) {
    error->setProperty("data", data);
  }

  juce::var errorVar;
  errorVar = error;
  response->setProperty("error", errorVar);

  juce::var v;
  v = response;
  writeMessage(v);
}

void MCPServer::sendNotification(const juce::String &method,
                                 const juce::var &params) {
  auto *notification = new juce::DynamicObject();
  notification->setProperty("jsonrpc", "2.0");
  notification->setProperty("method", method);
  if (!params.isVoid()) {
    notification->setProperty("params", params);
  }

  juce::var v;
  v = notification;
  writeMessage(v);
}

void MCPServer::writeMessage(const juce::var &message) {
  juce::ScopedLock lock(outputLock_);
  juce::String json = juce::JSON::toString(message, true);

  // Write to stdout with content-length header (LSP-style) is NOT used by
  // simple MCP over stdio Standard MCP uses newline-delimited JSON or JSON-RPC
  // directly
  std::cout << json << std::endl;
}

//==============================================================================
// Logging
//==============================================================================

void MCPServer::log(const juce::String &message) {
  std::cerr << message << std::endl;
}

void MCPServer::logError(const juce::String &message) {
  std::cerr << "[ERROR] " << message << std::endl;
}

} // namespace mcp
} // namespace zenith
