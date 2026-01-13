/**
 * @file TransportProtocolAgent.h
 * @brief Coordinator agent for network transport protocols
 * 
 * Thread Safety:
 * - serializeAudioPacket() can be called from AUDIO or BACKGROUND threads
 * - Network I/O happens on BACKGROUND threads
 * - All connection management is MESSAGE THREAD ONLY
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace zenith {

/**
 * @struct NetworkQualityMetrics
 * @brief Network performance statistics
 */
struct NetworkQualityMetrics {
    float latencyMs{0.0f};
    float jitterMs{0.0f};
    float packetLossPercent{0.0f};
    float bandwidthKbps{0.0f};
};

/**
 * @enum TransportProtocol
 * @brief Supported transport protocols
 */
enum class TransportProtocol {
    UDP,
    TCP,
    WebRTC
};

/**
 * @class TransportProtocolAgent
 * @brief Manages network transport for audio streaming
 * 
 * This agent handles low-level network transport, packet serialization,
 * and jitter buffering for real-time audio collaboration.
 */
class TransportProtocolAgent {
public:
    TransportProtocolAgent();
    ~TransportProtocolAgent();

    /**
     * @brief Initialize transport with protocol
     * @param protocol Transport protocol to use
     * @param sampleRate Audio sample rate
     * 
     * Thread: MESSAGE THREAD ONLY
     */
    void initialize(TransportProtocol protocol, double sampleRate);

    /**
     * @brief Serialize audio packet for transmission
     * @param audioData Audio buffer to serialize
     * @param numSamples Number of samples
     * @param numChannels Number of channels
     * @return Serialized packet data
     * 
     * Thread: AUDIO or BACKGROUND thread
     * Note: Uses pre-allocated buffers (RT-safe if from audio thread)
     */
    std::vector<uint8_t> serializeAudioPacket(const float* audioData,
                                               int numSamples,
                                               int numChannels);

    /**
     * @brief Deserialize received audio packet
     * @param packetData Received packet data
     * @param outputBuffer Output audio buffer
     * @param maxSamples Maximum samples to write
     * @return Number of samples written
     * 
     * Thread: BACKGROUND thread
     */
    int deserializeAudioPacket(const std::vector<uint8_t>& packetData,
                               float* outputBuffer,
                               int maxSamples);

    /**
     * @brief Get network quality metrics
     * @return Current network quality statistics
     * 
     * Thread: Any (uses atomics)
     */
    NetworkQualityMetrics getNetworkQuality() const;

    /**
     * @brief Send audio packet over network
     * @param packetData Serialized packet data
     * 
     * Thread: BACKGROUND thread
     */
    void sendPacket(const std::vector<uint8_t>& packetData);

private:
    std::atomic<float> latencyMs_{0.0f};
    std::atomic<float> jitterMs_{0.0f};
    std::atomic<float> packetLoss_{0.0f};
    std::atomic<bool> connected_{false};
    
    TransportProtocol protocol_{TransportProtocol::UDP};
    double sampleRate_{44100.0};
    
    // TODO: Add jitter buffer
    // TODO: Add packet queue
    // TODO: Add network socket abstraction
};

} // namespace zenith
