# Crash Capture & Reporting

Zenith DAW includes optional crash reporting powered by **Crashpad** (Google's out-of-process crash capture system). When enabled, crashes generate minidump files that can be analyzed with debugging tools to diagnose issues.

---

## Privacy-First Approach

Crash reporting is **disabled by default** and requires explicit user opt-in:

- **Local-only storage**: Crash dumps are stored on your computer, never uploaded automatically
- **No telemetry**: No analytics, tracking, or remote reporting
- **User control**: Enable/disable at any time via menu
- **Transparent**: You can inspect and delete crash dumps at any time

---

## Enabling Crash Reports

**Important**: This feature is only available when Zenith DAW is built with Crashpad support (`-DZENITH_USE_CRASHPAD=ON`).

### To enable crash reporting:

1. Open Zenith DAW
2. Navigate to **Help → Diagnostics → Enable Crash Reports**
3. Restart the application

### To disable crash reporting:

1. Navigate to **Help → Diagnostics → Enable Crash Reports** (uncheck)
2. Restart the application

---

## What's Collected in Crash Dumps

When the application crashes, Crashpad captures a **minidump** (`.dmp` file) containing:

### Technical Information (Always Included):
- **CPU registers**: Processor state at time of crash
- **Call stack**: Function call history leading to crash
- **Thread information**: All threads and their states
- **Module list**: Loaded DLLs/executables with versions
- **Exception details**: Crash type (access violation, divide by zero, etc.)
- **Memory regions**: Stack memory, limited heap samples

### Metadata (Annotations):
- Product name: "Zenith DAW"
- Version: "0.1.0"
- Build type: "Debug" or "Release"
- Platform: "Windows"

### What's NOT Collected:
- **Audio data**: No audio buffers or project audio files
- **Project files**: No session data or user projects
- **Personal files**: No documents, downloads, or file system access
- **Network data**: No browsing history or network activity
- **User input**: No passwords, text input, or keyboard history

---

## Crash Dump Locations

### Windows:
```
%APPDATA%\ZenithDAW\crashpad_db\
```

Typical path:
```
C:\Users\<YourUsername>\AppData\Roaming\ZenithDAW\crashpad_db\
```

### Folder Structure:
```
crashpad_db/
├── completed/           ← Finalized crash dumps (.dmp files)
├── pending/             ← In-progress captures
└── settings.dat         ← Crashpad database metadata
```

### Finding Your Crash Dumps:
1. Press `Win + R`
2. Type: `%APPDATA%\ZenithDAW\crashpad_db\completed`
3. Press Enter
4. Look for files with `.dmp` extension (sorted by date)

---

## Testing Crash Reporting (Debug Builds Only)

Debug builds include a test crash trigger to verify crash reporting is working:

### To trigger a test crash:

1. Enable crash reporting (see above)
2. Navigate to **Help → Diagnostics → Trigger Test Crash** (DEBUG menu only)
3. Confirm the warning dialog
4. Application will crash immediately
5. Check `%APPDATA%\ZenithDAW\crashpad_db\completed\` for new `.dmp` file

**Note**: Test crash is not available in Release builds (safety precaution).

---

## Analyzing Crash Dumps

### Prerequisites:
- **WinDbg** (Windows Debugger) - [Download from Microsoft Store](https://apps.microsoft.com/store/detail/windbg/9PGJGD53TN86) or Windows SDK
- **Symbol files** (`.pdb`) - Generated during build (see `build/` directory)

### Quick Analysis with Helper Script:

```cmd
cd scripts\windows\crashpad
open_latest_dump.cmd
```

This script automatically:
- Finds the newest crash dump
- Launches WinDbg
- Configures symbol paths

### Manual Analysis:

1. Open WinDbg
2. File → Open Crash Dump → Select `.dmp` file
3. Configure symbols (first time only):
   ```
   .sympath SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols
   .sympath+ C:\path\to\your\build\directory
   .reload
   ```
4. Run automatic analysis:
   ```
   !analyze -v
   ```

### Reading the Output:

Look for these sections in the analysis:
- **EXCEPTION_RECORD**: Crash type (e.g., `c0000005` = access violation)
- **FAULTING_MODULE**: Which DLL/executable crashed
- **FAULTING_IP**: Instruction pointer at crash
- **STACK_TEXT**: Call stack leading to crash
- **SYMBOL_NAME**: Function name where crash occurred

---

## Sharing Crash Dumps Privately

If you need to share a crash dump for debugging:

### Option 1: Share with Developer Directly
1. Locate `.dmp` file in `crashpad_db\completed\`
2. Compress with 7-Zip/WinRAR (minidumps are ~50-500 KB compressed)
3. Send via email or private file share (Google Drive, Dropbox, etc.)

### Option 2: Anonymize Before Sharing
Minidumps contain:
- File paths (e.g., `C:\Users\<YourUsername>\...`)
- Environment variables (may include username)

**To anonymize**:
- Use WinDbg to export only stack traces (no raw memory):
  ```
  !analyze -v > analysis.txt
  ~* kb > stacks.txt
  ```
- Share `.txt` files instead of `.dmp` (no personal data)

### What to Include with Reports:
- **Build info**: Version, Debug/Release, build date
- **Reproduction steps**: What were you doing when it crashed?
- **Frequency**: Does it crash every time or randomly?
- **System specs**: Windows version, CPU, RAM, audio interface

---

## Troubleshooting

### "Enable Crash Reports" menu item not visible
- **Cause**: Zenith was not built with Crashpad support
- **Solution**: Rebuild with CMake flags:
  ```cmd
  cmake -B build -DZENITH_USE_CRASHPAD=ON -DCRAS HPAD_HANDLER_PATH="C:\path\to\crashpad_handler.exe"
  cmake --build build --config Debug
  ```

### No crash dump created after crash
1. **Verify crash reporting is enabled**:
   - Check: Help → Diagnostics → Enable Crash Reports (should be checked)
   - Restart application after enabling

2. **Check handler is running**:
   - Open Task Manager after launching Zenith
   - Look for `crashpad_handler.exe` process
   - If missing, check CMake `CRASHPAD_HANDLER_PATH` is correct

3. **Check database directory exists**:
   - Navigate to `%APPDATA%\ZenithDAW\crashpad_db\`
   - If missing, Crashpad init may have failed (check debug logs)

4. **Verify handler path**:
   - Debug builds log handler path on startup:
     ```
     W8: Crashpad initialized successfully
       Handler: C:\path\to\crashpad_handler.exe
       Database: C:\Users\...\ZenithDAW\crashpad_db
     ```
   - If you see "handler not found", rebuild with correct path

### Crash dumps are huge (>100 MB)
- **Normal size**: 1-10 MB (compressed: 100-500 KB)
- **If larger**: May include full heap dumps
- **Solution**: Crashpad defaults to mini-dumps (limited memory capture)

### WinDbg can't find symbols
1. **Check PDB location**:
   ```
   .sympath+ C:\path\to\build\Debug
   .reload /f ZenithDAW.exe
   ```

2. **Verify PDB was generated**:
   - Look for `ZenithDAW.pdb` in same directory as `ZenithDAW.exe`
   - CMake builds generate PDBs in all configs (`/Zi /DEBUG:FULL`)

3. **Use Microsoft symbol server** (for system DLLs):
   ```
   .sympath SRV*C:\Symbols*https://msdl.microsoft.com/download/symbols
   ```

---

## Best Practices

### For Users:
- **Enable crash reporting** during early testing to help catch bugs
- **Keep crash dumps** for 30 days (in case follow-up analysis is needed)
- **Report crashes** with reproduction steps to developers

### For Developers:
- **Always build with PDBs** (CMake does this by default)
- **Archive PDBs** for every released build (required for later analysis)
- **Test crash reporting** in Debug builds before release
- **Document known crashes** and workarounds in issue tracker

---

## Additional Resources

- **Crashpad Documentation**: [chromium.googlesource.com/crashpad/crashpad](https://chromium.googlesource.com/crashpad/crashpad/)
- **WinDbg Tutorial**: [Microsoft Docs - Getting Started with WinDbg](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/getting-started-with-windbg)
- **Building Crashpad**: See `scripts/windows/crashpad/README.md`
- **ETW Performance Tracing**: See `scripts/windows/perf/etw/README.md` (complements crash analysis)

---

## Related Documentation

- **W6 Performance HUD**: Real-time frame-time monitoring (see `docs/performance_hud.md`)
- **W7 ETW Tracing**: Kernel-level performance analysis (see `scripts/windows/perf/etw/README.md`)
- **W9 Windows Roadmap**: Full Windows hardening plan (see `docs/windows_roadmap.md`)

---

**Last Updated**: 2025-01-12 (W8: Crashpad Integration)
