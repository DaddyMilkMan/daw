#include <catch2/catch_test_macros.hpp>
#include "engine/Engine.h"
#include "engine/Track.h"
#include "engine/AudioRenderer.h"
#include "engine/RealTimeGarbageCollector.h"
#include "commands/ProjectState.h"
#include <thread>
#include <atomic>
#include <vector>

using namespace zenith;

TEST_CASE("RT-Safety: Concurrent Track Removal and Rendering", "[engine][threading][safety]") {
    // We need a ProjectState and Engine
    auto projectState = std::make_unique<ProjectState>();
    Engine engine(*projectState);
    
    // Setup engine with dummy device info
    // (In a real test we'd mock the device, but Engine can run headlessly if prepared)
    engine.prepare(44100.0, 512);
    
    std::atomic<bool> stop{false};
    std::atomic<int> renderCount{0};
    std::atomic<int> removeCount{0};
    
    // 1. "Audio Thread" - Continuously rendering snapshots
    std::thread audioThread([&]() {
        juce::AudioBuffer<float> output(2, 512);
        AudioRenderContext context;
        context.prepare(44100.0, 512, 100, 32);
        
        while (!stop.load()) {
            // Simulate the high-frequency rendering loop
            engine.renderNextBlock(output, 512);
            renderCount.fetch_add(1);
            // No sleep - keep it tight to increase race chance
        }
    });
    
    // 2. "Message Thread" - Rapidly adding and removing tracks
    std::thread messageThread([&]() {
        // We must simulate being on message thread for JUCE assertions
        juce::MessageManager::getInstance(); 
        
        for (int i = 0; i < 50; ++i) {
            // Add a few tracks
            std::vector<int> addedIndices;
            for (int j = 0; j < 5; ++j) {
                juce::String name = "TestTrack_" + juce::String(i) + "_" + juce::String(j);
                engine.createTrack(name, "audio");
            }
            
            // Give audio thread a tiny bit of time to see them
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            
            // Remove tracks in reverse order (often a stress point)
            int numTracks = engine.getNumTracks();
            for (int k = numTracks - 1; k >= std::max(0, numTracks - 5); --k) {
                engine.removeTrack(k);
                removeCount.fetch_add(1);
            }
            
            // Allow GC to run occasionally
            if (i % 10 == 0) {
                // Manually trigger timer callback or wait
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    });
    
    messageThread.join();
    
    // Wait for GC to clear everything
    std::this_thread::sleep_for(std::chrono::milliseconds(1100)); // > kSafetyDurationMs
    
    stop.store(true);
    audioThread.join();
    
    REQUIRE(renderCount.load() > 0);
    REQUIRE(removeCount.load() == 250); // 50 loops * 5 tracks
}

TEST_CASE("AI Integration: Batch Command Transaction and Rollback", "[ai][commands]") {
    auto projectState = std::make_unique<ProjectState>();
    Engine engine(*projectState);
    CommandAPI commandAPI(*projectState, engine);
    
    // Create some initial state
    engine.createTrack("Initial", "audio");
    int initialTrackCount = engine.getNumTracks();
    
    SECTION("Successful batch execution") {
        juce::Array<juce::var> batch;
        
        juce::DynamicObject::Ptr cmd1 = new juce::DynamicObject();
        cmd1->setProperty("command", "create_track");
        juce::DynamicObject::Ptr params1 = new juce::DynamicObject();
        params1->setProperty("name", "Batch1");
        params1->setProperty("type", "audio");
        cmd1->setProperty("params", juce::var(params1));
        batch.add(juce::var(cmd1));
        
        juce::DynamicObject::Ptr cmd2 = new juce::DynamicObject();
        cmd2->setProperty("command", "create_track");
        juce::DynamicObject::Ptr params2 = new juce::DynamicObject();
        params2->setProperty("name", "Batch2");
        params2->setProperty("type", "audio");
        cmd2->setProperty("params", juce::var(params2));
        batch.add(juce::var(cmd2));
        
        auto result = commandAPI.executeBatch(batch, "AI Generation");
        REQUIRE(result.getProperty("success", false));
        REQUIRE(engine.getNumTracks() == initialTrackCount + 2);
        
        // Test "Deny" (Rollback)
        commandAPI.undo(); // This should undo the WHOLE batch because of beginNewTransaction
        REQUIRE(engine.getNumTracks() == initialTrackCount);
    }
}
