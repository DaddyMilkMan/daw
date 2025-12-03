# Industry Safe Stack for 2025 - Implementation Plan

## ✅ Current Stack Status

### 1. **Concurrency Layer: Moodycamel** ✅ RECOMMENDED
- **Status**: ⏳ Not yet integrated
- **Why**: MPMC queue handles chaos (multiple UI threads → audio thread)
- **Action**: Add to CMakeLists.txt

### 2. **Data Layer: Flecs (Archetype ECS)** ✅ INTEGRATED
- **Status**: ✅ Installed (v4.0.3)
- **Why**: Built-in relationships + debugging tools + cache-friendly iteration
- **Files**: 
  - `ECSComponents.h` - Component definitions
  - `ECSIntegrationExample.h` - Working example
  - `FLECS_INTEGRATION_GUIDE.md` - Full documentation

### 3. **Math Layer: xsimd** ✅ RECOMMENDED
- **Status**: ⏳ Not yet integrated
- **Why**: Standard for audio DSP, integrates with JUCE
- **Action**: Add to CMakeLists.txt

### 4. **Serialization: FlatBuffers** ✅ RECOMMENDED
- **Status**: ⏳ Not yet integrated
- **Why**: Zero-copy deserialization, perfect for project files
- **Action**: Add to CMakeLists.txt

---

## 🚀 Integration Roadmap

### Phase 1: Flecs Foundation ✅ COMPLETE
**Files Created**:
- `apps/desktop/include/ECSComponents.h` - Entity/component definitions
- `apps/desktop/include/ECSIntegrationExample.h` - Usage patterns
- `docs/FLECS_INTEGRATION_GUIDE.md` - Architecture documentation
- `docs/FLECS_INTEGRATION_SUMMARY.md` - Quick reference

**CMake Changes**:
```cmake
FetchContent_Declare(flecs ...)
target_link_libraries(ZenithDAW PRIVATE flecs::flecs_static)
```

**What You Get**:
- 🎯 Entity-component system for state management
- 🐛 Flecs Explorer at http://localhost:27750 (debug builds)
- 🚀 Cache-friendly iteration (perfect for Skia rendering)
- 🔗 Built-in relationships (Project → Track → Clip → Note)

---

### Phase 2: Moodycamel (Concurrency) ⏳ NEXT

**Use Case**: Thread-safe communication between UI and audio thread.

**Example**:
```cpp
// UI Thread: Send command to audio thread
struct SetVolumeCommand { int trackId; float volume; };

moodycamel::ConcurrentQueue<SetVolumeCommand> commandQueue;

// UI thread
commandQueue.enqueue({ trackId: 3, volume: 0.8f });

// Audio thread (in processBlock)
SetVolumeCommand cmd;
while (commandQueue.try_dequeue(cmd)) {
    tracks_[cmd.trackId]->setVolume(cmd.volume);
}
```

**Integration**:
```cmake
FetchContent_Declare(
    moodycamel
    GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
    GIT_TAG v1.0.4
)
FetchContent_MakeAvailable(moodycamel)

target_link_libraries(ZenithDAW PRIVATE moodycamel::concurrentqueue)
```

**Files to Create**:
- `include/CommandQueue.h` - Wrapper around moodycamel queue
- `docs/COMMAND_QUEUE_GUIDE.md` - Usage patterns

---

### Phase 3: xsimd (SIMD Math) ⏳ LATER

**Use Case**: Vectorized DSP operations (process 8 samples at once).

**Example**:
```cpp
#include <xsimd/xsimd.hpp>

// Process 8 samples at once
void applyGain(float* buffer, int numSamples, float gain) {
    using batch = xsimd::batch<float, xsimd::default_arch>;
    
    for (int i = 0; i < numSamples; i += batch::size) {
        auto samples = batch::load_unaligned(&buffer[i]);
        auto result = samples * gain;
        result.store_unaligned(&buffer[i]);
    }
}
```

**Integration**:
```cmake
find_package(xsimd CONFIG REQUIRED)
target_link_libraries(ZenithDAW PRIVATE xsimd)
```

**When to Use**:
- Track mixer (parallel gain/pan operations)
- Automation curves (linear interpolation across buffers)
- Audio effects (filters, EQ, reverb tails)

---

### Phase 4: FlatBuffers (Serialization) ⏳ LATER

**Use Case**: Fast project file save/load with zero-copy reads.

**Example Schema** (`project.fbs`):
```flatbuffers
namespace zenith;

table Track {
    id: string;
    name: string;
    volume: float;
    pan: float;
    clips: [Clip];
}

table Clip {
    start_sample: long;
    length_sample: long;
    audio_file: string;
}

table Project {
    tempo: float;
    sample_rate: int;
    tracks: [Track];
}

root_type Project;
```

**Integration**:
```cmake
find_package(flatbuffers CONFIG REQUIRED)
target_link_libraries(ZenithDAW PRIVATE flatbuffers::flatbuffers)
```

**Files to Create**:
- `schemas/project.fbs` - FlatBuffers schema
- `include/ProjectSerializer.h` - Save/load logic

---

## 🎯 Recommended Order

| Phase | Library | Priority | Complexity | Impact |
|-------|---------|----------|------------|--------|
| ✅ 1 | **Flecs** | Critical | Medium | High (debugging, relationships) |
| ⏳ 2 | **Moodycamel** | High | Low | Medium (thread safety) |
| ⏳ 3 | **xsimd** | Medium | Medium | Medium (DSP performance) |
| ⏳ 4 | **FlatBuffers** | Low | High | Low (save/load works with JUCE) |

**Why This Order?**

1. **Flecs First**: Establishes data model (entities, relationships)
2. **Moodycamel Next**: Enables safe UI → Audio communication
3. **xsimd Later**: Optimize DSP after core features work
4. **FlatBuffers Last**: Current JUCE ValueTree works fine for now

---

## 🔥 The Stack in Action

### Complete Example: Piano Roll with Full Stack

```cpp
// === FLECS: Note storage ===
world.entity("Note_C4")
    .child_of(clip)
    .set<ecs::Note>({ pitch: 60, velocity: 100, ... })
    .add<ecs::Selected>();

// === MOODYCAMEL: UI → Audio commands ===
struct AddNoteCommand { int pitch; int64_t start; };
commandQueue.enqueue({ pitch: 60, start: playhead });

// === XSIMD: Vectorized rendering ===
using batch = xsimd::batch<float, 8>;
for (int i = 0; i < numNotes; i += 8) {
    auto x = batch::load(&notePositions[i]);
    auto y = batch(&notePitches[i]) * noteHeight;
    // Draw 8 notes at once
}

// === FLATBUFFERS: Save project ===
auto project = CreateProject(builder, 
    tempo, sampleRate, trackVector);
builder.Finish(project);
file.write(builder.GetBufferPointer(), builder.GetSize());
```

---

## 📊 Performance Targets

With the full stack, you should achieve:

| Metric | Target | Current | Stack Component |
|--------|--------|---------|-----------------|
| Piano roll draw (100K notes) | <4ms @ 144fps | ? | **Flecs** (iteration) + **xsimd** (SIMD) |
| UI command latency | <100μs | ? | **Moodycamel** (queue) |
| Audio buffer process | <1ms @ 128 samples | ? | **xsimd** (SIMD DSP) |
| Project load (100 tracks) | <200ms | ? | **FlatBuffers** (zero-copy) |

---

## 🛠️ Next Actions

### Immediate (Today)
1. ✅ **Test Flecs integration**: Run debug build, open http://localhost:27750
2. ✅ **Read integration guide**: `docs/FLECS_INTEGRATION_GUIDE.md`
3. ✅ **Experiment with example**: `ECSIntegrationExample.h`

### This Week
1. ⏳ **Integrate Moodycamel**: Add command queue for UI → Audio messages
2. ⏳ **Refactor one feature to ECS**: Convert piano roll notes to Flecs entities
3. ⏳ **Benchmark Flecs queries**: Confirm cache-friendly iteration performance

### This Month
1. ⏳ **Add xsimd**: Vectorize mixer gain/pan operations
2. ⏳ **Profile with Flecs Explorer**: Identify slow queries
3. ⏳ **Document patterns**: Add real-world examples to codebase

---

## 📚 Resources

### Flecs
- [Official Docs](https://www.flecs.dev/flecs/)
- [Query Manual](https://www.flecs.dev/flecs/md_docs_2Queries.html)
- [Relationships Guide](https://www.flecs.dev/flecs/md_docs_2Relationships.html)

### Moodycamel
- [GitHub](https://github.com/cameron314/concurrentqueue)
- [Benchmarks](https://github.com/cameron314/concurrentqueue#benchmarks)

### xsimd
- [GitHub](https://github.com/xtensor-stack/xsimd)
- [Tutorial](https://xsimd.readthedocs.io/en/latest/)

### FlatBuffers
- [Official Docs](https://flatbuffers.dev/)
- [Schema Reference](https://flatbuffers.dev/flatbuffers_guide_writing_schema.html)

---

## ✅ Decision: Flecs is Installed

**The fork in the road is resolved.** You chose **Flecs** for:
- 🎨 Cache-friendly rendering (perfect for Skia + 100K notes)
- 🐛 Built-in debugging (Flecs Explorer saves weeks)
- 🔗 Native relationships (DAW hierarchy just works)
- 🚀 Performance (archetypes beat sparse sets for iteration)

**The other stack components** (Moodycamel, xsimd, FlatBuffers) will integrate smoothly as you need them.

---

**You're now on the Industry Safe Stack for 2025** 🎯✨
