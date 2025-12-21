# Objective: Enable Skia Integration and Build

The goal is to enable Skia rendering (`ZENITH_ENABLE_SKIA=ON`) for the Zenith DAW project and achieve a successful build.

## Current Status
1.  **Fallback Mode Works:** The application currently builds and runs successfully with `ZENITH_ENABLE_SKIA=OFF` (using JUCE graphics).
2.  **Code Fixes Applied:**
    *   Resolved `ZenithLookAndFeel` compilation and linker errors.
    *   Fixed `InstrumentBrowserPanel.cpp` corruption.
    *   Addressed `Spacing` vs `Metrics` type alias issues.
3.  **Skia Issue:**
    *   `SkiaManualIntegration.cmake` defaults to looking for Skia at `C:\zenith\skia`.
    *   The build fails with `ZENITH_ENABLE_SKIA=ON` because it cannot find the Skia library.
    *   **Crucial Context:** The user states that Skia is **already installed** on the system, but previous attempts to locate it in `c:\zenith\daw` or `c:\zenith\skia` (via standard checks) were unsuccessful or inconclusive due to tool limitations/cancellations.

## Next Steps for the New Session
1.  **Locate Skia:** Work with the user to identify the exact path of the existing Skia installation. It might be in a non-standard location or I missed it.
    *   Ask the user for the path or try broader search commands if permitted.
2.  **Configure CMake:** Once the path is found, run the CMake configuration command explicitly setting the `SKIA_DIR` variable:
    ```cmd
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DZENITH_ENABLE_SKIA=ON -DSKIA_DIR="C:/Path/To/Skia"
    ```
3.  **Build:** Run the build command:
    ```cmd
    cmake --build build --verbose --parallel 4
    ```
4.  **Verify:** Launch the executable and confirm it is using the Skia backend (not fallback).

## Relevant Files
*   `c:\zenith\daw\zenith-core\CMakeLists.txt`
*   `c:\zenith\daw\cmake\SkiaManualIntegration.cmake`
