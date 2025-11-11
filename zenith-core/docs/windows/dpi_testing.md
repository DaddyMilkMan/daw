# Windows DPI Testing Guide

## Overview

Zenith DAW uses **Per-Monitor V2 DPI awareness** for crisp rendering across multi-monitor setups with mixed DPI settings.

## What is Per-Monitor V2?

- **Windows 10 1703+** feature
- Each window automatically scales based on its current monitor's DPI
- Non-client area (title bar, borders) scales automatically
- Best-in-class DPI handling for multi-monitor workflows

## Testing Checklist

### Single Monitor Tests

#### 1. Test at Standard Scale Factors

Test at each common scale factor:

| Scale | Resolution Example | Notes |
|-------|-------------------|-------|
| 100% | 1920x1080 | Baseline |
| 125% | 1920x1080 | Common laptop setting |
| 150% | 3840x2160 | 4K monitor default |
| 175% | 3840x2160 | Less common |
| 200% | 3840x2160 | High DPI displays |

**How to change:**
1. Right-click desktop → Display settings
2. Scroll to "Scale and layout"
3. Select scale factor from dropdown
4. **Sign out and back in** (recommended) or just restart Zenith

**What to check:**
- [ ] Window renders crisp, not blurry
- [ ] Text is readable and sharp
- [ ] UI elements (buttons, sliders, meters) are correctly sized
- [ ] No clipped text or truncated controls
- [ ] Waveforms/graphics render smoothly
- [ ] Timeline grid lines are 1 logical pixel (not fuzzy)

#### 2. Font Rendering Test

At each scale factor above:

- [ ] TopBar project name: sharp edges, no color fringing
- [ ] Transport BPM display: crisp numbers
- [ ] Timeline ruler markers: readable at all zoom levels
- [ ] Sidebar track names: no blurriness

**Expected:** Text should be rendered by DirectWrite with subpixel anti-aliasing and proper hinting.

---

### Multi-Monitor Tests (Critical for DAW workflows)

#### Setup: Mixed DPI Configuration

**Recommended test setup:**
- **Monitor 1 (Primary):** 100% scale (e.g., 1920x1080)
- **Monitor 2 (Secondary):** 150% or 200% scale (e.g., 4K)

**How to configure:**
1. Windows Settings → System → Display
2. Select each monitor and set individual scale factors
3. Restart Zenith after changes

#### Test Cases

##### TC1: Window Drag Between Monitors

1. Launch Zenith on 100% monitor
2. **Slowly** drag window to 150% monitor
3. **Expected:** Window should re-render smoothly during drag; may briefly show slight resize
4. **Check:** Text/UI remains sharp after drag completes

**Common issues to watch for:**
- ❌ Window stuck at wrong size after drag
- ❌ Content blurry after drag (indicates DPI not updated)
- ❌ Window larger than screen (scale factor miscalculation)

##### TC2: Maximized Window Behavior

1. Maximize window on 100% monitor
2. Use **Win + Shift + Arrow** to move maximized window to 150% monitor
3. **Expected:** Window maximizes on new monitor at correct DPI scale

##### TC3: Rapid Monitor Switching

1. Open Zenith on Monitor 1
2. Quickly drag to Monitor 2 and back to Monitor 1 (3-5 times)
3. **Expected:** No crashes, no stuck scaling, UI remains responsive

**Debug if issues:**
- Check Event Viewer for WM_DPICHANGED handling
- Verify manifest is embedded: `dumpbin /manifest ZenithDAW.exe`

##### TC4: Detach/Reattach Monitor

1. Launch Zenith on secondary monitor (150%)
2. **Physically disconnect** secondary monitor while Zenith is running
3. **Expected:** Window moves to primary monitor, rescales to 100%
4. Reconnect monitor
5. **Expected:** Can drag back to secondary, rescales to 150%

---

### Programmatic DPI Queries (for developers)

Add this debug code to MainComponent::paint() (DEBUG only):

```cpp
#if JUCE_DEBUG
    auto dpi = getPeer()->getNativeHandle() ?
               GetDpiForWindow((HWND)getPeer()->getNativeHandle()) : 96;
    auto scale = dpi / 96.0;
    g.setColour(Colours::red);
    g.drawText("DPI: " + String(dpi) + " (Scale: " + String(scale, 2) + "x)",
               getLocalBounds().removeFromTop(20), Justification::left);
#endif
```

Should show:
- **96 DPI** = 100% scale
- **120 DPI** = 125% scale
- **144 DPI** = 150% scale
- **192 DPI** = 200% scale

---

### Regression Tests

After any UI changes:

1. **Grid alignment:** Timeline ruler ticks should align perfectly with beat markers
2. **Pixel-perfect borders:** No doubled lines or gaps at any scale
3. **Hover states:** Buttons/sliders should highlight correctly at all scales
4. **Resize handles:** Drag handles should be hittable at all scales (not too small at 200%)

---

### Known Issues & Workarounds

#### Issue: Blurry rendering after monitor switch
- **Cause:** JUCE not handling WM_DPICHANGED properly
- **Workaround:** Manually trigger repaint in ComponentPeer::handleDPIChange

#### Issue: Wrong window size after drag (Windows 10 1703-1803)
- **Cause:** OS bug in early PerMonitorV2 builds
- **Solution:** Update to Windows 10 1809+ or use PerMonitor (V1) fallback

#### Issue: Non-client area (title bar) not scaling
- **Expected:** This is correct if using native title bar
- **Solution:** If using custom title bar, ensure `setUsingNativeTitleBar(false)` and handle DPI in custom draw

---

### Tools

**Windows SDK tools:**

```powershell
# Dump embedded manifest
dumpbin /manifest ZenithDAW.exe

# Check DPI awareness at runtime
Get-Process ZenithDAW | Select-Object -ExpandProperty ProcessName | ForEach-Object { (Get-Process $_).StartInfo.EnvironmentVariables }
```

**Third-party tools:**
- **WinSpy++**: Inspect window DPI awareness flags
- **Spy++**: Monitor WM_DPICHANGED messages
- **Process Explorer**: Check process DPI awareness mode

---

### Performance Metrics

At each scale factor, measure (add to debug HUD in STEP W6):

| Metric | 100% | 125% | 150% | 200% |
|--------|------|------|------|------|
| Paint time (ms) | ___ | ___ | ___ | ___ |
| Frames per second | ___ | ___ | ___ | ___ |
| Memory (MB) | ___ | ___ | ___ | ___ |

**Expected:** Paint time should scale roughly linearly with pixel count, FPS should stay >60 at all scales.

---

### Sign-off Checklist

Before marking DPI support as "stable":

- [ ] Tested all scale factors (100%, 125%, 150%, 200%)
- [ ] Tested multi-monitor with mixed DPI (100% + 150%)
- [ ] Verified manifest is embedded (`dumpbin /manifest`)
- [ ] No blurry rendering at any scale
- [ ] Window drag between monitors works smoothly
- [ ] Maximize/restore works correctly across monitors
- [ ] Hot-plug monitor disconnect/reconnect works
- [ ] Performance acceptable at 200% (highest load)
- [ ] No crashes during rapid monitor switching

---

## References

- [Microsoft: High DPI Desktop Application Development](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows)
- [Microsoft: Per-Monitor DPI Awareness](https://learn.microsoft.com/en-us/windows/win32/hidpi/dpi-awareness-context)
- JUCE Forum: Search "DPI" for known issues and workarounds
