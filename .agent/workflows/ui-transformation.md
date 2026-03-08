---
description: Complete UI transformation to apply the premium matte-black / blue-accent design system
---

# 🔥 UI TRANSFORMATION MASTER PLAN

## Overview

This is the orchestration document for transforming Zenith DAW's UI from **"Infrastructure-Ready Skeleton"** to **"Premium Production-Ready"**.

Execute these 5 workflows IN ORDER. Each depends on the previous.

---

## EXECUTION ORDER

### Phase 1: /ui-fonts-integration
**Priority: CRITICAL**
**Estimated Time: 2-4 hours**

Without custom fonts, everything looks amateur. This is the foundation.

Deliverables:
- Inter font family loaded and cached
- JetBrains Mono for code/numeric displays
- All UI text using design system fonts
- Subpixel antialiasing configured

Trigger: `/ui-fonts-integration`

---

### Phase 2: /ui-icon-system  
**Priority: CRITICAL**
**Estimated Time: 3-5 hours**

Unicode symbols must die. Professional vector icons.

Deliverables:
- ZenithIcons.h with all transport/UI icons as SkPath
- drawIcon() helper with proper scaling
- All buttons using vector icons
- Icon glow effects for active states

Trigger: `/ui-icon-system`

---

### Phase 3: /ui-widget-refactor
**Priority: HIGH**
**Estimated Time: 4-6 hours**

Split the mega-file, eliminate duplicates, add polish.

Deliverables:
- ZenithUIComponents.h split into components/ directory
- Each widget in own .h/.cpp pair
- Duplicate button class eliminated
- Knobs show value on hover
- Double-click reset, shift+fine control

Trigger: `/ui-widget-refactor`

---

### Phase 4: /ui-backdrop-blur
**Priority: MEDIUM-HIGH**
**Estimated Time: 4-8 hours**

Real glassmorphism, not fake transparency.

Deliverables:
- BackdropBlur utility class
- Real blur on TransportBar, RightSidePanel
- Performance settings for blur quality
- Fallback for low-end systems

Trigger: `/ui-backdrop-blur`

Note: This is technically complex. May require render-to-texture approach.

---

### Phase 5: /ui-waveform-thumbnails
**Priority: HIGH**
**Estimated Time: 4-6 hours**

Clips must show content, not empty rectangles.

Deliverables:
- WaveformThumbnail class with peak caching
- MidiThumbnail class for note visualization
- Async thumbnail generation
- Proper LOD for different zoom levels

Trigger: `/ui-waveform-thumbnails`

---

## TOTAL ESTIMATED TIME
**17-29 hours of focused work**

---

## VERIFICATION PHILOSOPHY

Each workflow includes:
1. **PRE-TASK RESEARCH** - Web searches to verify approach
2. **STEP-BY-STEP IMPLEMENTATION** - No ambiguity
3. **VERIFICATION CHECKLIST** - Concrete pass/fail criteria
4. **ACCEPTANCE CRITERIA** - Visual proof required

**NO TASK IS COMPLETE WITHOUT VERIFICATION.**

---

## GIT WORKFLOW

For each phase:
```bash
git checkout -b feature/ui-phase-N-description
# ... do work ...
git add -A
git commit -m "UI Phase N: Description"
git checkout master
git merge feature/ui-phase-N-description
```

Or use single branch:
```bash
git checkout -b feature/ui-transformation
# ... all phases ...
git checkout master
git merge feature/ui-transformation --no-ff
```

---

## SUCCESS CRITERIA

The UI transformation is **COMPLETE** when:

1. [ ] First-time viewers say "wow, this looks professional"
2. [ ] Fonts are crisp and consistent across DPI scales
3. [ ] Icons are sharp at all sizes
4. [ ] Panels have visible blur effect
5. [ ] Knobs/sliders feel premium with value display
6. [ ] Clips show waveforms and MIDI notes
7. [ ] 60fps performance maintained with all effects

---

## ANTI-PATTERNS TO AVOID

❌ **Premature optimization** - Get it working first, optimize later  
❌ **Half implementations** - Finish each phase completely before moving on  
❌ **Skipping research** - Web searches are MANDATORY, not optional  
❌ **Ignoring verification** - If checklist items fail, fix before proceeding  
❌ **Scope creep** - Stick to the defined deliverables  

---

## AGENT INSTRUCTIONS

If you are an AI agent executing these workflows:

1. **Read the entire workflow file first** - Don't start coding until you understand scope
2. **Do the web searches** - Don't assume you know the API. Verify.
3. **Complete each step** - No placeholders, no TODOs, no stubs
4. **Test after each phase** - Build must succeed before moving on
5. **Document any deviations** - If you change approach, explain why
6. **Take screenshots** - Visual proof of before/after required

---

## START HERE

To begin transformation:
```
/ui-fonts-integration
```

Good luck. Make it beautiful. 🚀
