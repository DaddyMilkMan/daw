/**
 * @file Mixer.h
 * @brief Multi-track mixer with sample-accurate event handling
 *
 * Phase 1: Segment-based rendering with transport event routing
 * Routes events to tracks, renders track voices into master mix
 */

#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include <vector>
#include <memory>

// Forward declarations
class Engine;

namespace zenith {

/**
 * @brief Multi-track mixer
 *
 * Thread model:
 * - Track management: MESSAGE THREAD only (when !isPlaying)
 * - Audio processing: AUDIO THREAD only
 * - No shared mutable state between threads
 */
class Mixer
{
public:
    Mixer();
    ~Mixer();

    //==========================================================================
    // Track Management (MESSAGE THREAD ONLY)
    //==========================================================================

    /**
     * @brief Get number of tracks
     * @return Track count
     */
    int getNumTracks() const noexcept { return static_cast<int>(tracks_.size()); }

    /**
     * @brief Get track by index
     * @param index Track index (0-based)
     * @return Pointer to track, or nullptr if invalid index
     */
    Track* getTrack(int index) noexcept;
    const Track* getTrack(int index) const noexcept;

    /**
     * @brief Add empty track
     * @return Index of new track
     *
     * MUST be called when playback is stopped
     * MESSAGE THREAD only
     */
    int addTrack();

    /**
     * @brief Remove all tracks
     *
     * MUST be called when playback is stopped
     * MESSAGE THREAD only
     */
    void clearTracks();

    //==========================================================================
    // Audio Processing (AUDIO THREAD)
    //==========================================================================

    /**
     * @brief Process audio segment across all tracks
     * @param mixBuffer Destination buffer (cleared before mixing)
     * @param segmentOffset Offset into mixBuffer
     * @param segmentLength Number of samples to process
     * @param timelineSample Absolute timeline position
     *
     * AUDIO THREAD only - RT-safe
     * Clears the segment, then additively mixes all tracks
     */
    void processSegment(juce::AudioBuffer<float>& mixBuffer,
                        int segmentOffset,
                        int segmentLength,
                        int64_t timelineSample);

    /**
     * @brief Handle transport event (route to appropriate track)
     * @param ev Transport event
     * @param offsetInBlock Offset within current block
     * @param timelineSample Absolute timeline position
     *
     * AUDIO THREAD only - RT-safe
     * Routes event to track specified in ev.trackIndex
     */
    void handleTransportEventRT(const struct TransportEvent& ev,
                                int offsetInBlock,
                                int64_t timelineSample);

private:
    //==========================================================================
    // Member Variables
    //==========================================================================

    /// All tracks (MESSAGE THREAD creates, AUDIO THREAD reads)
    std::vector<std::unique_ptr<Track>> tracks_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Mixer)
};

} // namespace zenith
