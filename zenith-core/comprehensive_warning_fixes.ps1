# comprehensive_warning_fixes.ps1
$ErrorActionPreference = 'Continue'
$projectRoot = "c:\zenith\daw\zenith-core"

Write-Host "Starting comprehensive warning fixes..."

# Get all source files
$files = Get-ChildItem -Path "$projectRoot\Source","$projectRoot\src","$projectRoot\tests" -Include *.cpp,*.h -Recurse

foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw
    $modified = $false
    
    # 1. Fix Font deprecation (juce::Font( -> juce::FontOptions()
    if ($content -match 'juce::Font\s*\(') {
        $content = $content -replace 'juce::Font\s*\(([^)]+)\)', 'juce::FontOptions($1)'
        $modified = $true
        Write-Host "  Fixed Font in: $($file.Name)"
    }
    
    # 2. Comment out unused variables
    $unusedVars = @('row3Height', 'relativeY', 'oldPixelsPerBeat', 'centerPitch', 'ATTACK', 'newStartSamples')
    foreach ($var in $unusedVars) {
        if ($content -match "\s+$var\s*=") {
            $content = $content -replace "(\s+)((?:int|float|double|auto)\s+$var\s*=[^;]+;)", "`$1// `$2  // Unused variable"
            $modified = $true
            Write-Host "  Commented unused var '$var' in: $($file.Name)"
        }
    }
    
    # 3. Replace 'new' with std::make_unique for simple cases
    if ($content -match '\bnew\s+\w+\s*\(') {
        # Only replace simple patterns to avoid breaking complex code
        $content = $content -replace '=\s*new\s+(\w+)\s*\(\)', '= std::make_unique<$1>()'
        $content = $content -replace '=\s*new\s+(\w+)\s*\(([^,\)]+)\)', '= std::make_unique<$1>($2)'
        $modified = $true
        Write-Host "  Replaced 'new' in: $($file.Name)"
    }
    
    # 4. Replace redundant types with auto for iterators
    if ($content -match 'juce::Rectangle<int>') {
        $content = $content -replace '(juce::Rectangle<int>)\s+(\w+)\s*=', 'auto $2 ='
        $modified = $true
    }
    
    # 5. Add default case to switches (only if missing)
    if ($content -match 'switch\s*\([^)]+\)\s*\{[^}]*\}\s*(?!.*default:)') {
        # This is tricky - only add if there's no default already
        $content = $content -replace '(case\s+\d+:[^}]+break;\s*)(\})', "`$1    default:`n        break;`n`$2"
        $modified = $true
        Write-Host "  Added default case in: $($file.Name)"
    }
    
    # Save if modified
    if ($modified) {
        Set-Content $file.FullName $content -NoNewline
        Write-Host "✓ Updated: $($file.Name)"
    }
}

Write-Host "`nPhase 1 complete - basic fixes applied"
Write-Host "Now applying const-correctness fixes..."

# Phase 2: Add const to paint/drawing functions
foreach ($file in $files) {
    if ($file.Extension -ne '.cpp') { continue }
    
    $lines = Get-Content $file.FullName
    $modified = $false
    
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        
        # Add const to paint/draw functions that should have it
        if ($line -match '^(\s*void\s+\w+)\s*\(juce::Graphics&\s+g,?\s*.*\)\s*$' -and $line -notmatch '\bconst\b') {
            $lines[$i] = $line -replace '\)\s*$', ') const'
            $modified = $true
        }
        
        # Add const to getter-style functions
        if ($line -match '^(\s*\w+\s+get\w+)\s*\(\s*\)\s*$' -and $line -notmatch '\bconst\b' -and $line -notmatch '\bvoid\b') {
            $lines[$i] = $line -replace '\)\s*$', ') const'
            $modified = $true
        }
    }
    
    if ($modified) {
        Set-Content $file.FullName ($lines -join "`n")
        Write-Host "✓ Added const to: $($file.Name)"
    }
}

Write-Host "`nAll fixes applied!"
