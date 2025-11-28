# CMakeLists.txt Merge Conflict Resolution Plan

## The Conflict

Lines 104-136 in `zenith-core/CMakeLists.txt` have conflicting additions:

### My Branch (HEAD)
```cmake
# Built-in instruments and macro/preset system
include/instruments/InstrumentMetadata.h
include/instruments/InstrumentPreset.h
include/instruments/InstrumentRegistry.h
include/instruments/InstrumentRegistration.h
src/instruments/InstrumentRegistration.cpp

# ZenithPolySynth
include/instruments/ZenithPolySynth.h
src/instruments/ZenithPolySynth.cpp
include/instruments/ZenithPolySynthEditor.h
src/instruments/ZenithPolySynthEditor.cpp

# ZenithSampler
include/instruments/ZenithSampler.h
src/instruments/ZenithSampler.cpp
include/instruments/ZenithSamplerEditor.h
src/instruments/ZenithSamplerEditor.cpp
```

### Master Branch
```cmake
# Instrument system
Source/instruments/InstrumentMetadata.h
Source/instruments/Instrument.h
Source/instruments/Instrument.cpp
Source/instruments/InstrumentRegistry.h
Source/instruments/InstrumentRegistry.cpp
Source/instruments/ZenithPolySynth.h
Source/instruments/ZenithPolySynth.cpp
Source/instruments/ZenithSampler.h
Source/instruments/ZenithSampler.cpp
Source/instruments/RegisterBuiltInInstruments.h
Source/instruments/RegisterBuiltInInstruments.cpp
include/InstrumentCommandHelpers.h
```

## Resolution Strategy

Since master's implementation is already merged (PR #100), we should:

1. **Accept master's file structure** (`Source/instruments/`)
2. **Add unique files from my implementation:**
   - `InstrumentPreset.h` - File-based preset system
   - UI editors (ZenithPolySynthEditor, ZenithSamplerEditor)
3. **Merge features into master's files:**
   - Add MacroEngine to master's InstrumentMetadata.h
   - Add preset file operations to master's Instrument class
   - Enhance master's ZenithPolySynth/Sampler with macro integration
   - Add UI editor support

## Recommended CMakeLists.txt Resolution

```cmake
    # Engine primitives
    Source/engine/Track.h
    Source/engine/Track.cpp
    Source/engine/Clip.h
    Source/engine/Clip.cpp
    Source/engine/MixerChannel.h
    Source/engine/MixerChannel.cpp

    # Instrument system (using master's structure)
    Source/instruments/InstrumentMetadata.h
    Source/instruments/Instrument.h
    Source/instruments/Instrument.cpp
    Source/instruments/InstrumentRegistry.h
    Source/instruments/InstrumentRegistry.cpp
    Source/instruments/ZenithPolySynth.h
    Source/instruments/ZenithPolySynth.cpp
    Source/instruments/ZenithSampler.h
    Source/instruments/ZenithSampler.cpp
    Source/instruments/RegisterBuiltInInstruments.h
    Source/instruments/RegisterBuiltInInstruments.cpp

    # Additional instrument features (from my branch)
    Source/instruments/InstrumentPreset.h         # NEW: Preset system
    Source/instruments/ZenithPolySynthEditor.h    # NEW: UI editor
    Source/instruments/ZenithPolySynthEditor.cpp  # NEW: UI editor
    Source/instruments/ZenithSamplerEditor.h      # NEW: UI editor
    Source/instruments/ZenithSamplerEditor.cpp    # NEW: UI editor

    # Command API integration
    include/InstrumentCommandHelpers.h
)
```

## Files to Migrate from My Branch

Move these files from `include/instruments/` and `src/instruments/` to `Source/instruments/`:

**New files to add:**
1. `InstrumentPreset.h` - Preset data structures and ZenithPresetManager
2. `ZenithPolySynthEditor.h/cpp` - Custom UI with macro knobs
3. `ZenithSamplerEditor.h/cpp` - Custom UI with macro knobs

**Features to merge into existing files:**
1. **InstrumentMetadata.h**: Add MacroEngine class from my implementation
2. **ZenithPolySynth.h/cpp**: Add macro integration, preset loading, custom editor
3. **ZenithSampler.h/cpp**: Add macro integration, preset loading, custom editor
4. **Instrument.h**: Add preset file operations interface

## Include Directory Update

```cmake
target_include_directories(ZenithDAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/instruments
)
```

Remove `include/instruments` since we're using `Source/instruments`.

## Next Steps

1. Resolve CMakeLists.txt conflict with above structure
2. Move UI editor files to Source/instruments/
3. Merge MacroEngine into master's InstrumentMetadata.h
4. Enhance master's instruments with macro/preset features
5. Test that everything builds
6. Verify CommandAPI still works
7. Test presets and UI editors

This preserves both implementations' best features while maintaining a unified structure.
