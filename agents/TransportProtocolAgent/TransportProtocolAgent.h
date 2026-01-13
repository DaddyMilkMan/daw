/**
 * @file TransportProtocolAgent.h
 * @brief Agent for managing transport synchronization protocols
 * 
 * Thread Safety:
 * - Configuration methods are MESSAGE THREAD ONLY
 * - generateMidiClock() can be called from AUDIO THREAD (RT-safe)
 * - Protocol parsing methods are RT-safe
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

namespace zenith {

/**
 * @enum SyncProtocol
 * @brief Supported synchronization protocols
 */
enum class SyncProtocol {
    None,
    MidiClock,
    MTC,
    MMC,
    Ableton_Link
};

/**
 * @struct MidiClockEvent
 * @brief Represents a MIDI clock event
 */
struct MidiClockEvent {
    uint8_t status;    // MIDI status byte
    int64_t samplePosition;
};

/**
 * @class TransportProtocolAgent
 * @brief Manages transport synchronization protocols (MIDI Clock, MTC, MMC)
 */
class TransportProtocolAgent {
public:
    TransportProtocolAgent();
    ~TransportProtocolAgent();

    /**
     * Initialize the agent
     * MESSAGE THREAD ONLY
     */
    void initialize(double sampleRate, double initialTempo);

    /**
     * Enable or disable a sync protocol
     * MESSAGE THREAD ONLY
     * 
     * @param protocol Protocol to configure
     * @param enabled Whether to enable the protocol
     */
    void setProtocolEnabled(SyncProtocol protocol, bool enabled);

    /**
     * Generate MIDI clock messages for current buffer
     * AUDIO THREAD SAFE - RT-safe, lock-free
     * 
     * @param bufferSize Number of samples in current buffer
     * @param currentSamplePosition Current playhead position
     * @param outEvents Output vector for MIDI clock events
     * @return Number of clock events generated
     */
    int generateMidiClock(int bufferSize, int64_t currentSamplePosition,
                         std::vector<MidiClockEvent>& outEvents);

    /**
     * Update tempo for clock generation
     * MESSAGE THREAD ONLY (or use atomic for RT-safe updates)
     * 
     * @param bpm New tempo in beats per minute
     */
    void setTempo(double bpm);

    /**
     * Get current sync protocol status
     * Thread-safe via atomic
     */
    bool isProtocolEnabled(SyncProtocol protocol) const;

private:
    double sampleRate_{44100.0};
    double tempo_{120.0};
    
    // Clock generation state
    double samplesPerClock_{0.0};
    int64_t nextClockSample_{0};
    
    // Atomic protocol enable flags
    std::atomic<bool> midiClockEnabled_{false};
    std::atomic<bool> mtcEnabled_{false};
    std::atomic<bool> mmcEnabled_{false};
    
    void calculateClockTiming();
};

} // namespace zenith
