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

#include "Engine.h"
#include "RecordingManager.h"
#include "TransportController.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include "../engine/TrackAutomationSynchronizer.h"
#include "../engine/Track.h"

namespace zenith {

//==============================================================================
// MIDI and Audio Recording
//==============================================================================

void Engine::record() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  DBG("Engine: Record");

  if (!transportController_) {
    DBG("Engine::record() called with null transport controller");
    return;
  }

  if (!recordingManager_) {
    DBG("Engine::record() called with null recording manager");
    return;
  }

  if (!transportController_->isPlaying()) {
    play();
  }

  // Create recordings directory
  juce::File recordingsDir;
  if (projectState_ != nullptr &&
      projectState_->getProjectFile().existsAsFile()) {
    recordingsDir =
        projectState_->getProjectFile().getSiblingFile("Audio Files");
  } else {
    recordingsDir =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("ZenithDAW/Recordings");
  }

  if (!recordingsDir.exists()) {
    recordingsDir.createDirectory();
  }

  // Set recording directory
  recordingManager_->setRecordingDirectory(recordingsDir);

  // Start recording on managed sessions
  recordingManager_->startRecording(transportController_->getPlayheadSamples(),
                                    tracks_);
  DBG("Engine: Recording started (Delegated)");
}

void Engine::stopRecording() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  DBG("Engine: Stop recording");
  TempoMap tempoMap; // Use default tempo map for now
  recordingManager_->stopRecording(tracks_, tempoMap);
}

void Engine::toggleRecording() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (recordingManager_) {
    if (recordingManager_->isRecording()) {
      stopRecording();
    } else {
      record();
    }
  }
}

} // namespace zenith
