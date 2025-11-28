# 🗺️ Zenith DAW Design System - Complete File Map

```
C:\zenith\daw\zenith-core\
│
├─── 📘 DOCUMENTATION (Read First!)
│    ├── FILE_LOCATIONS.md          ← ⭐ START HERE! Where everything is
│    ├── MODERN_DESIGN_SYSTEM.md    ← Complete design theory & guide
│    ├── COLOR_REFERENCE.md         ← Visual color palette with hex codes
│    ├── IMPLEMENTATION_GUIDE.md    ← Step-by-step implementation guide
│    ├── CLAUDE_CODE_PROMPT.md      ← Full detailed prompt for Claude Code
│    ├── QUICK_PROMPT.md            ← Quick copy-paste version
│    └── open-in-claude-code.bat    ← Helper script to open in Claude Code
│
├─── 🎨 DESIGN SYSTEM (New Implementation)
│    └── Source/ui/
│         ├── ZenithLookAndFeel.h   ← Design tokens, colors, spacing, etc.
│         └── ZenithLookAndFeel.cpp ← Full implementation of all drawing
│
└─── 🖼️ UI COMPONENTS (To Be Updated by Claude Code)
     └── Source/ui/
          ├── ArrangerComponent.*           [Priority 1] Track display
          ├── TransportControlComponent.*   [Priority 1] Play/record buttons
          ├── MasterOutputComponent.*       [Priority 1] Level meters
          ├── ZenithStatusBar.*             [Priority 2] Status bar
          ├── InstrumentBrowserPanel.*      [Priority 2] Browser panel
          ├── ZenithTransportBar.*          [Priority 2] Transport bar
          ├── PianoRollComponent.*          [Priority 3] Piano roll
          ├── ZenithButton.*                [Priority 3] Custom button
          ├── ZenithKnob.*                  [Priority 3] Custom knob
          └── ZenithSlider.cpp              [Priority 3] Custom slider
```

---

## 🎯 Quick Start Workflow

```
┌─────────────────────────────────────────────────────────────┐
│  STEP 1: Understand What You Have                           │
├─────────────────────────────────────────────────────────────┤
│  📖 Read: FILE_LOCATIONS.md (this file!)                    │
│  📖 Skim: MODERN_DESIGN_SYSTEM.md                           │
│  📖 Browse: COLOR_REFERENCE.md                              │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  STEP 2: Open in Claude Code                                │
├─────────────────────────────────────────────────────────────┤
│  🌐 Go to: https://claude.ai/code                           │
│  📁 Open folder: C:\zenith\daw\zenith-core                  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  STEP 3: Give Claude the Prompt                             │
├─────────────────────────────────────────────────────────────┤
│  📋 Copy: CLAUDE_CODE_PROMPT.md (full) OR                   │
│  📋 Copy: QUICK_PROMPT.md (short)                           │
│  💬 Paste into Claude Code chat                             │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  STEP 4: Claude Analyzes                                    │
├─────────────────────────────────────────────────────────────┤
│  🔍 Claude reads design system                              │
│  🔍 Claude analyzes UI components                           │
│  📊 Claude provides analysis report                         │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  STEP 5: Claude Suggests Improvements                       │
├─────────────────────────────────────────────────────────────┤
│  ✅ Issues found per component                              │
│  ✅ Specific improvements needed                            │
│  ✅ Priority levels assigned                                │
│  ✅ Code examples (before/after)                            │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  STEP 6: Review & Approve                                   │
├─────────────────────────────────────────────────────────────┤
│  👀 Review Claude's suggestions                             │
│  ✔️  Approve changes (all or by component)                  │
│  💬 Ask questions if unclear                                │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  STEP 7: Claude Implements                                  │
├─────────────────────────────────────────────────────────────┤
│  ⚙️  Updates component files                                │
│  🎨 Applies design tokens                                   │
│  🌈 Adds color coding                                       │
│  ✨ Implements hover states                                 │
│  📏 Fixes spacing                                           │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  STEP 8: Build & Test                                       │
├─────────────────────────────────────────────────────────────┤
│  🔨 Run: .\rebuild.bat                                      │
│  🚀 Launch Zenith                                           │
│  👀 Verify improvements                                     │
│  🎉 Enjoy your beautiful DAW!                               │
└─────────────────────────────────────────────────────────────┘
```

---

## 📊 What Each File Does

### 📘 Documentation Files

| File | Purpose | When to Read |
|------|---------|--------------|
| **FILE_LOCATIONS.md** | This file! Shows where everything is | **Read first** |
| **MODERN_DESIGN_SYSTEM.md** | Complete design theory, principles, examples | After overview |
| **COLOR_REFERENCE.md** | Visual color palette with all hex codes | When coding |
| **IMPLEMENTATION_GUIDE.md** | Step-by-step how to apply manually | If doing manually |
| **CLAUDE_CODE_PROMPT.md** | Detailed prompt for Claude Code | Copy to Claude |
| **QUICK_PROMPT.md** | Short version of prompt | Quick start |

### 🎨 Design System Files

| File | What's Inside | Who Uses It |
|------|---------------|-------------|
| **ZenithLookAndFeel.h** | Design tokens (colors, spacing, fonts) | All UI code |
| **ZenithLookAndFeel.cpp** | Implementation (button/slider drawing) | JUCE framework |

### 🖼️ UI Component Files (To Update)

| Component | What It Does | Priority | Changes Needed |
|-----------|--------------|----------|----------------|
| **ArrangerComponent** | Track display/timeline | **🔴 P1** | Color-coded tracks, elevation, spacing |
| **TransportControlComponent** | Play/record/stop | **🔴 P1** | Semantic colors, hover glows |
| **MasterOutputComponent** | Level meters | **🔴 P1** | Standard meter colors |
| **ZenithStatusBar** | Status info | **🟡 P2** | Elevation, spacing, typography |
| **InstrumentBrowserPanel** | Browser list | **🟡 P2** | Elevation, hover states |
| **ZenithTransportBar** | Transport controls | **🟡 P2** | Semantic colors |
| **PianoRollComponent** | Piano roll editor | **🟢 P3** | Elevation, colors |
| **ZenithButton** | Custom button | **🟢 P3** | Uses LookAndFeel (mostly done) |
| **ZenithKnob** | Custom knob | **🟢 P3** | Uses LookAndFeel (mostly done) |
| **ZenithSlider** | Custom slider | **🟢 P3** | Uses LookAndFeel (mostly done) |

---

## 🎨 Design System Quick Reference

### Colors You'll Use Most

```cpp
// Backgrounds (Material Design elevation)
Elevation::dp0   // #121212 - Main window
Elevation::dp2   // #232323 - Panels
Elevation::dp4   // #272727 - Buttons
Elevation::dp8   // #2e2e2e - Hover

// Text (87% opacity, not pure white!)
Colors::textPrimary     // #dedede - Main text
Colors::textSecondary   // #999999 - Labels

// Accents (vibrant!)
Colors::accentPrimary   // #00d9ff - Cyan
Colors::accentSecondary // #ff8c42 - Orange

// Transport (semantic)
Colors::playGreen   // #4caf50
Colors::recordRed   // #ff5252

// Meters (industry standard)
Colors::meterGreen  // Safe zone
Colors::meterAmber  // Caution
Colors::meterRed    // Clipping
```

### Functions You'll Use Most

```cpp
// Get track color (auto-cycles through 12)
auto color = ZenithLookAndFeel::getTrackColor(trackIndex);

// Check contrast (accessibility)
auto ratio = ZenithLookAndFeel::calculateContrastRatio(fg, bg);

// Get readable text for any background
auto textColor = ZenithLookAndFeel::ensureReadableText(bgColor, true);

// Draw glow (better than shadow in dark mode)
lookAndFeel.drawGlow(g, bounds, radius, color, glowSize);
```

---

## 🚦 Implementation Status

```
DESIGN SYSTEM
├── ✅ ZenithLookAndFeel.h - Complete design system interface
├── ✅ ZenithLookAndFeel.cpp - Full implementation
├── ✅ MODERN_DESIGN_SYSTEM.md - Complete documentation
├── ✅ COLOR_REFERENCE.md - Color palette
├── ✅ IMPLEMENTATION_GUIDE.md - How-to guide
└── ✅ Prompts for Claude Code - Ready to use

UI COMPONENTS (Waiting for Claude Code)
├── ⏳ ArrangerComponent - Needs color-coded tracks
├── ⏳ TransportControlComponent - Needs semantic colors
├── ⏳ MasterOutputComponent - Needs standard meter colors
├── ⏳ ZenithStatusBar - Needs elevation & spacing
├── ⏳ InstrumentBrowserPanel - Needs hover states
├── ⏳ ZenithTransportBar - Needs semantic colors
├── ⏳ PianoRollComponent - Needs elevation
├── ⏳ ZenithButton - Mostly handled by LookAndFeel
├── ⏳ ZenithKnob - Mostly handled by LookAndFeel
└── ⏳ ZenithSlider - Mostly handled by LookAndFeel
```

---

## 💡 Common Questions

### Q: Do I need to manually update files?
**A:** No! That's what Claude Code is for. Just give it the prompt and it will:
1. Analyze all the files
2. Suggest improvements
3. Implement the changes

### Q: What if I want to do it manually?
**A:** Use `IMPLEMENTATION_GUIDE.md` which has step-by-step instructions and code examples.

### Q: Which prompt should I use?
**A:** 
- **Full:** `CLAUDE_CODE_PROMPT.md` (recommended, most detailed)
- **Quick:** `QUICK_PROMPT.md` (faster, less detail)

### Q: Do I need to read all the documentation?
**A:** 
- **Must read:** FILE_LOCATIONS.md (this file)
- **Should skim:** MODERN_DESIGN_SYSTEM.md (overview)
- **Reference:** COLOR_REFERENCE.md (when coding)
- **Claude reads:** Everything (automatically)

### Q: What if Claude Code isn't available?
**A:** Use the implementation guide and update files manually with the examples provided.

### Q: How long will this take?
**A:**
- Claude analyzing: 2-5 minutes
- Claude suggesting: 5-10 minutes  
- You reviewing: 10-20 minutes
- Claude implementing: 20-40 minutes
- **Total: ~1 hour** (mostly automated!)

### Q: Can I approve changes incrementally?
**A:** Yes! Review and approve component by component. Start with Priority 1 items.

---

## ✅ Checklist

Before starting:
- [ ] Located all files in `C:\zenith\daw\zenith-core\`
- [ ] Read FILE_LOCATIONS.md (this file)
- [ ] Skimmed MODERN_DESIGN_SYSTEM.md
- [ ] Browsed COLOR_REFERENCE.md
- [ ] Opened Claude Code
- [ ] Opened project folder in Claude Code
- [ ] Ready to paste prompt!

After Claude implements:
- [ ] Rebuild project (`.\rebuild.bat`)
- [ ] Launch Zenith
- [ ] Check backgrounds (comfortable #121212, not harsh #000000)
- [ ] Check text (readable, not pure white)
- [ ] Check buttons (cyan accent, hover effects)
- [ ] Check tracks (color-coded)
- [ ] Check transport (semantic colors)
- [ ] Check meters (standard colors)
- [ ] Check spacing (consistent 8px grid)
- [ ] Enjoy your beautiful DAW! 🎉

---

## 🎉 You're All Set!

Everything is ready. Your next step:

1. **Open Claude Code:** https://claude.ai/code
2. **Open folder:** `C:\zenith\daw\zenith-core`
3. **Copy prompt:** From `CLAUDE_CODE_PROMPT.md` or `QUICK_PROMPT.md`
4. **Paste & go!** Let Claude work its magic

**Your DAW is about to get a major glow-up!** ✨🎨🚀
