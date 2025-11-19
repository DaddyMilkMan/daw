# Zenith DAW Instrument System - Manual Test Checklist
**Date**: 2025-11-18
**Purpose**: Step-by-step manual testing guide for instrument system

---

## Quick Reference

**Test Status Legend**:
- ⬜ Not Started
- 🔄 In Progress
- ✅ Passed
- ❌ Failed

---

## Prerequisites

### System Dependencies (Linux)
```bash
sudo apt-get install -y \
    libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libfreetype-dev libasound2-dev \
    libgl1-mesa-dev
```

### Build
```bash
cd zenith-core
rm -rf build && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j4
```

---

## Test Suite 1: Automated Tests

### Test 1.1: Instrument Validation Tests
**Run**:
```bash
cd build
ctest -R InstrumentValidationTests -V
```

**Expected Output**:
```
✓ All validation tests passed!
✓ 2 instruments validated
```

**Status**: ⬜

---

### Test 1.2: Track+Instrument Integration Tests
**Run**:
```bash
cd build
ctest -R TrackInstrumentIntegrationTests -V
```

**Expected Output**:
```
✓✓✓ ALL TESTS PASSED ✓✓✓
Tests passed: 15
Tests failed: 0
```

**Status**: ⬜

---

## Test Suite 2: UI Testing (Full DAW)

### Test 2.1: Launch DAW
**Steps**:
1. Run: `./build/ZenithDAW_artefacts/Debug/ZenithDAW`
2. Verify: Main window opens
3. Verify: No crashes or error dialogs

**Expected**: ✅ DAW launches successfully

**Status**: ⬜

---

### Test 2.2: Create Instrument Track (UI)
**Steps**:
1. Click **"Add Track"** button (or Ctrl+T)
2. Select type: **"Instrument"**
3. Name: **"Test Synth"**
4. Click **"Create"**

**Expected**:
- ✅ New track appears in arranger
- ✅ Track shows type "Instrument"
- ✅ Track has empty instrument slot

**Status**: ⬜

---

### Test 2.3: Create Instrument Track (CommandAPI)
**Prerequisites**: DAW running with CommandAPI server on port 3737

**Run**:
```bash
curl -X POST http://localhost:3737/api/command \
  -H "Content-Type: application/json" \
  -d '{"command": "track.create", "args": {"type": "instrument", "name": "API Test Synth"}}'
```

**Expected**:
- ✅ New track appears in UI
- ✅ Response: `{"success": true, "trackId": "..."}`

**Status**: ⬜

---

### Test 2.4: Attach Instrument to Track (UI)
**Steps**:
1. Click on instrument track
2. In track inspector, click **"Add Instrument"**
3. Select **"ZenithPolySynth"**
4. Click **"Load"**

**Expected**:
- ✅ Instrument editor window opens
- ✅ Track icon shows synth icon
- ✅ No crashes

**Status**: ⬜

---

### Test 2.5: Attach Instrument to Track (CommandAPI)
**Run**:
```bash
# Get track ID from previous test
TRACK_ID="<track-id-from-test-2.3>"

curl -X POST http://localhost:3737/api/command \
  -H "Content-Type: application/json" \
  -d "{\"command\": \"instrument.attach\", \"args\": {\"trackId\": \"$TRACK_ID\", \"instrumentId\": \"zenith.poly_synth\"}}"
```

**Expected**:
- ✅ Instrument attached
- ✅ Editor opens
- ✅ Response: `{"success": true}`

**Status**: ⬜

---

### Test 2.6: Create MIDI Clip with Notes
**Steps**:
1. Double-click on instrument track timeline (bar 1)
2. Verify: MIDI clip created
3. Double-click clip to open MIDI editor
4. Draw notes:
   - C4 (MIDI 60) at beat 0, length 1 beat
   - E4 (MIDI 64) at beat 1, length 1 beat
   - G4 (MIDI 67) at beat 2, length 1 beat
5. Close MIDI editor

**Expected**:
- ✅ Clip appears on timeline
- ✅ Notes visible in MIDI editor
- ✅ Notes saved when editor closed

**Status**: ⬜

---

### Test 2.7: Playback with Instrument
**Steps**:
1. Move playhead to start (Home key)
2. Press **Play** (Spacebar)
3. Listen for 4 seconds
4. Press **Stop** (Spacebar)

**Expected**:
- ✅ Hear C major chord from ZenithPolySynth
- ✅ Track level meters show audio activity
- ✅ No audio dropouts or glitches
- ✅ No crackling or distortion
- ✅ No crashes

**Status**: ⬜

---

### Test 2.8: Parameter Control (UI)
**Steps**:
1. Open instrument editor (click track instrument icon)
2. Move **Filter Cutoff** slider to 30%
3. Press **Play** and listen
4. Move **Resonance** slider to 70%
5. Listen for filter sweep effect

**Expected**:
- ✅ Parameters respond immediately
- ✅ Sound changes as expected
- ✅ No audio glitches during parameter changes

**Status**: ⬜

---

### Test 2.9: Parameter Control (CommandAPI)
**Run**:
```bash
TRACK_ID="<track-id>"

# Set filter cutoff to 30%
curl -X POST http://localhost:3737/api/command \
  -H "Content-Type: application/json" \
  -d "{\"command\": \"instrument.setParameter\", \"args\": {\"trackId\": \"$TRACK_ID\", \"parameterId\": \"filter_cutoff\", \"value\": 0.3}}"

# Set resonance to 70%
curl -X POST http://localhost:3737/api/command \
  -H "Content-Type: application/json" \
  -d "{\"command\": \"instrument.setParameter\", \"args\": {\"trackId\": \"$TRACK_ID\", \"parameterId\": \"filter_resonance\", \"value\": 0.7}}"
```

**Expected**:
- ✅ Parameters update in real-time
- ✅ UI sliders move to new positions
- ✅ Sound changes immediately

**Status**: ⬜

---

### Test 2.10: Preset Loading (UI)
**Steps**:
1. Open instrument editor
2. Click **"Presets"** dropdown
3. Select **"Brass Section"** (or any available preset)
4. Press **Play** and listen

**Expected**:
- ✅ All parameters update
- ✅ Sound character changes
- ✅ UI reflects new parameter values

**Status**: ⬜

---

### Test 2.11: Preset Loading (CommandAPI)
**Run**:
```bash
TRACK_ID="<track-id>"

# Load brass preset
curl -X POST http://localhost:3737/api/command \
  -H "Content-Type: application/json" \
  -d "{\"command\": \"instrument.loadPreset\", \"args\": {\"trackId\": \"$TRACK_ID\", \"presetId\": \"poly_synth.brass_section\"}}"
```

**Expected**:
- ✅ Preset loads
- ✅ UI updates
- ✅ Response: `{"success": true}`

**Status**: ⬜

---

### Test 2.12: Multiple Instrument Tracks
**Steps**:
1. Create 3 instrument tracks
2. Attach different instruments:
   - Track 1: ZenithPolySynth
   - Track 2: ZenithSampler
   - Track 3: ZenithPolySynth (different preset)
3. Add MIDI clips to each track
4. Press **Play**

**Expected**:
- ✅ All instruments play simultaneously
- ✅ No audio dropouts
- ✅ CPU usage reasonable (<50% on modern CPU)
- ✅ No crashes

**Status**: ⬜

---

### Test 2.13: Track Mixer Controls
**Steps**:
1. Play instrument track
2. Adjust track **Volume** slider (50% → 100%)
3. Adjust track **Pan** slider (Left → Center → Right)
4. Click track **Mute** button
5. Click track **Solo** button

**Expected**:
- ✅ Volume changes audible
- ✅ Pan moves sound left/right
- ✅ Mute silences track
- ✅ Solo isolates track

**Status**: ⬜

---

### Test 2.14: Project Save/Load with Instruments
**Steps**:
1. Create instrument track with ZenithPolySynth
2. Set some parameters (filter, envelope, etc.)
3. Add MIDI clip with notes
4. Save project: `File → Save` or **Ctrl+S**
5. Close DAW
6. Reopen DAW
7. Open saved project: `File → Open`

**Expected**:
- ✅ Instrument track restored
- ✅ ZenithPolySynth attached
- ✅ All parameter values restored
- ✅ MIDI clips and notes present
- ✅ Playback sounds identical

**Status**: ⬜

---

### Test 2.15: WAV Export with Instruments
**Steps**:
1. Create project with instrument track and MIDI notes
2. Select `File → Export → Export to WAV`
3. Choose output path: `/tmp/zenith_instrument_test.wav`
4. Set duration: 5 seconds
5. Click **Export**
6. Wait for export to complete
7. Open exported WAV in media player

**Expected**:
- ✅ Export completes without errors
- ✅ WAV file created
- ✅ File plays back correctly
- ✅ Audio matches real-time playback
- ✅ No clicks or pops at start/end

**Status**: ⬜

---

## Test Suite 3: Stress Testing

### Test 3.1: Polyphony Stress Test
**Steps**:
1. Create instrument track with ZenithPolySynth
2. Create MIDI clip with **32 overlapping notes** (chord)
3. Loop clip for 30 seconds
4. Press **Play** and monitor

**Expected**:
- ✅ All voices play (up to polyphony limit)
- ✅ No audio glitches
- ✅ CPU usage stable
- ✅ No memory leaks (check System Monitor)

**Status**: ⬜

---

### Test 3.2: Fast Parameter Automation
**Steps**:
1. Create instrument track
2. Automate **Filter Cutoff** with rapid changes (LFO-like)
3. Play for 1 minute

**Expected**:
- ✅ Parameter updates smoothly
- ✅ No audio glitches
- ✅ No crashes

**Status**: ⬜

---

### Test 3.3: Rapid Note Triggering
**Steps**:
1. Create MIDI clip with **128 notes in quick succession** (1/32nd notes)
2. Loop and play for 30 seconds

**Expected**:
- ✅ All notes trigger
- ✅ No stuck notes
- ✅ No audio dropouts

**Status**: ⬜

---

## Test Suite 4: Edge Cases

### Test 4.1: Instrument Without MIDI
**Steps**:
1. Create instrument track with ZenithPolySynth
2. Do NOT add any MIDI clips
3. Press **Play**

**Expected**:
- ✅ No crashes
- ✅ No audio output (silence)
- ✅ Track meters show zero activity

**Status**: ⬜

---

### Test 4.2: Remove Instrument While Playing
**Steps**:
1. Create instrument track, add MIDI, press **Play**
2. While playing, click instrument and select **"Remove Instrument"**

**Expected**:
- ✅ Audio stops immediately
- ✅ No crashes
- ✅ No audio artifacts

**Status**: ⬜

---

### Test 4.3: Change Sample Rate
**Steps**:
1. Create project with instrument
2. Stop playback
3. Change audio device sample rate (44.1kHz → 48kHz)
4. Resume playback

**Expected**:
- ✅ Instrument re-initializes
- ✅ Playback continues normally
- ✅ No crashes

**Status**: ⬜

---

### Test 4.4: Change Buffer Size
**Steps**:
1. Create project with instrument
2. Stop playback
3. Change audio buffer size (512 → 1024 samples)
4. Resume playback

**Expected**:
- ✅ Instrument re-initializes
- ✅ Playback continues normally
- ✅ No crashes

**Status**: ⬜

---

## Test Results Summary

### Automated Tests
- ⬜ Test 1.1: Instrument Validation Tests
- ⬜ Test 1.2: Track+Instrument Integration Tests

### UI Tests
- ⬜ Test 2.1: Launch DAW
- ⬜ Test 2.2: Create Instrument Track (UI)
- ⬜ Test 2.3: Create Instrument Track (CommandAPI)
- ⬜ Test 2.4: Attach Instrument (UI)
- ⬜ Test 2.5: Attach Instrument (CommandAPI)
- ⬜ Test 2.6: Create MIDI Clip
- ⬜ Test 2.7: Playback
- ⬜ Test 2.8: Parameter Control (UI)
- ⬜ Test 2.9: Parameter Control (CommandAPI)
- ⬜ Test 2.10: Preset Loading (UI)
- ⬜ Test 2.11: Preset Loading (CommandAPI)
- ⬜ Test 2.12: Multiple Instruments
- ⬜ Test 2.13: Mixer Controls
- ⬜ Test 2.14: Save/Load
- ⬜ Test 2.15: WAV Export

### Stress Tests
- ⬜ Test 3.1: Polyphony Stress
- ⬜ Test 3.2: Fast Automation
- ⬜ Test 3.3: Rapid Notes

### Edge Cases
- ⬜ Test 4.1: No MIDI Input
- ⬜ Test 4.2: Remove While Playing
- ⬜ Test 4.3: Sample Rate Change
- ⬜ Test 4.4: Buffer Size Change

---

## Sign-Off

**Tested By**: ________________
**Date**: ________________
**Result**: ⬜ PASSED ⬜ FAILED
**Notes**:

---

**End of Manual Test Checklist**
