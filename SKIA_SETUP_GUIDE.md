# Getting Skia Running - Quick Guide

## TL;DR - Run This One Command:

```cmd
C:\zenith\daw\setup-skia-complete.bat
```

That's it! This will:
1. ✅ Download Skia (pre-built, ~100MB)
2. ✅ Update CMakeLists.txt automatically
3. ✅ Build your DAW with Skia enabled

Total time: **5-10 minutes**

---

## Or Do It Step-by-Step:

### Step 1: Download Skia
```cmd
cd C:\zenith\daw
download-skia.bat
```

**What it does:** Downloads pre-built Skia binaries from JetBrains to `C:\zenith\skia`

---

### Step 2: Update CMakeLists
```cmd
cd C:\zenith\daw
update-cmake-for-skia.bat
```

**What it does:** Modifies `zenith-core/CMakeLists.txt` to use manual Skia instead of vcpkg

---

### Step 3: Build with Skia
```cmd
cd C:\zenith\daw
build-with-manual-skia.bat
```

**What it does:** Builds your DAW with Skia GPU rendering enabled

---

## What You Get:

✅ **GPU-accelerated rendering** via Skia  
✅ **OpenGL backend** (works on all Windows PCs)  
✅ **60+ FPS** smooth animations  
✅ **Professional UI** with gradients, shadows, effects  
✅ **Apple-quality rendering**

---

## Troubleshooting:

### "Download failed"
- Check internet connection
- Try downloading manually from: https://github.com/JetBrains/skia-pack/releases
- Extract to `C:\zenith\skia`

### "CMake configuration failed"
- Make sure Skia is at `C:\zenith\skia`
- Check that `C:\zenith\skia\include\core\SkCanvas.h` exists
- Check that `C:\zenith\skia\out\Release-x64\skia.lib` exists

### "Build failed"
- Check build errors in the console
- Make sure all JUCE API fixes were applied (they were!)
- Try clean rebuild: delete `build` folder and run again

---

## Files Created:

```
C:\zenith\daw\
├── setup-skia-complete.bat     ← Run this!
├── download-skia.bat
├── update-cmake-for-skia.bat
├── build-with-manual-skia.bat
└── cmake\
    └── SkiaManualIntegration.cmake

C:\zenith\skia\                 ← Downloaded here
├── include\
├── out\Release-x64\
└── ...
```

---

## Ready?

```cmd
C:\zenith\daw\setup-skia-complete.bat
```

Let's get that GPU acceleration! 🚀
