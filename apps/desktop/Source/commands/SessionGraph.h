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

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../engine/Track.h"

namespace zenith {

// Forward declarations
class Engine;
class ProjectState;

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
    juce::var serializeTrack(const Track* track, int trackIndex);
    juce::var serializePlugins(const Track* track);
    juce::var serializeClips(const Track* track);
    juce::var serializeClip(const Clip* clip, int clipIndex);

    //==============================================================================
    // Member variables

    Engine& engine;
    ProjectState& projectState;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionGraph)
};

} // namespace zenith

