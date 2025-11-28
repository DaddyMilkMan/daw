# Claude Code Prompt: Implement Modern Design System in Zenith DAW

## 📍 Context

I'm developing Zenith, a modern DAW (Digital Audio Workstation) built with JUCE. I've researched and designed a comprehensive modern design system based on Logic Pro, Ableton Live, Material Design, and WCAG AAA accessibility standards.

The new design system has been created in:
- `Source/ui/ZenithLookAndFeel.h` (design tokens and interface)
- `Source/ui/ZenithLookAndFeel.cpp` (implementation)
- `MODERN_DESIGN_SYSTEM.md` (full documentation)
- `COLOR_REFERENCE.md` (color palette guide)
- `IMPLEMENTATION_GUIDE.md` (how-to guide)

## 🎯 Your Task

Please review the following files and implement the new design system throughout the Zenith DAW UI components:

### Phase 1: Review & Analysis
1. **Read and understand the design system:**
   - `Source/ui/ZenithLookAndFeel.h`
   - `Source/ui/ZenithLookAndFeel.cpp`
   - `MODERN_DESIGN_SYSTEM.md`
   - `COLOR_REFERENCE.md`
   - `IMPLEMENTATION_GUIDE.md`

2. **Analyze existing UI components:**
   - `Source/ui/ArrangerComponent.cpp` & `.h`
   - `Source/ui/TransportControlComponent.cpp` & `.h`
   - `Source/ui/MasterOutputComponent.cpp` & `.h`
   - `Source/ui/InstrumentBrowserPanel.cpp` & `.h`
   - `Source/ui/ZenithStatusBar.cpp` & `.h`
   - `Source/ui/ZenithTransportBar.cpp` & `.h`
   - `Source/ui/PianoRollComponent.cpp` & `.h`
   - `Source/ui/ZenithButton.cpp` & `.h`
   - `Source/ui/ZenithKnob.cpp` & `.h`
   - `Source/ui/ZenithSlider.cpp`

3. **Identify issues in existing code:**
   - Are they using pure black (#000000) or pure white (#FFFFFF)?
   - Are they using custom grays instead of the elevation system?
   - Is spacing inconsistent (not following 8px grid)?
   - Are colors hardcoded instead of using design tokens?
   - Are hover states missing or poorly implemented?
   - Is typography inconsistent?

### Phase 2: Suggest Improvements
For EACH component file you analyze, provide:
1. **Current issues** - What's wrong with the current implementation
2. **Specific improvements** - Concrete changes needed
3. **Priority level** - Critical, High, Medium, Low
4. **Code examples** - Show before/after snippets

Format your suggestions like this:
```
## Component: ArrangerComponent.cpp

### Issues Found:
- ❌ Using pure black background (0xff000000)
- ❌ Track headers have no color coding
- ❌ Spacing is inconsistent (mix of 10px, 12px, 15px)
- ❌ No hover states on tracks

### Suggested Improvements:
1. [CRITICAL] Replace background with Elevation::dp0 (#121212)
2. [HIGH] Add color-coded track headers using getTrackColor()
3. [HIGH] Apply 8px grid spacing throughout
4. [MEDIUM] Add hover states with 15% brightening

### Code Examples:
[Show specific before/after code]
```

### Phase 3: Implementation
After I approve your suggestions, implement the changes:

1. **Update component files** to use the new design system
2. **Apply design tokens** instead of hardcoded values
3. **Add color coding** where appropriate (tracks, meters, etc.)
4. **Implement hover states** with proper timing and glows
5. **Fix spacing** to use 8px grid system
6. **Update typography** to use Typography system
7. **Ensure accessibility** (verify contrast ratios)

## 🎨 Key Design System Elements to Use

### Material Design Elevation (not custom grays!)
```cpp
Elevation::dp0  // #121212 - Base background
Elevation::dp1  // #1e1e1e - Cards, tracks
Elevation::dp2  // #232323 - Panels, browser
Elevation::dp4  // #272727 - Buttons
Elevation::dp8  // #2e2e2e - Hover states
Elevation::dp24 // #383838 - Modals
```

### Colors (vibrant yet professional!)
```cpp
Colors::accentPrimary        // #00d9ff - Cyan (primary actions)
Colors::accentPrimaryHover   // #33e0ff - Hover state
Colors::accentSecondary      // #ff8c42 - Orange (secondary)

Colors::textPrimary          // #dedede - 87% white (not pure white!)
Colors::textSecondary        // #999999 - 60% white
Colors::textDisabled         // #616161 - 38% white

Colors::playGreen            // #4caf50 - Play button
Colors::recordRed            // #ff5252 - Record button
Colors::meterGreen           // Safe zone
Colors::meterAmber           // Caution zone
Colors::meterRed             // Clipping
```

### Track Colors (frequency-based!)
```cpp
// Cycles through 12 colors automatically
auto trackColor = ZenithLookAndFeel::getTrackColor(trackIndex);
```

### Spacing (8px grid!)
```cpp
Spacing::xs  // 4px
Spacing::s   // 8px
Spacing::m   // 16px
Spacing::l   // 24px
Spacing::xl  // 32px
```

### Typography
```cpp
Typography::getH1()       // 24px bold - Major headings
Typography::getBody()     // 14px - Main text
Typography::getBodyBold() // 14px bold - Emphasis
Typography::getSmall()    // 12px - Secondary text
Typography::getMonospace() // 14px mono - Timecode/numbers
```

### Border Radius
```cpp
Radius::s   // 4px - Small controls
Radius::m   // 6px - Standard buttons
Radius::l   // 8px - Cards/panels
```

### Animation Timing
```cpp
Timing::quickMs    // 100ms - Press feedback
Timing::fastMs     // 150ms - Hover
Timing::normalMs   // 200ms - Standard
Timing::slowMs     // 300ms - Large elements
```

## ✅ Success Criteria

After implementation, the UI should have:
- [ ] All backgrounds use Elevation system (no pure black)
- [ ] All text uses Colors::text* (no pure white)
- [ ] All spacing follows 8px grid
- [ ] Tracks are color-coded with getTrackColor()
- [ ] Transport buttons use semantic colors (playGreen, recordRed)
- [ ] Level meters use standard colors (green → amber → red)
- [ ] Hover states work smoothly (15% brighter + optional glow)
- [ ] Typography is consistent throughout
- [ ] Border radius is consistent (4-8px range)
- [ ] WCAG AAA contrast compliance (verify with calculateContrastRatio)

## 🚫 What NOT to Do

- ❌ Don't use pure black (#000000) or pure white (#FFFFFF)
- ❌ Don't create custom gray values - use Elevation system
- ❌ Don't hardcode spacing - use Spacing constants
- ❌ Don't use inconsistent border radius
- ❌ Don't skip hover states
- ❌ Don't forget to use getTrackColor() for tracks
- ❌ Don't exceed 300ms animation duration
- ❌ Don't use drop shadows in dark mode (use glows instead)

## 📋 Deliverables

Please provide:
1. **Analysis Report** - Issues found in each component
2. **Improvement Suggestions** - Prioritized list of changes
3. **Updated Code** - Implement approved changes
4. **Testing Notes** - What to verify after changes

## 🆘 Questions to Consider

As you analyze the code, ask yourself:
1. Is this background using the elevation system?
2. Is this text readable against its background? (Check contrast)
3. Is this spacing following the 8px grid?
4. Does this have a proper hover state?
5. Should this track/element be color-coded?
6. Is this typography using the Typography system?
7. Are animations smooth and properly timed?

## 📁 File Locations

All files are in: `C:\zenith\daw\zenith-core\`

**Design System:**
- `Source/ui/ZenithLookAndFeel.h`
- `Source/ui/ZenithLookAndFeel.cpp`
- `MODERN_DESIGN_SYSTEM.md`
- `COLOR_REFERENCE.md`
- `IMPLEMENTATION_GUIDE.md`

**UI Components to Update:**
- `Source/ui/ArrangerComponent.*`
- `Source/ui/TransportControlComponent.*`
- `Source/ui/MasterOutputComponent.*`
- `Source/ui/InstrumentBrowserPanel.*`
- `Source/ui/ZenithStatusBar.*`
- `Source/ui/ZenithTransportBar.*`
- `Source/ui/PianoRollComponent.*`
- `Source/ui/ZenithButton.*`
- `Source/ui/ZenithKnob.*`
- `Source/ui/ZenithSlider.cpp`

## 🎯 Priority Order

Focus on these components first (highest visual impact):
1. **ArrangerComponent** - Track display (color-coding!)
2. **TransportControlComponent** - Play/record buttons (semantic colors!)
3. **MasterOutputComponent** - Level meters (standard colors!)
4. **ZenithStatusBar** - Status display (elevation & spacing!)
5. **InstrumentBrowserPanel** - Browser (elevation & hover!)
6. **ZenithTransportBar** - Transport bar (semantic colors!)
7. **PianoRollComponent** - Piano roll (elevation & colors!)
8. **ZenithButton/Knob/Slider** - Controls (already mostly handled by LookAndFeel)

## 💡 Pro Tips

- The LookAndFeel already handles standard JUCE components automatically
- Focus on custom painting code in `paint()` and `paintOverChildren()` methods
- Look for `g.setColour()` calls - these are prime candidates for updates
- Look for `.reduced()` calls - these should use Spacing constants
- Look for `fillRoundedRectangle()` calls - these should use Radius constants
- Use `lookAndFeel.drawGlow()` instead of drop shadows in dark mode

---

## 🚀 Let's Get Started!

Begin by reading all the design system files, then analyze each UI component and provide your suggestions. I'll review them before you implement the changes.

Ready? Let's make Zenith look amazing! 🎨
