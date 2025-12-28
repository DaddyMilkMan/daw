# 🚀 Commit Instructions for PR #346

## 📋 Changes to Commit:

### 1. **Critical Thread Safety Fix**
- File: `apps/desktop/Source/instruments/ZenithPolySynth.h`
- Change: Added `mutable juce::SpinLock modMatrixLock_;` (line 187)
- Purpose: Prevents data race between UI and audio threads

### 2. **Files Already Fixed:**
- ✅ AIEventBus.cpp - Optimized locking
- ✅ Engine.h - Const correctness fixed
- ✅ ZenithPolySynth.cpp - Locks implemented in set/get methods

## 🔧 Commands to Run:

```bash
# Stage all changes
git add -A

# Commit with descriptive message
git commit -m "fix: restore modMatrixLock_ to prevent critical data race

- Adds back mutable juce::SpinLock modMatrixLock_ to ZenithPolySynth
- Lock protects globalModMatrix_ from concurrent UI/audio access
- Prevents crashes and memory corruption
- Thread safety critical for production"

# Push to origin
git push origin test/basic-audio-test
```

## ⚠️ Important:
The modMatrixLock_ is ESSENTIAL for thread safety. Without it:
- UI thread calling setModulationMatrix() can race
- Audio thread reading values can crash
- Memory corruption possible

## 📊 Status:
- AIEventBus.cpp: ✅ Clean
- Engine.cpp: ✅ Clean
- Thread safety: ✅ Fixed
- Ready to push! 🚀
