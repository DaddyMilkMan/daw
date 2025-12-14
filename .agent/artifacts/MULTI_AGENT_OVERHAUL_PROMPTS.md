# Zenith DAW: Multi-Agent Overhaul Mission
## 5 Parallel Agent Prompts for A+ Grade Achievement

**Generated:** 2025-12-11
**Priority:** CRITICAL
**Objective:** Transform a non-compiling, architecturally challenged DAW codebase into a clean, buildable, professional project.

---

# 🔧 AGENT 1: Build Surgeon
## Mission: Fix All Compilation Errors and Establish Green Master

### Context
The Zenith DAW project at `c:\zenith\daw` currently **does not compile**. Build errors exist in `MainWindow.cpp`, and potentially other files. The project uses JUCE 8.0, Skia for rendering, CMake as the build system, and targets Windows with Visual Studio 2022.

### Your Tasks

#### Task 1.1: Fix MainWindow.cpp Errors
The following errors exist in `apps/desktop/Source/ui/MainWindow.cpp`:
- **Line 419**: Incorrect use of `juce::Rectangle::translated()` - check JUCE 8 API
- **Line 424**: `getRelativeAsRectangle()` does not exist - find correct JUCE method
- **Line 426**: Variable `relative` used before initialization
- **Line 701**: Type mismatch with `std::unique_ptr<Engine>`

**Actions:**
1. View `MainWindow.cpp` lines 410-440 and 695-710
2. Check JUCE 8.0 Rectangle API documentation
3. Fix the coordinate/bounds calculation logic
4. Fix the unique_ptr assignment issue (likely needs `std::move()` or ownership fix)

#### Task 1.2: Full Build Verification
After MainWindow fixes:
```powershell
cd c:\zenith\daw\build
cmake --build . --config Release --parallel 2>&1 | Tee-Object build_agent1.log
```

If additional errors appear, fix them iteratively. Common issues to watch for:
- Missing includes
- Skia API changes
- JUCE 8 breaking changes from JUCE 7
- Template instantiation errors from `/bigobj`

#### Task 1.3: Fix Any Header Encoding Issues
Some headers like `MixerChannel.h` have UTF-16 encoding issues. Convert to UTF-8:
```powershell
Get-ChildItem -Path "apps/desktop/Source" -Recurse -Include "*.h","*.cpp" | ForEach-Object {
    $content = Get-Content $_.FullName -Raw -Encoding Unicode -ErrorAction SilentlyContinue
    if ($content) {
        Set-Content $_.FullName -Value $content -Encoding UTF8
    }
}
```

#### Task 1.4: Establish CI Green State
Once building:
1. Run the test suite if `BUILD_TESTS=ON`
2. Document any runtime crashes or assertion failures
3. Create a `BUILD_STATUS.md` in `.agent/artifacts/` confirming green build

### Success Criteria
- [ ] `cmake --build . --config Release` completes with 0 errors
- [ ] Application launches without immediate crash
- [ ] Build log saved and clean

### Files You'll Modify
- `apps/desktop/Source/ui/MainWindow.cpp`
- Potentially: `apps/desktop/Source/engine/Engine.h`, any files with encoding issues
- Create: `.agent/artifacts/BUILD_STATUS.md`

---

# 🏗️ AGENT 2: Architecture Surgeon  
## Mission: Decompose God Objects and Establish Clean Threading Model

### Context
The codebase suffers from massive god objects:
- `Engine.cpp`: 1830 lines, 106 methods
- `ProjectState.cpp`: 2302 lines, 121 methods
- `ArrangerComponent.cpp`: 2094 lines, 92 methods

There's also an inconsistent threading model where atomic operations are wrapped in message-thread assertions.

### Your Tasks

#### Task 2.1: Complete Engine.cpp Decomposition
The refactoring to `AudioRenderer`, `RecordingManager`, and `TransportController` was started but not finished. Complete it:

1. **Audit current delegation**: Check which methods in `Engine.cpp` already delegate to the managers
2. **Move remaining audio processing**: All `processBlock`, `audioDeviceIOCallback`, metering logic → `AudioRenderer`
3. **Move transport logic**: All playhead, loop region, tempo sync → `TransportController`
4. **Move recording logic**: All arm state, file creation, baking → `RecordingManager`

**Target**: `Engine.cpp` should be <400 lines - a facade that coordinates the managers.

#### Task 2.2: Split ProjectState.cpp
Current: 2302 lines doing track management, clip management, automation, tempo, undo/redo, file I/O.

Create these focused modules:
```
ProjectState.cpp          → Core tree management, undo/redo (~300 lines)
TrackStateManager.cpp     → addTrack, removeTrack, getTrack* methods
ClipStateManager.cpp      → addClip, removeClip, clip queries
AutomationStateManager.cpp → Automation points, lanes
ProjectFileIO.cpp         → loadFromFile, saveToFile, saveCrashDump
```

#### Task 2.3: Establish Threading Contract
Create `docs/THREADING_MODEL.md` documenting:

1. **Audio Thread** (real-time, lock-free):
   - Reads from atomic state only
   - Uses track snapshots (`trackSnapshot_`)
   - Never allocates, never blocks

2. **Message Thread** (JUCE main thread):
   - All UI callbacks
   - All ProjectState mutations
   - All Engine control methods

3. **Background Threads**:
   - File I/O (AudioFilePool loading)
   - Plugin scanning
   - AI agent work

Then **remove conflicting patterns**:
- If a method uses atomics, remove `jassert(isMessageThread)`
- If a method requires message thread, don't use atomics internally

#### Task 2.4: Kill the Synchronizer Explosion
You have 4 synchronizers doing ValueTree ↔ Engine sync. Consolidate to **one**:

1. Create `ProjectEngineBridge.cpp` that:
   - Listens to `ProjectState` ValueTree changes
   - Applies them to `Engine` in batched updates
   - Handles the reverse (Engine → ProjectState) via explicit commit calls

2. Delete or deprecate:
   - `ClipSynchronizer.cpp`
   - `TrackStateSynchronizer.cpp`  
   - `TrackAutomationSynchronizer.cpp`
   - `TempoMapSynchronizer.cpp`

### Success Criteria
- [ ] `Engine.cpp` < 400 lines
- [ ] `ProjectState.cpp` < 400 lines
- [ ] Single `ProjectEngineBridge` class handles all sync
- [ ] `THREADING_MODEL.md` exists and is accurate
- [ ] No mixing of atomic + message-thread assertions

### Files You'll Create/Modify
- `apps/desktop/Source/engine/Engine.cpp` (shrink)
- `apps/desktop/Source/engine/ProjectState.cpp` (split)
- Create: `TrackStateManager.cpp/.h`
- Create: `ClipStateManager.cpp/.h`
- Create: `ProjectEngineBridge.cpp/.h`
- Create: `docs/THREADING_MODEL.md`

---

# 🌿 AGENT 3: Git Hygiene Specialist
## Mission: Clean Repository State and Establish Sustainable Branch Strategy

### Context
The repository has 33 branches including:
- Orphaned AI experiment branches (`cursor/cloud-agent-*`)
- "god-merge" panic branches
- Multiple fix/ and feature/ branches with overlapping work
- Currently in detached HEAD state rebasing master

### Your Tasks

#### Task 3.1: Escape Detached HEAD State
```powershell
cd c:\zenith\daw
git status
git rebase --abort  # If mid-rebase
git checkout master
git pull origin master
```

Document any conflicts or issues encountered.

#### Task 3.2: Audit All Branches
For each branch, determine:
1. Last commit date
2. Commits ahead/behind master
3. Whether it contains unique valuable work
4. Recommendation: KEEP, MERGE, or DELETE

Create `.agent/artifacts/BRANCH_AUDIT.md` with a table:
```markdown
| Branch | Last Activity | Status | Recommendation | Notes |
|--------|--------------|--------|----------------|-------|
| fix/engine-core | 2025-12-10 | 3 ahead, 0 behind | MERGE | Has PDC fixes |
| cursor/cloud-agent-* | 2025-12-11 | 50 ahead | DELETE | AI experiment garbage |
```

#### Task 3.3: Execute Branch Cleanup
After audit approval:
```powershell
# Delete local branches
git branch -D cursor/cloud-agent-1765080378742-rdl5e
# ... etc

# Delete remote branches
git push origin --delete cursor/cloud-agent-1765080378742-rdl5e
# ... etc
```

Keep only:
- `master` (stable)
- `develop` (integration) - create if doesn't exist
- Max 3-5 active feature branches

#### Task 3.4: Integrate Valuable Work
For branches marked MERGE:
1. Create fresh branch from master: `git checkout -b integration/final-merge`
2. Cherry-pick or merge valuable commits
3. Resolve conflicts
4. Squash into clean commits with proper messages
5. PR to master

#### Task 3.5: Establish Branch Strategy Document
Create `.github/BRANCH_STRATEGY.md`:
```markdown
# Branch Strategy

## Protected Branches
- `master`: Production-ready, always builds
- `develop`: Integration branch, may have WIP

## Feature Branches
- Naming: `feature/<ticket-or-description>`
- Max lifetime: 2 weeks
- Must be rebased on develop before merge

## Fix Branches
- Naming: `fix/<issue-description>`
- For bug fixes, branch from master
- Merge to both master and develop

## Forbidden
- No `god-merge-*` branches
- No AI agent experiment branches committed to remote
- No branches older than 30 days without documented reason
```

### Success Criteria
- [ ] On `master` branch, not detached HEAD
- [ ] < 10 total branches (local + remote)
- [ ] All AI experiment branches deleted
- [ ] Valuable work from deleted branches preserved
- [ ] `BRANCH_STRATEGY.md` committed

### Files You'll Create
- `.agent/artifacts/BRANCH_AUDIT.md`
- `.github/BRANCH_STRATEGY.md`

---

# 🎨 AGENT 4: UI/Rendering Unification Specialist
## Mission: Resolve Skia vs JUCE Hybrid Mess, Establish Single Rendering Path

### Context
The UI currently uses BOTH:
- JUCE native rendering (`juce::Graphics`, `paint()`, `resized()`)
- Skia hardware-accelerated rendering (72 files in `ui/skia/`)

This creates:
- Double-buffering overhead
- Maintenance burden (two systems to update)
- Unclear which path any given component uses

### Your Tasks

#### Task 4.1: Inventory Rendering Approaches
Create `.agent/artifacts/RENDERING_AUDIT.md` listing every UI component and its rendering approach:

```markdown
| Component | File | Rendering | Recommendation |
|-----------|------|-----------|----------------|
| TransportBar | ui/skia/TransportBar.h | Skia (SkiaComponent) | KEEP SKIA |
| ArrangerComponent | ui/ArrangerComponent.cpp | Hybrid (JUCE+Skia) | CONVERT TO SKIA |
| PianoRollComponent | ui/PianoRollComponent.cpp | Hybrid | CONVERT TO SKIA |
| SettingsComponent | ui/SettingsComponent.h | Pure JUCE | KEEP JUCE (dialog) |
```

#### Task 4.2: Commit to Skia for Main Views
The main performance-critical views should be pure Skia:
- Arranger timeline
- Piano roll
- Mixer
- Waveform displays

For each, ensure:
1. Remove `paint(juce::Graphics& g)` overrides
2. Implement only `drawSkia(SkCanvas* canvas)`
3. Use `SkiaComponent` base class consistently

#### Task 4.3: Keep JUCE for Dialogs/Settings
Simple dialogs don't need Skia complexity:
- Settings panels
- File browsers
- Alert dialogs

These can remain pure JUCE. Document this decision.

#### Task 4.4: Fix the SkiaMainWindowIntegration
Current `MainComponent::paint()` does:
```cpp
void MainComponent::paint(juce::Graphics &g) {
  SkiaMainWindowIntegration::paint(g);  // Renders Skia to JUCE context
}
```

This is inefficient. Instead:
1. Make `MainComponent` inherit from a Skia-native window class
2. Use Direct3D/Metal swap chain directly
3. Only use JUCE for event handling, not rendering

Or if keeping JUCE as container, ensure single-buffer path with `SkImage::MakeFromTexture`.

#### Task 4.5: Clean Up ui/skia/ Directory
That directory has 72 files including misplaced things like `AudioFifo.h`. 

1. Move non-rendering utilities out of `ui/skia/`
2. Organize into subdirectories:
   ```
   ui/skia/
     components/     # SkiaComponent subclasses
     rendering/      # SkCanvas helpers, effects
     design/         # ZenithDesignSystem, colors, typography
   ```

#### Task 4.6: Document the Rendering Architecture
Create `docs/RENDERING_ARCHITECTURE.md`:
- Skia initialization flow
- Component lifecycle
- How to create a new Skia component
- Performance considerations

### Success Criteria
- [ ] Clear documentation of what uses Skia vs JUCE
- [ ] Main views (Arranger, PianoRoll, Mixer) pure Skia
- [ ] No hybrid components (pick one approach per component)
- [ ] `ui/skia/` directory organized
- [ ] `RENDERING_ARCHITECTURE.md` exists

### Files You'll Modify
- `apps/desktop/Source/ui/ArrangerComponent.cpp`
- `apps/desktop/Source/ui/PianoRollComponent.cpp`
- `apps/desktop/Source/ui/MainWindow.cpp`
- Reorganize `apps/desktop/Source/ui/skia/*`

---

# 🧹 AGENT 5: Code Quality & Cleanup Enforcer
## Mission: Remove Dead Code, Fix Naming, Establish Quality Gates

### Context
The codebase has:
- 242KB `.bak` files
- Empty section comments (Flecs ECS)
- 156 documentation files (more than code files)
- Legacy method overloads marked "deprecated"
- Committed cleanup Python scripts
- `// ROAST FIX #1` style comments

### Your Tasks

#### Task 5.1: Delete Dead Files
Remove these files that shouldn't be in the repo:
```powershell
Remove-Item "c:\zenith\daw\apps\desktop\Source\engine\Engine.cpp.bak"
Remove-Item "c:\zenith\daw\ClipSynchronizer.h.bak"
Remove-Item "c:\zenith\daw\analyze_duplicates.py"
Remove-Item "c:\zenith\daw\cleanup_duplicates.py"
Remove-Item "c:\zenith\daw\cleanup_get.py"
Remove-Item "c:\zenith\daw\cleanup_headers.py"
Remove-Item "c:\zenith\daw\build_errors.txt"
Remove-Item "c:\zenith\daw\build_log.txt"
Remove-Item "c:\zenith\daw\build_log_2.txt"
Remove-Item "c:\zenith\daw\build_log_3.txt"
Remove-Item "c:\zenith\daw\build_output.txt"
Remove-Item "c:\zenith\daw\residual_plot.png"  # ???
Remove-Item "c:\zenith\daw\scatter_lsrl_graph.png"  # ???
```

Add to `.gitignore`:
```
*.bak
build_*.txt
build_*.log
*.pyc
__pycache__/
```

#### Task 5.2: Remove Dead Code Sections
Search for and remove:
```cpp
// Flecs ECS Integration
//==============================================================================
// (empty section)
```

```cpp
// Legacy: for setAudioBuffer()
// Legacy method (deprecated - loads file directly without pool)
// Legacy overloads (use internal transportPosition)
```

Either implement these or remove them entirely. No "legacy" comments without a deprecation timeline.

#### Task 5.3: Fix Comment Style
Replace tribal knowledge comments:
```cpp
// ROAST FIX #1: Use make_shared instead of make_unique
// CODEX FIX P2: Set shutdown flag
```

With meaningful comments:
```cpp
// Use shared_ptr to allow track references to outlive snapshot updates
// Set shutdown flag to prevent async callbacks during destruction
```

#### Task 5.4: Consolidate Documentation
The `docs/` folder has 156 files, many outdated. 

1. **Archive old docs**: Move to `docs/archive/` (already exists):
   - `PHASE1_*.md` files
   - `SESSION_COMPLETE_SUMMARY.md`
   - `BUILD_FIX_SESSION_SUMMARY.md`
   - Any file referencing completed/abandoned work

2. **Keep active docs** (max 15 files in `docs/`):
   - README.md
   - ARCHITECTURE.md
   - DEVELOPER_WORKFLOW.md
   - INSTALL_WINDOWS.md
   - THREADING_MODEL.md (created by Agent 2)
   - RENDERING_ARCHITECTURE.md (created by Agent 4)
   - Instrument/Plugin API docs

3. **Create index**: Update `docs/DOCUMENTATION_INDEX.md` to only list active docs

#### Task 5.5: Remove Excessive DBG() Calls
The constructor shouldn't need 12 debug prints. Replace:
```cpp
DBG("Engine: Constructor");
DBG("Engine: Modular components initialized");
DBG("Engine: AudioFilePool created");
// ... 9 more
```

With:
```cpp
DBG("Engine: Initialized with AudioRenderer, RecordingManager, TransportController");
```

One line per major initialization phase, not per object.

#### Task 5.6: Establish Quality Gates
Create `.github/workflows/code-quality.yml` (even if just documentation for now):
```yaml
# Code Quality Checklist (Manual until CI is set up)
# 
# Before merging any PR:
# - [ ] Builds with 0 errors
# - [ ] No new .bak files
# - [ ] No legacy/deprecated code without removal date
# - [ ] DBG() calls are meaningful, not per-line traces
# - [ ] Files < 500 lines (or justified exception)
# - [ ] No void* or type-erased pointers without comment
```

#### Task 5.7: Fix the void* Type Erasure
In `Clip.h`:
```cpp
std::shared_ptr<const void> audioFileHandle_;  // Type-erased to avoid forward decl issues
```

Fix with proper forward declaration:
```cpp
// Forward declare in Clip.h
namespace zenith { struct AudioFileHandle; }

// In Clip.h
std::shared_ptr<const AudioFileHandle> audioFileHandle_;
```

### Success Criteria
- [ ] No `.bak` files in repo
- [ ] No Python cleanup scripts in repo
- [ ] No random image files in root
- [ ] `docs/` has < 20 active files (rest archived)
- [ ] No `// ROAST FIX` or `// CODEX FIX` comments
- [ ] No `void*` without explicit justification
- [ ] Constructor DBG spam reduced by 80%

### Files You'll Delete
- All `.bak` files
- All `build_*.txt` logs
- Python cleanup scripts
- Stray image files

### Files You'll Modify
- `apps/desktop/Source/engine/Engine.cpp` (reduce DBG)
- `apps/desktop/Source/engine/Clip.h` (fix void*)
- `.gitignore`
- `docs/DOCUMENTATION_INDEX.md`

---

# 📊 Coordination Notes

## Parallel Execution Strategy
These agents can run in parallel with minimal conflicts:

| Agent | Primary Files | Conflicts With |
|-------|--------------|----------------|
| 1 (Build) | MainWindow.cpp, build system | Agent 4 (if touching MainWindow) |
| 2 (Architecture) | Engine.cpp, ProjectState.cpp | Agent 5 (both modify Engine) |
| 3 (Git) | .git/, branches | None (repo-level) |
| 4 (UI) | ui/*.cpp, ui/skia/* | Agent 1 (if fixing MainWindow) |
| 5 (Cleanup) | Various, docs/* | Agent 2 (both modify Engine) |

**Recommended Execution Order:**
1. **Agent 3 first** - Fix git state so others have clean foundation
2. **Agent 1 next** - Get it building so others can verify their work
3. **Agents 2, 4, 5 in parallel** - Coordinate on Engine.cpp changes

## Merge Strategy
After all agents complete:
1. Agent 3 creates integration branch
2. Each agent's work is reviewed and merged sequentially
3. Final verification build
4. Squash merge to master

## Communication Protocol
If an agent encounters a conflict with another agent's domain:
1. Document the conflict in `.agent/artifacts/CONFLICT_LOG.md`
2. Make minimal fix to unblock
3. Leave `// TODO: Agent X should handle this` comment
4. Continue with primary mission

---

# ✅ Combined Success Criteria for A+ Grade

## Buildability: A+
- [ ] Zero compilation errors
- [ ] Zero linker errors
- [ ] Application launches and runs

## Architecture: A+
- [ ] No file > 500 lines without justification
- [ ] Clear separation: Engine, UI, State
- [ ] Single synchronization mechanism
- [ ] Documented threading model

## Git Hygiene: A+
- [ ] < 10 branches
- [ ] Clean commit history on master
- [ ] Branch strategy documented

## UI/Rendering: A+
- [ ] Single rendering approach per component
- [ ] Documented architecture
- [ ] No hybrid Skia+JUCE components

## Code Quality: A+
- [ ] No dead code
- [ ] No .bak files
- [ ] Meaningful comments
- [ ] < 20 active doc files

---

*Generated by Zenith DAW Code Review System*
*Execute these prompts with your preferred AI coding agent*
