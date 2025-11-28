# Zenith DAW - Quick Reference Guide

## Build Commands

### Clean Rebuild
```bash
cmd /c rebuild_clean.bat
```
- Removes old build files
- Configures CMake with Skia enabled
- Builds entire project with 8 parallel jobs

### Quick Rebuild (incremental)
```bash
cmd /c "cd build && cmake --build . --config Debug --parallel 8"
```

### Verification
```bash
cmd /c verify_build.bat
```
- Tests main executable
- Runs all test suites
- Verifies Skia integration

---

## Running Tests

### Individual Tests

**Synth Headless Test:**
```bash
build\SynthHeadlessTest_artefacts\Debug\SynthHeadlessTest.exe
```

**Preset Regression Tests:**
```bash
build\PresetRegressionTests_artefacts\Debug\PresetRegressionTests.exe
```

**Instrument Validation:**
```bash
build\InstrumentValidationTests_artefacts\Debug\InstrumentValidationTests.exe
```

---

## Main Application

### Running Zenith DAW
```bash
build\ZenithDAW_artefacts\Debug\ZenithDAW.exe
```

### Debug Mode
Built with Debug configuration by default for development.

---

## Warning Fix Scripts

All located in project root:

### Run All Fixes
```bash
powershell -ExecutionPolicy Bypass -File comprehensive_warning_fixes.ps1
powershell -ExecutionPolicy Bypass -File fix_all_warnings.ps1
powershell -ExecutionPolicy Bypass -File final_warning_elimination.ps1
powershell -ExecutionPolicy Bypass -File eliminate_style_warnings.ps1
```

### After Running Scripts
Always rebuild to verify:
```bash
cmd /c rebuild_clean.bat
```

---

## Project Structure

```
zenith-core/
├── Source/              # Main source code
│   ├── instruments/     # ZenithPolySynth, ZenithSampler
│   ├── ui/             # UI components
│   ├── engine/         # Audio engine, tracks, mixer
│   └── rendering/      # Skia rendering (new)
├── src/                # Additional source
├── include/            # Public headers
├── tests/              # Test executables
├── build/              # CMake build output (gitignored)
└── Content/            # Resources, presets
```

---

## Skia Integration

### Enabled Components
When built with `-DZENITH_ENABLE_SKIA=ON`:
- SkiaTheme - GPU theming
- SkiaTextRenderer - Hardware text
- SkiaWaveformRenderer - Fast waveforms
- SkiaPianoRollRenderer - Smooth piano roll
- SkiaClipRenderer - Optimized clips
- SkiaMixerChannelComponent - GPU mixer UI
- SkiaTransportControlComponent - Smooth transport
- Plus 6 more components

### Check Skia Status
```bash
findstr /C:"ZENITH_ENABLE_SKIA" build\CMakeCache.txt
```

---

## Common Issues

### Build fails - "Generator not found"
**Solution:** Ensure Visual Studio 2022 is installed
```bash
# Check VS installation
dir "C:\Program Files\Microsoft Visual Studio\2022"
```

### CMake configuration takes forever
**Normal:** First configure downloads JUCE (~500MB), can take 10-15 min
**Subsequent builds:** Should be much faster (cached)

### Tests fail to run
**Check:** Executables built correctly
```bash
dir build\*_artefacts\Debug\*.exe
```

### Skia not found
**Solution:** Install via vcpkg
```bash
C:\vcpkg\vcpkg.exe install skia:x64-windows
```

---

## IDE Setup

### Visual Studio Code
1. Install C++ extension
2. Open folder: `c:\zenith\daw\zenith-core`
3. CMake will auto-configure
4. Use "CMake: Build" command

### Visual Studio 2022
1. Open folder as CMake project
2. Select configuration: Debug
3. Build → Build All

---

## Documentation

- **WARNING_FIXES.md** - Complete warning fix documentation
- **BUILD_NOTES.md** - Build system notes
- **INSTRUMENTS_AND_MACROS.md** - Instrument system
- **AUDIO_RECORDING_IMPLEMENTATION.md** - Recording features
- **COMMAND_API_REFERENCE.md** - Command system

---

## Performance Tips

### Build Faster
```bash
# Use all CPU cores
cmake --build build --config Debug --parallel 16
```

### Clean Build Only When Needed
Most changes only need:
```bash
cmake --build build --config Debug
```

### Skip Tests During Development
Comment out test targets in CMakeLists.txt if not needed.

---

## Version Control

### Before Committing
1. Run all warning fix scripts
2. Build successfully
3. Run verify_build.bat
4. Check no new warnings introduced

### Commit Message Template
```
fix: Eliminate all 172 IDE warnings

- Replaced juce::Font with juce::FontOptions
- Converted raw pointers to smart pointers
- Added const correctness
- Modernized to C++20 patterns
- Enabled Skia integration

Verified: All tests passing
```

---

## Getting Help

### Build Issues
Check `build_log.txt` or CMake output

### Test Failures
Each test executable produces console output

### IDE Warnings
Re-run warning fix scripts and rebuild

---

**Last Updated:** November 20, 2025  
**Build Status:** ✅ Clean  
**Warning Count:** 0 (critical/high priority)  
**Skia Status:** ✅ Integrated
