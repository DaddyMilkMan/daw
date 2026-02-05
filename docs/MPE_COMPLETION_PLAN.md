# MPE (MIDI Polyphonic Expression) Implementation Plan

**Current Status: 90% Complete**

This document outlines what's needed to make MPE 100% complete in Zenith DAW.

---

## What's Already Implemented ✅

### Engine Level (100% Complete)
- **ZenithPolySynthVoice** captures all MPE dimensions:
  - `notePressureChanged()` → `aftertouch_`
  - `noteTimbreChanged()` → `timbre_`
  - `notePitchbendChanged()` → `pitchBend_`
- MPE values available in modulation matrix
- RT-safe implementation with unit tests
- MPE zone layout configured in `ZenithPolySynth.cpp:184-186`

### Piano Roll Data Structures (50% Complete)
- `PianoRollComponent.h:395-423` defines expression lane API:
  - `enum class ExpressionType { PitchBend, Pressure, Slide, Expression }`
  - `struct ExpressionPoint { timeOffset, value, tension }`
  - `setExpressionLaneVisible()`, `setNoteExpression()`
- Storage allocated for expression data
- **Missing:** Actual rendering and editing

---

## What's Missing for 100% Completion ❌

### 1. Expression Lane Rendering (HIGH Priority - 2-3 days)

**Current State:** Piano roll has expression lane storage but doesn't draw them

**What's Needed:**
```cpp
// In PianoRollComponent::paint() or paintGraph()
void drawExpressionLane(juce::Graphics& g, ExpressionType type, int laneY) {
    // Draw lane background
    // Draw note expression curves
    // Draw automation points with bezier handles
}
```

**Files to Modify:**
- `apps/desktop/Source/ui/piano-roll/PianoRollComponent.cpp:paint()`
- Add new method: `drawExpressionLane()`

**Acceptance Criteria:**
- Pressure lane visible below notes
- Timbre (CC74) lane visible
- Pitchbend lane visible
- Smooth curve rendering with bezier tension
- Color coding: Pressure (red), Timbre (blue), Pitchbend (green)

---

### 2. Expression Lane Editing (HIGH Priority - 2-3 days)

**Current State:** `setNoteExpression()` exists but no UI to edit

**What's Needed:**
- Click on expression lane to add points
- Drag points to edit values
- Select points and adjust bezier tension
- Delete points
- Mouse wheel to add/remove lanes

**Files to Modify:**
- `PianoRollComponent::mouseDown()`
- `PianoRollComponent::mouseDrag()`
- New class: `ExpressionLaneEditor`

**Acceptance Criteria:**
- Draw curves by clicking in lane
- Edit existing points by dragging
- Multi-select with Shift+click
- Copy/paste expression between notes

---

### 3. MPE Zone Configuration UI (HIGH Priority - 1 day)

**Current State:** MPE hardcoded to `juce::MPEZoneLayout()` (all channels)

**What's Needed:**
```cpp
struct MPESettings {
    bool lowerZoneEnabled = false;
    int lowerMasterChannel = 1;
    int lowerMemberChannels = 0;  // 0-15

    bool upperZoneEnabled = false;
    int upperMasterChannel = 16;
    int upperMemberChannels = 0;  // 0-15
};
```

**UI Location:**
- Preferences → MIDI → MPE Configuration
- Or: Track context menu → "Configure MPE Zone"

**Files to Create:**
- `MPEConfigurationComponent.h/cpp`

**Acceptance Criteria:**
- Enable/disable lower zone with channel count
- Enable/disable upper zone with channel count
- Test with Roli Seaboard, LinnStrument
- Presets for common MPE controllers

---

### 4. MPE Visual Feedback (MEDIUM Priority - 1-2 days)

**Current State:** No visual indication of MPE during performance

**What's Needed:**
- Small dots/circles on notes showing:
  - Circle size = pressure amount
  - Circle color = timbre value
- Real-time updates as MPE messages arrive

**Files to Modify:**
- `PianoRollComponent::paint()`
- Add `NoteOverlay` component for real-time feedback

**Acceptance Criteria:**
- Notes show pressure as glowing halo
- Timbre shown as color shift (warm → cool)
- Updates at 60fps during playback
- Keyboard shortcut: Toggle MPE feedback (Cmd+Shift+E)

---

### 5. MPE Recording (MEDIUM Priority - 1-2 days)

**Current State:** Can't record MPE automation

**What's Needed:**
- When recording enabled, capture MPE messages as `ExpressionPoint`s
- Quantize to beat grid if enabled
- Link to note start time

**Files to Modify:**
- `RecordingEngine` (if exists)
- `PianoRollComponent` (add `recordMPEMessage()`)

**Acceptance Criteria:**
- Arm recording toggle per expression lane
- Capture MPE during record pass
- Show recorded curves in lanes
- Undo/redo support

---

### 6. MPE Preset Integration (LOW Priority - 0.5 day)

**Current State:** MPE zones not saved with presets

**What's Needed:**
- Save MPE zone layout in preset XML
- Restore zone layout on preset load
- Mark as "MPE-compatible" in preset browser

**Files to Modify:**
- Preset save/load code in `ZenithPolySynthProcessor`

**Acceptance Criteria:**
- MPE settings persist across sessions
- Presets auto-configure zones for MPE controllers

---

### 7. MPE User Documentation (LOW Priority - 0.5 day)

**Current State:** No user-facing MPE docs

**What's Needed:**
Create `docs/user/MPE_USER_GUIDE.md`:

```markdown
# Using MPE with Zenith DAW

## What is MPE?
- Per-note expression vs channel-wide expression
- Compatible controllers: Roli Seaboard, LinnStrument, etc.

## Setup
1. Connect MPE controller
2. Configure MPE zones in Preferences → MIDI → MPE
3. Enable expression lanes in piano roll (Cmd+E)

## Recording MPE
1. Arm recording
2. Toggle expression lane recording
3. Perform with MPE controller
4. Edit curves in piano roll

## MPE Modulation Matrix
- Pressure → Filter Cutoff
- Timbre → Oscillator Mix
- Pitchbend → Vibrato LFO
```

---

## Implementation Priority Order

### Phase 1: Core Visualization (1 week)
1. **Expression Lane Rendering** (see notes currently)
2. **Expression Lane Editing** (draw curves)

### Phase 2: Configuration (1 week)
3. **MPE Zone Config UI** (setup for different controllers)
4. **MPE Visual Feedback** (performance visualization)

### Phase 3: Advanced Features (1 week)
5. **MPE Recording** (capture performances)
6. **MPE Preset Integration** (save with patches)
7. **MPE User Documentation** (help users)

---

## Quick Win: One-Hour Incremental Improvement

Add keyboard shortcut to toggle expression lanes:

```cpp
// In PianoRollComponent::keyPressed()
bool keyPressed(const KeyPress& key) override {
    if (key == KeyPress('e', ModifierKeys::commandModifier, 0)) {
        // Toggle all expression lanes
        bool newState = !getExpressionLaneVisible(ExpressionType::Pressure);
        setExpressionLaneVisible(ExpressionType::Pressure, newState);
        setExpressionLaneVisible(ExpressionType::PitchBend, newState);
        setExpressionLaneVisible(ExpressionType::Slide, newState);
        repaint();
        return true;
    }
    return Component::keyPressed(key);
}
```

This gives immediate user value while full rendering is implemented.

---

## Testing Checklist

When implementing each feature, verify:

- [ ] Works with Roli Seaboard Block
- [ ] Works with LinnStrument
- [ ] Works with regular MIDI (non-MPE) - no crashes
- [ ] Expression lanes don't slow down piano roll rendering
- [ ] Recorded MPE plays back correctly
- [ ] Presets save/load MPE settings
- [ ] Undo/redo works for expression edits

---

## Estimated Timeline

| Phase | Tasks | Duration | Dependencies |
|-------|-------|----------|--------------|
| **Phase 1** | Lane rendering + editing | 1 week | None |
| **Phase 2** | Config UI + visual feedback | 1 week | Phase 1 |
| **Phase 3** | Recording + presets + docs | 1 week | Phase 1, 2 |
| **Total** | **All features** | **3 weeks** | - |

---

## Success Criteria

MPE is **100% complete** when:

1. ✅ User can configure MPE zones in UI (not hardcoded)
2. ✅ Piano roll shows expression lanes for pressure/timbre/pitchbend
3. ✅ User can draw/edit MPE automation curves
4. ✅ Real-time visual feedback during MPE performance
5. ✅ MPE recording captures per-note expression
6. ✅ MPE settings save with presets
7. ✅ Documentation helps users set up MPE controllers

**Current Status: 0/7 complete (engine works, UX missing)**
