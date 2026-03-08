#pragma once

#include "ICECandidate.h"
#include <juce_core/juce_core.h>
#include <functional>

namespace zenith {

// STUN Message Types (RFC 5389)
enum class STUNMessageType : uint16_t {
    BindingRequest = 0x0001,
    BindingResponse = 0x0101,
    BindingErrorResponse = 0x0111,
    SharedSecretRequest = 0x0002,
    SharedSecretResponse = 0x0102,
    SharedSecretErrorResponse = 0x0112
};

// STUN Attributes
enum class STUNAttributeType : uint16_t {
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
    Fingerprint = 0x8028
};

// NAT Types discovered through STUN
enum class NATType {
    OpenInternet,        // No NAT
    FullCone,            // Full cone NAT
    RestrictedCone,      // Restricted cone NAT
    PortRestrictedCone,  // Port restricted cone NAT
    Symmetric,           // Symmetric NAT (hardest to traverse)
    Blocked,             // Firewall blocking UDP
    Unknown
};

// STUN Discovery Result
struct STUNDiscoveryResult {
    bool success = false;
    NATType natType = NATType::Unknown;
    juce::String publicIP;
    int publicPort = 0;
    juce::String changedIP;   // Changed address from STUN server
    int changedPort = 0;
    juce::String errorMessage;
};

// STUN Client for NAT discovery
class STUNClient {
public:
    STUNClient();
    ~STUNClient();

    // Synchronous STUN binding request to discover public IP and NAT type
    STUNDiscoveryResult performBindingRequest(
        const juce::String& stunServer,
        int stunPort = 3478,
        int timeoutMs = 3000
    );

    // Asynchronous STUN binding request
    using ResultCallback = std::function<void(STUNDiscoveryResult)>;
    void performBindingRequestAsync(
        const juce::String& stunServer,
        int stunPort,
        ResultCallback callback,
        int timeoutMs = 3000
    );

    // Determine NAT type by running multiple STUN tests
    NATType determineNATType(
        const juce::String& stunServer1,
        const juce::String& stunServer2,
        int stunPort = 3478
    );

    // Get server reflexive candidate from STUN result
    static ICECandidate createServerReflexiveCandidate(
        const STUNDiscoveryResult& result,
        int localPort
    );

    // Default STUN servers (Google, Twilio, etc.)
    static juce::StringArray getDefaultSTUNServers() {
        return {
            "stun.l.google.com:19302",
            "stun1.l.google.com:19302",
            "stun2.l.google.com:19302",
            "global.stun.twilio.com:3478"
        };
    }

    // Helper to build STUN binding request (made public for ICE connectivity checks)
    static juce::MemoryBlock buildBindingRequest();

    // Helper to parse STUN response (made public for ICE connectivity checks)
    static STUNDiscoveryResult parseBindingResponse(
        const void* data,
        int size,
        const juce::String& stunServer = ""
    );

private:
    class Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    // Extract XOR-mapped address from attribute
    static bool extractXorMappedAddress(
        const void* attributeData,
        int attributeSize,
        const void* magicCookieAndTransactionId,
        juce::String& outIP,
        int& outPort
    );

    juce::Random random;
};

} // namespace zenith
