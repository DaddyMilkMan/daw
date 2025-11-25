# Automated Skia Conversion Script - FIXED
# Converts ALL major components to use native Skia rendering

$ErrorActionPreference = "Stop"

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "  Zenith DAW - Mass Skia Conversion Script (Fixed)" -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan

# Full component list
$componentsToConvert = @(
    @{ Name="MixerComponent"; Header="include/MixerComponent.h"; Cpp="src/MixerComponent.cpp" },
    @{ Name="ArrangerComponent"; Header="Source/ui/ArrangerComponent.h"; Cpp="Source/ui/ArrangerComponent.cpp" },
    @{ Name="TransportControlComponent"; Header="Source/ui/TransportControlComponent.h"; Cpp="Source/ui/TransportControlComponent.cpp" },
    @{ Name="MasterOutputComponent"; Header="Source/ui/MasterOutputComponent.h"; Cpp="Source/ui/MasterOutputComponent.cpp" },
    @{ Name="PianoRollComponent"; Header="Source/ui/PianoRollComponent.h"; Cpp="Source/ui/PianoRollComponent.cpp" }
)

$skiaIncludes = @"
#ifdef ZENITH_USE_SKIA
    #include "../Source/ui/skia/SkiaComponent.h"
    class SkCanvas;
    struct SkRect;
#endif
"@

$skiaInheritance = @"
#ifdef ZENITH_USE_SKIA
                       , public zenith::SkiaComponent
#endif
"@

$skiaMethods = @"
#ifdef ZENITH_USE_SKIA
    void paintToSkia(SkCanvas* canvas, SkRect bounds) override;
    bool supportsSkiaRendering() const override { return true; }
#endif
"@

$cppSkiaIncludes = @"
#ifdef ZENITH_USE_SKIA
    #include <include/core/SkCanvas.h>
    #include <include/core/SkPaint.h>
    #include <include/core/SkFont.h>
    #include <include/core/SkPath.h>
    #include <include/core/SkRRect.h>
    #include <include/effects/SkGradientShader.h>
    #include "../Source/ui/skia/SkiaTheme.h"
#endif
"@

function Add-SkiaToHeader {
    param( [string]$FilePath, [string]$ClassName )

    if (!(Test-Path $FilePath)) {
        Write-Host "  WARNING: Header file not found: $FilePath" -ForegroundColor Yellow
        return $false
    }

    $content = Get-Content $FilePath -Raw

    if ($content -match "SkiaComponent") {
        Write-Host "  Header already has Skia support" -ForegroundColor Green
        return $true
    }

    # Inject Includes
    if ($content -match "#pragma once") {
        $content = $content.Replace("#pragma once", "#pragma once`n`n$skiaIncludes")
    }

    # Inject Inheritance
    $content = $content -replace "(class\s+$ClassName\s*:\s*public\s+juce::Component)", "`$1$skiaInheritance"

    # Inject Methods
    if ($content -match "private:") {
        $content = $content.Replace("private:", "$skiaMethods`n`nprivate:")
    } else {
        # Fallback if no private section
        $content = $content -replace "(\};)", "$skiaMethods`n`$1"
    }

    Set-Content -Path $FilePath -Value $content -NoNewline
    Write-Host "  Header updated successfully" -ForegroundColor Green
    return $true
}

function Add-SkiaToCpp {
    param( [string]$FilePath, [string]$ClassName )

    if (!(Test-Path $FilePath)) {
        Write-Host "  WARNING: CPP file not found: $FilePath" -ForegroundColor Yellow
        return $false
    }

    $content = Get-Content $FilePath -Raw

    if ($content -match "void.*paintToSkia") {
        Write-Host "  CPP already has Skia implementation" -ForegroundColor Green
        return $true
    }

    # Add Includes
    if ($content -match "#include.*$ClassName\.h.*") {
        $content = $content -replace "(#include.*$ClassName\.h.*)", "`$1`n$cppSkiaIncludes"
    }

    # Add Implementation Stub
    $stub = @"

#ifdef ZENITH_USE_SKIA
void $ClassName::paintToSkia(SkCanvas* canvas, SkRect bounds)
{
    auto& theme = zenith::SkiaTheme::getInstance();
    canvas->clear(theme.getColors().bg1);

    SkPaint p;
    p.setColor(theme.getColors().textStrong);
    // TODO: Implement custom Skia rendering for $ClassName
}
#endif
"@
    $content += $stub
    Set-Content -Path $FilePath -Value $content -NoNewline
    Write-Host "  CPP updated successfully" -ForegroundColor Green
    return $true
}

Write-Host ""
Write-Host "Processing components..." -ForegroundColor Yellow
Write-Host ""

$successCount = 0
$failureCount = 0

foreach ($c in $componentsToConvert) {
    Write-Host "[$($c.Name)]" -ForegroundColor Cyan

    $hPath = Join-Path (Get-Location) $c.Header
    $cPath = Join-Path (Get-Location) $c.Cpp

    $hSuccess = Add-SkiaToHeader -FilePath $hPath -ClassName $c.Name
    $cSuccess = Add-SkiaToCpp -FilePath $cPath -ClassName $c.Name

    if ($hSuccess -and $cSuccess) {
        $successCount++
    } else {
        $failureCount++
    }

    Write-Host ""
}

Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "Conversion Summary" -ForegroundColor Cyan
Write-Host "===============================================" -ForegroundColor Cyan
Write-Host "Successful: $successCount" -ForegroundColor Green
Write-Host "Failed/Skipped: $failureCount" -ForegroundColor Yellow
Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Yellow
Write-Host "1. Review the updated component files"
Write-Host "2. Implement the paintToSkia() methods in each component"
Write-Host "3. Build and test the project"
Write-Host ""
Write-Host "Done." -ForegroundColor Green
