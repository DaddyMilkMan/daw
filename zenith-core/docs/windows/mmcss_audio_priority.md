# Windows MMCSS Audio Priority

## Overview

Zenith uses **Windows Multimedia Class Scheduler Service (MMCSS)** to boost audio thread priority, preventing glitches and dropouts during low-latency operation.

## What is MMCSS?

MMCSS is a Windows Vista+ system service that prioritizes time-critical multimedia threads (audio, video) over background tasks, while still ensuring the system remains responsive.

## Implementation

### Automatic "Pro Audio" Priority

When the audio engine starts (`Engine::audioDeviceAboutToStart`), Zenith automatically registers the audio callback thread with MMCSS using the **"Pro Audio"** task:

```cpp
static MMCSSAudioPriority audioPriority(L"Pro Audio");
```

This provides:
- **Thread priority boost**: Audio thread runs at a higher scheduler priority than normal threads
- **CPU reservation**: Windows reserves CPU time for audio threads, even under heavy load
- **Glitch prevention**: Reduces risk of buffer underruns causing clicks/pops

### Task Profiles

MMCSS supports different task profiles:

| Task Name | Use Case | Priority | Typical Latency |
|-----------|----------|----------|-----------------|
| **Pro Audio** | Professional DAWs, buffers < 10ms | Highest | 64-256 samples |
| **Audio** | Consumer apps, buffers >= 10ms | High | 480-1024 samples |
| **Playback** | Media players | Medium | 1000+ samples |

Zenith uses **"Pro Audio"** for maximum real-time performance.

## Configuration

### Enable/Disable MMCSS

MMCSS is **enabled by default**. To disable (for debugging thread priority issues):

```bash
# Disable MMCSS
cmake -S . -B build -DZENITH_ENABLE_MMCSS=OFF

# Re-enable (default)
cmake -S . -B build -DZENITH_ENABLE_MMCSS=ON
```

### Runtime Detection

Check if MMCSS is active at runtime:

```cpp
#ifdef _WIN32
    MMCSSAudioPriority priority(L"Pro Audio");
    if (priority.isActive()) {
        DBG("MMCSS active");
    } else {
        DBG("MMCSS failed - check if service is running");
    }
#endif
```

## Testing

### Test 1: CPU Spike Stress Test

1. Launch Zenith with 64-sample buffer size (1.3ms @ 48kHz)
2. Start playback
3. Open Task Manager → Details → Find `ZenithDAW.exe`
4. Right-click → "Set priority" → Try changing to "Below Normal"
5. **Expected:** Zenith should maintain stable audio despite priority change (MMCSS overrides manual priority)

### Test 2: Background Load Test

1. Launch Zenith with 128-sample buffer
2. Start looping playback with multiple tracks
3. Run CPU stress test in background (e.g., Prime95, Cinebench)
4. **Expected:** No dropouts or glitches (MMCSS protects audio thread)

Without MMCSS (disabled):
- Rebuild with `-DZENITH_ENABLE_MMCSS=OFF`
- Repeat test → likely to hear glitches under heavy load

### Test 3: MMCSS Service Availability

Check if MMCSS service is running:

```powershell
# Check MMCSS service status
Get-Service MMCSS

# Should show: Status = Running, StartType = Automatic
```

If MMCSS is stopped:
```powershell
# Start MMCSS service (admin required)
Start-Service MMCSS
```

If permanently disabled (some "optimization" guides recommend this - **BAD for audio work**):
```powershell
# Re-enable MMCSS (admin required)
Set-Service MMCSS -StartupType Automatic
Start-Service MMCSS
```

## Performance Impact

### Overhead

| Operation | Cost | Frequency |
|-----------|------|-----------|
| `AvSetMmThreadCharacteristicsW` | ~1-5 µs | Once per audio device start |
| Per-callback overhead | 0 µs | No runtime cost |

MMCSS registration happens **once** when the audio device starts, not in the audio callback loop.

### CPU vs. Dropout Trade-off

MMCSS prevents dropouts by ensuring audio threads always run, even under extreme load. However:

- **Pro:** Audio remains glitch-free
- **Con:** Other tasks may slow down slightly when audio is active
- **Note:** This is the correct trade-off for DAW work

## Advanced: MMCSS Registry Tuning (Optional)

For extreme low-latency scenarios (< 64 samples), you can tune MMCSS parameters:

**Registry location:**
```
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Multimedia\SystemProfile\Tasks\Pro Audio
```

| Key | Default | Low-Latency Tweak | Description |
|-----|---------|-------------------|-------------|
| `Priority` | 1 | 1 | Priority level (1 = highest) |
| `Scheduling Category` | High | High | Scheduler hint |
| `SFIO Priority` | High | High | System I/O priority |
| `GPU Priority` | 8 | 8 | GPU scheduling |

**WARNING:** Only modify these if you know what you're doing. Incorrect values can destabilize the system.

## Debugging MMCSS Issues

### Symptom: Audio glitches despite MMCSS

**Check 1: Is MMCSS actually active?**

Add debug output in Engine.cpp:

```cpp
if (audioPriority.isActive()) {
    DBG("MMCSS active - task index: " + String(audioPriority.getTaskIndex()));
} else {
    DBG("MMCSS FAILED!");
}
```

**Check 2: Is avrt.dll loaded?**

```powershell
# In x64 Native Tools Command Prompt
dumpbin /imports ZenithDAW.exe | findstr avrt
```

Should show `avrt.dll` if linked correctly.

**Check 3: Thread priorities**

Use Process Explorer (Sysinternals):
1. Find ZenithDAW.exe
2. Double-click → Threads tab
3. Look for thread with **Base Priority: 26-31** (real-time range)
4. That's your audio thread if MMCSS is working

### Symptom: MMCSS disabled despite ZENITH_ENABLE_MMCSS=ON

**Cause 1:** `avrt.dll` not found (Windows versions before Vista)
- **Solution:** MMCSS requires Windows Vista or later

**Cause 2:** MMCSS service disabled by user
- **Solution:** `Start-Service MMCSS` (PowerShell as admin)

**Cause 3:** Preprocessor define not set correctly
- **Check:** Look for `ZENITH_ENABLE_MMCSS=1` in compile commands:
  ```bash
  cmake --build build --verbose | grep ZENITH_ENABLE_MMCSS
  ```

## References

- [Microsoft: Multimedia Class Scheduler Service](https://learn.microsoft.com/en-us/windows/win32/procthread/multimedia-class-scheduler-service)
- [Microsoft: Exclusive-Mode Streams](https://learn.microsoft.com/en-us/windows/win32/coreaudio/exclusive-mode-streams)
- [JUCE WASAPI implementation](https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_devices/native/juce_win32_WASAPI.cpp) (reference)

## Summary

✅ **MMCSS is enabled by default in Zenith**
✅ **Zero runtime overhead** (registered once, not per-callback)
✅ **Prevents dropouts** under heavy CPU load
✅ **Configurable** at build time with `ZENITH_ENABLE_MMCSS`
✅ **Safe fallback** if MMCSS unavailable (no crashes)

For professional low-latency DAW work, **keep MMCSS enabled**.
