#pragma once
#include <juce_core/juce_core.h>

namespace zenith {

// Forward declarations
class Engine;
class ProjectState;

class TransportCommands {
public:
  TransportCommands(Engine &engine, ProjectState &projectState);

  juce::var play(const juce::var &params);
  juce::var stop(const juce::var &params);
  juce::var record(const juce::var &params);
  juce::var rewind(const juce::var &params);
  juce::var setLoop(const juce::var &params);
  juce::var setTempo(const juce::var &params);
  juce::var setTimeSignature(const juce::var &params);
  juce::var addTempoChange(const juce::var &params);
  juce::var getTempoMap(const juce::var &params);

private:
  Engine &engine;
  ProjectState &projectState;
};

} // namespace zenith
