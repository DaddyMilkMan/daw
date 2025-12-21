# Zenith PolySynth UI Design - Persona Interviews

**Date:** 2025-11-27  
**Goal:** Design simple, expandable UI like Logic Pro

---

## 🎭 **PERSONA INTERVIEWS: UI DESIGN**

### **Interview Question:**
*"You open Zenith PolySynth in your DAW. What do you want to see in the main window? What should be hidden in advanced sections?"*

---

## 👤 **ALEX - EDM Producer (5 years experience)**

### **Initial Window (Simple View):**
```
Alex: "I want to see the essentials immediately. Like, the stuff I touch 90% of the time."

MUST SEE (Main Window):
├─ OSCILLATOR SECTION
│  ├─ Waveform selector (dropdown or buttons)
│  ├─ Sub level (NEW! This is essential for bass)
│  └─ Noise level (for texture)
│
├─ FILTER SECTION  
│  ├─ Cutoff (big knob)
│  ├─ Resonance
│  ├─ Filter Env Amount (NEW! This is key)
│  └─ Filter type (LP/BP/HP buttons)
│
├─ ENVELOPE SECTION
│  ├─ Attack
│  ├─ Decay  
│  ├─ Sustain
│  ├─ Release
│  └─ (Just the amp envelope, not mod envelope)
│
└─ EFFECTS SECTION
   ├─ Distortion (knob)
   ├─ Chorus (knob)
   └─ Reverb (when you add it)

HIDDEN (Advanced Sections):
├─ "OSC DETAILS" button → Opens: Detune, Unison, individual osc mixes
├─ "MOD MATRIX" button → Opens: Full modulation routing
├─ "FILTER 2" button → Opens: Second filter controls
└─ "ADVANCED" button → Opens: Glide, mono mode, quality settings

Alex: "Keep it clean. I don't want to see 50 knobs. Just the core stuff."
```

### **Layout Preference:**
```
Alex: "Horizontal sections, left to right:
[OSCILLATOR] [FILTER] [ENVELOPE] [EFFECTS]

Each section has like 4-6 knobs max. Clean, modern, dark theme.
Big knobs I can grab quickly."
```

### **Size:**
```
Alex: "Small window by default. Like 600x400 pixels. 
If I click 'Advanced', it expands down or to the right."
```

---

## 👤 **MAYA - Beginner (6 months experience)**

### **Initial Window (Simple View):**
```
Maya: "I need labels! And I want to understand what each thing does."

MUST SEE (Main Window):
├─ SOUND SECTION (top)
│  ├─ Preset browser (dropdown)
│  └─ "What kind of sound?" helper text
│
├─ SHAPE SECTION
│  ├─ Waveform (with pictures of the waves!)
│  ├─ Brightness (this should be filter cutoff, but call it "Brightness")
│  └─ Thickness (this should be unison/sub, but call it "Thickness")
│
├─ MOVEMENT SECTION
│  ├─ Attack (with tooltip: "How fast the sound starts")
│  ├─ Release (with tooltip: "How long the sound fades")
│  └─ Filter Sweep (this is filter env amount)
│
└─ EFFECTS SECTION
   ├─ Space (reverb)
   ├─ Warmth (distortion)
   └─ Width (chorus)

HIDDEN (Advanced):
├─ "Show Advanced" checkbox
└─ Everything else (modulation, LFOs, etc.)

Maya: "I don't want to see 'LFO1 Rate' or 'Osc2 Detune'. 
I want to see 'Brightness' and 'Thickness'. 
Use simple words!"
```

### **Layout Preference:**
```
Maya: "Top to bottom:
[PRESET SELECTOR]
[SHAPE: Waveform, Brightness, Thickness]
[MOVEMENT: Attack, Release, Filter Sweep]
[EFFECTS: Space, Warmth, Width]
[Show Advanced checkbox]

Make it colorful! Not too dark. I want to see what I'm doing."
```

### **Size:**
```
Maya: "Small! Like 500x500 pixels. 
I don't want it taking up my whole screen."
```

---

## 👤 **JORDAN - Sound Designer (10+ years experience)**

### **Initial Window (Simple View):**
```
Jordan: "I want quick access to sound shaping, but I also need depth."

MUST SEE (Main Window):
├─ OSCILLATORS (compact)
│  ├─ Osc 1: [Wave] [Detune] [Mix]
│  ├─ Osc 2: [Wave] [Detune] [Mix]  
│  ├─ Osc 3: [Wave] [Detune] [Mix]
│  ├─ Sub: [Level]
│  └─ Noise: [Level]
│
├─ FILTER (dual display)
│  ├─ Filter 1: [Type] [Cutoff] [Res] [Env]
│  ├─ Filter 2: [Type] [Cutoff] [Res] [Env]
│  └─ Routing: [Serial/Parallel toggle]
│
├─ ENVELOPES (tabbed)
│  ├─ Tab: Amp | Mod
│  └─ [A] [D] [S] [R] (for selected envelope)
│
└─ MODULATION (compact matrix)
   ├─ Quick slots: [Source] → [Dest] [Amount]
   └─ "Show Full Matrix" button

EFFECTS (bottom bar):
├─ [Dist] [Chorus] [Reverb] [Delay]

HIDDEN (Expandable):
├─ "Unison" section (click to expand)
├─ "LFO Details" (click to expand)
└─ "Advanced" (glide, mono, quality)

Jordan: "I want density but organization. 
Show me everything, but group it logically."
```

### **Layout Preference:**
```
Jordan: "Grid layout:
┌─────────────┬─────────────┐
│ OSCILLATORS │   FILTER    │
├─────────────┼─────────────┤
│  ENVELOPES  │ MODULATION  │
├─────────────┴─────────────┤
│        EFFECTS             │
└────────────────────────────┘

Compact but not cramped. Dark theme, colored sections."
```

### **Size:**
```
Jordan: "Medium window. 800x600 pixels. 
Can resize if needed, but that's a good default."
```

---

## 📊 **CONSENSUS ANALYSIS**

### **Common Requests:**
1. ✅ **Small default window** (500-800px wide)
2. ✅ **Essential controls visible** (filter, envelope, effects)
3. ✅ **Advanced features hidden** (expandable sections)
4. ✅ **Dark theme** (Alex & Jordan prefer)
5. ✅ **Clear sections** (all three want organization)

### **Conflicts:**
1. **Terminology:**
   - Maya wants "Brightness" (beginner-friendly)
   - Jordan wants "Cutoff" (technical)
   - **Solution:** Use "Brightness (Cutoff)" or mode toggle

2. **Density:**
   - Maya wants minimal (5-10 controls)
   - Jordan wants comprehensive (20-30 controls)
   - **Solution:** Beginner/Advanced mode toggle

3. **Layout:**
   - Alex wants horizontal sections
   - Maya wants vertical flow
   - Jordan wants grid
   - **Solution:** Responsive layout that adapts

---

## 🎨 **RECOMMENDED UI DESIGN**

### **Mode 1: SIMPLE (Default)**
**Target:** Maya & Alex  
**Size:** 600x400px  
**Controls:** 12-15 essential knobs

```
┌─────────────────────────────────────────┐
│  ZENITH POLY SYNTH      [Simple ▼]     │
├─────────────────────────────────────────┤
│  SOUND                                  │
│  ┌─────┐ ┌─────┐ ┌─────┐              │
│  │Wave │ │ Sub │ │Noise│              │
│  └─────┘ └─────┘ └─────┘              │
├─────────────────────────────────────────┤
│  FILTER                                 │
│  ┌─────┐ ┌─────┐ ┌─────┐ [LP][BP][HP] │
│  │Cutoff│ │ Res │ │ Env │              │
│  └─────┘ └─────┘ └─────┘              │
├─────────────────────────────────────────┤
│  ENVELOPE                               │
│  ┌───┐ ┌───┐ ┌───┐ ┌───┐              │
│  │ A │ │ D │ │ S │ │ R │              │
│  └───┘ └───┘ └───┘ └───┘              │
├─────────────────────────────────────────┤
│  EFFECTS                                │
│  ┌─────┐ ┌─────┐ ┌─────┐              │
│  │Dist │ │Chorus│ │Reverb│             │
│  └─────┘ └─────┘ └─────┘              │
└─────────────────────────────────────────┘
```

### **Mode 2: ADVANCED**
**Target:** Jordan  
**Size:** 800x600px (expands from simple)  
**Controls:** 30-40 controls

```
┌──────────────────────────────────────────────────┐
│  ZENITH POLY SYNTH      [Advanced ▼]            │
├──────────────────────────────────────────────────┤
│ OSCILLATORS          │  FILTER 1 & 2            │
│ Osc1 [▼][Det][Mix]  │  F1: [LP▼][Cut][Res][Env]│
│ Osc2 [▼][Det][Mix]  │  F2: [LP▼][Cut][Res][Env]│
│ Osc3 [▼][Det][Mix]  │  Route: [Serial][Parallel]│
│ Sub  [Level]         │                           │
│ Noise [Level]        │                           │
├──────────────────────┼───────────────────────────┤
│ ENVELOPES            │  MODULATION MATRIX        │
│ [Amp][Mod] ◄tabs     │  Slot 1: [LFO1→][Cut][50%]│
│ [A][D][S][R]         │  Slot 2: [Env2→][Res][30%]│
│                      │  [+ Add Slot]             │
├──────────────────────┴───────────────────────────┤
│ EFFECTS: [Dist][Chorus][Reverb][Delay]          │
│ UNISON: [Voices][Detune]  LFO: [Rate][Shape]    │
└──────────────────────────────────────────────────┘
```

---

## 🎯 **IMPLEMENTATION PLAN**

### **Phase 1: Simple Mode (Priority)**
1. Create base window (600x400px)
2. Implement essential sections:
   - Sound (waveform, sub, noise)
   - Filter (cutoff, res, env)
   - Envelope (ADSR)
   - Effects (dist, chorus, reverb)
3. Mode toggle (Simple/Advanced)

### **Phase 2: Advanced Mode**
1. Expand window to 800x600px
2. Add advanced sections:
   - Individual osc controls
   - Dual filters
   - Modulation matrix
   - LFO controls

### **Phase 3: Polish**
1. Tooltips for Maya
2. Preset browser
3. Visual feedback (modulation wiggle)
4. Resizable window

---

## 💬 **PERSONA QUOTES**

**Alex:**
> "Keep it simple by default. I don't want to see a modulation matrix when I'm just trying to make a bass."

**Maya:**
> "Please use simple words! And show me pictures of the waveforms!"

**Jordan:**
> "Give me a simple mode for quick sounds, but let me dive deep when I need to."

---

**Next Step:** Implement Simple Mode UI with Skia, starting with the essential controls!
