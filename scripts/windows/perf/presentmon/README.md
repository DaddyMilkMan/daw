# PresentMon Performance Monitoring for Zenith DAW

**W6 Deliverable**: External frame-time capture using Intel PresentMon.

## Overview

PresentMon is Intel's performance monitoring tool that captures high-level graphics performance characteristics on Windows. It provides frame timing, frame rates, and GPU/CPU performance metrics for DirectX, OpenGL, and Vulkan applications.

## Prerequisites

- **Windows 10 1809+** or **Windows 11** (required for modern ETW tracing)
- **Administrator privileges** (PresentMon requires elevated access)
- **ZenithDAW.exe** running (process name must match)

## Installation

### Option 1: Download Pre-built Binaries (Recommended)

1. Visit the official PresentMon releases page:
   ```
   https://github.com/GameTechDev/PresentMon/releases
   ```

2. Download the latest `PresentMon-<version>-x64.exe` or `PresentMon-<version>.zip`

3. Extract to a known location (e.g., `C:\Tools\PresentMon\`)

4. **DO NOT** commit binaries to this repository - PresentMon is externally maintained

### Option 2: Build from Source

```bash
git clone https://github.com/GameTechDev/PresentMon.git
cd PresentMon
# Follow build instructions in PresentMon/README.md
```

## Usage

### Quickstart: Capture Zenith DAW Frame Times

Run the provided batch script (requires PresentMon.exe in PATH or same directory):

```cmd
run_presentmon_example.cmd
```

This will:
- Create `presentmon_logs\` directory if it doesn't exist
- Capture per-frame metrics for `ZenithDAW.exe`
- Output to `presentmon_logs\zenith_frame_times.csv`
- Stream live output to console

**To stop capture**: Press `Ctrl+C` in the console

### Manual Command

```cmd
PresentMon.exe --process_name ZenithDAW.exe ^
               --output presentmon_logs\zenith_frame_times.csv ^
               --no_csv_summary ^
               --qpc_time ^
               --scroll_output
```

### Command Flags Explained

| Flag | Purpose |
|------|---------|
| `--process_name ZenithDAW.exe` | Capture only Zenith DAW (not entire system) |
| `--output <path>` | CSV output file path |
| `--no_csv_summary` | Skip summary stats file (only raw frame data) |
| `--qpc_time` | Use QueryPerformanceCounter timestamps (high-resolution) |
| `--scroll_output` | Show live frame metrics in console |
| `--terminate_after_timed <seconds>` | Auto-stop after N seconds (optional) |
| `--timed <seconds>` | Capture duration in seconds (optional) |

### Additional Useful Flags

```cmd
# Capture for 60 seconds only
--terminate_after_timed 60

# Include GPU/CPU metrics (if supported)
--include_mixed_reality
--include_compute

# Filter to specific window title
--window_title "Zenith DAW - Untitled Project"

# Verbose output for debugging
--verbose
```

## Output CSV Format

PresentMon generates CSV files with the following key columns:

### Critical Columns for Analysis

| Column | Description | Unit |
|--------|-------------|------|
| `TimeInSeconds` | Absolute timestamp | seconds |
| `msBetweenPresents` | Frame time (1/FPS) | milliseconds |
| `msInPresentAPI` | Time in Present() call | milliseconds |
| `msBetweenDisplayChange` | Display refresh interval | milliseconds |
| `msUntilRenderComplete` | GPU render time | milliseconds |
| `msUntilDisplayed` | Total latency to screen | milliseconds |

### Example CSV Header

```csv
Application,ProcessID,SwapChainAddress,Runtime,SyncInterval,PresentFlags,AllowsTearing,PresentMode,TimeInSeconds,msBetweenPresents,msInPresentAPI,msBetweenDisplayChange,msUntilRenderComplete,msUntilDisplayed
ZenithDAW.exe,12345,0x0000ABCD,DXGI,1,0,0,Hardware: Legacy Flip,1.234,16.67,0.12,16.67,2.34,18.91
```

## Analysis

### Quick Performance Check (PowerShell)

```powershell
# Calculate average FPS
$csv = Import-Csv presentmon_logs\zenith_frame_times.csv
$avgFrameTime = ($csv.msBetweenPresents | Measure-Object -Average).Average
$avgFps = 1000 / $avgFrameTime
Write-Host "Average FPS: $avgFps"
```

### Excel / CapFrameX Analysis

1. Open `presentmon_logs\zenith_frame_times.csv` in Excel
2. Create pivot table on `msBetweenPresents` column
3. Calculate:
   - **Average FPS**: `1000 / AVERAGE(msBetweenPresents)`
   - **1% Low FPS**: `1000 / PERCENTILE(msBetweenPresents, 0.99)`
   - **Frametime variance**: `STDEV(msBetweenPresents)`

### Ideal Metrics for 60Hz DAW

| Metric | Target | Notes |
|--------|--------|-------|
| Average FPS | ≥60 | Smooth UI responsiveness |
| msBetweenPresents | ≤16.67ms | 60fps frame budget |
| msInPresentAPI | <1ms | Low driver overhead |
| 1% Low FPS | ≥50 | No severe stutters |
| Frametime variance | <2ms | Consistent frame pacing |

## Common Issues

### "No process found matching name"

- Ensure `ZenithDAW.exe` is actually running
- Check Task Manager for exact process name (case-sensitive on some systems)
- Try `--process_id <PID>` instead of `--process_name`

### "Access Denied"

- Run Command Prompt/PowerShell **as Administrator**
- Windows Defender/antivirus may block ETW tracing

### "Missing DLL" or "Entry Point Not Found"

- Download latest Visual C++ Redistributables:
  https://aka.ms/vs/17/release/vc_redist.x64.exe

### High Overhead / Performance Impact

- PresentMon adds <1% overhead in most cases
- If seeing impact, remove `--scroll_output` and `--verbose` flags

## Integration with W6 HUD

The in-app stats overlay (W6) shows **real-time** paint() metrics (unavailable externally). PresentMon captures **system-level** present timing (GPU/driver overhead). Use both for complete picture:

| Tool | Measures | Use Case |
|------|----------|----------|
| **W6 HUD** | TrackView paint cost, visible clip count | Debug UI virtualization, CPU-side rendering |
| **PresentMon** | Present() latency, GPU render time, display sync | Debug driver stalls, VSync issues, GPU bottlenecks |

## Further Reading

- **PresentMon GitHub**: https://github.com/GameTechDev/PresentMon
- **Official Docs**: https://github.com/GameTechDev/PresentMon/blob/main/README-CaptureApplication.md
- **TechSpot Tutorial**: https://www.techspot.com/article/2723-intel-presentmon/
- **Intel Gaming Access**: https://game.intel.com/us/intel-presentmon/

## Troubleshooting Checklist

- [ ] PresentMon.exe in PATH or local directory?
- [ ] Running as Administrator?
- [ ] ZenithDAW.exe process actually running?
- [ ] Output directory exists and writable?
- [ ] Visual C++ Redistributables installed?
- [ ] Antivirus not blocking ETW tracing?

---

**Note**: PresentMon is an external tool maintained by Intel. Do not commit PresentMon binaries to this repository. Users must download it separately.
