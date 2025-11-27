# 🎯 OPUS 4.5 IMPLEMENTATION - CHOOSE YOUR PROMPT

## 📍 Files Location
All files: `C:\zenith\daw\zenith-core\`

---

## 🎯 Which Prompt To Use?

### Option 1: Full Detailed Prompt (Recommended)
**Use:** `OPUS_PROMPT.md`
**Best for:** Complete analysis and implementation
**Length:** ~400 lines
**Time:** Claude will take 10-15 min to analyze

✅ Most thorough
✅ Detailed formatting requirements
✅ Comprehensive examples
✅ Clear success criteria

**Copy this to Claude Opus 4.5:**
👉 Open `OPUS_PROMPT.md` and copy everything

---

### Option 2: Quick Prompt
**Use:** `OPUS_QUICK.md`
**Best for:** Fast implementation
**Length:** ~100 lines
**Time:** Claude will take 5-10 min to analyze

✅ All essential info
✅ Quick to read
✅ Focus on action

**Copy this to Claude Opus 4.5:**
👉 Open `OPUS_QUICK.md` and copy everything

---

### Option 3: Ultra-Quick (Inline Below)

**Just copy-paste this:**

```
Hey Opus! Implement my Zenith DAW design system.

READ FIRST:
- Source/ui/ZenithLookAndFeel.h (design tokens)
- Source/ui/ZenithLookAndFeel.cpp (implementation)
- MODERN_DESIGN_SYSTEM.md (principles)

ANALYZE & FIX:
- Source/ui/ArrangerComponent.* (add color-coded tracks)
- Source/ui/TransportControlComponent.* (semantic colors)
- Source/ui/MasterOutputComponent.* (standard meters)
- Source/ui/ZenithStatusBar.* (elevation/spacing)
- Source/ui/InstrumentBrowserPanel.* (hover states)

RULES:
✅ USE: Elevation::dp0-8 (NOT pure black), Colors::textPrimary (NOT pure white), Spacing::s/m/l (8px grid), getTrackColor(i), Colors::playGreen/recordRed/meter*, Typography::getBody()
❌ NEVER: #000000, #ffffff, custom grays, arbitrary spacing

PATTERN - Color tracks:
auto c = ZenithLookAndFeel::getTrackColor(i);
g.setColour(c.withAlpha(0.12f));

PATTERN - Hovers:
if(hover) c = c.brighter(0.15f);

PATTERN - Meters:
if(lvl<0.6f) c=meterGreen; else if(lvl<0.9f) c=meterAmber; else c=meterRed;

1. Analyze (report issues)
2. Wait for approval
3. Implement (comment changes)

Location: C:\zenith\daw\zenith-core\
```

---

## 🚀 Step-by-Step Instructions

### Step 1: Choose Your Prompt
- **Thorough?** → Use `OPUS_PROMPT.md`
- **Fast?** → Use `OPUS_QUICK.md`
- **Ultra-fast?** → Use inline above

### Step 2: Open Claude Opus 4.5
Go to: https://claude.ai

Make sure you're using **Claude Opus 4.5** (select from model dropdown)

### Step 3: Start New Conversation
Click "New conversation" or "+"

### Step 4: Paste The Prompt
Copy your chosen prompt and paste it into Claude

### Step 5: Let Claude Work
Claude will:
1. Ask to read files (approve it)
2. Analyze all components (~5-15 min)
3. Provide detailed analysis report
4. Wait for your approval
5. Implement changes after approval

### Step 6: Review Analysis
Claude will report issues like:
```
ArrangerComponent:
❌ Using pure black (line 45)
❌ No track colors
❌ Inconsistent spacing
Priority: CRITICAL
```

### Step 7: Approve Changes
Say: "Approved, please implement" or "Fix ArrangerComponent first"

### Step 8: Get Updated Code
Claude will provide complete updated files with:
- All changes commented
- Before/after snippets
- Full file contents

### Step 9: Test
```bash
cd C:\zenith\daw\zenith-core
.\rebuild.bat
.\build\Release\Zenith.exe
```

---

## 📊 What Each Prompt Contains

### OPUS_PROMPT.md (Full)
```
✅ Detailed instructions
✅ Complete rule set
✅ Multiple examples per pattern
✅ Success criteria checklist
✅ Expected output format
✅ Important notes section
✅ ~400 lines
```

### OPUS_QUICK.md (Quick)
```
✅ Essential instructions
✅ Core rules
✅ Key patterns only
✅ Quick reference
✅ ~100 lines
```

### Inline (Ultra-Quick)
```
✅ Bare minimum
✅ Action-focused
✅ Copy-paste ready
✅ ~30 lines
```

---

## 🎨 Design System Cheat Sheet

### Most Common Replacements

| ❌ DON'T | ✅ DO |
|---------|------|
| `juce::Colours::black` | `juce::Colour(Elevation::dp0)` |
| `juce::Colours::white` | `juce::Colour(Colors::textPrimary)` |
| `juce::Colour(0xff1a1a1a)` | `juce::Colour(Elevation::dp1)` |
| `juce::Colour(0xff2a2a2a)` | `juce::Colour(Elevation::dp4)` |
| `bounds.reduced(12, 8)` | `bounds.reduced(Spacing::m, Spacing::s)` |
| `g.setFont(14.0f)` | `g.setFont(Typography::getBody())` |
| `fillRoundedRect(5.0f)` | `fillRoundedRect(Radius::m)` |

### Quick Color Reference

```cpp
// Backgrounds
Elevation::dp0  // #121212
Elevation::dp2  // #232323
Elevation::dp4  // #272727
Elevation::dp8  // #2e2e2e

// Text
Colors::textPrimary    // #dedede
Colors::textSecondary  // #999999

// Accents
Colors::accentPrimary  // #00d9ff

// Semantic
Colors::playGreen   // #4caf50
Colors::recordRed   // #ff5252
```

---

## ✅ Verification Checklist

After implementation, check:
- [ ] No `#000000` or `#ffffff` in code
- [ ] All backgrounds use `Elevation::dpX`
- [ ] All text uses `Colors::text*`
- [ ] All spacing uses `Spacing::*`
- [ ] Tracks are color-coded
- [ ] Transport buttons use semantic colors
- [ ] Level meters use standard colors
- [ ] Hover states work
- [ ] Build succeeds
- [ ] UI looks modern and vibrant

---

## 💡 Pro Tips

1. **Start with full prompt** - better results
2. **Review analysis carefully** - Claude finds everything
3. **Approve incrementally** - one component at a time
4. **Test after each component** - catch issues early
5. **Keep documentation open** - reference COLOR_REFERENCE.md

---

## 🆘 If Something Goes Wrong

**Build errors:**
```bash
cd C:\zenith\daw\zenith-core
rmdir /s /q build
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

**Claude confused:**
- Paste relevant section from MODERN_DESIGN_SYSTEM.md
- Show specific example from IMPLEMENTATION_GUIDE.md
- Reference COLOR_REFERENCE.md for exact hex codes

**Wrong colors used:**
- Remind Claude: "Use Colors::textPrimary not pure white"
- Remind Claude: "Use Elevation::dp0 not pure black"
- Point to ZenithLookAndFeel.h for exact values

---

## 🎯 Quick Decision Guide

**I want thorough analysis:**
→ Use `OPUS_PROMPT.md` (full version)

**I want fast results:**
→ Use `OPUS_QUICK.md` (quick version)

**I want immediate action:**
→ Use inline ultra-quick version above

**I'm not sure:**
→ Start with `OPUS_QUICK.md`, upgrade to full if needed

---

## 📁 Files Summary

```
For Opus 4.5:
├── OPUS_PROMPT.md           ← Full detailed (recommended)
├── OPUS_QUICK.md            ← Quick version
└── THIS_FILE.md             ← You are here!

Design System:
├── Source/ui/ZenithLookAndFeel.h
├── Source/ui/ZenithLookAndFeel.cpp
├── MODERN_DESIGN_SYSTEM.md
├── COLOR_REFERENCE.md
└── IMPLEMENTATION_GUIDE.md

For Reference:
├── START_HERE.md
├── FILE_LOCATIONS.md
└── CLAUDE_CODE_PROMPT.md    ← For Claude Code, not Opus
```

---

## 🚀 Ready To Go!

1. **Choose prompt:** Full, Quick, or Ultra-quick
2. **Copy to clipboard**
3. **Open Claude Opus 4.5:** https://claude.ai
4. **Paste and send**
5. **Wait for analysis**
6. **Review and approve**
7. **Get beautiful code**
8. **Build and test**
9. **Enjoy! 🎉**

---

**Your DAW is about to look professional and modern!** ✨🎨🚀
