Write-Host "Building Pure Skia UI..." -ForegroundColor Cyan
cd C:\zenith\daw\build
& cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 && ninja ZenithDAW' 2>&1 | Select-Object -Last 15
if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful!" -ForegroundColor Green
    Write-Host "Launching..." -ForegroundColor Cyan
    Stop-Process -Name "Zenith DAW" -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
    Start-Process "C:\zenith\daw\build\zenith-core\ZenithDAW_artefacts\Debug\Zenith DAW.exe"
    Write-Host "PURE SKIA UI LAUNCHED!" -ForegroundColor Green
} else {
    Write-Host "Build failed!" -ForegroundColor Red
}
