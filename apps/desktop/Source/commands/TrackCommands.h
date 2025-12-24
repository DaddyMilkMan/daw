#pragma once
#include <juce_core/juce_core.h>

namespace zenith {

// Forward declarations
class Engine;
class ProjectState;
class CommandAPI;

class TrackCommands {
public:
  TrackCommands(Engine &engine, ProjectState &projectState, CommandAPI &api);

  juce::var listTracks(const juce::var &params);
  juce::var createTrack(const juce::var &params);
  juce::var deleteTrack(const juce::var &params);
  juce::var renameTrack(const juce::var &params);
  juce::var setTrackVolume(const juce::var &params);
  juce::var setTrackPan(const juce::var &params);
  juce::var setTrackSend(const juce::var &params);
  juce::var setTrackEQ(const juce::var &params);
  juce::var setTrackCompressor(const juce::var &params);
  juce::var separateTrack(const juce::var &params);

private:
  Engine &engine;
  ProjectState &projectState;
  CommandAPI &api;
};

} // namespace zenith
