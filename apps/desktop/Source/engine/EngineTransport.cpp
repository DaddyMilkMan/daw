/**
 * @file EngineTransport.cpp
 * @brief Transport controls, playhead, looping, and PDC management
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "Engine.h"
#include "ProjectState.h"
#include "../engine/TransportController.h"
#include "../engine/RecordingManager.h"
#include "../engine/TrackAutomationSynchronizer.h"
#include "../engine/AudioRenderer.h"
#include "../engine/Track.h"

namespace zenith {

//==============================================================================
// Transport Controls
//==============================================================================

void Engine::play() {
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

void Engine::panic() {
  DBG("Engine: PANIC triggered!");
  
  // 1. Stop Transport
  stop();
  
  // 2. Iterate all tracks (message thread is safe)
  for (const auto& track : tracks_) {
    if (track) {
      // Clear any pending MIDI events in the track
      // (Track doesn't expose a method for this yet, assuming implementation needed later)
      
      // Mute temporarily to stop audio output immediately
      // track->setMuted(true); // Maybe too aggressive?
      
      // Allow reverb tails to fade naturally or kill them?
      // Panic usually implies immediate silence.
      // Ideally we would send MIDI CC 123 (All Notes Off) and 120 (All Sound Off)
      // but we need a mechanism to inject MIDI into the track.
      // For now, we will rely on stop() stopping the engine processing primarily.
    }
  }
}

//==============================================================================
// Transport Position & Looping
//==============================================================================

void Engine::setPlayheadSamples(juce::int64 position) {
  if (transportController_) {
    transportController_->setPlayheadSamples(position);
  }
}

void Engine::setLooping(bool shouldLoop) {
  if (transportController_) {
    transportController_->setLooping(shouldLoop);
  }
}

void Engine::setLoopRegion(juce::int64 start, juce::int64 end) {
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
// Plugin Delay Compensation (PDC)
//==============================================================================

int Engine::getTrackLatency(int trackIndex) const {
  if (audioRenderer_) {
    return audioRenderer_->getTrackLatency(trackIndex);
  }
  return 0;
}

int Engine::getMasterLatency() const {
  if (audioRenderer_) {
    return audioRenderer_->getMasterLatency();
  }
  return 0;
}

void Engine::setPDCEnabled(bool enabled) {
  if (audioRenderer_) {
    audioRenderer_->setPDCEnabled(enabled);
  }
}

bool Engine::isPDCEnabled() const {
  return audioRenderer_ ? audioRenderer_->isPDCEnabled() : false;
}

int Engine::getMaxTrackLatency() const {
  return audioRenderer_ ? audioRenderer_->getMaxTrackLatency() : 0;
}

void Engine::recalculatePDC() {
  if (audioRenderer_) {
    // Build raw pointer vector for AudioRenderer
    std::vector<Track*> trackPtrs;
    trackPtrs.reserve(tracks_.size());
    for (const auto& t : tracks_) trackPtrs.push_back(t.get());
    
    audioRenderer_->calculatePDC(trackPtrs);
  }
}

} // namespace zenith
