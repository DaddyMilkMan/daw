# fix_warnings.ps1
$projectRoot = "$(Resolve-Path .)"

# 1. Replace juce::Font constructors with FontOptions
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    (Get-Content $_.FullName) -replace 'juce::Font\(([^)]+)\)', 'juce::FontOptions($1)' | Set-Content $_.FullName
}

# 2. Add [[maybe_unused]] to unused parameters (simple heuristic)
# This will prepend the attribute to any parameter named "value" that is not used (very naive)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    (Get-Content $_.FullName) -replace '(\bvoid\s+\w+\s*\([^)]*\b\w+\b\s+\w+\s*\))', '$1 [[maybe_unused]]' | Set-Content $_.FullName
}

# 3. Replace raw new with std::make_unique where possible (heuristic)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    (Get-Content $_.FullName) -replace 'new\s+(\w+)\s*\(([^)]*)\)', 'std::make_unique<$1>($2)' | Set-Content $_.FullName
}

# 4. Insert default: into switch statements lacking it (very simple heuristic)
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    $content = Get-Content $_.FullName -Raw
    if ($content -match 'switch\s*\([^)]*\)\s*\{[^}]*\}') {
        $new = $content -replace '(switch\s*\([^)]*\)\s*\{[^}]*)(\})', "`$1\n    default: break;\n`$2"
        Set-Content $_.FullName $new
    }
}

# 5. Comment out dead/unused variables (list known names)
$deadVars = @('row3Height','relativeY','oldPixelsPerBeat','centerPitch','ATTACK')
foreach ($var in $deadVars) {
    Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
        (Get-Content $_.FullName) -replace "(\s*$var\s*;)", "// $1" | Set-Content $_.FullName
    }
}

# 6. Use transparent comparator std::less<> for std::map/std::set declarations
Get-ChildItem -Path $projectRoot -Recurse -Include *.cpp,*.h | ForEach-Object {
    (Get-Content $_.FullName) -replace '(std::map<[^,]+,\s*[^,>]+)\s*,\s*std::less<\s*>\s*>', '$1, std::less<>>' | Set-Content $_.FullName
}

Write-Host "Automated warning fixes applied."
