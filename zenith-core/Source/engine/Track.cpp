/**
 * @file Track.cpp
 * @brief Track implementation
 */

#include "Track.h"

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
// Audio Processing (AUDIO THREAD)
//==============================================================================

void Track::processBlock(juce::AudioBuffer<float>& mixBuffer, int numSamples)
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

    // For Phase 1, we'll just render silence (no clips yet)
    // TODO W11: Render clips, apply plugins, automation, etc.

    // For now, just apply gain/pan to silence (no-op)
    // When we add clip rendering, this will mix track audio into mixBuffer

    // Example placeholder for future clip rendering:
    // 1. Clear trackBuffer
    // 2. Render all clips into trackBuffer (additive)
    // 3. Apply track gain/pan
    // 4. Mix trackBuffer into mixBuffer (additive)

    // Since we have no clips yet, this is a no-op
    // The mixBuffer will remain silent for this track
}
