/**
 * @file EngineRecording.cpp
 * @brief Recording state management
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "Engine.h"
#include "ProjectState.h"
#include "../engine/RecordingManager.h"
#include "../engine/TransportController.h"
#include "../engine/Track.h"

namespace zenith {

//==============================================================================
// MIDI and Audio Recording
//==============================================================================

void Engine::record() {
  DBG("Engine: Record");

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
  DBG("Engine: Stop recording");
  recordingManager_->stopRecording(tracks_, getTempoMap());
}

void Engine::toggleRecording() {
  if (recordingManager_) {
    if (recordingManager_->isRecording()) {
      stopRecording();
    } else {
      record();
    }
  }
}

} // namespace zenith
