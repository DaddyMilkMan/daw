#pragma once

#include "STUNClient.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <memory>

namespace zenith {

// TURN Methods (RFC 5766)
enum class TURNMethod : uint16_t {
    Allocate = 0x0003,
    Refresh = 0x0004,
    Send = 0x0006,
    Data = 0x0007,
    CreatePermission = 0x0008,
    ChannelBind = 0x0009
};

// TURN Attributes (RFC 5766)
enum class TURNAttributeType : uint16_t {
    // STUN attributes (inherited)
    MappedAddress = 0x0001,
    ResponseAddress = 0x0002,
    ChangeRequest = 0x0003,
    SourceAddress = 0x0004,
    ChangedAddress = 0x0005,
    Username = 0x0006,
    Password = 0x0007,
    MessageIntegrity = 0x0008,
    ErrorCode = 0x0009,
    UnknownAttributes = 0x000a,
    ReflectedFrom = 0x000b,
    XorMappedAddress = 0x0020,
    Software = 0x8022,
    AlternateServer = 0x8023,
    Fingerprint = 0x8028,

    // TURN-specific attributes
    ChannelNumber = 0x000C,
    Lifetime = 0x000D,
    XORPeerAddress = 0x0012,
    Data = 0x0013,
    XORRelayedAddress = 0x0016,
    EvenPort = 0x0018,
    RequestedTransport = 0x0019,
    DontFragment = 0x001A,
    ReservationToken = 0x0022,
    Priority = 0x0024,
    ICEControlled = 0x8029,
    ICEControlling = 0x802A
};

// TURN Channel Data
enum class TURNChannelNumber : uint16_t {
    ChannelData = 0x4000,
    MinChannel = 0x4000,
    MaxChannel = 0x7FFF
};

// TURN Allocation Result
struct TURNAllocationResult {
    bool success = false;
    juce::String relayedAddress;  // Public IP on TURN server
    int relayedPort = 0;
    juce::String serverAddress;   // TURN server IP
    int serverPort = 0;
    uint32_t lifetime = 0;        // Allocation lifetime in seconds
    uint32_t transactionID = 0;   // Transaction ID for refresh
    juce::String errorMessage;

    // Authentication info for refresh
    juce::String username;
    juce::String password;
    juce::String realm;
    juce::String nonce;
};

// TURN Client for relay allocation
class TURNClient {
public:
    TURNClient();
    ~TURNClient();

    // Allocate relay address from TURN server
    TURNAllocationResult allocateRelay(
        const juce::String& turnServer,
        int turnPort,
        const juce::String& username,
        const juce::String& password,
        int timeoutMs = 5000
    );

    // Asynchronous allocation
    using AllocationCallback = std::function<void(TURNAllocationResult)>;
    void allocateRelayAsync(
        const juce::String& turnServer,
        int turnPort,
        const juce::String& username,
        const juce::String& password,
        AllocationCallback callback,
        int timeoutMs = 5000
    );

    // Refresh allocation to extend lifetime
    bool refreshAllocation(
        const TURNAllocationResult& allocation,
        const juce::String& username,
        const juce::String& password
    );

    // Deallocate relay
    bool deallocate(
        const TURNAllocationResult& allocation,
        const juce::String& username,
        const juce::String& password
    );

    // Create permission for peer to send data
    bool createPermission(
        const TURNAllocationResult& allocation,
        const juce::String& peerIP,
        int peerPort,
        const juce::String& username,
        const juce::String& password
    );

    // Send data to peer via TURN server
    bool sendData(
        const TURNAllocationResult& allocation,
        const juce::String& peerIP,
        int peerPort,
        const void* data,
        int size,
        const juce::String& username,
        const juce::String& password
    );

    // Channel data operations (more efficient)
    bool bindChannel(
        const TURNAllocationResult& allocation,
        const juce::String& peerIP,
        int peerPort,
        uint16_t channelNumber,
        const juce::String& username,
        const juce::String& password
    );

    // Convert TURN allocation to ICE candidate
    static ICECandidate createRelayedCandidate(
        const TURNAllocationResult& allocation,
        int localPort
    );

    // Default TURN servers (public TURN services)
    static juce::StringArray getDefaultTURNServers() {
        return {
            // You should deploy your own TURN server for production
            // For testing, you can use:
            // - coturn (open source)
            // - restund (open source)
            // - Twilio Network Traversal Service (paid)
            "turn.your-server.com:3478"  // Replace with your TURN server
        };
    }

private:
    class Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    // Build TURN allocate request
    juce::MemoryBlock buildAllocateRequest(
        const juce::String& username,
        uint32_t lifetime = 600  // 10 minutes default
    );

    // Build TURN refresh request
    juce::MemoryBlock buildRefreshRequest(
        const TURNAllocationResult& allocation,
        uint32_t lifetime
    );

    // Parse TURN allocate response
    TURNAllocationResult parseAllocateResponse(
        const void* data,
        int size,
        const juce::String& turnServer
    );

    // Add message integrity (HMAC-SHA1)
    bool addMessageIntegrity(
        juce::MemoryBlock& packet,
        const juce::String& username,
        const juce::String& password,
        const juce::String& realm,
        const juce::String& nonce
    );

    // Calculate HMAC-SHA1 for TURN authentication
    static juce::MemoryBlock calculateHMACSHA1(
        const juce::String& key,
        const void* data,
        int size
    );

    juce::Random random;
    juce::DatagramSocket socket;
};

} // namespace zenith
