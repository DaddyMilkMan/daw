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
    struct DiscoveredSession {
        juce::String id;
        juce::String name;
        juce::String host;
        int port;
        int currentUsers;
        int maxUsers;
        bool hasPassword;
        juce::String description;
        juce::Time discovered;
    };

    SessionDiscovery();
    ~SessionDiscovery() = default;

    // Discovery
    void startDiscovery();
    void stopDiscovery();
    bool isDiscovering() const;

    // Sessions
    std::vector<DiscoveredSession> getDiscoveredSessions() const;
    void clearDiscoveredSessions();

    // Broadcasting (for hosts)
    void startBroadcasting(const SessionConfig& config);
    void stopBroadcasting();
    bool isBroadcasting() const;

    // Listeners

} // namespace
