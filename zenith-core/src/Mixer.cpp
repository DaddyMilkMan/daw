/**
 * @file Mixer.cpp
 * @brief Mixer implementation
 */

#include "../include/Mixer.h"

//==============================================================================
Mixer::Mixer()
{
    DBG("Mixer: Constructor");

    // Pre-allocate mix buffer (2 channels, 4096 samples)
    mixBuffer.setSize(2, 4096);
}

Mixer::~Mixer()
{
    DBG("Mixer: Destructor");
}

//==============================================================================
// Processing (AUDIO THREAD)
//==============================================================================

void Mixer::process(juce::AudioBuffer<float>& outputBuffer, int numSamples, juce::int64 playheadPosition)
{
    // Clear output buffer
    outputBuffer.clear();

    // Process all tracks and mix to output
    processTracks(outputBuffer, numSamples, playheadPosition);

    // Apply master volume and limiting
    applyMasterProcessing(outputBuffer, numSamples);

    // Update master peak level
    float peak = 0.0f;
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
    {
        const auto* channelData = outputBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            peak = std::max(peak, std::abs(channelData[i]));
        }
    }
    masterPeakLevel.store(peak);
}

void Mixer::processTracks(juce::AudioBuffer<float>& outputBuffer, int numSamples, juce::int64 playheadPosition)
{
    // Try to acquire lock without blocking
    // If we can't get the lock, skip this frame (better than blocking audio thread)
    if (!trackLock.tryEnter())
    {
        return;
    }

    // Check if any tracks are soloed
    bool hasSoloedTracks = false;
    for (const auto& track : tracks)
    {
        if (track->isSoloed())
        {
            hasSoloedTracks = true;
            break;
        }
    }
    anySoloActive.store(hasSoloedTracks);

    // Process each track
    for (auto& track : tracks)
    {
        // Solo logic:
        // - If any track is soloed, only play soloed tracks
        // - If no tracks are soloed, play all non-muted tracks
        bool shouldPlay = !track->isMuted();

        if (hasSoloedTracks)
        {
            shouldPlay = track->isSoloed();
        }

        if (shouldPlay)
        {
            track->process(outputBuffer, numSamples, playheadPosition);
        }
    }

    trackLock.exit();
}

void Mixer::applyMasterProcessing(juce::AudioBuffer<float>& buffer, int numSamples)
{
    // Apply master volume
    const float vol = masterVolume.load();
    buffer.applyGain(0, numSamples, vol);

    // Simple soft clipping to prevent digital clipping
    // For a commercial DAW, you'd use a proper limiter here
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            // Soft clip at ±1.0
            float sample = channelData[i];
            if (sample > 1.0f)
                channelData[i] = 1.0f - (1.0f / (1.0f + (sample - 1.0f)));
            else if (sample < -1.0f)
                channelData[i] = -1.0f + (1.0f / (1.0f + (-sample - 1.0f)));
        }
    }
}

//==============================================================================
// Track Management (MESSAGE THREAD)
//==============================================================================

AudioTrack* Mixer::addTrack(const juce::String& name)
{
    const juce::ScopedLock lock(trackLock);

    int index = static_cast<int>(tracks.size());
    auto track = std::make_unique<AudioTrack>(name, index);
    AudioTrack* trackPtr = track.get();

    tracks.push_back(std::move(track));

    DBG("Mixer: Added track '" + name + "' (index " + juce::String(index) + ")");

    return trackPtr;
}

void Mixer::removeTrack(int index)
{
    const juce::ScopedLock lock(trackLock);

    if (index >= 0 && index < static_cast<int>(tracks.size()))
    {
        juce::String name = tracks[index]->getName();
        tracks.erase(tracks.begin() + index);

        DBG("Mixer: Removed track '" + name + "' (index " + juce::String(index) + ")");

        // Update track indices
        for (size_t i = 0; i < tracks.size(); ++i)
        {
            // Note: Track indices would need to be updated here
            // For now, we keep original indices
        }
    }
}

AudioTrack* Mixer::getTrack(int index)
{
    const juce::ScopedLock lock(trackLock);

    if (index >= 0 && index < static_cast<int>(tracks.size()))
    {
        return tracks[index].get();
    }

    return nullptr;
}

int Mixer::getNumTracks() const
{
    // Note: Reading size() without lock is technically data race
    // For commercial DAW, use std::atomic<int> numTracks
    return static_cast<int>(tracks.size());
}

void Mixer::clearAllTracks()
{
    const juce::ScopedLock lock(trackLock);
    tracks.clear();

    DBG("Mixer: Cleared all tracks");
}

//==============================================================================
// Master Controls (MESSAGE THREAD)
//==============================================================================

void Mixer::setMasterVolume(float newVolume)
{
    masterVolume.store(juce::jlimit(0.0f, 2.0f, newVolume));
}

float Mixer::getMasterPeakLevel() const
{
    float peak = masterPeakLevel.load();

    // Convert to dB
    if (peak < 0.00001f)
        return -100.0f;

    return juce::Decibels::gainToDecibels(peak);
}

//==============================================================================
// Initialization
//==============================================================================

void Mixer::prepare(double sampleRate, int maxBlockSize)
{
    DBG("Mixer: Prepare - Sample rate: " + juce::String(sampleRate) +
        ", Max block size: " + juce::String(maxBlockSize));

    currentSampleRate.store(sampleRate);
    currentMaxBlockSize.store(maxBlockSize);

    // Resize mix buffer to handle max block size
    mixBuffer.setSize(2, maxBlockSize);
}

void Mixer::release()
{
    DBG("Mixer: Release");

    // Clear mix buffer
    mixBuffer.setSize(0, 0);
}
