#include <memory>
#include <utility>
#include <vector>

#include "Actions.h"
#include "CommandAPI.h"
#include "CommandUtils.h"
#include "Engine.h"
#include "ProjectState.h"
#include "TransportCommands.h"

namespace zenith {

TransportCommands::TransportCommands(Engine &eng, ProjectState &state,
                                     CommandAPI &api)
    : engine(eng), projectState(state), api(api) {}

juce::var TransportCommands::play(const juce::var &params) {
  juce::ignoreUnused(params);
  engine.play();
  return createSuccessResponse(juce::var());
}

juce::var TransportCommands::stop(const juce::var &params) {
  juce::ignoreUnused(params);
  engine.stop();
  return createSuccessResponse(juce::var());
}

juce::var TransportCommands::record(const juce::var &params) {
  juce::ignoreUnused(params);
  engine.record();
  return createSuccessResponse(juce::var());
}

juce::var TransportCommands::rewind(const juce::var &params) {
  juce::ignoreUnused(params);
  engine.setPlayheadSamples(0);
  return createSuccessResponse(juce::var());
}

juce::var TransportCommands::setLoop(const juce::var &params) {
  if (!params.hasProperty("enabled"))
    return createErrorResponse("Missing 'enabled' parameter");

  bool enabled = params["enabled"];
  engine.setLooping(enabled);

  if (params.hasProperty("start") && params.hasProperty("end")) {
    juce::int64 start = params["start"];
    juce::int64 end = params["end"];
    engine.setLoopRegion(start, end);
  }

  return createSuccessResponse(juce::var());
}

juce::var TransportCommands::setTempo(const juce::var &params) {
  if (!params.hasProperty("bpm"))
    return createErrorResponse("Missing 'bpm'");

  double bpm = (double)params["bpm"];

  if (api.performAction(std::make_unique<SetTempoAction>(projectState, bpm))) {
    engine.syncTempoMap();

    auto *resultObj = new juce::DynamicObject();
    resultObj->setProperty("bpm", bpm);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
  }

  return createErrorResponse("Failed to set tempo");
}

juce::var TransportCommands::setTimeSignature(const juce::var &params) {
  if (!params.hasProperty("numerator") || !params.hasProperty("denominator"))
    return createErrorResponse("Missing numerator/denominator");

  int num = params["numerator"];
  int den = params["denominator"];

  if (api.performAction(
          std::make_unique<SetTimeSignatureAction>(projectState, num, den))) {
    return createSuccessResponse(juce::var());
  }

  return createErrorResponse("Failed to set time signature");
}

juce::var TransportCommands::addTempoChange(const juce::var &params) {
  if (!params.hasProperty("timeBeats"))
    return createErrorResponse("Missing 'timeBeats'");
  if (!params.hasProperty("bpm"))
    return createErrorResponse("Missing 'bpm'");

  double timeBeats = (double)params["timeBeats"];
  double bpm = (double)params["bpm"];

  auto action =
      std::make_unique<AddTempoPointAction>(projectState, timeBeats, bpm);
  auto *rawAction = action.get();

  if (api.performAction(std::move(action))) {
    engine.syncTempoMap();

    juce::String pointId = rawAction->getPointId();
    auto *resultObj = new juce::DynamicObject();
    resultObj->setProperty("pointId", pointId);
    resultObj->setProperty("success", true);

    return createSuccessResponse(juce::var(resultObj));
  }

  return createErrorResponse("Failed to add tempo change");
}

juce::var TransportCommands::getTempoMap(const juce::var &params) {
  juce::ignoreUnused(params);

  juce::var pointsArray;
  auto *pointsArrayPtr = pointsArray.getArray();

  juce::ValueTree tempoMap = projectState.getTempoMap();
  int numPoints = tempoMap.getNumChildren();

  for (int i = 0; i < numPoints; ++i) {
    auto point = tempoMap.getChild(i);
    // Ensure we only process tempo points
    if (point.hasType(ProjectState::ID_TEMPO_POINT)) {
      auto *pointObj = new juce::DynamicObject();
      pointObj->setProperty(
          "id", point.getProperty(ProjectState::PROP_ID).toString());
      pointObj->setProperty("timeBeats",
                            point.getProperty(ProjectState::PROP_TIME_BEATS));
      pointObj->setProperty("bpm", point.getProperty(ProjectState::PROP_BPM));

      pointsArrayPtr->add(juce::var(pointObj));
    }
  }

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("points", pointsArray);
  resultObj->setProperty("count", (int)pointsArray.size());
  resultObj->setProperty("currentTempo", projectState.getTempo());
  resultObj->setProperty("timeData", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var TransportCommands::playAt(const juce::var &params) {
  if (!params.hasProperty("whenMs"))
    return createErrorResponse("Missing 'whenMs' parameter");

  int64_t whenMs = static_cast<int64_t>(params["whenMs"]);
  double positionSeconds = params.hasProperty("positionSeconds") 
    ? static_cast<double>(params["positionSeconds"]) 
    : -1.0;

  bool success = engine.getTransportController().playAt(whenMs, positionSeconds);
  
  if (success) {
    auto *resultObj = new juce::DynamicObject();
    resultObj->setProperty("scheduled", true);
    resultObj->setProperty("whenMs", whenMs);
    if (positionSeconds >= 0.0) {
      resultObj->setProperty("positionSeconds", positionSeconds);
    }
    return createSuccessResponse(juce::var(resultObj));
  }
  
  return createErrorResponse("Failed to schedule play action (queue full)");
}

juce::var TransportCommands::stopAt(const juce::var &params) {
  if (!params.hasProperty("whenMs"))
    return createErrorResponse("Missing 'whenMs' parameter");

  int64_t whenMs = static_cast<int64_t>(params["whenMs"]);

  bool success = engine.getTransportController().stopAt(whenMs);
  
  if (success) {
    auto *resultObj = new juce::DynamicObject();
    resultObj->setProperty("scheduled", true);
    resultObj->setProperty("whenMs", whenMs);
    return createSuccessResponse(juce::var(resultObj));
  }
  
  return createErrorResponse("Failed to schedule stop action (queue full)");
}

juce::var TransportCommands::seekAt(const juce::var &params) {
  if (!params.hasProperty("whenMs"))
    return createErrorResponse("Missing 'whenMs' parameter");
  if (!params.hasProperty("positionSeconds"))
    return createErrorResponse("Missing 'positionSeconds' parameter");

  int64_t whenMs = static_cast<int64_t>(params["whenMs"]);
  double positionSeconds = static_cast<double>(params["positionSeconds"]);

  bool success = engine.getTransportController().seekAt(whenMs, positionSeconds);
  
  if (success) {
    auto *resultObj = new juce::DynamicObject();
    resultObj->setProperty("scheduled", true);
    resultObj->setProperty("whenMs", whenMs);
    resultObj->setProperty("positionSeconds", positionSeconds);
    return createSuccessResponse(juce::var(resultObj));
  }
  
  return createErrorResponse("Failed to schedule seek action (queue full)");
}

} // namespace zenith
