/*
  ==============================================================================
    TransportProtocolAgent.h
    Coordinator agent for transport protocol implementation and management
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>
#include <vector>

namespace zenith {
namespace agents {

/**
 * @class TransportProtocolAgent
 * 
 * Manages transport protocol implementations (MIDI clock, MTC, LTC, etc.).
 * Ensures reliable sync signal handling and protocol compatibility.
 * 
 * Thread Safety:
 * - Protocol configuration is MESSAGE THREAD ONLY
 * - Signal processing may occur on audio or dedicated MIDI threads
 * - Uses atomics for protocol state
 */
class TransportProtocolAgent
{
public:
    TransportProtocolAgent();
    ~TransportProtocolAgent();

    // Protocol management
    enum class Protocol {
        None,
        MIDIClock,
        MTC,
        LTC,
        AbletonLink,
        OSC
    };

    void enableProtocol(Protocol protocol);
    void disableProtocol(Protocol protocol);
    bool isProtocolEnabled(Protocol protocol) const;

    // Protocol analysis
    struct ProtocolStatus {
        Protocol protocol;
        bool active = false;
        double signalQuality = 0.0;
        int errorCount = 0;
        juce::String statusMessage;
    };

    std::vector<ProtocolStatus> getAllProtocolStatuses();
    ProtocolStatus getProtocolStatus(Protocol protocol);

    // Signal quality monitoring
    void startMonitoring();
    void stopMonitoring();

private:
    std::atomic<bool> isMonitoring_{false};
    std::atomic<int> enabledProtocols_{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportProtocolAgent)
};

} // namespace agents
} // namespace zenith
