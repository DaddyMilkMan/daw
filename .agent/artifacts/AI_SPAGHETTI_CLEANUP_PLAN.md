# 🧹 AI Spaghetti Cleanup Plan - Zenith DAW
**Created:** 2025-12-09
**Status:** Completed (Safe Phases)

---

## Executive Summary

This plan addresses 15 identified issues in the Zenith DAW codebase resulting from AI-assisted development. The cleanup is organized into phases based on risk and impact. "Safe Deletions" and "Code Cleanup" phases are largely complete.

---

## Phase 1: Safe Deletions (Low Risk, High Impact)
*Removing dead code and duplicates that won't affect compilation*

### Task 1.1: Delete Deprecated CollabPopup.h
- **File:** `apps/desktop/Source/ui/CollabPopup.h`
- **Reason:** Marked as deprecated, replaced by `ui/skia/CollabPanel.h`
- **Risk:** Low - just need to verify no includes
- **Status:** ✅ Done (Verified no usages)

### Task 1.2: Delete Duplicate PianoKeyboardViewSkia
- **Files to DELETE:**
  - `apps/desktop/Source/ui/views/PianoKeyboardViewSkia.cpp`
  - `apps/desktop/Source/ui/views/PianoKeyboardViewSkia.h`
- **Keep:** `apps/desktop/Source/ui/skia/views/PianoKeyboardViewSkia.cpp/h`
- **Risk:** Medium - need to update any includes
- **Status:** ✅ Done (Includes updated in BottomBar.h and MainWindow.h)

### Task 1.3: Delete Duplicate SkiaButton Implementations
- **Files to DELETE:**
  - `apps/desktop/Source/ui/skia/SkiaButtonComponent.h` (Unused)
  - `apps/desktop/Source/ui/skia/components/SkiaButton.cpp/h` (Duplicate)
- **Keep:** `apps/desktop/Source/ui/skia/SkiaButton.cpp/h`
- **Status:** ⚠️ PARTIAL
  - Deleted `SkiaButtonComponent.h`.
  - Deferred `components/SkiaButton` usage (requires refactor of AlertWindow/FileChooser).

### Task 1.4: Delete Code Templates in docs/
- **Directory:** `docs/code-templates/`
- **Files to DELETE:** All 7 files (CMakeLists.txt, Engine.cpp/h, Main.cpp, MainWindow.h, ProjectState.cpp/h)
- **Reason:** These are stale template files with different class names (ZenithEngine vs zenith::Engine)
- **Risk:** None - documentation only
- **Status:** ✅ Done

### Task 1.5: Clean Up Build Error Logs
- **Files to DELETE:** All `build_errors*.txt`, `build_*.log`, `cmake_error*.log` in root
- **Risk:** None - logs only
- **Status:** ✅ Done (Checked root, no large logs found or user managed)

---

## Phase 2: Code Cleanup (Medium Risk)
*Removing dead code blocks and comments*

### Task 2.1: Remove #if 0 Blocks from SkiaRenderer.cpp
- **File:** `apps/desktop/Source/rendering/SkiaRenderer.cpp`
- **Remove:** 7 `#if 0` blocks with disabled D3D12 code (~200 lines)
- **Risk:** Medium - must preserve functional code
- **Status:** ✅ Done (Cleaned up D3D blocks and simplified backend selection)

### Task 2.2: Remove "goto" Statement from SkiaRenderer.cpp
- **File:** `apps/desktop/Source/rendering/SkiaRenderer.cpp`
- **Location:** `initialize()` method, line 104
- **Action:** Refactor to proper control flow
- **Risk:** Medium
- **Status:** ✅ Done (Replaced with structured flow)

### Task 2.3: Clean Up Commented-Out Code in PresetGenerator
- **File:** `apps/desktop/Source/utils/PresetGenerator.h`
- **Remove:** Lines 59-64 "// Removed" comments
- **Risk:** Low
- **Status:** ✅ Done (File was superseded by FactoryPresetGenerator, see 3.1)

### Task 2.4: Remove Unused Legacy Member in Clip.h
- **File:** `apps/desktop/Source/engine/Clip.h`
- **Remove:** `std::unique_ptr<juce::AudioFormatReaderSource> audioSource; // Unused legacy` at line 245
- **Risk:** Medium - verify no usage
- **Status:** ⚠️ RESTORED - The member is still used by legacy methods `setAudioFile` and `prepareToPlay` which are called by `Engine.cpp` and `ZenithSampler`. Cannot remove without refactoring legacy paths.

### Task 2.5: Clean Up FIX Comments
- **Files:** Multiple files with "FIX:" comments
- **Action:** Convert to normal explanatory comments or remove
- **Risk:** Low
- **Status:** ✅ Done (Cleaned BrowserModel.cpp, verified others were clean or explanatory safety notes)

---

## Phase 3: Consolidation (Higher Risk)
*Resolving duplicate/conflicting implementations*

### Task 3.1: Resolve PresetGenerator Naming Conflict
- **Conflict:**
  - `utils/PresetGenerator.h` (factory preset generation)
  - `instruments/PresetGenerator.h` (AI preset helper)
- **Action:** Rename `utils/PresetGenerator.h` to `FactoryPresetGenerator.h`
- **Risk:** Medium - needs include updates
- **Status:** ✅ Done (Renamed and updated usage in RegisterBuiltInInstruments.cpp)

### Task 3.2: Add Pragma Once to PresetGenerator
- **File:** `apps/desktop/Source/utils/PresetGenerator.h`
- **Action:** Add `#pragma once` include guard
- **Risk:** Low
- **Status:** ✅ Done (Included in new FactoryPresetGenerator.h)

### Task 3.3: Delete Ghost Auth Service
- **Directory:** `services/auth/`
- **Action:** Delete entire directory (only contains .env)
- **Risk:** Low - orphaned structure
- **Status:** 🛑 SKIPPED - User currently editing files in this directory.

---

## Phase 4: Structural Improvements
*These are larger refactoring tasks that should be done incrementally*

### Task 4.1: Split Engine.cpp (~2777 lines)
- **Status:** ⏸️ DEFERRED - Needs separate dedicated session
- **Proposed Split:** `Engine.cpp`, `EngineTransport.cpp`, `EngineAudioCallback.cpp`, `EngineTrackManagement.cpp`, `EnginePluginHost.cpp`

### Task 4.2: Split ProjectState.cpp (~2302 lines)
- **Status:** ⏸️ DEFERRED - Needs separate dedicated session
- **Proposed Split:** `ProjectState.cpp`, `ProjectStateIO.cpp`, `ProjectStateTrack.cpp`, `ProjectStateClip.cpp`, `ProjectStateAutomation.cpp`

### Task 4.3: Standardize Namespace Usage
- **Status:** ⏸️ DEFERRED - Would require touching 50+ files to add explicit `zenith::` qualification.

### Task 4.4: Reduce Singleton Usage
- **Status:** ⏸️ DEFERRED - Architectural change (Dependency injection).

---
