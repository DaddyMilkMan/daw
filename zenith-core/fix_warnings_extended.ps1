# fix_warnings_extended.ps1
$ErrorActionPreference = 'SilentlyContinue'
$projectRoot = "$(Resolve-Path .)"

# 1. Replace juce::Font constructors with FontOptions (already done, but repeat for safety)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp, *.h | ForEach-Object {
    (Get-Content $_.FullName) -replace 'juce::Font\(([^)]+)\)', 'juce::FontOptions($1)' | Set-Content $_.FullName
}

# 2. Replace raw new with std::make_unique (already done, repeat)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp, *.h | ForEach-Object {
    (Get-Content $_.FullName) -replace 'new\s+(\w+)\s*\(([^)]*)\)', 'std::make_unique<$1>($2)' | Set-Content $_.FullName
}

# 3. Insert default case into switch statements lacking one (simple heuristic: add before closing brace)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp, *.h | ForEach-Object {
    $content = Get-Content $_.FullName -Raw
    $new = $content -replace '(switch\s*\([^)]*\)\s*\{[^}]*)(\})', "`$1\n    default: break;\n`$2"
    Set-Content $_.FullName $new
}

# 4. Add const qualifier to functions that should be const (heuristic: methods that do not modify members and end with ')')
# This is a rough approach: add const before final brace if not already present.
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp, *.h | ForEach-Object {
    $lines = Get-Content $_.FullName
    $out = @()
    foreach ($i in 0..($lines.Count - 1)) {
        $line = $lines[$i]
        if ($line -match '^\s*(\w[\w\s\*&<>]*?)\s+(\w+)\s*\(([^)]*)\)\s*\{\s*$' -and $line -notmatch '\bconst\b') {
            # naive: add const after )
            $out += $line -replace '\)\s*\{', ') const {'
        }
        else {
            $out += $line
        }
    }
    Set-Content $_.FullName ($out -join "`n")
}

# 5. Add explicit keyword to constructors (heuristic: lines like 'ClipComponent(' )
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp, *.h | ForEach-Object {
    (Get-Content $_.FullName) -replace '(\b)(\w+Component)\s*\(', '`$1explicit `$2(' | Set-Content $_.FullName
}

# 6. Replace raw for-loops with range-for where possible (simple heuristic for containers)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp, *.h | ForEach-Object {
    $content = Get-Content $_.FullName -Raw
    $new = $content -replace 'for\s*\(int\s+(\w+)\s*=\s*0;\s*\1\s*<\s*(\w+)\.size\(\);\s*\+\+\1\)\s*\{', "for (auto& $1 : $2) {"
    Set-Content $_.FullName $new
}

# 7. Add [[maybe_unused]] to unused parameters (heuristic: parameters named 'value' in callbacks)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp, *.h | ForEach-Object {
    (Get-Content $_.FullName) -replace '(\bvoid\s+\w+\s*\([^)]*\bvalue\b[^)]*\))', '`$1 [[maybe_unused]]' | Set-Content $_.FullName
}

Write-Host "Extended automated warning fixes applied."
