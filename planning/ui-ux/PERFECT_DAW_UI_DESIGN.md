# Perfect DAW UI Design Specification

## Executive Summary

This document defines the UI/UX for a custom DAW that combines the best-loved features from all major DAWs while eliminating their pain points. The design is **evidence-based** from user feedback across forums, YouTube, and reviews.

### Design Philosophy

**"Zero Friction, Maximum Flow"**

- 🎯 **Idea → Sound in 3 clicks** (Ableton's speed)
- 🎹 **Best-in-class editing** (FL's piano roll + Pro Tools' audio)
- 🧩 **Everything in reach** (Studio One's drag-and-drop)
- 🎨 **Beautiful by default** (premium matte-black surfaces with restrained blue accents)
- ⚡ **Customizable for power users** (Reaper's flexibility)
- 🤖 **AI-first workflow** (Wingman integrated)

---

## UI Architecture Overview

### The Tri-Pane Layout

```
┌─────────────────────────────────────────────────────────────────┐
│  TOP BAR: Transport, Master, Wingman AI Chat                    │ 80px
├───────┬─────────────────────────────────────────────┬───────────┤
│       │                                             │           │
│ LEFT  │         CENTER: MAIN WORKSPACE              │  RIGHT    │
│ PANEL │                                             │  PANEL    │
│       │  ┌────────────────────────────────────┐    │           │
│ Smart │  │  SESSION VIEW (Clip Launcher)      │    │  Mixer    │
│ Browse│  │  - or -                            │◄───┤  Inspector│
│       │  │  ARRANGEMENT VIEW (Timeline)       │    │  Browser  │
│ 250px │  │                                    │    │  250px    │
│       │  └────────────────────────────────────┘    │           │
│       │                                             │           │
├───────┴─────────────────────────────────────────────┴───────────┤
│  BOTTOM: EDITOR (Piano Roll / Audio Editor / Automation)        │ 400px
└─────────────────────────────────────────────────────────────────┘
```

**Inspired by:**
- Session/Arrangement split: **Ableton Live**
- Docked panels: **Studio One** (no floating windows!)
- Bottom editor: **FL Studio** (persistent, instant access)
- Clean aesthetics: matte obsidian surfaces + restrained electric-blue accents

---

## Component Breakdown

### 1. Top Bar (80px height)

```
┌─────────────────────────────────────────────────────────────────┐
│ [≡] Project | ◄◄ |◄ ● ► ►► | ♩=120 | 4/4 | [CPU:12%] [RAM:3GB]│
│                                                                  │
│ [🎤] "Hey Wingman..." _________________________ [Send] [⚙️] [?] │
└─────────────────────────────────────────────────────────────────┘
```

**Left Section (Transport):**
- Menu button (≡) → Project, Edit, View, Insert, Mix, Window
- Transport controls: Rewind, Back, Play/Pause, Stop, Forward, Loop
- Tempo display (click to type, scroll to adjust)
- Time signature
- CPU/RAM meters

**Right Section (Wingman AI):**
- **Voice input:** "Hey Wingman, create a trap beat"
- **Text input:** Type commands naturally
- **Quick actions:** Settings, Help
- **Status indicator:** Green dot = AI listening

**Design Notes:**
- Dark theme by default (less eye strain)
- **Always visible** (never hide transport)
- Wingman input **always accessible** (zero-click AI)
- Follows a studio-grade Skia language: matte obsidian, calm spacing, restrained blue focus accents

---

### 2. Left Panel: Smart Browser (250px width, collapsible to 40px)

```
┌─────────────────────┐
│ 🔍 Search...        │
├─────────────────────┤
│ 📁 Folders          │
│ 🎸 Instruments      │
│ 🎛️ Effects          │
│ 🎵 Samples          │
│ 🎹 Presets          │
│ 💾 Projects         │
├─────────────────────┤
│ ┌─────────────────┐ │
│ │  808 Kick      🔊│ │ ← Waveform preview
│ │  ▂▃▅▇▅▃▂       │ │
│ ├─────────────────┤ │
│ │  Snare Tight   🔊│ │
│ │  ▁▃▇▃▁         │ │
│ └─────────────────┘ │
├─────────────────────┤
│ [★] Favorites       │
│ [🕐] Recent         │
│ [🏷️] Tags           │
└─────────────────────┘
```

**Inspired by:**
- **Studio One** browser (unified, fast)
- **Logic Pro** loop browser (audition in tempo)
- **Ableton** collections/favorites

**Features:**
- **Unified search:** Finds everything (instruments, effects, samples, presets)
- **Visual previews:** Waveforms for audio, plugin UIs for instruments
- **Auto-audition:** Hover to preview in current project tempo/key
- **Drag-anywhere:** Drag to tracks, mixer, arranger, piano roll
- **Smart tags:** AI-generated tags (e.g., "aggressive", "warm", "punchy")
- **Wingman integration:** "Show me warm bass presets" → filters browser

**Collapse behavior:**
- Click chevron (◀) → collapses to 40px icon strip
- Hotkey `B` → toggle browser
- Auto-hide on drag (more space for editing)

---

### 3. Center: Main Workspace (Dynamic)

#### Mode 1: SESSION VIEW (Clip Launcher)

```
┌─────────────────────────────────────────────────────────────┐
│ Scene 1 ►  [Kick Clip] [Bass Clip] [       ] [Synth Clip]  │
│ Scene 2 ►  [Kick Var ] [       ] [Hi-Hat  ] [Synth Var ]    │
│ Scene 3 ►  [       ] [Bass Var] [       ] [Pad Clip  ]      │
│ Scene 4 ►  [       ] [       ] [Perc Loop] [       ]         │
│            ──────────────────────────────────────────────    │
│            Drums      Bass      Hi-Hats     Synth             │
│            [M][S]     [M][S]    [M][S]      [M][S]            │
│            ▓▓▓▓▓▓     ▓▓▓▓▓▓    ▓▓        ▓▓▓▓▓▓▓            │
│            -6.0dB     -3.0dB    -12.0dB    -2.0dB             │
└─────────────────────────────────────────────────────────────┘
```

**Inspired by:**
- **Ableton Live** Session View
- **Bitwig** clip launcher

**Features:**
- **Fire clips:** Click to launch
- **Scenes:** Launch entire horizontal row
- **Color coding:** Auto-color by track, manual override
- **Clip states:**
  - Empty (gray): Nothing recorded
  - Recording (red pulsing): Currently recording
  - Queued (yellow): Will play next bar
  - Playing (green): Currently playing
  - Stopped (dim): Clip exists but stopped
- **Drag & drop:** Rearrange clips, duplicate, move between tracks
- **Wingman:** "Launch scene 2 with a 4-bar loop"

**Toggle to Arrangement:**
- Tab key or button in top-right
- Smooth transition animation (clips fade to timeline)

#### Mode 2: ARRANGEMENT VIEW (Timeline)

```
┌─────────────────────────────────────────────────────────────┐
│     0s    10s   20s   30s   40s   50s   60s   70s   80s      │
│ ┌────────────────────────────────────────────────────────┐   │
│ │ Drums   ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│   │
│ │         [Intro      ][Verse 1   ][Chorus     ]       │   │
│ ├────────────────────────────────────────────────────────┤   │
│ │ Bass    ░░░░░░░░░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│   │
│ ├────────────────────────────────────────────────────────┤   │
│ │ Vocals              ▓▓▓▓▓[Take 1]▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│   │
│ │                     ░░░░░[Take 2]░░░░░░░░░░░░░░░░░░░░│   │ ← Comp lanes
│ │                     ▓▓▓▓▓[Take 3]▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│   │
│ ├────────────────────────────────────────────────────────┤   │
│ │ Synth             ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│   │
│ └────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

**Inspired by:**
- **Pro Tools** comp lanes (best audio comping)
- **Logic Pro** take folders
- **Studio One** arranger track

**Features:**
- **Comp lanes:** Record multiple takes, swipe to choose best parts
- **Arranger sections:** Name sections (Intro, Verse, Chorus), drag to rearrange
- **Clip gain:** Handles on clips for volume adjustment (like Pro Tools)
- **Elastic audio:** Time-stretch by dragging clip edges (like Ableton Warp)
- **Automation lanes:** Below each track, toggle visibility
- **Markers:** Click top ruler to add markers, name them
- **Wingman:** "Create a verse section here" → AI adds section, suggests length

**View modes:**
- Zoom: `+`/`-` keys or pinch gesture
- Fit to window: `F` key
- Vertical zoom: Shift + scroll

---

### 4. Bottom: Editor Panel (400px height, resizable)

#### Editor Mode 1: PIANO ROLL

```
┌─────────────────────────────────────────────────────────────┐
│ [Piano Roll] [Audio Editor] [Automation]  Scale:A Minor▼   │
├───┬─────────────────────────────────────────────────────────┤
│ C5│         ▓▓▓     ▓▓▓                                     │
│ B4│     ▓▓▓     ▓▓▓                                         │
│A#4│                                                          │
│ A4│ ▓▓▓                 ▓▓▓▓▓▓▓                             │ ← Velocity bars below
│G#4│                                                          │
│ G4│     ▓▓▓▓▓▓▓             ▓▓▓                             │
│F#4│                                                          │
│ F4│ ▓▓▓         ▓▓▓                                         │
│   │ ▃▅▇ ▅▇▃ ▇▅▃ ▃▅▇ ▃▃▇ ▅▇▃                                │ ← Velocity
│   └─────┬───────┬───────┬───────┬───────┬───────┬──────────┤
│         1       2       3       4       1       2           │
└─────────────────────────────────────────────────────────────┘
```

**Inspired by:**
- **FL Studio** (best piano roll ever)
- **Logic Pro** (great MIDI editing)
- **Cubase** Logical Editor

**FL Studio Features Implemented:**
- **Scale highlighting:** Gray out non-scale notes
- **Ghost notes:** See MIDI from other tracks (helps harmony)
- **Slide notes:** Drag note end to slide pitch (portamento)
- **Strumming tool:** Select chord, click strum → instant guitar strum
- **Arpeggiator:** Built-in arp tool
- **Humanize:** Randomize velocity/timing (make it less robotic)
- **Chop:** Auto-slice long notes
- **Riff machine:** Generate MIDI variations

**Cubase Features:**
- **Logical Editor:** "Select all notes below C3" → transform them
- **Quantize to groove:** Load audio groove, quantize MIDI to it
- **Expression maps:** For orchestral libraries (keyswitches, articulations)

**Wingman Integration:**
- **AI chord suggestions:** "What chord goes here?" → AI suggests
- **Generate melody:** "Add a melody over this" → AI creates MIDI
- **Harmonize:** Select notes → "harmonize in thirds" → AI adds harmony
- **Detect key:** "What key is this?" → AI analyzes, sets scale

**Editing:**
- Draw: Pencil tool (click to add notes)
- Select: Arrow tool (drag to select, resize)
- Erase: Delete tool (click notes)
- Split: Scissor tool (split notes)
- Velocity: Drag velocity bars below
- Multi-select: Shift+click, Cmd+A for all
- Copy/paste: Cmd+C / Cmd+V
- Duplicate: Alt+drag

#### Editor Mode 2: AUDIO EDITOR

```
┌─────────────────────────────────────────────────────────────┐
│ [Piano Roll] [Audio Editor] [Automation]   [Comp] [Stretch]│
├─────────────────────────────────────────────────────────────┤
│  Vocal Take 1  ▁▃▅▇█▇▅▃▁ ▃▅█▅▃ ▁▃▇▃▁                       │ ✓ Best
│  Vocal Take 2  ▂▄▆██▆▄▂ ▂▄█▄▂ ▂▄▆▄▂                         │
│  Vocal Take 3  ▁▃▅▇█▇▅▃▁ ▃▅█▅▃ ▁▃▇▃▁                       │ ✓ Best
│  ──────────────────────────────────────────────────────     │
│  Comp Result   ▁▃▅▇█▇▅▃▁ ▂▄█▄▂ ▁▃▇▃▁                       │ ← Final comp
│                ├─────────┤                                   │
│                Clip Gain: -2.5dB                             │
├─────────────────────────────────────────────────────────────┤
│ [Pitch -12c] [Formant +5] [Stretch 102%] [Fade In 10ms]    │ ← Tools
└─────────────────────────────────────────────────────────────┘
```

**Inspired by:**
- **Pro Tools** (gold standard audio editing)
- **Logic Pro** Flex Time/Pitch
- **Ableton** Warp modes

**Pro Tools Features:**
- **Comp lanes:** Record multiple takes, swipe best parts
- **Clip gain:** Rubber-band gain envelope on clip
- **Crossfades:** Auto-crossfade at edit points (configurable)
- **Strip silence:** Remove silence between phrases
- **Nudge:** Sample-accurate positioning (< / > keys)

**Ableton Features:**
- **Warp modes:** Complex (polyphonic), Complex Pro (best), Beats, Texture, etc.
- **Time-stretch:** Drag clip edge to stretch without changing pitch
- **Pitch-shift:** Transpose without changing speed
- **Formant:** Preserve vocal character when shifting

**Wingman Integration:**
- **Auto-comp:** "Comp these takes for me" → AI chooses best parts
- **Clean up:** "Remove breaths and clicks" → AI processes
- **Tune vocals:** "Tune this to C minor" → AI pitch correction
- **Tighten timing:** "Quantize to the grid" → AI elastic audio

**Tools:**
- Trim: Drag clip edges
- Fade: Drag fade handles (top corners)
- Split: Cmd+E at playhead
- Normalize: Right-click → Normalize
- Reverse: Right-click → Reverse
- Bounce: Render selection to new clip

#### Editor Mode 3: AUTOMATION

```
┌─────────────────────────────────────────────────────────────┐
│ [Piano Roll] [Audio Editor] [Automation]  Param: Volume ▼  │
├─────────────────────────────────────────────────────────────┤
│  0dB ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄   │
│      │                                                        │
│ -6dB ●━━━●                 ●━━━━━●                          │ ← Automation line
│      │   │                 │     │                           │
│-12dB ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄   │
│      │   │                 │     │                           │
│-∞ dB ┼───┴─────────────────┴─────┴──────────────────────    │
│      0s  10s               20s   30s                         │
├─────────────────────────────────────────────────────────────┤
│ [Line] [Curve] [S-Curve] [Stepped]  [Learn] [Clear]        │
└─────────────────────────────────────────────────────────────┘
```

**Inspired by:**
- **Bitwig** (modulation system)
- **Ableton** automation lanes
- **Logic Pro** automation curves

**Features:**
- **Draw modes:**
  - Line: Straight line between points
  - Curve: Bezier curve (drag points to adjust)
  - S-Curve: Smooth ease in/out
  - Stepped: Discrete jumps (for MIDI CC switching)
- **MIDI learn:** Click [Learn], touch any plugin parameter → auto-creates automation
- **Multiple lanes:** Stack automation for different parameters
- **Automation modes:**
  - Read: Playback automation
  - Touch: Write while touching control, read otherwise
  - Latch: Write from first touch until stop
  - Write: Always write (overwrites existing)

**Bitwig-Style Modulation:**
- Right-click any parameter → "Add modulator"
- Choose: LFO, Envelope, Audio Sidechain, MIDI, Macro
- Drag amount slider to control depth
- **Chain modulators:** LFO → Envelope → Filter cutoff (crazy routing!)

**Wingman Integration:**
- **Generate automation:** "Automate filter cutoff from 200Hz to 2kHz over 16 bars"
- **Copy automation:** "Copy volume automation from drums to bass"
- **Smart automation:** "Make this build up" → AI creates automation for all tracks

---

### 5. Right Panel: Mixer / Inspector (250px width, collapsible)

#### View Mode 1: MIXER

```
┌─────────────────────┐
│  Drums  Bass  Synth │
│  ──┬──  ──┬──  ──┬──│
│    │      │      │   │
│  [EQ]  [EQ]  [Comp] │ ← Plugin slots
│  [Comp][Sat] [EQ]   │
│    │      │      │   │
│    ●      ●      ●   │ ← Pan
│    │      │      │   │
│  ──┼──  ──┼──  ──┼──│ ← Fader
│    │      │      │   │
│    ▓      ▓      ▓   │
│    ▓      ▓▓     ▓▓  │
│    ▓▓     ▓▓     ▓▓▓ │ ← Meters
│    ▓▓     ▓▓▓    ▓▓▓ │
│  -6.0   -3.0   -2.0  │ ← dB
│  [M][S] [M][S] [M][S]│ ← Mute/Solo
└─────────────────────┘
```

**Inspired by:**
- **Pro Tools** mixer (clean, professional)
- **Logic Pro** mixer (good routing viz)
- **Console 1** (tactile feel)

**Features:**
- **Vertical strips:** Each track is a vertical channel strip
- **Plugin slots:** Click to add, drag to reorder
- **Pre/post fader sends:** Color-coded (blue = pre, green = post)
- **Groups:** Color-code and collapse track groups
- **VCA faders:** Control multiple tracks with one fader
- **Stereo/mono:** Toggle per track
- **Phase invert:** Button to flip phase
- **Input monitoring:** Off / On / Auto

**Wingman Integration:**
- **Auto-level:** "Balance this mix" → AI sets all faders
- **Reference matching:** "Make this sound like [reference track]"
- **Frequency unmasking:** "Fix the mud" → AI EQs to separate tracks
- **Add bus:** "Create a reverb bus" → AI adds return track with reverb

#### View Mode 2: INSPECTOR

```
┌─────────────────────┐
│ Track: Drums        │
│ ┌─────────────────┐ │
│ │ Name: Drums     │ │
│ │ Color: [🔴]     │ │
│ │ Icon: [🥁]      │ │
│ └─────────────────┘ │
├─────────────────────┤
│ Input: Stereo In 1  │
│ Output: Master      │
│ MIDI In: All        │
│ MIDI Out: None      │
├─────────────────────┤
│ 📎 Devices          │
│ ┌─────────────────┐ │
│ │ ► EQ Eight      │ │ ← Expandable
│ │   Low: +3dB     │ │
│ │   Mid: -2dB     │ │
│ │   High: +1dB    │ │
│ ├─────────────────┤ │
│ │ ► Compressor    │ │
│ │   Ratio: 4:1    │ │
│ │   Threshold:-12 │ │
│ │   Attack: 5ms   │ │
│ └─────────────────┘ │
├─────────────────────┤
│ 📊 Clip Properties  │
│ Start: 0.0s         │
│ Length: 4.0s        │
│ Loop: ✓             │
│ Fade In: 10ms       │
│ Fade Out: 50ms      │
└─────────────────────┘
```

**Features:**
- **Track properties:** Name, color, icon, routing
- **Device parameters:** Inline editing (no need to open plugin UI)
- **Clip properties:** Detailed clip settings
- **Macro controls:** User-defined macros (like Ableton Racks)

**Wingman Integration:**
- **Smart inspector:** "Show me all reverb parameters" → filters to reverbs
- **Batch edit:** "Set all compressor ratios to 4:1" → AI updates all

---

## Advanced UI Features

### 1. Modular View System (Bitwig-Inspired)

**The Grid View:**
Press `G` to enter Grid view → see track as modular routing:

```
┌─────────────────────────────────────────┐
│  [Audio In] ──→ [EQ] ──→ [Compressor]  │
│                   │                      │
│                   ↓                      │
│              [Sidechain] ←─── [Drums]   │
│                   │                      │
│                   ↓                      │
│              [Reverb] ──→ [Out]         │
└─────────────────────────────────────────┘
```

**Features:**
- Visual signal routing (like Reason's rack back panel)
- **Modulation routing:** See all modulations visually
- **Sidechain connections:** Drag from one track to another
- **Parallel processing:** Split signal, apply different FX, merge

**Wingman Integration:**
- "Show me the signal flow" → Opens Grid view
- "Route drums to sidechain compressor on bass" → AI creates routing

---

### 2. Command Palette (Reaper-Inspired)

Press `Cmd+K` → Opens command palette:

```
┌─────────────────────────────────────────┐
│ 🔍 Type a command...                    │
├─────────────────────────────────────────┤
│ ▶ Insert MIDI track                     │
│   Insert audio track                    │
│   Insert return track                   │
│   Duplicate track                       │
│   Delete track                          │
│   Freeze track                          │
│   Bounce track                          │
│   Export stems                          │
│ ──────────────────────────────────────  │
│ Recently used:                          │
│   • Normalize clip                      │
│   • Auto-tune vocals                    │
└─────────────────────────────────────────┘
```

**Features:**
- **Fuzzy search:** Type "ins mid" → finds "Insert MIDI track"
- **Keyboard shortcuts:** Shows shortcut next to each command
- **Custom actions:** Record macro, assign to palette
- **AI integration:** Wingman commands appear here too

**Reaper-Style Custom Actions:**
- Record sequence: "Start recording action"
- Perform sequence: Create track → Load plugin → Set parameters
- Save: "Save as macro: Setup Drum Track"
- Result: "Setup Drum Track" now in command palette

---

### 3. Smart Themes & Customization

**Theme Presets:**
- Dark (default): Easy on eyes
- Light: High-contrast for bright rooms
- OLED: Pure black for OLED screens
- Colorblind: Red/green alternative colors
- Custom: User-defined

**Customization:**
- **Track colors:** Auto-assign by type, manual override
- **Clip colors:** By source, by key, by energy, custom
- **Waveform colors:** Gradient by frequency, solid, custom
- **Grid intensity:** Subtle → Prominent
- **Font size:** 10px → 20px (accessibility)

**Reaper-Level Customization:**
- Custom toolbar: Drag any action to toolbar
- Custom menus: Edit menu structure
- Custom layouts: Save window arrangements
- **But unlike Reaper:** Good defaults ship out-of-box

---

### 4. Wingman AI Integration (UI)

#### Wingman Panel (Persistent Right Sidebar Option)

```
┌────────────────────────────┐
│ 🤖 Wingman                 │
├────────────────────────────┤
│ "Create a trap beat"       │ ← User input
│                            │
│ Creating trap beat...      │
│ ✓ Set tempo to 95 BPM     │
│ ✓ Created drum track       │
│ ✓ Generated pattern        │
│ ✓ Added 808 bass           │
│ ✓ Added hi-hats            │
│                            │
│ [View Pattern] [Undo]      │
├────────────────────────────┤
│ Quick Actions:             │
│ [➕ Drums] [➕ Bass]        │
│ [🎚️ Auto-Mix] [🎹 Chords] │
│ [🎤 Tune Vocals]           │
├────────────────────────────┤
│ Chat History ▼             │
│ • "Create trap beat"       │
│ • "Make vocals brighter"   │
│ • "Add reverb to snare"    │
└────────────────────────────┘
```

**Wingman Modes:**
1. **Floating Input (Default):** Top bar only
2. **Sidebar Panel:** Full chat history + quick actions
3. **Fullscreen:** For complex requests (F11)
4. **Voice-Only:** Just speak, no typing

**Visual Feedback:**
- **Typing indicator:** "Wingman is thinking..."
- **Progress:** "Step 2 of 5: Generating drums..."
- **Success:** Green checkmark, auto-dismiss or stay
- **Error:** Red X, explanation, retry button

**Context Awareness:**
- Wingman sees: Current track selected, playhead position, tempo, key
- Example: "Add a snare here" → knows "here" = playhead position
- Example: "Make this louder" → knows "this" = selected clip

---

## Interaction Patterns

### Mouse Interactions

**Left Click:**
- Click clip: Select
- Click+drag clip: Move
- Click parameter: Adjust
- Click waveform: Place playhead

**Right Click:**
- Context menu (actions relevant to item)
- Example: Right-click clip → Duplicate, Delete, Normalize, Reverse, etc.

**Double Click:**
- Double-click clip → Open in editor
- Double-click plugin → Open UI
- Double-click empty space → Create clip

**Drag:**
- Drag from browser → Creates track/adds to track
- Drag clip → Move
- Alt+drag clip → Duplicate
- Drag automation point → Adjust value

**Scroll:**
- Scroll vertical: Scroll tracks
- Scroll horizontal: Scroll timeline
- Shift+scroll: Zoom horizontal
- Cmd+scroll: Zoom vertical

**Hover:**
- Hover sample in browser → Auto-preview
- Hover parameter → Shows tooltip with value
- Hover clip → Shows clip name + length

### Keyboard Shortcuts

**Transport:**
- `Space`: Play/Pause
- `Enter`: Stop
- `0`: Return to start
- `L`: Toggle loop
- `/`: Toggle metronome

**Editing:**
- `Cmd+C`: Copy
- `Cmd+V`: Paste
- `Cmd+D`: Duplicate
- `Cmd+Z`: Undo
- `Cmd+Shift+Z`: Redo
- `Delete`: Delete selection

**View:**
- `Tab`: Toggle Session ↔ Arrangement
- `B`: Toggle browser
- `M`: Toggle mixer
- `I`: Toggle inspector
- `E`: Toggle editor (piano roll/audio)
- `F`: Fit to window (zoom)
- `+/-`: Zoom in/out

**Track:**
- `Cmd+T`: New MIDI track
- `Cmd+Shift+T`: New audio track
- `Cmd+Alt+T`: New return track
- `M`: Mute selected track
- `S`: Solo selected track
- `R`: Arm for recording

**Wingman:**
- `Cmd+Shift+W`: Open Wingman
- `Cmd+/`: Voice input
- `Esc`: Close Wingman panel

**Power User:**
- `Cmd+K`: Command palette
- `Cmd+,`: Preferences
- `Cmd+Shift+P`: Quick actions
- `F11`: Fullscreen

### Touch/Tablet Support

**Gestures:**
- Pinch: Zoom
- Two-finger scroll: Pan
- Tap: Select
- Long-press: Context menu
- Swipe left/right: Switch tracks
- Swipe up/down: Adjust fader

**Touch Targets:**
- Minimum 44x44px (Apple HIG)
- Faders have larger touch area than visual
- Buttons have hover state on touch

---

## Implementation Priorities

### Phase 1: Core UI (Weeks 1-4)

**Must-Have:**
1. ✅ Top bar with transport
2. ✅ Arrangement view (timeline)
3. ✅ Track list (left of timeline)
4. ✅ Playhead and loop markers
5. ✅ Basic clip editing (trim, move, delete)
6. ✅ Right-click context menus
7. ✅ Zoom and scroll

**Deliverable:** Can import audio, arrange on timeline, play back

---

### Phase 2: Essential Editing (Weeks 5-8)

**Must-Have:**
1. ✅ Piano roll (bottom editor)
2. ✅ MIDI note editing (draw, select, delete)
3. ✅ Velocity editing
4. ✅ Quantize
5. ✅ Audio editor (comp lanes)
6. ✅ Clip gain and fades
7. ✅ Browser (basic file browser)

**Deliverable:** Can create MIDI, edit audio, find samples

---

### Phase 3: Mixer & Effects (Weeks 9-12)

**Must-Have:**
1. ✅ Mixer panel (right side)
2. ✅ Plugin hosting (VST3/AU)
3. ✅ Plugin slot routing
4. ✅ Send/return tracks
5. ✅ Volume/pan automation
6. ✅ Meters and levels
7. ✅ Mute/solo

**Deliverable:** Full mixing workflow

---

### Phase 4: Advanced Features (Weeks 13-16)

**Nice-to-Have:**
1. ✅ Session view (clip launcher)
2. ✅ Modulation system
3. ✅ Grid view (routing)
4. ✅ Command palette
5. ✅ Custom themes
6. ✅ Macro controls
7. ✅ Freeze/bounce

**Deliverable:** Power-user features complete

---

### Phase 5: Wingman Integration (Weeks 17-20)

**AI Features:**
1. ✅ Wingman UI panel
2. ✅ Natural language commands
3. ✅ AI music generation (Magenta)
4. ✅ Auto-mixing
5. ✅ Smart suggestions
6. ✅ Voice control
7. ✅ Context awareness

**Deliverable:** Full AI-assisted workflow

---

## Technology Stack for UI

### Framework: Electron + React

**Why:**
- ✅ Cross-platform (Win/Mac/Linux)
- ✅ Fast UI updates (React)
- ✅ Modern web technologies
- ✅ Wingman already uses this!

**Libraries:**
- **React**: UI components
- **TailwindCSS**: Styling (rapid development)
- **Framer Motion**: Smooth animations
- **React Flow**: Grid view (modular routing)
- **react-draggable**: Drag-and-drop
- **WaveSurfer.js**: Waveform rendering
- **Tone.js**: Audio preview in browser
- **WebSocket**: Communication with audio engine

### Audio Engine Integration

**IPC (Inter-Process Communication):**

```typescript
// Renderer (UI) → Main (Audio Engine)
ipcRenderer.send('play-transport', {});
ipcRenderer.send('create-track', { name: 'Drums', type: 'midi' });
ipcRenderer.send('set-tempo', { bpm: 120 });

// Main (Audio Engine) → Renderer (UI)
ipcMain.on('transport-update', (event, data) => {
  // Update playhead position
  setPlayheadPosition(data.position);
});

ipcMain.on('meters-update', (event, data) => {
  // Update track meters
  updateMeters(data.levels);
});
```

**Real-Time Updates:**
- 60 FPS playhead animation
- 30 Hz meter updates
- On-change parameter updates
- Throttled state sync (don't spam audio engine)

---

## Accessibility

### WCAG 2.1 AA Compliance

**Visual:**
- ✅ 4.5:1 contrast ratio (text)
- ✅ 3:1 contrast ratio (UI elements)
- ✅ Scalable text (10px → 20px)
- ✅ Colorblind-friendly palettes
- ✅ High-contrast mode

**Motor:**
- ✅ Keyboard-only operation
- ✅ Large touch targets (44x44px)
- ✅ No time-based interactions
- ✅ Sticky keys support

**Auditory:**
- ✅ Visual metronome (flashing)
- ✅ Waveform visualization
- ✅ Captions for Wingman voice

**Cognitive:**
- ✅ Clear visual hierarchy
- ✅ Consistent patterns
- ✅ Undo/redo everything
- ✅ Non-destructive editing
- ✅ Tooltips and help

**Screen Readers:**
- ✅ ARIA labels on all controls
- ✅ Semantic HTML
- ✅ Keyboard focus indicators
- ✅ Announced state changes

---

## Conclusion

### What Makes This UI "Perfect"

| Principle | Implementation | Inspired By |
|-----------|----------------|-------------|
| **Fast ideation** | Session View + Drag-and-drop browser | Ableton + Studio One |
| **Best editing** | FL piano roll + Pro Tools audio editor | FL Studio + Pro Tools |
| **Power without complexity** | Command palette + Smart defaults | Reaper (customization) + premium studio simplicity |
| **Beautiful & modern** | Matte-black theme, spacious, clean | Professional studio black + blue-accent language |
| **Modular routing** | Grid view, visual routing | Bitwig + Reason |
| **AI-first** | Wingman integrated everywhere | Unique innovation |
| **Cross-platform** | Electron + React | Industry standard |
| **Accessible** | WCAG AA, keyboard-only, screen reader | Inclusive design |

### Development Timeline

**Total: 20 weeks**

- Week 1-4: Core UI (transport, timeline, clips)
- Week 5-8: Editors (piano roll, audio, automation)
- Week 9-12: Mixer and plugin hosting
- Week 13-16: Advanced features (session view, modulation)
- Week 17-20: Wingman AI integration

**Then:** Beta testing, polish, release

---

## Next Steps

1. **Prototype wireframes** (Figma)
2. **Build core UI** (Electron + React)
3. **Connect to Wingman** (WebSocket bridge)
4. **Iterate with users**
5. **Ship 1.0**

---

**This UI combines the best of all DAWs while being AI-native from day one.**

**Let's build it! 🚀**
