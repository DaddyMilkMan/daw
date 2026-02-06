# Safe Refactoring Scripts

**Fixed versions of the 5-star navigation scripts that are actually safe to run.**

## Quick Start

```bash
# 1. Run preflight check
./scripts/preflight-check.sh

# 2. Preview what would be renamed
python3 scripts/safe-rename.py --list-approved
python3 scripts/safe-rename.py --dry-run

# 3. Execute renames (if preflight passes)
python3 scripts/safe-rename.py --execute

# 4. Update includes
python3 scripts/update-includes-safe.py --map-file renames.json --dry-run
python3 scripts/update-includes-safe.py --map-file renames.json --execute

# 5. Migrate tests
python3 scripts/migrate-tests-safe.py --analyze
python3 scripts/migrate-tests-safe.py --execute
```

## Scripts

### 1. preflight-check.sh

Validates preconditions before any refactoring.

**Checks:**
- Git state (clean working directory)
- Build system (CMake available)
- Source files (counts, problematic patterns)
- Test infrastructure
- Disk space

**Usage:**
```bash
./scripts/preflight-check.sh
```

**Exit codes:**
- 0: All checks passed (or warnings only)
- 1: Errors found, fix before proceeding

---

### 2. safe-rename.py

Safely renames files using `git mv` to preserve history.

**Safety features:**
- ✅ Uses `git mv` (preserves history)
- ✅ Detects active refactoring (skips TransportController, etc.)
- ✅ Updates CMakeLists.txt automatically
- ✅ Dry-run mode (default)
- ✅ Rollback capability
- ✅ Validates each rename before executing

**Usage:**
```bash
# List all approved renames
python3 scripts/safe-rename.py --list-approved

# Preview what would happen (dry run)
python3 scripts/safe-rename.py --dry-run

# Execute the renames
python3 scripts/safe-rename.py --execute

# Rollback if something went wrong
python3 scripts/safe-rename.py --rollback <timestamp>
```

**What it renames:**
Only files in `APPROVED_RENAMES` list:
- `SkiaRenderer.h` → `EngineSkiaRenderer.h` / `UISkiaRenderer.h` (clearly different)

**What it skips:**
- `TransportController` (active refactoring in progress)
- `views2` directories (migration in progress)
- Any file with uncommitted changes

---

### 3. update-includes-safe.py

Updates `#include` statements WITHOUT using dangerous regex.

**Safety features:**
- ✅ Token-aware parsing (understands C++ syntax)
- ✅ Preserves comments (doesn't modify them)
- ✅ Preserves string literals (doesn't modify them)
- ✅ Only modifies actual include directives
- ✅ Dry-run mode

**Usage:**
```bash
# Create rename map from safe-rename.py output, then:
python3 scripts/update-includes-safe.py --map-file renames.json --dry-run
python3 scripts/update-includes-safe.py --map-file renames.json --execute
```

**Why not regex?**
```cpp
// Regex would break this:
const char* help = "Use #include \"file.h\"";  // ❌ Modified by regex

// Token parser preserves it:
const char* help = "Use #include \"file.h\"";  // ✅ Left alone
```

---

### 4. migrate-tests-safe.py

Migrates tests to mirrored module structure.

**Safety features:**
- ✅ Uses `git mv` (preserves history)
- ✅ No duplicates (moves, doesn't copy)
- ✅ Explicit mappings (no guessing)
- ✅ Checks for conflicts before migrating
- ✅ Generates CMake updates needed

**Usage:**
```bash
# Analyze what would be migrated
python3 scripts/migrate-tests-safe.py --analyze

# Verify current structure
python3 scripts/migrate-tests-safe.py --verify

# Execute migration
python3 scripts/migrate-tests-safe.py --execute

# See CMake updates needed
python3 scripts/migrate-tests-safe.py --cmake-updates
```

**Migration strategy:**
- Tests move from `apps/desktop/Source/tests/` 
- To `modules/[module]/tests/[subpath]/`
- Only known test mappings are migrated
- Unknown tests are reported (you decide)

---

### 5. flatten-incremental.py

Safely flattens directory structure incrementally.

**Safety features:**
- ✅ Preserves protected directories (tests, instruments)
- ✅ Never overwrites files (checks for conflicts)
- ✅ Only flattens explicitly approved directories
- ✅ Removes empty directories after move
- ✅ Creates README files for new directories

**Usage:**
```bash
# Analyze what would be flattened
python3 scripts/flatten-incremental.py --analyze

# Execute (dry run first)
python3 scripts/flatten-incremental.py --execute --dry-run
python3 scripts/flatten-incremental.py --execute
```

**What gets flattened:**
- `ui/controls/` → `components/`
- `ui/common/` → `components/`
- `ui/visualization/` → `components/`
- `ui/design-system/` → `theme/`

**What is protected:**
- `tests/` directories (keep structure)
- `instruments/` (meaningful subdirs)
- `effects/` (meaningful subdirs)

---

## Workflow

### Phase 1: Rename Files (Safe)

```bash
# Check prerequisites
./scripts/preflight-check.sh

# Preview renames
python3 scripts/safe-rename.py --dry-run

# Execute if happy
python3 scripts/safe-rename.py --execute

# Verify
python3 scripts/safe-rename.py --list-approved
```

### Phase 2: Update Includes (Safe)

```bash
# Generate rename map (saved by safe-rename.py)
# Preview include updates
python3 scripts/update-includes-safe.py --map-file .rename-rollback/rollback_*.json --dry-run

# Execute
python3 scripts/update-includes-safe.py --map-file .rename-rollback/rollback_*.json --execute
```

### Phase 3: Migrate Tests (Safe)

```bash
# Analyze
python3 scripts/migrate-tests-safe.py --analyze

# Execute
python3 scripts/migrate-tests-safe.py --execute

# Get CMake updates
python3 scripts/migrate-tests-safe.py --cmake-updates
```

### Phase 4: Flatten Structure (Safe)

```bash
# Analyze
python3 scripts/flatten-incremental.py --analyze

# Execute
python3 scripts/flatten-incremental.py --execute --dry-run
python3 scripts/flatten-incremental.py --execute
```

---

## Rollback

If anything goes wrong:

```bash
# After safe-rename.py --execute
python3 scripts/safe-rename.py --rollback <timestamp>

# Or manually with git
git reset --hard <commit-before-rename>
```

Rollback states are saved in `.rename-rollback/rollback_<timestamp>.json`

---

## Differences from Original Scripts

| Issue | Original | Fixed |
|-------|----------|-------|
| Git history | `mv` (lost) | `git mv` (preserved) |
| TransportController | Would rename (breaking) | Detected as refactoring, skipped |
| Include updates | Regex (dangerous) | Token parser (safe) |
| Test migration | `cp` (duplicates) | `git mv` (no duplicates) |
| CMake updates | Not handled | Automatic |
| Dry run | Partial | All scripts support --dry-run |
| Rollback | None | Full rollback support |
| Active refactoring detection | None | Detects and skips |

---

## Safety Guarantees

1. **No data loss**: All scripts use `git mv`, not `cp` or `mv`
2. **No overwrites**: Scripts check if target exists before moving
3. **No breaking changes**: Active refactoring targets are detected and skipped
4. **No regex on code**: Include updater uses token parsing
5. **Always reversible**: Rollback states saved before any changes

---

## When NOT to Use These Scripts

Don't run these if:
- ❌ You have uncommitted changes (run `git status` first)
- ❌ You're on main/master branch (create a feature branch)
- ❌ Tests are failing (fix tests first)
- ❌ Someone else is actively working on the same files

Use these scripts when:
- ✅ Preflight check passes
- ✅ You're on a feature branch
- ✅ Build is working
- ✅ You have time to verify changes

---

## Troubleshooting

### "Uncommitted changes detected"
```bash
git stash
git checkout -b refactor/navigation
git stash pop
./scripts/preflight-check.sh
```

### "TransportController is an active refactoring"
This is correct behavior. The TransportController files are intentionally left alone because they're part of an active refactoring (legacy in `engine/`, new in `engine/core/`).

### "CMakeLists.txt not updated"
The `safe-rename.py` script updates CMake files automatically, but you may need to:
1. Update include directories
2. Update target sources manually for complex cases

### "Tests still in old location"
The test migrator only moves tests with explicit mappings. Add your test to `KNOWN_MAPPINGS` in `migrate-tests-safe.py` if it's not recognized.

---

## Support

- Check preflight output: `./scripts/preflight-check.sh`
- Use dry-run mode: `--dry-run` (default for most scripts)
- Review rollback state: `cat .rename-rollback/rollback_*.json`
- When in doubt: `git reset --hard HEAD` (before committing!)

---

## Original vs Fixed Comparison

| Aspect | Original (Broken) | Fixed (Safe) |
|--------|-------------------|--------------|
| Git history | ❌ Lost (`mv`) | ✅ Preserved (`git mv`) |
| Active refactoring | ❌ Would break | ✅ Detected & skipped |
| Include updates | ❌ Regex (dangerous) | ✅ Token parsing (safe) |
| Test migration | ❌ `cp` (duplicates) | ✅ `git mv` (no duplicates) |
| CMake updates | ❌ None | ✅ Automatic |
| Rollback | ❌ None | ✅ Full support |
| Dry run | ❌ Partial | ✅ All operations |

**Key fixes:**
1. **TransportController** - Original would rename and break the active refactoring. Fixed version detects both legacy (`engine/`) and new (`engine/core/`) exist, skips it.
2. **Includes** - Original regex modified comments and strings. Fixed token parser only touches real includes.
3. **Tests** - Original `cp` created duplicates. Fixed `git mv` moves without duplication.
4. **CMake** - Original ignored CMake files. Fixed auto-updates them.

