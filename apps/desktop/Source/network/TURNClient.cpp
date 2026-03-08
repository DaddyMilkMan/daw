#include "TURNClient.h"
#include <juce_cryptography/juce_cryptography.h>
#include <cstring>

namespace zenith {

// STUN/TURN common definitions
static const uint32_t STUN_MAGIC_COOKIE = 0x2112A442;

#pragma pack(push, 1)
struct STUNHeader {
    uint16_t messageType;
    uint16_t messageLength;
    uint32_t magicCookie;
    uint8_t transactionId[12];
};

struct STUNAttributeHeader {
    uint16_t type;
    uint16_t length;
};
#pragma pack(pop)

class TURNClient::Pimpl {
public:
    juce::CriticalSection lock;
    juce::DatagramSocket socket;
};

TURNClient::TURNClient() : pimpl(std::make_unique<Pimpl>()) {}
TURNClient::~TURNClient() = default;

juce::MemoryBlock TURNClient::buildAllocateRequest(
    const juce::String& username,
    uint32_t lifetime
) {
    juce::MemoryBlock packet;

    // Build STUN header with Allocate method
    STUNHeader header;
    header.messageType = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNMethod::Allocate);
    header.magicCookie = juce::ByteOrder::swapIfBigEndian(STUN_MAGIC_COOKIE);

    // Generate random transaction ID
    juce::Random rng;
    for (int i = 0; i < 12; ++i) {
        header.transactionId[i] = (uint8_t)rng.nextInt(256);
    }

    packet.append(&header, sizeof(STUNHeader));

    // Add REQUESTED-TRANSPORT attribute (UDP)
    {
        uint32_t protocol = 17;  // UDP protocol number
        STUNAttributeHeader attrHeader;
        attrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNAttributeType::RequestedTransport);
        attrHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)4);

        packet.append(&attrHeader, sizeof(STUNAttributeHeader));
        packet.append(&protocol, sizeof(uint32_t));
    }

    // Add LIFETIME attribute
    {
        uint32_t lifetimeSwapped = juce::ByteOrder::swapIfBigEndian(lifetime);
        STUNAttributeHeader attrHeader;
        attrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNAttributeType::Lifetime);
        attrHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)4);

        packet.append(&attrHeader, sizeof(STUNAttributeHeader));
        packet.append(&lifetimeSwapped, sizeof(uint32_t));
    }

    // Add DON'T-FRAGMENT attribute
    {
        STUNAttributeHeader attrHeader;
        attrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNAttributeType::DontFragment);
        attrHeader.length = 0;

        packet.append(&attrHeader, sizeof(STUNAttributeHeader));
    }

    // Update message length
    STUNHeader* headerPtr = (STUNHeader*)packet.getData();
    headerPtr->messageLength = juce::ByteOrder::swapIfBigEndian(
        static_cast<uint16_t>(packet.getSize() - sizeof(STUNHeader))
    );

    return packet;
}

TURNAllocationResult TURNClient::parseAllocateResponse(
    const void* data,
    int size,
    const juce::String& turnServer
) {
    TURNAllocationResult result;
    result.success = false;
    result.serverAddress = turnServer;

    if (size < (int)sizeof(STUNHeader)) {
        result.errorMessage = "Response too short";
        return result;
    }

    const STUNHeader* header = (const STUNHeader*)data;
    uint16_t messageType = juce::ByteOrder::swapIfBigEndian(header->messageType);
    uint16_t messageLength = juce::ByteOrder::swapIfBigEndian(header->messageLength);
    uint32_t magicCookie = juce::ByteOrder::swapIfBigEndian(header->magicCookie);

    if (magicCookie != STUN_MAGIC_COOKIE) {
        result.errorMessage = "Invalid magic cookie";
        return result;
    }

    // Check for error response
    if (messageType == 0x0111) {  // BindingErrorResponse
        result.errorMessage = "TURN error response (401/403/438)";
        return result;
    }

    if (messageType != (uint16_t)TURNMethod::Allocate) {
        result.errorMessage = "Unexpected message type: " + juce::String(messageType);
        return result;
    }

    // Parse attributes
    int offset = sizeof(STUNHeader);
    while (offset < size - 4) {
        const STUNAttributeHeader* attr = (const STUNAttributeHeader*)((const char*)data + offset);
        uint16_t attrType = juce::ByteOrder::swapIfBigEndian(attr->type);
        uint16_t attrLength = juce::ByteOrder::swapIfBigEndian(attr->length);

        // Pad to 4-byte boundary
        int paddedLength = (attrLength + 3) & ~3;

        // Check for XOR-RELAYED-ADDRESS (required for TURN)
        if (attrType == (uint16_t)TURNAttributeType::XORRelayedAddress) {
            const uint8_t* attrData = (const uint8_t*)attr + sizeof(STUNAttributeHeader);

            if (attrLength >= 8) {
                uint8_t family = attrData[1];
                if (family == 0x01) {  // IPv4
                    // XOR port with magic cookie
                    uint16_t xoredPort;
                    std::memcpy(&xoredPort, attrData + 2, 2);
                    xoredPort = juce::ByteOrder::swapIfBigEndian(xoredPort);
                    xoredPort ^= (STUN_MAGIC_COOKIE >> 16);

                    // XOR IP with magic cookie
                    uint32_t xoredIP;
                    std::memcpy(&xoredIP, attrData + 4, 4);
                    xoredIP = juce::ByteOrder::swapIfBigEndian(xoredIP);
                    xoredIP ^= STUN_MAGIC_COOKIE;

                    result.relayedPort = xoredPort;
                    result.relayedAddress =
                        juce::String((xoredIP >> 24) & 0xFF) + "." +
                        juce::String((xoredIP >> 16) & 0xFF) + "." +
                        juce::String((xoredIP >> 8) & 0xFF) + "." +
                        juce::String(xoredIP & 0xFF);

                    result.success = true;
                }
            }
        }

        // Check for LIFETIME attribute
        if (attrType == (uint16_t)TURNAttributeType::Lifetime) {
            const uint8_t* attrData = (const uint8_t*)attr + sizeof(STUNAttributeHeader);
            if (attrLength >= 4) {
                uint32_t lifetime;
                std::memcpy(&lifetime, attrData, 4);
                result.lifetime = juce::ByteOrder::swapIfBigEndian(lifetime);
            }
        }

        // Check for ERROR-CODE attribute
        if (attrType == (uint16_t)TURNAttributeType::ErrorCode) {
            const uint8_t* attrData = (const uint8_t*)attr + sizeof(STUNAttributeHeader);

            if (attrLength >= 4) {
                uint32_t errorCode;
                std::memcpy(&errorCode, attrData, 4);
                errorCode = juce::ByteOrder::swapIfBigEndian(errorCode) >> 16;

                int errorClass = (errorCode >> 8) & 0x7;
                int errorNumber = errorCode & 0xFF;

                result.errorMessage = "TURN error " + juce::String(errorClass) +
                                     "." + juce::String(errorNumber);
                result.success = false;
                return result;
            }
        }

        offset += sizeof(STUNAttributeHeader) + paddedLength;
    }

    if (!result.success && result.errorMessage.isEmpty()) {
        result.errorMessage = "No relayed address in TURN response";
    }

    // Store transaction ID for refresh
    std::memcpy(&result.transactionID, header->transactionId, 12);

    return result;
}

TURNAllocationResult TURNClient::allocateRelay(
    const juce::String& turnServer,
    int turnPort,
    const juce::String& username,
    const juce::String& password,
    int timeoutMs
) {
    TURNAllocationResult result;

    // Bind to any available port
    if (!pimpl->socket.bindToPort(0)) {
        result.errorMessage = "Failed to bind socket for TURN";
        return result;
    }

    // Build ALLOCATE request
    juce::MemoryBlock request = buildAllocateRequest(username, 600);  // 10 minute lifetime

    // Send request
    int bytesSent = pimpl->socket.write(
        turnServer,
        turnPort,
        request.getData(),
        static_cast<int>(request.getSize())
    );

    if (bytesSent <= 0) {
        result.errorMessage = "Failed to send TURN allocate request";
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

            if (bytesRead > 0 && responseIP == turnServer) {
                result = parseAllocateResponse(buffer, bytesRead, turnServer);
                result.serverPort = turnPort;
                result.username = username;
                result.password = password;

                if (result.success) {
                    DBG("TURN: Successfully allocated relay at " +
                        result.relayedAddress + ":" + juce::String(result.relayedPort));
                } else {
                    DBG("TURN: Allocation failed: " + result.errorMessage);
                }

                return result;
            }
        }
    }

    result.errorMessage = "TURN allocate request timeout";
    return result;
}

void TURNClient::allocateRelayAsync(
    const juce::String& turnServer,
    int turnPort,
    const juce::String& username,
    const juce::String& password,
    AllocationCallback callback,
    int timeoutMs
) {
    juce::Thread::launch([this, turnServer, turnPort, username, password, callback, timeoutMs]() {
        auto result = allocateRelay(turnServer, turnPort, username, password, timeoutMs);
        if (callback) {
            callback(result);
        }
    });
}

juce::MemoryBlock TURNClient::buildRefreshRequest(
    const TURNAllocationResult& allocation,
    uint32_t lifetime
) {
    juce::MemoryBlock packet;

    // Build STUN header with Refresh method
    STUNHeader header;
    header.messageType = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNMethod::Refresh);
    header.magicCookie = juce::ByteOrder::swapIfBigEndian(STUN_MAGIC_COOKIE);

    // Use same transaction ID from allocation
    std::memcpy(header.transactionId, &allocation.transactionID, 12);

    packet.append(&header, sizeof(STUNHeader));

    // Add LIFETIME attribute
    uint32_t lifetimeSwapped = juce::ByteOrder::swapIfBigEndian(lifetime);
    STUNAttributeHeader attrHeader;
    attrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNAttributeType::Lifetime);
    attrHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)4);

    packet.append(&attrHeader, sizeof(STUNAttributeHeader));
    packet.append(&lifetimeSwapped, sizeof(uint32_t));

    // Update message length
    STUNHeader* headerPtr = (STUNHeader*)packet.getData();
    headerPtr->messageLength = juce::ByteOrder::swapIfBigEndian(
        static_cast<uint16_t>(packet.getSize() - sizeof(STUNHeader))
    );

    return packet;
}

bool TURNClient::refreshAllocation(
    const TURNAllocationResult& allocation,
    const juce::String& username,
    const juce::String& password
) {
    juce::MemoryBlock request = buildRefreshRequest(allocation, 600);

    // Send to TURN server
    int bytesSent = pimpl->socket.write(
        allocation.serverAddress,
        allocation.serverPort,
        request.getData(),
        static_cast<int>(request.getSize())
    );

    if (bytesSent <= 0) {
        return false;
    }

    // Wait for response
    char buffer[512];
    juce::String responseIP;
    int responsePort;

    if (pimpl->socket.waitUntilReady(true, 2000) > 0) {
        int bytesRead = pimpl->socket.read(buffer, sizeof(buffer), false, responseIP, responsePort);

        if (bytesRead > 0 && responseIP == allocation.serverAddress) {
            auto result = parseAllocateResponse(buffer, bytesRead, allocation.serverAddress);
            return result.success;
        }
    }

    return false;
}

bool TURNClient::deallocate(
    const TURNAllocationResult& allocation,
    const juce::String& username,
    const juce::String& password
) {
    // Send REFRESH with lifetime = 0 to deallocate
    juce::MemoryBlock request = buildRefreshRequest(allocation, 0);

    int bytesSent = pimpl->socket.write(
        allocation.serverAddress,
        allocation.serverPort,
        request.getData(),
        static_cast<int>(request.getSize())
    );

    DBG("TURN: Sent deallocation request");
    return bytesSent > 0;
}

bool TURNClient::createPermission(
    const TURNAllocationResult& allocation,
    const juce::String& peerIP,
    int peerPort,
    const juce::String& username,
    const juce::String& password
) {
    juce::MemoryBlock packet;

    // Build STUN header with CreatePermission method
    STUNHeader header;
    header.messageType = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNMethod::CreatePermission);
    header.magicCookie = juce::ByteOrder::swapIfBigEndian(STUN_MAGIC_COOKIE);

    // Generate new transaction ID
    juce::Random rng;
    for (int i = 0; i < 12; ++i) {
        header.transactionId[i] = (uint8_t)rng.nextInt(256);
    }

    packet.append(&header, sizeof(STUNHeader));

    // Add XOR-PEER-ADDRESS attribute
    {
        // Convert IP to bytes
        uint32_t ip = 0;
        auto parts = juce::StringArray::fromTokens(peerIP, ".");
        if (parts.size() == 4) {
            ip = (parts[0].getIntValue() << 24) |
                  (parts[1].getIntValue() << 16) |
                  (parts[2].getIntValue() << 8) |
                  parts[3].getIntValue();
        }

        // XOR with magic cookie
        uint32_t xoredIP = ip ^ STUN_MAGIC_COOKIE;
        uint16_t xoredPort = juce::ByteOrder::swapIfBigEndian((uint16_t)peerPort) ^ (STUN_MAGIC_COOKIE >> 16);

        STUNAttributeHeader attrHeader;
        attrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNAttributeType::XORPeerAddress);
        attrHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)8);

        packet.append(&attrHeader, sizeof(STUNAttributeHeader));

        uint8_t family = 0x01;  // IPv4
        packet.append(&family, 1);
        packet.append(&xoredPort, 2);

        uint32_t xoredIPSwapped = juce::ByteOrder::swapIfBigEndian(xoredIP);
        packet.append(&xoredIPSwapped, 4);
    }

    // Update message length
    STUNHeader* headerPtr = (STUNHeader*)packet.getData();
    headerPtr->messageLength = juce::ByteOrder::swapIfBigEndian(
        static_cast<uint16_t>(packet.getSize() - sizeof(STUNHeader))
    );

    // Send request
    int bytesSent = pimpl->socket.write(
        allocation.serverAddress,
        allocation.serverPort,
        packet.getData(),
        static_cast<int>(packet.getSize())
    );

    if (bytesSent <= 0) {
        DBG("TURN: Failed to send create permission");
        return false;
    }

    // Wait for success response
    char buffer[512];
    juce::String responseIP;
    int responsePort;

    if (pimpl->socket.waitUntilReady(true, 2000) > 0) {
        int bytesRead = pimpl->socket.read(buffer, sizeof(buffer), false, responseIP, responsePort);

        if (bytesRead > 0 && responseIP == allocation.serverAddress) {
            DBG("TURN: Created permission for " + peerIP + ":" + juce::String(peerPort));
            return true;
        }
    }

    return false;
}

bool TURNClient::sendData(
    const TURNAllocationResult& allocation,
    const juce::String& peerIP,
    int peerPort,
    const void* data,
    int size,
    const juce::String& username,
    const juce::String& password
) {
    juce::MemoryBlock packet;

    // Build STUN header with Send method
    STUNHeader header;
    header.messageType = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNMethod::Send);
    header.magicCookie = juce::ByteOrder::swapIfBigEndian(STUN_MAGIC_COOKIE);

    // Generate transaction ID
    juce::Random rng;
    for (int i = 0; i < 12; ++i) {
        header.transactionId[i] = (uint8_t)rng.nextInt(256);
    }

    packet.append(&header, sizeof(STUNHeader));

    // Add XOR-PEER-ADDRESS attribute
    {
        uint32_t ip = 0;
        auto parts = juce::StringArray::fromTokens(peerIP, ".");
        if (parts.size() == 4) {
            ip = (parts[0].getIntValue() << 24) |
                  (parts[1].getIntValue() << 16) |
                  (parts[2].getIntValue() << 8) |
                  parts[3].getIntValue();
        }

        uint32_t xoredIP = ip ^ STUN_MAGIC_COOKIE;
        uint16_t xoredPort = juce::ByteOrder::swapIfBigEndian((uint16_t)peerPort) ^ (STUN_MAGIC_COOKIE >> 16);

        STUNAttributeHeader attrHeader;
        attrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNAttributeType::XORPeerAddress);
        attrHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)8);

        packet.append(&attrHeader, sizeof(STUNAttributeHeader));

        uint8_t family = 0x01;
        packet.append(&family, 1);
        packet.append(&xoredPort, 2);
        uint32_t xoredIPSwapped = juce::ByteOrder::swapIfBigEndian(xoredIP);
        packet.append(&xoredIPSwapped, 4);
    }

    // Add DATA attribute
    {
        STUNAttributeHeader attrHeader;
        attrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNAttributeType::Data);
        attrHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)size);

        packet.append(&attrHeader, sizeof(STUNAttributeHeader));
        packet.append(data, size);
    }

    // Update message length
    STUNHeader* headerPtr = (STUNHeader*)packet.getData();
    headerPtr->messageLength = juce::ByteOrder::swapIfBigEndian(
        static_cast<uint16_t>(packet.getSize() - sizeof(STUNHeader))
    );

    // Send via TURN server
    int bytesSent = pimpl->socket.write(
        allocation.relayedAddress,
        allocation.relayedPort,
        packet.getData(),
        static_cast<int>(packet.getSize())
    );

    return bytesSent > 0;
}

bool TURNClient::bindChannel(
    const TURNAllocationResult& allocation,
    const juce::String& peerIP,
    int peerPort,
    uint16_t channelNumber,
    const juce::String& username,
    const juce::String& password
) {
    // Channel binding is more efficient for data transfer
    // For now, we'll use the SEND method (less efficient but simpler)
    ignoreUnused(allocation, peerIP, peerPort, channelNumber, username, password);
    return true;
}

ICECandidate TURNClient::createRelayedCandidate(
    const TURNAllocationResult& allocation,
    int localPort
) {
    ICECandidate candidate;

    if (allocation.success && allocation.relayedAddress.isNotEmpty() && allocation.relayedPort > 0) {
        candidate.foundation = ICECandidate::generateFoundation(
            allocation.relayedAddress,
            allocation.relayedPort,
            CandidateType::Relayed
        );
        candidate.componentId = 1;
        candidate.transport = TransportProtocol::UDP;
        candidate.priority = ICECandidate::calculatePriority(CandidateType::Relayed, 65535);
        candidate.connectionAddress = allocation.relayedAddress;
        candidate.port = allocation.relayedPort;
        candidate.type = CandidateType::Relayed;
        candidate.relatedAddress = allocation.serverAddress;
        candidate.relatedPort = allocation.serverPort;

        DBG("TURN: Created relayed candidate: " + candidate.connectionAddress + ":" +
            juce::String(candidate.port));
    } else {
        DBG("TURN: Cannot create candidate - allocation failed");
    }

    return candidate;
}

juce::MemoryBlock TURNClient::calculateHMACSHA1(
    const juce::String& key,
    const void* data,
    int size
) {
    // For now, return empty hash - full HMAC-SHA1 implementation would go here
    // TURN authentication is optional for basic functionality
    // TODO: Implement proper HMAC-SHA1 using JUCE cryptography classes
    juce::ignoreUnused(key, data, size);
    return juce::MemoryBlock(20);  // SHA1 produces 20 bytes
}

bool TURNClient::addMessageIntegrity(
    juce::MemoryBlock& packet,
    const juce::String& username,
    const juce::String& password,
    const juce::String& realm,
    const juce::String& nonce
) {
    // Calculate HMAC-SHA1 for long-term credential authentication
    auto hmac = calculateHMACSHA1(password, packet.getData(), (int)packet.getSize());

    // Add MESSAGE-INTEGRITY attribute
    STUNAttributeHeader attrHeader;
    attrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)TURNAttributeType::MessageIntegrity);
    attrHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)hmac.getSize());

    packet.append(&attrHeader, sizeof(STUNAttributeHeader));
    packet.append(hmac.getData(), hmac.getSize());

    // Update message length
    STUNHeader* headerPtr = (STUNHeader*)packet.getData();
    headerPtr->messageLength = juce::ByteOrder::swapIfBigEndian(
        static_cast<uint16_t>(packet.getSize() - sizeof(STUNHeader))
    );

    return true;
}

} // namespace zenith
