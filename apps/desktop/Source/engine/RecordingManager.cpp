/*
  ==============================================================================

    RecordingManager.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Recording manager implementation with clip creation.

  ==============================================================================
*/

#include "RecordingManager.h"
#include "ProjectState.h"
#include "AudioRecorder.h"
#include "Track.h"
#include "TempoMap.h"

namespace zenith {

//==============================================================================
RecordingManager::RecordingManager() {
  // Initialize MIDI fifo buffer
  midiFifoData_.resize(constants::kMidiRecordFifoSize);

  // Create audio recorder
  audioRecorder_ = std::make_unique<AudioRecorder>();
}

RecordingManager::~RecordingManager() {
  if (isRecording_.load()) {
    // Force stop - don't finalize properly
    isRecording_.store(false);
    
    // Bug 16: Ensure thread-safe access when clearing sessions
    const juce::ScopedLock sl(sessionLock_);
    midiSessions_.clear();
  }

  // AudioRecorder destructor will handle cleanup
  audioRecorder_.reset();
}

//==============================================================================
void RecordingManager::prepare(double sampleRate) {
  sampleRate_ = sampleRate;

  if (audioRecorder_) {
    audioRecorder_->prepare(sampleRate);
  }
}

//==============================================================================
void RecordingManager::prepareRecordingForTrack(
    const Track &track, int trackIndex, const juce::File &recordingsDir) {
  juce::ignoreUnused(track, trackIndex);
  // Optimization: Pre-allocate resources or create directory
  // For now we just ensure the directory exists to avoid glitches during start
  if (!recordingsDir.exists()) {
    if (!recordingsDir.createDirectory()) {
      DBG("RecordingManager: Warning - failed to create recordings directory: " +
          recordingsDir.getFullPathName());
    }
  }
}

//==============================================================================
void RecordingManager::setRecordingDirectory(const juce::File &recordDir) {
  // Thread safety check - return if not on message thread
  if (!juce::MessageManager::getInstanceWithoutCreating() ||
      !juce::MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread()) {
    DBG("RecordingManager: Method called from wrong thread - ignoring");
    return;
  }

  // Store recording directory for later use
  recordingDirectory_ = recordDir;
  if (!recordingDirectory_.exists()) {
    if (!recordingDirectory_.createDirectory()) {
      DBG("RecordingManager: Error - failed to create recording directory: " +
          recordDir.getFullPathName());
    }
  }

  DBG("RecordingManager: Recording directory set to: " +
      recordDir.getFullPathName());
}

//==============================================================================
void RecordingManager::startRecording(
    juce::int64 startPosition,
    const std::vector<std::shared_ptr<Track>> &tracks) {
  // Thread safety check - return if not on message thread
  if (!juce::MessageManager::getInstanceWithoutCreating() ||
      !juce::MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread()) {
    DBG("RecordingManager: Method called from wrong thread - ignoring");
    return;
  }

  if (isRecording_.load()) {
    DBG("RecordingManager: Already recording");
    return;
  }

  // Store start position for clip creation
  recordingStartPosition_ = startPosition;

  // Ensure we have a valid device manager
  if (deviceManager_ == nullptr) {
    DBG("RecordingManager: No device manager set, cannot start recording");
    return;
  }

  // Ensure recording directory exists
  if (!recordingDirectory_.exists()) {
    recordingDirectory_ =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("ZenithDAW/Recordings");
    recordingDirectory_.createDirectory();
  }

  // Start audio recording via AudioRecorder
  if (audioRecorder_) {
    audioRecorder_->startRecording(tracks, *deviceManager_, startPosition,
                                   recordingDirectory_);
  }

  // Create MIDI sessions for armed MIDI/Instrument tracks
  {
    const juce::ScopedLock sl(sessionLock_);
    midiSessions_.clear();

    // Add defensive null check for tracks vector
    if (tracks.empty()) {
      DBG("RecordingManager: No tracks provided, cannot start recording");
      return;
    }

    for (size_t i = 0; i < tracks.size(); ++i) {
      auto &track = tracks[i];
      
      // Check for null track pointer
      if (!track) {
        DBG("RecordingManager: Null track at index " + juce::String(i));
        continue;
      }
      
      if (track->isArmed()) {
        Track::Type trackType = Track::Type::Audio;
        try {
          trackType = track->getType();
        } catch (...) {
          DBG("RecordingManager: Invalid track type for index " + juce::String(i));
          continue;
        }
        
        if (trackType == Track::Type::MIDI ||
            trackType == Track::Type::Instrument) {
          MidiRecordingSession midiSession;
          midiSession.trackId = track->getTrackId();
          midiSession.trackIndex = static_cast<int>(i);
          midiSession.startSamplePosition = startPosition;
          midiSession.isActive = true;
          midiSession.sequence.clear();
          midiSessions_.push_back(std::move(midiSession));

          juce::String trackName = "Unknown";
          try {
            trackName = track->getName();
          } catch (...) {
            trackName = "Invalid Track " + juce::String(i);
          }
          
          DBG("RecordingManager: Created MIDI session for track " +
              juce::String(i) + " (" + trackName + ")");
        }
      }
    }
  }

  isRecording_.store(true);
  DBG("RecordingManager: Recording started at sample " +
      juce::String(startPosition));
}

//==============================================================================
void RecordingManager::stopRecording(
    const std::vector<std::shared_ptr<Track>> &tracks,
    const TempoMap& tempoMap) {
  // Thread safety check - return if not on message thread
  if (!juce::MessageManager::getInstanceWithoutCreating() ||
      !juce::MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread()) {
    DBG("RecordingManager: Method called from wrong thread - ignoring");
    return;
  }

  if (!isRecording_.load()) {
    return;
  }

  // Stop accepting new input
  isRecording_.store(false);

  // Drain MIDI fifo first
  drainMidiFifo();

  // Stop AudioRecorder and finalize clips with results
  if (audioRecorder_) {
    audioRecorder_->stopRecording([this, &tracks, &tempoMap](std::vector<RecordingResult> results) {
      // Finalize recordings and create clips with the actual results
      finalizeRecordings(results, tracks, tempoMap);
      
      // Clear MIDI sessions
      {
        const juce::ScopedLock sl(sessionLock_);
        midiSessions_.clear();
      }

      DBG("RecordingManager: Recording stopped");
    });
  } else {
    // No audio recorder, just finalize MIDI
    finalizeRecordings({}, tracks, tempoMap);
    
    // Clear MIDI sessions
    {
      const juce::ScopedLock sl(sessionLock_);
      midiSessions_.clear();
    }

    DBG("RecordingManager: Recording stopped (no audio recorder)");
  }
}

void RecordingManager::discardCurrentRecording() {
  // Thread safety check - return if not on message thread
  if (!juce::MessageManager::getInstanceWithoutCreating() ||
      !juce::MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread()) {
    DBG("RecordingManager: Method called from wrong thread - ignoring");
    return;
  }

  if (!isRecording_.load()) {
    return;
  }

  // Stop accepting new input
  isRecording_.store(false);

  // Stop AudioRecorder and delete generated files
  if (audioRecorder_) {
    audioRecorder_->stopRecording([](std::vector<RecordingResult> results) {
      for (const auto &result : results) {
        if (result.file.exists()) {
          result.file.deleteFile();
          DBG("RecordingManager: Deleted discarded recording file: " + result.file.getFileName());
        }
      }
    });
  }

  // Clear MIDI sessions
  {
    const juce::ScopedLock sl(sessionLock_);
    midiSessions_.clear();
  }

  // Drain MIDI fifo to clear it (ignore data)
  int start1, size1, start2, size2;
  const int numReady = midiFifoIndex_.getNumReady();
  midiFifoIndex_.prepareToRead(numReady, start1, size1, start2, size2);
  midiFifoIndex_.finishedRead(size1 + size2);

  DBG("RecordingManager: Recording discarded");
}

//==============================================================================
void RecordingManager::captureAudio(
    const float *const *inputData, int numInputChannels, int numSamples,
    const std::vector<std::shared_ptr<Track>> &tracks) {
  // Delegate to AudioRecorder (RT-safe)
  if (audioRecorder_ && isRecording_.load()) {
    audioRecorder_->write(inputData, numInputChannels, numSamples, tracks);
  }
}

//==============================================================================
void RecordingManager::captureMidi(const juce::MidiMessage &message,
                                   juce::int64 samplePosition, int trackIndex) {
  if (!isRecording_.load()) {
    return;
  }

  // Lock-free write to fifo (RT-safe)
  int start1, size1, start2, size2;
  midiFifoIndex_.prepareToWrite(1, start1, size1, start2, size2);

  if (size1 > 0) {
    midiFifoData_[start1] = {message, samplePosition, trackIndex};
    midiFifoIndex_.finishedWrite(1);
  } else {
    // Bug 17: FIFO overflow - track dropped messages
    // RT-Safety: Just increment counter, do not log (DBG is not RT-safe)
    droppedMidiMessages_.fetch_add(1, std::memory_order_relaxed);
  }
}

//==============================================================================
void RecordingManager::drainMidiFifo() {
  // Thread safety check - return if not on message thread
  if (!juce::MessageManager::getInstanceWithoutCreating() ||
      !juce::MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread()) {
    DBG("RecordingManager: Method called from wrong thread - ignoring");
    return;
  }

  int start1, size1, start2, size2;
  const int numReady = midiFifoIndex_.getNumReady();
  midiFifoIndex_.prepareToRead(numReady, start1, size1, start2, size2);

  // Process both blocks under a single lock for efficiency
  {
      // Acquire lock ONCE before the loops to avoid overhead
      const juce::ScopedLock sl(sessionLock_);

      // Process first block
      for (int i = 0; i < size1; ++i) {
        const auto &entry = midiFifoData_[start1 + i];

        // Find matching session
        for (auto &session : midiSessions_) {
          if (session.trackIndex == entry.trackIndex && session.isActive) {
            // Calculate time relative to recording start
            double timeSeconds = static_cast<double>(entry.samplePosition -
                                                     session.startSamplePosition) /
                                 sampleRate_;

            // Only add positive time events
            if (timeSeconds >= 0.0) {
              session.sequence.addEvent(entry.message, timeSeconds);
            }
            break;
          }
        }
      }

      // Process second block (wrap-around)
      for (int i = 0; i < size2; ++i) {
        const auto &entry = midiFifoData_[start2 + i];
        
        // Find matching session
        for (auto &session : midiSessions_) {
          if (session.trackIndex == entry.trackIndex && session.isActive) {
            double timeSeconds = static_cast<double>(entry.samplePosition -
                                                     session.startSamplePosition) /
                                 sampleRate_;
            if (timeSeconds >= 0.0) {
              session.sequence.addEvent(entry.message, timeSeconds);
            }
            break;
          }
        }
      }
  }
  midiFifoIndex_.finishedRead(size1 + size2);
}

//==============================================================================
void RecordingManager::finalizeRecordings(
    const std::vector<RecordingResult>& results,
    const std::vector<std::shared_ptr<Track>> &tracks,
    const TempoMap& tempoMap) {

  if (projectState_ == nullptr) {
    DBG("RecordingManager: No project state, cannot create clips");
    return;
  }

  // Finalize audio recordings and create clips
  // Note: The 'results' parameter now contains the recording results passed from stopRecording
  for (const auto &result : results) {
    juce::String trackId = result.trackId;

    // Fallback to index if ID missing (legacy safety)
    if (trackId.isEmpty() && result.trackIndex >= 0 &&
        result.trackIndex < static_cast<int>(tracks.size()) &&
        tracks[result.trackIndex]) {
      trackId = tracks[result.trackIndex]->getTrackId();
    }

    if (trackId.isNotEmpty() && result.samplesRecorded > 0) {
      createAudioClip(result.file, trackId, result.startSamplePosition,
                      result.samplesRecorded, tempoMap);
    }
  }

  // Finalize MIDI recordings
  {
    const juce::ScopedLock sl(sessionLock_);

    for (auto &session : midiSessions_) {
      if (!session.isActive || session.sequence.getNumEvents() == 0) {
        continue;
      }

      // Sort events by time
      session.sequence.sort();

      // Match note on/off pairs
      session.sequence.updateMatchedPairs();

      createMidiClip(session.sequence, session.trackId,
                     session.startSamplePosition, tempoMap);

      DBG("RecordingManager: Created MIDI clip with " +
          juce::String(session.sequence.getNumEvents()) + " events for track " +
          session.trackId);
    }
  }
}

//==============================================================================
void RecordingManager::createAudioClip(const juce::File &audioFile,
                                       const juce::String &trackId,
                                       juce::int64 startSamplePosition,
                                       juce::int64 lengthSamples,
                                       const TempoMap& tempoMap) {
  if (projectState_ == nullptr || trackId.isEmpty()) {
    return;
  }

  // Convert samples to beats for ProjectState using sampleRate_ member
  double tempo = projectState_->getTempo();
  if (tempo <= 0.0) {
      DBG("RecordingManager: Invalid tempo, defaulting to 120 BPM");
      tempo = 120.0;
  }
  const double samplesPerBeat = sampleRate_ * 60.0 / tempo;

  const double startBeats =
      static_cast<double>(startSamplePosition) / samplesPerBeat;
  const double lengthBeats =
      static_cast<double>(lengthSamples) / samplesPerBeat;

  // Create clip via ProjectState API
  juce::String clipName = audioFile.getFileNameWithoutExtension();
  juce::String clipId = projectState_->createClip(
      trackId, "audio",
      static_cast<juce::int64>(startBeats *
                               samplesPerBeat), // Convert back for internal use
      static_cast<juce::int64>(lengthBeats * samplesPerBeat), clipName,
      "Record audio");

  if (clipId.isEmpty()) {
    DBG("RecordingManager: Failed to create clip via ProjectState");
    return;
  }

  // Set the audio file path on the clip
  if (!projectState_->setClipAudioFile(trackId, clipId, audioFile,
                                       "Set recording file")) {
    DBG("RecordingManager: Failed to set audio file on clip");
  }

  DBG("RecordingManager: Created audio clip '" + clipName + "' at beat " +
      juce::String(startBeats, 2) + " with length " +
      juce::String(lengthBeats, 2) + " beats");
}

//==============================================================================
void RecordingManager::createMidiClip(const juce::MidiMessageSequence &sequence,
                                      const juce::String &trackId,
                                      juce::int64 startSamplePosition,
                                      const TempoMap& tempoMap) {
  if (projectState_ == nullptr || trackId.isEmpty()) {
    return;
  }

  // Calculate clip length from MIDI events
  double endTimeSeconds = 0.0;
  for (int i = 0; i < sequence.getNumEvents(); ++i) {
    const auto *event = sequence.getEventPointer(i);
    if (event) {
      double eventEnd = event->message.getTimeStamp();
      if (event->noteOffObject) {
        eventEnd = event->noteOffObject->message.getTimeStamp();
      }
      endTimeSeconds = std::max(endTimeSeconds, eventEnd);
    }
  }

  // Add small padding at end (1 beat worth)
  // BUG FIX #9: Validate tempo to prevent division by zero
  double tempo = projectState_->getTempo();
  if (tempo <= 0.0) {
    DBG("RecordingManager: Invalid tempo, defaulting to 120 BPM");
    tempo = 120.0;
  }
  const double samplesPerBeat = sampleRate_ * 60.0 / tempo;
  const double paddingSeconds = 60.0 / tempo; // 1 beat
  endTimeSeconds += paddingSeconds;

  const juce::int64 lengthSamples =
      static_cast<juce::int64>(endTimeSeconds * sampleRate_);

  // Convert to beats
  const double startBeats =
      static_cast<double>(startSamplePosition) / samplesPerBeat;
  const double lengthBeats =
      static_cast<double>(lengthSamples) / samplesPerBeat;

  // Create MIDI clip
  juce::String clipName =
      "MIDI Recording " + juce::Time::getCurrentTime().formatted("%H:%M:%S");

  juce::String clipId = projectState_->createClip(
      trackId, "midi", static_cast<juce::int64>(startBeats * samplesPerBeat),
      static_cast<juce::int64>(lengthBeats * samplesPerBeat), clipName,
      "Record MIDI");

  if (clipId.isEmpty()) {
    DBG("RecordingManager: Failed to create MIDI clip");
    return;
  }

  // Add MIDI notes to the clip
  std::vector<ProjectState::MidiNoteSpec> notes;
  notes.reserve(static_cast<size_t>(sequence.getNumEvents() /
                                    2)); // Approximate note count

  for (int i = 0; i < sequence.getNumEvents(); ++i) {
    const auto *event = sequence.getEventPointer(i);
    if (event && event->message.isNoteOn() && event->noteOffObject) {
      ProjectState::MidiNoteSpec note;

      // Convert time from seconds to beats (relative to clip start)
      note.startBeats = event->message.getTimeStamp() * tempo / 60.0;
      const double endBeats =
          event->noteOffObject->message.getTimeStamp() * tempo / 60.0;
      note.lengthBeats = endBeats - note.startBeats;

      note.pitch = event->message.getNoteNumber();
      note.velocity = event->message.getVelocity();
      note.muted = false;
      note.probability = 1.0f;

      notes.push_back(note);
    }
  }

  if (!notes.empty()) {
    projectState_->addNotes(clipId, notes, "Add recorded notes");
    DBG("RecordingManager: Added " + juce::String(notes.size()) +
        " notes to MIDI clip");
  }
}

} // namespace zenith
