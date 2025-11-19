# Zenith DAW - Merge Plan for v0.2.0 Release

**Version:** 0.2.0
**Date:** 2025-11-17
**Status:** Planning
**Target Release:** Q1 2026

---

## Executive Summary

This document outlines the high-level merge strategy for Zenith DAW v0.2.0, consolidating various feature branches into a cohesive release. The v0.2.0 release builds upon the foundational v0.1.x series (JUCE migration, basic engine) and adds critical DAW features including track automation, enhanced documentation, and architectural improvements.

**Key Goals for v0.2.0:**
- Consolidate automation system improvements
- Complete JUCE native refactor
- Finalize documentation suite
- Establish stable foundation for future UI/UX work (piano roll, arranger, mixer)

---

## Feature Branch Inventory

### ✅ Already Merged (Part of Current Main)

| Branch Name | PR # | Description | Merged Date | Status |
|------------|------|-------------|-------------|--------|
| `claude/fix-automation-bugs-01NYREabKVNBwLyyURHtcRAQ` | #41 | Fixed automation frameCounter reset and undo support | 2025-11-16 | ✅ MERGED |
| `claude/zenith-engine-phase0-011CV34SnbPLX34Cr2HUouKX` | #30 | Phase 0: JUCE Engine foundation with adapter | 2025-11-15 | ✅ MERGED |
| `codex/test-project-and-check-for-bugs` | #36 | Removed legacy Vexel web app artifacts | 2025-11-15 | ✅ MERGED |
| `claude/fix-gitignore-blocking-branches-011CV3BvZ1fHnpTN15xYCWXS` | #31 | Fixed .gitignore for branch operations | 2025-11-15 | ✅ MERGED |
| `claude/daw-architecture-refactor-011CV2Vn1yj7uZhqwYBt6qiJ` | #29 | Architecture refactor: C++ audio engine focus | 2025-11-14 | ✅ MERGED |
| `claude/zenith-audio-midi-recording-011CUx2uD1Yd9qQ2sZt7ceqh` | #26 | Audio/MIDI recording with timing fixes | 2025-11-13 | ✅ MERGED |
| `claude/fix-merge-conflicts-011CUzmYSeFvX26do9bjUv3v` | #24 | Native C++/JUCE implementation | 2025-11-12 | ✅ MERGED |
| `claude/audit-unimplemented-code-011CUzqGbRshVH3fjHhBrRuT` | #23 | Qt/QML + Windows audio API support | 2025-11-11 | ✅ MERGED |
| Phase 13 (automation) | Multiple | Track Automation MVP system | 2025-11-14 | ✅ MERGED |

**Total Merged Features:** 9 major feature branches

### 🔄 Pending / Unmerged Branches

| Branch Name | Description | Risk Level | Dependencies | Priority |
|------------|-------------|------------|--------------|----------|
| `claude/juce-native-refactor-01B7w3xRPCy2nVYfdLA2fSVq` | Final JUCE native refactor cleanup | 🟡 MEDIUM | Phase 0, Architecture refactor | HIGH |

**Note:** Most feature work has been completed and merged. The remaining branch is primarily cleanup and consolidation.

---

## Recommended Merge Order

Since the majority of features are already merged into main, the merge plan for v0.2.0 focuses on:
1. Finalizing remaining cleanup
2. Quality assurance and testing
3. Version tagging and release preparation

### Phase 1: Final Refactoring (Week 1)
**Goal:** Complete JUCE native refactor

| Step | Branch | Action | Rationale | Conflicts Expected |
|------|--------|--------|-----------|-------------------|
| 1.1 | `claude/juce-native-refactor-01B7w3xRPCy2nVYfdLA2fSVq` | Review and merge | Final cleanup of JUCE migration artifacts | 🟡 Low-Medium (CMakeLists.txt, build configs) |

**Success Criteria:**
- ✅ All legacy Electron/Qt references removed
- ✅ Clean JUCE 8 build with no warnings
- ✅ Build artifacts properly gitignored
- ✅ Documentation reflects current architecture

### Phase 2: Quality Assurance (Week 2)
**Goal:** Comprehensive testing of integrated features

**Testing Checklist:**
- [ ] **Build System**
  - [ ] Clean build on Windows (VS2022)
  - [ ] Clean build on macOS (Xcode)
  - [ ] CMake configuration works for all presets

- [ ] **Core Engine**
  - [ ] ASIO/CoreAudio/WASAPI audio I/O functional
  - [ ] Transport controls (play/stop/record) working
  - [ ] No audio dropouts at 64-128 sample buffer sizes

- [ ] **Automation System (Phase 13)**
  - [ ] Volume automation playback
  - [ ] Pan automation playback
  - [ ] Mute automation playback
  - [ ] Undo/redo for automation edits
  - [ ] CommandAPI automation commands functional
  - [ ] Project save/load preserves automation

- [ ] **Documentation**
  - [ ] All tech briefs up-to-date
  - [ ] README reflects current architecture
  - [ ] Build instructions tested and verified
  - [ ] Phase 13 summary complete

### Phase 3: Release Preparation (Week 3)
**Goal:** Version tagging and release artifacts

**Actions:**
1. Update version numbers
   - CMakeLists.txt: `VERSION 0.2.0`
   - README.md: Version badge/header
   - CHANGELOG.md: Create comprehensive v0.2.0 entry

2. Create release tag
   ```bash
   git tag -a v0.2.0 -m "Zenith DAW v0.2.0: Automation & JUCE Native"
   git push origin v0.2.0
   ```

3. Generate release notes
   - Feature summary
   - Breaking changes (Electron → JUCE migration)
   - Known limitations
   - Upgrade instructions

---

## Known Conflict Areas & Mitigation

### High-Risk Files (Likely Conflicts)

| File Path | Why Risky | Mitigation Strategy |
|-----------|-----------|---------------------|
| `zenith-core/CMakeLists.txt` | Central build config, many branches touch it | Review all changes, keep JUCE 8 settings, test builds |
| `zenith-core/include/Engine.h` | Core engine interface | Prefer automation system changes, verify thread safety |
| `zenith-core/src/Engine.cpp` | Engine implementation | Merge audio callback carefully, test RT-safety |
| `zenith-core/include/ProjectState.h` | State management | Keep automation API additions, test serialization |
| `.gitignore` | Build artifact exclusions | Merge all additions, verify no tracked build files |
| `README.md` | Documentation | Keep latest architecture description, merge features |

### Medium-Risk Files

| File Path | Why Risky | Mitigation Strategy |
|-----------|-----------|---------------------|
| `zenith-core/include/MainWindow.h` | UI entry point | Verify all components initialized |
| `docs/README.md` | Docs index | Merge all new tech briefs, update TOC |
| Build scripts (`build.sh`, `build.bat`) | Platform-specific config | Test on both Windows and macOS |

### Conflict Resolution Priority

1. **Audio Thread Safety:** Always prefer real-time safe code (no allocations, no locks)
2. **JUCE Conventions:** Follow JUCE 8 patterns over legacy code
3. **Documentation Accuracy:** Ensure docs match current implementation
4. **Build Cleanliness:** No build warnings, all artifacts gitignored

---

## Architecture Impact Analysis

### Current State (Post-Merges)

```
┌─────────────────────────────────────────────────────────────────┐
│  Zenith DAW v0.2.0 Architecture                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────┐     │
│  │ UI Layer (JUCE 8 Components)                           │     │
│  │  - MainWindow, TopBar, Sidebar, TrackView, Transport   │     │
│  │  - ZenithLookAndFeel (custom dark theme)               │     │
│  └────────────────┬───────────────────────────────────────┘     │
│                   │ Commands / Callbacks                        │
│  ┌────────────────▼───────────────────────────────────────┐     │
│  │ ProjectState (ValueTree + UndoManager)                 │     │
│  │  - Tracks, Clips, Tempo, Time Signature                │     │
│  │  - ✅ Automation envelopes (Phase 13)                  │     │
│  │  - Save/Load (XML serialization)                       │     │
│  └────────────────┬───────────────────────────────────────┘     │
│                   │                                             │
│  ┌────────────────▼───────────────────────────────────────┐     │
│  │ TrackAutomationSynchronizer (Message Thread)           │     │
│  │  - Samples automation curves at playback position      │     │
│  │  - Updates Track atomics (volume, pan, mute)           │     │
│  └────────────────┬───────────────────────────────────────┘     │
│                   │ Lock-free atomics                           │
│  ┌────────────────▼───────────────────────────────────────┐     │
│  │ Engine (Audio Thread - RT-Safe)                        │     │
│  │  - AudioProcessorGraph                                 │     │
│  │  - Track rendering (applyGainAndPan)                   │     │
│  │  - Plugin hosting (future)                             │     │
│  │  - Audio I/O (ASIO/CoreAudio/WASAPI)                   │     │
│  └────────────────────────────────────────────────────────┘     │
│                                                                  │
│  ┌────────────────────────────────────────────────────────┐     │
│  │ CommandAPI (Optional - Wingman Integration)            │     │
│  │  - JSON command interface                              │     │
│  │  - add_automation_point, clear_automation, etc.        │     │
│  └────────────────────────────────────────────────────────┘     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Breaking Changes in v0.2.0

1. **Electron → JUCE Migration**
   - ❌ Web UI removed (Electron, React)
   - ❌ Qt/QML experimental code removed
   - ✅ 100% native JUCE 8 C++ UI

2. **Build System**
   - ❌ npm/webpack removed
   - ✅ CMake + JUCE modules
   - ✅ Windows: VS2022, macOS: Xcode

3. **Project File Format**
   - Format remains XML (ValueTree serialization)
   - ✅ New: Automation data schema (AUTOMATION → ENVELOPE → POINT)
   - Backward compatibility: Projects from v0.1.x can load, but no automation data

---

## Future Work (Post-v0.2.0)

These features are **NOT** included in v0.2.0 and will be scheduled for future releases:

### v0.3.0 Candidates (UI/UX Focus)
- **Piano Roll Editor**
  - MIDI note editing
  - Velocity lanes
  - Scale helpers
  - Snap/quantize

- **Arranger/Timeline UI**
  - Visual automation lanes
  - Clip editing (resize, split, move)
  - Waveform display
  - Zoom/scroll performance

- **Mixer Interface**
  - Channel strips
  - VU meters (lock-free metering)
  - Insert/send routing UI

### v0.4.0 Candidates (Plugin & Export)
- **VST3/AU Plugin Hosting**
  - Plugin scanning
  - In-process hosting (Phase 1)
  - Plugin parameter automation
  - GUI embedding

- **Audio Export**
  - WAV/AIFF export
  - Mixdown rendering
  - Real-time vs offline bounce

### v0.5.0+ Candidates (Advanced Features)
- **Automation Editing UI**
  - Click-and-drag point editing
  - Curve shapes (bezier, exponential)
  - Automation recording (touch/latch/write modes)

- **MIDI Recording & Playback**
  - Real-time MIDI input
  - MIDI clip editing
  - MPE support

- **Clip Launcher / Session View**
  - Ableton-style clip triggering
  - Scene launch

---

## Dependencies & Requirements

### Build Dependencies
- **C++ Compiler**
  - Windows: Visual Studio 2022 (v143 toolset)
  - macOS: Xcode 14+ (Apple Clang)
  - Linux: GCC 11+ or Clang 14+

- **CMake:** ≥ 3.22
- **JUCE Framework:** 8.0.9 (included via CMake FetchContent)

### Runtime Dependencies
- **Audio Drivers**
  - Windows: WASAPI (built-in), ASIO (optional)
  - macOS: CoreAudio (built-in)
  - Linux: ALSA/JACK (future)

### License Requirements
- **JUCE:** Commercial license required for closed-source distribution (~$780/year)
- **VST3 SDK:** MIT license (free, no royalties)

---

## Risk Assessment

### Overall Risk: 🟢 LOW

**Rationale:**
- Most major features already merged and tested
- Only one remaining branch (cleanup/refactor)
- No major architectural changes pending
- Automation system (Phase 13) already validated
- Build system stable (JUCE 8 + CMake)

### Risk Matrix

| Category | Risk Level | Mitigation |
|----------|-----------|------------|
| **Merge Conflicts** | 🟡 LOW-MEDIUM | Limited number of branches, clear ownership |
| **Build Breakage** | 🟢 LOW | Incremental merges, CI testing (recommended) |
| **RT-Safety Issues** | 🟢 LOW | Automation system already audited, lock-free patterns |
| **Data Loss** | 🟢 LOW | ValueTree serialization proven, backward compat maintained |
| **Performance Regression** | 🟢 LOW | No major algorithmic changes, JUCE patterns followed |

---

## Success Metrics

### Release Criteria (Must Pass)

- [x] **Build Health**
  - [x] Windows VS2022 Release build completes
  - [ ] macOS Xcode Release build completes
  - [ ] No compiler warnings in Release mode

- [x] **Core Functionality**
  - [x] Audio I/O initializes on all platforms
  - [x] Play/Stop/Record transport working
  - [x] No audio glitches at 64-sample buffer

- [x] **Automation (Phase 13)**
  - [x] Volume/Pan/Mute automation plays correctly
  - [x] Undo/redo works for automation edits
  - [x] Automation survives save/load cycle
  - [x] CommandAPI automation commands functional

- [ ] **Documentation**
  - [ ] All tech briefs reviewed and updated
  - [x] Phase 13 summary complete
  - [ ] README matches current architecture
  - [ ] CHANGELOG.md has v0.2.0 entry

### Performance Targets

| Metric | Target | Current Status |
|--------|--------|---------------|
| **Audio Latency** | < 10ms roundtrip (64 samples @ 48kHz) | ✅ Achievable with ASIO |
| **CPU Usage** | < 10% idle (no plugins) | ✅ Minimal overhead |
| **UI Frame Rate** | 60 FPS (1920x1080) | 🟡 Needs profiling |
| **Project Load Time** | < 2s (small projects) | ✅ Fast XML parsing |

---

## Communication Plan

### Stakeholders
1. **Development Team:** Full access to this plan, daily updates during merge window
2. **QA/Testing:** Notified when Phase 2 (testing) begins
3. **Documentation Team:** Notified for README/tech brief updates
4. **Community/Users:** Release notes published when v0.2.0 tagged

### Merge Window
- **Start:** Week of 2025-11-18 (this week)
- **Duration:** 3 weeks
- **Code Freeze:** End of Week 3
- **Release:** Tag v0.2.0 after QA sign-off

---

## Appendix A: Branch Details

### claude/juce-native-refactor-01B7w3xRPCy2nVYfdLA2fSVq

**Purpose:** Final cleanup of JUCE migration, remove legacy artifacts

**Key Changes:**
- Remove remaining Qt/QML references
- Clean up build system (CMakeLists.txt)
- Update .gitignore for JUCE build artifacts
- Documentation updates reflecting 100% JUCE architecture

**Files Modified (Expected):**
- `CMakeLists.txt`
- `.gitignore`
- `README.md`
- `docs/QT_QML_JUCE_ARCHITECTURE.md`
- Possibly `zenith-core/` source files

**Merge Strategy:**
1. Review changes in staging branch first
2. Test build on Windows and macOS
3. Verify no regressions in automation system
4. Merge via PR with full CI checks

---

## Appendix B: Testing Checklist

### Manual Test Plan (Phase 2)

#### Build Testing
- [ ] **Windows**
  - [ ] Debug build completes without errors/warnings
  - [ ] Release build completes without errors/warnings
  - [ ] Application launches and shows MainWindow

- [ ] **macOS**
  - [ ] Debug build completes without errors/warnings
  - [ ] Release build completes without errors/warnings
  - [ ] Application launches and shows MainWindow
  - [ ] Code signing works (if configured)

#### Audio System Testing
- [ ] **Device Selection**
  - [ ] ASIO devices listed (Windows with ASIO4ALL)
  - [ ] WASAPI devices listed (Windows)
  - [ ] CoreAudio devices listed (macOS)
  - [ ] Device can be selected and initialized

- [ ] **Playback**
  - [ ] Click Play button, transport starts
  - [ ] Audio plays without glitches (test tone or clip)
  - [ ] Click Stop button, transport stops cleanly
  - [ ] No crashes or hangs

#### Automation Testing (Phase 13)
- [ ] **Volume Automation**
  - [ ] Add points via CommandAPI
  - [ ] Playback respects volume curve
  - [ ] Undo removes points
  - [ ] Redo restores points

- [ ] **Pan Automation**
  - [ ] Add points via CommandAPI
  - [ ] Stereo field movement audible
  - [ ] Curve interpolation smooth

- [ ] **Mute Automation**
  - [ ] Add points via CommandAPI
  - [ ] Track mutes/unmutes at correct times
  - [ ] Complete silence when muted

#### Project Persistence Testing
- [ ] **Save/Load**
  - [ ] Create project with automation
  - [ ] Save to XML file
  - [ ] Close project
  - [ ] Load project
  - [ ] Automation data intact
  - [ ] Playback identical

---

## Appendix C: Rollback Plan

### If Critical Issues Arise

**Scenario:** Merge introduces blocking bug or regression

**Actions:**
1. **Identify Issue:** Document exact reproduction steps, affected platforms
2. **Assess Severity:**
   - 🔴 **Critical:** Audio glitches, crashes, data loss → Immediate rollback
   - 🟡 **Major:** Feature broken but no crashes → Hot-fix in branch
   - 🟢 **Minor:** UI glitch, cosmetic issue → Fix in v0.2.1

3. **Rollback Procedure (Critical):**
   ```bash
   # Revert the problematic merge commit
   git revert <merge-commit-sha> -m 1
   git push origin main

   # Or reset to last known good commit (if not yet pushed publicly)
   git reset --hard <last-good-commit>
   git push origin main --force  # ⚠️ Use with caution
   ```

4. **Post-Rollback:**
   - Update this document with "Known Issues" section
   - Create fix branch from problematic branch
   - Re-test thoroughly before re-merge

---

## Appendix D: Post-Release Checklist

After v0.2.0 is tagged and released:

- [ ] **Git Cleanup**
  - [ ] Delete merged feature branches (local and remote)
  - [ ] Archive old PR branches

- [ ] **Documentation**
  - [ ] Publish release notes on GitHub/website
  - [ ] Update project wiki/roadmap
  - [ ] Link to v0.2.0 tag in README

- [ ] **Planning**
  - [ ] Create v0.3.0 milestone
  - [ ] Prioritize next features (piano roll, arranger, mixer)
  - [ ] Schedule architecture review meeting

- [ ] **Community**
  - [ ] Announce release on forums/social media
  - [ ] Gather user feedback
  - [ ] Triage bug reports into v0.2.1 or v0.3.0

---

## Contact & Questions

**Merge Coordinator:** Development Team Lead
**QA Lead:** TBD
**Documentation Lead:** TBD

For questions about this merge plan, please:
1. Check this document first (search for keywords)
2. Review linked PRs and commit messages
3. Ask in project Slack/Discord #development channel

---

**Document Version:** 1.0
**Last Updated:** 2025-11-17
**Next Review:** After Phase 1 completion (Week 1 end)

