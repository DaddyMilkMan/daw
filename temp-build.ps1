# Temporary build script
Write-Host "=== Starting Clean Build ==="

# Remove build directory
if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
    Write-Host "Removed old build directory"
}

# Create build directory
New-Item -ItemType Directory -Path "build" | Out-Null
Write-Host "Created build directory"

# Load VS Developer environment
Import-Module "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "C:\Program Files\Microsoft Visual Studio\18\Community" -SkipAutomaticLocation
Write-Host "Loaded VS 2026 environment"

# Configure with CMake
Set-Location "build"
Write-Host "Running CMake configure..."
& "C:\Program Files\CMake\bin\cmake.exe" -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed"
    exit 1
}

Write-Host "Running Ninja build..."
ninja -j8
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed"
    exit 1
}

Write-Host "=== Build Complete ==="
