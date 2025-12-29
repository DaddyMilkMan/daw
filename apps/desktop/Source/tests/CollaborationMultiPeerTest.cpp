/**
 * @file CollaborationMultiPeerTest.cpp
 * @brief Test for multi-peer collaboration and CRDT sync
 */

#include <juce_core/juce_core.h>
#include "network/CollaborationManager.h"
#include "engine/ProjectState.h"

namespace zenith {

class CollaborationMultiPeerTest : public juce::UnitTest {
public:
    CollaborationMultiPeerTest() : juce::UnitTest("Collaboration Multi-Peer", "Networking") {}

    void runTest() override {
        beginTest("Initial State");
        
        auto& mgr1 = zenith::CollaborationManager::getInstance();
        // Verify initial disconnected state
        expect(mgr1.getState() == zenith::CollaborationManager::ConnectionState::Disconnected);
        
        beginTest("CRDT Initialization");
        zenith::ProjectState ps;
        
        // Ensure root has an ID before CRDT init (required for sync)
        if (ps.getState().getProperty("id").toString().isEmpty()) {
            ps.getState().setProperty("id", juce::Uuid().toString(), nullptr);
        }
        
        mgr1.initializeCRDT(ps.getState());
        
        // Check if root has ID
        expect(ps.getState().getProperty("id").toString().isNotEmpty());
        
        beginTest("Connection State Machine");
        // Verify state is still disconnected (we haven't started hosting/joining)
        expect(mgr1.getState() == zenith::CollaborationManager::ConnectionState::Disconnected);
        
        // Verify we can get remote users (should be empty initially)
        auto remoteUsers = mgr1.getRemoteUsers();
        expect(remoteUsers.empty());
        
        // Cleanup: release CRDT bridge before ProjectState goes out of scope
        mgr1.shutdownCRDT();
    }
};

static CollaborationMultiPeerTest collaborationMultiPeerTest;

} // namespace Zenith
