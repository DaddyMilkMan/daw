# Flecs ECS Integration Guide

## Architecture Overview

Zenith DAW uses a **hybrid architecture**:

- **Flecs ECS** → State management (tracks, clips, notes, selection)
- **Existing C++ Classes** → DSP/Audio processing (Track.cpp, Clip.cpp, Engine.cpp)
- **JUCE ValueTree** → Serialization/Undo (ProjectState.cpp)

**Why?** Flecs excels at *relationships* and *debugging*, not DSP. We use each tool for what it's best at.

---

## The "Web UI" Clarification

**Flecs Explorer is NOT your app's UI.** It's a debugging tool that runs on `localhost:27750`.

### What It Does:
- Shows entity hierarchy (Project → Tracks → Clips → Notes)
- Lets you inspect component values in real-time
- Visualizes relationships (which clip references which audio file)
- Monitors query performance

### What It Doesn't Do:
- Replace your Skia/JUCE UI
- Ship with the release build (debug-only)
- Require internet connection (fully local)

**Think of it like Visual Studio's debugger, but for your audio graph.**

---

## Data Model

### Entity Hierarchy

```
Project (root entity)
├── Track 1 (entity)
│   ├── Clip A (entity, child_of Track 1)
│   │   └── Note C4 (entity, child_of Clip A)
│   └── Clip B (entity, child_of Track 1)
│       └── References → AudioFile.wav (relationship)
└── Track 2 (entity)
    └── Clip C (entity, child_of Track 2)
```

### Components (POD structs)

```cpp
struct TrackState {
    float volume, pan;
    bool muted, solo, armed;
    juce::Colour color;
};

struct ClipState {
    int64_t startSample, lengthSample;
    float gain;
    bool looping;
};

struct Note {
    int pitch, velocity;
    int64_t startSample, lengthSample;
    bool selected;
};
```

---

## Real-Time Safety Rules

### ❌ NEVER in Audio Thread

```cpp
// BAD: Creates entities (allocates memory)
world.entity().set<TrackState>({...});

// BAD: Runs systems (not deterministic)
world.progress();

// BAD: Creates new query (allocates)
auto q = world.query<TrackState>();
```

### ✅ SAFE in Audio Thread

```cpp
// GOOD: Iterate cached query (lock-free, wait-free)
cachedQuery.each([](const TrackState& t) {
    // Process track...
});

// GOOD: Read component (const access)
if (const auto* clip = entity.get<ClipState>()) {
    processClip(*clip);
}

// GOOD: Iterate children (hierarchy is immutable during iteration)
trackEntity.children([](flecs::entity clip) {
    // Process clip...
});
```

---

## Integration Pattern

### 1. Create Entities (Message Thread)

When user adds a track in UI:

```cpp
void Engine::addTrack(const juce::String& name) {
    jassert(MessageManager::isThisTheMessageThread());
    
    // Create ECS entity
    auto trackEntity = ecsWorld_.entity(name.toStdString().c_str())
        .set<ecs::TrackState>({...})
        .add<ecs::IsAudio>();
    
    // Also create legacy Track object (for DSP)
    auto track = std::make_unique<zenith::Track>(name, Track::Type::Audio);
    tracks_.push_back(std::move(track));
    
    // Link them (store entity ID in Track)
    track->setEntityId(trackEntity.id());
}
```

### 2. Query Entities (Audio Thread)

In `audioDeviceIOCallback`:

```cpp
void Engine::processBlock(AudioBuffer<float>& buffer, int64_t playhead) {
    // Iterate cached query (created in constructor)
    activeTracksQuery_.each([&](flecs::entity e, const ecs::TrackState& state) {
        // Get linked DSP object
        auto* track = findTrackByEntityId(e.id());
        
        // Apply ECS state to DSP
        track->setVolume(state.volume);
        track->setPan(state.pan);
        
        // Process audio using existing Track.cpp code
        track->processBlock(buffer, playhead);
    });
}
```

### 3. Render UI (Message Thread)

In `PianoRollComponent::paint`:

```cpp
void PianoRollComponent::renderWithFlecs(SkCanvas* canvas) {
    // Query all selected notes
    ecsWorld_.query<const ecs::Note, const ecs::Selected>([&](const ecs::Note& note) {
        // Draw note with Skia
        SkRect noteRect = SkRect::MakeXYWH(
            timeToX(note.startSample),
            pitchToY(note.pitch),
            timeToX(note.lengthSample),
            noteHeight
        );
        
        canvas->drawRect(noteRect, selectedPaint_);
    });
}
```

---

## Performance Characteristics

### Flecs Archetypes = Cache Heaven

When you query `world.query<Position, Velocity>()`, Flecs stores matching entities in **archetype tables**:

```
Archetype Table [Position, Velocity]:
  Entity 0: Position{x:10, y:20}, Velocity{dx:1, dy:2}
  Entity 1: Position{x:15, y:25}, Velocity{dx:2, dy:3}
  Entity 2: Position{x:20, y:30}, Velocity{dx:3, dy:4}
  ...
```

All components are **contiguous in memory** → Perfect for:
- CPU cache prefetching
- SIMD vectorization (xsimd can process 8 entities at once)
- Rendering loops (Skia batch operations)

**Benchmark**: Iterating 100,000 notes with Flecs is **faster** than iterating `std::vector<Note>` because of cache locality.

---

## Migration Strategy

### Phase 1: Add Flecs (This PR)
- ✅ Integrate Flecs via CMake
- ✅ Create component definitions (`ECSComponents.h`)
- ✅ Add example integration (`ECSIntegrationExample.h`)
- ✅ Enable Flecs Explorer (debug builds)

### Phase 2: Hybrid Mode (Next)
- Keep existing Track/Clip classes for DSP
- Use Flecs for **new features**:
  - Piano roll note storage
  - Automation point storage
  - Selection state
- Sync ECS ↔ JUCE ValueTree

### Phase 3: Full ECS (Future)
- Refactor Track/Clip to be pure ECS
- Remove manual `std::vector` management
- Use Flecs queries everywhere

---

## Debugging with Flecs Explorer

### Step 1: Enable Explorer (Already Done)

In `ECSComponents.h`:

```cpp
#ifdef JUCE_DEBUG
    world.set<flecs::Rest>({});
    world.import<flecs::monitor>();
#endif
```

### Step 2: Run Zenith DAW (Debug Build)

```bash
cmake --build build --config Debug
./build/Debug/ZenithDAW.exe
```

### Step 3: Open Browser

Navigate to: `http://localhost:27750`

You'll see:
- **Entity Tree**: Hierarchical view of all entities
- **Component Inspector**: Click entity → see all component values
- **Query Debugger**: See which queries are running and their cache hit rate
- **Performance Monitor**: CPU time per system

### Example Workflow

1. User reports: "Track 3 doesn't play after undo"
2. Open Flecs Explorer → Find "Track 3" entity
3. Inspect components:
   - `TrackState.muted = true` ← **Found the bug!**
4. Fix undo system to restore mute state correctly

**Time saved: Hours → Minutes** 🎯

---

## FAQ

### Q: Does this replace JUCE ValueTree?
**A:** No. Keep ValueTree for serialization/undo. Use Flecs for runtime state.

### Q: Do I need to refactor my entire codebase?
**A:** No. Add Flecs incrementally. Start with piano roll notes.

### Q: What if I want to disable the web server (security)?
**A:** It's already debug-only. Release builds have zero web server code.

### Q: Can I use Flecs in VST3 plugins?
**A:** Yes. Flecs compiles to a static library. Just disable the REST module.

### Q: Is Flecs thread-safe?
**A:** **Reads are lock-free** (safe in audio thread). **Writes require staging** (use message thread).

---

## Next Steps

1. ✅ **Build the project** (Flecs will download automatically)
2. 🧪 **Test the example** (see `ECSIntegrationExample.h`)
3. 🎨 **Open Flecs Explorer** (http://localhost:27750 in debug build)
4. 🚀 **Migrate piano roll** (use `Note` component for storage)

---

## Resources

- [Flecs Documentation](https://www.flecs.dev/flecs/)
- [Flecs Query Manual](https://www.flecs.dev/flecs/md_docs_2Queries.html)
- [Flecs Relationships Guide](https://www.flecs.dev/flecs/md_docs_2Relationships.html)
- [Flecs Explorer Demo](https://www.flecs.dev/explorer/)

---

**Questions?** Check existing entities:

```cpp
ecsEngine_.debugPrintHierarchy();  // See ECSIntegrationExample.h
```
