# MPE 100% COMPLETION - MASTER IMPLEMENTATION PLAN

**Status:** In Progress (17 tasks)
**Estimated Time:** 2-3 weeks
**Current Completion:** 30% → Target: 100%

---

## EXECUTIVE SUMMARY

This document outlines all remaining work to achieve 100% MPE completion in Zenith DAW.

**Current State:**
- ✅ MPE Engine: 100% (capture, modulation, RT-safe)
- ✅ Rendering: 90% (color-coded lanes, accessibility)
- ❌ Editing: 0%
- ❌ Configuration: 0%
- ❌ Recording: 0%
- ❌ Real-time feedback: 0%

**Target State:** All features at 100% with production quality.

---

## PHASE 1: EXPRESSION LANE EDITING (CRITICAL - Week 1)

**Status:** 🔴 IN PROGRESS
**Priority:** BLOCKING SHIP
**Estimated:** 2-3 days

### Tasks

#### 1.1 Click to Add Points ✅ STARTED
- [x] Create `ExpressionLaneEditor.h` header
- [ ] Implement `ExpressionLaneEditor.cpp`
  - [ ] mouseDown() handler
  - [ ] Add point to selected note
  - [ ] Clamp to lane bounds
  - [ ] Trigger undo action
- [ ] Integrate with PianoRollComponent
  - [ ] Route mouse events to editor
  - [ ] Update setNoteExpression()
  - [ ] Repaint lane

**Acceptance Criteria:**
- Click in empty lane space → Creates new point
- Point added at mouse position
- Value snapped to 0.0-1.0 range
- Undo/redo supported
- Only works when notes are selected

---

#### 1.2 Drag to Edit Points
- [ ] Implement mouseDrag() handler
  - [ ] Find hovered point
  - [ ] Update value based on Y position
  - [ ] Update time based on X position (if enabled)
  - [ ] Clamp to lane bounds
- [ ] Visual feedback during drag
  - [ ] Highlight dragged point
  - [ ] Show value tooltip
  - [ ] Update curve in real-time
- [ ] Snap to grid (optional)

**Acceptance Criteria:**
- Drag point up → Value increases
- Drag point down → Value decreases
- Drag point left/right → Adjusts timing
- Smooth 60fps updates
- Visual feedback throughout

---

#### 1.3 Delete Points
- [ ] Implement deleteSelectedPoint()
  - [ ] Delete hovered/selected point
  - [ ] Trigger undo action
  - [ ] Remove from expression data
- [ ] Keyboard shortcut: Backspace/Delete
  - [ ] Route keyPressed() to editor
  - [ ] Check if hovering point
  - [ ] Delete with confirmation

**Acceptance Criteria:**
- Backspace → Deletes hovered point
- Delete → Deletes hovered point
- Undo restores point
- No crashes if no point selected

---

#### 1.4 Multi-Select Points
- [ ] Shift+click to add to selection
- [ ] Ctrl+click to toggle selection
- [ ] Marquee select (drag in lane)
- [ ] Select all in note (Cmd+A in lane)
- [ ] Visual feedback for selection

**Acceptance Criteria:**
- Multiple points can be selected
- Dragging one point moves all selected
- Delete removes all selected
- Selection highlights clearly visible

---

#### 1.5 Bezier Curve Tension
- [ ] Right-click point → Context menu
- [ ] Menu options: "Linear", "Curved", "Custom"
- [ ] Drag bezier handle (when in custom mode)
- [ ] Visualize curve with tension applied
- [ ] Tension range: -1.0 to +1.0

**Acceptance Criteria:**
- Right-click point → Shows tension menu
- Select tension → Curve updates immediately
- Custom mode → Show bezier handle
- Drag handle → Curve updates in real-time

---

### Code Structure

**New Files:**
```
apps/desktop/Source/ui/piano-roll/
├── ExpressionLaneEditor.h          (165 lines) ✅ CREATED
├── ExpressionLaneEditor.cpp        (est. 400 lines)
└── ExpressionLaneEditorTests.cpp   (est. 200 lines)
```

**Modified Files:**
```
apps/desktop/Source/ui/piano-roll/
├── PianoRollComponent.h            (+1 member: ExpressionLaneEditor)
├── PianoRollComponent.cpp          (+50 lines: event routing)
└── PianoRollComponent.h            (+1 method: getExpressionLaneEditor())
```

---

## PHASE 2: MPE ZONE CONFIGURATION UI (CRITICAL - Week 1)

**Status:** 🔴 NOT STARTED
**Priority:** BLOCKING SHIP
**Estimated:** 1 day

### Tasks

#### 2.1 Create MPE Settings Panel
- [ ] Design UI layout
  ```
  ┌─────────────────────────────────┐
  │ MPE Configuration               │
  ├─────────────────────────────────┤
  │ Lower Zone                      │
  │ ☑ Enable                        │
  │ Master Channel: [1 ▼]           │
  │ Member Channels: [15 ▼]         │
  │                                 │
  │ Upper Zone                      │
  │ ☐ Enable                        │
  │ Master Channel: [16 ▼]          │
  │ Member Channels: [0 ▼]          │
  │                                 │
  │ Presets: [Roli Seaboard ▼]      │
  │         [LinnStrument ▼]         │
  │         [Custom]                 │
  │                                 │
  │ [Apply] [Reset] [Close]         │
  └─────────────────────────────────┘
  ```

- [ ] Implement `MPEConfigurationPanel.h/cpp`
- [ ] Add to Preferences/Settings window
- [ ] Keyboard shortcut: Cmd+, (MPE tab)

---

#### 2.2 Zone Controls
- [ ] Lower zone enable/disable checkbox
- [ ] Upper zone enable/disable checkbox
- [ ] Master channel dropdown (1-16)
- [ ] Member channel count (0-15)
- [ ] Validation:
  - Only one zone per master channel
  - Total member channels ≤ 15
  - Lower zone uses channels 1-N
  - Upper zone uses channels 16-N

---

#### 2.3 Controller Presets
- [ ] **Roli Seaboard Block**
  - Lower zone: 15 member channels, master = 1
  - Upper zone: Disabled

- [ ] **Roli Seaboard Rise**
  - Lower zone: 15 member channels, master = 1
  - Upper zone: 15 member channels, master = 16

- [ ] **LinnStrument**
  - Lower zone: 8 member channels, master = 1
  - Upper zone: 8 member channels, master = 16

- [ ] **K-Board**
  - Lower zone: 4 member channels, master = 1
  - Upper zone: Disabled

- [ ] **Custom** (user-specified)

---

#### 2.4 Integration with ZenithPolySynth
- [ ] Route settings to `ZenithPolySynthProcessor`
- [ ] Update `juce::MPEZoneLayout` on apply
- [ ] Save settings to project state
- [ ] Load settings on project load
- [ ] Reset to defaults

**Acceptance Criteria:**
- Settings panel opens
- Changing zones updates synth immediately
- Presets configure correct zones
- Settings persist across sessions
- Reset restores defaults

---

### Code Structure

**New Files:**
```
apps/desktop/Source/ui/settings/
├── MPEConfigurationPanel.h       (est. 150 lines)
├── MPEConfigurationPanel.cpp     (est. 300 lines)
└── MPEConfigurationPanelTests.cpp (est. 100 lines)
```

**Modified Files:**
```
apps/desktop/Source/
├── ui/common/SettingsPanel.h     (+1 tab: MPE)
├── ui/common/SettingsPanel.cpp   (+20 lines: add MPE panel)
├── instruments/ZenithPolySynth.h  (+1 method: setMPEZoneLayout)
└── instruments/ZenithPolySynth.cpp (+20 lines: apply zones)
```

---

## PHASE 3: REAL-TIME VISUAL FEEDBACK (HIGH - Week 2)

**Status:** 🔴 NOT STARTED
**Priority:** HIGH
**Estimated:** 1-2 days

### Tasks

#### 3.1 Glowing Notes During Performance
- [ ] Create `NoteOverlay` component
- [ ] Track active MPE notes per channel
- [ ] Draw glow around notes with active MPE
- [ ] Glow intensity based on pressure
- [ ] Glow color based on timbre
- [ ] 60fps updates (request repaint)

**Visual Design:**
```
Note with pressure=100 (max):
┌──────────────┐
│ ▓▓▓▓▓▓▓▓▓▓▓ │ ← Red glow (10px blur)
│ ▓▓NOTE ▓▓▓ │
│ ▓▓▓▓▓▓▓▓▓▓▓ │
└──────────────┘

Note with pressure=0:
┌──────────────┐
│    NOTE      │ ← No glow
└──────────────┘
```

---

#### 3.2 Pressure Circle Indicators
- [ ] Draw circle on each note
- [ ] Circle size = pressure amount (0-127)
  - 0 → No circle
  - 64 → Medium circle
  - 127 → Large circle (touches note edges)
- [ ] Circle color matches lane color (red)
- [ ] Semi-transparent (50% alpha)
- [ ] Updates in real-time (60fps)

---

#### 3.3 Timbre Color Shifts
- [ ] Note color shifts based on timbre (CC74)
- [ ] Default: White/grey
- [ ] Timbre 0 → Blue tint
- [ ] Timbre 64 → Purple tint
- [ ] Timbre 127 → Red tint
- [ ] Smooth interpolation
- [ ] Only visible when timbre lane visible

---

#### 3.4 Pitchbend Visualization
- [ ] Show pitchbend as note position offset
- [ ] Note shifts up/down based on pitchbend
- [ ] Small offset (visual only, doesn't affect timing)
- [ -48 semitones → Note shifted up 12px
- - +48 semitones → Note shifted down 12px
- [ ] Smooth interpolation

**Acceptance Criteria:**
- Notes glow when MPE active
- Circle size shows pressure
- Color shows timbre
- Position shows pitchbend
- 60fps smooth updates
- No performance degradation

---

### Code Structure

**New Files:**
```
apps/desktop/Source/ui/piano-roll/
├── NoteOverlay.h                  (est. 120 lines)
├── NoteOverlay.cpp                (est. 250 lines)
└── NoteOverlayTests.cpp           (est. 150 lines)
```

**Modified Files:**
```
apps/desktop/Source/ui/piano-roll/
├── PianoRollComponent.h           (+1 member: NoteOverlay)
├── PianoRollComponent.cpp         (+30 lines: update overlay)
└── PianoRollComponent.h           (+1 method: getMPEStateForNote)
```

---

## PHASE 4: MPE RECORDING (HIGH - Week 2)

**Status:** 🔴 NOT STARTED
**Priority:** HIGH
**Estimated:** 1-2 days

### Tasks

#### 4.1 Arm Record Toggle Per Lane
- [ ] Add record arm button to each lane header
- [ ] Button shows armed state (red circle when armed)
- [ ] Click to toggle arm
- [ ] Only armed lanes record MPE
- [ ] Visual indicator: "● REC" in lane label

**UI Design:**
```
┌─────────────────────────────────┐
│ PRESSURE ⬇ ● REC               │ ← Armed (red)
├─────────────────────────────────┤
│ PITCH ♫                         │ ← Not armed
├─────────────────────────────────┤
│ SLIDE (MPE) ↔                   │ ← Not armed
└─────────────────────────────────┘
```

---

#### 4.2 MPE Message Capture
- [ ] Hook into MIDI message processing
- [ ] Filter MPE messages by channel
- [ ] Match to currently playing notes
- [ ] Convert to ExpressionPoint
- [ ] Add to note's expression data
- [ ) Quantize to grid (if enabled)
- [ ] Auto-create points on recording

**Algorithm:**
```
For each MIDI buffer:
  For each MPE message:
    If lane armed:
      Find note matching channel + pitch
      If note found:
        Convert to value (0.0-1.0)
        Create ExpressionPoint at current time
        Add to note's expression data
        Repaint lane
```

---

#### 4.3 Recording UX
- [ ] Transport record button arms all lanes
- [ ] Individual lane arm for selective recording
- [ ] Record enable/disable while recording
- [ ] Overdub mode (add to existing)
- [ ] Replace mode (clear existing, then record)
- [ ] Count-in support (record after N beats)

**Acceptance Criteria:**
- Arm lane → Recording enabled for that expression type
- Start transport → MPE messages captured
- Stop transport → Points saved, lanes repaint
- Undo removes recorded points
- Recording doesn't affect playback

---

### Code Structure

**New Files:**
```
apps/desktop/Source/ui/piano-roll/
├── MPERecorder.h                  (est. 100 lines)
├── MPERecorder.cpp                (est. 250 lines)
└── MPERecorderTests.cpp           (est. 150 lines)
```

**Modified Files:**
```
apps/desktop/Source/ui/piano-roll/
├── PianoRollComponent.h           (+1 member: MPERecorder)
├── PianoRollComponent.cpp         (+40 lines: route MIDI to recorder)
└── PianoRollComponent.h           (+1 method: isLaneArmedForRecord())
```

---

## PHASE 5: TESTING (MEDIUM - Week 2-3)

**Status:** 🔴 NOT STARTED
**Priority:** MEDIUM
**Estimated:** 2-3 days

### Tasks

#### 5.1 UI Automation Tests
- [ ] Test clicking adds points
- [ ] Test dragging edits points
- [ ] Test deleting points
- [ ] Test multi-select
- [ ] Test bezier tension
- [ ] Test record arm toggle
- [ ] Test MPE recording workflow

**Framework:** JUCE UnitTest with synthetic mouse events

---

#### 5.2 Integration Tests with Real Controllers
- [ ] Test with Roli Seaboard (if available)
- [ ] Test with LinnStrument (if available)
- [ ] Test with standard MIDI (verify no crashes)
- [ ] Test with multiple MPE controllers
- [ ] Test zone configuration with actual hardware

**Framework:** Manual testing with hardware + automated MIDI file playback

---

#### 5.3 Helper Function Unit Tests
- [ ] Test getExpressionColor() for all types
- [ ] Test getExpressionLabel() for all types
- [ ] Test getExpressionSymbol() for all types
- [ ] Test configureExpressionCurvePaint()
- [ ] Test configureExpressionPointPaint()
- [ ] Test configureExpressionLabelPaint()
- [ ] Test screenYToValue() conversion
- [ ] Test valueToScreenY() conversion

**Framework:** JUCE UnitTest

---

#### 5.4 Accessibility Tests
- [ ] Verify WCAG 2.1 AA contrast ratios
- [ ] Test with screen reader (NVDA/VoiceOver)
- [ ] Test keyboard navigation
- [ ] Test colorblind simulation
- [ ] Test high contrast mode

---

## PHASE 6: DOCUMENTATION (MEDIUM - Week 3)

**Status:** 🔴 NOT STARTED
**Priority:** MEDIUM
**Estimated:** 0.5-1 day

### Tasks

#### 6.1 MPE User Guide
**File:** `docs/user/MPE_USER_GUIDE.md`

**Sections:**
1. What is MPE?
2. Supported Controllers
3. Quick Start (5-minute setup)
4. Configuration Guide
5. Expression Lane Editing
6. Recording MPE
7. Tips & Tricks
8. Troubleshooting
9. FAQ

---

#### 6.2 Controller Setup Tutorials
**Files:**
- `docs/user/MPE_ROLI_SEABOARD_SETUP.md`
- `docs/user/MPE_LINNSTRUMENT_SETUP.md`
- `docs/user/MPE_KBOARD_SETUP.md`
- `docs/user/MPE_GENERAL_MIDI_SETUP.md`

**Each Tutorial Contains:**
1. Controller photo/diagram
2. Connection steps (USB, Bluetooth)
3. Zone configuration steps
4. Test procedure
5. Known issues
6. Support links

---

#### 6.3 Keyboard Shortcut Reference Card
**File:** `docs/user/MPE_SHORTCUTS.md`

**Shortcuts:**
- `Cmd+E` - Toggle all expression lanes
- `Cmd+Shift+E` - Toggle specific lane
- `Backspace` - Delete selected point
- `Right-click` - Context menu (tension)
- `Shift+click` - Multi-select
- `Cmd+A` - Select all points in note

---

#### 6.4 API Documentation
- [ ] Document PianoRollComponent MPE methods
- [ ] Document ExpressionLaneEditor API
- [ ] Document MPEConfigurationPanel API
- [ ] Document MPERecorder API
- [ ] Add code examples

---

## TIMELINE

| Week | Tasks | Deliverable |
|------|-------|-------------|
| **Week 1** | Phase 1 + Phase 2 | Expression editing + Zone config |
| **Week 2** | Phase 3 + Phase 4 | Real-time feedback + Recording |
| **Week 3** | Phase 5 + Phase 6 | Testing + Documentation |
| **End** | **100% Complete** | **Ship MPE v1.0** |

---

## SUCCESS CRITERIA

MPE is **100% complete** when:

1. ✅ **Engine** (already done)
   - MPE capture, modulation, RT-safe

2. ✅ **Editing** (Phase 1)
   - Click to add points
   - Drag to edit
   - Delete points
   - Multi-select
   - Bezier curves

3. ✅ **Configuration** (Phase 2)
   - Zone settings panel
   - Controller presets
   - Save/load settings

4. ✅ **Real-time** (Phase 3)
   - Glowing notes
   - Pressure indicators
   - Timbre color shifts
   - Pitchbend visualization

5. ✅ **Recording** (Phase 4)
   - Arm per lane
   - Capture MPE
   - Auto-create points

6. ✅ **Testing** (Phase 5)
   - UI automation tests
   - Integration tests
   - Accessibility tests

7. ✅ **Documentation** (Phase 6)
   - User guide
   - Controller tutorials
   - Shortcuts reference

---

## NEXT STEPS

1. ✅ **STARTED:** Phase 1.1 - ExpressionLaneEditor.h created
2. **NEXT:** Implement ExpressionLaneEditor.cpp
3. **THEN:** Route mouse events from PianoRollComponent
4. **THEN:** Test click-to-add-points
5. **THEN:** Iterate through all phases

---

**Let's build this! 🚀**
