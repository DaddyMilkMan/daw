#!/usr/bin/env bash
# =============================================================================
# check_rt_forbidden.sh — Static analysis for forbidden RT-thread constructs
#
# Usage:
#   tools/lint/check_rt_forbidden.sh [--warn-only] [FILE ...]
#
# Options:
#   --warn-only   Print violations but exit with code 0 (useful for CI reports)
#   FILE ...      Specific files to check (default: entire source tree)
#
# What this checks
# ----------------
# Files that contain RT annotations (ZENITH_RT_THREAD, ZENITH_RT_SAFE, or
# ZENITH_RT_REGION_BEGIN) are scanned for forbidden constructs:
#
#   1. Heap allocation: new (as keyword), delete, malloc, calloc, realloc, free
#   2. Blocking synchronisation: std::lock_guard, std::unique_lock,
#      std::condition_variable, juce::ScopedLock / ScopedReadLock / ScopedWriteLock
#   3. Blocking waits / sleeps: std::this_thread::sleep, Thread::sleep
#   4. Dynamic containers: .push_back(), .emplace_back()
#   5. System I/O / logging: std::cout <<, std::cerr <<, printf(), fprintf(),
#      DBG(, juce::Logger, .writeToLog()
#
# Suppressions
# ------------
# Add the inline comment  // ZENITH_RT_ALLOWLIST  (or /* ZENITH_RT_ALLOWLIST */)
# to a line to suppress the check for that specific line.  Use this sparingly
# and only for non-RT functions in files that also contain RT-annotated code.
#
# Example:
#   const juce::ScopedReadLock lock(tracksLock_); // ZENITH_RT_ALLOWLIST: non-RT function
#
# Comment lines (starting with // or *) are excluded from the scan automatically.
#
# Output format
# -------------
#   VIOLATION  <file>:<line>  [<category>]  <matched text>
#
# Exit codes
# ----------
#   0  No violations found (or --warn-only was given)
#   1  One or more violations found
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
WARN_ONLY=false
FILE_ARGS=()

for arg in "$@"; do
    if [[ "$arg" == "--warn-only" ]]; then
        WARN_ONLY=true
    else
        FILE_ARGS+=("$arg")
    fi
done

# Default search paths when no files given
if [[ ${#FILE_ARGS[@]} -eq 0 ]]; then
    SEARCH_PATHS=(
        "$ROOT_DIR/modules/zenith_core/engine"
        "$ROOT_DIR/modules/zenith_core/dsp"
        "$ROOT_DIR/modules/zenith_core/instruments"
        "$ROOT_DIR/apps/desktop/Source"
    )
else
    SEARCH_PATHS=("${FILE_ARGS[@]}")
fi

# Files to always exclude (documentation / macro headers whose text mentions
# forbidden constructs on purpose)
EXCLUDE_FILES_PATTERN="include/zenith/RTSafety\.h|include/zenith/LockFreeCommandQueue\.h"

# ---------------------------------------------------------------------------
# Forbidden pattern definitions
# Each entry: "CATEGORY|PATTERN"  (PATTERN is an extended-regex for grep)
#
# Patterns skip:
#   - Comment lines (lines starting with optional whitespace then // or *)
#   - Lines with ZENITH_RT_ALLOWLIST suppression marker
#
# Variable names that *start with* "new" (e.g. newValue) are excluded by
# requiring the preceding char to be non-identifier.
# ---------------------------------------------------------------------------
declare -a FORBIDDEN_PATTERNS=(
    # Heap allocation — "new " or "new(" but NOT inside identifiers
    "ALLOC|[^_a-zA-Z0-9]new[[:space:]]+[a-zA-Z_]"
    "ALLOC|[^_a-zA-Z0-9]new[[:space:]]*\("
    "ALLOC|[^_a-zA-Z0-9]delete[[:space:]]"
    "ALLOC|[^_a-zA-Z0-9]delete\["
    "ALLOC|[^_a-zA-Z0-9]malloc[[:space:]]*\("
    "ALLOC|[^_a-zA-Z0-9]calloc[[:space:]]*\("
    "ALLOC|[^_a-zA-Z0-9]realloc[[:space:]]*\("
    "ALLOC|[^_a-zA-Z0-9]free[[:space:]]*\("
    "ALLOC|std::make_unique[[:space:]]*<"
    "ALLOC|std::make_shared[[:space:]]*<"
    # Dynamic container growth
    "ALLOC|\.push_back[[:space:]]*\("
    "ALLOC|\.emplace_back[[:space:]]*\("
    # Blocking / locking (SCOPED instantiations or local variables)
    "LOCK|std::lock_guard[[:space:]]*<"
    "LOCK|std::unique_lock[[:space:]]*<"
    "LOCK|std::condition_variable[[:space:]]"
    "LOCK|juce::ScopedLock[[:space:]]"
    "LOCK|juce::ScopedReadLock[[:space:]]"
    "LOCK|juce::ScopedWriteLock[[:space:]]"
    # Blocking waits / sleeps
    "SLEEP|std::this_thread::sleep"
    "SLEEP|Thread::sleep[[:space:]]*\("
    # Logging / I/O (active expressions, not declarations)
    "IO|std::cout[[:space:]]*<<"
    "IO|std::cerr[[:space:]]*<<"
    "IO|[^_a-zA-Z0-9]printf[[:space:]]*\("
    "IO|[^_a-zA-Z0-9]fprintf[[:space:]]*\("
    "IO|[^_a-zA-Z0-9]DBG[[:space:]]*\("
    "IO|juce::Logger::"
    "IO|\.writeToLog[[:space:]]*\("
)

# ---------------------------------------------------------------------------
# Helper: check a single file
# ---------------------------------------------------------------------------
check_file() {
    local file="$1"

    # Skip excluded files
    if echo "$file" | grep -qE "$EXCLUDE_FILES_PATTERN"; then
        return 0
    fi

    # Only scan files containing RT annotations
    if ! grep -qE "ZENITH_RT_THREAD|ZENITH_RT_SAFE|ZENITH_RT_REGION_BEGIN" "$file" 2>/dev/null; then
        return 0
    fi

    local file_violations=0

    for entry in "${FORBIDDEN_PATTERNS[@]}"; do
        local category="${entry%%|*}"
        local pattern="${entry#*|}"

        # Scan non-comment lines, also excluding ZENITH_RT_ALLOWLIST suppressions:
        while IFS= read -r match; do
            local lineno="${match%%:*}"
            local text="${match#*:}"
            printf "VIOLATION  %s:%s  [%s]  %s\n" "$file" "$lineno" "$category" "$text"
            file_violations=$((file_violations + 1))
        done < <(
            grep -nE "$pattern" "$file" 2>/dev/null \
            | grep -vE "^[0-9]+:[[:space:]]*(//|/\*|\*)" \
            | grep -v "ZENITH_RT_ALLOWLIST" \
            || true
        )
    done

    return $file_violations
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
total_violations=0

while IFS= read -r -d '' src_file; do
    check_file "$src_file" || total_violations=$((total_violations + $?))
done < <(
    for path in "${SEARCH_PATHS[@]}"; do
        if [[ -f "$path" ]]; then
            printf '%s\0' "$path"
        elif [[ -d "$path" ]]; then
            find "$path" \( -name "*.cpp" -o -name "*.h" -o -name "*.cc" \) -print0 2>/dev/null
        fi
    done
)

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
if [[ $total_violations -eq 0 ]]; then
    echo "✅  RT-safety check passed: no forbidden constructs found in RT-annotated code."
    exit 0
else
    echo "❌  RT-safety check FAILED: $total_violations forbidden construct(s) found in RT-annotated code."
    echo "    Review the VIOLATION lines above and either:"
    echo "      1. Remove the forbidden construct, or"
    echo "      2. Move it out of the RT path (before ZENITH_RT_REGION_BEGIN or into a non-RT function)."
    echo "      3. Add '// ZENITH_RT_ALLOWLIST: <reason>' to suppress a known-safe usage."
    echo ""
    echo "    See docs/RT_SAFETY.md for guidance."
    if [[ "$WARN_ONLY" == "true" ]]; then
        exit 0
    else
        exit 1
    fi
fi
