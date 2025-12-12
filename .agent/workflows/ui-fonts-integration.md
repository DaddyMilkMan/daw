---
description: Implement professional custom font system with Inter and JetBrains Mono
---

# UI Fix #1: Custom Font Integration

## MISSION
Replace the embarrassing system font fallback with a professional custom font system. No more naked `SkFont font; font.setSize(18.0f);` garbage.

## PRE-TASK RESEARCH (MANDATORY)
Before writing ANY code:

1. **Web Search**: "Skia SkFontMgr custom typeface loading C++"
   - Verify correct API for loading .ttf/.otf files
   - Check if Skia needs FreeType or uses built-in parsers
   
2. **Web Search**: "Inter font download Google Fonts"
   - Download Inter (Variable or static weights: Regular 400, Medium 500, SemiBold 600, Bold 700)
   
3. **Web Search**: "JetBrains Mono download"
   - Download JetBrains Mono for code displays and fixed-width text
   
4. **Web Search**: "Skia font rendering subpixel antialiasing Windows"
   - Verify best practices for crisp text rendering on Windows

## IMPLEMENTATION STEPS

### Step 1: Create Font Asset Directory
```
c:\zenith\daw\apps\desktop\Resources\fonts\
├── Inter-Regular.ttf
├── Inter-Medium.ttf
├── Inter-SemiBold.ttf
├── Inter-Bold.ttf
├── JetBrainsMono-Regular.ttf
└── JetBrainsMono-Bold.ttf
```

### Step 2: Create FontManager Singleton
Create `Source/ui/skia/FontManager.h` and `FontManager.cpp`:

Requirements:
- Load fonts from embedded resources or file system at startup
- Cache SkTypeface objects (sk_sp<SkTypeface>)
- Provide getFont(FontFamily, FontWeight, Size) -> SkFont
- Support enum FontFamily { UI, Mono, Display }
- Support enum FontWeight { Regular, Medium, SemiBold, Bold }
- Thread-safe (fonts queried from audio thread for value displays)

### Step 3: Update ZenithDesignSystem.h
Replace the placeholder:
```cpp
inline SkFont getSkFont(float size) {
    SkFont font;
    font.setSize(size);
    return font;
}
```
With:
```cpp
inline SkFont getSkFont(float size, FontWeight weight = FontWeight::Regular) {
    return FontManager::getInstance().getFont(FontFamily::UI, weight, size);
}
inline SkFont getMonoFont(float size) {
    return FontManager::getInstance().getFont(FontFamily::Mono, FontWeight::Regular, size);
}
```

### Step 4: Update CMakeLists.txt
- Add font files as resources
- Ensure fonts are copied to output directory or embedded

### Step 5: Global Find/Replace
Search entire codebase for:
- `SkFont font;` followed by bare `font.setSize()`
- Replace with proper `design::getSkFont()` calls

### Step 6: Configure Subpixel Rendering
In SkFont initialization:
```cpp
font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
font.setSubpixel(true);
font.setHinting(SkFontHinting::kSlight);
```

## VERIFICATION CHECKLIST
- [ ] Fonts load without crash on startup
- [ ] Text renders crisp at 100%, 125%, 150%, 200% DPI
- [ ] All UI text uses Inter (not system font)
- [ ] Tempo/timing displays use JetBrains Mono
- [ ] Build succeeds with zero font-related warnings
- [ ] Web search confirms API usage is correct per Skia docs

## ACCEPTANCE CRITERIA
Screenshot the transport bar before/after. If you can't immediately tell the font changed, you failed.
