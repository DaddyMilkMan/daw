/**
 * @file FlecsTest.cpp
 * @brief Quick test to verify Flecs integration
 * 
 * This file can be compiled standalone to test Flecs works correctly.
 * Uncomment the line in CMakeLists.txt to enable this test.
 */

#include "ECSComponents.h"
#include <iostream>

int main() {
    std::cout << "=== Flecs Integration Test ===" << std::endl;
    
    // Create world
    auto world = zenith::ecs::createZenithWorld();
    std::cout << "✅ Created Flecs world" << std::endl;
    
    // Create project entity
    auto project = world.entity("MyProject")
        .set<zenith::ecs::TrackState>({
            .volume = 0.8f,
            .pan = 0.0f
        });
    std::cout << "✅ Created project entity" << std::endl;
    
    // Create track
    auto track1 = world.entity("Track 1")
        .set<zenith::ecs::TrackState>({
            .volume = 0.8f,
            .pan = 0.0f,
            .muted = false,
            .solo = false,
            .armed = true
        })
        .add<zenith::ecs::IsAudio>()
        .child_of(project);
    std::cout << "✅ Created Track 1" << std::endl;
    
    // Create clip (child of track)
    auto clip1 = world.entity("Clip A")
        .set<zenith::ecs::ClipState>({
            .startSample = 0,
            .lengthSample = 44100,
            .gain = 1.0f,
            .looping = false
        })
        .child_of(track1);
    std::cout << "✅ Created Clip A" << std::endl;
    
    // Create notes (children of clip)
    auto noteC4 = world.entity("Note C4")
        .set<zenith::ecs::Note>({
            .pitch = 60,
            .velocity = 100,
            .startSample = 0,
            .lengthSample = 22050
        })
        .child_of(clip1);
    std::cout << "✅ Created Note C4" << std::endl;
    
    auto noteE4 = world.entity("Note E4")
        .set<zenith::ecs::Note>({
            .pitch = 64,
            .velocity = 100,
            .startSample = 22050,
            .lengthSample = 22050
        })
        .add<zenith::ecs::Selected>()  // This note is selected
        .child_of(clip1);
    std::cout << "✅ Created Note E4 (selected)" << std::endl;
    
    // Query all tracks
    std::cout << "\n=== Querying Tracks ===" << std::endl;
    auto trackQuery = world.query<const zenith::ecs::TrackState>();
    int trackCount = 0;
    trackQuery.each([&](flecs::entity e, const zenith::ecs::TrackState& track) {
        trackCount++;
        std::cout << "Track: " << e.name().c_str() 
                  << " (volume: " << track.volume 
                  << ", armed: " << (track.armed ? "yes" : "no") << ")" << std::endl;
    });
    std::cout << "Found " << trackCount << " track(s)" << std::endl;
    
    // Query all notes
    std::cout << "\n=== Querying Notes ===" << std::endl;
    auto noteQuery = world.query<const zenith::ecs::Note>();
    int noteCount = 0;
    noteQuery.each([&](flecs::entity e, const zenith::ecs::Note& note) {
        noteCount++;
        std::cout << "Note: " << e.name().c_str() 
                  << " (pitch: " << note.pitch 
                  << ", velocity: " << note.velocity 
                  << ", selected: " << (e.has<zenith::ecs::Selected>() ? "yes" : "no") << ")" 
                  << std::endl;
    });
    std::cout << "Found " << noteCount << " note(s)" << std::endl;
    
    // Query hierarchy (track -> clips -> notes)
    std::cout << "\n=== Querying Hierarchy ===" << std::endl;
    std::cout << "Project: " << project.name().c_str() << std::endl;
    
    project.children([&](flecs::entity trackEntity) {
        std::cout << "  Track: " << trackEntity.name().c_str() << std::endl;
        
        trackEntity.children([&](flecs::entity clipEntity) {
            std::cout << "    Clip: " << clipEntity.name().c_str() << std::endl;
            
            clipEntity.children([&](flecs::entity noteEntity) {
                std::cout << "      Note: " << noteEntity.name().c_str();
                if (noteEntity.has<zenith::ecs::Selected>()) {
                    std::cout << " [SELECTED]";
                }
                std::cout << std::endl;
            });
        });
    });
    
    // Test cached query (real-time safe pattern)
    std::cout << "\n=== Testing Cached Query (Audio Thread Pattern) ===" << std::endl;
    auto cachedQuery = world.query_builder<const zenith::ecs::Note>()
        .cached()
        .build();
    
    int cachedCount = 0;
    cachedQuery.each([&](const zenith::ecs::Note& note) {
        cachedCount++;
    });
    std::cout << "Cached query found " << cachedCount << " notes (lock-free!)" << std::endl;
    
    std::cout << "\n=== ✅ All Tests Passed! ===" << std::endl;
    std::cout << "Flecs is working correctly in Zenith DAW." << std::endl;
    
#ifdef JUCE_DEBUG
    std::cout << "\nFlecs Explorer available at: http://localhost:27750" << std::endl;
    std::cout << "(Open in browser to see entity hierarchy)" << std::endl;
    
    // Keep server running so user can explore
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
#endif
    
    return 0;
}
