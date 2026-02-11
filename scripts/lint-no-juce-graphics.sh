#!/bin/bash
# CI Linter: Fail on new juce::Graphics usage in UI code
# Only legacy/bridge files are allowed to use juce::Graphics
# New code must use Skia rendering

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
UI_DIR="$PROJECT_ROOT/apps/desktop/Source/ui"

# Legacy/bridge files allowed to use juce::Graphics
ALLOWED_PATTERNS=(
    "ui/legacy/"
    "ui/design-system/ZenithTheme.cpp"
    "ui/framework/SkiaComponent.h"
    "ui/framework/SkiaMainWindowIntegration.cpp"
    "ui/framework/SkiaMainWindowIntegration.h"
    "ui/common/MainWindow.cpp"
)

if [[ ! -d "$UI_DIR" ]]; then
    echo "Error: UI directory not found at $UI_DIR"
    exit 1
fi

echo "Checking for juce::Graphics usage in $UI_DIR..."

# Find all files containing juce::Graphics
violations=()

while IFS= read -r -d '' file; do
    rel_path="${file#$PROJECT_ROOT/apps/desktop/Source/}"
    
    # Check if file matches any allowed pattern
    allowed=false
    for pattern in "${ALLOWED_PATTERNS[@]}"; do
        if [[ "$rel_path" == $pattern* ]]; then
            allowed=true
            break
        fi
    done
    
    if [[ "$allowed" == false ]]; then
        # Check if file actually contains juce::Graphics
        if grep -q "juce::Graphics" "$file" 2>/dev/null; then
            violations+=("$rel_path")
        fi
    fi
done < <(find "$UI_DIR" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0)

if [[ ${#violations[@]} -gt 0 ]]; then
    echo ""
    echo "❌ ERROR: juce::Graphics usage detected in non-legacy files!"
    echo ""
    echo "The following files violate the Skia-only rendering policy:"
    echo ""
    for file in "${violations[@]}"; do
        echo "  • $file"
        # Show the offending lines
        full_path="$PROJECT_ROOT/apps/desktop/Source/$file"
        grep -n "juce::Graphics" "$full_path" 2>/dev/null | while read -r line; do
            echo "      $line"
        done
    done
    echo ""
    echo "New UI code must use Skia rendering. See docs/ARCHITECTURE.md for guidance."
    echo ""
    echo "Allowed legacy/bridge files:"
    for pattern in "${ALLOWED_PATTERNS[@]}"; do
        echo "  • $pattern"
    done
    exit 1
fi

echo "✓ No juce::Graphics violations found"
exit 0
