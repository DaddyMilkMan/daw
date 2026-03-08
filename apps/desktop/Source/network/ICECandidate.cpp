#include "ICECandidate.h"
#include <random>
#include <sstream>

namespace zenith {

juce::String ICECandidate::toString() const {
    // SDP candidate attribute format:
    // a=candidate:foundation componentID transport priority connectionAddress port typ [relatedAddress] [relatedPort]
    juce::String typeStr;
    switch (type) {
        case CandidateType::Host: typeStr = "host"; break;
        case CandidateType::ServerReflexive: typeStr = "srflx"; break;
        case CandidateType::PeerReflexive: typeStr = "prflx"; break;
        case CandidateType::Relayed: typeStr = "relay"; break;
    }

    juce::String s = "a=candidate:";
    s += foundation + " ";
    s += juce::String(componentId) + " ";
    s += juce::String(transport == TransportProtocol::UDP ? "UDP" : "TCP") + " ";
    s += juce::String(priority) + " ";
    s += connectionAddress + " ";
    s += juce::String(port) + " ";
    s += "typ " + typeStr;

    if (relatedAddress.isNotEmpty()) {
        s += " raddr " + relatedAddress;
        s += " rport " + juce::String(relatedPort);
    }

    return s;
}

ICECandidate ICECandidate::fromString(const juce::String& str) {
    ICECandidate candidate;

    // Parse SDP candidate format
    // a=candidate:foundation componentID transport priority connectionAddress port typ type [raddr relatedAddress] [rport relatedPort]
    auto parts = juce::StringArray::fromTokens(str, " ", "");

    if (parts.size() < 8 || !parts[0].startsWith("a=candidate:")) {
        return candidate;  // Invalid
    }

    candidate.foundation = parts[0].substring(11);  // Remove "a=candidate:"
    candidate.componentId = parts[1].getIntValue();
    candidate.transport = (parts[2] == "UDP") ? TransportProtocol::UDP : TransportProtocol::TCP;
    candidate.priority = parts[3].getLargeIntValue();
    candidate.connectionAddress = parts[4];
    candidate.port = parts[5].getIntValue();

    // Parse type
    if (parts.size() > 7 && parts[6] == "typ") {
        juce::String typeStr = parts[7];
        if (typeStr == "host") candidate.type = CandidateType::Host;
        else if (typeStr == "srflx") candidate.type = CandidateType::ServerReflexive;
        else if (typeStr == "prflx") candidate.type = CandidateType::PeerReflexive;
        else if (typeStr == "relay") candidate.type = CandidateType::Relayed;
    }

    // Parse related address and port if present
    for (int i = 8; i < parts.size() - 1; ++i) {
        if (parts[i] == "raddr" && i + 1 < parts.size()) {
            candidate.relatedAddress = parts[i + 1];
        } else if (parts[i] == "rport" && i + 1 < parts.size()) {
            candidate.relatedPort = parts[i + 1].getIntValue();
        }
    }

    return candidate;
}

uint64_t ICECandidatePair::calculatePairPriority(
    const ICECandidate& local,
    const ICECandidate& remote,
    bool isControllingAgent
) {
    // RFC 5245 5.7.2: Pair priority formula
    // pair priority = 2^32 * MIN(local, remote) + 2 * MAX(local, remote) +
    //                 (local preference > remote preference ? 1 : 0)

    uint64_t G;
    if (local.priority < remote.priority) {
        G = ((uint64_t)local.priority << 32) + (2 * (uint64_t)remote.priority) + 1;
    } else {
        G = ((uint64_t)remote.priority << 32) + (2 * (uint64_t)local.priority);
    }

    return G;
}

juce::String ICECandidatePair::getPairFoundation() const {
    // Foundation for pair identifies unique combination of candidate foundations
    return local.foundation + "-" + remote.foundation;
}

juce::String ICECandidate::generateFoundation(const juce::String& ip, int port, CandidateType type) {
    // Foundation uniquely identifies a candidate
    // Format: "type:ip:port" hashed
    juce::String input = juce::String((int)type) + ":" + ip + ":" + juce::String(port);

    // Simple hash (not cryptographically secure, just for uniqueness)
    uint32_t hash = 0;
    for (auto c : input) {
        hash = hash * 31 + (uint32_t)c;
    }

    return juce::String(hash);
}

uint64_t ICECandidate::calculatePriority(CandidateType type, int localPref) {
    // Priority formula from RFC 5245:
    // priority = (2^24 * type preference) + (2^8 * local preference) + (256 - component ID)

    // Type preferences: host (126) > srflx (100) > prflx (110) > relay (0)
    int typePref = 0;
    switch (type) {
        case CandidateType::Host: typePref = 126; break;
        case CandidateType::PeerReflexive: typePref = 110; break;
        case CandidateType::ServerReflexive: typePref = 100; break;
        case CandidateType::Relayed: typePref = 0; break;
    }

    uint64_t priority = ((uint64_t)typePref << 24) +
                        ((uint64_t)localPref << 8) +
                        (256 - 1);  // component ID = 1

    return priority;
}

} // namespace zenith
