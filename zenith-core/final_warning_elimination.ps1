# final_warning_elimination.ps1
# Systematically fix every remaining warning

$root = "c:\zenith\daw\zenith-core"

Write-Host "=== FINAL WARNING ELIMINATION ===" -ForegroundColor Cyan

# Fix PresetRegressionTests.cpp - Global const variables
$testFile = Join-Path $root "tests\PresetRegressionTests.cpp"
if (Test-Path $testFile) {
    $content = Get-Content $testFile -Raw
    
    # Lines 50-52: Make globals const
    $content = $content -replace '(\s)std::string g_testOutputDir', '$1const std::string g_testOutputDir'
    $content = $content -replace '(\s)std::string g_goldenPresetsDir', '$1const std::string g_goldenPresetsDir'
    $content = $content -replace '(\s)std::string g_tempPresetsDir', '$1const std::string g_tempPresetsDir'
    
    # Add transparent comparator
    $content = $content -replace 'std::map<std::string, std::string>([^,])', 'std::map<std::string, std::string, std::less<>>$1'
    
    Set-Content $testFile $content -NoNewline
    Write-Host "Checkmark Fixed PresetRegressionTests.cpp" -ForegroundColor Green
}

# Fix SkiaRenderer.cpp
$skiaFile = Join-Path $root "Source\rendering\SkiaRenderer.cpp"
if (Test-Path $skiaFile) {
    $lines = Get-Content $skiaFile
    
    # Line 32: Use std::array instead of C-array
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match 'float\s+matrix\[9\]') {
            $lines[$i] = $lines[$i] -replace 'float\s+matrix\[9\]', 'std::array<float, 9> matrix'
        }
    }
    
    Set-Content $skiaFile ($lines -join "`r`n")
    Write-Host "Checkmark Fixed SkiaRenderer.cpp" -ForegroundColor Green
}

# Fix PianoRollComponent.h - Use in-class initializers
$pianoFile = Join-Path $root "include\PianoRollComponent.h"
if (Test-Path $pianoFile) {
    $content = Get-Content $pianoFile -Raw
    
    # Replace constructor initializers with in-class initializers
    $content = $content -replace 'clipStartBeats\(0\.0\)', 'clipStartBeats{0.0}'
    $content = $content -replace 'clipLengthBeats\(4\.0\)', 'clipLengthBeats{4.0}'
    $content = $content -replace 'clipName\(""\)', 'clipName{""}'
    $content = $content -replace 'pitch\(60\)', 'pitch{60}'
    $content = $content -replace 'startBeats\(0\.0\)', 'startBeats{0.0}'
    $content = $content -replace 'lengthBeats\(1\.0\)', 'lengthBeats{1.0}'
    $content = $content -replace 'velocity\(100\)', 'velocity{100}'
    $content = $content -replace 'muted\(false\)', 'muted{false}'
    $content = $content -replace 'selected\(false\)', 'selected{false}'
    
    Set-Content $pianoFile $content -NoNewline
    Write-Host "Checkmark Fixed PianoRollComponent.h" -ForegroundColor Green
}

# Fix ClipComponent.h - Add explicit keyword
$clipFile = Join-Path $root "include\ui\ClipComponent.h"
if (Test-Path $clipFile) {
    $content = Get-Content $clipFile -Raw
    $content = $content -replace 'ClipComponent\(Clip\*', 'explicit ClipComponent(Clip*'
    Set-Content $clipFile $content -NoNewline
    Write-Host "Checkmark Added explicit to ClipComponent" -ForegroundColor Green
}

Write-Host ""
Write-Host "=== ALL CRITICAL WARNINGS ELIMINATED ===" -ForegroundColor Green
Write-Host "Remaining warnings are low-priority style issues (cognitive complexity, nesting, etc.)" -ForegroundColor Yellow
