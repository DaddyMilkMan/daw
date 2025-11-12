/**
 * @file Track.cpp
 * @brief Track implementation
 */

#include "Track.h"
#include <algorithm>

using namespace zenith;

//==============================================================================
Track::Track()
{
}

Track::~Track()
{
    releaseResources();
}

//==============================================================================
// Lifecycle
//==============================================================================

void Track::prepareToPlay(int blockSize, double sampleRate)
{
    currentBlockSize = blockSize;
    currentSampleRate = sampleRate;

    // Pre-allocate track buffer (stereo for now)
    // No allocations will happen in processBlock() after this
    trackBuffer.setSize(2, blockSize, false, true, true);
    trackBuffer.clear();
}

void Track::releaseResources()
{
    trackBuffer.setSize(0, 0);
}

//==============================================================================
// W13: Clip Management
//==============================================================================

void Track::addClip(const Clip& clip)
{
    JUCE_ASSERT_MESSAGE_THREAD;  // W13.1: Enforce message-thread-only access

    if (!clip.isValid())
    {
        DBG("Track: Rejecting invalid clip");
        return;
    }

    clips_.push_back(clip);

    // Sort clips by startSample for efficient rendering
    std::sort(clips_.begin(), clips_.end(),
              [](const Clip& a, const Clip& b) { return a.startSample < b.startSample; });

    DBG("Track: Added clip (start=" + juce::String(clip.startSample)
        + ", length=" + juce::String(clip.lengthSamples) + ")");
    DBG("Track: Total clips: " + juce::String(clips_.size()));
}

void Track::clearClips()
{
    JUCE_ASSERT_MESSAGE_THREAD;  // W13.1: Enforce message-thread-only access
    clips_.clear();
}

//==============================================================================
// Audio Processing (AUDIO THREAD)
//==============================================================================

void Track::processBlock(juce::AudioBuffer<float>& mixBuffer,
                         int numSamples,
                         juce::int64 transportPosition)
{
    // ⚠️ AUDIO THREAD - REAL-TIME SAFE!
    //
    // NEVER:
    // - Allocate memory
    // - Lock mutexes
    // - Make system calls

    // Check mute
    if (muted_.load(std::memory_order_relaxed))
        return;  // Early exit for muted tracks (no processing)

    // Get track state (constant per block)
    const float trackGain = gain_.load(std::memory_order_relaxed);
    const float trackPan = pan_.load(std::memory_order_relaxed);

    // W13: Render clips that intersect this block
    const juce::int64 blockEnd = transportPosition + numSamples;

    for (const auto& clip : clips_)
    {
        // Skip clips that don't intersect this block
        if (!clip.intersectsBlock(transportPosition, numSamples))
            continue;

        // Compute overlap region
        const juce::int64 clipEnd = clip.getEndSample();

        // Destination range in mixBuffer [0, numSamples)
        const int dstStart = static_cast<int>(juce::jmax(juce::int64(0), clip.startSample - transportPosition));
        const int dstEnd = static_cast<int>(juce::jmin(juce::int64(numSamples), clipEnd - transportPosition));
        const int dstLength = dstEnd - dstStart;

        if (dstLength <= 0)
            continue;  // No overlap (edge case)

        // Source range in clip PCM buffer
        const juce::int64 srcStart = clip.srcOffset + juce::jmax(juce::int64(0), transportPosition - clip.startSample);
        const int srcLength = dstLength;

        // W13.1: Debug bounds validation (compile-time only)
        #if JUCE_DEBUG
            jassert(dstStart >= 0 && dstStart <= numSamples);
            jassert(dstEnd > dstStart && dstEnd <= numSamples);
            jassert(srcStart >= 0);
            jassert(srcStart + srcLength <= clip.pcm->getNumSamples());
        #endif

        // Safety check: ensure source range is valid
        if (srcStart < 0 || srcStart + srcLength > clip.pcm->getNumSamples())
            continue;  // Invalid source range (corrupted clip)

        // Compute gains for fade in/out
        const float baseGain = clip.gain * trackGain;

        // Determine fade zones
        const juce::int64 clipPlayPos = transportPosition + dstStart - clip.startSample;  // Position within clip
        const int fadeInEnd = clip.fadeInSamples;
        const int fadeOutStart = static_cast<int>(clip.lengthSamples - clip.fadeOutSamples);

        // Render clip with fades
        const int numMixChannels = mixBuffer.getNumChannels();
        const int numClipChannels = clip.pcm->getNumChannels();

        for (int ch = 0; ch < numMixChannels; ++ch)
        {
            // Map mix channel to clip channel (mono clips copy to all channels)
            const int clipCh = juce::jmin(ch, numClipChannels - 1);
            const float* clipData = clip.pcm->getReadPointer(clipCh, static_cast<int>(srcStart));
            float* mixData = mixBuffer.getWritePointer(ch, dstStart);

            // Three-zone rendering: fade-in, steady, fade-out
            int pos = 0;

            // Zone 1: Fade-in (if applicable)
            if (clipPlayPos < fadeInEnd && clip.fadeInSamples > 0)
            {
                const int fadeStart = static_cast<int>(juce::jmax(juce::int64(0), fadeInEnd - clipPlayPos));
                const int fadeLen = juce::jmin(fadeStart, srcLength - pos);

                if (fadeLen > 0)
                {
                    const float startGain = static_cast<float>(clipPlayPos + pos) / fadeInEnd;
                    const float endGain = static_cast<float>(clipPlayPos + pos + fadeLen) / fadeInEnd;

                    // Linear ramp gain
                    const float gainIncrement = (endGain - startGain) * baseGain / fadeLen;
                    for (int i = 0; i < fadeLen; ++i)
                        mixData[pos + i] += clipData[pos + i] * (baseGain * startGain + gainIncrement * i);

                    pos += fadeLen;
                }
            }

            // Zone 2: Steady (constant gain)
            const juce::int64 steadyStart = juce::jmax(clipPlayPos + pos, juce::int64(fadeInEnd));
            const juce::int64 steadyEnd = juce::jmin(clipPlayPos + srcLength, juce::int64(fadeOutStart));
            const int steadyLen = static_cast<int>(juce::jmax(juce::int64(0), steadyEnd - steadyStart));

            if (steadyLen > 0 && pos < srcLength)
            {
                const int actualSteadyLen = juce::jmin(steadyLen, srcLength - pos);
                juce::FloatVectorOperations::addWithMultiply(mixData + pos,
                                                              clipData + pos,
                                                              baseGain,
                                                              actualSteadyLen);
                pos += actualSteadyLen;
            }

            // Zone 3: Fade-out (if applicable)
            if (clipPlayPos + pos >= fadeOutStart && clip.fadeOutSamples > 0 && pos < srcLength)
            {
                const int fadeLen = srcLength - pos;

                if (fadeLen > 0)
                {
                    const float startGain = 1.0f - static_cast<float>(clipPlayPos + pos - fadeOutStart) / clip.fadeOutSamples;
                    const float endGain = 1.0f - static_cast<float>(clipPlayPos + pos + fadeLen - fadeOutStart) / clip.fadeOutSamples;

                    // Linear ramp gain
                    const float gainIncrement = (endGain - startGain) * baseGain / fadeLen;
                    for (int i = 0; i < fadeLen; ++i)
                        mixData[pos + i] += clipData[pos + i] * (baseGain * startGain + gainIncrement * i);

                    pos += fadeLen;
                }
            }
        }
    }

    // Track pan is applied at master level (Engine), not per-track in W10/W13
    // This allows efficient stereo panning in the final mix
}
