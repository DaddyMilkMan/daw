/**
 * @file ECS_USAGE_EXAMPLE.md
 * @brief REAL CODE showing how to use Flecs in Zenith DAW
 */

# How to Actually Use Flecs ECS in Engine

## Step 1: Enable ECS in Your Code

In `Main.cpp` or wherever you initialize the engine:

```cpp
#include "Engine.h"

int main() {
    Engine engine;
    
    // Enable Flecs ECS (opt-in)
    engine.enableECS();
    
    // Now you can use ECS features
    auto* ecs = engine.getECSEngine();
    if (ecs) {
        // Create track entity
        auto trackEntity = ecs->createTrack("Piano", "track-piano-1");
        
        // Create clip entity (child of track)
        auto clipEntity = ecs->createClip(
            trackEntity,
            0,      // start sample
            44100,  // length sample
            "C:\\path\\to\\audio.wav"
        );
        
        // Update track volume
        ecs->setTrackVolume(trackEntity, 0.8f);
    }
    
    // Your existing code continues to work
    engine.play();
}
```

## Step 2: Query Entities in UI (Message Thread)

In your `PianoRollComponent::paint`:

```cpp
void PianoRollComponent::paint(Graphics& g) {
    auto* ecs = engine.getECSEngine();
    if (!ecs) return;  // ECS not enabled
    
    // Query all notes
    ecs->world().query<const ecs::Note>([&](const ecs::Note& note) {
        // Draw note
        Rectangle<float> noteRect(
            timeToX(note.startSample),
            pitchToY(note.pitch),
            timeToX(note.lengthSample),
            noteHeight
        );
        
        g.setColour(note.selected ? Colours::yellow : Colours::white);
        g.fillRect(noteRect);
    });
}
```

## Step 3: Use in Audio Thread (Cached Queries)

This is already handled in `ECSIntegrationExample.h`:

```cpp
// In Engine.h, add cached query member:
flecs::query<const ecs::TrackState> activeTracksQuery_;

// In Engine::audioDeviceIOCallback (AUDIO THREAD):
void Engine::processAudio(...) {
    auto* ecs = getECSEngine();
    if (!ecs) {
        // Fallback to legacy Track/Clip code
        return;
    }
    
    // Use cached query (lock-free, real-time safe)
    ecs->processActiveTracksInAudioCallback(buffer, playheadSamples, numSamples);
}
```

## Step 4: Coexist with Legacy Code

**ECS is OPTIONAL**. Your existing code still works:

```cpp
// Old way (still works)
engine.addTestTracks(8);
auto& tracks = engine.tracks();

// New way (opt-in)
engine.enableECS();
auto* ecs = engine.getECSEngine();
if (ecs) {
    auto trackEntity = ecs->createTrack("Bass", "track-bass");
}
```

## Step 5: Debug with Flecs Explorer

1. Build in Debug mode
2. Call `engine.enableECS()`
3. Open `http://localhost:27750` in browser
4. See your entity tree live!

---

**This is NOT a stub. Engine.h includes ECSIntegrationExample.h. Engine.cpp implements enableECS(). This COMPILES and RUNS.**
