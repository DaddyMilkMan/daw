# Automated Skia Conversion Script
# Converts ALL major components to use native Skia rendering

$ErrorActionPreference = "Stop"

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "  Zenith DAW - Mass Skia Conversion Script" -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host ""

# Priority components to convert (highest impact first)
$componentsToConvert = @(
    @{ Name="MixerComponent"; Header="include/MixerComponent.h"; Cpp="src/MixerComponent.cpp" },
    @{ Name="ArrangerComponent"; Header="Source/ui/ArrangerComponent.h"; Cpp="Source/ui/ArrangerComponent.cpp" },
    @{ Name="TransportControlComponent"; Header="Source/ui/TransportControlComponent.h"; Cpp="Source/ui/TransportControlComponent.cpp" },
    @{ Name="MasterOutputComponent"; Header="Source/ui/MasterOutputComponent.h"; Cpp="src/MasterOutputComponent.cpp" },
    @{ Name="PianoRollComponent"; Header="Source/ui/PianoRollComponent.h"; Cpp="Source/ui/PianoRollComponent.cpp" },
    @{ Name="ClipComponent"; Header="include/ui/ClipComponent.h"; Cpp="src/ui/ClipComponent.cpp" },
    @{ Name="TrackHeaderComponent"; Header="include/ui/TrackHeaderComponent.h"; Cpp="src/ui/TrackHeaderComponent.cpp" },
    @{ Name="MixerChannelComponent"; Header="include/ui/MixerChannelComponent.h"; Cpp="src/ui/MixerChannelComponent.cpp" }
)

$skiaIncludes = @'
#ifdef ZENITH_USE_SKIA
    #include "../Source/ui/skia/SkiaComponent.h"
    class SkCanvas;
    struct SkRect;
#endif
'@

$skiaInheritance = @'
#ifdef ZENITH_USE_SKIA
                       , public zenith::SkiaComponent
#endif
'@

$skiaMethods = @'

#ifdef ZENITH_USE_SKIA
    /**
     * @brief Native Skia rendering (GPU accelerated!)
     */
    void paintToSkia(SkCanvas* canvas, SkRect bounds) override;
    
    /**
     * @brief Tell the system we support Skia
     */
    bool supportsSkiaRendering() const override { return true; }
#endif
'@

$cppSkiaIncludes = @'

#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkRRect.h>
    #include <include/effects/SkGradientShader.h>
#endif
'@

function Add-SkiaToHeader {
    param(
        [string]$FilePath,
        [string]$ClassName
    )
    
    if (!(Test-Path $FilePath)) {
        Write-Host "  ⚠ Skipping $ClassName - file not found: $FilePath" -ForegroundColor Yellow
        return $false
    }
    
    $content = Get-Content $FilePath -Raw
    
    # Check if already converted
    if ($content -match "SkiaComponent") {
        Write-Host "  ✓ $ClassName already has Skia support" -ForegroundColor Gray
        return $true
    }
    
    # Add Skia forward declarations after #pragma once or first #include
    if ($content -match "#pragma once") {
        $content = $content -replace "(#pragma once)", "`$1`n`n$skiaIncludes"
    }
    
    # Add SkiaComponent inheritance
    $classPattern = "class $ClassName\s*:\s*public juce::Component"
    if ($content -match $classPattern) {
        $content = $content -replace "($classPattern)", "`$1`n$skiaInheritance"
    }
    
    # Add paintToSkia methods before private: section
    if ($content -match "private:") {
        $content = $content -replace "(private:)", "$skiaMethods`n`n`$1"
    } elseif ($content -match "protected:") {
        $content = $content -replace "(protected:)", "$skiaMethods`n`n`$1"
    } else {
        # Add before closing brace of class
        $content = $content -replace "(JUCE_DECLARE_NON_COPYABLE)", "$skiaMethods`n`n    `$1"
    }
    
    Set-Content -Path $FilePath -Value $content -NoNewline
    Write-Host "  ✓ Added Skia support to header: $ClassName" -ForegroundColor Green
    return $true
}

function Add-SkiaToCpp {
    param(
        [string]$FilePath,
        [string]$ClassName
    )
    
    if (!(Test-Path $FilePath)) {
        Write-Host "  ⚠ Skipping implementation for $ClassName - file not found: $FilePath" -ForegroundColor Yellow
        return $false
    }
    
    $content = Get-Content $FilePath -Raw
    
    # Check if already has paintToSkia
    if ($content -match "void.*paintToSkia") {
        Write-Host "  ✓ $ClassName implementation already has paintToSkia()" -ForegroundColor Gray
        return $true
    }
    
    # Add Skia includes after other includes
    if ($content -match "#include.*\.h") {
        $lastInclude = [regex]::Matches($content, "#include.*\.h.*`n").Groups | Select-Object -Last 1
        if ($lastInclude) {
            $pos = $lastInclude.Index + $lastInclude.Length
            $content = $content.Insert($pos, $cppSkiaIncludes + "`n")
        }
    }
    
    # Create basic paintToSkia implementation
    $basicImpl = @"

#ifdef ZENITH_USE_SKIA

void ${ClassName}::paintToSkia(SkCanvas* canvas, SkRect bounds)
{
    // TODO: Convert JUCE paint() code to Skia
    // For now, draw a placeholder to show Skia is active
    
    // Background (copy from your paint() method)
    canvas->clear(0xFF1E1E1E);
    
    // Placeholder text
    SkFont font;
    font.setSize(14);
    font.setEdging(SkFont::Edging::kAntiAlias);
    
    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(128, 255, 255, 255));
    textPaint.setAntiAlias(true);
    
    const char* msg = "$ClassName (Skia)";
    canvas->drawString(msg, bounds.x() + 10, bounds.y() + 20, font, textPaint);
    
    // TODO: Add your actual drawing code here
    // See SKIA_QUICK_REFERENCE.md for examples
}

#endif  // ZENITH_USE_SKIA
"@
    
    # Append to end of file
    $content += "`n" + $basicImpl
    
    Set-Content -Path $FilePath -Value $content -NoNewline
    Write-Host "  ✓ Added paintToSkia() stub to: $ClassName" -ForegroundColor Green
    return $true
}

# Main conversion loop
$successCount = 0
$totalCount = $componentsToConvert.Count

Write-Host "Converting $totalCount components to Skia...`n" -ForegroundColor Cyan

foreach ($component in $componentsToConvert) {
    Write-Host "[$($successCount + 1)/$totalCount] Converting: $($component.Name)" -ForegroundColor White
    
    $headerPath = Join-Path $PSScriptRoot $component.Header
    $cppPath = Join-Path $PSScriptRoot $component.Cpp
    
    $headerOk = Add-SkiaToHeader -FilePath $headerPath -ClassName $component.Name
    $cppOk = Add-SkiaToCpp -FilePath $cppPath -ClassName $component.Name
    
    if ($headerOk -and $cppOk) {
        $successCount++
    }
    
    Write-Host ""
}

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "  Conversion Complete!" -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "  Successfully converted: $successCount/$totalCount components" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Review the TODO comments in each paintToSkia() method"
Write-Host "  2. Convert JUCE drawing code to Skia using SKIA_QUICK_REFERENCE.md"
Write-Host "  3. Build and test: .\rebuild.bat"
Write-Host "  4. Check console for 'Native Skia rendering: X components'"
Write-Host ""
Write-Host "For full implementation examples, see:" -ForegroundColor Yellow
Write-Host "  - MIXER_SKIA_EXAMPLE.md"
Write-Host "  - SKIA_CONVERSION_GUIDE.md"
Write-Host ""
