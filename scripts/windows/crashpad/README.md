# Crashpad Crash Reporting for Zenith DAW (W8)

**W8 Deliverable**: Optional crash capture using Google Crashpad (local minidumps, no uploads by default).

## Overview

Crashpad is Google's crash-reporting system that generates minidump files when an application crashes. Unlike traditional Windows Error Reporting (WER), Crashpad:

- **Captures rich crash context**: CPU registers, stack traces, loaded modules
- **Works out-of-process**: Crash handler runs separately, survives app crash
- **Cross-platform**: Same API for Windows, macOS, Linux
- **Privacy-first**: Local-only by default (no automatic uploads)

Zenith DAW uses Crashpad **optionally** and **locally** - minidumps are saved to disk for manual inspection, not sent to any server.

---

## Privacy & Data Handling

**What's in a minidump?**

- Process memory snapshot (stack, heap excerpts)
- CPU registers, thread contexts
- Loaded modules (DLLs) and their versions
- Exception information (crash address, type)
- **Application annotations** (version, build config)

**What's NOT in a minidump?**

- Full memory dump (only crash-relevant regions)
- File contents, project data, audio samples
- User credentials or personal data (unless actively in memory at crash)

**Default Behavior:**

- **Uploads: DISABLED** - Minidumps saved locally to `%APPDATA%\ZenithDAW\crashpad_db\completed\`
- **Retention: Unlimited** - Crashpad doesn't auto-delete dumps; user can clean manually
- **Opt-in: Required** - Crash reporting disabled by default; enable via settings

---

## Prerequisites

### 1. Install Debugging Tools for Windows (Optional, for dump analysis)

**WinDbg** is needed to open and analyze `.dmp` files with symbols.

**Option A: Microsoft Store (Recommended)**

1. Search "WinDbg" in Microsoft Store
2. Install "WinDbg Preview" (modern UI)

**Option B: Windows SDK**

1. Download Windows SDK: https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
2. During install, select **only** "Debugging Tools for Windows"

### 2. Build or Download Crashpad Handler

Crashpad requires `crashpad_handler.exe` at runtime. Two options:

**Option A: Use Pre-built Handler (Easiest)**

1. Download from BugSplat or community builds:
   - https://github.com/bugsplat-git/bugsplat-crashpad/releases
   - Extract `crashpad_handler.exe` (x64 Release)

2. Place in: `scripts\windows\crashpad\bin\crashpad_handler.exe`

**Option B: Build from Source (Advanced)**

Use the provided script (requires Visual Studio 2022, ~1 hour build time):

```cmd
scripts\windows\crashpad\fetch_build_crashpad.cmd
```

This will:
- Clone `depot_tools` and Crashpad source (~500 MB)
- Build `crashpad_handler.exe` with VS 2022 (x64 Release)
- Copy handler to `scripts\windows\crashpad\bin\`
- Echo absolute path for CMake configuration

---

## Enabling Crash Reporting

### Step 1: Configure CMake

When building Zenith DAW, enable Crashpad:

```cmd
cmake -B build -DZENITH_USE_CRASHPAD=ON -DCRAS HPAD_HANDLER_PATH="C:\path\to\crashpad_handler.exe"
```

**Important**: `CRASHPAD_HANDLER_PATH` must be **absolute path** to where `crashpad_handler.exe` will be at **runtime**.

Recommended runtime locations:
- Development: `<repo>\scripts\windows\crashpad\bin\crashpad_handler.exe`
- Installed: `C:\Program Files\Zenith DAW\crashpad_handler.exe` (ship with installer)

### Step 2: Enable in Application Settings

Crash reporting is **disabled by default** even when compiled with `ZENITH_USE_CRASHPAD=ON`.

Enable via:
1. **Debug Menu** (Debug builds): `Help → Diagnostics → Enable Crash Reports`
2. **Settings File** (manual): Set `diagnostics.crashReportsEnabled=true` in `ZenithDAW.settings`

Settings file location:
- Windows: `%APPDATA%\ZenithDAW\ZenithDAW.settings`
- macOS: `~/Library/Application Support/ZenithDAW/ZenithDAW.settings`
- Linux: `~/.config/ZenithDAW/ZenithDAW.settings`

### Step 3: Test Crash Capture (Debug Only)

**Debug builds** include a crash test trigger:

1. Launch Zenith DAW (Debug build with Crashpad enabled)
2. Menu: `Help → Diagnostics → Trigger Test Crash`
3. App crashes (intentional null pointer dereference)
4. Check for `.dmp` file in `%APPDATA%\ZenithDAW\crashpad_db\completed\`

**Release builds** do NOT include crash test trigger (safety).

---

## Crash Dump Locations

Crashpad creates the following directory structure:

```
%APPDATA%\ZenithDAW\crashpad_db\
├── completed\          <- Uploaded dumps (none, since uploads disabled)
├── pending\            <- Dumps waiting for upload (will move to completed)
├── new\                <- Newly created dumps (immediately processed)
└── settings.dat        <- Crashpad configuration
```

**Typical dump file name**: `<UUID>.dmp` (e.g., `a1b2c3d4-e5f6-7890-abcd-ef1234567890.dmp`)

---

## Analyzing Crash Dumps

### Quick Open with Provided Script

```cmd
scripts\windows\crashpad\open_latest_dump.cmd
```

This finds the newest `.dmp` in `crashpad_db\completed\` and launches WinDbg.

### Manual Analysis with WinDbg

1. **Open WinDbg** (WinDbg Preview from Microsoft Store)

2. **Configure Symbol Path**:
   - File → Settings → Debugging Settings → Symbol Path
   - Add: `SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols`
   - Add local PDB path: `<repo>\zenith-core\build\Debug` (or Release)

3. **Open Dump File**:
   - File → Open Dump File → select `.dmp`

4. **Analyze Crash**:
   ```
   !analyze -v
   ```
   This shows:
   - Faulting module/function
   - Exception code (e.g., `0xc0000005` = Access Violation)
   - Call stack with symbols

5. **View Call Stack**:
   ```
   k
   ```
   Shows function call chain leading to crash.

6. **Examine Variables** (if symbols loaded):
   ```
   dv
   ```
   Shows local variables at crash site.

### Sharing Dumps for Support

**Before sharing a `.dmp` file**:

1. **Check for sensitive data**: Minidumps may contain memory snapshots
2. **Strip identifying info** (optional): Use `minidumpwrite` tool to filter
3. **Share via secure channel**: Email, encrypted file share, private GitHub issue

**What to include**:
- Minidump file (`*.dmp`)
- Application version/build info
- Steps to reproduce (if known)
- System info (OS version, CPU, RAM)

---

## Disabling Crash Reporting

### Temporarily (User Setting)

Uncheck: `Help → Diagnostics → Enable Crash Reports`

### Permanently (Rebuild Without Crashpad)

Rebuild with CMake:

```cmd
cmake -B build -DZENITH_USE_CRASHPAD=OFF
```

This removes all Crashpad code (`#if ZENITH_USE_CRASHPAD` blocks compiled out).

### Clean Up Old Dumps

Manually delete:

```cmd
rmdir /s /q "%APPDATA%\ZenithDAW\crashpad_db"
```

---

## Advanced: Enabling Uploads (Future)

**Current default**: Uploads disabled (`SetUploadsEnabled(false)`).

**To enable uploads** (requires backend server):

1. Modify `CrashpadInit.cpp`:
   ```cpp
   settings->SetUploadsEnabled(true); // Change false → true
   ```

2. Set upload URL:
   ```cpp
   std::string uploadURL = "https://your-crash-server.com/upload";
   // Pass to StartHandler(..., uploadURL, ...)
   ```

3. Deploy crash ingestion server (e.g., BugSplat, Sentry, self-hosted Crashpad receiver)

**Privacy Note**: If enabling uploads, update privacy policy and get user consent.

---

## Troubleshooting

### Handler Not Found (`crashpad_handler.exe` missing)

**Symptom**: DBG log: `Crashpad: handler missing`

**Fix**:
1. Verify `CRASHPAD_HANDLER_PATH` in CMake cache
2. Check file exists at specified path
3. Rebuild with correct `-DCRAS HPAD_HANDLER_PATH=...`

### No `.dmp` Files After Crash

**Possible Causes**:

1. **Crash reporting disabled** in settings
   - Check: `diagnostics.crashReportsEnabled` in `ZenithDAW.settings`

2. **Handler failed to start**
   - Check DBG log: `Crashpad StartHandler: OK` vs `FAIL`
   - Ensure handler path is absolute, not relative

3. **Database directory not writable**
   - Verify `%APPDATA%\ZenithDAW\crashpad_db\` exists and is writable

4. **SEH exception not caught** (rare)
   - Crashpad catches most crashes, but some (e.g., stack overflow, heap corruption) may bypass

### WinDbg Shows "Symbols Not Found"

**Fix**:

1. **Configure symbol path**:
   - `.sympath SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols`
   - `.reload`

2. **Add local PDB path**:
   - `.sympath+ <repo>\zenith-core\build\Debug`
   - `.reload /f ZenithDAW.exe`

3. **Verify PDB generation**:
   - CMake builds with `/Zi` (Debug info) and `/DEBUG:FULL` (PDB)
   - Check `build\Debug\ZenithDAW.pdb` exists

### Build Errors (Missing Crashpad Headers)

**Symptom**: `fatal error: client/crashpad_client.h: No such file or directory`

**Cause**: `ZENITH_USE_CRASHPAD=ON` but Crashpad not available.

**Fix Options**:

1. **Disable Crashpad**: Build with `-DZENITH_USE_CRASHPAD=OFF`

2. **Install Crashpad**:
   - Run `fetch_build_crashpad.cmd` (builds locally)
   - OR use vcpkg/conan package manager (if available)

3. **Set include path** (if Crashpad installed elsewhere):
   ```cmake
   target_include_directories(ZenithDAW PRIVATE /path/to/crashpad/include)
   ```

---

## File Reference

```
scripts/windows/crashpad/
├── README.md                        <- This file
├── fetch_build_crashpad.cmd         <- Build script (depot_tools + gn/ninja)
├── open_latest_dump.cmd             <- Launch WinDbg on newest dump
└── bin/
    └── crashpad_handler.exe         <- Runtime handler (x64 Release)

%APPDATA%\ZenithDAW\
├── ZenithDAW.settings               <- App settings (crash reports toggle)
└── crashpad_db/                     <- Crash database
    ├── completed/*.dmp              <- Processed minidumps
    ├── pending/*.dmp                <- Queued for upload (none if disabled)
    └── settings.dat                 <- Crashpad config
```

---

## Further Reading

- **Crashpad Documentation**: https://chromium.googlesource.com/crashpad/crashpad/+/HEAD/doc/
- **WinDbg Tutorial**: https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/
- **Minidump Format**: https://learn.microsoft.com/en-us/windows/win32/debug/minidump-files
- **BugSplat Crashpad Guide**: https://docs.bugsplat.com/introduction/getting-started/integrations/cross-platform/crashpad

---

**Note**: Crashpad is optional and disabled by default. Enable only if you need crash diagnostics. For production releases, consider privacy implications and user consent before enabling uploads.
