# 🗺️ MASTER INDEX - All Files for Zenith DAW Design System

## 📍 Quick Navigation

**START HERE:** 👉 `START_HERE.md` or `OPUS_GUIDE.md`

---

## 📚 Documentation Files

### 🌟 Essential Reading
| File | Purpose | Read When |
|------|---------|-----------|
| **START_HERE.md** | Visual overview, complete workflow | **First!** |
| **FILE_LOCATIONS.md** | Where everything is located | After start |
| **OPUS_GUIDE.md** | How to use with Claude Opus 4.5 | When ready to implement |

### 📖 Design System Documentation
| File | Purpose | Read When |
|------|---------|-----------|
| **MODERN_DESIGN_SYSTEM.md** | Complete design theory, principles, examples | Deep dive |
| **COLOR_REFERENCE.md** | Visual color palette with hex codes | While coding |
| **IMPLEMENTATION_GUIDE.md** | Step-by-step manual implementation | Manual approach |

### 🤖 For AI Implementation
| File | AI Assistant | When To Use |
|------|-------------|-------------|
| **OPUS_PROMPT.md** | Claude Opus 4.5 | Full detailed prompt |
| **OPUS_QUICK.md** | Claude Opus 4.5 | Quick version |
| **CLAUDE_CODE_PROMPT.md** | Claude Code | For IDE integration |
| **QUICK_PROMPT.md** | Claude Code | Short version |

### 🛠️ Helper Tools
| File | Type | Purpose |
|------|------|---------|
| **open-in-claude-code.bat** | Script | Opens project in Claude Code |
| **MASTER_INDEX.md** | Index | This file! |

---

## 🎨 Design System Implementation

### Core Files (Already Created)
| File | What It Contains | Status |
|------|------------------|--------|
| **Source/ui/ZenithLookAndFeel.h** | Design tokens (colors, spacing, fonts) | ✅ Complete |
| **Source/ui/ZenithLookAndFeel.cpp** | Implementation (all drawing code) | ✅ Complete |

### UI Components (Need Updates)
| Component | Priority | What Needs Fixing |
|-----------|----------|------------------|
| **ArrangerComponent.***| 🔴 P1 | Color-coded tracks, elevation, spacing |
| **TransportControlComponent.*** | 🔴 P1 | Semantic colors, hover glows |
| **MasterOutputComponent.*** | 🔴 P1 | Standard meter colors |
| **ZenithStatusBar.*** | 🟡 P2 | Elevation, spacing, typography |
| **InstrumentBrowserPanel.*** | 🟡 P2 | Hover states, elevation |
| **ZenithTransportBar.*** | 🟡 P2 | Semantic colors |
| **PianoRollComponent.*** | 🟢 P3 | Elevation, colors |
| **ZenithButton.*** | 🟢 P3 | LookAndFeel handles most |
| **ZenithKnob.*** | 🟢 P3 | LookAndFeel handles most |
| **ZenithSlider.cpp** | 🟢 P3 | LookAndFeel handles most |

---

## 🚀 Implementation Paths

### Path 1: Claude Opus 4.5 (Recommended)
```
1. Read: OPUS_GUIDE.md
2. Choose: OPUS_PROMPT.md (full) OR OPUS_QUICK.md (fast)
3. Copy prompt
4. Paste to Claude Opus 4.5
5. Review analysis
6. Approve implementation
7. Build & test
```

### Path 2: Claude Code
```
1. Read: START_HERE.md
2. Open Claude Code (claude.ai/code)
3. Open folder: C:\zenith\daw\zenith-core
4. Copy: CLAUDE_CODE_PROMPT.md OR QUICK_PROMPT.md
5. Paste to Claude Code
6. Review & approve
7. Build & test
```

### Path 3: Manual Implementation
```
1. Read: IMPLEMENTATION_GUIDE.md
2. Study: COLOR_REFERENCE.md
3. Reference: ZenithLookAndFeel.h
4. Update components one by one
5. Copy code examples
6. Build & test frequently
```

---

## 📖 Reading Order Recommendations

### For Quick Start (30 minutes)
1. `START_HERE.md` (5 min) - Overview
2. `OPUS_GUIDE.md` (5 min) - Choose prompt
3. `COLOR_REFERENCE.md` (5 min) - See colors
4. `OPUS_PROMPT.md` or `OPUS_QUICK.md` (2 min) - Copy prompt
5. **Paste to Claude Opus 4.5** (13 min) - Let Claude work

### For Deep Understanding (2 hours)
1. `START_HERE.md` (10 min)
2. `FILE_LOCATIONS.md` (10 min)
3. `MODERN_DESIGN_SYSTEM.md` (45 min) - Full theory
4. `COLOR_REFERENCE.md` (15 min)
5. `IMPLEMENTATION_GUIDE.md` (30 min)
6. `ZenithLookAndFeel.h` (10 min) - See tokens

### For Implementation (1-3 hours)
1. `OPUS_GUIDE.md` (5 min)
2. Choose and copy prompt (2 min)
3. **Claude analyzes** (10-15 min)
4. **Review analysis** (15-30 min)
5. **Approve & implement** (30-60 min)
6. **Build & test** (10-30 min)

---

## 🎯 Quick Reference Cards

### Design Token Quick Lookup
```cpp
// Backgrounds
Elevation::dp0  = #121212  // Base
Elevation::dp2  = #232323  // Panels
Elevation::dp4  = #272727  // Buttons
Elevation::dp8  = #2e2e2e  // Hover

// Text (87% opacity)
Colors::textPrimary    = #dedede
Colors::textSecondary  = #999999
Colors::textDisabled   = #616161

// Accents
Colors::accentPrimary  = #00d9ff  // Cyan
Colors::accentSecondary = #ff8c42  // Orange

// Semantic
Colors::playGreen = #4caf50
Colors::recordRed = #ff5252

// Spacing (8px grid)
Spacing::s  = 8px
Spacing::m  = 16px
Spacing::l  = 24px
```

### Common Patterns
```cpp
// Color-coded track
auto c = getTrackColor(i);
g.setColour(c.withAlpha(0.12f));

// Hover effect
if (hover) c = c.brighter(0.15f);

// Level meter
if (lvl<0.6f) c=meterGreen;
else if (lvl<0.9f) c=meterAmber;
else c=meterRed;
```

---

## ✅ Verification Checklist

### Before Implementation
- [ ] Read START_HERE.md
- [ ] Understand design system (MODERN_DESIGN_SYSTEM.md)
- [ ] Know color palette (COLOR_REFERENCE.md)
- [ ] Have prompt ready (OPUS_PROMPT.md or OPUS_QUICK.md)

### During Implementation
- [ ] Claude reads all design files
- [ ] Review analysis report carefully
- [ ] Approve changes incrementally
- [ ] Test after each component
- [ ] Keep documentation open

### After Implementation
- [ ] No pure black (#000000) anywhere
- [ ] No pure white (#FFFFFF) for text
- [ ] All backgrounds use Elevation system
- [ ] All text uses Colors::text*
- [ ] All spacing follows 8px grid
- [ ] Tracks are color-coded
- [ ] Transport uses semantic colors
- [ ] Meters use standard colors
- [ ] Hover states work
- [ ] Build succeeds
- [ ] UI looks modern & vibrant

---

## 🗂️ File Structure Map

```
C:\zenith\daw\zenith-core\

📘 START HERE
├── START_HERE.md               ⭐ Visual overview
├── OPUS_GUIDE.md              ⭐ How to use Opus 4.5
├── MASTER_INDEX.md            ← You are here!

📘 DOCUMENTATION
├── FILE_LOCATIONS.md          File locations guide
├── MODERN_DESIGN_SYSTEM.md   Complete design guide
├── COLOR_REFERENCE.md        Color palette
└── IMPLEMENTATION_GUIDE.md   Manual how-to

🤖 FOR OPUS 4.5
├── OPUS_PROMPT.md            Full detailed prompt
└── OPUS_QUICK.md             Quick version

🤖 FOR CLAUDE CODE
├── CLAUDE_CODE_PROMPT.md     Full detailed prompt
├── QUICK_PROMPT.md           Quick version
└── open-in-claude-code.bat   Helper script

🎨 DESIGN SYSTEM
└── Source/ui/
    ├── ZenithLookAndFeel.h      Design tokens
    └── ZenithLookAndFeel.cpp    Implementation

🖼️ UI COMPONENTS (Need updates)
└── Source/ui/
    ├── ArrangerComponent.*
    ├── TransportControlComponent.*
    ├── MasterOutputComponent.*
    ├── ZenithStatusBar.*
    ├── InstrumentBrowserPanel.*
    ├── ZenithTransportBar.*
    ├── PianoRollComponent.*
    ├── ZenithButton.*
    ├── ZenithKnob.*
    └── ZenithSlider.cpp
```

---

## 🎯 What Each File Type Does

### 📘 Documentation (.md files)
- Explain concepts
- Show examples
- Provide reference
- Guide implementation

### 🤖 Prompt Files (.md)
- Instructions for AI
- Copy-paste ready
- Complete or quick versions
- For Opus or Claude Code

### 🎨 Design System (.h/.cpp)
- Define design tokens
- Implement drawing code
- **Already complete!**
- UI components will use these

### 🖼️ UI Components (.cpp/.h)
- Application UI code
- **Need updates!**
- Will use design tokens
- AI will fix these

---

## 💡 Decision Tree

**Want to understand everything first?**
→ Read: MODERN_DESIGN_SYSTEM.md + COLOR_REFERENCE.md

**Want to implement quickly?**
→ Use: OPUS_QUICK.md with Claude Opus 4.5

**Want thorough implementation?**
→ Use: OPUS_PROMPT.md with Claude Opus 4.5

**Want IDE integration?**
→ Use: CLAUDE_CODE_PROMPT.md with Claude Code

**Want to do it manually?**
→ Follow: IMPLEMENTATION_GUIDE.md

**Not sure what to do?**
→ Start with: START_HERE.md

---

## 🆘 Help & Troubleshooting

### "I'm lost, where do I start?"
→ Open **START_HERE.md** - it has a visual workflow

### "Which prompt should I use?"
→ Open **OPUS_GUIDE.md** - it compares all options

### "What colors should I use?"
→ Open **COLOR_REFERENCE.md** - it has all hex codes

### "How do I implement manually?"
→ Open **IMPLEMENTATION_GUIDE.md** - step-by-step examples

### "What's the design theory?"
→ Open **MODERN_DESIGN_SYSTEM.md** - complete explanation

### "Where are the files?"
→ Open **FILE_LOCATIONS.md** - shows all locations

### "Claude needs more info?"
→ Point Claude to specific documentation files

---

## 🎉 Summary

**You have everything you need!**

✅ Complete design system (ZenithLookAndFeel.*)
✅ Full documentation (multiple .md files)
✅ AI prompts (for Opus & Claude Code)
✅ Implementation guides (manual & automated)
✅ Color reference (with hex codes)
✅ Helper tools (batch scripts)

**Next step:** Choose your path above and start!

**Your Zenith DAW is about to look incredible!** 🚀✨🎨
