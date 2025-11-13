/**
 * @file AudioTrack.cpp
 * @brief Audio track implementation
 */

#include "../include/AudioTrack.h"

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO
#include "../include/Engine.h"
#endif

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

//==============================================================================
// W10.3: Voice-based rendering (AUDIO THREAD - RT-SAFE!)
//==============================================================================

#if defined(ZENITH_ENABLE_PHASE1_AUDIO) && ZENITH_ENABLE_PHASE1_AUDIO

//==============================================================================
// Voice Management Helpers
//==============================================================================

const AudioTrack::ClipDef* AudioTrack::findClipDef(int32_t clipId) const noexcept
{
    for (const auto& def : clipDefs_)
    {
        if (def.clipId == clipId)
            return &def;
    }
    return nullptr;
}

AudioTrack::ActiveVoice* AudioTrack::findVoiceForClip(int32_t clipId) noexcept
{
    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voices_[i].clip != nullptr && voices_[i].clip->clipId == clipId)
            return &voices_[i];
    }
    return nullptr;
}

AudioTrack::ActiveVoice* AudioTrack::allocVoiceRT() noexcept
{
    // First, try to find a free voice
    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voices_[i].clip == nullptr)
            return &voices_[i];
    }

    // No free voices - steal the oldest (voice 0)
    // Future: Could track age or implement LRU
    return &voices_[0];
}

//==============================================================================
// Event Handling
//==============================================================================

void AudioTrack::handleEventRT(const Engine::TransportEvent& ev, int offsetInBlock)
{
    switch (ev.type)
    {
        case Engine::TransportEvent::Type::StartClip:
            startClipRT(ev.clipId, offsetInBlock);
            break;

        case Engine::TransportEvent::Type::StopClip:
            stopClipRT(ev.clipId, offsetInBlock);
            break;
    }
}

void AudioTrack::startClipRT(int32_t clipId, int offsetInBlock)
{
    // Find ClipDef (message-thread data, read-only in RT)
    const ClipDef* def = findClipDef(clipId);
    if (def == nullptr)
        return;  // Clip not found (could be removed between schedule and playback)

    // Check if already playing
    if (findVoiceForClip(clipId) != nullptr)
        return;  // Already playing, ignore

    // Allocate voice
    ActiveVoice* voice = allocVoiceRT();
    if (voice == nullptr)
        return;  // Should not happen (allocVoiceRT always returns a voice)

    // Initialize voice state
    voice->clip = def;
    voice->playheadInClip = 0;
    voice->fadeInDone = (def->fadeInSamples == 0);
    voice->fadeOutDone = false;
    voice->fadingOut = false;

    // Note: offsetInBlock is when the event occurs, but we start rendering from the next segment
    // For true sample-accurate start, we'd render from offsetInBlock with partial segment
    // For now: voice starts playing in next segment (acceptable for W10.3)
    juce::ignoreUnused(offsetInBlock);
}

void AudioTrack::stopClipRT(int32_t clipId, int offsetInBlock)
{
    ActiveVoice* voice = findVoiceForClip(clipId);
    if (voice == nullptr)
        return;  // Not playing, ignore

    // Begin fade-out
    voice->fadingOut = true;

    // Note: offsetInBlock is when fade-out should begin
    // For now: fade starts immediately (acceptable for W10.3)
    juce::ignoreUnused(offsetInBlock);
}

//==============================================================================
// Segment Processing
//==============================================================================

void AudioTrack::processSegment(juce::AudioBuffer<float>& buffer, int64_t transportStart,
                                int offsetInBuffer, int segmentLen)
{
    // Render all active voices into segment
    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voices_[i].clip != nullptr)
        {
            renderVoiceIntoSegment(voices_[i], buffer, transportStart, offsetInBuffer, segmentLen);

            // Free voice if fade-out is complete
            if (voices_[i].fadeOutDone)
            {
                voices_[i].clip = nullptr;  // Mark as free
            }
        }
    }
}

//==============================================================================
// Voice Rendering with Fades (W13 logic)
//==============================================================================

void AudioTrack::renderVoiceIntoSegment(ActiveVoice& voice, juce::AudioBuffer<float>& buffer,
                                       int64_t transportStart, int offsetInBuffer, int segmentLen)
{
    const ClipDef* clip = voice.clip;
    if (clip == nullptr || clip->audioData == nullptr)
        return;

    // Calculate how many samples to read from clip
    const int64_t samplesRemaining = clip->clipLengthSamples - voice.playheadInClip;
    const int samplesToRead = (int) juce::jmin((int64_t) segmentLen, samplesRemaining);

    if (samplesToRead <= 0)
    {
        // Reached end of clip
        voice.fadeOutDone = true;
        return;
    }

    // Render clip audio into buffer with gain
    const int numChannels = juce::jmin(buffer.getNumChannels(), clip->audioData->getNumChannels());
    for (int ch = 0; ch < numChannels; ++ch)
    {
        buffer.addFrom(
            ch,
            offsetInBuffer,
            *clip->audioData,
            ch,
            (int) voice.playheadInClip,
            samplesToRead,
            clip->gain
        );
    }

    // Apply fades (W13 logic - per-voice state)
    const int fadeInSamples = clip->fadeInSamples;
    const int fadeOutSamples = clip->fadeOutSamples;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch, offsetInBuffer);

        for (int i = 0; i < samplesToRead; ++i)
        {
            float gain = 1.0f;
            const int64_t posInClip = voice.playheadInClip + i;

            // Fade-in (if not done)
            if (!voice.fadeInDone && fadeInSamples > 0)
            {
                if (posInClip < fadeInSamples)
                {
                    gain *= (float) posInClip / (float) fadeInSamples;
                }
                else
                {
                    voice.fadeInDone = true;
                }
            }

            // Fade-out (if fading out)
            if (voice.fadingOut && fadeOutSamples > 0)
            {
                const int64_t fadeOutStart = clip->clipLengthSamples - fadeOutSamples;
                if (posInClip >= fadeOutStart)
                {
                    const int64_t fadeOutPos = posInClip - fadeOutStart;
                    gain *= 1.0f - ((float) fadeOutPos / (float) fadeOutSamples);

                    if (fadeOutPos >= fadeOutSamples - 1)
                    {
                        voice.fadeOutDone = true;
                    }
                }
            }

            channelData[i] *= gain;
        }
    }

    // Advance playhead
    voice.playheadInClip += samplesToRead;

    // If reached end of clip and not fading out, mark as done
    if (voice.playheadInClip >= clip->clipLengthSamples)
    {
        voice.fadeOutDone = true;
    }
}

#if JUCE_DEBUG
//==============================================================================
// Test-Only API
//==============================================================================

void AudioTrack::addClipDefForTest(const ClipDef& def)
{
    clipDefs_.push_back(def);
}
#endif

#endif // ZENITH_ENABLE_PHASE1_AUDIO
