/**
 * @file ECSComponents.h
 * @brief Flecs ECS Component Definitions for Zenith DAW
 * 
 * Architecture Philosophy:
 * - ECS for STATE (Flecs entities/components)
 * - Classes for DSP (Track.cpp, Clip.cpp - keep existing code)
 * - Never run world.progress() in audio thread, only iterate cached queries
 */

#pragma once

#include <flecs.h>
#include <juce_core/juce_core.h>

namespace zenith::ecs {

//==============================================================================
// COMPONENTS (Pure Data Structures - No Logic)
//==============================================================================

/** Track mixer state */
struct TrackState {
    float volume = 0.8f;
    float pan = 0.0f;
    bool muted = false;
    bool solo = false;
    bool armed = false;
    juce::Colour color = juce::Colours::grey;
};

/** Clip position and timing */
struct ClipState {
    int64_t startSample = 0;
    int64_t lengthSample = 0;
    float gain = 1.0f;
    bool looping = false;
};

/** MIDI Note data */
struct Note {
    int pitch = 60;           // 0-127
    int velocity = 100;       // 0-127
    int64_t startSample = 0;
    int64_t lengthSample = 0;
    bool selected = false;
};

/** Audio file reference */
struct AudioFileRef {
    juce::String filePath;
    int64_t sampleRate = 44100;
    int numChannels = 2;
};

/** UI Selection state (for rendering) */
struct Selected {
    bool value = true;
};

/** UI Color (for rendering) */
struct Color {
    juce::Colour value = juce::Colours::white;
};

//==============================================================================
// RELATIONSHIPS (Flecs Hierarchies)
//==============================================================================

/** Parent-child relationship (built-in to Flecs via .child_of()) */
// Usage: world.entity("Clip1").child_of(track);

/** Reference relationship (e.g., Clip -> AudioFile) */
struct References {};

//==============================================================================
// TAGS (Zero-size markers)
//==============================================================================

struct IsPlaying {};
struct IsRecording {};
struct IsMIDI {};
struct IsAudio {};

//==============================================================================
// HELPER: Create World with Debugging (Debug builds only)
//==============================================================================

inline flecs::world createZenithWorld() {
    flecs::world world;
    
#ifdef JUCE_DEBUG
    // Enable Flecs Explorer on localhost:27750
    // Access via browser: http://localhost:27750
    world.set<flecs::Rest>({});
    world.import<flecs::monitor>();
    
    DBG("Flecs Explorer available at: http://localhost:27750");
#endif
    
    return world;
}

} // namespace zenith::ecs
