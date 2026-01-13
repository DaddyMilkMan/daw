/**
 * @file TransportProtocolAgent.h
 * @brief Coordinator agent for transport protocol handling
 * 
 * Thread Safety:
 * - Protocol encoding/decoding can be called from BACKGROUND THREADS
 * - State updates must be posted to MESSAGE THREAD
 * - Uses lock-free queues for inter-thread communication
 */

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <vector>

namespace zenith {

/**
 * @class TransportProtocolAgent
 * @brief Handles transport protocol encoding/decoding for network sync
 */
class TransportProtocolAgent {
public:
    enum class ProtocolType {
        MidiClock,
        MTC,
        CustomSync
    };

    TransportProtocolAgent();
    ~TransportProtocolAgent();

    /**
     * Initialize the agent
     */
    void initialize();

    /**
     * Encode transport state to protocol message
     * @param protocolType Type of protocol to encode
     * @param position Current transport position in samples
     * @param isPlaying Whether transport is playing
     * @return Encoded message as byte array
     */
    std::vector<uint8_t> encodeTransportState(
        ProtocolType protocolType,
        juce::int64 position,
        bool isPlaying);

    /**
     * Decode protocol message to transport state
     * @param protocolType Type of protocol to decode
     * @param messageData Encoded message bytes
     * @return true if decoding was successful
     */
    bool decodeTransportMessage(
        ProtocolType protocolType,
        const std::vector<uint8_t>& messageData);

    /**
     * Get supported protocol types
     * @return Vector of supported protocol types
     */
    std::vector<ProtocolType> getSupportedProtocols() const;

private:
    std::atomic<bool> initialized_{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportProtocolAgent)
};

} // namespace zenith
