# Clean Build Logs Script
# Removes all build log files from the repository
# Run this script to clean up build logs before committing

Write-Host "Cleaning build log files..." -ForegroundColor Cyan

$filesRemoved = 0
$totalSize = 0

# Build log patterns to remove
$patterns = @(
    "build_errors*.txt",
    "build_log*.txt",
    "build_verbose.txt",
    "build_diag.txt",
    "cmake_error*.log",
    "msbuild_*.log",
    "*.log"
)

foreach ($pattern in $patterns) {
    $files = Get-ChildItem -Path . -Filter $pattern -Recurse -ErrorAction SilentlyContinue | 
             Where-Object { $_.FullName -notlike "*\.git\*" -and $_.FullName -notlike "*\modules\*" }
    
    foreach ($file in $files) {
        $size = $file.Length
        Write-Host "  Removing: $($file.Name) ($([math]::Round($size/1KB, 2)) KB)" -ForegroundColor Yellow
        Remove-Item $file.FullName -Force
        $filesRemoved++
        $totalSize += $size
    }
}

Write-Host ""
Write-Host "Summary:" -ForegroundColor Green
Write-Host "  Files removed: $filesRemoved" -ForegroundColor White
Write-Host "  Total size freed: $([math]::Round($totalSize/1MB, 2)) MB" -ForegroundColor White
Write-Host ""
Write-Host "Note: These files are now in .gitignore and won't be committed in the future." -ForegroundColor Cyan
