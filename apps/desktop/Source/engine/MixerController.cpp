/*
  ==============================================================================

    MixerController.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

    MixerController implementation.

  ==============================================================================
*/

#include "MixerController.h"
#include "Engine.h"
#include "Track.h"

namespace zenith {

MixerController::MixerController(Engine& engine) : engine_(engine) {
}

//==============================================================================
// Track Strip Controls
//==============================================================================

void MixerController::setVolume(int trackIndex, float volume) {
    engine_.setTrackVolume(trackIndex, volume);
}

float MixerController::getVolume(int trackIndex) const {
    auto tracks = engine_.getTracksSnapshot();
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size()) && tracks[trackIndex]) {
        return tracks[trackIndex]->getVolume();
    }
    return 1.0f; // Default unity
}

void MixerController::setPan(int trackIndex, float pan) {
    engine_.setTrackPan(trackIndex, pan);
}

float MixerController::getPan(int trackIndex) const {
    auto tracks = engine_.getTracksSnapshot();
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size()) && tracks[trackIndex]) {
        return tracks[trackIndex]->getPan();
    }
    return 0.0f; // Center
}

void MixerController::setMute(int trackIndex, bool muted) {
    engine_.setTrackMute(trackIndex, muted);
}

bool MixerController::isMuted(int trackIndex) const {
    auto tracks = engine_.getTracksSnapshot();
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size()) && tracks[trackIndex]) {
        return tracks[trackIndex]->isMuted();
    }
    return false;
}

void MixerController::setSolo(int trackIndex, bool solo) {
    engine_.setTrackSolo(trackIndex, solo);
}

bool MixerController::isSolo(int trackIndex) const {
    auto tracks = engine_.getTracksSnapshot();
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size()) && tracks[trackIndex]) {
        return tracks[trackIndex]->isSolo();
    }
    return false;
}

void MixerController::setRecArm(int trackIndex, bool armed) {
    engine_.setTrackArmed(trackIndex, armed);
}

bool MixerController::isRecArmed(int trackIndex) const {
    auto tracks = engine_.getTracksSnapshot();
    if (trackIndex >= 0 && trackIndex < static_cast<int>(tracks.size()) && tracks[trackIndex]) {
        return tracks[trackIndex]->isArmed();
    }
    return false;
}

//==============================================================================
// Global Mixer Actions
//==============================================================================

void MixerController::clearAllSolos() {
    // Use stable snapshot for iteration
    auto tracks = engine_.getTracksSnapshot();
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
        if (tracks[i] && tracks[i]->isSolo()) {
            engine_.setTrackSolo(i, false);
        }
    }
}

void MixerController::resetAllPeakMeters() {
    engine_.resetPeakMeters();
}



} // namespace zenith
