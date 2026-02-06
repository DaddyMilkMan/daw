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

// TempoMapSynchronizer.h

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include "ProjectState.h"
#include "TempoMap.h"
#include <memory>

// Forward declarations
class Engine;

//==============================================================================
namespace zenith {

/**
 * @class TempoMapSynchronizer
 // Brief: Syncs tempo map from ProjectState to Engine
 *
 * This class listens to ProjectState changes and updates the Engine's TempoMap.
 * Unlike TrackAutomationSynchronizer, this doesn't need a timer because tempo
 * changes are instantaneous (not sampled over time).
 */
class TempoMapSynchronizer : private juce::ValueTree::Listener
{
public:
    //==========================================================================
    /**
     // Brief: Constructor
     * @param projectState Reference to project state (must outlive this object)
     * @param tempoMap Reference to engine's tempo map (must outlive this object)
     */
    TempoMapSynchronizer(zenith::ProjectState& projectState, zenith::TempoMap& tempoMap);

    /**
     // Brief: Destructor
     */
    ~TempoMapSynchronizer() override;

    //==========================================================================
    /**
     // Brief: Start synchronization
     */
    void start();

    /**
     // Brief: Stop synchronization
     */
    void stop();

    /**
     // Brief: Force immediate update from current ProjectState
     // Note: Call this after loading a project or when ProjectState changes externally
     */
    void forceUpdate();

private:
    //==========================================================================
    // ValueTree::Listener (MESSAGE THREAD)
    //==========================================================================

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) [[maybe_unused]] override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) [[maybe_unused]] override;
    void valueTreeParentChanged(juce::ValueTree& tree) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     // Brief: Update tempo map from current ProjectState
     // Note: MESSAGE THREAD ONLY
     */
    void updateTempoMap();

    /**
     // Brief: Check if a ValueTree node is part of the tempo map
     */
    bool isTempoMapNode(const juce::ValueTree& tree) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    zenith::ProjectState& projectState;
    zenith::TempoMap& tempoMap;

    bool isActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoMapSynchronizer)
};

} // namespace zenith

