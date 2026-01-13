/**
 * @file ScheduledTransportAgent.h
 * @brief Coordinator agent for transport scheduling and timing
 * 
 * Thread Safety:
 * - advancePlayhead() is AUDIO THREAD ONLY (RT-safe)
 * - setTempo(), setTimeSignature() are MESSAGE THREAD ONLY
 * - getPlayheadPosition(), getCurrentTempo() use atomics (safe from any thread)
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace zenith {

/**
 * @struct TimeSignature
 * @brief Represents a musical time signature
 */
struct TimeSignature {
    int numerator{4};      // Beats per bar
    int denominator{4};    // Beat unit (4 = quarter note)
};

/**
 * @class ScheduledTransportAgent
 * @brief Manages transport timing, tempo, and scheduled events
 * 
 * This agent handles sample-accurate playhead advancement, tempo changes,
 * and event scheduling for the DAW transport system.
 */
class ScheduledTransportAgent {
public:
    ScheduledTransportAgent();
    ~ScheduledTransportAgent();

    /**
     * @brief Initialize transport with sample rate
     * @param sampleRate Audio device sample rate
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void initialize(double sampleRate);

    /**
     * @brief Advance playhead by number of samples (RT-safe)
     * @param numSamples Number of samples to advance
     * @return New playhead position in samples
     * 
     * Thread: AUDIO THREAD ONLY
     * RT-Safe: Yes
     */
    int64_t advancePlayhead(int numSamples);

    /**
     * @brief Set tempo in BPM
     * @param bpm Beats per minute
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void setTempo(double bpm);

    /**
     * @brief Get current tempo
     * @return Tempo in BPM
     * 
     * Thread: Any (uses atomics)
     */
    double getCurrentTempo() const;

    /**
     * @brief Set time signature
     * @param numerator Beats per bar
     * @param denominator Beat unit
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void setTimeSignature(int numerator, int denominator);

    /**
     * @brief Get playhead position in samples
     * @return Current playhead position
     * 
     * Thread: Any (uses atomics)
     */
    int64_t getPlayheadPosition() const;

    /**
     * @brief Set loop region
     * @param startSamples Loop start position
     * @param endSamples Loop end position
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void setLoopRegion(int64_t startSamples, int64_t endSamples);

private:
    std::atomic<int64_t> playheadSamples_{0};
    std::atomic<double> currentTempoBpm_{120.0};
    std::atomic<bool> isLooping_{false};
    
    double sampleRate_{44100.0};
    TimeSignature timeSignature_;
    int64_t loopStart_{0};
    int64_t loopEnd_{0};
    
    // TODO: Add tempo automation map
    // TODO: Add scheduled event queue
    // TODO: Add timecode synchronization
};

} // namespace zenith
