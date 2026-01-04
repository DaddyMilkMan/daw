# Quick Reference - Start Here

**Status**: Early Alpha (v0.1.0)  
**Build**: Release only (Debug broken)  
**Last Updated**: December 11, 2025

---

## 🚦 Project Health

| Metric | Status | Notes |
|--------|--------|-------|
| **Release Build** | ✅ Compiles | Use this for development |
| **Debug Build** | ❌ Broken | `Track.h:426` error - see TODO.md |
| **Tests** | ❌ None | Placeholders exist, need assertions |
| **Core Audio** | ✅ Working | Multi-track, MIDI, plugins |
| **AI Assistant** | ❌ Mocked | Not using real Grok API |
| **Stem Separation** | ❌ Disabled | ONNX Runtime not linked |

---

## 📁 Critical Files

### Core Engine
- `apps/desktop/Source/engine/Engine.cpp` - Main audio engine
- `apps/desktop/Source/engine/Track.h` - **BROKEN** (line 426)
- `apps/desktop/Source/engine/ProjectState.cpp` - Project data (ValueTree)
- `apps/desktop/Source/engine/Clip.cpp` - Audio/MIDI clips

### UI
- `apps/desktop/Source/ui/MainWindow.cpp` - Main window setup
- `apps/desktop/Source/ui/skia/` - Skia-based UI components

### Instruments
- `apps/desktop/Source/instruments/ZenithPolySynth.cpp` - Built-in synth (works)
- `apps/desktop/Source/instruments/ZenithSampler.cpp` - Sampler (works)

### AI (Currently Broken)
- `apps/desktop/Source/network/AIBridgeClient.cpp` - **Uses mock, not real API**
- `apps/desktop/Source/network/GrokAPIClient.cpp` - Real API (not wired up)

### Tests (Need Work)
- `apps/desktop/Source/tests/` - Placeholder tests, no assertions

---

## 🔧 Common Commands

### Build
```bash
# Release (ONLY working build)
build.bat

# Debug (BROKEN - don't use)
build.bat --debug

# Clean rebuild
build.bat --clean
```

### Run
```bash
# After successful build
run.bat
```

### Development
```bash
# Rebuild after code changes
rebuild.bat

# Run tests (currently useless)
build\Debug\ZenithDAWTests.exe
```

---

## 🐛 Known Issues - Cheat Sheet

| Issue | Severity | Workaround |
|-------|----------|------------|
| Debug build fails | Critical | Use Release only |
| AI gives canned responses | High | Feature not implemented yet |
| Stem separation doesn't work | High | Feature disabled (ONNX not linked) |
| Some VST3 plugins crash | High | Delete from scan folder |
| No tests validate behavior | High | Manual testing only |
| Track.h is 450+ lines | Medium | Needs refactor |
| No audio export | Medium | Record output with external tool |

See `docs/KNOWN_ISSUES.md` for full list.

---

## 🎯 What to Work on Next

**Priority 1 (This Week)**:
1. Fix `Track.h:426` compilation error (30 min)
2. Add one real test with assertions (2 hours)
3. Enable sanitizers in debug builds (30 min)
4. ✅ Update documentation (DONE)

**Priority 2 (This Month)**:
1. Wire up real Grok API (4 hours)
2. Add plugin crash recovery (8 hours)
3. Fix thread safety issues (16 hours)

See `docs/TODO.md` for full roadmap.

---

## 📚 Documentation Map

```
docs/
├── README.md                    # You are here
├── KNOWN_ISSUES.md             # All bugs and limitations
├── TODO.md                     # Development roadmap
├── ARCHITECTURE.md             # System design overview
├── INSTALL_WINDOWS.md          # Build instructions
└── INSTRUMENT_COMMAND_API.md   # Command system reference
```

---

## 🔍 Finding Your Way Around

### "Where is the audio callback?"
`apps/desktop/Source/engine/Engine.cpp` → `processBlock()`

### "Where are tracks stored?"
`apps/desktop/Source/engine/ProjectState.cpp` → ValueTree structure

### "How do I add a new command?"
`apps/desktop/Source/commands/CommandAPI.cpp` → `initializeCommandMap()`

### "Where's the UI rendering?"
`apps/desktop/Source/ui/skia/` → Skia components

### "How do plugins work?"
`apps/desktop/Source/engine/PluginHost.cpp` → VST3 hosting

---

## ⚠️ Before You Start Coding

1. ✅ Read `KNOWN_ISSUES.md` - don't work on broken features
2. ✅ Check `TODO.md` - see if your idea is already planned
3. ✅ Fix the compilation error first (`Track.h:426`)
4. ✅ Use Release build (Debug doesn't work)
5. ✅ Don't add new features until critical bugs fixed

---

## 🆘 Getting Help

**Build Issues**:
- Check `build_debug.log` for errors
- Make sure Visual Studio 2022 installed
- Verify vcpkg installed Skia correctly

**Runtime Crashes**:
- Likely VST3 plugin issue
- Delete problematic plugins from scan folder
- Check `AppData/Roaming/Zenith/` for logs

**Feature Not Working**:
- Check `KNOWN_ISSUES.md` - might be disabled/broken
- AI and stem separation are NOT functional yet

---

## 🎓 Learning the Codebase

**Start Here**:
1. Read `ARCHITECTURE.md` - understand system design
2. Look at `Main.cpp` - entry point
3. Follow code from `MainWindow` → `Engine` → `Track`
4. Read comments in header files

**Key Concepts**:
- **ValueTree**: Project data structure (JUCE)
- **RCU Snapshots**: Lock-free audio thread safety
- **Command API**: JSON-based automation interface
- **Skia**: GPU-accelerated rendering

---

## ✅ Verification Checklist

Before considering project "working":
- [ ] Debug build compiles
- [ ] At least one test asserts actual behavior
- [ ] Sanitizers enabled and clean
- [ ] AI uses real Grok API or is removed
- [ ] Plugin scanner doesn't crash
- [ ] Thread safety validated
- [ ] Audio export implemented

**Current Score**: 0/7

---

**Remember**: This is an alpha prototype. Expect bugs. Back up your work. Be honest about what works and what doesn't.

---

Updated: December 11, 2025
