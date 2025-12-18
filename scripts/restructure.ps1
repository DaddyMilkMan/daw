# restructure.ps1 - Zenith DAW Structure Enforcer
# Verifies that code stays in the modular apps/desktop/Source layout

$ErrorActionPreference = "Stop"

function Print-Header {
    param([string]$Title)
    Write-Host "`n=== $Title ===" -ForegroundColor Cyan
}

function Test-Directory {
    param([string]$Path)
    if (Test-Path $Path) {
        Write-Host " [OK] $Path" -ForegroundColor Green
    } else {
        Write-Host " [MISSING] $Path" -ForegroundColor Red
        return $false
    }
    return $true
}

$root = Resolve-Path "$PSScriptRoot/.."
Print-Header "Verifying Zenith DAW Structure at $root"

# 1. Check Core Source Directories
Print-Header "Checking Source Directories"
$requiredDirs = @(
    "apps/desktop/Source/engine",
    "apps/desktop/Source/ui",
    "apps/desktop/Source/tests",
    "apps/desktop/Resources",
    "docs",
    "planning",
    "scripts"
)

$allDirsExist = $true
foreach ($dir in $requiredDirs) {
    $fullPath = Join-Path $root $dir
    if (-not (Test-Directory $fullPath)) {
        $allDirsExist = $false
    }
}

# 2. Check for Misplaced C++ Files in Root
Print-Header "Checking for Misplaced Files"
$misplacedFiles = Get-ChildItem -Path $root -Filter "*.cpp" -Recurse -Depth 0
$misplacedHeaders = Get-ChildItem -Path $root -Filter "*.h" -Recurse -Depth 0

if ($misplacedFiles.Count -eq 0 -and $misplacedHeaders.Count -eq 0) {
    Write-Host " [OK] No source files in root" -ForegroundColor Green
} else {
    Write-Host " [WARNING] Found source files in root directory (should be in apps/desktop/Source):" -ForegroundColor Yellow
    foreach ($file in $misplacedFiles) { Write-Host "  - $($file.Name)" -ForegroundColor Yellow }
    foreach ($file in $misplacedHeaders) { Write-Host "  - $($file.Name)" -ForegroundColor Yellow }
}

# 3. Check Critical Config Files
Print-Header "Checking Configuration"
$configFiles = @(
    "CMakeLists.txt",
    "build.bat",
    "TODO.md"
)

foreach ($file in $configFiles) {
    $fullPath = Join-Path $root $file
    Test-Directory $fullPath | Out-Null
}

Print-Header "Verification Complete"
if ($allDirsExist) {
    Write-Host "Structure looks good!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "Critical directories are missing." -ForegroundColor Red
    exit 1
}
