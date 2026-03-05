# Branch Protection Policy

This document describes the branch protection rules for the Zenith DAW repository and provides step-by-step instructions for repository administrators to configure them.

---

## Protected Branches

| Branch    | Purpose                          |
|-----------|----------------------------------|
| `main`    | Stable, production-ready code    |
| `develop` | Integration branch for features  |

---

## Required Rules

### 1. Require Pull Requests Before Merging

All changes to `main` and `develop` must go through a pull request. Direct pushes are **not allowed**.

- **Required approvals:** minimum **1** (recommend 2 for `main`)
- **Dismiss stale reviews** when new commits are pushed: **enabled**
- **Require review from Code Owners:** enabled (see `CODEOWNERS`)

### 2. Require Status Checks to Pass Before Merging

The following CI checks are **required** and must pass before any PR can be merged:

| Check Name                           | Workflow          | Description                                       |
|--------------------------------------|-------------------|---------------------------------------------------|
| `CI Gate (required for merge)`       | `ci.yml`          | Aggregates all required platform and sanitizer jobs |
| `build-linux (Release, gcc-12)`      | `ci.yml`          | Linux Release build                               |
| `build-linux (Debug, gcc-12)`        | `ci.yml`          | Linux Debug build                                 |
| `build-macos (Release)`              | `ci.yml`          | macOS Release build                               |
| `build-windows (Release)`            | `ci.yml`          | Windows Release build                             |
| `Linux ASan+UBSan (Clang)`           | `ci.yml`          | AddressSanitizer + UBSan — **blocks merge on failure** |
| `Linux TSan (Clang)`                 | `ci.yml`          | ThreadSanitizer — **blocks merge on failure**     |
| `macOS ASan+UBSan`                   | `ci.yml`          | AddressSanitizer + UBSan on macOS — **blocks merge on failure** |

> **Sanitizer failures always block merge.** A data race detected by TSan or a memory error detected by ASan must be resolved before the PR is accepted.

- **Require branches to be up to date before merging:** enabled

### 3. Disallow Force Pushes

Force pushes to `main` and `develop` are **disabled** for all users including administrators.

### 4. Disallow Branch Deletion

`main` and `develop` cannot be deleted.

### 5. Require Signed Commits (Recommended)

Signing commits with GPG or SSH is recommended for `main`.

---

## Setup Instructions (GitHub Web UI)

1. Navigate to **Settings → Branches** in the repository.
2. Click **Add branch protection rule** (or edit an existing rule).
3. In **Branch name pattern**, enter `main` (repeat separately for `develop`).
4. Enable the following options:

   | Option | Setting |
   |--------|---------|
   | Require a pull request before merging | ✅ Enabled |
   | → Required approvals | 1 (main: 2 recommended) |
   | → Dismiss stale PR reviews | ✅ Enabled |
   | → Require review from Code Owners | ✅ Enabled |
   | Require status checks to pass | ✅ Enabled |
   | → Require branches to be up to date | ✅ Enabled |
   | → Status checks (add each): | See table above |
   | Do not allow bypassing the above settings | ✅ Enabled |
   | Allow force pushes | ❌ Disabled |
   | Allow deletions | ❌ Disabled |

5. Click **Save changes**.
6. Repeat for `develop` with the same settings (1 required approval is sufficient).

---

## Setup Instructions (GitHub CLI)

```bash
# Requires gh CLI >= 2.x and admin access to the repository
gh api repos/Sylorlabs/zenith-daw/branches/main/protection \
  --method PUT \
  --field required_status_checks='{"strict":true,"contexts":["CI Gate (required for merge)","Linux ASan+UBSan (Clang)","Linux TSan (Clang)","macOS ASan+UBSan"]}' \
  --field enforce_admins=true \
  --field required_pull_request_reviews='{"required_approving_review_count":2,"dismiss_stale_reviews":true,"require_code_owner_reviews":true}' \
  --field restrictions=null \
  --field allow_force_pushes=false \
  --field allow_deletions=false
```

Replace `main` with `develop` (and reduce `required_approving_review_count` to `1`) for the develop branch.

---

## Sanitizer Policy

| Sanitizer | Platform | CMake Preset | Failure action |
|-----------|----------|-------------|----------------|
| AddressSanitizer + UBSan | Linux (Clang 15) | `linux-asan` | **Blocks merge** |
| ThreadSanitizer | Linux (Clang 15) | `linux-tsan` | **Blocks merge** |
| AddressSanitizer + UBSan | macOS (AppleClang) | `macos-asan` | **Blocks merge** |
| DrMemory / Application Verifier | Windows | Manual (see below) | Recommended |

### Running Sanitizers Locally

```bash
# Linux ASan
cmake --preset linux-asan
cmake --build --preset linux-asan --target ZenithTests -j$(nproc)
ASAN_OPTIONS=halt_on_error=1:detect_leaks=1 cd build-asan && ctest --output-on-failure

# Linux TSan
cmake --preset linux-tsan
cmake --build --preset linux-tsan --target ZenithTests -j$(nproc)
TSAN_OPTIONS=halt_on_error=1:detect_deadlocks=1 cd build-tsan && ctest --output-on-failure

# macOS ASan
cmake --preset macos-asan
cmake --build --preset macos-asan --target ZenithTests
ASAN_OPTIONS=halt_on_error=1:detect_leaks=0 cd build-asan && ctest --output-on-failure
```

### Windows (DrMemory — optional)

DrMemory provides memory error detection on Windows similar to ASan/Valgrind. It is not enforced in CI by default but is recommended for contributors working on Windows:

```powershell
# Install DrMemory
choco install drmemory -y

# Run tests under DrMemory
drmemory -report_max -1 -- .\build\ZenithTests.exe
```

---

## Workflow for Contributors

1. Create a feature branch from `develop` (never push directly to `main` or `develop`).
2. Open a pull request against `develop` (or `main` for hotfixes).
3. Fill out the **RT Path Declaration** in the PR template.
4. Wait for all required CI status checks to go green.
5. Address any review feedback.
6. A maintainer merges after all checks pass and the required approvals are met.

---

## References

- [`docs/tech-briefs/06-audio-thread-safety-policy.md`](../docs/tech-briefs/06-audio-thread-safety-policy.md) — RT thread rules
- [`docs/THREADING_MODEL.md`](../docs/THREADING_MODEL.md) — Threading architecture
- [`CMakePresets.json`](../CMakePresets.json) — Sanitizer build presets
- [GitHub Branch Protection Documentation](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches)
