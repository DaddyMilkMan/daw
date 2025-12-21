/**
 * @file TempoMap.h
 * @brief RT-safe tempo map for beat/time conversions
 *
 * Phase 15: Tempo Map & Global Markers MVP
 *
 * Provides beat↔time/sample conversions using a variable tempo map.
 * Uses a snapshot pattern for RT-safety:
 * - Message thread builds TempoMapSnapshot from ProjectState
 * - Audio thread reads snapshot via atomic shared_ptr (lock-free)
 *
 * Thread Safety:
 * - updateFromValueTree() runs on MESSAGE THREAD
 * - All conversion methods (beatsToSeconds, etc.) are RT-SAFE
 * - Uses atomic shared_ptr swap for lock-free read access
 */

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <memory>
#include <vector>

namespace zenith {

//==============================================================================
/**
 * @struct TempoPoint
 * @brief Single tempo change point
 */
struct TempoPoint
{
    double timeBeats;    // Position in beats from session start
    double bpm;          // Tempo at/after this point

    TempoPoint(double beats, double tempoBpm)
        : timeBeats(beats), bpm(tempoBpm) {}
};

//==============================================================================
/**
 * @struct TempoMapSnapshot
 * @brief Immutable snapshot of tempo map data
 *
 * This structure is built on the message thread and shared to the audio thread.
 * Once created, it is never modified (immutable).
 */
struct TempoMapSnapshot
{
    std::vector<TempoPoint> points;  // Sorted by timeBeats

    TempoMapSnapshot() = default;

    // Pre-calculate cumulative time for each tempo point for fast lookups
    void prepare();

    struct CachedPoint
    {
        double timeBeats;
        double bpm;
        double cumulativeSeconds;  // Total time from start to this point
    };

    std::vector<CachedPoint> cachedPoints;
};

//==============================================================================
/**
 * @class TempoMap
 * @brief RT-safe tempo map for beat/time conversions
 *
 * This class provides beat↔time/sample conversions based on a variable tempo map.
 * It uses an atomic shared_ptr to allow lock-free reads from the audio thread.
 */
class TempoMap
{
public:
    //==========================================================================
    TempoMap();
    ~TempoMap();

    //==========================================================================
    // Message Thread API
    //==========================================================================

    /**
     * @brief Update tempo map from ValueTree
     * @param tempoMapTree ValueTree containing TEMPO_POINT children
     * @note MESSAGE THREAD ONLY
     */
    void updateFromValueTree(const juce::ValueTree& tempoMapTree);

    /**
     * @brief Set a single static tempo
     * @param bpm Tempo in BPM
     * @note MESSAGE THREAD ONLY
     */
    void setSingleTempo(double bpm) [[maybe_unused]];

    //==========================================================================
    // RT-Safe Conversion API (Audio Thread Safe)
    //==========================================================================

    /**
     * @brief Convert beats to seconds
     * @param beats Time in beats
     * @param sampleRate Current sample rate (informational, not used in this method)
     * @return Time in seconds
     * @note RT-SAFE (audio thread safe)
     */
    double beatsToSeconds(double beats, double sampleRate) const;

    /**
     * @brief Convert seconds to beats
     * @param seconds Time in seconds
     * @param sampleRate Current sample rate (informational, not used in this method)
     * @return Time in beats
     * @note RT-SAFE (audio thread safe)
     */
    double secondsToBeats(double seconds, double sampleRate) const;

    /**
     * @brief Convert beats to samples
     * @param beats Time in beats
     * @param sampleRate Current sample rate
     * @return Time in samples
     * @note RT-SAFE (audio thread safe)
     */
    int64_t beatsToSamples(double beats, double sampleRate) const;

    /**
     * @brief Convert samples to beats
     * @param samples Time in samples
     * @param sampleRate Current sample rate
     * @return Time in beats
     * @note RT-SAFE (audio thread safe)
     */
    double samplesToBeats(int64_t samples, double sampleRate) const;

    /**
     * @brief Get current tempo at a given beat position
     * @param beats Time in beats
     * @return Tempo in BPM
     * @note RT-SAFE (audio thread safe)
     */
    double getTempoAt(double beats) const;

private:
    //==========================================================================
    // Atomic shared_ptr for lock-free access from audio thread
    //==========================================================================

    // Current snapshot (read by audio thread, swapped by message thread)
    std::shared_ptr<const TempoMapSnapshot> snapshot_;

    // Spinlock to protect snapshot_ access
    mutable juce::SpinLock snapshotLock_;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Swap snapshot atomically
     * @param newSnapshot New snapshot to install
     * @note MESSAGE THREAD ONLY
     */
    void swapSnapshot(std::shared_ptr<const TempoMapSnapshot> newSnapshot);

    /**
     * @brief Load current snapshot for reading
     * @return Shared pointer to current snapshot
     * @note RT-SAFE (audio thread safe)
     */
    std::shared_ptr<const TempoMapSnapshot> loadSnapshot() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoMap)
};

} // namespace zenith

