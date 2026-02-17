/*
  ==============================================================================
    CollaborativeSession.h
    Real-time collaborative editing system
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace zenith {
namespace collaboration {

// User information
    struct Listener {
        virtual ~Listener() = default;
        virtual void sessionDiscovered(const DiscoveredSession& session) {}
        virtual void sessionLost(const DiscoveredSession& session) {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    bool isCurrentlyDiscovering = false;
    bool isCurrentlyBroadcasting = false;

    std::vector<DiscoveredSession> discoveredSessions;
    std::mutex sessionsMutex;

    std::unique_ptr<juce::DatagramSocket> discoverySocket;
    std::unique_ptr<juce::DatagramSocket> broadcastSocket;

    std::vector<Listener*> listeners;

    static constexpr int DISCOVERY_PORT = 19000;
    static constexpr int BROADCAST_INTERVAL = 5000;  // 5 seconds

    void sendDiscoveryRequest();
    void sendBroadcastMessage();
    void handleDiscoveryResponse(const juce::MemoryBlock& data, const juce::String& senderAddress);
    void handleBroadcastMessage(const juce::MemoryBlock& data, const juce::String& senderAddress);

    void notifySessionDiscovered(const DiscoveredSession& session);
    void notifySessionLost(const DiscoveredSession& session);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionDiscovery)
};

} // namespace collaboration
} // namespace zenith
