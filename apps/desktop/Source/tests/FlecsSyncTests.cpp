#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../../include/ECSIntegrationExample.h"

namespace zenith {
namespace tests {

/**
 * @class FlecsSyncTests
 * @brief Tests for Flecs state synchronization
 */
class FlecsSyncTests : public juce::UnitTest {
public:
    FlecsSyncTests() : juce::UnitTest("Flecs Sync", "ECS") {}
    
    void runTest() override {
        beginTest("Flecs Component Sync");
        {
            // Setup ECSEngine (this creates the world)
            zenith::ECSEngine ecsEngine;
            
            // Create a track entity
            juce::String trackName = "Audio 1";
            juce::String trackId = "track_123";
            auto trackEntity = ecsEngine.createTrack(trackName, trackId);
            
            expect(trackEntity.is_alive());
            expectEquals(trackEntity.name(), std::string("track_123")); // Flecs names are const char*
            
            // Verify initial state
            const auto* state = trackEntity.get<zenith::ecs::TrackState>();
            expect(state != nullptr);
            expectEquals(state->volume, 0.8f); // Default from createTrack
            
            // Simulate update from "Old World" (e.g. UI slider)
            float newVolume = 0.5f;
            ecsEngine.setTrackVolume(trackEntity, newVolume);
            
            // Verify "New World" (Flecs) state updated
            state = trackEntity.get<zenith::ecs::TrackState>();
            expectEquals(state->volume, 0.5f);
            
            // Verify via query (simulating Audio Thread access)
            bool volumeUpdated = false;
            ecsEngine.world().each([&](flecs::entity e, const zenith::ecs::TrackState& t) {
                if (e == trackEntity && t.volume == 0.5f) {
                    volumeUpdated = true;
                }
            });
            
            expect(volumeUpdated);
        }
        
        beginTest("Flecs Hierarchy Sync");
        {
            zenith::ECSEngine ecsEngine;
            auto track = ecsEngine.createTrack("Track 1", "t1");
            
            // Create clip as child
            auto clip = ecsEngine.createClip(track, 0, 44100);
            
            expect(clip.is_alive());
            expect(clip.parent() == track);
            
            bool childFound = false;
            track.children([&](flecs::entity child){
                if (child == clip) childFound = true;
            });
            
            expect(childFound);
        }
    }
};

static FlecsSyncTests flecsSyncTests;

} // namespace tests
} // namespace zenith
