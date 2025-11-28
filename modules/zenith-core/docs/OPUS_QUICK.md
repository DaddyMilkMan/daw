# 🚀 QUICK OPUS PROMPT - Copy This Entire Message

Hey Claude Opus 4.5! I need help implementing a modern design system in my Zenith DAW (built with JUCE).

## 📁 Project
Location: `C:\zenith\daw\zenith-core\`

## 📖 What To Read First
1. `Source/ui/ZenithLookAndFeel.h` - Design tokens (MEMORIZE THIS!)
2. `Source/ui/ZenithLookAndFeel.cpp` - Implementation reference
3. `MODERN_DESIGN_SYSTEM.md` - Design principles
4. `COLOR_REFERENCE.md` - All color hex codes
5. `IMPLEMENTATION_GUIDE.md` - Examples

## 🎯 Your Task

**PHASE 1:** Analyze these components:
- `Source/ui/ArrangerComponent.*` (tracks)
- `Source/ui/TransportControlComponent.*` (play/record)
- `Source/ui/MasterOutputComponent.*` (meters)
- `Source/ui/ZenithStatusBar.*` (status)
- `Source/ui/InstrumentBrowserPanel.*` (browser)
- `Source/ui/ZenithTransportBar.*` (transport)
- `Source/ui/PianoRollComponent.*` (piano roll)
- `Source/ui/ZenithButton.*`, `ZenithKnob.*`, `ZenithSlider.cpp` (controls)

**PHASE 2:** For each component, report:
```
Component: [Name]
Issues: [Pure black/white? Custom grays? Bad spacing? No hovers?]
Priority: [CRITICAL/HIGH/MEDIUM/LOW]
Changes Needed: [Specific fixes]
```

**PHASE 3:** After approval, implement changes.

## 🎨 MUST USE THESE (Design Tokens):

```cpp
// Backgrounds (Material Design Elevation - NOT pure black!)
Elevation::dp0   // #121212 base
Elevation::dp2   // #232323 panels
Elevation::dp4   // #272727 buttons
Elevation::dp8   // #2e2e2e hover

// Text (87% white - NOT pure white!)
Colors::textPrimary    // #dedede
Colors::textSecondary  // #999999

// Accents (vibrant!)
Colors::accentPrimary   // #00d9ff cyan
Colors::accentSecondary // #ff8c42 orange

// Semantic
Colors::playGreen   // #4caf50
Colors::recordRed   // #ff5252
Colors::meterGreen  // safe
Colors::meterAmber  // caution
Colors::meterRed    // clip

// Track colors (auto-cycles!)
getTrackColor(trackIndex)

// Spacing (8px grid!)
Spacing::s   // 8px
Spacing::m   // 16px
Spacing::l   // 24px

// Typography
Typography::getBody()      // 14px
Typography::getBodyBold()  // 14px bold
Typography::getMonospace() // numbers

// Radius
Radius::m  // 6px standard

// Glows (not shadows!)
drawGlow(g, bounds, radius, color, glowSize)
```

## ❌ NEVER USE:

```cpp
juce::Colours::black      // ❌ Use Elevation::dp0
juce::Colours::white      // ❌ Use Colors::textPrimary
juce::Colour(0xff000000)  // ❌ Pure black
juce::Colour(0xffffffff)  // ❌ Pure white
juce::Colour(0xff1a1a1a)  // ❌ Custom gray - use Elevation
bounds.reduced(12, 8)     // ❌ Use Spacing::m
g.setFont(14.0f)          // ❌ Use Typography::getBody()
```

## 🎯 Key Patterns:

**Color-coded tracks:**
```cpp
auto color = ZenithLookAndFeel::getTrackColor(trackIndex);
g.setColour(color.withAlpha(0.12f)); // 12% for background
g.fillRoundedRectangle(bounds, Radius::m);
```

**Hover states:**
```cpp
if (isHovered) {
    color = color.brighter(0.15f); // +15% brightness
    // Optional glow
}
```

**Level meters:**
```cpp
if (level < 0.6f) color = Colors::meterGreen;      // -18 to -6dB
else if (level < 0.9f) color = Colors::meterAmber; // -6 to 0dB
else color = Colors::meterRed;                     // 0dB+
```

## ✅ Success = 
- No pure black/white
- All use Elevation system
- All use Colors::text*
- All use Spacing::*
- Tracks color-coded
- Transport semantic colors
- Meters standard colors
- Hover states work
- Typography consistent
- Comment all changes with `// DESIGN SYSTEM:`

## 🚀 Start:
1. Read design files
2. Analyze components (report issues + priorities)
3. Wait for approval
4. Implement

Let's make this DAW beautiful! 🎨
