# Save/Load Integration Checklist

## ✅ What's Been Done

- [x] Created `ProjectFileIO.h` - Full API header
- [x] Created `ProjectFileIO.cpp` - Complete implementation
- [x] Updated `ProjectState.h` - Added required methods and friend class
- [x] Added comprehensive error handling (9 error types)
- [x] Implemented atomic writes (temp → target)
- [x] Implemented auto-save with configurable intervals
- [x] Implemented crash recovery system
- [x] Implemented backup management
- [x] Implemented metadata/validation
- [x] Created integration guide: `docs/SAVE_LOAD_IMPLEMENTATION.md`

## ❌ What You Need To Do

### 1. Implement Methods in ProjectState.cpp

Add these implementations:

```cpp
void ProjectState::createDefaultState()
{
    // Create empty default project structure
    // See SAVE_LOAD_IMPLEMENTATION.md for code
}

void ProjectState::rebuildIdCounter()
{
    // Scan loaded state for max ID
    // See SAVE_LOAD_IMPLEMENTATION.md for code
}

void ProjectState::rebuildTrackMap()
{
    // Rebuild trackIdMap_ from ValueTree
    // See SAVE_LOAD_IMPLEMENTATION.md for code
}

int ProjectState::getNumTracks() const
{
    // Return track count
    // See SAVE_LOAD_IMPLEMENTATION.md for code
}
```

**Estimated Time:** 30 minutes

### 2. Update CMakeLists.txt

Add to your target source files:

```cmake
target_sources(ZenithDAW PRIVATE
    Source/engine/ProjectFileIO.h
    Source/engine/ProjectFileIO.cpp
)
```

**Estimated Time:** 5 minutes

### 3. Implement File Menu in MainWindow

- [ ] Add `fileIO_` member to MainWindow
- [ ] Construct in MainWindow constructor
- [ ] Implement `menuItemSelected_NewProject()`
- [ ] Implement `menuItemSelected_OpenProject()`
- [ ] Implement `menuItemSelected_Save()`
- [ ] Implement `menuItemSelected_SaveAs()`
- [ ] Implement `checkForRecovery()`
- [ ] Implement `timerCallback()` for auto-save
- [ ] Implement `closeButtonPressed()` for unsaved changes check
- [ ] Implement `updateWindowTitle()` for dirty indicator
- [ ] Wire file menu buttons to methods

See `docs/SAVE_LOAD_IMPLEMENTATION.md` Step 4 for full code.

**Estimated Time:** 1-2 hours

### 4. Track Dirty Flag Throughout ProjectState

Every time state changes, call `isDirty.store(true)`:

- [ ] In `addTrack()`
- [ ] In `deleteTrack()`
- [ ] In `addClip()`
- [ ] In `deleteClip()`
- [ ] In `setTrackVolume()`
- [ ] In `setTrackPan()`
- [ ] In `setTrackMute()`
- [ ] In `setTrackSolo()`
- [ ] In `setAutomationPoint()`
- [ ] In any other state-mutating method

**Estimated Time:** 1 hour

### 5. Test the System

- [ ] Create new project → check `isDirty` is false
- [ ] Add a track → check `isDirty` is true
- [ ] Save → check `isDirty` is false
- [ ] Modify something → check `isDirty` is true
- [ ] Close with unsaved changes → check dialog appears
- [ ] Auto-save trigger → check recovery file created
- [ ] Load project → check state restored correctly
- [ ] Intentional crash → check recovery available on restart
- [ ] Create backup → check backup file in Backups/ directory

**Estimated Time:** 1-2 hours

### 6. Fix Build Errors

You currently have Skia compilation errors. Before integrating save/load:

- [ ] Fix `RemoteCursorOverlay.h` Skia errors OR remove if experimental
- [ ] Fix `SettingsComponent.h` errors
- [ ] Get project building cleanly

See your build logs: `logs/build_latest.log`

**Estimated Time:** 2-4 hours depending on Skia issues

## Total Estimated Integration Time

- Implement ProjectState methods: 30 min
- Update CMakeLists: 5 min
- Implement MainWindow: 1-2 hours
- Track dirty flag: 1 hour
- Testing: 1-2 hours
- Build fixes: 2-4 hours
- **Total: 5-10 hours**

## Why This Matters

### Before (Current State)
```
- Save/load is unreliable (README says so)
- No crash recovery
- No backups
- No error messages (just "it failed")
- No way to recover from crash
```

### After (With This Implementation)
```
✓ Atomic writes prevent corruption
✓ Auto-save every 5 minutes to recovery folder
✓ Recovery files kept for 7 days
✓ Manual backups with automatic cleanup
✓ Detailed error messages
✓ Can recover from crash on startup
✓ Can browse/restore any backup
✓ Metadata reading without loading
✓ File validation before opening
```

## Priority Order

1. **Fix build errors** - Can't test anything if it won't compile
2. **Implement ProjectState methods** - Core functionality
3. **Implement MainWindow integration** - User-facing features
4. **Track isDirty flag** - Essential for detecting changes
5. **Test thoroughly** - Verify all paths work

## Questions?

Refer to:
- Implementation details: `docs/SAVE_LOAD_IMPLEMENTATION.md`
- Code examples: Lines in that file with full implementations
- API reference: Same doc, "API Quick Reference" section
