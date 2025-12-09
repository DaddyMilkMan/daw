#pragma once
#include <juce_core/juce_core.h>

namespace zenith {

// Forward declarations
class Engine;
class ProjectState;

class ClipCommands {
public:
    ClipCommands(Engine &engine, ProjectState &projectState);

    juce::var listClips(const juce::var &params);
    juce::var createClip(const juce::var &params);
    juce::var deleteClip(const juce::var &params);
    juce::var splitClip(const juce::var &params);
    juce::var moveClip(const juce::var &params);
    juce::var resizeClip(const juce::var &params);
    juce::var setClipNotes(const juce::var &params);

private:
    Engine &engine;
    ProjectState &projectState;
};

} // namespace zenith
