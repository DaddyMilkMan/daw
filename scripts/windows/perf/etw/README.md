# ETW (Event Tracing for Windows) Performance Analysis for Zenith DAW

**W7 Deliverable**: Deep system-level performance tracing using Windows Performance Toolkit.

## Overview

Event Tracing for Windows (ETW) is a kernel-level tracing facility that lets you log kernel or application-defined events to a log file. ETW provides deep visibility into CPU usage, context switches, DPC/ISR latency, GPU rendering, disk I/O, and audio subsystem behavior.

Unlike W6's HUD (which measures application-level paint() timing) or PresentMon (which measures Present() API latency), ETW captures **system-wide** events from the Windows kernel and user-mode providers, enabling analysis of:

- **CPU**: Per-thread sampling, context switches, scheduling delays
- **GPU**: DWM composition, DXGI present chains, GPU utilization
- **Audio**: WASAPI glitches, AudioSes buffer underruns, MMCSS priority inversions
- **Disk/Memory**: Page faults, file I/O, memory allocations

## Prerequisites

### Install Windows Performance Toolkit

**Option 1: Windows ADK (Recommended)**

1. Download Windows Assessment and Deployment Kit (ADK):
   ```
   https://learn.microsoft.com/en-us/windows-hardware/get-started/adk-install
   ```

2. Run installer, select **only** "Windows Performance Toolkit"

3. Verify installation:
   ```cmd
   wpr /?
   wpa /?
   ```

**Option 2: Standalone (Windows 11)**

Windows 11 includes WPR in the OS. Download WPA separately from Microsoft Store:
- Search "Windows Performance Analyzer" in Microsoft Store

### Requirements

- **Windows 10 1809+** or **Windows 11**
- **Administrator privileges** (ETW requires kernel access)
- **10+ GB free disk** (traces can be large, esp. with GPU providers)
- **WPA Symbols**: Configure symbol path for call stacks (see below)

---

## Quick Start

### 1. Capture a Trace

Use the provided script with one of 4 presets:

```cmd
REM Basic debugging (30 seconds, ~500 MB)
scripts\windows\perf\etw\run_etw_capture.cmd debug-light 30

REM Audio glitch hunting (continuous until Ctrl+C, ~2 GB/min)
scripts\windows\perf\etw\run_etw_capture.cmd glitch-hunt

REM GPU/UI performance (60 seconds, ~1 GB)
scripts\windows\perf\etw\run_etw_capture.cmd gpu-ui 60

REM Full audio stack analysis (30 seconds, ~1.5 GB)
scripts\windows\perf\etw\run_etw_capture.cmd audio-stack 30
```

**Output**: Traces are saved to `logs\etw\YYYYMMDD_HHMMSS\session.etl`

### 2. Open in WPA

1. Launch Windows Performance Analyzer (WPA)
2. **File → Open** → select `session.etl`
3. **Profiles → Browse** → load `scripts\windows\perf\etw\wpa_starter_profile.wpaProfile`
4. Analyze graphs (see "WPA Analysis" section below)

---

## Capture Presets

The `zenith_etw.wprp` profile defines 4 presets tailored to different scenarios:

### `debug-light` - General Debugging

**Use Case**: First-pass performance investigation, startup profiling, CPU hotspots

**Providers**:
- Kernel: CPU sampling (8 kHz), CSwitch, DPC/ISR
- User: DWM, DXGI (Present), Win32k
- Buffer: 256 MB (circular)

**Expected Size**: ~500 MB for 30s capture

**When to Use**:
- App feels sluggish, want to find CPU bottlenecks
- Investigating startup time
- General "what's the system doing?" overview

---

### `glitch-hunt` - Audio Dropouts & Glitches

**Use Case**: Debugging audio dropouts, clicks, pops, buffer underruns

**Providers**:
- Kernel: CPU sampling (8 kHz), CSwitch, DPC/ISR (high priority)
- Audio: WASAPI, AudioSes, AudioKSE, MMCSS
- Buffer: 512 MB (circular)

**Expected Size**: ~2 GB/min (high-frequency audio events)

**When to Use**:
- Hearing clicks/pops during playback
- ASIO/WASAPI buffer underruns reported
- Audio dropouts when system is busy
- Investigating MMCSS priority boost effectiveness (W2 validation)

**Pro Tip**: Use W6 HUD timestamps to correlate glitches. If HUD shows spike at `T+15.234s`, zoom to that timestamp in WPA.

---

### `gpu-ui` - GPU/DWM/Present Performance

**Use Case**: UI jank, frame pacing issues, DWM composition delays

**Providers**:
- Kernel: CPU sampling, CSwitch, DPC/ISR
- GPU: DWM, DXGI, Win32k, DxgKrnl (DirectX kernel)
- Buffer: 512 MB (circular)

**Expected Size**: ~1 GB for 60s capture

**When to Use**:
- UI repaints stutter despite low paint times (W6 HUD)
- PresentMon shows high `msInPresentAPI` or `msUntilDisplayed` (W6 cross-ref)
- Suspected GPU driver overhead or DWM composition stalls
- VSync/tearing issues

---

### `audio-stack` - Full Audio Subsystem

**Use Case**: Comprehensive audio stack analysis (kernel → driver → WASAPI → app)

**Providers**:
- Kernel: CPU sampling, CSwitch, DPC/ISR, Disk I/O
- Audio: WASAPI, AudioSes, AudioKSE, MMCSS, Audio (driver-level)
- User: DWM (for A/V sync analysis)
- Buffer: 1024 MB (circular)

**Expected Size**: ~1.5 GB for 30s capture

**When to Use**:
- Deep-dive audio architecture investigation
- Driver-level glitch analysis (ASIO vs WASAPI comparison)
- A/V sync issues (correlate with GPU timeline)
- MMCSS vs non-MMCSS thread priority comparison

---

## WPA Analysis Workflow

### Symbol Configuration (First Time Only)

1. **WPA → Trace → Configure Symbol Paths**
2. Set symbol path:
   ```
   SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols
   ```
3. **Load Symbols**: Trace → Load Symbols (wait ~1-5 min for first load)

### Recommended Graphs (Starter Profile)

The provided `wpa_starter_profile.wpaProfile` includes:

#### CPU Analysis

1. **CPU Usage (Sampled) - Attributed**
   - Group by Process → Thread → Stack
   - Find `ZenithDAW.exe` → expand threads
   - Look for hot functions (high % Weight)

2. **CPU Usage (Precise)**
   - Shows exact time on/off CPU (not sampled)
   - Group by Process → Thread → Waits
   - Identify blocking (I/O, synchronization, page faults)

3. **Context Switches**
   - Group by Process → Thread → Ready Thread Stack
   - High context switch rate = thread contention or priority inversion

#### DPC/ISR Latency

4. **DPC/ISR Duration**
   - Filter to `Duration > 1ms` (high latency)
   - Group by Module (driver), Function
   - Audio glitches often correlate with ISR storms

#### GPU/Present

5. **GPU / DWM**
   - Present timeline: DXGI → DWM composition → Display
   - Group by Process → Present Type
   - Cross-reference with PresentMon CSV (W6)

6. **Frame Analysis (DXGI Present)**
   - Shows frame-by-frame present intervals
   - Identify stutter (irregular spacing)

#### Audio

7. **WASAPI / AudioSes**
   - Buffer underruns, glitches, latency spikes
   - Filter by `ZenithDAW.exe` process
   - Look for `GlitchEvent` or `UnderrunEvent` markers

8. **MMCSS Priority Boost**
   - CPU Usage → Thread Priority column
   - Verify audio threads running at elevated priority (W2 validation)
   - Compare with non-MMCSS threads

### Cross-Reference with W6 HUD

**Scenario**: HUD shows frame spike at `T+15.234s`, paint time jumped from 2ms → 18ms

1. Note timestamp from W6 HUD overlay
2. In WPA, zoom to `T+15.2s` → `T+15.3s`
3. Check graphs:
   - CPU Sampling: Was `paint()` function hot?
   - DPC/ISR: Any driver interrupts during that window?
   - GPU: DWM composition delay?
   - Disk: Page fault or I/O stall?

**Example**: You might find a network driver DPC took 12ms, blocking the UI thread.

---

## Manual Capture (Advanced)

If you need custom provider configurations:

### Start Recording

```cmd
REM Use one of the presets from zenith_etw.wprp
wpr -start scripts\windows\perf\etw\zenith_etw.wprp!debug-light

REM Or use built-in profiles
wpr -start CPU -start DiskIO
```

### Stop & Save

```cmd
REM Stop and save to file
wpr -stop output.etl

REM Cancel without saving
wpr -cancel
```

### Validate Profile

```cmd
REM Check WPRP syntax
wpr -validate scripts\windows\perf\etw\zenith_etw.wprp
```

---

## ETW Provider Reference

### Kernel Providers (in `zenith_etw.wprp`)

| Provider | Keyword/Stack | Purpose |
|----------|---------------|---------|
| `CpuConfig` | | CPU topology, frequency scaling |
| `Loader` | | Module load events (DLL loading) |
| `ProcessThread` | `WINEVENT_KEYWORD_PROCESS`<br>`WINEVENT_KEYWORD_THREAD` | Process/thread creation/exit |
| `CSwitch` | `WINEVENT_KEYWORD_CSWITCH`<br>Stack: `CSwitch` | Context switches (critical for glitch hunting) |
| `Dispatcher` | `WINEVENT_KEYWORD_DISPATCHER` | Scheduler ready/wait events |
| `SampledProfile` | Stack: `SampledProfile` | CPU sampling (8 kHz default) |
| `DPC` | Stack: `Dpc` | Deferred Procedure Calls (driver latency) |
| `ISR` | Stack: `Isr` | Interrupt Service Routines (driver latency) |
| `DiskIo` | `WINEVENT_KEYWORD_DISKIO` | Disk I/O (file access, page faults) |

### User-Mode Providers (Selected)

| Provider Name | GUID | Purpose |
|---------------|------|---------|
| `Microsoft-Windows-Dwm-Core` | `{9e9bba3c-2e38-40cb-99f4-9e8281425164}` | Desktop Window Manager composition |
| `Microsoft-Windows-DXGI` | `{ca11c036-0102-4a2d-a6ad-f03cfed5d3c9}` | DirectX Graphics Infrastructure (Present) |
| `Microsoft-Windows-Win32k` | `{8c416c79-d49b-4f01-a467-e56d3aa8234c}` | Win32 kernel (window messages, GDI) |
| `Microsoft-Windows-Audio` | `{ae4bd3be-f36f-45b6-8d21-bdd6fb832853}` | Audio driver events |
| `Microsoft-Windows-AudioSes` | `{0c38e9f0-f748-4fb5-aa6b-ea0fd8f73832}` | Windows Audio Session (WASAPI) |
| `Microsoft-Windows-Kernel-Processor-Power` | `{0f67e49f-fe51-4e9f-b490-6f2948cc6027}` | CPU frequency, C-states |
| `Microsoft-Windows-MMCSS` | `{f8f10121-b617-4a56-868b-9df1b27fe32c}` | Multimedia Class Scheduler Service |

---

## Output & Analysis

### Trace File Structure

```
logs\etw\
└── 20251112_143022\          (YYYYMMDD_HHMMSS)
    ├── session.etl          (main trace file)
    ├── session.etl.ngenpdb  (auto-generated symbol cache)
    └── providers.txt        (list of providers used)
```

### Typical Trace Sizes

| Preset | Duration | Size | Notes |
|--------|----------|------|-------|
| `debug-light` | 30s | ~500 MB | General CPU/GPU |
| `glitch-hunt` | 60s | ~2 GB | High-frequency audio events |
| `gpu-ui` | 30s | ~800 MB | DWM + Present chains |
| `audio-stack` | 30s | ~1.5 GB | Full audio subsystem |

**Circular Buffers**: Traces use circular (ring) buffers. If duration exceeds buffer size, oldest events are overwritten. For long captures, increase buffer sizes in `.wprp` file.

---

## Privacy & Data Handling

**What's in an ETL file?**

- Process names, module names (DLLs), function names (with symbols)
- Thread IDs, CPU times, context switch timing
- File paths accessed (if DiskIO enabled)
- **NO** file contents, passwords, or user data

**Sharing Traces**:

- ETL files may contain system-wide data (other running processes)
- Strip sensitive data before sharing: Use WPA → Export → Filtered trace
- Only share with trusted developers/Microsoft support

---

## Troubleshooting

### "Access Denied" when starting WPR

**Fix**: Run Command Prompt **as Administrator**

### "Failed to start: 0x800700B7"

**Cause**: WPR session already running

**Fix**:
```cmd
wpr -cancel
wpr -start ...
```

### Huge Trace Files (>10 GB)

**Cause**: Long capture duration with large buffers

**Fix**:
- Reduce capture duration (use `run_etw_capture.cmd <preset> <seconds>`)
- Use smaller buffer sizes (edit `.wprp` file)
- Use more focused presets (`debug-light` instead of `audio-stack`)

### WPA Shows No Symbols (Empty Stacks)

**Cause**: Symbol path not configured

**Fix**:
1. WPA → Trace → Configure Symbol Paths
2. Set: `SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols`
3. Trace → Load Symbols (wait for download)

### Audio Glitches Not Visible in Trace

**Cause**: Audio providers not enabled, or buffer too small

**Fix**:
- Use `glitch-hunt` or `audio-stack` preset (includes WASAPI/AudioSes)
- Increase buffer size if long capture needed (edit `.wprp` `<BufferSize>` in KB)

### WPA Crashes or Hangs on Large Traces

**Cause**: Insufficient RAM (WPA loads entire trace into memory)

**Fix**:
- Close other applications
- Use filtered trace export (WPA → Select Time Range → Export)
- Capture shorter duration

---

## Integration with W6 HUD & PresentMon

Zenith DAW provides **three complementary performance tools**:

| Tool | Layer | Metrics | Best For |
|------|-------|---------|----------|
| **W6 HUD**<br>(in-app overlay) | Application | paint() CPU time<br>Visible clip count<br>Paints/sec | UI virtualization<br>Rendering bottlenecks<br>Real-time monitoring |
| **PresentMon**<br>(external, W6 docs) | GPU/Driver | Present() API latency<br>GPU render time<br>Display sync | Driver overhead<br>VSync/tearing<br>GPU bottlenecks |
| **ETW (W7)**<br>(system-wide) | Kernel + User | CPU scheduling<br>DPC/ISR latency<br>Audio glitches<br>Full system context | Root-cause analysis<br>Audio dropouts<br>Priority inversion<br>Driver issues |

### Example Workflow

1. **W6 HUD**: Notice frame spike at `T+42.5s` (paint time 2ms → 25ms)
2. **PresentMon CSV**: Check if `msInPresentAPI` spiked (GPU driver stall?)
3. **ETW**: Capture with `debug-light`, zoom to `T+42.5s` in WPA
4. **Result**: DPC from network driver took 20ms, blocking UI thread

---

## Further Reading

- **Microsoft Docs - WPR**: https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-recorder
- **Microsoft Docs - WPA**: https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-analyzer
- **Authoring WPRP Profiles**: https://learn.microsoft.com/en-us/windows-hardware/test/wpt/authoring-recording-profiles
- **ETW Provider GUIDs**: https://github.com/repnz/etw-providers-docs
- **Microsoft Audio Team - Logging**: https://github.com/microsoft/audio (reference for audio ETW providers)
- **Bruce Dawson's Blog**: https://randomascii.wordpress.com/ (ETW expert, performance analysis)

---

## Support & Contribution

**Issues**: Report ETW profile bugs or WPA profile improvements at:
- GitHub: https://github.com/DaddyMilkMan/daw/issues

**Contributing**:
- Share improved WPA profiles (`.wpaProfile` files)
- Submit additional capture presets for specific scenarios
- Document analysis workflows for common issues

---

**Note**: ETW tracing is a powerful but complex tool. Start with `debug-light` preset and WPA starter profile. Consult Microsoft docs for advanced provider configuration.
