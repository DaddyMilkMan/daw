# Zenith DAW Project Reorganization Plan

## Executive Summary
This document outlines the step-by-step plan to reorganize the Zenith DAW codebase from a monolithic structure into a modular, maintainable architecture.

## Current State (Before)

```
apps/desktop/
├── Source/
│   ├── ai/                    # 12 files
│   ├── browser/               # 9 files
│   ├── commands/              # 11 files
│   ├── dsp/                   # 15 files
│   ├── engine/                # 50 files (DISORGANIZED)
│   ├── instruments/           # 34 files
│   ├── network/               # 23 files
│   ├── pch/                   # 1 file
│   ├── rendering/             # 3 files
│   ├── tests/                 # 9 files
│   ├── ui/                    # 160 files (MEGA-FOLDER)
│   │   ├── skia/              # Mixed widgets/components
│   │   └── views/             # 6 files
│   └── utils/                 # 6 files
├── include/                   # 33 files (INCONSISTENT)
│   └── ui/                    # 18 files
└── Resources/                 # 114 files
```

## Target State (After)

```
apps/desktop/
├── src/                       # All source code (.cpp)
│   ├── app/                   # Application entry point
│   │   └── Main.cpp
│   ├── core/                  # Core engine (no UI deps)
│   │   ├── engine/
│   │   ├── audio/
│   │   └── routing/
│   ├── audio/                 # Audio processing
│   │   ├── recorder/
│   │   ├── renderer/
│   │   └── dsp/
│   ├── instruments/           # Built-in instruments
│   │   ├── polysynth/
│   │   ├── sampler/
│   │   └── effects/
│   ├── ai/                    # AI agents
│   │   ├── agents/
│   │   └── providers/
│   ├── network/               # Network services
│   │   ├── clients/
│   │   └── collab/
│   ├── ui/                    # User interface
│   │   ├── framework/         # Base components
│   │   ├── design-system/     # Theming, fonts
│   │   ├── arranger/          # Arranger view
│   │   ├── mixer/             # Mixer view
│   │   ├── piano-roll/        # MIDI editor
│   │   ├── browser/           # File browser
│   │   ├── transport/         # Transport bar
│   │   └── widgets/           # Reusable widgets
│   └── tests/                 # Unit tests
├── include/zenith/            # Public API headers
│   ├── core/
│   ├── audio/
│   └── ui/
├── resources/                 # Fonts, icons, samples
│   ├── fonts/
│   ├── icons/
│   └── presets/
└── cmake/                     # CMake modules
```

---

## Phase 1: Cleanup (COMPLETED ✅)
- [x] Delete `build_*` folders
- [x] Move log files to `logs/`
- [x] Delete empty `modules/zenith-core/`
- [x] Update `.gitignore`

---

## Phase 2: UI Reorganization

### 2.1 Create New UI Directory Structure
```powershell
# Create new folders
mkdir -p apps/desktop/Source/ui/framework
mkdir -p apps/desktop/Source/ui/design-system
mkdir -p apps/desktop/Source/ui/arranger
mkdir -p apps/desktop/Source/ui/mixer
mkdir -p apps/desktop/Source/ui/piano-roll
mkdir -p apps/desktop/Source/ui/browser
mkdir -p apps/desktop/Source/ui/transport
mkdir -p apps/desktop/Source/ui/common
```

### 2.2 Move Files by Domain

#### Framework (Base Components)
| Current Location | New Location |
|------------------|--------------|
| `ui/skia/SkiaComponent.cpp` | `ui/framework/SkiaComponent.cpp` |
| `ui/skia/SkiaComponent.h` | `ui/framework/SkiaComponent.h` |
| `ui/skia/RenderTree.h` | `ui/framework/RenderTree.h` |
| `ui/skia/SkiaAccessibility.h` | `ui/framework/SkiaAccessibility.h` |

#### Design System
| Current Location | New Location |
|------------------|--------------|
| `ui/skia/ZenithDesignSystem.cpp` | `ui/design-system/ZenithDesignSystem.cpp` |
| `ui/skia/ZenithDesignSystem.h` | `ui/design-system/ZenithDesignSystem.h` |
| `ui/skia/FontManager.cpp` | `ui/design-system/FontManager.cpp` |
| `ui/skia/FontManager.h` | `ui/design-system/FontManager.h` |
| `ui/skia/ZenithIcons.h` | `ui/design-system/ZenithIcons.h` |
| `ui/ZenithTheme.cpp` | `ui/design-system/ZenithTheme.cpp` |
| `ui/ZenithTheme.h` | `ui/design-system/ZenithTheme.h` |
| `ui/ZenithLookAndFeel.cpp` | `ui/design-system/ZenithLookAndFeel.cpp` |
| `ui/ZenithLookAndFeel.h` | `ui/design-system/ZenithLookAndFeel.h` |

#### Arranger View
| Current Location | New Location |
|------------------|--------------|
| `ui/ArrangerComponent.cpp` | `ui/arranger/ArrangerComponent.cpp` |
| `ui/ArrangerTrackComponent.cpp` | `ui/arranger/ArrangerTrackComponent.cpp` |
| `ui/TrackHeaderComponent.cpp` | `ui/arranger/TrackHeaderComponent.cpp` |
| `ui/ModernTrackHeader.cpp` | `ui/arranger/ModernTrackHeader.cpp` |
| `ui/ClipComponent.cpp` | `ui/arranger/ClipComponent.cpp` |
| `ui/AutomationLaneComponent.cpp` | `ui/arranger/AutomationLaneComponent.cpp` |
| `ui/TimelineRuler.cpp` | `ui/arranger/TimelineRuler.cpp` |
| `ui/ModernTimelineRuler.cpp` | `ui/arranger/ModernTimelineRuler.cpp` |
| `ui/MarkerLaneComponent.cpp` | `ui/arranger/MarkerLaneComponent.cpp` |
| `ui/TempoLaneComponent.cpp` | `ui/arranger/TempoLaneComponent.cpp` |
| `ui/MiniMapComponent.cpp` | `ui/arranger/MiniMapComponent.cpp` |

#### Mixer View
| Current Location | New Location |
|------------------|--------------|
| `ui/MixerComponent.cpp` | `ui/mixer/MixerComponent.cpp` |
| `ui/MixerChannelComponent.cpp` | `ui/mixer/MixerChannelComponent.cpp` |
| `ui/MixerView.cpp` | `ui/mixer/MixerView.cpp` |

#### Piano Roll / MIDI Editor
| Current Location | New Location |
|------------------|--------------|
| `ui/PianoRollComponent.cpp` | `ui/piano-roll/PianoRollComponent.cpp` |
| `ui/views/ModulationMatrixView.cpp` | `ui/piano-roll/ModulationMatrixView.cpp` |
| `ui/QuantizeSettingsComponent.h` | `ui/piano-roll/QuantizeSettingsComponent.h` |

#### Browser
| Current Location | New Location |
|------------------|--------------|
| `ui/skia/BrowserPanel.cpp` | `ui/browser/BrowserPanel.cpp` |
| `ui/skia/BrowserPanel.h` | `ui/browser/BrowserPanel.h` |
| `ui/PluginBrowserComponent.cpp` | `ui/browser/PluginBrowserComponent.cpp` |
| `ui/PresetBrowserComponent.cpp` | `ui/browser/PresetBrowserComponent.cpp` |
| `ui/InstrumentBrowserPanel.cpp` | `ui/browser/InstrumentBrowserPanel.cpp` |

#### Transport
| Current Location | New Location |
|------------------|--------------|
| `ui/skia/TransportBar.cpp` | `ui/transport/TransportBar.cpp` |
| `ui/skia/TransportBar.h` | `ui/transport/TransportBar.h` |

#### Common/Shared
| Current Location | New Location |
|------------------|--------------|
| `ui/skia/BottomBar.cpp` | `ui/common/BottomBar.cpp` |
| `ui/skia/RightSidePanel.cpp` | `ui/common/RightSidePanel.cpp` |
| `ui/ResizablePanelContainer.cpp` | `ui/common/ResizablePanelContainer.cpp` |
| `ui/MainLayoutComponent.cpp` | `ui/common/MainLayoutComponent.cpp` |
| `ui/MenuBar.cpp` | `ui/common/MenuBar.cpp` |
| `ui/ZenithHubComponent.cpp` | `ui/common/ZenithHubComponent.cpp` |
| `ui/WingmanPanel.cpp` | `ui/common/WingmanPanel.cpp` |

---

## Phase 3: Header Consolidation

### Decision: Keep Headers with Sources
We will use the "modern CMake" pattern where headers live alongside their source files.

### Actions:
1. Move all headers from `include/ui/*.h` to their corresponding `Source/ui/*/` domains
2. Update include paths in CMakeLists.txt
3. Create a single `include/zenith/` directory for truly public API headers only

---

## Phase 4: CMake Library Separation

### New Target Structure:
```cmake
# Core library (no UI)
add_library(ZenithCore STATIC
    src/core/engine/Engine.cpp
    src/core/engine/Track.cpp
    src/core/engine/Clip.cpp
    src/core/routing/RoutingGraph.cpp
    # ...
)

# Audio library
add_library(ZenithAudio STATIC
    src/audio/recorder/AudioRecorder.cpp
    src/audio/renderer/AudioRenderer.cpp
    src/audio/dsp/DSPStemSeparator.cpp
    # ...
)

# UI library (depends on Core)
add_library(ZenithUI STATIC
    src/ui/framework/SkiaComponent.cpp
    src/ui/arranger/ArrangerComponent.cpp
    # ...
)

# Main executable
add_executable(ZenithDAW
    src/app/Main.cpp
)
target_link_libraries(ZenithDAW PRIVATE ZenithCore ZenithAudio ZenithUI)
```

---

## Phase 5: Template Consolidation

### Files to Merge or Delete:
1. `ui/skia/SkiaTheme.h` → MERGE INTO `ui/design-system/ZenithDesignSystem.h`
2. `ui/skia/views/` → DELETE (duplicate of `ui/views/`)
3. Duplicate constants:
   - `engine/EngineConstants.h`
   - `engine/AudioConstants.h` → CREATE SINGLE `core/Constants.h`

---

## Execution Checklist

- [x] Phase 1: Cleanup
- [ ] Phase 2: UI Reorganization
  - [ ] Create directory structure
  - [ ] Move framework files
  - [ ] Move design-system files
  - [ ] Move arranger files
  - [ ] Move mixer files
  - [ ] Move piano-roll files
  - [ ] Move browser files
  - [ ] Move transport files
  - [ ] Move common files
  - [ ] Update CMakeLists.txt
- [ ] Phase 3: Header Consolidation
  - [ ] Move include/ui/*.h to Source/ui/*/
  - [ ] Create include/zenith/ for public API
- [ ] Phase 4: CMake Library Separation
  - [ ] Create ZenithCore target
  - [ ] Create ZenithAudio target
  - [ ] Create ZenithUI target
  - [ ] Update test dependencies
- [ ] Phase 5: Template Consolidation
  - [ ] Merge duplicate theme files
  - [ ] Create unified Constants.h
- [ ] Final Verification
  - [ ] Build passes
  - [ ] Tests pass
  - [ ] No broken includes

---

## Risk Mitigation

1. **Incremental commits**: Each phase is committed separately
2. **Build verification**: Build tested after each major change
3. **Git history**: All moves use `git mv` to preserve history
4. **Rollback plan**: Branch can be abandoned if issues arise

---

## Estimated Effort

| Phase | Time | Files Affected |
|-------|------|----------------|
| Phase 1 | ✅ Done | 10+ |
| Phase 2 | 2-3 hours | 80+ |
| Phase 3 | 1 hour | 30+ |
| Phase 4 | 2 hours | CMake files |
| Phase 5 | 1 hour | 5-10 |

**Total**: ~6-7 hours of careful refactoring

---

## Notes

- All moves must update `#include` paths
- CMakeLists.txt in subdirectories must be updated
- Headers in `include/` that are part of public API stay there
- Test files remain in `tests/` but may need include updates
