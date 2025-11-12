# STEP W3 - WASAPI Tuning Surface - Testing Checklist

**Build:** Windows 10/11, Visual Studio 2022, JUCE 8.0.9
**Scope:** UI stub only - no engine mutation (device changes not applied yet)
**Date:** 2025-11-12

---

## Pre-Test Setup

### Environment

- [ ] Windows 10 version 1909+ or Windows 11
- [ ] Visual Studio 2022 with v143 toolset installed
- [ ] CMake 3.22+ installed
- [ ] At least one audio output device (speakers, headphones)
- [ ] Optionally: Multiple audio devices (USB, HDMI, ASIO interface)

### Build

```bash
cd zenith-core
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Launch

```bash
.\build\Release\Zenith DAW.exe
```

Expected: Application launches without errors, audio device initializes.

---

## Test 1: Basic UI Visibility

### Steps

1. Launch Zenith DAW
2. Click the **⚙ (Settings)** button in the top bar

### Expected Behavior

- [ ] Audio settings panel appears as centered overlay (500x400 px)
- [ ] Panel has title "Audio Device Settings"
- [ ] Panel displays:
  - Device Type dropdown
  - Output Device dropdown
  - Input Device dropdown
  - Mode label (read-only)
  - Sample Rate dropdown
  - Buffer Size dropdown
  - Latency display (calculated, e.g., "~5.3 ms")
  - "Open ASIO Panel..." button (grayed out if not ASIO)
  - Status line at bottom (e.g., "Status: Ready")

### Debug Output

Check console for:
```
Settings clicked
Audio settings panel shown
AudioSettingsWindows: Refreshing devices...
Device types populated: <N> types
Devices populated - Output: <N>, Input: <N>
Sample rates populated: <N> rates
Buffer sizes populated: <N> sizes
```

---

## Test 2: Device Enumeration

### Steps

1. Open audio settings panel
2. Inspect **Device Type** dropdown

### Expected Behavior

- [ ] At least one device type is listed (e.g., "Windows Audio")
- [ ] On Windows, typical types:
  - "Windows Audio" (WASAPI) - Always present
  - "ASIO" - Only if ASIO driver installed
  - "DirectSound" - May be present (legacy)
- [ ] Current device type is pre-selected

### Steps (Output Device)

3. Click **Output Device** dropdown

### Expected Behavior

- [ ] Lists all available output devices (speakers, headphones, HDMI)
- [ ] Current output device is pre-selected (if audio is already playing)
- [ ] Device names match Windows Sound settings

### Steps (Input Device)

4. Click **Input Device** dropdown

### Expected Behavior

- [ ] Lists all available input devices (microphones, line-in)
- [ ] At least one device listed (built-in mic, USB mic, etc.)

---

## Test 3: Sample Rate Dropdown

### Steps

1. Open audio settings panel
2. Note the current **Sample Rate** selection
3. Click the dropdown

### Expected Behavior

- [ ] Dropdown shows multiple sample rates (e.g., 44100 Hz, 48000 Hz, etc.)
- [ ] Common rates available:
  - 44100 Hz (near-universal)
  - 48000 Hz (universal)
  - 88200 Hz, 96000 Hz (if pro interface)
- [ ] Current device's sample rate is pre-selected
- [ ] Changing selection updates **Latency** display

### Debug Output

```
Sample rate changed: <rate> Hz
```

### Edge Case: ASIO Device

- [ ] If ASIO device selected, sample rate dropdown may be disabled (ASIO driver controls rate)

---

## Test 4: Buffer Size Dropdown

### Steps

1. Open audio settings panel
2. Note the current **Buffer Size** selection
3. Click the dropdown

### Expected Behavior

- [ ] Dropdown shows multiple buffer sizes (e.g., 128, 256, 512 samples)
- [ ] WASAPI typical sizes:
  - Multiples of 32: 32, 64, 96, 128, 160, 192, 224, 256, ..., 512
  - Multiples of 64: 576, 640, ..., 1024
  - Larger sizes: 1152, 1280, ...
- [ ] ASIO typical sizes (if present):
  - Powers of 2: 32, 64, 128, 256, 512, 1024, 2048
- [ ] Current device's buffer size is pre-selected
- [ ] Changing selection updates **Latency** display

### Debug Output

```
Buffer size changed: <size> samples
```

---

## Test 5: Mode Detection

### Steps

1. Open audio settings panel
2. Observe the **Mode** label value

### Expected Behavior

**WASAPI devices:**
- [ ] If buffer sizes include 32 or 64: "Shared/Exclusive (automatic)"
- [ ] If only large buffer sizes (480+): "Shared (automatic)"

**ASIO devices:**
- [ ] "Exclusive (ASIO)"
- [ ] "Open ASIO Panel..." button is **enabled**

**DirectSound:**
- [ ] "Shared (automatic)"

### Note

Mode is read-only and automatically determined. Users cannot explicitly select Shared vs Exclusive in this UI stub.

---

## Test 6: Latency Calculation

### Steps

1. Open audio settings panel
2. Note **Sample Rate** and **Buffer Size**
3. Calculate expected latency manually:
   ```
   Latency (ms) = (Buffer Size / Sample Rate) × 1000
   ```
4. Compare with displayed **Latency** value

### Examples

| Sample Rate | Buffer Size | Expected Latency |
|-------------|-------------|-------------------|
| 48000 Hz    | 64          | ~1.33 ms          |
| 48000 Hz    | 128         | ~2.67 ms          |
| 48000 Hz    | 256         | ~5.33 ms          |
| 48000 Hz    | 480         | ~10.0 ms          |
| 44100 Hz    | 256         | ~5.80 ms          |

### Expected Behavior

- [ ] Displayed latency matches manual calculation (±0.1 ms)
- [ ] Latency updates immediately when sample rate or buffer size changes

---

## Test 7: ASIO Control Panel Button

### Steps (if ASIO driver installed)

1. Open audio settings panel
2. Select an ASIO device from **Device Type** or **Output Device**
3. Observe **"Open ASIO Panel..."** button

### Expected Behavior

- [ ] Button is **enabled** (not grayed out)
- [ ] Clicking button logs debug output:
  ```
  ASIO control panel requested
  ```
- [ ] (W3 stub: panel does not open yet - no engine mutation)

### Steps (WASAPI or DirectSound device)

4. Select a non-ASIO device

### Expected Behavior

- [ ] Button is **disabled** (grayed out)

---

## Test 8: Device Type Switching

### Steps

1. Open audio settings panel
2. Note the current **Device Type** (e.g., "Windows Audio")
3. Change **Device Type** dropdown (e.g., to "DirectSound" if available)

### Expected Behavior

- [ ] **Output Device** and **Input Device** dropdowns refresh with new device list
- [ ] **Sample Rate** and **Buffer Size** dropdowns refresh
- [ ] **Mode** label updates based on new device type
- [ ] **Latency** recalculates
- [ ] Status line shows: "Device type changed (restart required to apply)"

### Debug Output

```
Device type changed: <type>
Devices populated - Output: <N>, Input: <N>
```

---

## Test 9: Output Device Switching

### Steps

1. Open audio settings panel
2. Change **Output Device** dropdown to a different device

### Expected Behavior

- [ ] **Sample Rate** dropdown updates (devices may support different rates)
- [ ] **Buffer Size** dropdown updates (devices may support different sizes)
- [ ] **Mode** label updates (some devices don't support exclusive mode)
- [ ] **Latency** recalculates
- [ ] Status line shows: "Output device changed (restart required to apply)"

### Debug Output

```
Output device changed: <device name>
Sample rates populated: <N> rates
Buffer sizes populated: <N> sizes
Audio settings changed:
  Device Type: <type>
  Output: <device>
  Input: <device>
  Sample Rate: <rate>
  Buffer Size: <size>
```

---

## Test 10: Panel Toggle (Open/Close)

### Steps

1. Click **⚙ (Settings)** button
2. Panel opens
3. Click **⚙ (Settings)** button again

### Expected Behavior

- [ ] Panel closes (disappears)
- [ ] No crashes or errors

### Debug Output

```
Settings clicked
Audio settings panel shown
Settings clicked
Audio settings panel hidden
```

### Steps (Re-open)

4. Click **⚙ (Settings)** button again

### Expected Behavior

- [ ] Panel re-opens with same position and refreshed device list

---

## Test 11: Multiple Devices (Multi-Audio Hardware)

### Steps (if available)

1. Ensure multiple audio devices are connected:
   - Built-in speakers
   - USB headphones
   - HDMI audio
   - External audio interface (USB, Thunderbolt)
2. Open audio settings panel
3. Switch between devices in **Output Device** dropdown

### Expected Behavior

- [ ] All devices appear in the list
- [ ] Switching updates sample rate/buffer size options correctly
- [ ] No crashes or missing devices
- [ ] Device names are descriptive (not just "Device 1", "Device 2")

---

## Test 12: Edge Case - No Audio Device

### Steps (simulation)

1. Disable all audio devices in Windows Sound settings
2. Launch Zenith DAW

### Expected Behavior

- [ ] Application launches without crash
- [ ] Audio settings panel shows:
  - "No device" in Mode label
  - Default sample rates (44100 Hz, 48000 Hz)
  - Default buffer sizes (128, 256, 512 samples)
- [ ] Status line may show "No audio devices found"

---

## Test 13: Status Line Updates

### Steps

1. Open audio settings panel
2. Perform various actions:
   - Change device type → Check status
   - Change output device → Check status
   - Change sample rate → Check status
   - Change buffer size → Check status

### Expected Behavior

- [ ] Status line updates to reflect each change:
  - "Device type changed (restart required to apply)"
  - "Output device changed (restart required to apply)"
  - "Sample rate changed (restart required to apply)"
  - "Buffer size changed (restart required to apply)"

---

## Test 14: W3 Stub Limitation - No Engine Mutation

### Important Note

**W3 is a UI stub only.** Device changes are **NOT applied to the audio engine** yet.

### Steps

1. Open audio settings panel
2. Change **Output Device** to a different device
3. Close the panel
4. Play audio (test tone or project)

### Expected Behavior

- [ ] Audio continues playing on the **original** device (before change)
- [ ] Device change is **not applied** (expected behavior for W3)
- [ ] Debug log shows:
  ```
  Audio settings changed: ...
  W3 stub: no engine mutation yet
  ```

### Rationale

Engine device restart requires:
- Stopping audio callback
- Closing current device
- Opening new device
- Restarting audio callback

This will be implemented in a future step (W4 or later).

---

## Test 15: Visual Regression

### Steps

1. Open audio settings panel
2. Visually inspect the UI

### Expected Behavior

- [ ] Panel background: Dark (ZenithColours::backgroundDark)
- [ ] Border: Visible, 1px, ZenithColours::border
- [ ] Text: Readable, consistent font sizes
- [ ] Labels: Right-aligned, 120px width
- [ ] Dropdowns: Aligned, fill remaining width
- [ ] Latency value: Accent color (ZenithColours::accent)
- [ ] Mode value: Secondary text color (ZenithColours::textSecondary)
- [ ] No overlapping elements
- [ ] No clipped text

---

## Debugging Failed Tests

### If dropdowns are empty:

1. Check debug console for error messages
2. Verify JUCE's `AudioDeviceManager` is initialized in `Engine::initialize()`
3. Check `deviceManager.getAvailableDeviceTypes()` returns valid types

### If latency calculation is wrong:

1. Verify sample rate extraction: `sampleRateComboBox.getText().upToFirstOccurrenceOf(" ", false, false).getDoubleValue()`
2. Verify buffer size extraction: `bufferSizeComboBox.getText().upToFirstOccurrenceOf(" ", false, false).getIntValue()`
3. Check formula: `(bufferSize / sampleRate) * 1000.0`

### If panel doesn't appear:

1. Check `setupCallbacks()` is called in `MainComponent` constructor
2. Verify `audioSettingsPanel` is created and added with `addChildComponent()`
3. Check `toggleAudioSettings()` sets `setVisible(true)` and calls `toFront(true)`

---

## Performance Notes

**W3 is a UI stub with minimal performance impact:**
- Device enumeration happens only when panel opens (not every frame)
- No engine restarts or audio processing changes
- Debug logging may add overhead (disable for release builds)

---

## Next Steps (Post-W3)

After W3 approval, the following features will be added:

1. **Device mutation:** Apply settings changes to `AudioDeviceManager`
   - `deviceManager.setAudioDeviceSetup(newSetup)`
   - Handle device stop/restart gracefully

2. **ASIO control panel:** Implement `device->showControlPanel()` for ASIO devices

3. **Input device selection:** Support separate input/output devices (if JUCE permits)

4. **Per-device presets:** Save preferred sample rate/buffer size per device

5. **Hotplug handling:** Detect device add/remove events, update UI

---

**END OF W3 TESTING CHECKLIST**
