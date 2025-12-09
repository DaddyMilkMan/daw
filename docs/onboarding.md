# Developer Onboarding Guide

Welcome to the Zenith DAW project! This guide will help you set up your development environment and build the project.

## How to Navigate Documentation

- **Step 1**: Read [Architecture](architecture.md) if you touch engine or threading.
- **Step 2**: Read [UI/UX Spec](ui-ux-spec.md) if you touch any Skia/visual code.
- **Step 3**: Read [Contributing](contributing.md) before opening your first PR.

## Prerequisites

### 1. Tools
Ensure you have the following installed:
- **Git**: For version control.
- **CMake** (3.20+): The build system.
- **C++ Compiler**: Must support **C++20**.
    - Windows: Visual Studio 2022 (MSVC).
    - macOS: Xcode 13+ (Clang).
    - Linux: GCC 11+ or Clang 14+.
- **Ninja** (Optional): Recommended for faster builds.

### 2. Dependencies
Zenith uses **JUCE** and **Skia**.
- **JUCE**: Managed via Git Submodules or CMake FetchContent (automatic).
- **Skia**: Pre-built binaries are downloaded automatically by the CMake script, or you can provide your own.

## Getting the Code

```bash
git clone --recursive https://github.com/zenith-daw/zenith.git
cd zenith
```

## Building Zenith

### Windows (PowerShell)

1.  **Configure**:
    ```powershell
    cmake -B build -G "Visual Studio 17 2022" -A x64
    ```
    *Optionally, to use Ninja:*
    ```powershell
    cmake -B build -G Ninja
    ```

2.  **Build**:
    ```powershell
    cmake --build build --config Release
    ```

### macOS / Linux

1.  **Configure**:
    ```bash
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    ```

2.  **Build**:
    ```bash
    cmake --build build
    ```

## Configuration Options

You can customize the build using CMake flags:

| Flag | Default | Description |
|------|---------|-------------|
| `ZENITH_USE_SKIA` | `ON` | Enable Skia-based UI rendering. |
| `ZENITH_BUILD_TESTS` | `ON` | Build the unit test suite. |
| `ZENITH_BUILD_EXAMPLES` | `OFF` | Build example plugins/apps. |

Example: Disabling Skia
```bash
cmake -B build -DZENITH_USE_SKIA=OFF
```

## Running Tests

After building, you can run the test suite using CTest:

```bash
cd build
ctest -C Release --output-on-failure
```

## Project Layout for New Developers

- **`zenith-core`**: The main library. Most of your work will be here.
- **`zenith-app`**: The standalone executable entry point.
- **`zenith-plugin`**: The VST3/AU plugin wrapper (if applicable).

## Adding a New Component

1.  **Create Files**: Add `.h` in `zenith-core/Source/ui` and `.cpp` in `zenith-core/src/ui`.
2.  **Register**: Add the new files to `zenith-core/CMakeLists.txt` (if not using globbing).
3.  **Implement**:
    - Inherit from `juce::Component` for standard widgets.
    - Inherit from `SkiaCanvasComponent` for high-performance custom drawing.

## Troubleshooting

- **"Skia not found"**: Ensure your internet connection is active during the first CMake configure, as it downloads binaries.
- **Linker Errors**: Check that you are not mixing Runtime Library settings (e.g., /MD vs /MT) on Windows.

## Next Steps

- Read the [Architecture Overview](architecture.md) to understand the system design.
- Check [Contributing Guidelines](contributing.md) before submitting a PR.
