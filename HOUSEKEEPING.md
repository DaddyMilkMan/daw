# Repository Housekeeping Guide

**Last Updated:** 2025-11-12
**Purpose:** Repository maintenance policies and procedures for Zenith DAW

---

## 📋 Table of Contents

1. [Branch Strategy](#branch-strategy)
2. [Branch Protection Rules](#branch-protection-rules)
3. [Code Review Policy](#code-review-policy)
4. [Merge Policy](#merge-policy)
5. [Branch Cleanup](#branch-cleanup)
6. [Safety & Recovery](#safety--recovery)
7. [CI/CD Guidelines](#cicd-guidelines)

---

## 🌳 Branch Strategy

### Primary Branches

| Branch | Purpose | Protection | Merge Policy |
|--------|---------|------------|--------------|
| `main` | Production-ready code, pure JUCE 8 Zenith | 🔒 Protected | PR + Review + CI |
| `develop` | Integration branch for features | 🔒 Protected | PR + CI |
| `feature/*` | New feature development | None | PR to `develop` |
| `fix/*` | Bug fixes | None | PR to `develop` |
| `hotfix/*` | Emergency production fixes | None | PR to `main` + backport |

### Branch Naming Convention

```
feature/<description>-<ticket>       # New features
fix/<description>-<ticket>           # Bug fixes
hotfix/<description>-<ticket>        # Emergency fixes
docs/<description>                   # Documentation only
refactor/<description>               # Code refactoring
perf/<description>                   # Performance improvements
test/<description>                   # Test additions/fixes
```

**Examples:**
- `feature/piano-roll-velocity-lanes-ZEN-123`
- `fix/audio-dropout-wasapi-ZEN-456`
- `hotfix/crash-on-vst3-load-ZEN-789`
- `docs/update-build-instructions`

---

## 🔒 Branch Protection Rules

### `main` Branch Protection

**Required Settings (via GitHub Settings → Branches):**

```yaml
Branch Protection Rules for 'main':
  ✅ Require a pull request before merging
     - Required approvals: 1
     - Dismiss stale pull request approvals when new commits are pushed
     - Require review from Code Owners (if CODEOWNERS file exists)

  ✅ Require status checks to pass before merging
     - Require branches to be up to date before merging
     - Status checks required:
       - ci/build-windows
       - ci/build-macos
       - ci/build-linux
       - ci/tests

  ✅ Require conversation resolution before merging

  ✅ Require signed commits (optional but recommended)

  ✅ Require linear history
     - Use "Squash and merge" or "Rebase and merge" only
     - No merge commits

  ✅ Do not allow bypassing the above settings
     - Administrators are NOT included

  ❌ Allow force pushes: Disabled
  ❌ Allow deletions: Disabled
```

**To enable via GitHub CLI:**

```bash
gh api repos/DaddyMilkMan/daw/branches/main/protection \
  -X PUT \
  -f required_status_checks='{"strict":true,"contexts":["ci/build-windows","ci/build-macos","ci/build-linux","ci/tests"]}' \
  -f enforce_admins=true \
  -f required_pull_request_reviews='{"dismissal_restrictions":{},"dismiss_stale_reviews":true,"require_code_owner_reviews":false,"required_approving_review_count":1}' \
  -f required_linear_history=true \
  -f allow_force_pushes=false \
  -f allow_deletions=false \
  -f required_conversation_resolution=true
```

### `develop` Branch Protection

Same as `main`, but with slightly relaxed rules:
- Required approvals: 1 (can be from any team member)
- Allow "Merge commits" in addition to "Squash and merge"

---

## 👀 Code Review Policy

### Review Requirements

**For `main` branch:**
- ✅ 1 approval from maintainers
- ✅ All CI checks passing (build + tests)
- ✅ All conversations resolved
- ✅ Code follows [C++20 style guide](docs/CODING_STANDARDS.md)
- ✅ No memory leaks (verified with sanitizers)
- ✅ Real-time audio safety (no allocations in hot paths)

**For `develop` branch:**
- ✅ 1 approval from any team member
- ✅ All CI checks passing
- ✅ Code follows style guide

### What Reviewers Should Check

1. **Architecture Purity**
   - ✅ No Qt/QML code (use JUCE only)
   - ✅ No Electron/web frameworks
   - ✅ JUCE 8.0.9 or later
   - ✅ No legacy dependencies (JUCE 7, old VexelDAW-Native code)

2. **Audio Thread Safety**
   - ✅ No heap allocations in audio callbacks
   - ✅ No mutex locks in audio thread
   - ✅ No system calls in audio thread
   - ✅ Lock-free FIFO for inter-thread communication

3. **Code Quality**
   - ✅ Clear variable names
   - ✅ Appropriate comments for complex logic
   - ✅ No magic numbers (use constants)
   - ✅ Error handling present

4. **Testing**
   - ✅ Unit tests for new functionality
   - ✅ Integration tests if touching audio engine
   - ✅ Manual testing instructions provided

---

## 🔀 Merge Policy

### Merge Methods

| Branch Target | Merge Method | Commit Message Format |
|---------------|--------------|----------------------|
| `main` | **Squash and merge** | `feat(<scope>): <description> (#PR)` |
| `develop` | **Squash and merge** or **Rebase** | `feat(<scope>): <description> (#PR)` |
| Feature branches | Any | Descriptive |

### Commit Message Format

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `perf`: Performance improvement
- `refactor`: Code refactoring (no functional change)
- `docs`: Documentation only
- `test`: Test additions/fixes
- `build`: Build system changes
- `ci`: CI/CD configuration
- `chore`: Maintenance tasks

**Scopes:**
- `ui`: User interface (JUCE Components)
- `engine`: Audio engine
- `win`: Windows-specific code
- `mac`: macOS-specific code
- `linux`: Linux-specific code
- `cmake`: Build configuration
- `vst`: Plugin hosting
- `midi`: MIDI functionality
- `transport`: Playback controls

**Examples:**
```
feat(ui): add velocity lanes to piano roll (#123)

Implements velocity editing with:
- Mouse drag to draw velocities
- Pencil/line/curve tools
- Undo/redo support

Closes ZEN-123

---

fix(engine): prevent audio dropout on buffer size change (#456)

The audio engine was not properly handling buffer size changes
at runtime, causing dropouts. Now uses double-buffering pattern.

Fixes ZEN-456

---

perf(ui): optimize TrackView rendering with virtualization (#789)

Reduces overdraw by only rendering visible tracks. Improves
scroll performance from 30 FPS to 60 FPS with 100+ tracks.

Closes ZEN-789
```

---

## 🗑️ Branch Cleanup

### When to Delete Branches

**Automated Deletion:**
- ✅ After PR is merged (GitHub can auto-delete)
- ✅ Enable "Automatically delete head branches" in repo settings

**Manual Deletion:**
- Stale feature branches (>30 days, no activity)
- Abandoned experiments
- Superseded by other work

### Archive Instead of Delete

For historically significant branches:

```bash
# Create archive tag
git tag -a "archive/<branch-name>@$(date +%Y%m%d)" <branch-sha> \
  -m "Archive tag for <branch-name> - <reason>"

# Push tag
git push origin "archive/<branch-name>@$(date +%Y%m%d)"

# Delete branch
git push origin --delete <branch-name>
git branch -D <branch-name>
```

**Branches to Archive (not delete):**
- Major architecture pivots
- Experimental features that might be revived
- Branches with extensive research/documentation

### Legacy Branch Status (2025-11-12)

**Archived & Deleted:**
- ❌ `claude/audit-unimplemented-code-*` (merged to main)
- ❌ `claude/full-implementation-*` (Electron legacy)
- ❌ `claude/zenith-plugin-system-*` (Electron legacy)
- ❌ `claude/zenith-tempo-metronome-*` (Electron legacy, 2 variants)

**Archive Tags Created:**
- `archive/audit-unimplemented-code-*@20251112`
- `archive/full-implementation-*@20251112`
- `archive/zenith-plugin-system-*@20251112`
- `archive/zenith-tempo-metronome-*@20251112` (v1 and v2)

---

## 🛡️ Safety & Recovery

### Before Major Changes

**Always create safety tags:**

```bash
# Tag current state
DATE=$(date -u +%Y%m%d-%H%M%S)
git tag -a "safety/pre-<operation>-${DATE}" HEAD \
  -m "Safety checkpoint before <operation>"

# Optional: Push to remote
git push origin "safety/pre-<operation>-${DATE}"
```

**Before deleting branches:**
- ✅ Verify branch is merged or archived
- ✅ Create archive tag
- ✅ Export patch file: `git format-patch -1 <sha> -o backups/`

### Recovery Procedures

**Recover deleted branch:**

```bash
# Find commit SHA in reflog
git reflog show --all | grep <branch-name>

# Or find by commit message
git log --all --grep="<search-term>" --oneline

# Recreate branch
git branch <branch-name> <sha>
```

**Recover from archive tag:**

```bash
# List archive tags
git tag -l "archive/*"

# Checkout archived code
git checkout archive/<branch-name>@<date>

# Create new branch from archive
git checkout -b recover/<branch-name> archive/<branch-name>@<date>
```

**Rollback main branch (EMERGENCY ONLY):**

```bash
# Create safety tag of current state first!
git tag -a "safety/rollback-$(date +%Y%m%d-%H%M%S)" main

# Rollback to previous commit
git checkout main
git reset --hard <good-commit-sha>

# Force push (requires admin override of protection)
git push --force-with-lease origin main
```

---

## 🤖 CI/CD Guidelines

### Required CI Checks

**All PRs must pass:**

1. **Build Checks**
   - `ci/build-windows` - MSVC 2022, Release + Debug
   - `ci/build-macos` - Xcode 14+, x86_64 + arm64
   - `ci/build-linux` - GCC 10+, Release + Debug

2. **Test Checks**
   - `ci/tests` - Unit tests (100% pass rate)
   - `ci/audio-tests` - Audio engine tests with ASAN/TSAN

3. **Code Quality**
   - `ci/clang-format` - C++ formatting
   - `ci/clang-tidy` - Static analysis (zero warnings)
   - `ci/cppcheck` - Additional static analysis

4. **Audio Safety**
   - `ci/realtime-check` - No allocations in audio callbacks
   - `ci/thread-sanitizer` - No data races

### CI Configuration Files

- `.github/workflows/build.yml` - Build matrix
- `.github/workflows/tests.yml` - Test suite
- `.github/workflows/code-quality.yml` - Linters
- `.clang-format` - Code formatting rules
- `.clang-tidy` - Static analysis rules

### Local Pre-commit Checks

Before pushing:

```bash
# Format code
clang-format -i $(find zenith-core/Source -name "*.cpp" -o -name "*.h")

# Run tests
cd zenith-core/build
ctest --output-on-failure

# Check for real-time violations
./scripts/check-realtime-safety.sh
```

---

## 📊 Repository Health Metrics

### Monitor These Regularly

1. **Branch Count**
   - Target: < 20 active branches
   - Alert: > 50 branches (cleanup needed)

2. **Open PRs**
   - Target: < 10 open PRs
   - Alert: PRs open > 14 days

3. **CI Pass Rate**
   - Target: > 95% on `develop`
   - Target: 100% on `main`

4. **Code Coverage**
   - Target: > 70% for audio engine
   - Target: > 50% for UI code

5. **Build Time**
   - Target: < 5 minutes (full rebuild)
   - Alert: > 10 minutes (investigate)

### Quarterly Cleanup Tasks

**Every 3 months:**

- [ ] Delete stale branches (>90 days inactive)
- [ ] Archive old releases (create tags)
- [ ] Review and update CODEOWNERS
- [ ] Audit dependencies (JUCE version, libraries)
- [ ] Update documentation
- [ ] Review and update this HOUSEKEEPING.md

---

## 🎯 Architecture Enforcement

### Allowed Technologies

✅ **APPROVED:**
- JUCE 8.0.9+ (C++20)
- VST3 SDK (plugin hosting)
- Audio Unit (macOS only)
- Standard library (C++20)
- CMake 3.22+

❌ **FORBIDDEN:**
- Qt/QML (use JUCE instead)
- Electron/CEF (deprecated)
- JUCE 7.x (upgrade required)
- VexelDAW-Native code (legacy)
- Web frameworks (React, Vue, etc.)

### Pre-merge Checklist

Before merging to `main`, verify:

- [ ] ✅ No `VexelDAW-Native/` directory
- [ ] ✅ No `src/qt-qml/` directory
- [ ] ✅ No `src/juce-engine/` directory (redundant)
- [ ] ✅ No `src/audio/` skeleton code
- [ ] ✅ No `.qml` files
- [ ] ✅ No `Qt::` references in code
- [ ] ✅ JUCE 8.0.9 pinned in CMakeLists.txt
- [ ] ✅ All CI checks passing
- [ ] ✅ Code review approved

---

## 📞 Contact & Questions

**For questions about:**
- Branch strategy → Engineering Lead
- CI/CD issues → DevOps Team
- Code review policy → Technical Lead
- Emergency fixes → On-call rotation

**Resources:**
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [CODING_STANDARDS.md](docs/CODING_STANDARDS.md) - C++ style guide
- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - System architecture
- [BRANCH_CONFLICT_RESOLUTION_GUIDE.md](BRANCH_CONFLICT_RESOLUTION_GUIDE.md) - Merge conflict help

---

**Version:** 1.0
**Last Audit:** 2025-11-12
**Next Review:** 2026-02-12 (3 months)
