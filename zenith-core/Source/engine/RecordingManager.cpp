/*
  ==============================================================================

    RecordingManager.cpp
    Created: 2025-11-17
    Author:  Zenith DAW

    Real-time safe audio recording implementation

  ==============================================================================
*/

#include "RecordingManager.h"
#include "../../include/ProjectState.h"
#include "Track.h"
#include "Clip.h"

namespace zenith {

//==============================================================================
RecordingManager::RecordingManager()
    : fifo_(RING_BUFFER_SIZE)
{
    // Initialize ring buffer with 2 channels (stereo input)
    ringBuffer_.setSize(2, RING_BUFFER_SIZE);
    ringBuffer_.clear();

    // Pre-allocate recorded audio buffer (will grow as needed)
    recordedAudioBuffer_.setSize(2, 44100 * 60); // Start with 60 seconds capacity
    recordedAudioBuffer_.clear();
}

RecordingManager::~RecordingManager()
{
    // Ensure recording is stopped
    if (isRecording_.load())
    {
        juce::Logger::writeToLog("RecordingManager destroyed while recording - forcing stop");
        isRecording_.store(false);
    }
}

//==============================================================================
void RecordingManager::startRecording(double sampleRate, double currentBeat, int64_t samplePosition)
{
    const juce::ScopedLock sl(recordingStateLock_);

    if (isRecording_.load())
    {
        juce::Logger::writeToLog("RecordingManager: Already recording, ignoring start request");
        return;
    }

    // Store recording state
    recordingSampleRate_.store(sampleRate);
    recordingStartBeat_ = currentBeat;
    recordingStartSample_ = samplePosition;
    recordedSampleCount_ = 0;

    // Reset ring buffer
    fifo_.reset();
    ringBuffer_.clear();

    // Clear recorded audio buffer
    recordedAudioBuffer_.clear();

    // Ensure we have enough capacity (grow if needed)
    const int initialCapacity = static_cast<int>(sampleRate * 60.0); // 60 seconds
    if (recordedAudioBuffer_.getNumSamples() < initialCapacity)
    {
        recordedAudioBuffer_.setSize(maxRecordedChannels_, initialCapacity, false, false, true);
    }

    juce::Logger::writeToLog(
        "RecordingManager: Started recording at beat " + juce::String(currentBeat) +
        ", sample " + juce::String(samplePosition) +
        ", rate " + juce::String(sampleRate));

    // Start recording (atomic flag for audio thread)
    isRecording_.store(true);
}

int RecordingManager::stopRecording(ProjectState* projectState, const std::vector<Track*>& armedTracks)
{
    // Stop recording first (audio thread will stop writing)
    isRecording_.store(false);

    const juce::ScopedLock sl(recordingStateLock_);

    if (recordedSampleCount_ == 0)
    {
        juce::Logger::writeToLog("RecordingManager: No audio recorded, not creating clips");
        return 0;
    }

    // Process any remaining audio in the ring buffer
    processRecordedAudio();

    juce::Logger::writeToLog(
        "RecordingManager: Stopped recording, recorded " +
        juce::String(recordedSampleCount_) + " samples");

    // Create clips for each armed track
    int clipsCreated = 0;

    for (auto* track : armedTracks)
    {
        if (track == nullptr || !track->isArmed())
            continue;

        // Create a new clip with the recorded audio
        auto clip = std::make_unique<Track::Clip>();
        clip->setName(track->getName() + " Recording");
        clip->setType(Track::Clip::Type::Audio);

        // Copy recorded audio to clip's buffer
        // Create a new buffer with just the recorded samples (not the full capacity)
        juce::AudioBuffer<float> clipBuffer(
            juce::jmin(2, recordedAudioBuffer_.getNumChannels()),
            recordedSampleCount_);

        for (int ch = 0; ch < clipBuffer.getNumChannels(); ++ch)
        {
            clipBuffer.copyFrom(ch, 0, recordedAudioBuffer_, ch, 0, recordedSampleCount_);
        }

        clip->setAudioBuffer(clipBuffer);

        // Set clip position and length (in samples)
        // Position: where recording started (sample position)
        // Length: number of samples recorded
        clip->setStartPosition(recordingStartSample_);
        clip->setLength(recordedSampleCount_);

        // Add clip to track
        track->addClip(std::move(clip));

        juce::Logger::writeToLog(
            "RecordingManager: Created clip on track '" + track->getName() +
            "' at sample " + juce::String(recordingStartSample_) +
            ", length " + juce::String(recordedSampleCount_) + " samples");

        clipsCreated++;
    }

    // Reset state
    recordedSampleCount_ = 0;

    return clipsCreated;
}

//==============================================================================
void RecordingManager::pushInputFromAudioCallback(
    const float* const* inputChannelData,
    int numInputChannels,
    int numSamples)
{
    // ⚠️ AUDIO THREAD - MUST BE REAL-TIME SAFE!

    if (!isRecording_.load())
        return;

    if (inputChannelData == nullptr || numInputChannels == 0 || numSamples == 0)
        return;

    // Check if we have space in the ring buffer
    const int freeSpace = fifo_.getFreeSpace();
    if (freeSpace < numSamples)
    {
        // Ring buffer is full - drop samples (better than blocking)
        // This shouldn't happen with a 10-second buffer, but handle it gracefully
        return;
    }

    // Get write pointers from FIFO
    int start1, size1, start2, size2;
    fifo_.prepareToWrite(numSamples, start1, size1, start2, size2);

    // Write to ring buffer (lock-free)
    // We write up to 2 channels (stereo)
    const int channelsToWrite = juce::jmin(numInputChannels, ringBuffer_.getNumChannels());

    // First block
    if (size1 > 0)
    {
        for (int ch = 0; ch < channelsToWrite; ++ch)
        {
            if (inputChannelData[ch] != nullptr)
            {
                juce::FloatVectorOperations::copy(
                    ringBuffer_.getWritePointer(ch, start1),
                    inputChannelData[ch],
                    size1);
            }
        }
    }

    // Second block (if buffer wraps around)
    if (size2 > 0)
    {
        for (int ch = 0; ch < channelsToWrite; ++ch)
        {
            if (inputChannelData[ch] != nullptr)
            {
                juce::FloatVectorOperations::copy(
                    ringBuffer_.getWritePointer(ch, start2),
                    inputChannelData[ch] + size1,
                    size2);
            }
        }
    }

    // Finalize write (updates FIFO state)
    fifo_.finishedWrite(size1 + size2);
}

void RecordingManager::processRecordedAudio()
{
    // ⚠️ MESSAGE THREAD - Can allocate/lock if needed

    if (!isRecording_.load() && fifo_.getNumReady() == 0)
        return;

    // Process all available samples in the ring buffer
    while (fifo_.getNumReady() > 0)
    {
        // Get read pointers from FIFO
        int start1, size1, start2, size2;
        const int numReady = fifo_.getNumReady();
        fifo_.prepareToRead(numReady, start1, size1, start2, size2);

        const juce::ScopedLock sl(recordingStateLock_);

        // Ensure we have enough space in recorded buffer
        const int requiredCapacity = recordedSampleCount_ + size1 + size2;
        if (requiredCapacity > recordedAudioBuffer_.getNumSamples())
        {
            // Grow buffer (double the size)
            const int newSize = juce::jmax(requiredCapacity, recordedAudioBuffer_.getNumSamples() * 2);
            juce::AudioBuffer<float> newBuffer(maxRecordedChannels_, newSize);
            newBuffer.clear();

            // Copy existing data
            for (int ch = 0; ch < maxRecordedChannels_; ++ch)
            {
                newBuffer.copyFrom(ch, 0, recordedAudioBuffer_, ch, 0, recordedSampleCount_);
            }

            recordedAudioBuffer_ = std::move(newBuffer);
        }

        // Copy from ring buffer to recorded buffer
        const int numChannels = ringBuffer_.getNumChannels();

        // First block
        if (size1 > 0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                recordedAudioBuffer_.copyFrom(
                    ch,
                    recordedSampleCount_,
                    ringBuffer_,
                    ch,
                    start1,
                    size1);
            }
            recordedSampleCount_ += size1;
        }

        // Second block (if buffer wraps)
        if (size2 > 0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                recordedAudioBuffer_.copyFrom(
                    ch,
                    recordedSampleCount_,
                    ringBuffer_,
                    ch,
                    start2,
                    size2);
            }
            recordedSampleCount_ += size2;
        }

        // Finalize read (updates FIFO state)
        fifo_.finishedRead(size1 + size2);
    }
}

} // namespace zenith
