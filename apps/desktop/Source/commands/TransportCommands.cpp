#include "TransportCommands.h"
#include "Engine.h"
#include "ProjectState.h"
#include "CommandUtils.h"


namespace zenith {

TransportCommands::TransportCommands(Engine &eng, ProjectState &state)
    : engine(eng), projectState(state) {}

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
  projectState.setTempo(bpm);

  engine.syncTempoMap();

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("bpm", bpm);
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
}

juce::var TransportCommands::setTimeSignature(const juce::var &params) {
  if (!params.hasProperty("numerator") || !params.hasProperty("denominator"))
    return createErrorResponse("Missing numerator/denominator");

  int num = params["numerator"];
  int den = params["denominator"];

  projectState.setTimeSignature(num, den);
  return createSuccessResponse(juce::var());
}

juce::var TransportCommands::addTempoChange(const juce::var &params) {
  if (!params.hasProperty("timeBeats"))
    return createErrorResponse("Missing 'timeBeats'");
  if (!params.hasProperty("bpm"))
    return createErrorResponse("Missing 'bpm'");

  double timeBeats = (double)params["timeBeats"];
  double bpm = (double)params["bpm"];

  juce::String pointId =
      projectState.addTempoChange(timeBeats, bpm, "Add Tempo Change");

  engine.syncTempoMap();

  auto *resultObj = new juce::DynamicObject();
  resultObj->setProperty("pointId", pointId);
  resultObj->setProperty("success", true);

  return createSuccessResponse(juce::var(resultObj));
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

} // namespace zenith
