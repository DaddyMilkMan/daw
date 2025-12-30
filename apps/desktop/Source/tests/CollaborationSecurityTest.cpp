#include <juce_core/juce_core.h>
#include "../network/CollaborationManager.h"

namespace zenith {

class CollaborationSecurityTest : public juce::UnitTest {
public:
    CollaborationSecurityTest() : juce::UnitTest("CollaborationSecurityTest") {}

    void runTest() override {
        beginTest("Verify HMAC Vectors");
        {
            // RFC 4231 Test Case 1
            juce::String key;
            for (int i=0; i<20; ++i) key += juce::String::charToString(0x0b);
            
            juce::String msg = "Hi There";
            
            juce::String hmac = CollaborationManager::calculateHMAC(msg, key);
            expectEquals(hmac.toLowerCase(), juce::String("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"));
        }

        beginTest("Authentication Flow Success");
        {
            auto& cm = CollaborationManager::getInstance();
            cm.disconnect(); // Reset state
            
            cm.currentState.store(CollaborationManager::ConnectionState::Handshaking);
            cm.sessionCode = "SECURE_SESSION_123";
            
            juce::String peerIP = "192.168.1.50";
            int peerPort = 12345;
            juce::String salt = "0123456789ABCDEF0123456789ABCDEF";
            int challenge = 9999;
            
            {
                const juce::ScopedLock sl(cm.peersLock);
                cm.activePeers.clear();
                CollaborationManager::PeerConnection peer;
                peer.ip = peerIP;
                peer.port = peerPort;
                peer.authenticated = false;
                peer.lastSeen = (juce::uint64)juce::Time::currentTimeMillis();
                peer.challenge = challenge;
                peer.salt = salt;
                cm.activePeers.push_back(peer);
            }
            
            juce::String timestamp = juce::String(juce::Time::currentTimeMillis() / 30000);
            
            // Re-derive key using PBKDF2 (as the manager does)
            juce::MemoryBlock sessionKey = CollaborationManager::deriveSessionKey(cm.sessionCode, salt);
            juce::String secret = juce::String::toHexString(sessionKey.getData(), (int)sessionKey.getSize(), 0);
            
            juce::String message = juce::String(challenge);
            juce::String signature = CollaborationManager::calculateHMAC(message, secret);
            
            juce::MemoryBlock packet;
            int type = (int)PacketType::ChallengeResponse;
            packet.append(&type, sizeof(int));
            packet.append(signature.toRawUTF8(), signature.length());
            
            cm.handleIncomingPacket(packet.getData(), (int)packet.getSize(), peerIP, peerPort);
            
            {
                const juce::ScopedLock sl(cm.peersLock);
                expect(!cm.activePeers.empty());
                if (!cm.activePeers.empty()) {
                    expect(cm.activePeers[0].authenticated);
                }
            }
            expect(cm.currentState.load() == CollaborationManager::ConnectionState::Connected);
            
            cm.disconnect();
        }

        beginTest("Authentication Flow Failure (Wrong Signature)");
        {
            auto& cm = CollaborationManager::getInstance();
            cm.disconnect();
            
            cm.currentState.store(CollaborationManager::ConnectionState::Handshaking);
            cm.sessionCode = "SECURE_SESSION_123";
            
            juce::String peerIP = "192.168.1.51";
            int peerPort = 12345;
            {
                const juce::ScopedLock sl(cm.peersLock);
                cm.activePeers.clear();
                CollaborationManager::PeerConnection peer;
                peer.ip = peerIP;
                peer.port = peerPort;
                peer.authenticated = false;
                peer.lastSeen = (juce::uint64)juce::Time::currentTimeMillis();
                peer.challenge = 9999;
                peer.salt = "SALT";
                cm.activePeers.push_back(peer);
            }
            
            juce::String signature = "0000000000000000000000000000000000000000000000000000000000000000";
            
            juce::MemoryBlock packet;
            int type = (int)PacketType::ChallengeResponse;
            packet.append(&type, sizeof(int));
            packet.append(signature.toRawUTF8(), signature.length());
            
            cm.handleIncomingPacket(packet.getData(), (int)packet.getSize(), peerIP, peerPort);
            
            {
                const juce::ScopedLock sl(cm.peersLock);
                expect(!cm.activePeers.empty());
                if (!cm.activePeers.empty()) {
                    expect(!cm.activePeers[0].authenticated);
                }
            }
             cm.disconnect();
        }

        beginTest("Replay Attack (Old Timestamp)");
        {
            // Note: Current implementation uses challenge instead of global timestamp for signing in ChallengeResponse,
            // but we can test that an old signature (captured from previous block) doesn't work if challenge changed.
            // Actually, the new implementation doesn't use timestamp in ChallengeResponse anymore, it uses per-peer challenge.
            // So we'll skip this or update it to test challenge mismatch.
            beginTest("Challenge Mismatch Prevention");
            
            auto& cm = CollaborationManager::getInstance();
            cm.disconnect();
            
            cm.currentState.store(CollaborationManager::ConnectionState::Handshaking);
            cm.sessionCode = "SECURE_SESSION_123";
            
            juce::String peerIP = "192.168.1.52";
            int peerPort = 12345;
            juce::String salt = "SALT";
            int actualChallenge = 9999;
            int oldChallenge = 8888;
            
            {
                const juce::ScopedLock sl(cm.peersLock);
                cm.activePeers.clear();
                CollaborationManager::PeerConnection peer;
                peer.ip = peerIP;
                peer.port = peerPort;
                peer.authenticated = false;
                peer.lastSeen = (juce::uint64)juce::Time::currentTimeMillis();
                peer.challenge = actualChallenge;
                peer.salt = salt;
                cm.activePeers.push_back(peer);
            }
            
            // Signature computed for OLD challenge
            juce::MemoryBlock sessionKey = CollaborationManager::deriveSessionKey(cm.sessionCode, salt);
            juce::String secret = juce::String::toHexString(sessionKey.getData(), (int)sessionKey.getSize(), 0);
            juce::String message = juce::String(oldChallenge);
            juce::String signature = CollaborationManager::calculateHMAC(message, secret);
            
            juce::MemoryBlock packet;
            int type = (int)PacketType::ChallengeResponse;
            packet.append(&type, sizeof(int));
            packet.append(signature.toRawUTF8(), signature.length());
            
            cm.handleIncomingPacket(packet.getData(), (int)packet.getSize(), peerIP, peerPort);
            
            {
                const juce::ScopedLock sl(cm.peersLock);
                expect(!cm.activePeers.empty());
                if (!cm.activePeers.empty()) {
                    expect(!cm.activePeers[0].authenticated);
                }
            }
            cm.disconnect();
        }
    }
};

static CollaborationSecurityTest collaborationSecurityTest;

} // namespace zenith
