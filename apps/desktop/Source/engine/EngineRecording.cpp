/**
 * @file EngineRecording.cpp
 * @brief Recording state management and MIDI input handling
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "../engine/RecordingManager.h"
#include "../engine/Track.h"
#include "../engine/TransportController.h"
#include "Engine.h"
#include "ProjectState.h"

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
  recordingManager_->stopRecording(tracks_);
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

void Engine::setSidechainSource(int destTrackIndex, int pluginIndex,
                                int sourceTrackIndex) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (destTrackIndex < 0 || destTrackIndex >= tracks_.size())
    return;
  if (sourceTrackIndex < 0 || sourceTrackIndex >= tracks_.size())
    return;

  auto &destTrack = tracks_[destTrackIndex];
  auto &sourceTrack = tracks_[sourceTrackIndex];

  DBG("Engine: Routing Sidechain: " << sourceTrack->getName() << " -> "
                                    << destTrack->getName() << " (Plugin "
                                    << pluginIndex << ")");

  // Connect in routing graph (Stub Logic for Phase 2)
  if (destTrack && sourceTrack) {
    // routingGraph_.connect(sourceTrack->getTrackId(),
    // destTrack->getTrackId(), 1.0f); Note: Real implementation needs to target
    // specific plugin inputs, not just track mix.
  }
}

//==============================================================================
// Phase 2A: MIDI Input Handling
//==============================================================================

void Engine::enableMidiInput() {
  DBG("Engine: Enabling MIDI input...");

  // Get list of available MIDI input devices
  auto midiInputs = juce::MidiInput::getAvailableDevices();

  if (midiInputs.isEmpty()) {
    DBG("Engine: No MIDI input devices available");
    return;
  }

  // Open all available MIDI input devices (Omni mode)
  for (const auto &input : midiInputs) {
    DBG("Engine: Opening MIDI input: " + input.name);

    auto newInput = juce::MidiInput::openDevice(input.identifier, this);
    if (newInput != nullptr) {
      newInput->start();
      midiInputs_.push_back(std::move(newInput));
      DBG("Engine: MIDI input started: " + input.name);
    } else {
      DBG("Engine: Failed to open MIDI input: " + input.name);
    }
  }

  if (midiInputs_.empty()) {
    DBG("Engine: No MIDI inputs could be opened");
  }
}

void Engine::disableMidiInput() {
  DBG("Engine: Disabling MIDI inputs...");

  for (auto &input : midiInputs_) {
    if (input)
      input->stop();
  }
  midiInputs_.clear();
  DBG("Engine: MIDI inputs stopped");
}

void Engine::handleIncomingMidiMessage(juce::MidiInput *source,
                                       const juce::MidiMessage &message) {
  juce::ignoreUnused(source);

// This runs on MIDI input thread (NOT audio thread or message thread)
// Buffer the message for processing in audio callback

// Debug log for MIDI activity
#if JUCE_DEBUG
  if (message.isNoteOn()) {
    DBG("MIDI In: Note On " + juce::String(message.getNoteNumber()) + " Vel " +
        juce::String(message.getVelocity()));
  }
#endif

  // Add message to FIFO for playback (lock-free)
  midiFifo_.push(message);

  // MIDI Recording: Capture MIDI for ALL armed MIDI/Instrument tracks
  // This enables multi-track MIDI recording from a single source
  if (recordingManager_ && recordingManager_->isRecording()) {
    juce::int64 currentPosition =
        transportController_ ? transportController_->getPlayheadSamples() : 0;

    // Route to ALL armed MIDI/Instrument tracks for multi-track recording
    // Use snapshot for RT-safe access to tracks
    auto snapshot = activeSnapshot_.load(std::memory_order_acquire);
    if (snapshot) {
      for (size_t i = 0; i < snapshot->tracks.size(); ++i) {
        auto *track = snapshot->tracks[i];
        if (track && track->isArmed()) {
          if (track->getType() == Track::Type::MIDI ||
              track->getType() == Track::Type::Instrument) {
            // Capture MIDI for this track (RT-safe, uses lock-free FIFO)
            recordingManager_->captureMidi(message, currentPosition,
                                           static_cast<int>(i));
          }
        }
      }
    }
  }
}

} // namespace zenith
