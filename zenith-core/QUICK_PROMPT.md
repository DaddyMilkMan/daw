# Quick Prompt for Claude Code

Hey Claude! I need your help implementing a modern design system in my Zenith DAW.

## What I've Done
I've created a comprehensive design system based on Logic Pro, Ableton Live, and Material Design principles:
- `Source/ui/ZenithLookAndFeel.h` & `.cpp` - Complete design system
- `MODERN_DESIGN_SYSTEM.md` - Full documentation
- `COLOR_REFERENCE.md` - Color palette
- `IMPLEMENTATION_GUIDE.md` - How-to guide

## What I Need You to Do

**Phase 1: Analyze**
Read these files and analyze the UI components:
- Design system files (ZenithLookAndFeel.*)
- Documentation files (*.md)
- UI component files in `Source/ui/` (ArrangerComponent, TransportControlComponent, MasterOutputComponent, etc.)

**Phase 2: Suggest**
For each component, tell me:
- What's wrong (using pure black/white, inconsistent spacing, no hover states, etc.)
- What to improve (use Elevation system, add color coding, fix spacing, etc.)
- Priority level (Critical, High, Medium, Low)

**Phase 3: Implement**
After I approve, update the components to use:
- `Elevation::dp0-24` for backgrounds (not pure black)
- `Colors::textPrimary` for text (not pure white)
- `Spacing::s/m/l` for 8px grid
- `getTrackColor()` for color-coded tracks
- `Colors::playGreen`, `recordRed` for transport
- `Colors::meterGreen/Amber/Red` for level meters
- `Typography::getBody()` for consistent fonts
- `Radius::m` for consistent border radius
- Hover states with 15% brightening

## Key Rules
✅ DO: Use Elevation system, design tokens, 8px grid, color coding
❌ DON'T: Use pure black/white, custom grays, arbitrary spacing, skip hover states

## Priority Components
1. ArrangerComponent (tracks - color code them!)
2. TransportControlComponent (play/record buttons)
3. MasterOutputComponent (level meters)
4. ZenithStatusBar (status display)
5. InstrumentBrowserPanel (browser list)

Start by reading the design system files, then analyze each component. Ready? Let's make Zenith beautiful! 🎨
