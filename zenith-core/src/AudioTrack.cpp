/**
 * @file AudioTrack.cpp
 * @brief Audio track implementation
 */

#include "../include/AudioTrack.h"

//==============================================================================
AudioTrack::AudioTrack(const juce::String& trackName, int trackIndex)
    : name(trackName), index(trackIndex)
{
    // Pre-allocate temp buffer for mixing (2 channels, 4096 samples)
    tempBuffer.setSize(2, 4096);

    DBG("AudioTrack: Created track '" + name + "' (index " + juce::String(index) + ")");
}

AudioTrack::~AudioTrack()
{
    DBG("AudioTrack: Destroyed track '" + name + "'");
}

//==============================================================================
// Processing (AUDIO THREAD)
//==============================================================================

void AudioTrack::process(juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 playheadPosition)
{
    // If muted, nothing to do
    if (muted.load())
    {
        return;
    }

    // Clear temp buffer
    tempBuffer.clear(0, numSamples);

    // Render all clips at current playhead position
    renderClips(tempBuffer, numSamples, playheadPosition);

    // Apply gain and pan
    applyGainAndPan(tempBuffer, numSamples);

    // Update peak level for metering
    float peak = 0.0f;
    for (int ch = 0; ch < tempBuffer.getNumChannels(); ++ch)
    {
        const auto* channelData = tempBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            peak = std::max(peak, std::abs(channelData[i]));
        }
    }
    peakLevel.store(peak);

    // Add to output buffer
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), tempBuffer.getNumChannels()); ++ch)
    {
        buffer.addFrom(ch, 0, tempBuffer, ch, 0, numSamples);
    }
}

void AudioTrack::renderClips(juce::AudioBuffer<float>& buffer, int numSamples, juce::int64 playheadPosition)
{
    // Try to acquire lock without blocking (non-blocking for real-time safety)
    // If we can't get the lock immediately, skip this frame (better than blocking)
    if (!clipLock.tryEnter())
    {
        return;
    }

    for (const auto& clip : clips)
    {
        // Check if clip is active at current playhead position
        juce::int64 clipStart = clip.startPosition;
        juce::int64 clipEnd = clipStart + clip.length;

        if (playheadPosition >= clipEnd || playheadPosition + numSamples <= clipStart)
        {
            // Clip is not active in this buffer
            continue;
        }

        // Calculate which part of the clip to play
        juce::int64 offsetIntoClip = std::max<juce::int64>(0, playheadPosition - clipStart);
        int samplesToRead = static_cast<int>(std::min<juce::int64>(
            numSamples,
            clip.length - offsetIntoClip
        ));

        // Calculate offset in output buffer (if clip starts mid-buffer)
        int startSampleInBuffer = static_cast<int>(std::max<juce::int64>(0, clipStart - playheadPosition));

        // Safety check: ensure we don't read beyond clip data
        if (offsetIntoClip + samplesToRead > clip.audioData.getNumSamples())
        {
            samplesToRead = clip.audioData.getNumSamples() - static_cast<int>(offsetIntoClip);
        }

        if (samplesToRead <= 0)
            continue;

        // Add clip audio to buffer
        for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), clip.audioData.getNumChannels()); ++ch)
        {
            buffer.addFrom(
                ch,
                startSampleInBuffer,
                clip.audioData,
                ch,
                static_cast<int>(offsetIntoClip),
                samplesToRead,
                clip.gain
            );
        }

        // Apply fade in/out if needed
        if (clip.fadeInSamples > 0 || clip.fadeOutSamples > 0)
        {
            applyFade(buffer, startSampleInBuffer, samplesToRead,
                     clip.fadeInSamples, clip.fadeOutSamples,
                     static_cast<int>(clip.length));
        }
    }

    clipLock.exit();
}

void AudioTrack::applyGainAndPan(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const float vol = volume.load();
    const float panValue = pan.load();

    // Calculate left and right gains based on pan (constant power panning)
    const float panRadians = panValue * juce::MathConstants<float>::pi * 0.25f;
    const float leftGain = vol * std::cos(panRadians);
    const float rightGain = vol * std::sin(panRadians);

    // Apply gain to channels
    if (buffer.getNumChannels() >= 1)
    {
        buffer.applyGain(0, 0, numSamples, leftGain);
    }

    if (buffer.getNumChannels() >= 2)
    {
        buffer.applyGain(1, 0, numSamples, rightGain);
    }
}

void AudioTrack::applyFade(juce::AudioBuffer<float>& buffer, int startSample, int numSamples,
                          int fadeInSamples, int fadeOutSamples, int clipTotalLength)
{
    // TODO: Implement fade in/out curves
    // For now, this is a placeholder
    // A real implementation would use:
    // - Linear, exponential, or S-curve fades
    // - Proper handling of very short fades
}

//==============================================================================
// Track Controls (MESSAGE THREAD)
//==============================================================================

void AudioTrack::setVolume(float newVolume)
{
    volume.store(juce::jlimit(0.0f, 2.0f, newVolume));
}

void AudioTrack::setPan(float newPan)
{
    pan.store(juce::jlimit(-1.0f, 1.0f, newPan));
}

void AudioTrack::setMute(bool shouldMute)
{
    muted.store(shouldMute);
}

void AudioTrack::setSolo(bool shouldSolo)
{
    soloed.store(shouldSolo);
}

void AudioTrack::setArmed(bool shouldArm)
{
    armed.store(shouldArm);
}

//==============================================================================
// Clip Management (MESSAGE THREAD)
//==============================================================================

void AudioTrack::addClip(const AudioClip& clip)
{
    const juce::ScopedLock lock(clipLock);
    clips.push_back(clip);

    DBG("AudioTrack: Added clip '" + clip.id + "' to track '" + name + "'");
}

void AudioTrack::removeClip(const juce::String& clipId)
{
    const juce::ScopedLock lock(clipLock);

    clips.erase(
        std::remove_if(clips.begin(), clips.end(),
            [&clipId](const AudioClip& c) { return c.id == clipId; }),
        clips.end()
    );

    DBG("AudioTrack: Removed clip '" + clipId + "' from track '" + name + "'");
}

void AudioTrack::clearClips()
{
    const juce::ScopedLock lock(clipLock);
    clips.clear();

    DBG("AudioTrack: Cleared all clips from track '" + name + "'");
}

//==============================================================================
// Info
//==============================================================================

float AudioTrack::getPeakLevel() const
{
    float peak = peakLevel.load();

    // Convert to dB
    if (peak < 0.00001f)
        return -100.0f;

    return juce::Decibels::gainToDecibels(peak);
}
