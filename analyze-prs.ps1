$prs = @(47,48,50,51,52,53,54,55,56,63,65,69,70,73,74,75,76,77,78,79,80,81,82,84,85,88)

foreach ($pr in $prs) {
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "PR #$pr" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Cyan
    
    # Get commit message
    $commit = git log pr-$pr --oneline -1
    Write-Host "Latest commit: $commit" -ForegroundColor White
    
    # Get file changes
    $files = git diff --name-only origin/master...pr-$pr
    Write-Host "`nFiles changed:" -ForegroundColor Green
    $files | ForEach-Object { Write-Host "  $_" }
    
    # Check for web app files
    $webFiles = $files | Where-Object { $_ -match "\.tsx?$|zenith-daw/|electron|package\.json" }
    if ($webFiles) {
        Write-Host "`n[SKIP - WEB APP FILES]" -ForegroundColor Red
    }
    
    # Check for docs only
    $nonDocFiles = $files | Where-Object { $_ -notmatch "^docs/|\.md$|README" }
    if ($nonDocFiles.Count -eq 0 -and $files.Count -gt 0) {
        Write-Host "`n[SKIP - DOCS ONLY]" -ForegroundColor Red
    }
    
    # Check for C++/JUCE files
    $cppFiles = $files | Where-Object { $_ -match "\.(cpp|h)$|CMakeLists\.txt" }
    if ($cppFiles) {
        Write-Host "`n[MERGE - C++/JUCE CODE]" -ForegroundColor Green
    }
}
