/**
 * @file Mixer.cpp
 * @brief Mixer implementation
 */

#include "Mixer.h"
#include "../../include/Engine.h"

namespace zenith {

Mixer::Mixer()
{
}

Mixer::~Mixer()
{
}

//==============================================================================
// Track Management (MESSAGE THREAD)
//==============================================================================

Track* Mixer::getTrack(int index) noexcept
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();
    return nullptr;
}

const Track* Mixer::getTrack(int index) const noexcept
{
    if (index >= 0 && index < static_cast<int>(tracks_.size()))
        return tracks_[static_cast<size_t>(index)].get();
    return nullptr;
}

int Mixer::addTrack()
{
    // TODO: Add jassert(isMessageThread())
    // TODO: Add jassert(!engine.isPlaying())
    tracks_.push_back(std::make_unique<Track>());
    return static_cast<int>(tracks_.size()) - 1;
}

void Mixer::clearTracks()
{
    tracks_.clear();
}

void Mixer::setNumTracks(int numTracks)
{
    // TODO: Add jassert(isMessageThread())
    // TODO: Add jassert(!engine.isPlaying())

    if (numTracks < 0)
        numTracks = 0;

    const int currentCount = static_cast<int>(tracks_.size());

    if (numTracks > currentCount)
    {
        // Add tracks
        for (int i = currentCount; i < numTracks; ++i)
            tracks_.push_back(std::make_unique<Track>());
    }
    else if (numTracks < currentCount)
    {
        // Remove tracks from end
        tracks_.resize(static_cast<size_t>(numTracks));
    }
}

//==============================================================================
// Audio Processing (AUDIO THREAD)
//==============================================================================

void Mixer::processSegment(juce::AudioBuffer<float>& mixBuffer,
                           int segmentOffset,
                           int segmentLength,
                           int64_t timelineSample)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    // Clear this segment before mixing
    const int numChans = mixBuffer.getNumChannels();
    for (int ch = 0; ch < numChans; ++ch)
    {
        float* dst = mixBuffer.getWritePointer(ch, segmentOffset);
        juce::FloatVectorOperations::clear(dst, segmentLength);
    }

    // Mix all tracks additively into this segment
    for (auto& track : tracks_)
    {
        if (track)
            track->processSegment(mixBuffer, segmentOffset, segmentLength, timelineSample);
    }
#else
    juce::ignoreUnused(mixBuffer, segmentOffset, segmentLength, timelineSample);
#endif
}

void Mixer::handleTransportEventRT(const TransportEvent& ev,
                                   int offsetInBlock,
                                   int64_t timelineSample)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    if (ev.trackIndex < 0 || ev.trackIndex >= static_cast<int>(tracks_.size()))
        return; // Invalid track index

    if (auto* track = tracks_[static_cast<size_t>(ev.trackIndex)].get())
        track->handleEventRT(ev, offsetInBlock, timelineSample);
#else
    juce::ignoreUnused(ev, offsetInBlock, timelineSample);
#endif
}

} // namespace zenith
