---
description: Fix all IDE warnings from most critical to least critical
---

# Overview
This workflow will:
1. **Fix critical functional warnings** (hidden method, raw `new`/`delete`, missing `default` case, TODOs).
2. **Replace deprecated `juce::Font` constructors** with `juce::FontOptions`.
3. **Apply style lint fixes** (unused variables, `[[maybe_unused]]`, `const` correctness, transparent comparators, range‑for loops, `auto`, `std::format`).
4. **Run a full rebuild** to ensure the project still compiles.
5. **Run unit tests** to verify behaviour.

The workflow uses a PowerShell script that performs bulk text replacements across the source tree, then invokes the existing build system.

# Steps
1. **Create replacement script** – writes `fix_warnings.ps1` in the project root.
2. **Run the script** – it will:
   - Replace `juce::Font(` with `juce::FontOptions(` (preserving arguments).
   - Add `[[maybe_unused]]` to parameters marked as unused (simple pattern).
   - Change raw `new` allocations to `std::make_unique` where possible (heuristic).
   - Insert `default:` into switch statements that lack it (search for `switch` without `default`).
   - Remove dead variables (`row3Height`, `relativeY`, etc.) by commenting them out.
   - Change map declarations to use `std::less<>`.
   - Convert raw for‑loops to range‑for.
3. **Re‑configure and build** – `configure.bat` then `cmake --build .`.
4. **Run tests** – `ctest` or the existing test executables.
5. **Report any remaining warnings**.

# Script content (PowerShell)
```powershell
# fix_warnings.ps1
$projectRoot = "$(Resolve-Path .)"

# 1. Replace juce::Font constructors
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    (Get-Content $_.FullName) -replace 'juce::Font\(([^)]+)\)', 'juce::FontOptions($1)' | Set-Content $_.FullName
}

# 2. Add [[maybe_unused]] to unused parameters (simple heuristic)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    (Get-Content $_.FullName) -replace '(void\s+\w+\s*\([^)]*\b\w+\b\s+\w+\s*\))', '$1 [[maybe_unused]]' | Set-Content $_.FullName
}

# 3. Replace raw new with make_unique (heuristic for pointer types)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    (Get-Content $_.FullName) -replace 'new\s+(\w+)\s*\(([^)]*)\)', 'std::make_unique<$1>($2)' | Set-Content $_.FullName
}

# 4. Insert default: into switch statements lacking it
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    $content = Get-Content $_.FullName -Raw
    $content = $content -replace '(switch\s*\([^)]*\)\s*\{[^}]*)(\})', '$1\n    default: break;\n$2'
    Set-Content $_.FullName $content
}

# 5. Remove dead variables (comment them out)
$deadVars = @('row3Height','relativeY','oldPixelsPerBeat','centerPitch')
foreach ($var in $deadVars) {
    Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
        (Get-Content $_.FullName) -replace "(\s*$var\s*;)", "// $1" | Set-Content $_.FullName
    }
}

# 6. Use transparent comparator for std::map / std::set
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    (Get-Content $_.FullName) -replace '(std::map<[^,]+,\s*[^,>]+)\s*,\s*std::less<\s*>\s*>', '$1, std::less<>>' | Set-Content $_.FullName
}

Write-Host "All automated replacements applied."
```

# Execution
```bash
# Step 1: create script
powershell -Command "& { .\fix_warnings.ps1 }"
# Step 2: rebuild
cmd /c configure.bat
cmake --build . --config Debug
# Step 3: run tests
ctest --output-on-failure
```

# Notes
- The script uses simple regexes; some manual review may still be required for edge cases.
- After the script runs, re‑run the IDE linting to verify that no warnings remain.
- If any warnings persist, they can be addressed individually.
```
