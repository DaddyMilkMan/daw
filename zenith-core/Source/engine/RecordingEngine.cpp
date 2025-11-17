/*
  ==============================================================================

    RecordingEngine.cpp
    Created: 2025-11-17
    Author:  Zenith DAW

    Real-time safe audio recording engine implementation

  ==============================================================================
*/

#include "RecordingEngine.h"
#include "Track.h"
#include "Clip.h"
#include "../../include/Engine.h"
#include "../../include/ProjectState.h"

namespace zenith {

//==============================================================================
RecordingEngine::RecordingEngine(ProjectState& projectState, Engine& engine)
    : projectState_(projectState)
    , engine_(engine)
{
    DBG("RecordingEngine: Constructor");
    wavFormat_ = std::make_unique<juce::WavAudioFormat>();

    // Start recording thread with low priority
    recordingThread_.startThread(juce::Thread::Priority::low);
}

RecordingEngine::~RecordingEngine()
{
    DBG("RecordingEngine: Destructor");

    // Ensure recording is stopped
    if (isRecording())
    {
        stopRecording();
    }

    // Clean up any temp files
    if (tempRecordingFile_.existsAsFile())
    {
        tempRecordingFile_.deleteFile();
    }
}

//==============================================================================
// Recording Control (Message Thread)
//==============================================================================

void RecordingEngine::startRecording(double sampleRate, int numChannels)
{
    DBG("RecordingEngine: Starting recording...");
    DBG("  Sample Rate: " + juce::String(sampleRate) + " Hz");
    DBG("  Channels: " + juce::String(numChannels));

    // Store parameters
    recordingSampleRate_ = sampleRate;
    recordingNumChannels_ = numChannels;
    recordedSampleCount_.store(0);

    // Get current playback position from engine as recording start point
    // TODO: Engine needs to expose playback position - for now use 0
    recordingStartSample_ = 0;

    // Create temporary recording file
    tempRecordingFile_ = getTempRecordingFile();
    DBG("  Temp File: " + tempRecordingFile_.getFullPathName());

    // Create file output stream as unique_ptr<OutputStream>
    std::unique_ptr<juce::OutputStream> tempFileStream =
        std::make_unique<juce::FileOutputStream>(tempRecordingFile_);

    if (!static_cast<juce::FileOutputStream*>(tempFileStream.get())->openedOk())
    {
        DBG("RecordingEngine: ERROR - Failed to create temp file!");
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Recording Error",
            "Failed to create temporary recording file:\n" + tempRecordingFile_.getFullPathName(),
            "OK");
        return;
    }

    // Create WAV writer (16-bit PCM for now) using JUCE 8 API
    juce::AudioFormatWriter::Options options;
    options = options.withSampleRate(sampleRate)
                     .withNumChannels(numChannels)
                     .withBitsPerSample(16);

    // Create writer - this takes ownership of the stream
    formatWriter_ = wavFormat_->createWriterFor(tempFileStream, options);

    if (formatWriter_ == nullptr)
    {
        DBG("RecordingEngine: ERROR - Failed to create WAV writer!");
        return;
    }

    // Create threaded writer for RT-safe recording
    // Buffer size: 32768 samples should be plenty for the FIFO
    const int fifoSize = 32768;
    threadedWriter_ = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
        formatWriter_.get(),
        recordingThread_,  // Use our TimeSliceThread
        fifoSize);

    // Set recording flag (audio thread will start writing)
    isRecording_.store(true);

    DBG("RecordingEngine: Recording started!");
}

void RecordingEngine::stopRecording()
{
    if (!isRecording())
        return;

    DBG("RecordingEngine: Stopping recording...");

    // Stop recording (audio thread will stop writing)
    isRecording_.store(false);

    // Get final sample count
    const int64_t totalSamples = recordedSampleCount_.load();
    DBG("  Recorded Samples: " + juce::String(totalSamples));
    DBG("  Duration: " + juce::String(totalSamples / recordingSampleRate_, 2) + " seconds");

    // Flush and close threaded writer
    if (threadedWriter_)
    {
        threadedWriter_.reset();  // Flushes remaining samples and stops thread
    }

    // Close format writer
    if (formatWriter_)
    {
        formatWriter_->flush();
        formatWriter_.reset();
    }

    // Create clips from the recording
    createClipsFromRecording();

    DBG("RecordingEngine: Recording stopped!");
}

//==============================================================================
// Audio Input (Audio Thread - RT-safe!)
//==============================================================================

void RecordingEngine::pushAudioInput(const float* const* inputData, int numChannels, int numSamples)
{
    // ⚠️ AUDIO THREAD - MUST BE REAL-TIME SAFE!

    if (!isRecording())
        return;

    if (threadedWriter_ == nullptr)
        return;

    // Write to threaded writer (lock-free FIFO)
    // This is RT-safe - writes to a lock-free buffer that a background thread reads
    threadedWriter_->write(inputData, numSamples);

    // Update sample count
    recordedSampleCount_.fetch_add(numSamples);
}

//==============================================================================
// Helper Methods
//==============================================================================

void RecordingEngine::createClipsFromRecording()
{
    DBG("RecordingEngine: Creating clips from recording...");

    // Check if we have a valid recording
    if (!tempRecordingFile_.existsAsFile())
    {
        DBG("RecordingEngine: ERROR - Temp file doesn't exist!");
        return;
    }

    const int64_t totalSamples = recordedSampleCount_.load();
    if (totalSamples == 0)
    {
        DBG("RecordingEngine: WARNING - No samples recorded, skipping clip creation");
        tempRecordingFile_.deleteFile();
        return;
    }

    // Read the recorded audio back into memory
    std::unique_ptr<juce::AudioFormatReader> reader(wavFormat_->createReaderFor(
        new juce::FileInputStream(tempRecordingFile_), true));

    if (reader == nullptr)
    {
        DBG("RecordingEngine: ERROR - Failed to read recorded audio!");
        tempRecordingFile_.deleteFile();
        return;
    }

    // Load audio into buffer
    juce::AudioBuffer<float> recordedAudio(
        static_cast<int>(reader->numChannels),
        static_cast<int>(reader->lengthInSamples));

    reader->read(&recordedAudio,
                 0,
                 static_cast<int>(reader->lengthInSamples),
                 0,
                 true,
                 true);

    DBG("RecordingEngine: Loaded " + juce::String(recordedAudio.getNumSamples()) + " samples");

    // Find all armed tracks in the engine
    const auto& tracks = engine_.tracks();
    int clipsCreated = 0;

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        const auto* track = tracks[i].get();

        if (track == nullptr || !track->isArmed())
            continue;

        DBG("RecordingEngine: Creating clip for armed track: " + track->getName());

        // Create a new clip with the recorded audio
        auto clip = std::make_unique<Track::Clip>();
        clip->setType(Track::Clip::Type::Audio);
        clip->setName("Recording " + juce::Time::getCurrentTime().formatted("%H:%M:%S"));
        clip->setAudioBuffer(recordedAudio);

        // Set timeline position
        clip->setStartPosition(recordingStartSample_);
        clip->setLength(recordedAudio.getNumSamples());

        // Add clip to track
        // Note: This is not ideal - we're modifying tracks directly
        // TODO: Should go through ProjectState/UndoManager for proper undo support
        const_cast<Track*>(track)->addClip(std::move(clip));

        clipsCreated++;
    }

    if (clipsCreated == 0)
    {
        DBG("RecordingEngine: WARNING - No armed tracks found, recording discarded!");
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Recording",
            "Recording completed, but no tracks were armed.\nThe recording has been discarded.",
            "OK");
    }
    else
    {
        DBG("RecordingEngine: Created " + juce::String(clipsCreated) + " clip(s)");
    }

    // Clean up temp file
    tempRecordingFile_.deleteFile();
}

juce::File RecordingEngine::getTempRecordingFile()
{
    // Create temp file in system temp directory
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto timestamp = juce::Time::getCurrentTime().toMilliseconds();
    auto filename = "zenith_recording_" + juce::String(timestamp) + ".wav";

    return tempDir.getChildFile(filename);
}

} // namespace zenith
