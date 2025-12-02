# 🎯 SKIA TRANSITION PLAN - Operation Polish Phase 2

**Goal**: Remove all JUCE rendering fallback code, commit fully to Skia

---

## 📊 Files to Modify (27 total)

### UI Core Components
1. ✅ `ui/ArrangerComponent.h` - Remove `#else` JUCE path
2. ⏳ `ui/ClipComponent.cpp` - Skia-only rendering
3. ⏳ `ui/MixerChannelComponent.cpp` - Skia-only
4. ⏳ `ui/PianoRollEditor.cpp` - Skia-only
5. ⏳ `ui/TimelineRuler.cpp` - Skia-only
6. ⏳ `ui/TrackHeaderComponent.cpp` - Skia-only

### Skia UI Components (already Skia-based, just remove conditionals)
7. ⏳ `ui/skia/BottomBar.h`
8. ⏳ `ui/skia/BottomBar.cpp`
9. ⏳ `ui/skia/BrowserPanel.h`
10. ⏳ `ui/skia/BrowserPanel.cpp`
11. ⏳ `ui/skia/RightSidePanel.h`
12. ⏳ `ui/skia/RightSidePanel.cpp`
13. ⏳ `ui/skia/TransportBar.h`
14. ⏳ `ui/skia/TransportBar.cpp`
15. ⏳ `ui/skia/SkiaMainWindowIntegration.h`
16. ⏳ `ui/skia/SkiaMainWindowIntegration.cpp`
17. ⏳ `ui/skia/SkiaButtonComponent.h`
18. ⏳ `ui/skia/SkiaKnobComponent.h`
19. ⏳ `ui/skia/SkiaSliderComponent.h`
20. ⏳ `ui/skia/ZenithPolySynthUI.cpp`
21. ⏳ `ui/skia/ZenithUIComponents.h`

### Views
22. ⏳ `ui/views/PianoKeyboardViewSkia.h`
23. ⏳ `ui/views/PianoKeyboardViewSkia.cpp`
24. ⏳ `ui/skia/views/PianoKeyboardViewSkia.h`
25. ⏳ `ui/skia/views/PianoKeyboardViewSkia.cpp`
26. ⏳ `ui/skia/views/SessionViewComponent.h`

---

## 🔧 Transformation Strategy

### Step 1: Remove `#ifdef` Blocks
Find patterns like:
```cpp
#ifdef ZENITH_USE_SKIA
    // Skia code
#else
    // JUCE code (DELETE THIS)
#endif
```

Replace with:
```cpp
// Just the Skia code, no conditionals
```

### Step 2: Update Includes
Remove conditional includes:
```cpp
#ifdef ZENITH_USE_SKIA
    #include "skia/..."
#endif
```

Becomes:
```cpp
#include "skia/..."
```

### Step 3: Constructor/Class Updates
Change:
```cpp
#ifdef ZENITH_USE_SKIA
class MyComponent : public SkiaComponent
#else  
class MyComponent : public juce::Component
#endif
```

To:
```cpp
class MyComponent : public SkiaComponent
```

### Step 4: Method Implementations
Remove JUCE fallback paint methods:
```cpp
void paint(juce::Graphics& g) override {
#ifdef ZENITH_USE_SKIA
    // Don't use g, use Skia
#else
    g.fillAll(...); // DELETE
#endif
}
```

Becomes:
```cpp
void renderSkia(SkCanvas* canvas) override {
    // Skia rendering only
}
```

---

## 🎯 Build System Updates

### CMake Changes
Remove `-DZENITH_USE_SKIA=ON/OFF` option, always enable:
```cmake
# Old
option(ZENITH_USE_SKIA "Enable Skia rendering" ON)

# New (just define it always)
add_definitions(-DZENITH_USE_SKIA=1)
```

### Build Scripts
Update `build.bat`:
- Remove `--no-skia` option
- Always pass Skia flags to CMake

---

## ⚠️ Potential Issues & Solutions

### Issue 1: Missing Skia Headers
**Fix**: Ensure all files include proper Skia headers
```cpp
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
// etc.
```

### Issue 2: JUCE Component Methods
**Q**: What about JUCE lifecycle methods (resized, mouseDown, etc.)?  
**A**: Keep them! Skia only replaces *rendering*, not the component framework

### Issue 3: Mixed Rendering
**Fix**: Ensure SkiaComponent base class handles all paint() calls properly

---

## 📝 Testing Checklist

After transition:
- [ ] Project builds without errors
- [ ] Application launches
- [ ] All UI elements visible
- [ ] No rendering artifacts
- [ ] Smooth 60 FPS
- [ ] Mouse interaction works
- [ ] Resizing works properly

---

**Status**: Ready to execute  
**Estimated Time**: 2-3 hours for all 27 files  
**Risk Level**: Medium (Skia already working, just removing dead code)
