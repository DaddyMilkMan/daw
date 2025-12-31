/**
 * @file EngineMIDI.cpp
 * @brief MIDI input handling and panic routine
 * @note This is a modular component of Engine - declarations remain in Engine.h
 */

#include "Engine.h"
#include "../engine/RecordingManager.h"
#include "../engine/TransportController.h"
#include "../engine/Track.h"
#include "../engine/Midi2DiscoveryService.h"

namespace zenith {

//==============================================================================
// MIDI Input Handling
//==============================================================================

void Engine::enableMidiInput() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
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
  } else {
    // Trigger MIDI 2.0 Discovery
    if (midi2DiscoveryService_) {
      midi2DiscoveryService_->startDiscovery();
    }
  }
}

void Engine::disableMidiInput() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
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
    auto *snapshot = activeSnapshot_.load();
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

void Engine::panic() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  DBG("Engine: PANIC triggered!");

  // 1. Stop Transport
  stop();

  // 2. Send All Notes Off (CC 123) and All Sound Off (CC 120) to all MIDI tracks
  for (const auto &track : tracks_) {
    if (track) {
      auto trackType = track->getType();
      
      // Only send MIDI panic to tracks that process MIDI
      if (trackType == Track::Type::MIDI || trackType == Track::Type::Instrument) {
        // CC 120 = All Sound Off (immediate silence, kills reverb tails too)
        track->injectLiveMidiMessage(juce::MidiMessage::controllerEvent(1, 120, 0));
        
        // CC 123 = All Notes Off (releases all held notes)
        track->injectLiveMidiMessage(juce::MidiMessage::controllerEvent(1, 123, 0));
        
        // Also send Note Off for all possible notes (0-127) as belt-and-suspenders
        for (int note = 0; note < 128; ++note) {
          track->injectLiveMidiMessage(juce::MidiMessage::noteOff(1, note));
        }
      }
    }
  }
  
  DBG("Engine: PANIC complete - all notes silenced");
}

} // namespace zenith
