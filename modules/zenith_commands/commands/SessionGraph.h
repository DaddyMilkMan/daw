/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

