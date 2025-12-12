# Zenith DAW - Build Standards

## 1. Build Directory Structure
- All build artifacts MUST be generated inside a single `build/` directory at the project root (`C:/zenith/daw/build/`).
- No other build-related directories (e.g., `build2/`, `build_test/`, `build_trash_*`, `out/`, `bin/`, `lib/`) should exist in the project root or be committed to version control.

## 2. Build Artifact Exclusion (.gitignore)
- The `.gitignore` file MUST comprehensively exclude all generated build artifacts, intermediate files, and IDE-specific clutter.
- Common exclusions include:
    - Build directories: `build/`, `cmake-build-*/`
    - Build logs: `*.log`, `build_log*.txt`, `build_output.txt`, `debug_log.txt`
    - CMake caches: `CMakeCache.txt`, `CMakeFiles/`
    - Visual Studio files: `*.sln`, `*.vcxproj`, `*.vcxproj.filters`, `x64/`, `*_artefacts/`, `*.dir/`
- Periodically verify that no build artifacts are accidentally being tracked by Git.

## 3. Recommended Build Process
- Use `build.bat` for standard builds.
    - `build.bat` (default to Release using Ninja)
    - `build.bat --debug` (Debug build using Visual Studio)
    - `build.bat --release` (Release build using Visual Studio)
    - `build.bat --preset <preset_name>` (use a specific CMake preset)
- Use `clean.bat` for cleaning the build environment.
    - `clean.bat` removes all generated build directories and logs.

## 4. Build Logging
- Build logs should ideally be redirected to a `logs/` subdirectory within the main `build/` directory, not pollute the project root.
- Automatic build log generation should be configured by the build system itself (e.g., CMake's CMAKE_EXPORT_COMPILE_COMMANDS, or specific generator features).

## 5. CMake Presets
- The project uses `CMakePresets.json` to define standard configure and build configurations.
- Developers should prefer using `cmake --preset <name>` commands or the provided `build.bat` script, which wraps these.

## 6. Cleanup Scripts
- One-off cleanup scripts (`cleanup_duplicates.py`, `cleanup_get.py`, `cleanup_headers.py`) are indicative of deeper code issues.
- These scripts should be reviewed. If their purpose is valid and regularly needed, the underlying issue should be fixed in the C++ code or build system. Otherwise, they should be removed.

## 7. Version Control Best Practices
- NEVER commit generated build artifacts, log files, or temporary files to Git.
- Ensure feature branches are merged cleanly into `master` after review.
- Delete merged branches locally and remotely.

## 8. Continuous Integration (CI)
- For robust build verification, a CI/CD system should be used to automatically build and test changes. This helps catch build issues early and ensures build reliability.
