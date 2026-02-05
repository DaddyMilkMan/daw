#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
ALLOWLIST_FILE="$ROOT_DIR/tools/lint/juce_graphics_allowlist.txt"

if [[ ! -f "$ALLOWLIST_FILE" ]]; then
  echo "Allowlist not found: $ALLOWLIST_FILE" >&2
  echo "Generate it with: rg -n \"juce::Graphics\" $ROOT_DIR/apps/desktop/Source > $ALLOWLIST_FILE" >&2
  exit 1
fi

CURRENT="$ROOT_DIR/tools/lint/juce_graphics_current.txt"
ALLOW_SORTED="$ROOT_DIR/tools/lint/juce_graphics_allowlist_sorted.txt"
CURRENT_SORTED="$ROOT_DIR/tools/lint/juce_graphics_current_sorted.txt"
trap 'rm -f "$CURRENT" "$ALLOW_SORTED" "$CURRENT_SORTED"' EXIT
(cd "$ROOT_DIR" && rg -n "juce::Graphics" apps/desktop/Source > "$CURRENT") || true

sort "$ALLOWLIST_FILE" > "$ALLOW_SORTED"
sort "$CURRENT" > "$CURRENT_SORTED"

if ! diff -u "$ALLOW_SORTED" "$CURRENT_SORTED"; then
  echo "" >&2
  echo "New or removed juce::Graphics usages detected." >&2
  echo "If intentional, update the allowlist:" >&2
  echo "  (cd $ROOT_DIR && rg -n \"juce::Graphics\" apps/desktop/Source > $ALLOWLIST_FILE)" >&2
  exit 1
fi
