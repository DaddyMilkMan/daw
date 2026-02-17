/*
    Minimal CommandAPI stub implementation for builds where full command
    handlers are unavailable or out of sync.
*/

#include "CommandAPI.h"
#include "TrackCommands.h"
#include "ClipCommands.h"
#include "TransportCommands.h"

namespace zenith {

CommandAPI::CommandAPI(ProjectState& state, Engine& eng)
    : projectState(state), engine(eng) {
  initializeCommandMap();
}

CommandAPI::~CommandAPI() = default;

void CommandAPI::initializeCommandMap() {
  commandMap.clear();
}

juce::var CommandAPI::executeCommand(const juce::var& request) {
  juce::ignoreUnused(request);
  return createErrorResponse("Command API is running in stub mode");
}

juce::var CommandAPI::executeCommand(CommandID id, const juce::var& params) {
  juce::ignoreUnused(id, params);
  return createErrorResponse("Command API is running in stub mode");
}

juce::String CommandAPI::executeCommand(const juce::String& jsonRequest) {
  juce::ignoreUnused(jsonRequest);
  return "{\"status\":\"error\",\"message\":\"Command API is running in stub mode\"}";
}

juce::var CommandAPI::createSuccessResponse(const juce::var& data) const {
  juce::DynamicObject::Ptr r = new juce::DynamicObject();
  r->setProperty("status", "ok");
  r->setProperty("data", data);
  return juce::var(r.get());
}

juce::String CommandAPI::createErrorResponse(const juce::String& message) const {
  juce::DynamicObject::Ptr r = new juce::DynamicObject();
  r->setProperty("status", "error");
  r->setProperty("message", message);
  return juce::JSON::toString(juce::var(r.get()));
}

}  // namespace zenith
