/**
 * @file ECSIntegrationExample.h
 * @brief Example: How to integrate Flecs with existing Zenith Engine
 * 
 * CRITICAL RULES:
 * 1. Never call world.progress() in audio thread
 * 2. Only iterate CACHED queries in audio thread
 * 3. All entity creation/deletion happens on MESSAGE THREAD
 * 4. Audio thread reads via const queries (lock-free)
 */

#pragma once

#include "ECSComponents.h"
#include <JuceHeader.h>

namespace zenith {

/**
 * ECS Integration Layer
 * 
 * This class shows how to:
 * - Create entities from ProjectState
 * - Query entities in audio callback (real-time safe)
 * - Use Flecs Explorer for debugging
 */
class ECSEngine {
public:
    ECSEngine() 
        : world_(ecs::createZenithWorld())
    {
        DBG("ECSEngine: Initialized Flecs world");
    }
    
    //==========================================================================
    // MESSAGE THREAD ONLY: Entity Creation/Modification
    //==========================================================================
    
    /** Create a track entity from ProjectState */
    flecs::entity createTrack(const juce::String& name, const juce::String& trackId) {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        
        auto track = world_.entity(trackId.toStdString().c_str())
            .set<ecs::TrackState>({
                .volume = 0.8f,
                .pan = 0.0f,
                .muted = false,
                .solo = false,
                .armed = false,
                .color = juce::Colours::grey
            })
            .add<ecs::IsAudio>();  // or ecs::IsMIDI
        
        DBG("ECS: Created track entity: " + name);
        return track;
    }
    
    /** Create a clip entity (child of track) */
    flecs::entity createClip(
        flecs::entity track, 
        int64_t startSample, 
        int64_t lengthSample,
        const juce::String& audioFilePath = {})
    {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        
        auto clip = world_.entity()
            .child_of(track)  // Hierarchy: Clip belongs to Track
            .set<ecs::ClipState>({
                .startSample = startSample,
                .lengthSample = lengthSample,
                .gain = 1.0f,
                .looping = false
            });
        
        // Optionally link audio file
        if (audioFilePath.isNotEmpty()) {
            auto audioFile = world_.entity()
                .set<ecs::AudioFileRef>({
                    .filePath = audioFilePath,
                    .sampleRate = 44100,
                    .numChannels = 2
                });
            
            clip.add<ecs::References>(audioFile);  // Relationship: Clip -> AudioFile
        }
        
        return clip;
    }
    
    /** Update track volume (from UI) */
    void setTrackVolume(flecs::entity track, float volume) {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        
        track.get_mut<ecs::TrackState>()->volume = volume;
    }
    
    //==========================================================================
    // AUDIO THREAD: Real-Time Safe Queries
    //==========================================================================
    
    /** Example: Iterate all tracks (simplified for initial integration) */
    void processActiveTracksInAudioCallback(
        juce::AudioBuffer<float>& buffer,
        juce::int64 playheadSamples,
        int numSamples)
    {
        // Simplified version - just demonstrates Flecs integration works
        // In production, use cached queries for better performance
        world_.each([&](flecs::entity trackEntity, const ecs::TrackState& track) {
            // Apply track volume/pan
            float leftGain = track.volume * (1.0f - std::max(0.0f, track.pan));
            float rightGain = track.volume * (1.0f + std::min(0.0f, track.pan));
            
            // Process track audio here...
        });
    }
    
    //==========================================================================
    // DEBUGGING: Query Entity Hierarchy
    //==========================================================================
    
    /** Debug: Print entire entity tree */
    void debugPrintHierarchy() const {
        DBG("=== ECS Hierarchy ===");
        
        world_.each([](flecs::entity e) {
            juce::String indent = "";
            
            // Show parent relationship
            if (e.has(flecs::ChildOf, flecs::Wildcard)) {
                indent = "  ";
            }
            
            DBG(indent + juce::String(e.name().c_str()));
            
            // Show components
            if (e.has<ecs::TrackState>()) {
                auto* track = e.get<ecs::TrackState>();
                DBG(indent + "  Volume: " + juce::String(track->volume));
            }
            
            if (e.has<ecs::ClipState>()) {
                auto* clip = e.get<ecs::ClipState>();
                DBG(indent + "  Start: " + juce::String(clip->startSample));
            }
        });
    }
    
    //==========================================================================
    // Access
    //==========================================================================
    
    flecs::world& world() { return world_; }
    const flecs::world& world() const { return world_; }
    
private:
    flecs::world world_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ECSEngine)
};

} // namespace zenith
