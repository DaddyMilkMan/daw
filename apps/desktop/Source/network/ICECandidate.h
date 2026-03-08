#pragma once

#include <juce_core/juce_core.h>
#include <cstdint>
#include <array>

namespace zenith {

// ICE Candidate Types
enum class CandidateType {
    Host,           // Local interface IP
    ServerReflexive, // Public IP from STUN
    PeerReflexive,  // IP from peer connection
    Relayed         // Relay server (TURN)
};

// Network Transport
enum class TransportProtocol {
    UDP,
    TCP
};

// ICE Candidate for NAT traversal
struct ICECandidate {
    juce::String foundation;      // Unique identifier
    uint32_t componentId;         // 1 for RTP, 2 for RTCP
    TransportProtocol transport;  // UDP or TCP
    uint64_t priority;            // Candidate priority
    juce::String connectionAddress; // IP address
    int port;                     // Port number
    CandidateType type;           // Candidate type
    juce::String relatedAddress;  // For server reflexive/relayed
    int relatedPort;              // For server reflexive/relayed

    // Serialize to SDP-like format for signaling
    juce::String toString() const;

    // Parse from string
    static ICECandidate fromString(const juce::String& str);

    // Generate foundation string
    static juce::String generateFoundation(const juce::String& ip, int port, CandidateType type);

    // Calculate priority based on type and local preference
    static uint64_t calculatePriority(CandidateType type, int localPref = 65535);

    // Check if candidate is valid
    bool isValid() const { return port > 0 && connectionAddress.isNotEmpty(); }
};

// ICE Candidate Pair for connectivity checks
struct ICECandidatePair {
    ICECandidate local;
    ICECandidate remote;
    uint64_t priority;  // Combined priority

    // Connectivity check state (RFC 5245 5.7.4)
    enum State {
        Frozen = 0,      // Not yet checked
        Waiting = 1,     // Check sent, waiting for response
        InProgress = 2,  // Check in progress
        Succeeded = 3,   // Connectivity confirmed
        Failed = 4       // Check failed
    };
    State state = Frozen;

    bool nominated = false;  // Use for media

    // Transaction ID for STUN binding request
    std::array<uint8_t, 12> transactionId{};

    // Last check time
    juce::int64 lastCheckTime = 0;

    // Retry count
    int retryCount = 0;
    static constexpr int MAX_RETRIES = 3;

    // Calculate combined priority (RFC 5245 5.7.2)
    static uint64_t calculatePairPriority(
        const ICECandidate& local,
        const ICECandidate& remote,
        bool isControllingAgent
    );

    // Generate unique foundation for this pair
    juce::String getPairFoundation() const;
};

// ICE Agent Role (RFC 5245 5.7.1)
enum class ICERole {
    Controlling,   // Initiates connection, makes final selection
    Controlled     // Responds to checks
};

// ICE Agent State
class ICEAgent {
public:
    ICEAgent() : tieBreaker(generateTieBreaker()) {}

    ICERole role = ICERole::Controlling;
    uint64_t tieBreaker = 0;  // Random 64-bit value for conflict resolution

    // Generate random 64-bit tie-breaker
    static uint64_t generateTieBreaker() {
        juce::Random rng;
        uint64_t tb = 0;
        tb |= (uint64_t)rng.nextInt64();
        return tb;
    }

    // Compare tie-breakers to resolve conflicts
    // Returns true if this agent should be controlling
    bool shouldWinTieBreaker(uint64_t otherTieBreaker) const {
        return tieBreaker > otherTieBreaker;
    }
};

} // namespace zenith
