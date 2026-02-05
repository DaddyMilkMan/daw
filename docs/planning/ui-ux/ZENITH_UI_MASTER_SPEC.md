# Zenith DAW: Ultimate UI Specification
## Gen Z Premium • Glassmorphism • 60fps Skia

**Version**: 2.0  
**Date**: February 3, 2026  
**Target**: Teens & Young Producers  
**Design Philosophy**: Clean enough to breathe, powerful enough to impress

---

## 🎯 DESIGN PILLARS

### 1. **"TikTok Clean, Pro Tools Deep"**
First impression: simple, sleek, Instagram-worthy
Second look: every feature Logic and Ableton have, hidden until needed

### 2. **Navigation Philosophy**
| Key | Action |
|-----|--------|
| `Tab` | Toggle Arrangement ↔ Session View |
| `Shift+Tab` | Toggle AI Jam View (overlay) |
| `Cmd/Ctrl+1` | Arrangement View |
| `Cmd/Ctrl+2` | Session View |
| `Cmd/Ctrl+3` | AI Jam View |

### 3. **Visual Identity**
- **70% Dark Mode** - True blacks for OLED, Gen Z preference
- **20% Glassmorphism** - Frosted glass panels, depth without clutter
- **10% Accent Pops** - Electric blue (#3b82f6) + Purple (#8b5cf6) gradients

---

## 🎨 COLOR SYSTEM (Extended from ZenithTheme.h)

```
BACKGROUND LAYERS (Depth System)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Layer 0: #050508   "The Void"      - Absolute background
Layer 1: #0a0a0f   "Canvas"        - Main workspace  
Layer 2: #121218   "Surface"       - Panels, sidebars
Layer 3: #1a1a24   "Elevated"      - Cards, track headers
Layer 4: #24243a   "Floating"      - Dropdowns, tooltips
Layer 5: #2e2e4a   "Modal"         - Dialogs, overlays

GLASS EFFECTS
━━━━━━━━━━━━━
Glass Light:   rgba(255,255,255, 0.05)  - Subtle frost
Glass Medium:  rgba(255,255,255, 0.08)  - Standard panels
Glass Strong:  rgba(255,255,255, 0.12)  - Active elements
Glass Border:  rgba(255,255,255, 0.10)  - Edge highlights

ACCENT GRADIENT (The "Zenith Glow")
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Primary:    linear-gradient(135deg, #3b82f6 → #8b5cf6)
Hover:      linear-gradient(135deg, #60a5fa → #a78bfa)  
Active:     linear-gradient(135deg, #2563eb → #7c3aed)
Glow:       0 0 20px rgba(59,130,246, 0.4)

SEMANTIC COLORS
━━━━━━━━━━━━━━━
Record:     #ef4444 (Red - unmistakable)
Play:       #22c55e (Green - go)
Solo:       #eab308 (Yellow/Gold - standout)
Mute:       #64748b (Slate - silenced)
Automation: #ec4899 (Pink - stands out on timeline)
MIDI:       #8b5cf6 (Purple - distinct from audio)
Audio:      #3b82f6 (Blue - default waveform)
```

---

## 📐 LAYOUT ARCHITECTURE

### Master Layout Structure
```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ▼ TOP BAR (48px) - Transport + Project Info + View Toggle                   │
├────────┬────────────────────────────────────────────────────────┬───────────┤
│        │                                                        │           │
│  LEFT  │                   MAIN CANVAS                          │   RIGHT   │
│ PANEL  │            (Arrangement/Session/Jam)                   │   PANEL   │
│ (280px)│                                                        │  (320px)  │
│        │                                                        │           │
│ Glass  │              Track Area / Clip Grid                    │  AI Chat  │
│ Effect │                                                        │  + Pads   │
│        │                                                        │           │
├────────┴────────────────────────────────────────────────────────┴───────────┤
│ ▲ BOTTOM BAR - Piano Roll / Mixer / Device Rack (Tabbed, 200-400px)        │
├─────────────────────────────────────────────────────────────────────────────┤
│ STATUS BAR (24px) - CPU, MIDI, Latency, Zoom, Position                      │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Panel Behavior
| Panel | Collapse | Resize | Content |
|-------|----------|--------|---------|
| Left | Double-click edge | 200-400px | Browser, Instruments, Effects |
| Right | Double-click edge | 280-400px | AI Wingman, Drum Pads, Inspector |
| Bottom | Tab to minimize | 150-500px | Piano Roll, Mixer, Device Rack |

---

## 🎹 VIEW 1: ARRANGEMENT VIEW (Tab)

### Visual Reference: Logic Pro + Ableton's precision

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ ◀ ▶ ⏹ ⏺   ⟲  │ 120.0 BPM │ 4/4 │ 1.1.1 ━━━●━━━ 64.1.1 │ [Arr] [Ses] [Jam] │
├──────────────────────────────────────────────────────────────────────────────┤
│                    │ 1   │ 2   │ 3   │ 4   │ 5   │ 6   │ 7   │ 8   │         │
│ ╭─────────────────╮├─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┤         │
│ │ 🎸 Bass Track   ││░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░│         │
│ │ ◉ S M │ ─●── dB ││     Bass_Loop_01            Bass_Fill_02     │         │
│ ╰─────────────────╯├──────────────────────────────────────────────┤         │
│ ╭─────────────────╮│                                              │         │
│ │ 🥁 Drums        ││░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░│         │
│ │ ◉ S M │ ─●── dB ││ Kick_Pattern    HiHat_Roll       Full_Beat  │         │
│ ╰─────────────────╯├──────────────────────────────────────────────┤ AI      │
│ ╭─────────────────╮│                                              │ WINGMAN │
│ │ 🎤 Vocals       ││░░░░░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░│         │
│ │ ◉ S M │ ─●── dB ││         Verse_01          Chorus_01         │  💬     │
│ ╰─────────────────╯├──────────────────────────────────────────────┤         │
│ ╭─────────────────╮│                                              │ [Pads]  │
│ │ 🎹 Synth Lead   ││░░░░░░░░░░░░░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░│         │
│ │ ◉ S M │ ─●── dB ││                 Synth_Melody_Main            │         │
│ ╰─────────────────╯├──────────────────────────────────────────────┤         │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Track Header Component (Left 280px)
```cpp
// SkiaTrackHeader.h structure
┌─────────────────────────────────────┐
│ 🎸 │ Bass Track           │ ▾ Menu │  <- Track name + color + dropdown
├─────────────────────────────────────┤
│  [◉]  [S]  [M]  [R]  │  🔌 2      │  <- Record arm, Solo, Mute, Monitor, Plugin count
├─────────────────────────────────────┤
│  ━━━━━●━━━━  │  ◀━●━▶  │  -3.2 dB │  <- Volume fader, Pan, Level display
├─────────────────────────────────────┤
│  ▓▓▓▓▓▓▓░░░░ │ -12 ─┃─ 0 ─┃─ +6  │  <- Meter (stereo, peak hold)
└─────────────────────────────────────┘
```

### Clip Appearance
```
Audio Clip (Blue family):
┌──────────────────────────────────────┐
│ ▼ Clip_Name.wav                   🔒 │  <- Name, lock icon
│ ≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋│  <- Waveform (cached)
│ ≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋≋│
└──────────────────────────────────────┘
  ↑ Left edge: Trim handle
  ↓ Bottom edge: Fade handles (circular)
  ↗ Top-right: Loop toggle

MIDI Clip (Purple family):
┌──────────────────────────────────────┐
│ ▼ Piano_Melody                    ♪  │
│ ▪▪▪▪    ▪▪▪▪▪▪▪▪    ▪▪▪▪    ▪▪▪▪▪▪│  <- Mini piano roll preview
│   ▪▪▪▪▪▪    ▪▪    ▪▪▪▪▪▪▪▪    ▪▪  │
└──────────────────────────────────────┘
```

### Timeline Ruler Features
- Beat/bar markers with subdivisions
- Tempo changes shown inline
- Time signature changes
- Locators (start/end markers)
- Loop region (highlighted in accent color)
- Playhead with glow effect

---

## 🔲 VIEW 2: SESSION VIEW (Tab)

### Visual Reference: Ableton Live + Bitwig's clip launching

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ ◀ ▶ ⏹ ⏺   ⟲  │ 120.0 BPM │ 4/4 │ Scene 3 Playing │ [Arr] [Ses] [Jam]       │
├──────────────────────────────────────────────────────────────────────────────┤
│         │  Bass    │  Drums   │  Vocals  │  Synth   │  FX      │  Master   │
├─────────┼──────────┼──────────┼──────────┼──────────┼──────────┼───────────┤
│ Scene 1 │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │           │
│   ▶     │ │ Bass │ │ │ Beat │ │ │      │ │ │ Pad  │ │ │ Rev  │ │   ▶ ALL   │
│  Intro  │ │ ▶    │ │ │ ▶    │ │ │ ○    │ │ │ ▶    │ │ │ ○    │ │           │
│         │ └──────┘ │ └──────┘ │ └──────┘ │ └──────┘ │ └──────┘ │           │
├─────────┼──────────┼──────────┼──────────┼──────────┼──────────┤           │
│ Scene 2 │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │   ▶ ALL   │
│   ●     │ │ Bass │ │ │ Full │ │ │ Vrs1 │ │ │ Lead │ │ │ Dly  │ │           │
│  Verse  │ │ ●    │ │ │ ●    │ │ │ ●    │ │ │ ●    │ │ │ ○    │ │  (Glows   │
│         │ └──────┘ │ └──────┘ │ └──────┘ │ └──────┘ │ └──────┘ │  when     │
├─────────┼──────────┼──────────┼──────────┼──────────┼──────────┤  queued)  │
│ Scene 3 │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │ ┌──────┐ │           │
│   ◎     │ │ Sub  │ │ │ Drop │ │ │ Chrs │ │ │ Arp  │ │ │ Full │ │   ▶ ALL   │
│  Drop   │ │ ◎    │ │ │ ◎    │ │ │ ◎    │ │ │ ◎    │ │ │ ◎    │ │           │
│         │ └──────┘ │ └──────┘ │ └──────┘ │ └──────┘ │ └──────┘ │           │
├─────────┼──────────┼──────────┼──────────┼──────────┼──────────┼───────────┤
│ Stop    │   ⏹      │    ⏹     │    ⏹     │    ⏹     │    ⏹     │   ⏹ ALL   │
├─────────┼──────────┼──────────┼──────────┼──────────┼──────────┼───────────┤
│         │ [S] [M]  │ [S] [M]  │ [S] [M]  │ [S] [M]  │ [S] [M]  │           │
│         │ ▓▓▓▓░░░  │ ▓▓▓░░░░  │ ▓▓░░░░░  │ ▓▓▓▓▓░░  │ ▓░░░░░░  │  ▓▓▓▓▓▓  │
│         │  -6 dB   │  -3 dB   │  -12 dB  │  0 dB    │  -18 dB  │   0 dB    │
└─────────┴──────────┴──────────┴──────────┴──────────┴──────────┴───────────┘
```

### Clip Slot States
```
Empty:          ┌──────┐
                │  ○   │  <- Dim circle, click to record
                └──────┘

Stopped:        ┌──────┐
                │ ▶    │  <- Triangle, accent color
                │ Name │
                └──────┘

Playing:        ┌══════┐
                ║ ●    ║  <- Filled circle, pulsing glow
                ║ Name ║     Border animated (breathing)
                └══════┘

Queued:         ┌──────┐
                │ ◎    │  <- Ring, blinking/pulsing
                │ Name │     Accent color pulse
                └──────┘

Recording:      ┌──────┐
                │ ⏺    │  <- Red record indicator
                │ Rec  │     Red pulsing border
                └──────┘
```

### Scene Launcher (Left column)
- Scene name editable
- Master launch button launches all clips in row
- Scene follow actions (next, random, loop)
- Color coding per scene

---

## 🤖 VIEW 3: AI JAM VIEW (Shift+Tab Overlay)

### The Zenith Differentiator - AI-Native Jamming

This is your **killer feature**. No one has this.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ░░░ AI JAM MODE ░░░                               │
│                      Press Shift+Tab to exit                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│    ┌─────────────────────────────────────────────────────────────────────┐  │
│    │                                                                     │  │
│    │     🎵  "Make me a lo-fi beat with jazz chords"                     │  │
│    │                                                                     │  │
│    │     ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 🎤           │  │
│    │                                                                     │  │
│    └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│    ┌───────────────────────────────────────────────────────────────────────┐│
│    │                         GENERATED STEMS                               ││
│    ├───────────┬───────────┬───────────┬───────────┬───────────────────────┤│
│    │  DRUMS    │  BASS     │  CHORDS   │  MELODY   │      VARIATIONS       ││
│    │  ┌─────┐  │  ┌─────┐  │  ┌─────┐  │  ┌─────┐  │  ┌──┐ ┌──┐ ┌──┐ ┌──┐ ││
│    │  │ ▶   │  │  │ ▶   │  │  │ ▶   │  │  │ ▶   │  │  │A │ │B │ │C │ │D │ ││
│    │  │▓▓▓▓▓│  │  │▓▓▓▓▓│  │  │▓▓▓▓▓│  │  │▓▓▓▓▓│  │  └──┘ └──┘ └──┘ └──┘ ││
│    │  │≋≋≋≋≋│  │  │≋≋≋≋≋│  │  │♪♪♪♪♪│  │  │♪♪♪♪♪│  │                      ││
│    │  └─────┘  │  └─────┘  │  └─────┘  │  └─────┘  │  [🔄 Regenerate]      ││
│    │  [Solo]   │  [Solo]   │  [Solo]   │  [Solo]   │                       ││
│    └───────────┴───────────┴───────────┴───────────┴───────────────────────┘│
│                                                                             │
│    ┌───────────────────────────────────────────────────────────────────────┐│
│    │  QUICK ACTIONS                                                        ││
│    │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────┐  ││
│    │  │ 🎲 Surprise │ │ 🔊 Louder   │ │ 🎸 Add Bass │ │ ✨ Make Dreamy  │  ││
│    │  │    Me       │ │             │ │    Drop     │ │                 │  ││
│    │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────┘  ││
│    │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────┐  ││
│    │  │ 🥁 808      │ │ 🎹 Jazz It  │ │ 🎤 Add      │ │ 💾 Export to    │  ││
│    │  │    Pattern  │ │    Up       │ │    Vocals   │ │    Arrangement  │  ││
│    │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────┘  ││
│    └───────────────────────────────────────────────────────────────────────┘│
│                                                                             │
│    ┌───────────────────────────────────────────────────────────────────────┐│
│    │  JAM LOOP       ◀  [1] [2] [4] [8] [16] bars  ▶     BPM: 85 ━●━━━━   ││
│    │  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    ││
│    │  ↑ Playhead                                                           ││
│    └───────────────────────────────────────────────────────────────────────┘│
│                                                                             │
│    ┌────────────────────────────────────────────────────────────────────┐   │
│    │  🧠 AI CHAT                                                        │   │
│    │  ────────────────────────────────────────────────────────────────  │   │
│    │  You: Make the drums more aggressive                               │   │
│    │  AI: Added distortion and boosted the kick. Try variation B! 🔥    │   │
│    │  You: Perfect, now add some vinyl crackle                          │   │
│    │  AI: Added lo-fi texture to the master bus. Sounds cozy! ✨        │   │
│    │  ────────────────────────────────────────────────────────────────  │   │
│    │  [Type or speak...]                                          🎤    │   │
│    └────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### AI Jam View Features

1. **Voice Input** - Speak naturally, AI understands
2. **Stem Generation** - Create individual parts from text
3. **Variation Cards** - A/B/C/D alternatives to try
4. **Quick Actions** - One-tap musical transformations
5. **Loop Preview** - Hear changes before committing
6. **Export to Arrangement** - Send stems to main timeline
7. **Chat History** - Conversational refinement

### Glassmorphism Implementation
```cpp
// In SkiaAIJamView.cpp
void drawBackground(SkCanvas* canvas) {
    // Frosted glass effect
    SkPaint glassPaint;
    glassPaint.setColor(SkColorSetARGB(200, 10, 10, 15));  // 78% opacity
    
    // Blur the underlying content
    sk_sp<SkImageFilter> blur = SkImageFilters::Blur(20, 20, nullptr);
    glassPaint.setImageFilter(blur);
    
    // Glass panel
    SkRRect glass = SkRRect::MakeRectXY(bounds, 16, 16);
    canvas->drawRRect(glass, glassPaint);
    
    // Inner glow border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
    canvas->drawRRect(glass, borderPaint);
}
```

---

## 🎛️ SHARED COMPONENTS

### Transport Bar (Top, 48px)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ⏮  ◀  ▶  ⏹  ⏺  ⏭  │ ⟲ │ 120.00 │ 4/4 │ 01:23:456 │ ●━━━━━●━━━━━━━━━━ │ ⚙ │
│                      │   │  BPM   │     │  Position │      Timeline      │   │
└─────────────────────────────────────────────────────────────────────────────┘

Icons (Phosphor Icons style - thin, modern):
- Play: Filled triangle when playing, outline when stopped
- Record: Red filled circle, pulsing when armed
- Loop: Circular arrows, accent color when active
```

### Right Panel - AI Wingman (320px)
```
┌──────────────────────────────────────┐
│ ✨ WINGMAN                      [─]  │
├──────────────────────────────────────┤
│                                      │
│  ╭──────────────────────────────╮    │
│  │ How can I help you create    │    │
│  │ today?                       │    │
│  ╰──────────────────────────────╯    │
│                                      │
│  ┌──────────────────────────────┐    │
│  │ "Add a punchy kick drum"     │ 🎤 │
│  └──────────────────────────────┘    │
│                                      │
│  ╭──────────────────────────────╮    │
│  │ I added a 909 kick to your   │    │
│  │ drum track with some light   │    │
│  │ compression. Want me to      │    │
│  │ adjust the punch?            │    │
│  │                              │    │
│  │ [More punch] [Less punch]    │    │
│  │ [Try different kick]         │    │
│  ╰──────────────────────────────╯    │
│                                      │
├──────────────────────────────────────┤
│ 🧠 Thinking...  ●●●                  │
└──────────────────────────────────────┘
```

### Drum Pads (Right Panel, toggleable)
```
┌──────────────────────────────────────┐
│ 🥁 PADS                    [Drums ▾] │
├──────────────────────────────────────┤
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐    │
│  │ 🔴  │ │ 🟠  │ │ 🟡  │ │ 🟢  │    │
│  │ Kick│ │Snare│ │ Hat │ │ Clap│    │
│  └─────┘ └─────┘ └─────┘ └─────┘    │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐    │
│  │ 🔵  │ │ 🟣  │ │ ⚪  │ │ 🟤  │    │
│  │ Tom │ │Crash│ │ Ride│ │ Perc│    │
│  └─────┘ └─────┘ └─────┘ └─────┘    │
├──────────────────────────────────────┤
│ Velocity: ━━━━━━━━━●━━━   Swing: 50% │
└──────────────────────────────────────┘

Pad interaction:
- Click: Trigger sample
- Hold + Drag up/down: Velocity
- Glow on trigger (50ms decay)
- Color matches assigned sample type
```

### Bottom Panel - Tabbed (Piano Roll / Mixer / Devices)
```
┌─────────────────────────────────────────────────────────────────────────────┐
│ [Piano Roll] │ [Mixer] │ [Devices] │ [Sample Editor]               │ ▼ Hide │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Piano Roll when selected:                                                  │
│  ┌──────┬────────────────────────────────────────────────────────────────┐ │
│  │ C5   │                    ▓▓▓▓▓▓                                      │ │
│  │ B4   │            ▓▓▓▓▓▓▓▓                                            │ │
│  │ A4   │ ▓▓▓▓▓▓▓▓                      ▓▓▓▓▓▓▓▓                         │ │
│  │ G4   │                                          ▓▓▓▓▓▓▓▓▓▓▓▓          │ │
│  │ ...  │ 1       │ 2       │ 3       │ 4       │ 5       │ 6       │    │ │
│  └──────┴────────────────────────────────────────────────────────────────┘ │
│  Velocity: ▓▓▓ ▓▓▓▓▓ ▓▓ ▓▓▓▓▓▓▓ ▓▓▓ ▓▓▓▓ ▓▓▓▓▓▓▓▓▓                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## ✨ MICRO-INTERACTIONS & ANIMATIONS

### Button States
```
Default:    bg: transparent, border: subtle
Hover:      bg: white 8%, border: default, scale: 1.02
Pressed:    bg: white 12%, scale: 0.98
Disabled:   opacity: 0.4, cursor: not-allowed
```

### Transitions (all 200ms ease-out)
```css
/* Panel slide */
.panel-slide {
    transition: transform 200ms cubic-bezier(0.4, 0, 0.2, 1);
}

/* Fade in/out */
.fade {
    transition: opacity 150ms ease-out;
}

/* Scale on hover */
.button-hover {
    transition: transform 100ms ease-out, background 100ms ease-out;
}
```

### Special Effects
1. **Playhead Glow**: Soft accent-colored glow that pulses slightly
2. **Record Pulse**: Red glow that breathes (0.8s period)
3. **Clip Waveform**: Animate drawing left-to-right on first render
4. **Meter Smoothing**: 30ms attack, 300ms release
5. **Hover Lift**: Cards lift 2px with subtle shadow on hover
6. **Focus Ring**: 2px accent ring with 2px offset, animated appear

---

## 📱 RESPONSIVE BREAKPOINTS

```
Desktop XL (1920px+):   Full layout, all panels visible
Desktop    (1440px+):   Standard, right panel collapsible
Laptop     (1280px+):   Compact headers, thinner panels
Small      (1024px+):   Single panel visible, tabs to switch
```

---

## 🎨 TYPOGRAPHY

```
Font Stack: "Inter", "SF Pro", -apple-system, sans-serif

Sizes:
- Display:    28px, Bold,   Letter-spacing: -0.02em
- Heading:    18px, Medium, Letter-spacing: -0.01em
- Body:       14px, Regular, Letter-spacing: 0
- Small:      12px, Regular, Letter-spacing: 0
- Tiny:       10px, Medium,  Letter-spacing: 0.02em (uppercase)

Line Height:  1.4 for body, 1.2 for headings
```

---

## 🔧 SKIA IMPLEMENTATION PATTERNS

### Base Component Structure
```cpp
class SkiaArrangementView : public SkiaComponent {
public:
    void drawSkia(SkCanvas* canvas) override {
        drawBackground(canvas);
        drawTrackHeaders(canvas);
        drawTimeline(canvas);
        drawClips(canvas);
        drawPlayhead(canvas);
        drawOverlays(canvas);
    }
    
private:
    // Cached render layers (SkPicture)
    sk_sp<SkPicture> trackHeadersPicture_;
    sk_sp<SkPicture> gridPicture_;
    
    // Animation state
    float playheadX_ = 0.0f;
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    float zoomX_ = 1.0f;
    float zoomY_ = 1.0f;
    
    // Interaction state
    bool isDragging_ = false;
    SelectionRect selection_;
};
```

### Glass Panel Effect
```cpp
void drawGlassPanel(SkCanvas* canvas, SkRect bounds, float blur = 20.0f) {
    canvas->save();
    
    // Clip to rounded rect
    SkRRect rrect = SkRRect::MakeRectXY(bounds, 12, 12);
    canvas->clipRRect(rrect, true);
    
    // Background blur (expensive - cache when possible)
    SkPaint blurPaint;
    blurPaint.setImageFilter(SkImageFilters::Blur(blur, blur, nullptr));
    canvas->saveLayer(nullptr, &blurPaint);
    canvas->restore();
    
    // Tinted overlay
    SkPaint overlayPaint;
    overlayPaint.setColor(SkColorSetARGB(200, 18, 18, 24));
    canvas->drawRRect(rrect, overlayPaint);
    
    // Inner border (light)
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(SkColorSetARGB(25, 255, 255, 255));
    canvas->drawRRect(rrect, borderPaint);
    
    canvas->restore();
}
```

### Accent Gradient
```cpp
SkPaint getAccentGradientPaint(SkRect bounds) {
    SkPaint paint;
    SkPoint points[2] = {
        {bounds.fLeft, bounds.fTop},
        {bounds.fRight, bounds.fBottom}
    };
    SkColor colors[2] = {
        SkColorSetRGB(59, 130, 246),   // #3b82f6
        SkColorSetRGB(139, 92, 246)    // #8b5cf6
    };
    paint.setShader(SkGradientShader::MakeLinear(
        points, colors, nullptr, 2, SkTileMode::kClamp
    ));
    return paint;
}
```

---

## 📂 FILE STRUCTURE

```
apps/desktop/Source/ui/
├── views/
│   ├── SkiaArrangementView.h/.cpp      # Main arrangement
│   ├── SkiaSessionView.h/.cpp          # Clip launcher
│   ├── SkiaAIJamView.h/.cpp            # AI overlay
│   └── ViewSwitcher.h/.cpp             # Tab/Shift+Tab logic
├── arranger/
│   ├── SkiaTrackHeader.h/.cpp
│   ├── SkiaTrackLane.h/.cpp
│   ├── SkiaClipComponent.h/.cpp
│   ├── SkiaTimelineRuler.h/.cpp
│   └── SkiaPlayhead.h/.cpp
├── session/
│   ├── SkiaClipSlot.h/.cpp
│   ├── SkiaSceneLauncher.h/.cpp
│   └── SkiaSessionMixer.h/.cpp
├── ai-jam/
│   ├── SkiaAIPromptBar.h/.cpp
│   ├── SkiaStemCard.h/.cpp
│   ├── SkiaQuickActions.h/.cpp
│   ├── SkiaJamLoop.h/.cpp
│   └── SkiaAIChatPanel.h/.cpp
├── common/
│   ├── SkiaTransportBar.h/.cpp
│   ├── SkiaWingmanPanel.h/.cpp
│   ├── SkiaDrumPads.h/.cpp
│   └── SkiaBottomPanel.h/.cpp
└── design-system/
    ├── ZenithTheme.h/.cpp              # Colors, typography, spacing
    ├── SkiaGlassPanel.h/.cpp           # Glassmorphism component
    ├── SkiaButton.h/.cpp
    ├── SkiaSlider.h/.cpp
    ├── SkiaKnob.h/.cpp
    └── SkiaMeter.h/.cpp
```

---

## 🎯 IMPLEMENTATION PRIORITY

### Phase 1: Core Views (Week 1-2)
1. `ViewSwitcher` - Tab/Shift+Tab navigation
2. `SkiaArrangementView` - Basic layout, track headers, timeline
3. `SkiaSessionView` - Grid layout, clip slots

### Phase 2: Transport & Controls (Week 2-3)
4. `SkiaTransportBar` - Play/stop/record, tempo, position
5. `SkiaTrackHeader` - Arm, solo, mute, fader, meter
6. `SkiaClipComponent` - Waveform preview, drag handles

### Phase 3: AI Jam View (Week 3-4)
7. `SkiaAIJamView` - Overlay with glassmorphism
8. `SkiaStemCard` - Generated stem preview
9. `SkiaAIPromptBar` - Text/voice input
10. `SkiaQuickActions` - One-tap buttons

### Phase 4: Polish (Week 4-5)
11. Micro-interactions
12. Keyboard shortcuts
13. Accessibility (screen reader, high contrast)
14. Performance optimization (SkPicture caching)

---

## 🏁 SUCCESS METRICS

- **60fps** on 1080p with 100 tracks visible
- **<16ms** frame time for all animations
- **Zero** JUCE graphics calls (100% Skia)
- **<200MB** GPU memory for typical project
- **Pixel-perfect** at all DPI scales (1x, 1.5x, 2x, 3x)

---

*This is the UI that makes teens say "this looks fire" and pros say "this is serious."*
