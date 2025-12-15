# Multi-Agent Project Reorganization Fix Prompts

This document contains independent prompts for multiple agents to work in parallel to complete the Zenith DAW project reorganization.

---

## Agent 1: ZenithTheme Class Qualification Fix

### Branch: `fix/zenith-theme-qualification`

### Context:
The `ZenithTheme` is a **class**, not a namespace. Several files incorrectly use `using namespace ZenithTheme;` which causes compilation errors. All references to `Colors::`, `Typography::`, `Spacing::`, `Radius::`, and `Shadows::` must be qualified with `ZenithTheme::`.

### Task:
1. Create branch `fix/zenith-theme-qualification` from `feat/grid-visibility`
2. Find all files with `using namespace ZenithTheme`
3. For each file:
   - Remove the `using namespace ZenithTheme;` line
   - Replace all unqualified references:
     - `Colors::` → `ZenithTheme::Colors::`
     - `Typography::` → `ZenithTheme::Typography::`
     - `Spacing::` → `ZenithTheme::Spacing::`
     - `Radius::` → `ZenithTheme::Radius::`
     - `Shadows::` → `ZenithTheme::Shadows::`
     - `Animation::` → `ZenithTheme::Animation::`
     - `Components::` → `ZenithTheme::Components::`
4. Verify build compiles
5. Commit: `fix: qualify ZenithTheme nested types with class name`

### Files to Check:
```
apps/desktop/Source/ui/arranger/ModernTrackHeader.cpp
apps/desktop/Source/ui/arranger/ModernTimelineRuler.cpp
```

Search command: `grep -r "using namespace ZenithTheme" apps/desktop/Source/`

---

## Agent 2: Resolve Merge Conflicts

### Branch: `refactor/project-organization-merged`

### Context:
The `feat/grid-visibility` branch contains the project reorganization work but has conflicts with master. Need to carefully resolve conflicts and merge.

### Task:
1. Checkout `git checkout feat/grid-visibility`
2. Rebase onto master: `git rebase master`
3. For each conflict:
   - **CMakeLists.txt**: Keep the reorganized UI CMakeLists.txt structure
   - **SkiaManualIntegration.cmake**: Keep the updated version without duplicate source inclusions
   - **PianoRollComponent.h**: Accept both changes (master fixes + new location)
   - **ui/skia/CMakeLists.txt**: Delete this file (sources moved to ui/CMakeLists.txt)
4. Continue rebase until complete
5. Verify build: `cmake -B build && cmake --build build --target ZenithDAW --config Release`
6. Push branch

### Conflict Resolution Strategy:
- New UI domain structure (theirs) takes priority
- Master's bug fixes (ours) should be integrated
- Delete obsolete skia/ subdirectory references

---

## Agent 3: Complete Include Path Migration

### Branch: `fix/include-paths-complete`

### Context:
Some include paths still use relative paths that need to be simplified. The CMake include directories now cover all domains.

### Task:
1. Create branch from `feat/grid-visibility`
2. Run these search/replace patterns across `apps/desktop/Source/`:

```powershell
# Pattern 1: Any remaining ../ path includes
Get-ChildItem -Path "apps/desktop/Source" -Include "*.cpp","*.h" -Recurse | ForEach-Object {
    $content = Get-Content $_.FullName -Raw
    # Remove ../ prefixes from includes
    $content = $content -replace '#include\s+"\.\.\/([a-zA-Z0-9_\/]+\.h)"', '#include "$1"'
    Set-Content -Path $_.FullName -Value $content -NoNewline
}
```

3. Verify no `../` or `../../` includes remain (except for JUCE system headers)
4. Build and verify
5. Commit: `fix: complete include path migration to simple includes`

### Validation:
```bash
grep -r '#include "\.\./' apps/desktop/Source/ --include="*.cpp" --include="*.h" | grep -v "juce\|external"
```
Should return empty (no matches).

---

## Agent 4: CMake Library Separation

### Branch: `refactor/cmake-library-separation`

### Context:
The project currently builds as a monolithic executable. Separating into static libraries improves build times and enables headless use cases.

### Task:
1. Create branch from `feat/grid-visibility`
2. Create `apps/desktop/cmake/ZenithLibraries.cmake`:

```cmake
# ZenithCore - Engine without UI
add_library(ZenithCore STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/Engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/Track.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/Clip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/ProjectState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/RoutingGraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/MixerChannel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/AudioFilePool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/PluginHost.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/AuxBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/TempoMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/AudioRecorder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/AudioRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/RecordingManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/TrackFreeze.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine/ZenithLogger.cpp
)

target_include_directories(ZenithCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/Source/engine
)

target_link_libraries(ZenithCore PUBLIC
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_dsp
)
```

3. Update main CMakeLists.txt to include this file and link ZenithDAW to ZenithCore
4. Verify build
5. Commit: `refactor: extract ZenithCore static library from engine`

### Future Extensions:
- ZenithAudio (DSP, stem separation)
- ZenithUI (all UI components)

---

## Agent 5: Header Consolidation

### Branch: `refactor/header-consolidation`

### Context:
Headers are scattered across Source directories. Need to ensure each domain has its headers co-located with sources.

### Task:
1. Create branch from `feat/grid-visibility`
2. For each UI domain, verify headers exist alongside sources:
   - `ui/arranger/`: ArrangerComponent.h, etc.
   - `ui/mixer/`: MixerComponent.h, MixerChannelComponent.h, MixerView.h
   - `ui/piano-roll/`: PianoRollComponent.h, ModulationMatrixView.h
   - etc.
3. If any headers are missing from domains but exist elsewhere, move them with `git mv`
4. Update `apps/desktop/Source/ui/CMakeLists.txt` if needed
5. Verify build
6. Commit: `refactor: consolidate headers with their source files`

### Validation:
For each `.cpp` file, verify corresponding `.h` is in same directory:
```powershell
Get-ChildItem -Path "apps/desktop/Source/ui" -Include "*.cpp" -Recurse | ForEach-Object {
    $header = $_.FullName -replace '\.cpp$', '.h'
    if (-not (Test-Path $header)) {
        Write-Host "Missing header: $header"
    }
}
```

---

## Agent 6: Consolidate Duplicate Theme Systems

### Branch: `refactor/unify-theme-system`

### Context:
Multiple theme-related files exist that need consolidation:
- `ZenithTheme.h/cpp` - Main theme class
- `SkiaTheme.h` - Duplicate?
- `ZenithDesignSystem.h/cpp` - Extended design tokens
- `ZenithLookAndFeel.h/cpp` - JUCE LookAndFeel

### Task:
1. Create branch from master
2. Audit all theme files:
   ```
   apps/desktop/Source/ui/design-system/ZenithTheme.h
   apps/desktop/Source/ui/design-system/SkiaTheme.h
   apps/desktop/Source/ui/design-system/ZenithDesignSystem.h
   apps/desktop/Source/ui/design-system/ZenithLookAndFeel.h
   ```
3. Determine which can be merged:
   - If `SkiaTheme.h` duplicates `ZenithTheme.h`, delete it and update includes
   - If `ZenithDesignSystem.h` extends `ZenithTheme.h`, keep both but document relationship
4. Create unified include: `#include "Theme.h"` that includes all needed theme headers
5. Verify build
6. Commit: `refactor: consolidate duplicate theme definitions`

---

## Agent 7: Clean Up Old skia/ Directory

### Branch: `cleanup/remove-old-skia-dir`

### Context:
After reorganization, the old `ui/skia/` directory should be empty except for CMake artifacts. Need to fully remove it.

### Task:
1. Create branch from `feat/grid-visibility`
2. List contents of `apps/desktop/Source/ui/skia/`:
   - If only CMake files (CMakeFiles/, cmake_install.cmake, *.vcxproj), delete them
   - If source files remain, they were missed during reorganization - move them
3. Remove empty subdirectories: components/, config/, layout/, lifecycle/, settings/, testing/, views/, widgets/
4. Update any remaining references to `ui/skia/` path
5. Remove `add_subdirectory(skia)` from `ui/CMakeLists.txt` if present
6. Verify build
7. Commit: `cleanup: remove obsolete ui/skia directory structure`

---

## Execution Order

These agents can run **in parallel** except:
- Agent 2 (merge conflicts) should run first to establish baseline
- Agents 3-7 can run in parallel after Agent 2
- Agent 1 can run independently

### Recommended Sequence:
```
Phase 1 (Sequential):
  └── Agent 2: Resolve Merge Conflicts

Phase 2 (Parallel):
  ├── Agent 1: ZenithTheme Qualification
  ├── Agent 3: Include Path Migration
  ├── Agent 5: Header Consolidation
  ├── Agent 6: Theme Consolidation
  └── Agent 7: Clean Up skia/

Phase 3 (After Phase 2):
  └── Agent 4: CMake Library Separation
```

---

## Final Integration

After all agents complete:
1. Merge all fix branches into a single integration branch
2. Run full build: `cmake --build build --target ZenithDAW --config Release`
3. Run tests: `cmake --build build --target ZenithDAWTests --config Release`
4. If successful, merge to master with message:
   ```
   feat: complete project reorganization

   - Reorganized UI by domain (arranger, mixer, piano-roll, etc.)
   - Fixed all include paths for new structure
   - Unified theme system
   - Cleaned up obsolete directories
   - Prepared for library separation
   ```
