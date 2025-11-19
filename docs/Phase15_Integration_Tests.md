# Phase 15: Integration Test Plan

**Purpose:** Verify tempo map and markers work correctly across engine, ProjectState, UI, and CommandAPI.

---

## A. Engine / TempoMap-Level Tests

### Test 1: Single Tempo Sanity
**Setup:**
- Tempo map: one point at beat 0, 120 BPM

**Test Cases:**
```cpp
assert(tempoMap.beatsToSeconds(0, 44100) == 0.0);
assert(tempoMap.beatsToSeconds(4, 44100) ≈ 2.0);  // 4 beats @ 120 BPM = 2 sec
assert(tempoMap.secondsToBeats(2.0, 44100) ≈ 4.0);
assert(tempoMap.beatsToSamples(4, 44100) ≈ 88200);
assert(tempoMap.samplesToBeats(88200, 44100) ≈ 4.0);
```

**Repeat for:**
- 60 BPM (slow)
- 150 BPM (fast)
- 90 BPM, 180 BPM (power of 2 multiples)

---

### Test 2: Step Tempo Change
**Setup:**
- Tempo points: (0 beats, 120 BPM), (8 beats, 150 BPM)

**Test Cases:**
```cpp
// Before tempo change (in 120 BPM segment)
assert(tempoMap.beatsToSeconds(4, 44100) ≈ 2.0);

// At tempo change boundary
assert(tempoMap.beatsToSeconds(8, 44100) ≈ 4.0);

// After tempo change (in 150 BPM segment)
// 8 beats @ 120 = 4 sec, then 4 beats @ 150 = 1.6 sec = 5.6 sec total
assert(tempoMap.beatsToSeconds(12, 44100) ≈ 5.6);
```

**Round-trip test:**
```cpp
for (double beats : {0.0, 2.5, 7.9, 8.0, 8.1, 16.0, 32.0}) {
    double seconds = tempoMap.beatsToSeconds(beats, 44100);
    double roundTrip = tempoMap.secondsToBeats(seconds, 44100);
    assert(abs(roundTrip - beats) < 0.0001);  // Epsilon tolerance
}
```

---

### Test 3: Dense Grid Stability
**Setup:**
- Add tempo points every 4 bars up to 64 bars
- Vary BPM randomly between 80-160

**Test Cases:**
```cpp
// Create dense map
for (int bar = 0; bar < 16; bar++) {
    double beats = bar * 16.0;  // 4/4 time
    double bpm = 80.0 + (rand() % 80);
    projectState.addTempoPoint(beats, bpm, "Test");
}

// Snapshot the map
auto snapshot = engine.getTempoMap().getCurrentSnapshot();

// Query 1000 random positions
for (int i = 0; i < 1000; i++) {
    double beats = (rand() % 256) / 4.0;  // 0-64 beats in 0.25 increments

    double seconds = tempoMap.beatsToSeconds(beats, 44100);

    // Verify monotonicity (time never goes backwards)
    if (i > 0) {
        assert(seconds >= lastSeconds);
    }
    lastSeconds = seconds;
}
```

**Performance check:**
- 1000 queries should take < 1ms total (no pathological spikes)

---

### Test 4: Snapshot Atomicity (RT-Safety Critical)
**Setup:**
- Simulate message thread + audio thread concurrency

**Test Cases:**
```cpp
// Message thread: mutate tempo map in loop
std::thread messageThread([&]() {
    for (int i = 0; i < 100; i++) {
        projectState.addTempoPoint(i * 4.0, 100 + (i % 60), "Test");
        std::this_thread::sleep_for(1ms);
    }
});

// Simulated audio thread: read snapshot in tight loop
std::atomic<bool> failed{false};
std::thread audioThread([&]() {
    for (int i = 0; i < 10000; i++) {
        auto snapshot = engine.getTempoMap().loadSnapshot();

        if (!snapshot || snapshot->cachedPoints.empty()) {
            failed = true;
            break;
        }

        // Do 100 tempo conversions
        for (int j = 0; j < 100; j++) {
            double beats = (j % 64) * 1.0;
            double seconds = tempoMap.beatsToSeconds(beats, 44100);

            // Sanity check: time should be positive and reasonable
            if (seconds < 0 || seconds > 10000) {
                failed = true;
                break;
            }
        }
    }
});

messageThread.join();
audioThread.join();

assert(!failed);  // No crashes, no invalid data
```

**Success Criteria:**
- No data races (run with TSan if available)
- Snapshot pointer never nullptr
- No partial/invalid data observed

---

## B. ProjectState Integration Tests

### Test 5: Create → ValueTree → Engine
**Test Cases:**
```cpp
// Add tempo point via ProjectState API
String pointId = projectState.addTempoPoint(8.0, 140.0, "Test Add");

// Verify ValueTree contains it
auto tempoMap = projectState.getTempoMap();
assert(tempoMap.isValid());
assert(tempoMap.getNumChildren() >= 2);  // beat 0 + new point

auto points = projectState.getTempoPoints();
bool found = false;
for (auto& point : points) {
    if (point.getProperty("id") == pointId) {
        assert(point.getProperty("timeBeats") == 8.0);
        assert(point.getProperty("bpm") == 140.0);
        found = true;
    }
}
assert(found);

// Verify Engine's TempoMap sees the change
double tempo = engine.getTempoMap().getTempoAt(8.0);
assert(tempo == 140.0);
```

---

### Test 6: Undo/Redo Pipeline
**Test Cases:**
```cpp
UndoManager& undo = projectState.getUndoManager();

// Initial state: 1 point at beat 0
assert(projectState.getTempoPoints().size() == 1);

// Add point
String id1 = projectState.addTempoPoint(8.0, 140.0, "Add 1");
assert(projectState.getTempoPoints().size() == 2);
assert(engine.getTempoMap().getTempoAt(8.0) == 140.0);

// Move point
projectState.moveTempoPoint(id1, 12.0, 150.0, "Move 1");
assert(engine.getTempoMap().getTempoAt(12.0) == 150.0);

// Delete point
projectState.deleteTempoPoint(id1, "Delete 1");
assert(projectState.getTempoPoints().size() == 1);

// Undo delete
undo.undo();
assert(projectState.getTempoPoints().size() == 2);
assert(engine.getTempoMap().getTempoAt(12.0) == 150.0);

// Undo move
undo.undo();
assert(engine.getTempoMap().getTempoAt(8.0) == 140.0);

// Undo add
undo.undo();
assert(projectState.getTempoPoints().size() == 1);

// Redo all
undo.redo();  // Add
undo.redo();  // Move
undo.redo();  // Delete
assert(projectState.getTempoPoints().size() == 1);
```

**Verify at each step:**
- ValueTree matches
- Engine TempoMap snapshot matches
- UI repaint triggered (if visible)

---

### Test 7: Marker Integrity
**Test Cases:**
```cpp
// Add markers
String m1 = projectState.addMarker(0.0, "Intro", "Add M1");
String m2 = projectState.addMarker(16.0, "Verse", "Add M2");
String m3 = projectState.addMarker(32.0, "Chorus", "Add M3");

auto markers = projectState.getMarkers();
assert(markers.size() == 3);

// Verify sorted by time
assert(markers[0].getProperty("timeBeats") == 0.0);
assert(markers[1].getProperty("timeBeats") == 16.0);
assert(markers[2].getProperty("timeBeats") == 32.0);

// Rename
projectState.renameMarker(m2, "Verse 1", "Rename");
markers = projectState.getMarkers();
assert(markers[1].getProperty("name") == "Verse 1");

// Move (should re-sort)
projectState.moveMarker(m2, 40.0, "Move");
markers = projectState.getMarkers();
assert(markers[1].getProperty("timeBeats") == 32.0);  // Chorus moved to index 1
assert(markers[2].getProperty("timeBeats") == 40.0);  // Verse moved to index 2

// Undo/redo
projectState.getUndoManager().undo();
markers = projectState.getMarkers();
assert(markers[1].getProperty("timeBeats") == 16.0);  // Back to original order
```

**Verify:**
- No duplicate IDs
- No negative timeBeats
- Markers stay sorted after move

---

## C. CommandAPI / Wingman Integration

### Test 8: Command Dispatch
**Test Cases:**
```cpp
CommandAPI api(projectState, engine);

// Add tempo point
String request = R"({
    "command": "add_tempo_point",
    "params": { "timeBeats": 8.0, "bpm": 140.0 }
})";

String response = api.executeCommand(request);
auto responseObj = JSON::parse(response);

assert(responseObj.getProperty("status") == "ok");
String pointId = responseObj.getProperty("data").getProperty("pointId");
assert(!pointId.isEmpty());

// Verify in ProjectState
auto points = projectState.getTempoPoints();
bool found = false;
for (auto& pt : points) {
    if (pt.getProperty("id") == pointId) {
        assert(pt.getProperty("bpm") == 140.0);
        found = true;
    }
}
assert(found);

// Verify in Engine
assert(engine.getTempoMap().getTempoAt(8.0) == 140.0);
```

---

### Test 9: Error Handling
**Test Cases:**
```cpp
// Missing timeBeats
String badRequest1 = R"({
    "command": "add_tempo_point",
    "params": { "bpm": 140.0 }
})";
String response1 = api.executeCommand(badRequest1);
assert(JSON::parse(response1).getProperty("status") == "error");

// Negative BPM (should clamp to 40)
String badRequest2 = R"({
    "command": "add_tempo_point",
    "params": { "timeBeats": 8.0, "bpm": -50.0 }
})";
String response2 = api.executeCommand(badRequest2);
auto data = JSON::parse(response2).getProperty("data");
String pointId = data.getProperty("pointId");
auto points = projectState.getTempoPoints();
for (auto& pt : points) {
    if (pt.getProperty("id") == pointId) {
        assert(pt.getProperty("bpm") == 40.0);  // Clamped
    }
}

// Unknown command
String badRequest3 = R"({
    "command": "foo_bar_baz",
    "params": {}
})";
String response3 = api.executeCommand(badRequest3);
assert(JSON::parse(response3).getProperty("status") == "error");
```

**Verify:**
- Clear error messages
- No crashes
- State remains consistent

---

### Test 10: Idempotency
**Test Cases:**
```cpp
// Add marker twice with same name and time
String request = R"({
    "command": "add_marker",
    "params": { "timeBeats": 16.0, "name": "Verse" }
})";

String response1 = api.executeCommand(request);
String response2 = api.executeCommand(request);

// Both succeed (creates duplicates - this is allowed in MVP)
assert(JSON::parse(response1).getProperty("status") == "ok");
assert(JSON::parse(response2).getProperty("status") == "ok");

// Verify 2 markers at same position
auto markers = projectState.getMarkers();
int count = 0;
for (auto& m : markers) {
    if (m.getProperty("timeBeats") == 16.0 && m.getProperty("name") == "Verse") {
        count++;
    }
}
assert(count == 2);  // MVP allows duplicates
```

**Policy Decision:** Document whether duplicates are allowed or prevented.

---

## D. UI-Level Integration

### Test 11: Scrub + Edit (Manual)
**Steps:**
1. Start playback
2. While playing, drag playhead to jump around timeline
3. While playing, add tempo point via double-click
4. While playing, move existing tempo point
5. While playing, delete marker

**Verify:**
- No audio glitches
- No UI desync (cursor vs lanes vs grid)
- Engine sees updates without clicks/pops

---

### Test 12: Automation + Tempo Map (Critical!)
**Steps:**
1. Create tempo map: (0, 120), (16, 150)
2. Add track automation: volume ramp from 0.5 to 1.0 over 16 beats
3. Start playback
4. Verify automation samples correctly in both tempo segments

**Expected:**
- Automation uses `engine.getTempoMap().samplesToBeats()` (not single BPM)
- Volume ramps smoothly across tempo change
- No jumps at beat 16

**Code Verification:**
```cpp
// TrackAutomationSynchronizer.cpp should use:
double playbackBeats = engine.getTempoMap().samplesToBeats(frameCounter, sampleRate);

// NOT:
double playbackBeats = (frameCounter / sampleRate) * (tempo / 60.0);  // WRONG!
```

---

### Test 13: Long Session (Manual)
**Steps:**
1. Create 10-minute project (600 bars @ 4/4)
2. Add tempo changes at bars 0, 50, 100, 150, 200
3. Add markers: Intro, Verse, Chorus, Bridge, Outro (at appropriate bars)
4. Start playback
5. Jump between markers using marker-click navigation
6. Scrub through entire timeline

**Verify:**
- No drift in beat/time calculations over long duration
- Marker jumps are accurate
- Memory usage stays stable

---

## E. Save/Load Persistence

### Test 14: Project Save/Load
**Steps:**
1. Create tempo map: (0, 120), (8, 140), (16, 100)
2. Create markers: "Intro", "Verse", "Chorus"
3. Save to .zth file
4. Close project
5. Load .zth file

**Verify:**
- All tempo points restored with correct timeBeats and BPM
- All markers restored with correct names and positions
- Engine TempoMap snapshot rebuilt correctly
- UI lanes display correctly

**Test Cases:**
```cpp
// Before save
auto pointsBefore = projectState.getTempoPoints();
auto markersBefore = projectState.getMarkers();

// Save
projectState.saveToFile(File("test.zth"));

// Create new ProjectState and load
ProjectState newState;
newState.loadFromFile(File("test.zth"));

// Verify
auto pointsAfter = newState.getTempoPoints();
auto markersAfter = newState.getMarkers();

assert(pointsAfter.size() == pointsBefore.size());
assert(markersAfter.size() == markersBefore.size());

for (int i = 0; i < pointsAfter.size(); i++) {
    assert(pointsAfter[i].getProperty("timeBeats") == pointsBefore[i].getProperty("timeBeats"));
    assert(pointsAfter[i].getProperty("bpm") == pointsBefore[i].getProperty("bpm"));
}
```

---

## Summary Checklist

### Critical (Must Pass Before Merge)
- [ ] Test 1: Single tempo sanity
- [ ] Test 2: Step tempo change + round-trip
- [ ] Test 4: Snapshot atomicity (RT-safety)
- [ ] Test 5: ProjectState → Engine sync
- [ ] Test 6: Undo/redo pipeline
- [ ] Test 12: **Automation + TempoMap integration**
- [ ] Test 14: Save/load persistence

### Important (Should Pass)
- [ ] Test 3: Dense grid stability
- [ ] Test 7: Marker integrity
- [ ] Test 8: CommandAPI dispatch
- [ ] Test 9: Error handling
- [ ] Test 11: Scrub + edit (manual)

### Nice to Have
- [ ] Test 10: Idempotency
- [ ] Test 13: Long session (manual)

---

## Known Issues / TODO

1. **Frame counter drift:** `TrackAutomationSynchronizer` uses a rough frame counter estimate. Replace with actual Engine playback position getter.

2. **Jump-to-marker:** Not implemented in MVP. Add in next phase.

3. **Grid drawing:** Verify bar/beat lines in UI arranger use TempoMap (not hardcoded BPM).

4. **Export/Render:** Not yet tempo-aware. Defer to later phase.

---

## Test Execution Log

| Test | Date | Status | Notes |
|------|------|--------|-------|
| Test 1 | TBD | ⏳ | |
| Test 2 | TBD | ⏳ | |
| Test 3 | TBD | ⏳ | |
| Test 4 | TBD | ⏳ | |
| Test 5 | TBD | ⏳ | |
| Test 6 | TBD | ⏳ | |
| Test 7 | TBD | ⏳ | |
| Test 8 | TBD | ⏳ | |
| Test 9 | TBD | ⏳ | |
| Test 10 | TBD | ⏳ | |
| Test 11 | TBD | ⏳ | Manual test |
| Test 12 | TBD | ⏳ | **CRITICAL** |
| Test 13 | TBD | ⏳ | Manual test |
| Test 14 | TBD | ⏳ | |

---

**Updated:** After fixing TrackAutomationSynchronizer single-BPM bug
