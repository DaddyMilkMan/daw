/**
 * @file TempoMap.h
 * @brief RT-safe tempo map for beat <-> sample conversion
 *
 * Phase 15: Tempo Map + Markers v1
 *
 * The TempoMap class provides immutable, real-time safe conversion between
 * beats and samples. It stores a precomputed sequence of tempo segments
 * and uses binary search for efficient lookups.
 *
 * RT-Safety:
 * - All data is allocated at construction time
 * - Query methods (samplesToBeats, beatsToSamples) use only const operations
 * - No allocations, locks, or system calls in query methods
 * - Designed to be used via std::shared_ptr with atomic pointer swap
 */

#pragma once

#include <juce_core/juce_core.h>
#include <vector>

/**
 * @struct TimeSignature
 * @brief Time signature specification
 */
struct TimeSignature
{
    int numerator{4};
    int denominator{4};
};

/**
 * @class TempoMap
 * @brief Immutable tempo map for RT-safe beat/sample conversion
 *
 * Thread safety:
 * - Const after construction (all data is const or mutable for caching)
 * - Safe to read from audio thread if constructed on message thread
 * - Use std::atomic<std::shared_ptr<const TempoMap>> for thread-safe updates
 */
class TempoMap
{
public:
    /**
     * @struct TempoSegment
     * @brief Represents a tempo segment with precomputed values
     */
    struct TempoSegment
    {
        double startBeats{0.0};         // Start position in beats
        double bpm{120.0};               // Tempo in BPM
        juce::int64 startSamples{0};    // Start position in samples
        TimeSignature timeSig;           // Time signature

        // Precomputed conversion factors
        double secondsPerBeat{0.5};      // = 60.0 / bpm
        double samplesPerBeat{0.0};      // = sampleRate * secondsPerBeat
    };

    //==========================================================================
    /**
     * @brief Construct empty tempo map (120 BPM at 44100 Hz)
     */
    TempoMap();

    /**
     * @brief Construct tempo map from tempo point specifications
     * @param tempoPoints Array of tempo points (must include point at beat 0)
     * @param sampleRate Sample rate in Hz
     *
     * The tempo points will be sorted by time and precomputed for RT-safe access.
     */
    struct TempoPoint
    {
        double timeBeats;
        double bpm;
        int timeSigNum;
        int timeSigDen;
    };

    TempoMap(const std::vector<TempoPoint>& tempoPoints, double sampleRate);

    //==========================================================================
    // Conversion Methods (RT-SAFE)
    //==========================================================================

    /**
     * @brief Convert sample position to beats
     * @param samplePos Sample position
     * @return Position in beats
     * @note RT-SAFE: No allocations, no locks
     */
    double samplesToBeats(juce::int64 samplePos) const;

    /**
     * @brief Convert beats to sample position
     * @param beats Position in beats
     * @return Sample position
     * @note RT-SAFE: No allocations, no locks
     */
    juce::int64 beatsToSamples(double beats) const;

    /**
     * @brief Get tempo at beat position
     * @param beats Position in beats
     * @return Tempo in BPM
     * @note RT-SAFE: No allocations, no locks
     */
    double getTempoAtBeats(double beats) const;

    /**
     * @brief Get time signature at beat position
     * @param beats Position in beats
     * @return Time signature
     * @note RT-SAFE: No allocations, no locks
     */
    TimeSignature getTimeSignatureAtBeats(double beats) const;

    /**
     * @brief Get sample rate
     */
    double getSampleRate() const { return sampleRate_; }

    /**
     * @brief Get number of segments
     */
    int getNumSegments() const { return static_cast<int>(segments_.size()); }

private:
    //==========================================================================
    // Helper Methods
    //==========================================================================

    /**
     * @brief Find segment index for given beat position
     * @param beats Beat position
     * @return Segment index (or last segment if beats > end)
     * @note RT-SAFE: Binary search, no allocations
     */
    int findSegmentForBeats(double beats) const;

    /**
     * @brief Find segment index for given sample position
     * @param samplePos Sample position
     * @return Segment index (or last segment if samplePos > end)
     * @note RT-SAFE: Binary search, no allocations
     */
    int findSegmentForSamples(juce::int64 samplePos) const;

    //==========================================================================
    // Member Variables
    //==========================================================================

    double sampleRate_{44100.0};
    std::vector<TempoSegment> segments_;  // Allocated at construction, const after

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoMap)
};
