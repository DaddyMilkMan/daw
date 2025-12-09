/**
 * @file EngineRecording.cpp
 * @brief Recording infrastructure (audio/MIDI recording, input handling, baking clips)
 */

#include "../include/Engine.h"
#include "../include/ProjectState.h"
#include "AudioFilePool.h"
#include "Track.h"
#include "Clip.h"
#include <algorithm>

namespace zenith {

//==============================================================================
// Recording Transport
//==============================================================================

void Engine::record()
{
    DBG("Engine: Record");

    // Start playback if not already playing
    if (!isPlaying_.load())
    {
        play();
    }

    // Get current sample rate and start position
    const double sampleRate = currentSampleRate.load();
    const juce::int64 recordStartSamples = playheadSamples_.load();

    // Setup MIDI recording
    {
        const juce::ScopedLock sl(midiRecordingLock_);
        midiRecording_.recordingStartSamples = recordStartSamples;
        midiRecording_.trackRecordings.resize(tracks_.size());
        for (auto& trackRecording : midiRecording_.trackRecordings)
        {
            trackRecording.clear();
        }
    }

    // Setup Audio recording
    juce::File recordingsDir;
    
    if (projectState_ != nullptr && projectState_->getProjectFile().existsAsFile())
    {
        recordingsDir = projectState_->getProjectFile().getSiblingFile("Audio Files");
    }
    else
    {
        recordingsDir = juce::File::getSpecialLocation(
            juce::File::userDocumentsDirectory).getChildFile("ZenithDAW/Recordings");
    }

    if (!recordingsDir.exists())
    {
        recordingsDir.createDirectory();
    }

    // Create recording sessions for all armed audio tracks
    audioRecordingSessions_.clear();

    for (size_t i = 0; i < tracks_.size(); ++i)
    {
        auto& track = tracks_[i];

        // Skip if not armed or not an audio track
        if (!track->isArmed() || track->getType() != zenith::Track::Type::Audio)
            continue;

        DBG("Engine: Creating recording session for track " + juce::String(i) +
            " (" + track->getName() + ")");

        // Check for pre-prepared session
        bool foundPrepped = false;
        AudioRecordingSession sessionToUse;
        {
            const juce::ScopedLock sl(preppedSessionsLock_);
            auto it = std::find_if(preppedSessions_.begin(), preppedSessions_.end(),
                [i](const auto& s) { return s.trackIndex == static_cast<int>(i); });
            
            if (it != preppedSessions_.end())
            {
                sessionToUse = std::move(*it);
                preppedSessions_.erase(it);
                foundPrepped = true;
            }
        }

        if (foundPrepped)
        {
            sessionToUse.recordingStartSamples = recordStartSamples;
            audioRecordingSessions_.push_back(std::move(sessionToUse));
            DBG("Engine: Used pre-prepared recording session for track " + juce::String(i));
            continue;
        }

        // Fallback: Create synchronously (BLOCKING I/O)
        DBG("Engine: Creating recording session synchronously (fallback)");

        juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        juce::String filename = track->getName().replaceCharacter(' ', '_') +
                               "_" + timestamp + ".wav";
        juce::File recordFile = recordingsDir.getChildFile(filename);

        auto* device = deviceManager.getCurrentAudioDevice();
        const int deviceInputChannels = device ? device->getActiveInputChannels().countNumberOfSetBits() : 1;
        const int numChannels = juce::jmin(2, juce::jmax(1, deviceInputChannels));

        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> fileStream(new juce::FileOutputStream(recordFile));

        if (!fileStream->openedOk()) continue;

        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(
                fileStream.release(),
                sampleRate,
                static_cast<unsigned int>(numChannels),
                24,
                {},
                0
            ));

        if (writer == nullptr) continue;

        auto threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            writer.release(),
            *audioWriterThread_,
            32768
        );

        AudioRecordingSession session;
        session.writer = std::move(threadedWriter);
        session.file = recordFile;
        session.numChannels = numChannels;
        session.sampleRate = sampleRate;
        session.recordingStartSamples = recordStartSamples;
        session.trackIndex = static_cast<int>(i);
        session.inputChannelIndex = tracks_[i]->getInputChannel();

        audioRecordingSessions_.push_back(std::move(session));
        DBG("Engine: Recording to " + recordFile.getFullPathName());
    }

    isRecording_.store(true);
    DBG("Engine: Recording started at sample " + juce::String(recordStartSamples));
}

void Engine::stopRecording()
{
    DBG("Engine: Stop recording");

    isRecording_.store(false);

    if (isShuttingDown_.load())
    {
        return;
    }

    // Bake MIDI
    bakeMidiRecordingsIntoClips(true);
    clearMidiRecordings();

    // Bake Audio (Async)
    juce::MessageManager::callAsync([this]()
    {
        if (isShuttingDown_.load()) return;

        DBG("Engine: Flushing and closing " +
            juce::String(audioRecordingSessions_.size()) + " recording sessions");

        for (auto& session : audioRecordingSessions_)
        {
            session.writer.reset(); // Flush/Close

            if (session.trackIndex >= 0 &&
                session.trackIndex < static_cast<int>(tracks_.size()))
            {
                auto& track = tracks_[session.trackIndex];
                bakeAudioRecordingIntoTrack(
                    *track,
                    session.file,
                    session.recordingStartSamples,
                    session.sampleRate);
            }
        }
        audioRecordingSessions_.clear();
        DBG("Engine: All recording sessions processed");
    });
}

//==============================================================================
// MIDI Input Handling
//==============================================================================

void Engine::enableMidiInput()
{
    DBG("Engine: Enabling MIDI input...");
    auto midiInputs = juce::MidiInput::getAvailableDevices();

    if (midiInputs.isEmpty()) return;

    for (const auto& input : midiInputs)
    {
        auto newInput = juce::MidiInput::openDevice(input.identifier, this);
        if (newInput != nullptr)
        {
            newInput->start();
            midiInputs_.push_back(std::move(newInput));
        }
    }

    // Initialize MIDI recording buffers
    {
        const juce::ScopedLock sl(midiRecordingLock_);
        midiRecording_.trackRecordings.resize(tracks_.size());
    }
}

void Engine::disableMidiInput()
{
    if (!midiInputs_.empty())
    {
        for (auto& input : midiInputs_)
        {
            if (input) input->stop();
        }
        midiInputs_.clear();
    }
}

void Engine::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
    juce::ignoreUnused(source);
    midiFifo_.push(message);

    if (isRecording_.load())
    {
        const juce::int64 playhead = playheadSamples_.load();
        auto* snapshot = activeSnapshot_.load();
        if (snapshot != nullptr)
        {
            for (size_t i = 0; i < snapshot->tracks.size(); ++i)
            {
                auto* track = snapshot->tracks[i];
                if (track != nullptr && track->isArmed() &&
                    (track->getType() == zenith::Track::Type::MIDI ||
                     track->getType() == zenith::Track::Type::Instrument))
                {
                    int start1, size1, start2, size2;
                    midiRecordFifo_.prepareToWrite(1, start1, size1, start2, size2);
                    if (size1 > 0)
                    {
                        midiRecordBuffer_[start1] = MidiRecordEvent{
                            message,
                            static_cast<int>(i),
                            playhead
                        };
                        midiRecordFifo_.finishedWrite(1);
                    }
                }
            }
        }
    }
}

void Engine::drainMidiRecordFifo()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    
    if (midiRecording_.trackRecordings.size() != tracks_.size())
    {
        midiRecording_.trackRecordings.resize(tracks_.size());
    }
    
    const juce::int64 recordStart = midiRecording_.recordingStartSamples;
    const double sampleRate = currentSampleRate.load();
    
    int numReady = midiRecordFifo_.getNumReady();
    int start1, size1, start2, size2;
    midiRecordFifo_.prepareToRead(numReady, start1, size1, start2, size2);
    
    for (int i = 0; i < size1; ++i)
    {
        const auto& event = midiRecordBuffer_[start1 + i];
        if (event.trackIndex >= 0 && event.trackIndex < static_cast<int>(midiRecording_.trackRecordings.size()))
        {
            double positionInSeconds = static_cast<double>(event.timestampSamples - recordStart) / sampleRate;
            juce::MidiMessage timestampedMsg(event.message);
            timestampedMsg.setTimeStamp(positionInSeconds);
            midiRecording_.trackRecordings[event.trackIndex].addEvent(timestampedMsg);
        }
    }
    
    for (int i = 0; i < size2; ++i)
    {
        const auto& event = midiRecordBuffer_[start2 + i];
        if (event.trackIndex >= 0 && event.trackIndex < static_cast<int>(midiRecording_.trackRecordings.size()))
        {
            double positionInSeconds = static_cast<double>(event.timestampSamples - recordStart) / sampleRate;
            juce::MidiMessage timestampedMsg(event.message);
            timestampedMsg.setTimeStamp(positionInSeconds);
            midiRecording_.trackRecordings[event.trackIndex].addEvent(timestampedMsg);
        }
    }
    
    midiRecordFifo_.finishedRead(numReady);
}

//==============================================================================
// Baking logic
//==============================================================================

void Engine::bakeAudioRecordingIntoTrack(
    zenith::Track& track,
    const juce::File& file,
    juce::int64 recordingStartSamples,
    double sampleRate)
{
    if (!file.existsAsFile()) return;

    auto fileHandle = audioFilePool_->loadFile(file);
    if (fileHandle == nullptr || !fileHandle->isValid()) return;

    auto clip = std::make_unique<zenith::Clip>();
    clip->setType(zenith::Clip::Type::Audio);
    clip->setName(file.getFileNameWithoutExtension());
    clip->setStartPosition(recordingStartSamples);
    clip->setLength(fileHandle->lengthInSamples);
    clip->setAudioFile(file);

    track.addClip(std::move(clip));

    // Sync with ProjectState
    if (projectState_ != nullptr)
    {
        juce::String trackId = track.getTrackId();
        if (trackId.isNotEmpty())
        {
            juce::String clipName = file.getFileNameWithoutExtension();
            juce::String clipId = projectState_->createClip(trackId, "audio", recordingStartSamples, fileHandle->lengthInSamples, clipName, "Record Audio");
            if (clipId.isNotEmpty())
            {
                projectState_->setClipAudioFile(trackId, clipId, file, "Record Audio File");
            }
        }
    }
}

void Engine::bakeMidiRecordingsIntoClips(bool quantize)
{
    drainMidiRecordFifo();
    const double tempo = (projectState_ != nullptr) ? projectState_->getTempo() : 120.0;
    const juce::int64 recordStart = midiRecording_.recordingStartSamples;

    for (size_t i = 0; i < midiRecording_.trackRecordings.size() && i < tracks_.size(); ++i)
    {
        auto& recording = midiRecording_.trackRecordings[i];
        if (recording.getNumEvents() == 0) continue;

        juce::MidiMessageSequence finalSequence = recording;
        if (quantize)
        {
            finalSequence = quantizeMidiSequence(recording, tempo, 0.25);
        }
        finalSequence.updateMatchedPairs();

        const double endTimeSeconds = finalSequence.getEndTime();
        const juce::int64 clipLengthSamples = static_cast<juce::int64>(endTimeSeconds * currentSampleRate.load());

        auto clip = std::make_unique<zenith::Clip>();
        clip->setType(zenith::Clip::Type::MIDI);
        clip->setName("MIDI Recording");
        clip->setMidiSequence(finalSequence);
        clip->setStartPosition(recordStart);
        clip->setLength(clipLengthSamples);

        auto* track = tracks_[i].get();
        if (track != nullptr) {
            track->addClip(std::move(clip));
        }
    }
}

void Engine::clearMidiRecordings()
{
    for (auto& recording : midiRecording_.trackRecordings)
    {
        recording.clear();
    }
    midiRecording_.recordingStartSamples = 0;
    
    int numReady = midiRecordFifo_.getNumReady();
    if (numReady > 0)
    {
        int start1, size1, start2, size2;
        midiRecordFifo_.prepareToRead(numReady, start1, size1, start2, size2);
        midiRecordFifo_.finishedRead(numReady);
    }
}

juce::MidiMessageSequence Engine::quantizeMidiSequence(
    const juce::MidiMessageSequence& input,
    double tempo,
    double quantizeGrid)
{
    const double beatsPerSecond = tempo / 60.0;
    const double quarterNoteSeconds = 1.0 / beatsPerSecond;
    const double gridSeconds = quarterNoteSeconds * quantizeGrid;

    juce::MidiMessageSequence output;

    for (int i = 0; i < input.getNumEvents(); ++i)
    {
        auto* event = input.getEventPointer(i);
        if (event == nullptr) continue;

        double timestamp = event->message.getTimeStamp();
        double quantized = std::round(timestamp / gridSeconds) * gridSeconds;
        quantized = juce::jmax(0.0, quantized);

        juce::MidiMessage msg(event->message);
        msg.setTimeStamp(quantized);
        output.addEvent(msg);
    }
    output.updateMatchedPairs();
    return output;
}

void Engine::prepareRecordingForTrack(int trackIndex)
{
    juce::Thread::launch([this, trackIndex]() {
        if (audioFilePool_ == nullptr || audioWriterThread_ == nullptr) return;

        auto* device = deviceManager.getCurrentAudioDevice();
        if (device == nullptr) return;

        double sampleRate = device->getCurrentSampleRate();
        int numChannels = 2; // Force stereo

        juce::File recordingsDir;
        if (projectState_ != nullptr && projectState_->getProjectFile().existsAsFile())
        {
            recordingsDir = projectState_->getProjectFile().getSiblingFile("Audio Files");
        }
        else
        {
            recordingsDir = juce::File::getSpecialLocation(
                juce::File::userDocumentsDirectory).getChildFile("ZenithDAW/Recordings");
        }

        if (!recordingsDir.exists()) recordingsDir.createDirectory();

        juce::String trackName = "Track_" + juce::String(trackIndex);
        juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        juce::String filename = trackName + "_" + timestamp + ".wav";
        juce::File recordFile = recordingsDir.getChildFile(filename);
        
        auto fileStream = std::make_unique<juce::FileOutputStream>(recordFile);
        if (fileStream->failedToOpen()) return;

        juce::AudioFormatManager& formatManager = audioFilePool_->getFormatManager();
        auto* wavFormat = formatManager.findFormatForFileExtension("wav");
        if (wavFormat == nullptr) return;

        auto* writer = wavFormat->createWriterFor(fileStream.get(), sampleRate, numChannels, 24, {}, 0);
        if (writer == nullptr) return;

        fileStream.release();

        auto threadedWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            writer, *audioWriterThread_, 32768);

        AudioRecordingSession session;
        session.writer = std::move(threadedWriter);
        session.file = recordFile;
        session.numChannels = numChannels;
        session.sampleRate = sampleRate;
        session.trackIndex = trackIndex;
        
        {
            const juce::ScopedLock sl(preppedSessionsLock_);
            preppedSessions_.erase(std::remove_if(preppedSessions_.begin(), preppedSessions_.end(),
                [trackIndex](const auto& s) { return s.trackIndex == trackIndex; }), preppedSessions_.end());
            
            preppedSessions_.push_back(std::move(session));
        }
    });
}

//==============================================================================
// Audio Thread Capture
//==============================================================================

void Engine::captureAudioInput(
    const float* const* inputChannelData,
    int numInputChannels,
    int numSamples) noexcept
{
    if (inputChannelData == nullptr || numInputChannels == 0) return;

    auto* snapshot = activeSnapshot_.load();
    if (snapshot == nullptr) return;

    for (auto& session : audioRecordingSessions_)
    {
        if (session.writer == nullptr) continue;

        int inputChannel = 0;
        if (session.trackIndex >= 0 && session.trackIndex < static_cast<int>(snapshot->tracks.size()))
        {
            auto* track = snapshot->tracks[session.trackIndex];
            if (track != nullptr) inputChannel = track->getInputChannel();
        }

        if (session.numChannels == 1)
        {
            if (inputChannel < numInputChannels && inputChannelData[inputChannel] != nullptr)
            {
                const float* channelData[1] = { inputChannelData[inputChannel] };
                session.writer->write(channelData, numSamples);
            }
        }
        else if (session.numChannels == 2)
        {
            int leftCh = inputChannel;
            int rightCh = inputChannel + 1;
            
            const float* leftData = (leftCh < numInputChannels) ? inputChannelData[leftCh] : nullptr;
            const float* rightData = (rightCh < numInputChannels) ? inputChannelData[rightCh] : nullptr;
            
            if (leftData && rightData)
            {
                const float* channelData[2] = { leftData, rightData };
                session.writer->write(channelData, numSamples);
            }
            else if (leftData)
            {
                const float* channelData[2] = { leftData, leftData };
                session.writer->write(channelData, numSamples);
            }
        }
    }
}

} // namespace zenith
