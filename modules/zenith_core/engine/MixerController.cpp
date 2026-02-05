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

/*
    ==============================================================================
    Original file header:
*/

﻿/*
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
