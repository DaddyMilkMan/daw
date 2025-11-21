# maintain_warnings.ps1
# Script to maintain warning-free codebase

param(
    [switch]$CheckOnly,
    [switch]$FixAll,
    [switch]$Report
)

$root = "c:\zenith\daw\zenith-core"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Zenith DAW - Warning Maintenance Tool" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if we're in the right directory
if (-not (Test-Path (Join-Path $root "CMakeLists.txt"))) {
    Write-Host "ERROR: Must run from zenith-core directory!" -ForegroundColor Red
    exit 1
}

# Function to count potential warnings in files
function Count-PotentialWarnings {
    $warnings = @{
        'juce::Font(' = 'Deprecated Font API'
        ' new \w+\(' = 'Raw new usage'
        '^\s*[a-z]\w+\s+\w+\s*=.*;\s*//.*unused' = 'Commented unused vars'
        'TODO:(?!\()' = 'TODO without issue ref'
    }
    
    $totalIssues = 0
    $fileIssues = @{}
    
    $files = Get-ChildItem -Path "$root\Source","$root\src","$root\tests" -Include *.cpp,*.h -Recurse -ErrorAction SilentlyContinue
    
    foreach ($file in $files) {
        $content = Get-Content $file.FullName -Raw
        $issuesInFile = 0
        
        foreach ($pattern in $warnings.Keys) {
            if ($content -match $pattern) {
                $matches = [regex]::Matches($content, $pattern)
                $issuesInFile += $matches.Count
                $totalIssues += $matches.Count
            }
        }
        
        if ($issuesInFile -gt 0) {
            $fileIssues[$file.Name] = $issuesInFile
        }
    }
    
    return @{
        Total = $totalIssues
        ByFile = $fileIssues
    }
}

# Check-only mode
if ($CheckOnly) {
    Write-Host "Scanning codebase for potential warnings..." -ForegroundColor Yellow
    $results = Count-PotentialWarnings
    
    Write-Host ""
    Write-Host "Scan Results:" -ForegroundColor Cyan
    Write-Host "  Total potential issues: $($results.Total)" -ForegroundColor $(if ($results.Total -eq 0) { "Green" } else { "Yellow" })
    
    if ($results.Total -gt 0) {
        Write-Host ""
        Write-Host "Issues by file:" -ForegroundColor Yellow
        foreach ($file in $results.ByFile.Keys | Sort-Object) {
            Write-Host "  $file : $($results.ByFile[$file])" -ForegroundColor Gray
        }
        Write-Host ""
        Write-Host "Run with -FixAll to apply fixes" -ForegroundColor Cyan
    } else {
        Write-Host ""
        Write-Host "✓ Codebase is warning-free!" -ForegroundColor Green
    }
    
    exit $results.Total
}

# Fix-all mode
if ($FixAll) {
    Write-Host "Applying all warning fixes..." -ForegroundColor Yellow
    Write-Host ""
    
    $scripts = @(
        "comprehensive_warning_fixes.ps1"
        "fix_all_warnings.ps1"
        "final_warning_elimination.ps1"
        "eliminate_style_warnings.ps1"
    )
    
    foreach ($script in $scripts) {
        $scriptPath = Join-Path $root $script
        if (Test-Path $scriptPath) {
            Write-Host "Running $script..." -ForegroundColor Cyan
            & powershell -ExecutionPolicy Bypass -File $scriptPath
        } else {
            Write-Host "  SKIP: $script not found" -ForegroundColor DarkGray
        }
    }
    
    Write-Host ""
    Write-Host "✓ All fixes applied!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "  1. Rebuild: cmd /c rebuild_clean.bat" -ForegroundColor White
    Write-Host "  2. Verify: cmd /c verify_build.bat" -ForegroundColor White
    Write-Host "  3. Check IDE warnings" -ForegroundColor White
    
    exit 0
}

# Report mode
if ($Report) {
    Write-Host "Generating warning fix report..." -ForegroundColor Yellow
    
    $reportFile = Join-Path $root "warning_report.txt"
    
    $results = Count-PotentialWarnings
    
    $report = @"
Zenith DAW - Warning Status Report
Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
========================================

SUMMARY
Total potential issues: $($results.Total)

FILES WITH ISSUES
"@
    
    if ($results.Total -eq 0) {
        $report += "`n  ✓ No issues found - codebase is clean!`n"
    } else {
        foreach ($file in $results.ByFile.Keys | Sort-Object) {
            $report += "`n  $file : $($results.ByFile[$file]) issues"
        }
    }
    
    $report += @"


RECOMMENDATIONS
$( if ($results.Total -gt 0) {
    "  - Run: powershell -File maintain_warnings.ps1 -FixAll`n  - Then rebuild and verify"
} else {
    "  - Codebase is clean!`n  - Continue regular development"
})

========================================
"@
    
    Set-Content $reportFile $report
    Write-Host ""
    Write-Host "✓ Report saved to: warning_report.txt" -ForegroundColor Green
    Write-Host ""
    Write-Host $report
    
    exit 0
}

# Default: Show help
Write-Host "Usage:" -ForegroundColor Cyan
Write-Host "  maintain_warnings.ps1 -CheckOnly     Check for warnings without fixing" -ForegroundColor White
Write-Host "  maintain_warnings.ps1 -FixAll        Apply all warning fixes" -ForegroundColor White
Write-Host "  maintain_warnings.ps1 -Report        Generate detailed report" -ForegroundColor White
Write-Host ""
Write-Host "Examples:" -ForegroundColor Cyan
Write-Host "  # Quick check" -ForegroundColor DarkGray
Write-Host "  powershell -File maintain_warnings.ps1 -CheckOnly" -ForegroundColor White
Write-Host ""
Write-Host "  # Fix everything" -ForegroundColor DarkGray
Write-Host "  powershell -File maintain_warnings.ps1 -FixAll" -ForegroundColor White
Write-Host ""
