#include "../engine/ProjectState.h"
#include "../network/LoroCRDTBridge.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

class CRDTSyncTests : public juce::UnitTest {
public:
    CRDTSyncTests() : juce::UnitTest("CRDT Synchronization", "Network") {}

    void runTest() override {
        beginTest("Simple property synchronization");
        {
            ProjectState stateA, stateB;
            // Root and container IDs are now set by ProjectState::createDefaultState

            Zenith::LoroDoc docA, docB;
            Zenith::ValueTreeCRDTBridge bridgeA(stateA.getState(), docA);
            Zenith::ValueTreeCRDTBridge bridgeB(stateB.getState(), docB);

            // 1. Change volume on State A
            stateA.addTrack("Track 1");
            auto trackId = stateA.getTrackByIndex(0).getProperty(ProjectState::PROP_ID).toString();
            stateA.setTrackVolume(trackId, 0.5f);

            // 2. Export updates from A and import to B
            auto updates = docA.exportUpdates();
            bridgeB.applyRemoteUpdates(updates);

            // 3. Verify State B has the track and the volume
            expect(stateB.getNumTracks() == 1, "StateB should have 1 track");
            if (stateB.getNumTracks() > 0) {
                juce::String trackIdB = stateB.getTrackByIndex(0).getProperty(ProjectState::PROP_ID).toString();
                float volB = stateB.getTrackVolume(trackIdB); // Use the ID from B, though should be same
                expectWithinAbsoluteError(volB, 0.5f, 0.001f);
            }
        }

        beginTest("Concurrent edits resolution (LWW)");
        {
            ProjectState stateA, stateB;
            Zenith::LoroDoc docA, docB;
            
            // Initialize bridges first
            Zenith::ValueTreeCRDTBridge bridgeA(stateA.getState(), docA);
            Zenith::ValueTreeCRDTBridge bridgeB(stateB.getState(), docB);
            
            // Seed State A with a track
            stateA.addTrack("Shared");
            auto trackId = stateA.getTrackByIndex(0).getProperty(ProjectState::PROP_ID).toString();
            
            // Sync initial state from A to B so B has the track
            auto initialUpdates = docA.exportUpdates();
            bridgeB.applyRemoteUpdates(initialUpdates);
            
            // Verify B now has the track  
            expect(stateB.getNumTracks() == 1, "StateB should have track after initial sync");

            if (stateB.getNumTracks() == 1) {
                // Concurrent edits: B sets volume to 0.2, then A sets volume to 0.8 (later timestamp wins)
                stateB.setTrackVolume(trackId, 0.2f);
                
                // Force A's counter higher to ensure it wins LWW
                auto trackNode = stateA.getState().getChildWithProperty(ProjectState::PROP_ID, trackId);
                // Trigger a prop change to bump counter? Or just rely on natural order.
                // In test env, we can't easily force counters without hacking LoroDoc.
                // But bridge uses doc.nextCounter().
                // Call nextCounter() on docA a few times to ensure it's ahead.
                docA.nextCounter();
                docA.nextCounter();

                stateA.setTrackVolume(trackId, 0.8f);

                // Sync both ways - A's edit (0.8) has higher counter so should win
                auto updatesFromA = docA.exportUpdates();
                auto updatesFromB = docB.exportUpdates();
                
                bridgeB.applyRemoteUpdates(updatesFromA);
                bridgeA.applyRemoteUpdates(updatesFromB);

                // Both should converge to 0.8 (A's edit had higher counter)
                float volA = stateA.getTrackVolume(trackId);
                float volB_final = stateB.getTrackVolume(trackId);
                expectWithinAbsoluteError(volA, 0.8f, 0.01f);
                expectWithinAbsoluteError(volB_final, 0.8f, 0.01f);
            }
        }

        beginTest("Timeline synchronization (Moving Clips)");
        {
            ProjectState stateA, stateB;
            Zenith::LoroDoc docA, docB;
            Zenith::ValueTreeCRDTBridge bridgeA(stateA.getState(), docA);
            Zenith::ValueTreeCRDTBridge bridgeB(stateB.getState(), docB);

            stateA.addTrack("Audio");
            auto trackId = stateA.getTrackByIndex(0).getProperty(ProjectState::PROP_ID).toString();
            auto clipId = stateA.addClip(trackId, 0.0, 4.0, "Test Clip");

            // Sync A -> B
            bridgeB.applyRemoteUpdates(docA.exportUpdates());

            // Move clip on A
            stateA.setClipRange(clipId, 8.0, 4.0, "Move");

            // Sync A -> B
            bridgeB.applyRemoteUpdates(docA.exportUpdates());

            // Verify B moved
            auto clipB = stateB.getClip(trackId, clipId);
            expect(clipB.isValid(), "Clip B should be valid");
            if (clipB.isValid()) {
                double startB = clipB.getProperty(ProjectState::PROP_START_BEATS);
                expect(startB == 8.0);
            }
        }
    }
};

static CRDTSyncTests crdtSyncTests;

} // namespace tests
} // namespace zenith
