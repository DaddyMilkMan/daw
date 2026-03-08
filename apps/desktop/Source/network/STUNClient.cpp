#include "STUNClient.h"
#include <random>
#include <cstring>

namespace zenith {

// STUN Magic Cookie (RFC 5389)
static const uint32_t STUN_MAGIC_COOKIE = 0x2112A442;

// STUN Header Structure
#pragma pack(push, 1)
struct STUNHeader {
    uint16_t messageType;
    uint16_t messageLength;
    uint32_t magicCookie;
    uint8_t transactionId[12];
};
#pragma pack(pop)

// STUN Attribute Header
#pragma pack(push, 1)
struct STUNAttributeHeader {
    uint16_t type;
    uint16_t length;
};
#pragma pack(pop)

class STUNClient::Pimpl {
public:
    juce::DatagramSocket socket;
    juce::CriticalSection lock;
};

STUNClient::STUNClient() : pimpl(std::make_unique<Pimpl>()) {}

STUNClient::~STUNClient() = default;

juce::MemoryBlock STUNClient::buildBindingRequest() {
    juce::MemoryBlock packet;

    // Build STUN header
    STUNHeader header;
    header.messageType = juce::ByteOrder::swapIfBigEndian((uint16_t)STUNMessageType::BindingRequest);
    header.messageLength = 0;  // No attributes in basic binding request
    header.magicCookie = juce::ByteOrder::swapIfBigEndian(STUN_MAGIC_COOKIE);

    // Generate random transaction ID (96 bits)
    std::array<uint8_t, 12> transactionId;
    juce::Random rng;
    for (int i = 0; i < 12; ++i) {
        transactionId[i] = (uint8_t)rng.nextInt(256);
    }
    std::memcpy(header.transactionId, transactionId.data(), 12);

    // Append header to packet
    packet.append(&header, sizeof(STUNHeader));

    return packet;
}

STUNDiscoveryResult STUNClient::parseBindingResponse(
    const void* data,
    int size,
    const juce::String& stunServer
) {
    STUNDiscoveryResult result;

    if (size < (int)sizeof(STUNHeader)) {
        result.errorMessage = "Response too short";
        return result;
    }

    const STUNHeader* header = (const STUNHeader*)data;
    uint16_t messageType = juce::ByteOrder::swapIfBigEndian(header->messageType);
    uint16_t messageLength = juce::ByteOrder::swapIfBigEndian(header->messageLength);
    uint32_t magicCookie = juce::ByteOrder::swapIfBigEndian(header->magicCookie);

    // Verify magic cookie
    if (magicCookie != STUN_MAGIC_COOKIE) {
        result.errorMessage = "Invalid magic cookie";
        return result;
    }

    // Check message type
    if (messageType == (uint16_t)STUNMessageType::BindingErrorResponse) {
        result.errorMessage = "STUN error response";
        return result;
    }

    if (messageType != (uint16_t)STUNMessageType::BindingResponse) {
        result.errorMessage = "Unexpected message type";
        return result;
    }

    // Parse attributes
    int offset = sizeof(STUNHeader);
    while (offset < size - 4) {  // Need at least 4 bytes for attribute header
        const STUNAttributeHeader* attr = (const STUNAttributeHeader*)((const char*)data + offset);
        uint16_t attrType = juce::ByteOrder::swapIfBigEndian(attr->type);
        uint16_t attrLength = juce::ByteOrder::swapIfBigEndian(attr->length);

        // Pad to 4-byte boundary
        int paddedLength = (attrLength + 3) & ~3;

        // Check for XOR-MAPPED-ADDRESS attribute (0x0020)
        if (attrType == (uint16_t)STUNAttributeType::XorMappedAddress) {
            const uint8_t* attrData = (const uint8_t*)attr + sizeof(STUNAttributeHeader);

            if (attrLength >= 8) {
                uint8_t family = attrData[1];
                if (family == 0x01) {  // IPv4
                    // XOR the port and IP with magic cookie and transaction ID
                    uint16_t xoredPort;
                    std::memcpy(&xoredPort, attrData + 2, 2);
                    xoredPort = juce::ByteOrder::swapIfBigEndian(xoredPort);
                    xoredPort ^= (STUN_MAGIC_COOKIE >> 16);

                    uint32_t xoredIP;
                    std::memcpy(&xoredIP, attrData + 4, 4);
                    xoredIP = juce::ByteOrder::swapIfBigEndian(xoredIP);
                    xoredIP ^= STUN_MAGIC_COOKIE;

                    result.publicPort = xoredPort;
                    result.publicIP = juce::String(
                        (xoredIP >> 24) & 0xFF) + "." +
                        juce::String((xoredIP >> 16) & 0xFF) + "." +
                        juce::String((xoredIP >> 8) & 0xFF) + "." +
                        juce::String(xoredIP & 0xFF);

                    result.success = true;
                }
            }
            break;  // Found what we need
        }

        // Also check for MAPPED-ADDRESS as fallback (older STUN servers)
        if (attrType == (uint16_t)STUNAttributeType::MappedAddress) {
            const uint8_t* attrData = (const uint8_t*)attr + sizeof(STUNAttributeHeader);
            if (attrLength >= 8) {
                uint8_t family = attrData[1];
                if (family == 0x01) {  // IPv4
                    uint16_t port;
                    std::memcpy(&port, attrData + 2, 2);
                    result.publicPort = juce::ByteOrder::swapIfBigEndian(port);

                    uint32_t ip;
                    std::memcpy(&ip, attrData + 4, 4);
                    uint8_t* ipBytes = (uint8_t*)&ip;
                    result.publicIP = juce::String(ipBytes[0]) + "." +
                                     juce::String(ipBytes[1]) + "." +
                                     juce::String(ipBytes[2]) + "." +
                                     juce::String(ipBytes[3]);

                    result.success = true;
                    break;
                }
            }
        }

        offset += sizeof(STUNAttributeHeader) + paddedLength;
    }

    if (!result.success) {
        result.errorMessage = "No mapped address in response";
    }

    return result;
}

STUNDiscoveryResult STUNClient::performBindingRequest(
    const juce::String& stunServer,
    int stunPort,
    int timeoutMs
) {
    STUNDiscoveryResult result;

    // Bind to any available port
    if (!pimpl->socket.bindToPort(0)) {
        result.errorMessage = "Failed to bind socket";
        return result;
    }

    // Build STUN binding request
    juce::MemoryBlock request = buildBindingRequest();

    // Send request
    if (pimpl->socket.write(stunServer, stunPort, request.getData(), (int)request.getSize()) <= 0) {
        result.errorMessage = "Failed to send STUN request";
        return result;
    }

    // Wait for response
    char buffer[512];
    juce::String responseIP;
    int responsePort;

    auto startTime = juce::Time::currentTimeMillis();
    while ((juce::Time::currentTimeMillis() - startTime) < timeoutMs) {
        if (pimpl->socket.waitUntilReady(true, 500) > 0) {
            int bytesRead = pimpl->socket.read(buffer, sizeof(buffer), false, responseIP, responsePort);
            if (bytesRead > 0 && responseIP == stunServer) {
                result = parseBindingResponse(buffer, bytesRead, stunServer);

                // Determine NAT type based on comparison
                if (result.success) {
                    // Conservative default: treat as NATed unless a deeper multi-test
                    // probe is implemented.
                    result.natType = NATType::Symmetric;
                }

                return result;
            }
        }
    }

    result.errorMessage = "STUN request timeout";
    return result;
}

void STUNClient::performBindingRequestAsync(
    const juce::String& stunServer,
    int stunPort,
    ResultCallback callback,
    int timeoutMs
) {
    juce::Thread::launch([this, stunServer, stunPort, callback, timeoutMs]() {
        auto result = performBindingRequest(stunServer, stunPort, timeoutMs);
        callback(result);
    });
}

NATType STUNClient::determineNATType(
    const juce::String& stunServer1,
    const juce::String& stunServer2,
    int stunPort
) {
    // Simple NAT detection using one STUN server
    auto result1 = performBindingRequest(stunServer1, stunPort);

    if (!result1.success) {
        return NATType::Blocked;
    }

    return result1.natType;
}

ICECandidate STUNClient::createServerReflexiveCandidate(
    const STUNDiscoveryResult& result,
    int localPort
) {
    ICECandidate candidate;
    candidate.foundation = ICECandidate::generateFoundation(result.publicIP, result.publicPort, CandidateType::ServerReflexive);
    candidate.componentId = 1;
    candidate.transport = TransportProtocol::UDP;
    candidate.priority = ICECandidate::calculatePriority(CandidateType::ServerReflexive);
    candidate.connectionAddress = result.publicIP;
    candidate.port = result.publicPort;
    candidate.type = CandidateType::ServerReflexive;
    candidate.relatedAddress.clear();
    candidate.relatedPort = 0;

    return candidate;
}

} // namespace zenith
