---
description: Complete Arrangement and Session View Feature Implementation
---

# Arrangement & Session View Complete Overhaul

This workflow implements ALL missing features identified in the DAW comparison analysis.

## Phase 1: Critical Visual Feedback (Day 1)
// turbo-all

### 1.1 Playhead Line in ArrangerComponent
- [ ] Add `playheadPositionBeats_` member variable
- [ ] Add timer to poll Engine playhead position
- [ ] Draw vertical playhead line in `drawSkia()` and `paint()`
- [ ] Auto-scroll to follow playhead when playing

### 1.2 Loop Region Visualization
- [ ] Add `loopStartBeats_` and `loopEndBeats_` members
- [ ] Draw loop region highlight in timeline ruler
- [ ] Draw loop brace/markers
- [ ] Allow drag to set loop region

### 1.3 Waveform Display in Audio Clips
- [ ] Create `WaveformCache` class for thumbnail generation
- [ ] Generate waveform on clip creation/audio file load
- [ ] Draw waveform inside ClipComponent
- [ ] Cache waveform data for performance

### 1.4 MIDI Note Preview in Clips
- [ ] Calculate note rectangles from clip's MIDI data
- [ ] Draw mini piano roll inside MIDI clips
- [ ] Color by velocity

## Phase 2: Core Editing Tools (Day 2)

### 2.1 Grid Resolution Selector
- [ ] Add `GridResolution` enum (1, 1/2, 1/4, 1/8, 1/16, 1/32, triplets)
- [ ] Add toolbar dropdown component
- [ ] Update `snapToGrid()` to use selected resolution
- [ ] Draw grid lines at current resolution

### 2.2 Clip Split Tool
- [ ] Add `SplitTool` mode to ArrangerComponent
- [ ] Implement `splitClipAtPosition(clipId, beats)` in ProjectState
- [ ] Draw split cursor when hovering
- [ ] Handle click to split

### 2.3 Clip Fade Handles
- [ ] Draw fade handle triangles on clip edges
- [ ] Detect hover on fade handles
- [ ] Drag to adjust fade in/out length
- [ ] Update `PROP_FADE_IN` and `PROP_FADE_OUT` in ProjectState
- [ ] Visual fade curve in waveform

### 2.4 Crossfade Between Clips
- [ ] Detect overlapping clips on same track
- [ ] Create crossfade region when clips overlap
- [ ] Draw crossfade visual (X pattern or equal power curve)
- [ ] Store crossfade length in ProjectState

## Phase 3: Track Organization (Day 3)

### 3.1 Track Height Resize
- [ ] Add resize handle at bottom of track headers
- [ ] Store per-track height in ProjectState `PROP_HEIGHT`
- [ ] Update `trackIndexToY()` to use variable heights
- [ ] Minimum height enforcement

### 3.2 Track Folders / Grouping
- [ ] Add `PROP_PARENT_TRACK` to ProjectState
- [ ] Add `PROP_COLLAPSED` boolean
- [ ] Draw folder tracks with expand/collapse arrow
- [ ] Indent child tracks
- [ ] Summing of child track audio in engine

### 3.3 Track Type Differentiation
- [ ] Different icons for Audio vs MIDI tracks
- [ ] Color-coded track strip
- [ ] Show input/output routing indicator

### 3.4 Track Freeze
- [ ] Add `freezeTrack(trackId)` to Engine
- [ ] Render track to temp audio file
- [ ] Replace clips with frozen audio
- [ ] Show frozen indicator
- [ ] Unfreeze to restore

## Phase 4: Take Lanes & Comping (Day 4)

### 4.1 Take Lane Infrastructure
- [ ] Add `ID_TAKES` child to clips in ProjectState
- [ ] Add `PROP_ACTIVE_TAKE` property
- [ ] Track can have multiple takes per region

### 4.2 Take Lane UI
- [ ] Expand track to show take lanes
- [ ] Draw each take as separate lane
- [ ] Click to switch active take
- [ ] Comp tool to select regions from different takes

### 4.3 Cycle Recording to Takes
- [ ] When loop recording, create new take each pass
- [ ] Stack takes in take lanes
- [ ] Auto-switch to new take preview

## Phase 5: Timeline & Markers (Day 5)

### 5.1 Integrate MarkerLane into Arranger
- [ ] Add MarkerLaneComponent as child of ArrangerComponent
- [ ] Sync scroll position with arrangement
- [ ] Double-click ruler to add marker
- [ ] Navigate to marker on click

### 5.2 Integrate TempoLane into Arranger
- [ ] Add collapsible tempo lane above tracks
- [ ] Sync with TempoMap
- [ ] Visual tempo curve

### 5.3 Time Signature Changes
- [ ] Add `ID_TIME_SIGNATURE_EVENTS` to ProjectState
- [ ] Draw time sig changes in ruler
- [ ] Update grid line drawing for time sig changes

### 5.4 Time Selection Range
- [ ] Click and drag in ruler to select time range
- [ ] Delete to ripple delete
- [ ] Insert silence command
- [ ] Copy/paste time range

## Phase 6: Session View Complete Overhaul (Day 6-7)

### 6.1 Scene Infrastructure
- [ ] Add `ID_SCENES` to ProjectState
- [ ] Add `PROP_SCENE_INDEX` to clips
- [ ] Scene = row of slots across tracks

### 6.2 Clip Launching Engine
- [ ] Add `launchClip(trackId, sceneIndex)` to Engine
- [ ] Add `stopClip(trackId)` to Engine
- [ ] Add `launchScene(sceneIndex)` to Engine
- [ ] Quantized launch (wait for next beat/bar)

### 6.3 Session View Interaction
- [ ] Click slot to launch/stop clip
- [ ] Click scene trigger to launch row
- [ ] Visual playing/queued/stopped states
- [ ] Stop all clips button

### 6.4 Record into Slots
- [ ] Arm track + click empty slot to record
- [ ] Recording creates clip in that slot
- [ ] Loop recording fills slot with loop

### 6.5 Follow Actions
- [ ] Add `PROP_FOLLOW_ACTION` to clips
- [ ] Options: Stop, Next, Previous, Random, First, Last
- [ ] After clip plays, trigger follow action

### 6.6 Session View Visual Polish
- [ ] Clip colors from ProjectState
- [ ] Clip length indicator
- [ ] Record/Stop button in empty slots
- [ ] Scene names

## Phase 7: Automation Integration (Day 8)

### 7.1 Automation Lanes Under Tracks
- [ ] Toggle to show automation lanes per track
- [ ] List of automatable parameters
- [ ] Draw automation directly in arrangement

### 7.2 Automation Recording
- [ ] Touch/Latch/Write modes
- [ ] Record parameter changes during playback
- [ ] Show automation being written

## Phase 8: Polish & Integration (Day 9-10)

### 8.1 Punch In/Out Regions
- [ ] Visual punch locators
- [ ] Only record within punch region
- [ ] Pre-roll/post-roll settings

### 8.2 MIDI Learn for Session Slots
- [ ] Right-click slot -> MIDI Learn
- [ ] Map controller pads to slots
- [ ] Store mappings in project

### 8.3 Global Quantize Dropdown
- [ ] Toolbar control for launch quantization
- [ ] Affects all clip launches

### 8.4 Performance Optimization
- [ ] Waveform caching
- [ ] Clip render caching
- [ ] Lazy loading of clip content

---

## Execution Order

Start with Phase 1 (most visible impact), then proceed sequentially.

Each task should be verified to build and function before moving to the next.
