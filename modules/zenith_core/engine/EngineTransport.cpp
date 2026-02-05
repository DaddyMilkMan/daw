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

//     File: EngineTransport.cpp
//     Brief: Transport controls, playhead, and looping
//     Note: This is a modular component of Engine - declarations remain in Engine.h


#include "../engine/RecordingManager.h"
#include "../engine/TrackAutomationSynchronizer.h"
#include "../engine/AudioRenderer.h"
#include "../engine/Track.h"

namespace zenith {

//==============================================================================
// Transport Controls
//==============================================================================

void Engine::play() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  DBG("Engine: Play");

  // Handle loop region - reset to loop start if past loop end
  // If playhead is at or past loop end, reset to loop start or 0
  const juce::int64 loopEnd = transportController_->getLoopEndSamples();
  const juce::int64 loopStart = transportController_->getLoopStartSamples();
  const juce::int64 currentPos = transportController_->getPlayheadSamples();

  if (loopEnd > 0 && currentPos >= loopEnd) {
    transportController_->setPlayheadSamples(loopStart);
  }

  // Enable test tone for Phase 0 fallback (when no tracks)
  enableTestTone_.store(true);

  transportController_->play();

  // Start automation synchronizer for parameter recording/playback
  if (automationSynchronizer) {
    automationSynchronizer->start(60); // 60 Hz update rate
    DBG("Engine: Started automation synchronizer");
  }
}

void Engine::stop() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  DBG("Engine: Stop");

  if (transportController_) {
    transportController_->stop();
  }

  // Stop recording and bake recordings into clips
  if (isRecording()) {
    stopRecording();
  }

  enableTestTone_.store(false);

  // Stop automation synchronizer
  if (automationSynchronizer) {
    automationSynchronizer->stop();
    DBG("Engine: Stopped automation synchronizer");
  }
}

bool Engine::isPlaying() const {
  return transportController_ ? transportController_->isPlaying() : false;
}

bool Engine::isRecording() const {
  return recordingManager_ ? recordingManager_->isRecording() : false;
}

juce::int64 Engine::getPlayheadSamples() const {
  return transportController_ ? transportController_->getPlayheadSamples() : 0;
}

juce::int64 Engine::getPlaybackPosition() const {
  return transportController_ ? transportController_->getPlayheadSamples() : 0;
}

double Engine::getPlaybackPositionBeats() const {
  if (transportController_) {
    return transportController_->getPlayheadBeats();
  }
  return 0.0;
}

//==============================================================================
// Transport Position & Looping
//==============================================================================

void Engine::setPlayheadSamples(juce::int64 position) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (transportController_) {
    transportController_->setPlayheadSamples(position);
  }
}

void Engine::setLooping(bool shouldLoop) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (transportController_) {
    transportController_->setLooping(shouldLoop);
  }
}

void Engine::setLoopRegion(juce::int64 start, juce::int64 end) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (transportController_) {
    transportController_->setLoopRegionSamples(start, end);
  }
}

bool Engine::isLooping() const {
  return transportController_ ? transportController_->isLooping() : false;
}

juce::int64 Engine::getLoopStart() const {
  return transportController_ ? transportController_->getLoopStartSamples() : 0;
}

juce::int64 Engine::getLoopEnd() const {
  return transportController_ ? transportController_->getLoopEndSamples() : 0;
}

//==============================================================================
// Metronome
//==============================================================================

void Engine::toggleMetronome() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (transportController_) {
    bool newState = !transportController_->isMetronomeEnabled();
    transportController_->setMetronomeEnabled(newState);
    if (metronome_) {
      metronome_->setEnabled(newState);
    }
  }
}

bool Engine::isMetronomeEnabled() const {
  return transportController_ ? transportController_->isMetronomeEnabled()
                              : false;
}

void Engine::setMetronomeLevel(float level) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  if (transportController_) {
    transportController_->setMetronomeLevel(level);
    if (metronome_) {
      metronome_->setLevel(level);
    }
  }
}

} // namespace zenith
