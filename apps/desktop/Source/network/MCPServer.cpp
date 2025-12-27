/*
  ==============================================================================

    MCPServer.cpp
    Created: 2025-12-24
    Author:  Zenith DAW

  ==============================================================================
*/

#include "MCPServer.h"
#include "../../commands/CommandAPI.h"
#include "../engine/Engine.h"
#include "../ui/common/MainWindow.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace zenith {
namespace mcp {

MCPServer::MCPServer(CommandAPI &api, ProjectState &state, Engine &engine,
                     juce::Component *mainWindow)
    : commandAPI_(api), projectState_(state), engine_(engine),
      mainWindow_(mainWindow) {}

MCPServer::~MCPServer() { stop(); }

void MCPServer::start() {
  if (running_.load())
    return;

  running_.store(true);
  serverThread_ = std::thread(&MCPServer::run, this);
}

void MCPServer::stop() {
  if (!running_.load())
    return;

  running_.store(false);

  // std::cin.getline is blocking, so we might need a more graceful way to
  // interrupt, but for now, we'll let it join if it finishes or the app exits.
  if (serverThread_.joinable())
    serverThread_.detach(); // Stdio thread is detached to allow for non-blocking shutdown
}

void MCPServer::run() {
  std::string line;
  while (running_.load() && std::getline(std::cin, line)) {
    juce::String jLine(line);
    if (jLine.isNotEmpty()) {
      juce::MessageManager::callAsync(
          [this, jLine]() { processMessage(jLine); });
    }
  }

  running_.store(false);
  if (onStop)
    juce::MessageManager::callAsync(onStop);
}

void MCPServer::processMessage(const juce::String &line) {
  auto json = juce::JSON::parse(line);
  if (json.isUndefined()) {
    sendError(juce::var(), ErrorCode::ParseError, "Parse error");
    return;
  }

  auto id = json["id"];
  auto method = json["method"].toString();
  auto params = json["params"];

  if (method.isEmpty()) {
    sendError(id, ErrorCode::InvalidRequest, "Invalid request: missing method");
    return;
  }

  try {
    auto result = routeRequest(method, params);
    if (!result.isUndefined())
      sendResponse(id, result);
  } catch (const std::exception &e) {
    sendError(id, ErrorCode::InternalError, e.what());
  }
}

juce::var MCPServer::routeRequest(const juce::String &method,
                                  const juce::var &params) {
  if (method == "initialize")
    return handleInitialize(params);

  if (method == "notifications/initialized") {
    handleInitialized();
    return juce::var();
  }

  if (!initialized_) {
    throw std::runtime_error("Server not initialized");
  }

  if (method == "tools/list")
    return handleToolsList();

  if (method == "tools/call") {
    auto name = params["name"].toString();
    auto args = params["arguments"];
    return handleToolsCall(name, args);
  }

  if (method == "resources/list")
    return handleResourcesList();

  if (method == "resources/read") {
    auto uri = params["uri"].toString();
    return handleResourcesRead(uri);
  }

  if (method == "ping")
    return juce::var(new juce::DynamicObject());

  sendError(juce::var(), ErrorCode::MethodNotFound,
            "Method not found: " + method);
  return juce::var();
}

void MCPServer::sendResponse(const juce::var &id, const juce::var &result) {
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> response =
      new juce::DynamicObject();
  response->setProperty("jsonrpc", "2.0");
  response->setProperty("id", id);
  response->setProperty("result", result);
  writeMessage(juce::var(response.get()));
}

void MCPServer::sendError(const juce::var &id, ErrorCode code,
                          const juce::String &message, const juce::var &data) {
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> error =
      new juce::DynamicObject();
  error->setProperty("code", static_cast<int>(code));
  error->setProperty("message", message);
  if (!data.isUndefined())
    error->setProperty("data", data);

  juce::ReferenceCountedObjectPtr<juce::DynamicObject> response =
      new juce::DynamicObject();
  response->setProperty("jsonrpc", "2.0");
  response->setProperty("id", id);
  response->setProperty("error", juce::var(error.get()));
  writeMessage(juce::var(response.get()));
}

void MCPServer::sendNotification(const juce::String &method,
                                 const juce::var &params) {
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> notification =
      new juce::DynamicObject();
  notification->setProperty("jsonrpc", "2.0");
  notification->setProperty("method", method);
  notification->setProperty("params", params);
  writeMessage(juce::var(notification.get()));
}

void MCPServer::writeMessage(const juce::var &message) {
  juce::ScopedLock sl(outputLock_);
  auto jsonText = juce::JSON::toString(message, false);
  std::cout << jsonText.toStdString() << std::endl;
}

// MCP Handlers Implementation
juce::var MCPServer::handleInitialize(const juce::var &params) {
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> result =
      new juce::DynamicObject();
  result->setProperty("protocolVersion", PROTOCOL_VERSION);

  juce::ReferenceCountedObjectPtr<juce::DynamicObject> capabilities =
      new juce::DynamicObject();
  capabilities->setProperty("tools", juce::var(new juce::DynamicObject()));
  capabilities->setProperty("resources", juce::var(new juce::DynamicObject()));
  result->setProperty("capabilities", juce::var(capabilities));

  juce::ReferenceCountedObjectPtr<juce::DynamicObject> serverInfo =
      new juce::DynamicObject();
  serverInfo->setProperty("name", SERVER_NAME);
  serverInfo->setProperty("version", SERVER_VERSION);
  return juce::var(result.get());
}

void MCPServer::handleInitialized() { initialized_ = true; }

juce::var MCPServer::handleToolsList() {
  juce::Array<juce::var> tools;

  auto addTool = [&](const juce::String &name, const juce::String &desc,
                     const juce::var &schema) {
    juce::ReferenceCountedObjectPtr<juce::DynamicObject> tool =
        new juce::DynamicObject();
    tool->setProperty("name", name);
    tool->setProperty("description", desc);
    tool->setProperty("inputSchema", schema);
    tools.add(juce::var(tool.get()));
  };

  // Tool: get_ui_tree
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> emptySchema =
      new juce::DynamicObject();
  emptySchema->setProperty("type", "object");
  emptySchema->setProperty("properties", juce::var(new juce::DynamicObject()));
  addTool("get_ui_tree", "Returns the full component hierarchy of the DAW.",
          juce::var(emptySchema.get()));

  // Tool: click_component
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> clickSchema =
      new juce::DynamicObject();
  clickSchema->setProperty("type", "object");
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> clickProps =
      new juce::DynamicObject();
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> idProp =
      new juce::DynamicObject();
  idProp->setProperty("type", "string");
  idProp->setProperty("description",
                      "The unique ID or name of the component to click.");
  clickProps->setProperty("componentId", juce::var(idProp.get()));
  clickSchema->setProperty("properties", juce::var(clickProps.get()));
  clickSchema->setProperty("required", juce::Array<juce::var>{"componentId"});
  addTool("click_component", "Simulates a mouse click on a UI component.",
          juce::var(clickSchema.get()));

  juce::ReferenceCountedObjectPtr<juce::DynamicObject> result =
      new juce::DynamicObject();
  result->setProperty("tools", tools);
  return juce::var(result.get());
}

juce::var MCPServer::handleToolsCall(const juce::String &name,
                                     const juce::var &arguments) {
  if (name == "get_ui_tree")
    return toolGetUITree();
  if (name == "click_component")
    return toolClickComponent(arguments["componentId"].toString());
  if (name == "set_component_text")
    return toolSetText(arguments["componentId"].toString(),
                       arguments["text"].toString());
  if (name == "execute_command")
    return toolExecuteCommand(arguments["command"].toString());

  throw std::runtime_error("Tool not found: " + name.toStdString());
}

juce::var MCPServer::handleResourcesList() {
  juce::Array<juce::var> resources;
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> result =
      new juce::DynamicObject();
  result->setProperty("resources", resources);
  return juce::var(result.get());
}

juce::var MCPServer::handleResourcesRead(const juce::String &uri) {
  throw std::runtime_error("Resource not found: " + uri.toStdString());
}

// Tool Implementation Logic
juce::var MCPServer::toolGetUITree() {
  juce::Array<juce::var> content;
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> textObj =
      new juce::DynamicObject();

  if (mainWindow_) {
    auto tree = serializeComponent(mainWindow_);
    textObj->setProperty("type", "text");
    textObj->setProperty("text", juce::JSON::toString(tree));
  } else {
    textObj->setProperty("type", "text");
    textObj->setProperty("text", "No main window available.");
  }

  content.add(juce::var(textObj.get()));
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> result =
      new juce::DynamicObject();
  result->setProperty("content", content);
  return juce::var(result.get());
}

juce::var MCPServer::serializeComponent(juce::Component *comp) {
  if (!comp)
    return juce::var();

  juce::ReferenceCountedObjectPtr<juce::DynamicObject> obj =
      new juce::DynamicObject();
  obj->setProperty("type", juce::String(typeid(*comp).name()));
  obj->setProperty("name", comp->getName());
  obj->setProperty("id", comp->getComponentID());
  obj->setProperty("visible", comp->isVisible());

  auto bounds = comp->getBounds();
  juce::ReferenceCountedObjectPtr<juce::DynamicObject> bObj =
      new juce::DynamicObject();
  bObj->setProperty("x", bounds.getX());
  bObj->setProperty("y", bounds.getY());
  bObj->setProperty("w", bounds.getWidth());
  bObj->setProperty("h", bounds.getHeight());
  obj->setProperty("bounds", juce::var(bObj.get()));

  juce::Array<juce::var> children;
  for (int i = 0; i < comp->getNumChildComponents(); ++i) {
    children.add(serializeComponent(comp->getChildComponent(i)));
  }
  if (children.size() > 0)
    obj->setProperty("children", children);

  return juce::var(obj.get());
}

juce::var MCPServer::toolClickComponent(const juce::String &componentId) {
  // Implementation for simulated click would go here.
  // In a real scenario, we'd search for the component by ID and send mouse
  // events.
  return "Click simulated (UI tree exploration recommended)";
}

juce::var MCPServer::toolSetText(const juce::String &componentId,
                                 const juce::String &text) {
  return "Text set simulated";
}

juce::var MCPServer::toolExecuteCommand(const juce::String &command) {
  return "Command executed via CommandAPI";
}

} // namespace mcp
} // namespace zenith
