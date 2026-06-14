# Zenith DAW - Navigation Guide

## ⚠️ CURRENT STATE vs TARGET STATE

**This codebase is undergoing refactoring.** Some directories described here (like `components/`, `theme/`) are the **target** structure, not the current structure. Current structure has `ui/controls/`, `ui/design-system/`, etc.

See [Naming Standards Migration](#naming-standards-migration) below for details.

---

## 🎯 Quick Reference

| Looking for... | Go to... | Note |
|---------------|----------|------|
| Audio Engine | `modules/zenith_core/engine/` | Active refactoring in progress |
| UI Components | `modules/zenith_ui/ui/controls/` | Will move to `components/` |
| DSP Effects | `modules/zenith_core/effects/` | |
| Instruments | `modules/zenith_core/instruments/` | |
| Tests | `apps/desktop/Source/tests/` | Migrating to `modules/[module]/tests/` |
| Documentation | `docs/` | |
| Build Config | `cmake/`, `CMakeLists.txt` | |

---

## 📁 CURRENT Directory Structure (Reality)

```
zenith-daw/
├── apps/
│   └── desktop/
│       ├── Source/          # Main.cpp, application setup
│       │   ├── ai_client/   # AI integration
│       │   ├── browser/     # Built-in browser
│       │   ├── platform/    # OS-specific code
│       │   └── tests/       # Tests (currently here, migrating to modules/)
│       └── ...
│
├── modules/
│   ├── zenith_core/         # Audio engine, instruments, plugins
│   │   ├── engine/          # Transport, mixing, recording
│   │   │   └── core/        # NEW: Refactored engine core (migration in progress)
│   │   ├── instruments/     # Built-in synths & samplers
│   │   ├── plugins/         # VST3 hosting
│   │   ├── dsp/             # Shared DSP utilities
│   │   └── effects/         # Audio effects
│   │
│   ├── zenith_ui/           # UI framework & components
│   │   ├── ui/
│   │   │   ├── controls/    # UI controls (will be components/)
│   │   │   ├── views/       # Legacy views
│   │   │   ├── views2/      # NEW: Refactored views (migration in progress)
│   │   │   ├── common/      # Shared UI components
│   │   │   ├── design-system/  # Theme files (will be theme/)
│   │   │   └── visualization/  # Visualizers
│   │   └── rendering/       # Skia rendering
│   │
│   ├── zenith_network/      # Network & collaboration
│   ├── zenith_commands/     # Command API for AI
│   └── zenith_dsp/          # DSP processing
│
├── docs/                    # Documentation
├── cmake/                   # CMake modules
├── scripts/                 # Build & maintenance scripts
└── Content/                 # Presets, samples, fonts
```

---

## 🔍 Finding Code

### Active Refactoring Areas

**⚠️ These areas have duplicate filenames (migration in progress):**

```cpp
// TransportController - TWO versions exist:
#include "engine/TransportController.h"           // Legacy
#include "engine/core/TransportController.h"      // New refactored version
#include "engine/core/ITransportController.h"     // Interface

// SkiaRenderer - TWO versions exist:
#include "engine/rendering/SkiaRenderer.h"        // Engine version
#include "ui/rendering/SkiaRenderer.h"            // UI version

// Views - TWO versions exist:
// ui/views/     - Legacy
// ui/views2/    - New refactored
```

**When contributing:** Use the `engine/core/` and `views2/` versions for new code.

---

## 📝 Naming Conventions (Target)

**Note: Not all code follows this yet - migration in progress**

| Domain | Prefix | Example | Status |
|--------|--------|---------|--------|
| Engine | `Engine` | `EngineTransport.h` | 🔄 Migrating |
| UI | `UI` | `UISkiaButton.h` | 🔄 Migrating |
| DSP | `DSP` | `DSPCompressor.h` | ✅ Following |
| Network | `Net` | `NetWebSocketClient.h` | ✅ Following |

### Duplicate Filenames (To Be Renamed)

```bash
# These files have duplicates and will be renamed:
# - SkiaRenderer.h -> EngineSkiaRenderer.h / UISkiaRenderer.h
# - TransportController.h -> EngineTransportController.h
# - etc.
```

See `docs/NAMING_STANDARDS.md` for full convention.

---

## 🧪 Tests

**Current:** Tests are in `apps/desktop/Source/tests/`
**Target:** Tests will be in `modules/[module]/tests/`

```bash
# Current way to run tests:
cd build
./ZenithDAWTests

# Or:
ctest
```

---

## 🛠️ Safe Refactoring Scripts

**⚠️ The old scripts (rename-duplicates.sh, etc.) were broken and removed.**

Use these safe versions:

| Script | Purpose | Safety Features |
|--------|---------|-----------------|
| `scripts/preflight-check.sh` | Validate before changes | Git state, CMake, etc. |
| `scripts/safe-rename.py` | Rename files | `git mv`, detects refactoring |
| `scripts/update-includes-safe.py` | Update includes | Token parsing, not regex |
| `scripts/migrate-tests-safe.py` | Migrate tests | `git mv`, no duplicates |
| `scripts/flatten-incremental.py` | Flatten structure | Conflict detection |

**Usage:**
```bash
# Always start with preflight
./scripts/preflight-check.sh

# Preview changes
python3 scripts/safe-rename.py --dry-run

# Execute if happy
python3 scripts/safe-rename.py --execute
```

See `scripts/README.md` for full details.

---

## 📚 Key Documentation

| Document | Purpose | Status |
|----------|---------|--------|
| `README.md` | Project overview | ✅ Current |
| `GETTING_STARTED.md` | Setup & first build | ✅ Current |
| `docs/ARCHITECTURE.md` | System design | ⚠️ Describes target state |
| `docs/NAMING_STANDARDS.md` | Naming conventions | ✅ Target standards |
| `docs/INCLUDE_STANDARDS.md` | Include guidelines | ✅ Target standards |
| `BUILDING.md` | Build instructions | ✅ Current |
| `CONTRIBUTING.md` | Contribution guidelines | ✅ Current |

---

## 🚨 Common Issues

### "Which TransportController should I use?"

Use the new `engine/core/` version:
```cpp
#include "engine/core/TransportController.h"
```

The `engine/TransportController.h` is legacy and will be deprecated.

### "views/ vs views2/?"

Use `views2/` for new code. `views/` is legacy.

### "ui/controls/ vs components/?"

`components/` doesn't exist yet. Use `ui/controls/` for now.

---

## 💡 Tips

1. **Check for duplicates** before including:
   ```bash
   find modules -name "*.h" | xargs basename | sort | uniq -d
   ```

2. **Use IDE navigation** but verify the file is the right one

3. **Ask before refactoring** - active migration in progress

---

*Last updated: February 2026 (refactoring in progress)*
