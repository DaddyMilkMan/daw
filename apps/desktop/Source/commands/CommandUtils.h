#pragma once
#include <juce_core/juce_core.h>
#include "../engine/Track.h"
#include "../engine/Clip.h"
#include "Engine.h"
#include "ProjectState.h"

namespace zenith {

inline juce::var createSuccessResponse(const juce::var& result = juce::var()) {
    auto* response = new juce::DynamicObject();
    response->setProperty("success", true);
    if (!result.isVoid()) response->setProperty("result", result);
    return juce::var(response);
}

inline juce::var createErrorResponse(const juce::String& errorMessage) {
    auto* response = new juce::DynamicObject();
    response->setProperty("success", false);
    response->setProperty("error", errorMessage);
    return juce::var(response);
}

inline Track* findTrackById(Engine& engine, const juce::String& trackId) {
    for (const auto& track : engine.tracks()) {
        if (track->getTrackId() == trackId) {
            return track.get();
        }
    }
    return nullptr;
}

inline Track::Clip* findClipById(ProjectState& state, Track* track, const juce::String& clipId) {
    if (track == nullptr) return nullptr;
    
    // Get track ID to query ProjectState
    juce::String trackId = track->getTrackId();
    auto trackTree = state.getTrack(trackId);
    if (!trackTree.isValid()) return nullptr;
    
    // Find index of clip with this ID
    auto clipsList = trackTree.getChildWithName(ProjectState::ID_CLIPS);
    int index = 0;
    for (const auto& child : clipsList) {
        if (child.getProperty(ProjectState::PROP_ID).toString() == clipId) {
            return track->getClip(index);
        }
        index++;
    }
    return nullptr;
}

} // namespace zenith
