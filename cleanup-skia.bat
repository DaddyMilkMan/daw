@echo off
echo ============================================
echo Cleaning up failed Skia download...
echo ============================================
echo.

cd /d C:\zenith\skia
if exist "skia.zip" (
    echo Removing bad skia.zip...
    del skia.zip
)

cd /d C:\zenith
if exist "skia" (
    echo Removing incomplete Skia directory...
    rmdir /s /q skia
)

echo.
echo Cleanup complete!
echo.
echo Now run: download-skia.bat
echo.
pause
