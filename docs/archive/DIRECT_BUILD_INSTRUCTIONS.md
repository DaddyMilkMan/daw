# DIRECT CMAKE INTEGRATION INSTRUCTIONS
## Adding New Skia Components to Build

**Current Status**: CMakeLists.txt found, Skia section exists (lines 183-201)

---

## STEP 1: Add New Files to CMakeLists.txt

Open `modules/zenith-core/CMakeLists.txt` and find line 183 where it says:

```cmake
if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW PRIVATE
```

**ADD these lines** after line 201 (before the `else()` on line 202):

```cmake
        # NEW: Team-built Skia UI Components
        src/ui/skia/ZenithDesignSystem.h
        src/ui/skia/SkiaComponent.h
        src/ui/skia/SkiaComponent.cpp
        src/ui/skia/SkiaButton.h
        src/ui/skia/SkiaButton.cpp
```

The section should look like this:

```cmake
if(ZENITH_ENABLE_SKIA)
    target_sources(ZenithDAW PRIVATE
        src/ui/skia/ZenithPolySynthUI.h
        src/ui/skia/ZenithPolySynthUI.cpp
        src/ui/skia/ZenithUIComponents.h
        src/ui/skia/SkiaMainWindowIntegration.h
        src/ui/skia/SkiaMainWindowIntegration.cpp
        src/ui/skia/TransportBar.h
        src/ui/skia/TransportBar.cpp
        src/ui/skia/BottomBar.h
        src/ui/skia/BottomBar.cpp
        src/ui/skia/RightSidePanel.h
        src/ui/skia/RightSidePanel.cpp
        src/ui/skia/views/PianoKeyboardViewSkia.h
        src/ui/skia/views/PianoKeyboardViewSkia.cpp
        src/ui/MainLayoutComponent.h
        src/ui/MainLayoutComponent.cpp
        src/ArrangementComponent.cpp
        
        # NEW: Team-built Skia UI Components
        src/ui/skia/ZenithDesignSystem.h
        src/ui/skia/SkiaComponent.h
        src/ui/skia/SkiaComponent.cpp
        src/ui/skia/SkiaButton.h
        src/ui/skia/SkiaButton.cpp
    )
```

---

## STEP 2: Fix SkiaButton Inheritance

The files reference `SkiaControl` which doesn't exist yet. Quick fix:

Open `modules/zenith-core/src/ui/skia/SkiaButton.h` and change line that says:

```cpp
class SkiaButton : public SkiaControl {
```

TO:

```cpp
class SkiaButton : public SkiaComponent {
```

And add these missing members inside the `private:` section:

```cpp
private:
    // Temporary until SkiaControl is implemented
    float value_ = 0.5f;
    bool enabled_ = true;
```

---

## STEP 3: Direct Build Commands

Now run these exact commands:

```powershell
# 1. Clean and reconfigure
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -B build -DZENITH_ENABLE_SKIA=ON

# 2. Build (this will take a few minutes)
cmake --build build --config Debug

# 3. If successful, run
.\build\Debug\Zenith` DAW.exe
```

---

## EXPECTED ISSUES & FIXES

### Issue 1: Blur Style Error
```
error: 'kNormal_SkBlurStyle' was not declared
```

**Fix**: In both `SkiaComponent.cpp` and `SkiaButton.cpp`, comment out blur lines:

```cpp
// Temporarily disabled - Skia version compatibility
// paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, radius));
```

### Issue 2: Missing Timer Callback
```
error: 'timerCallback' is not a member of 'SkiaComponent'
```

**Fix**: Add to `SkiaComponent.h`:

```cpp
protected:
    void timerCallback() override;
```

And inherit from `juce::Timer`:

```cpp
class SkiaComponent : public juce::Component, public juce::Timer {
```

---

## QUICK FIX SCRIPT

Save this as `fix_and_build.ps1`:

```powershell
# Fix SkiaButton inheritance
(Get-Content modules\zenith-core\src\ui\skia\SkiaButton.h) -replace 'class SkiaButton : public SkiaControl', 'class SkiaButton : public SkiaComponent' | Set-Content modules\zenith-core\src\ui\skia\SkiaButton.h

# Comment out blur (if needed)
# (Get-Content modules\zenith-core\src\ui\skia\SkiaComponent.cpp) -replace 'paint.setMaskFilter', '// paint.setMaskFilter' | Set-Content modules\zenith-core\src\ui\skia\SkiaComponent.cpp

# Clean and build
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -B build -DZENITH_ENABLE_SKIA=ON
cmake --build build --config Debug
```

Then run: `.\fix_and_build.ps1`

---

## TEAM SAYS:

- **Sarah**: "Add the files to CMakeLists.txt first!"
- **Priya**: "The integration is straightforward - just add to the existing Skia section."
- **Dr. Aris**: "Watch for Skia API version issues with blur."
- **Viktor**: "Comment out problematic code temporarily to get it building."
- **Raj**: "Clean build recommended - `Remove-Item build`"

---

**READY TO BUILD!**
