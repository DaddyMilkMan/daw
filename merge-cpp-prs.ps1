# PRs to merge (C++/JUCE only, skipping web app and docs)
$prsToMerge = @(51, 52, 53, 54, 55, 56, 65, 69, 70, 74, 76, 77, 79, 80, 82, 84, 85, 88)

foreach ($pr in $prsToMerge) {
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "Processing PR #$pr" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Cyan
    
    # Checkout PR branch
    Write-Host "Checking out pr-$pr..." -ForegroundColor White
    git checkout pr-$pr 2>&1 | Out-Null
    
    # Merge master into PR
    Write-Host "Merging master into pr-$pr..." -ForegroundColor White
    $mergeResult = git merge master --no-edit 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Conflicts detected, resolving with master's version..." -ForegroundColor Yellow
        
        # Get conflicted files
        $conflicted = git diff --name-only --diff-filter=U
        
        if ($conflicted) {
            # Accept master's version for all conflicts
            git checkout --theirs $conflicted 2>&1 | Out-Null
            git add -A 2>&1 | Out-Null
            git commit -m "Merge master into pr-$pr`: Resolve conflicts" 2>&1 | Out-Null
            Write-Host "Conflicts resolved" -ForegroundColor Green
        }
    } else {
        Write-Host "Merged cleanly" -ForegroundColor Green
    }
    
    # Merge PR into master
    Write-Host "Merging pr-$pr into master..." -ForegroundColor White
    git checkout master 2>&1 | Out-Null
    git merge pr-$pr --no-edit 2>&1 | Out-Null
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ PR #$pr successfully merged into master" -ForegroundColor Green
    } else {
        Write-Host "✗ PR #$pr failed to merge into master" -ForegroundColor Red
        break
    }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Merge process complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
git log --oneline -5
