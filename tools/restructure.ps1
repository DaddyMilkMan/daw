# -- START OF OPERATION CLEAN SWEEP --
Write-Host "Initializing Citadel Protocol..." -ForegroundColor Cyan

# 1. Create the Citadel Skeleton
Write-Host "Creating directory structure..."
New-Item -ItemType Directory -Force -Path "apps/desktop" | Out-Null
New-Item -ItemType Directory -Force -Path "services" | Out-Null
New-Item -ItemType Directory -Force -Path "packages" | Out-Null
New-Item -ItemType Directory -Force -Path "cmake" | Out-Null
New-Item -ItemType Directory -Force -Path "docs/planning" | Out-Null
New-Item -ItemType Directory -Force -Path "docs/reports" | Out-Null
New-Item -ItemType Directory -Force -Path "tools" | Out-Null
New-Item -ItemType Directory -Force -Path "tests" | Out-Null

# 2. Relocate Core Application (The Desktop App)
# Moving 'zenith-core' to 'apps/desktop'
if (Test-Path "zenith-core") {
    Write-Host "Relocating zenith-core to apps/desktop..."
    # Using git mv for tracking. 
    # We move the *contents* of zenith-core to apps/desktop
    # Get-ChildItem "zenith-core" | ForEach-Object { git mv $_.FullName "apps/desktop/" }
    # PowerShell/git interaction can be tricky with wildcards, doing loop
    $items = Get-ChildItem "zenith-core"
    foreach ($item in $items) {
        git mv $item.FullName "apps/desktop/"
    }
    
    # If git mv leaves the empty folder, remove it
    if ((Get-ChildItem "zenith-core").Count -eq 0) {
        Remove-Item "zenith-core" -Force -Recurse -ErrorAction SilentlyContinue
    }
} else {
    Write-Host "WARNING: zenith-core folder not found. Skipping move." -ForegroundColor Yellow
}

# 3. Relocate Microservice (AI Bridge)
# Ensuring services/ai-bridge exists
if (Test-Path "services/ai-bridge") {
    Write-Host "AI Bridge is already in services. Good."
} else {
    # Check if it exists elsewhere, otherwise alert
    Write-Host "NOTE: Verify location of AI Bridge source." -ForegroundColor Yellow
}

# 4. Clean up the Root (Documentation & Clutter)
Write-Host "Sweeping Markdown files to /docs..."
Get-ChildItem -Path . -Filter "*.md" | Where-Object { $_.Name -ne "README.md" } | ForEach-Object {
    git mv $_.Name docs/
}

# 5. Remove Build Artifact Pollution
Write-Host "Purging temporary build directories..."
Remove-Item "build" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "build_modern_verification" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "build_cmake_test" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "test_cmake" -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "OPERATION CLEAN SWEEP COMPLETE." -ForegroundColor Green
Write-Host "NOTE: Your build system is now BROKEN until we execute Option B." -ForegroundColor Red
