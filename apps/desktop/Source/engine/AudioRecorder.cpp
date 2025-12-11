/*
  ==============================================================================

    AudioRecorder.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

  ==============================================================================
*/

#include "AudioRecorder.h"
#include "Track.h"

namespace zenith {

AudioRecorder::AudioRecorder() {
    writerThread_ = std::make_unique<juce::TimeSliceThread>("Audio Recorder Thread");
    writerThread_->startThread(juce::Thread::Priority::high);
}

AudioRecorder::~AudioRecorder() {
    writerThread_->stopThread(2000);
}

void AudioRecorder::prepare(double sampleRate) {
    // Reset or prepare internal state if needed
}

void AudioRecorder::startRecording(const std::vector<std::shared_ptr<Track>>& tracks,
                                  const juce::AudioDeviceManager& deviceManager,
                                  juce::int64 startSample,
                                  const juce::File& recordingsDir) {
    
    auto* device = deviceManager.getCurrentAudioDevice();
    const int numInputChannels = device ? device->getActiveInputChannels().countNumberOfSetBits() : 2;
    const double sampleRate = device ? device->getCurrentSampleRate() : 44100.0;

    activeSessions_.clear();

    for (size_t i = 0; i < tracks.size(); ++i) {
        auto& track = tracks[i];
        if (!track->isArmed() || track->getType() != Track::Type::Audio) continue;

        // Generate filename
        juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        juce::String filename = track->getName().replaceCharacter(' ', '_') + "_" + timestamp + ".wav";
        juce::File file = recordingsDir.getChildFile(filename);

        // Create writer
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::FileOutputStream> outStream(new juce::FileOutputStream(file));
        
        if (!outStream->openedOk()) continue;

        auto writer = std::unique_ptr<juce::AudioFormatWriter>(
            wavFormat.createWriterFor(outStream.release(), sampleRate, 
                                    2, // Stereo for now
                                    24, {}, 0));

        if (writer) {
            ActiveSession session;
            session.writer = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
                writer.release(), *writerThread_, 32768);
            session.file = file;
            session.trackIndex = static_cast<int>(i);
            session.inputChannelIndex = track->getInputChannel();
            session.startSample = startSample;
            session.sampleRate = sampleRate;
            
            activeSessions_.push_back(std::move(session));
        }
    }

    if (!activeSessions_.empty()) {
        isRecording_.store(true);
    }
}

std::vector<AudioRecorder::RecordingResult> AudioRecorder::stopRecording() {
    isRecording_.store(false);
    
    std::vector<RecordingResult> results;
    
    for (auto& session : activeSessions_) {
        // This flushes the writer
        session.writer.reset();
        
        RecordingResult result;
        result.file = session.file;
        result.trackIndex = session.trackIndex;
        result.startSample = session.startSample;
        result.sampleRate = session.sampleRate;
        results.push_back(result);
    }
    
    activeSessions_.clear();
    return results;
}

void AudioRecorder::processBlock(const float* const* inputChannelData, int numInputChannels, int numSamples) {
    if (!isRecording_.load()) return;

    // Simple stereo recording for all tracks for MVP
    // In a real scenario, map input channels to tracks
    
    for (auto& session : activeSessions_) {
        // Map track input channel to device channel
        // For now, just record channels 0+1
        const float* left = (numInputChannels > 0) ? inputChannelData[0] : nullptr;
        const float* right = (numInputChannels > 1) ? inputChannelData[1] : left;
        
        if (left && right) {
            const float* channels[] = { left, right };
            session.writer->write(channels, numSamples);
        }
    }
}

} // namespace zenith
