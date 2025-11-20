/*
  ==============================================================================

    SessionGraph.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 5: Wingman v0

    Project state serializer for AI context

    Responsibilities:
    - Export full project state as structured JSON
    - Include transport status, tracks, clips, plugins
    - Provide snapshot for AI reasoning about project

    Output format:
    {
        "transport": {
            "isPlaying": bool,
            "playheadSamples": int,
            "tempo": float,
            "timeSigNumerator": int,
            "timeSigDenominator": int
        },
        "tracks": [
            {
                "id": "track_0",
                "name": "Track Name",
                "type": "audio" | "midi",
                "volume": float (dB),
                "pan": float (-1.0 to 1.0),
                "muted": bool,
                "soloed": bool,
                "plugins": [
                    {
                        "name": "Plugin Name",
                        "format": "VST3",
                        "bypassed": bool
                    }
                ],
                "clips": [
                    {
                        "id": "clip_0",
                        "type": "audio" | "midi",
                        "name": "Clip Name",
                        "startSamples": int,
                        "lengthSamples": int,
                        "audioFile": "path/to/file.wav" (audio clips),
                        "midiNoteCount": int (MIDI clips)
                    }
                ]
            }
        ]
    }

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../engine/Track.h"

// Forward declarations
class Engine;
class ProjectState;

namespace zenith {

//==============================================================================
/**
    Serializes complete project state to JSON for AI context
*/
class SessionGraph
{
public:
    //==============================================================================
    SessionGraph(Engine& eng, ProjectState& state);
    ~SessionGraph();

    //==============================================================================
    /**
        Generate complete session graph as JSON
        Returns juce::var object representing the entire project state
    */
    juce::var generateGraph();

    //==============================================================================
    /**
        Generate session graph as formatted JSON string
        Convenience wrapper around generateGraph()
    */
    juce::String generateGraphString(bool prettyPrint = true);

private:
    //==============================================================================
    // Serialization helpers

    juce::var serializeTransport();
    juce::var serializeTracks();
    juce::var serializeTrack(Track* track, int trackIndex);
    juce::var serializePlugins(Track* track);
    juce::var serializeClips(Track* track);
    juce::var serializeClip(Track::Clip* clip, int clipIndex);

    //==============================================================================
    // Member variables

    Engine& engine;
    ProjectState& projectState;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionGraph)
};

} // namespace zenith
