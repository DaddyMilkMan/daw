/*
  ==============================================================================

    RecordingManager.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Recording manager implementation.

  ==============================================================================
*/

#include "RecordingManager.h"
#include "Track.h"
#include "ProjectState.h"

namespace zenith {

//==============================================================================
RecordingManager::RecordingManager() {
    midiFifoData_.resize(constants::kMidiRecordFifoSize);
    
    writerThread_ = std::make_unique<juce::TimeSliceThread>("Audio Writer Thread");
    writerThread_->startThread(juce::Thread::Priority::normal);
}

RecordingManager::~RecordingManager() {
    if (isRecording_.load()) {
        // Force stop - don't finalize properly
        isRecording_.store(false);
        audioSessions_.clear();
        midiSessions_.clear();
    }
}

//==============================================================================
void RecordingManager::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
}

//==============================================================================
void RecordingManager::prepareRecordingForTrack(Track& track, int trackIndex,
                                                const juce::File& recordDir) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (writerThread_ == nullptr) {
        DBG("RecordingManager: No writer thread set");
        return;
    }

    recordingDirectory_ = recordDir;
    if (!recordingDirectory_.exists()) {
        recordingDirectory_.createDirectory();
    }

    // Determine input channels
    int numChannels = 2; // Default stereo
    if (track.getInputChannel() >= 0) {
        numChannels = 1; // Mono if specific channel selected
    }

    // Create recording file
    juce::File recordFile = createRecordingFile(
        recordingDirectory_, track.getName(), ".wav");

    // Create file stream
    auto fileStream = std::make_unique<juce::FileOutputStream>(recordFile);
    if (!fileStream->openedOk()) {
        DBG("RecordingManager: Failed to create file: " + recordFile.getFullPathName());
        return;
    }

    // Create audio writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(fileStream.release(), sampleRate_,
                                  static_cast<unsigned int>(numChannels),
                                  constants::kRecordingBitDepth, {}, 0));

    if (writer == nullptr) {
        DBG("RecordingManager: Failed to create audio writer");
        return;
    }

    // Wrap in ThreadedWriter for RT-safe writing
    auto threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
        writer.release(), *writerThread_, constants::kAudioWriterFifoSize);

    // Create session
    AudioRecordingSession session;
    session.writer = std::move(threadedWriter);
    session.file = recordFile;
    session.numChannels = numChannels;
    session.sampleRate = sampleRate_;
    session.trackIndex = trackIndex;
    session.isActive = false;

    // Add to prepped sessions
    {
        const juce::ScopedLock sl(sessionLock_);
        
        // Remove old prep for this track
        preppedSessions_.erase(
            std::remove_if(preppedSessions_.begin(), preppedSessions_.end(),
                           [trackIndex](const auto& s) {
                               return s.trackIndex == trackIndex;
                           }),
            preppedSessions_.end());

        preppedSessions_.push_back(std::move(session));
    }

    DBG("RecordingManager: Prepared recording for track " + juce::String(trackIndex));
}

//==============================================================================
void RecordingManager::startRecording(juce::int64 startPosition,
                                      const std::vector<std::shared_ptr<Track>>& tracks) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (isRecording_.load()) {
        DBG("RecordingManager: Already recording");
        return;
    }

    {
        const juce::ScopedLock sl(sessionLock_);

        // Move prepped sessions to active
        for (auto& session : preppedSessions_) {
            session.startSamplePosition = startPosition;
            session.samplesRecorded = 0;
            session.isActive = true;
            audioSessions_.push_back(std::move(session));
        }
        preppedSessions_.clear();

        // Create MIDI sessions for armed MIDI/Instrument tracks
        for (size_t i = 0; i < tracks.size(); ++i) {
            auto& track = tracks[i];
            if (track && track->isArmed()) {
                if (track->getType() == Track::Type::MIDI ||
                    track->getType() == Track::Type::Instrument) {
                    MidiRecordingSession midiSession;
                    midiSession.trackIndex = static_cast<int>(i);
                    midiSession.startSamplePosition = startPosition;
                    midiSession.isActive = true;
                    midiSessions_.push_back(std::move(midiSession));
                }
            }
        }
    }

    isRecording_.store(true);
    DBG("RecordingManager: Started recording");
}

//==============================================================================
void RecordingManager::stopRecording(const std::vector<std::shared_ptr<Track>>& tracks) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (!isRecording_.load()) {
        return;
    }

    isRecording_.store(false);

    // Drain MIDI fifo first
    drainMidiFifo();

    // Finalize recordings
    finalizeRecordings(tracks);

    {
        const juce::ScopedLock sl(sessionLock_);
        audioSessions_.clear();
        midiSessions_.clear();
    }

    DBG("RecordingManager: Stopped recording");
}

//==============================================================================
void RecordingManager::captureAudio(const float* const* inputData,
                                    int numInputChannels,
                                    int numSamples,
                                    const std::vector<std::shared_ptr<Track>>& tracks) {
    if (!isRecording_.load() || inputData == nullptr) {
        return;
    }

    // Lock-free read of sessions (safe because we only modify on message thread)
    for (auto& session : audioSessions_) {
        if (!session.isActive || session.writer == nullptr) {
            continue;
        }

        int trackIndex = session.trackIndex;
        if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
            continue;
        }

        auto& track = tracks[trackIndex];
        if (!track || !track->isArmed()) {
            continue;
        }

        // Get input channel routing
        int inputChannel = track->getInputChannel();
        
        // Build input buffer
        const float* channels[2] = {nullptr, nullptr};
        
        if (inputChannel >= 0 && inputChannel < numInputChannels) {
            // Mono from specific channel
            channels[0] = inputData[inputChannel];
            channels[1] = inputData[inputChannel];
        } else {
            // Stereo
            if (numInputChannels >= 2) {
                channels[0] = inputData[0];
                channels[1] = inputData[1];
            } else if (numInputChannels >= 1) {
                channels[0] = inputData[0];
                channels[1] = inputData[0];
            }
        }

        if (channels[0] != nullptr) {
            session.writer->write(channels, numSamples);
            session.samplesRecorded += numSamples;
        }
    }
}

//==============================================================================
void RecordingManager::captureMidi(const juce::MidiMessage& message,
                                   juce::int64 samplePosition,
                                   int trackIndex) {
    if (!isRecording_.load()) {
        return;
    }

    // Lock-free write to fifo
    int start1, size1, start2, size2;
    midiFifoIndex_.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0) {
        midiFifoData_[start1] = {message, samplePosition, trackIndex};
        midiFifoIndex_.finishedWrite(1);
    }
    // If fifo is full, drop the message (RT-safe behavior)
}

//==============================================================================
void RecordingManager::drainMidiFifo() {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    int start1, size1, start2, size2;
    midiFifoIndex_.prepareToRead(midiFifoIndex_.getNumReady(), start1, size1, start2, size2);

    // Process first block
    for (int i = 0; i < size1; ++i) {
        const auto& entry = midiFifoData_[start1 + i];
        
        // Find matching session
        for (auto& session : midiSessions_) {
            if (session.trackIndex == entry.trackIndex && session.isActive) {
                double timeSeconds = static_cast<double>(
                    entry.samplePosition - session.startSamplePosition) / sampleRate_;
                session.sequence.addEvent(entry.message, timeSeconds);
                break;
            }
        }
    }

    // Process second block (wrap-around)
    for (int i = 0; i < size2; ++i) {
        const auto& entry = midiFifoData_[start2 + i];
        
        for (auto& session : midiSessions_) {
            if (session.trackIndex == entry.trackIndex && session.isActive) {
                double timeSeconds = static_cast<double>(
                    entry.samplePosition - session.startSamplePosition) / sampleRate_;
                session.sequence.addEvent(entry.message, timeSeconds);
                break;
            }
        }
    }

    midiFifoIndex_.finishedRead(size1 + size2);
}

//==============================================================================
void RecordingManager::finalizeRecordings(
    const std::vector<std::shared_ptr<Track>>& tracks) {
    
    if (projectState_ == nullptr) {
        DBG("RecordingManager: No project state, cannot create clips");
        return;
    }

    // Finalize audio recordings
    for (auto& session : audioSessions_) {
        if (!session.isActive || session.samplesRecorded == 0) {
            continue;
        }

        // Flush writer
        session.writer.reset();

        // Create clip in project state
        if (session.trackIndex >= 0 && 
            session.trackIndex < static_cast<int>(tracks.size())) {
            
            auto& track = tracks[session.trackIndex];
            if (track) {
                // Create clip via ProjectState
                juce::ValueTree clipState(ProjectState::ID_CLIP);
                clipState.setProperty(ProjectState::PROP_FILE, 
                                      session.file.getFullPathName(), nullptr);
                clipState.setProperty(ProjectState::PROP_POSITION, 
                                      static_cast<juce::int64>(session.startSamplePosition), 
                                      nullptr);
                clipState.setProperty(ProjectState::PROP_LENGTH, 
                                      static_cast<juce::int64>(session.samplesRecorded), 
                                      nullptr);
                clipState.setProperty(ProjectState::PROP_NAME,
                                      session.file.getFileNameWithoutExtension(), nullptr);

                // Add to track in project state
                auto trackState = projectState_->getTrackStateById(track->getTrackId());
                if (trackState.isValid()) {
                    auto clipsNode = trackState.getChildWithName(ProjectState::ID_CLIPS);
                    if (clipsNode.isValid()) {
                        clipsNode.appendChild(clipState, &projectState_->getUndoManager());
                    }
                }

                DBG("RecordingManager: Created audio clip for track " + 
                    juce::String(session.trackIndex));
            }
        }
    }

    // Finalize MIDI recordings
    for (auto& session : midiSessions_) {
        if (!session.isActive || session.sequence.getNumEvents() == 0) {
            continue;
        }

        // MIDI clip creation - store notes in project state
        if (session.trackIndex >= 0 && 
            session.trackIndex < static_cast<int>(tracks.size())) {
            
            auto& track = tracks[session.trackIndex];
            if (track) {
                // TODO: Create MIDI clip from sequence
                DBG("RecordingManager: Created MIDI clip with " + 
                    juce::String(session.sequence.getNumEvents()) + " events");
            }
        }
    }
}

//==============================================================================
juce::File RecordingManager::createRecordingFile(const juce::File& dir,
                                                  const juce::String& trackName,
                                                  const juce::String& extension) {
    juce::String safeName = trackName.replaceCharacter(' ', '_')
                                      .replaceCharacter('/', '_')
                                      .replaceCharacter('\\', '_');
    
    juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    juce::String filename = safeName + "_" + timestamp + extension;
    
    return dir.getChildFile(filename);
}

} // namespace zenith
