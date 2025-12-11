# Plugin Management System - Implementation Summary

## Branch: agent/architect/plugin-sandbox

**Date:** 2025-12-10  
**Focus:** Robust Plugin Management with Crash Recovery

---

## Overview

This implementation makes Zenith DAW resilient against crashing VST plugins by implementing:

1. **Dead-Man's Pedal** - Crash detection during plugin scanning
2. **Known Plugins Cache** - Serialization to XML to avoid rescanning
3. **Blacklist System** - Skip plugins that crashed in previous scans
4. **Async Scanning** - Background thread with progress updates

---

## Files Modified

### `apps/desktop/Source/engine/PluginHost.h`

**New Methods/Features:**
- `loadFromDisk()` - Loads cached plugins on startup
- `saveToDisk()` - Persists plugin list to XML
- `getCurrentlyScanning()` - Returns name of plugin being scanned
- `getBlacklistedPlugins()` - Returns list of crashed plugins
- `removeFromBlacklist()` - Allow retry of blacklisted plugins
- `clearBlacklist()` - Clear all blacklisted plugins
- `isBlacklisted()` - Check if plugin path is blacklisted
- `saveSearchPaths()` / `loadSearchPaths()` - Persist custom paths

**New Private Methods:**
- `getPluginCacheDirectory()` - Returns `%APPDATA%/ZenithAudio`
- `getKnownPluginsFile()` - Returns path to `known_plugins.xml`
- `getBlacklistFile()` - Returns path to `crashed_plugins.txt`
- `getDeadMansPedalFile()` - Returns path to `scan_in_progress.tmp`
- `checkForCrashedScan()` - Check for crash during last scan
- `writeDeadMansPedal()` - Write current plugin before scanning
- `clearDeadMansPedal()` - Clear after successful scan
- `scanPluginFileSafely()` - Wrapped scan with exception handling

**New Member Variables:**
- `currentlyScanning_` - Name of plugin being scanned
- `scanMutex_` - Thread-safe access to scan state
- `blacklistedPlugins_` - Set of crashed plugin paths
- `blacklistMutex_` - Thread-safe blacklist access
- `loadedFromDisk_` - Track initialization state

---

### `apps/desktop/Source/engine/PluginHost.cpp`

Complete rewrite with:

1. **Crash Recovery System:**
   - Before scanning each plugin, write path to `scan_in_progress.tmp`
   - On successful scan, delete the temp file
   - On startup, if temp file exists → that plugin crashed → add to blacklist

2. **Persistence:**
   - `known_plugins.xml` - XML serialization of `KnownPluginList`
   - `crashed_plugins.txt` - Line-delimited blacklist file
   - All files stored in `%APPDATA%/ZenithAudio/`

3. **Safe Scanning:**
   - `scanPluginFileSafely()` wraps all scans in try-catch
   - Blacklisted plugins are skipped entirely
   - Exception handling prevents cascading failures

4. **Async Scanning:**
   - Runs on dedicated `std::thread`
   - Progress callback marshalled to message thread via `MessageManager::callAsync`
   - Cancelable via `shouldCancel_` atomic flag

---

### `apps/desktop/Source/ui/SettingsComponent.h`

Enhanced `PluginSettingsTab` class with:

- **Visual Progress Bar** - Animated scanning indicator
- **Current Plugin Display** - Shows name of plugin being scanned
- **Plugin Count** - Shows number of known plugins
- **Blacklist Section** - Displays crashed plugins (red tint)
- **Add Path Button** - Add custom search paths
- **Clear Blacklist Button** - Reset all blacklisted plugins
- **Cancel Button** - Stop ongoing scans

Uses `juce::Timer` for real-time UI updates during scanning.

---

### `apps/desktop/Source/engine/Engine.cpp`

Added initialization call:
```cpp
pluginHost_ = std::make_unique<zenith::PluginHost>();
pluginHost_->loadFromDisk();  // Load cached plugins to avoid rescanning
DBG("Engine: PluginHost initialized with " + juce::String(pluginHost_->getKnownPlugins().getNumTypes()) + " plugins");
```

---

## File Locations

| File | Purpose |
|------|---------|
| `%APPDATA%/ZenithAudio/known_plugins.xml` | Cached plugin descriptions |
| `%APPDATA%/ZenithAudio/crashed_plugins.txt` | Blacklisted plugin paths |
| `%APPDATA%/ZenithAudio/scan_in_progress.tmp` | Dead-man's pedal |

---

## How It Works

### Startup Sequence

1. Engine creates `PluginHost`
2. `loadFromDisk()` is called:
   - Check for `scan_in_progress.tmp` → if exists, plugin crashed
   - Load `crashed_plugins.txt` into blacklist
   - Load `known_plugins.xml` into `KnownPluginList`
   - Load custom search paths from settings

### Scanning Sequence

1. User clicks "Scan Plugins"
2. `scanAsync()` starts background thread
3. For each `.vst3` file not in blacklist:
   - Write path to `scan_in_progress.tmp`
   - Try to scan plugin
   - On success: clear temp file
   - On exception: log error, continue
4. At completion:
   - Save `known_plugins.xml`
   - Callback with `progress=100`

### Crash Recovery

1. User scans plugins
2. Bad plugin causes DAW crash
3. On next startup:
   - `checkForCrashedScan()` finds temp file
   - Reads path from temp file
   - Adds path to blacklist
   - Deletes temp file
4. Future scans skip that plugin

---

## Testing Recommendations

1. **Happy Path:** Scan plugins, verify XML created, restart DAW, verify no rescan
2. **Crash Recovery:** Manually create `scan_in_progress.tmp` with a plugin path, verify blacklist on startup
3. **Blacklist Management:** Add plugin to blacklist, verify skipped, clear blacklist, verify scanned again
4. **Async Cancel:** Start scan, cancel mid-way, verify partial results saved

---

## Known Limitations

- Only VST3 plugins are scanned (future: AudioUnit on macOS)
- No timeout for hung plugins (future: process isolation)
- Blacklist is path-based (plugin updates won't auto-retry)

---

## Dependencies

- JUCE: `AudioPluginFormatManager`, `KnownPluginList`, `PluginDirectoryScanner`
- C++ Standard: `<thread>`, `<mutex>`, `<set>`, `<atomic>`
