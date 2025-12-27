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
            expect(stateB.getNumTracks() == 1);
            float volB = stateB.getTrackVolume(trackId);
            expect(volB == 0.5f);
        }

        beginTest("Concurrent edits resolution (LWW)");
        {
            ProjectState stateA, stateB;
            Zenith::LoroDoc docA, docB;
            
            // Seed both with same track
            stateA.addTrack("Shared");
            auto trackId = stateA.getTrackByIndex(0).getProperty(ProjectState::PROP_ID).toString();
            
            // Initialize bridges AFTER seeding to simulate established session
            Zenith::ValueTreeCRDTBridge bridgeA(stateA.getState(), docA);
            Zenith::ValueTreeCRDTBridge bridgeB(stateB.getState(), docB);
            
            // Sync initial state
            docB.importUpdates(docA.exportUpdates());
            bridgeB.applyRemoteUpdates(docA.exportUpdates());

            // Concurrent edits: A sets volume to 0.8, B sets volume to 0.2
            // We'll simulate A having a higher counter/timestamp
            stateB.setTrackVolume(trackId, 0.2f);
            juce::Thread::sleep(10); // Ensure slight timestamp difference if using real time
            stateA.setTrackVolume(trackId, 0.8f);

            // Sync A -> B
            bridgeB.applyRemoteUpdates(docA.exportUpdates());
            // Sync B -> A
            bridgeA.applyRemoteUpdates(docB.exportUpdates());

            // Both should converge to 0.8 (latest write)
            float volA = stateA.getTrackVolume(trackId);
            float volB_final = stateB.getTrackVolume(trackId);
            expectWithinAbsoluteError(volA, 0.8f, 0.01f);
            expectWithinAbsoluteError(volB_final, 0.8f, 0.01f);
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
            expect(clipB.isValid());
            double startB = clipB.getProperty(ProjectState::PROP_START_BEATS);
            expect(startB == 8.0);
        }
    }
};

static CRDTSyncTests crdtSyncTests;

} // namespace tests
} // namespace zenith
