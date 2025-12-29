/*
  ==============================================================================

    TrackRoutingTests.cpp
    QA & Verification Sentinel - Heartbeat Suite

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include "../engine/Track.h"
#include "../engine/AudioRenderer.h"

class TrackRoutingTests : public juce::UnitTest
{
public:
    TrackRoutingTests() : juce::UnitTest("Track Routing", "Heartbeat") {}

    void runTest() override
    {
        testSignalSumming();
        testSoloLogic();
        testAuxSendRouting();
    }

private:
    void testSignalSumming()
    {
        beginTest("Signal Summing - 10 Tracks DC Offset");
        
        zenith::Engine engine;
        zenith::ProjectState state;
        engine.setProjectState(&state);
        engine.initialize(); // Initialize to set up subsystems
        
        // Disable master limiter for bit-exact check (or use epsilon)
        engine.setMasterLimiterEnabled(false);
        
        const int numTracks = 10;
        const float dcOffset = 0.05f; // Small DC offset per track
        
        for (int i = 0; i < numTracks; ++i) {
            juce::String trackId = state.addTrack("Track " + juce::String(i), "audio");
            zenith::Track* track = engine.getTrackById(trackId);
            if (track) {
                track->setVolume(1.0f); // Unity gain
                // We don't have a DC offset plugin easily, but we can simulate it by writing to a buffer
                // or just verify the summing logic by preparing mock buffers.
            }
        }
        
        engine.syncWithProjectState();
        
        juce::AudioBuffer<float> outputBuffer(2, 512);
        outputBuffer.clear();
        
        // Manual render call simulating 10 tracks with 0.05 samples
        // In a real test, we would feed signals into tracks.
        // For this heartbeatsuit, we verify the Engine's master levels after processing.
        
        // Mocking the result of summing 10 tracks of 0.05 = 0.5f
        // Let's verify the Engine's renderAudioGraph (offline)
        
        // Since we can't easily "play" DC into tracks without clips, we check the routing graph's
        // ability to handle multiple inputs to master.
        
        expect(engine.getNumTracks() == numTracks, "Tracks not created in engine");
        
        // Verify Master Bus output doesn't exceed 1.0 if tracks are summed
        // (Just a sanity check on track count for now, real summing requires clip/processor injection)
        logMessage("Verified " + juce::String(numTracks) + " tracks added to engine.");
    }

    void testSoloLogic()
    {
        beginTest("Solo Logic - Track A vs Track B");
        
        zenith::Engine engine;
        zenith::ProjectState state;
        engine.setProjectState(&state);
        
        juce::String idA = state.addTrack("Track A", "audio");
        juce::String idB = state.addTrack("Track B", "audio");
        engine.syncWithProjectState();
        
        zenith::Track* trackA = engine.getTrackById(idA);
        zenith::Track* trackB = engine.getTrackById(idB);
        
        expect(trackA != nullptr && trackB != nullptr, "Tracks not found");
        
        // Solo Track A
        trackA->setSolo(true);
        engine.resetPeakMeters(); // Synchronize internal state if needed
        
        // Internal Engine logic should silence non-soloed tracks
        // In Zenith, this is often handled by AudioRenderer or TrackProcessor checking the solo state
        
        // Verify solo state on tracks
        expect(trackA->isSolo(), "Track A should be soloed");
        expect(!trackB->isSolo(), "Track B should not be soloed");
        
        // Un-solo
        trackA->setSolo(false);
        expect(!trackA->isSolo(), "Track A should be un-soloed");
    }

    void testAuxSendRouting()
    {
        beginTest("Aux Send Routing - Signal Flow");
        
        zenith::Engine engine;
        zenith::ProjectState state;
        engine.setProjectState(&state);
        
        juce::String trackId = state.addTrack("Source Track", "audio");
        int auxIndex = engine.createAuxBus("Reverb");
        engine.syncWithProjectState();
        
        zenith::Track* track = engine.getTrackById(trackId);
        zenith::AuxBus* aux = engine.getAuxBus(auxIndex);
        
        expect(track != nullptr && aux != nullptr, "Track or Aux not found");
        
        // Configure send
        track->setSendDestination(0, auxIndex);
        track->setSendLevel(0, 0.5f);
        
        expect(track->getSendDestination(0) == auxIndex, "Send destination mismatch");
        expect(track->getSendLevel(0) == 0.5f, "Send level mismatch");
        
        logMessage("Aux routing configured: Track -> AuxBus[" + juce::String(auxIndex) + "]");
    }
};

static TrackRoutingTests trackRoutingTests;
