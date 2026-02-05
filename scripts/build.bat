@echo off
REM =============================================================================
REM Zenith DAW Build Script for Windows
REM =============================================================================
REM This script automates the build process for Zenith DAW with error checking
REM and progress reporting.
REM
REM Usage:
REM   build.bat [options]
REM
REM Options:
REM   -h, --help          Show this help message
REM   -c, --clean         Clean build directory before building
REM   -r, --release       Build release version (default)
REM   -d, --debug         Build debug version
REM   -t, --tests         Build with tests
REM   -j, --jobs N        Use N parallel jobs (default: all cores)
REM   -v, --verbose       Verbose build output
REM   --skip-prerequisites Skip prerequisites check
REM   --skip-deps         Skip dependency fetching
REM   --install-deps      Install missing dependencies (requires vcpkg)
REM   --docker            Build in Docker container
REM   --configure-only    Only run CMake configuration
REM   --build-only        Only run build (skip configure)
REM
REM Examples:
REM   build.bat                    # Release build
REM   build.bat --clean --debug    # Clean debug build
REM   build.bat --tests --jobs 8   # Build with tests, 8 jobs
REM =============================================================================

setlocal enabledelayedexpansion

REM Script configuration
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "BUILD_DIR=%PROJECT_ROOT%\build"

REM Default build options
set "BUILD_TYPE=Release"
set "BUILD_TESTS=OFF"
set "JOBS=%NUMBER_OF_PROCESSORS%"
set "CLEAN_BUILD=OFF"
set "VERBOSE=OFF"
set "SKIP_PREREQS=OFF"
set "SKIP_DEPS=OFF"
set "INSTALL_DEPS=OFF"
set "DOCKER_BUILD=OFF"
set "CONFIGURE_ONLY=OFF"
set "BUILD_ONLY=OFF"

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :end_parse_args
if "%~1"=="-h" goto :show_help
if "%~1"=="--help" goto :show_help
if "%~1"=="-c" set "CLEAN_BUILD=ON" & shift & goto :parse_args
if "%~1"=="--clean" set "CLEAN_BUILD=ON" & shift & goto :parse_args
if "%~1"=="-r" set "BUILD_TYPE=Release" & shift & goto :parse_args
if "%~1"=="--release" set "BUILD_TYPE=Release" & shift & goto :parse_args
if "%~1"=="-d" set "BUILD_TYPE=Debug" & shift & goto :parse_args
if "%~1"=="--debug" set "BUILD_TYPE=Debug" & shift & goto :parse_args
if "%~1"=="-t" set "BUILD_TESTS=ON" & shift & goto :parse_args
if "%~1"=="--tests" set "BUILD_TESTS=ON" & shift & goto :parse_args
if "%~1"=="-j" set "JOBS=%~2" & shift & shift & goto :parse_args
if "%~1"=="--jobs" set "JOBS=%~2" & shift & shift & goto :parse_args
if "%~1"=="-v" set "VERBOSE=ON" & shift & goto :parse_args
if "%~1"=="--verbose" set "VERBOSE=ON" & shift & goto :parse_args
if "%~1"=="--skip-prerequisites" set "SKIP_PREREQS=ON" & shift & goto :parse_args
if "%~1"=="--skip-deps" set "SKIP_DEPS=ON" & shift & goto :parse_args
if "%~1"=="--install-deps" set "INSTALL_DEPS=ON" & shift & goto :parse_args
if "%~1"=="--docker" set "DOCKER_BUILD=ON" & shift & goto :parse_args
if "%~1"=="--configure-only" set "CONFIGURE_ONLY=ON" & shift & goto :parse_args
if "%~1"=="--build-only" set "BUILD_ONLY=ON" & shift & goto :parse_args

echo ERROR: Unknown option '%~1'
goto :show_help

:end_parse_args

REM Colors for output (Windows 10+ with ANSI support)
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "BLUE=[94m"
set "NC=[0m"

REM Check for Windows 10+ for color support
ver | findstr /i "10\.0" >nul
if %errorlevel% equ 0 (
    set "ENABLE_COLORS=1"
)

REM Helper functions
:print_status
if "%ENABLE_COLORS%"=="1" echo %BLUE%[INFO]%NC% %~1
if "%ENABLE_COLORS%"=="0" echo [INFO] %~1
goto :eof

:print_success
if "%ENABLE_COLORS%"=="1" echo %GREEN%[SUCCESS]%NC% %~1
if "%ENABLE_COLORS%"=="0" echo [SUCCESS] %~1
goto :eof

:print_warning
if "%ENABLE_COLORS%"=="1" echo %YELLOW%[WARNING]%NC% %~1
if "%ENABLE_COLORS%"=="0" echo [WARNING] %~1
goto :eof

:print_error
if "%ENABLE_COLORS%"=="1" echo %RED%[ERROR]%NC% %~1
if "%ENABLE_COLORS%"=="0" echo [ERROR] %~1
goto :eof

REM Show help message
:show_help
echo Zenith DAW Build Script for Windows
echo.
echo This script automates the build process for Zenith DAW with error checking
echo and progress reporting.
echo.
echo Usage:
echo   build.bat [options]
echo.
echo Options:
echo   -h, --help              Show this help message
echo   -c, --clean             Clean build directory before building
echo   -r, --release           Build release version (default)
echo   -d, --debug             Build debug version
echo   -t, --tests             Build with tests
echo   -j, --jobs N            Use N parallel jobs (default: all cores)
echo   -v, --verbose           Verbose build output
echo   --skip-prerequisites    Skip prerequisites check
echo   --skip-deps            Skip dependency fetching
echo   --install-deps         Install missing dependencies (requires vcpkg)
echo   --docker               Build in Docker container
echo   --configure-only       Only run CMake configuration
echo   --build-only           Only run build (skip configure)
echo.
echo Examples:
echo   build.bat                      # Release build
echo   build.bat --clean --debug      # Clean debug build
echo   build.bat --tests --jobs 8    # Build with tests, 8 jobs
echo.
echo Environment Variables:
echo   CC                      C compiler to use
echo   CXX                     C++ compiler to use
echo   CMAKE_PREFIX_PATH       Additional CMake prefix paths
echo   VCPKG_ROOT              Path to vcpkg installation
echo   NINJA_PATH              Path to ninja executable (if not in PATH)
echo.
echo Prerequisites:
echo   - Visual Studio 2022 with "Desktop development with C++" workload
echo   - CMake 3.25+
echo   - Ninja build system
echo   - vcpkg (optional, for dependency management)
echo   - Windows 10 or later
echo.
pause
exit /b 0

REM Check if command exists
:command_exists
where %1 >nul 2>&1
exit /b %errorlevel%

REM Check version (simplified for Windows)
:check_version
where %1 >nul 2>&1
if %errorlevel% neq 0 exit /b 1
exit /b 0

REM Check prerequisites
:check_prerequisites
if "%SKIP_PREREQS%"=="ON" (
    call :print_status "Skipping prerequisites check"
    exit /b 0
)

call :print_status "Checking prerequisites..."

REM Check CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "CMake not found. Please install CMake 3.25+"
    if "%INSTALL_DEPS%"=="ON" (
        goto :install_dependencies
    ) else (
        exit /b 1
    )
) else (
    for /f "tokens=3" %%i in ('cmake --version ^| findstr /i cmake') do set "cmake_version=%%i"
    call :print_success "CMake !cmake_version! found"
)

REM Check Ninja
where ninja >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "Ninja not found. Please install Ninja"
    if "%INSTALL_DEPS%"=="ON" (
        goto :install_dependencies
    ) else (
        exit /b 1
    )
) else (
    call :print_success "Ninja found"
)

REM Check Visual Studio
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "Visual Studio compiler not found. Please install Visual Studio 2022"
    call :print_error "Make sure to run this script from 'x64 Native Tools Command Prompt'"
    exit /b 1
) else (
    call :print_success "Visual Studio compiler found"
)

REM Check vcpkg if installing dependencies
if "%INSTALL_DEPS%"=="ON" (
    if defined VCPKG_ROOT (
        if exist "%VCPKG_ROOT%\vcpkg.exe" (
            call :print_success "vcpkg found at !VCPKG_ROOT!"
        ) else (
            call :print_error "vcpkg.exe not found in VCPKG_ROOT: !VCPKG_ROOT!"
            exit /b 1
        )
    ) else (
        call :print_error "VCPKG_ROOT not set. Please set it to your vcpkg installation directory"
        exit /b 1
    )
)

exit /b 0

REM Install dependencies
:install_dependencies
call :print_status "Installing dependencies..."

REM Check if vcpkg is available
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\vcpkg.exe" (
        call :print_status "Using vcpkg for dependency installation"
        echo This would install dependencies using vcpkg:
        echo   "%VCPKG_ROOT%\vcpkg install cmake ninja"
        echo.
        set /p install_choice="Proceed with dependency installation? (y/N): "
        if /i "!install_choice!"=="y" (
            "%VCPKG_ROOT%\vcpkg" install cmake ninja
            if !errorlevel! neq 0 (
                call :print_error "vcpkg installation failed"
                exit /b 1
            )
            call :print_success "Dependencies installed"
        ) else (
            call :print_warning "Dependency installation skipped"
        )
    )
)

exit /b 0

REM Clean build directory
:clean_build
if "%CLEAN_BUILD%"=="ON" if exist "%BUILD_DIR%" (
    call :print_status "Cleaning build directory..."
    rmdir /s /q "%BUILD_DIR%"
    if !errorlevel! neq 0 (
        call :print_error "Failed to clean build directory"
        exit /b 1
    )
    call :print_success "Build directory cleaned"
)

exit /b 0

REM Configure CMake
:configure_cmake
if "%CONFIGURE_ONLY%"=="ON" (
    call :print_status "Skipping CMake configuration (--configure-only)"
    exit /b 0
)

call :print_status "Configuring CMake for %BUILD_TYPE% build..."

REM Build CMake arguments
set "cmake_args=-S "%PROJECT_ROOT%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE="%BUILD_TYPE%" -DBUILD_TESTS="%BUILD_TESTS%""

if defined CMAKE_PREFIX_PATH (
    set "cmake_args=!cmake_args! -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%""
)

if "%VERBOSE%"=="ON" (
    set "cmake_args=!cmake_args! -DCMAKE_VERBOSE_MAKEFILE=ON"
)

call :print_status "Running: cmake !cmake_args!"

REM Run CMake
!cmake_args!
if %errorlevel% neq 0 (
    call :print_error "CMake configuration failed"
    exit /b 1
)

call :print_success "CMake configuration successful"

exit /b 0

REM Build project
:build_project
if "%BUILD_ONLY%"=="ON" (
    call :print_status "Skipping build (--build-only)"
    exit /b 0
)

call :print_status "Building project with %JOBS% jobs..."

REM Build arguments
set "build_args=--build "%BUILD_DIR%" --config "%BUILD_TYPE%" -- /m:%JOBS%"

if "%VERBOSE%"=="ON" (
    set "build_args=!build_args! --verbose"
)

call :print_status "Running: cmake !build_args!"

REM Run build
cmake !build_args!
if %errorlevel% neq 0 (
    call :print_error "Build failed"
    exit /b 1
)

call :print_success "Build completed successfully"

exit /b 0

REM Run tests if enabled
:run_tests
if "%BUILD_TESTS%"=="OFF" (
    exit /b 0
)

call :print_status "Running tests..."

if exist "%BUILD_DIR%\ZenithDAWTests.exe" (
    cd /d "%BUILD_DIR%"
    if "%BUILD_TYPE%"=="Debug" (
        if exist "%BUILD_DIR%\Debug\ZenithDAWTests.exe" (
            "%BUILD_DIR%\Debug\ZenithDAWTests.exe"
            if %errorlevel% neq 0 (
                call :print_error "Some tests failed"
                exit /b 1
            )
        )
    ) else (
        "%BUILD_DIR%\ZenithDAWTests.exe"
        if %errorlevel% neq 0 (
            call :print_error "Some tests failed"
            exit /b 1
        )
    )
) else if exist "%BUILD_DIR%\Release\ZenithDAWTests.exe" (
    cd /d "%BUILD_DIR%"
    "%BUILD_DIR%\Release\ZenithDAWTests.exe"
    if %errorlevel% neq 0 (
        call :print_error "Some tests failed"
        exit /b 1
    )
) else if exist "%BUILD_DIR%\x64\%BUILD_TYPE%\ZenithDAWTests.exe" (
    cd /d "%BUILD_DIR%"
    "%BUILD_DIR%\x64\%BUILD_TYPE%\ZenithDAWTests.exe"
    if %errorlevel% neq 0 (
        call :print_error "Some tests failed"
        exit /b 1
    )
) else (
    call :print_warning "Test executable not found"
)

call :print_success "All tests passed"

exit /b 0

REM Build in Docker
:build_in_docker
if "%DOCKER_BUILD%"=="ON" (
    call :print_status "Building in Docker container..."

    REM Check if Docker is installed
    where docker >nul 2>&1
    if %errorlevel% neq 0 (
        call :print_error "Docker not found. Please install Docker Desktop"
        exit /b 1
    )

    REM Build Docker image
    call :print_status "Building Docker image..."
    docker build -t zenith-daw-builder -f "%PROJECT_ROOT%\Dockerfile" "%PROJECT_ROOT%"
    if %errorlevel% neq 0 (
        call :print_error "Docker image build failed"
        exit /b 1
    )

    REM Run build in container
    call :print_status "Running build in container..."
    docker run --rm -v "%PROJECT_ROOT%:/workspace" -w /workspace zenith-daw-builder
    if %errorlevel% neq 0 (
        call :print_error "Docker build failed"
        exit /b 1
    )

    call :print_success "Docker build completed"
)

exit /b 0

REM Show build summary
:show_summary
call :print_status "Build Summary:"
echo   Build Type: %BUILD_TYPE%
echo   Tests: %BUILD_TESTS%
echo   Jobs: %JOBS%
echo   Clean Build: %CLEAN_BUILD%
echo   Build Directory: %BUILD_DIR%

REM Find and show executable
if exist "%BUILD_DIR%\ZenithDAW.exe" (
    for %%A in ("%BUILD_DIR%\ZenithDAW.exe") do set "size=%%~zA"
    set /a "size_mb=!size!/1024/1024"
    echo   Executable: %BUILD_DIR%\ZenithDAW.exe (!size_mb! MB)
    echo.
    call :print_success "You can run the application with:"
    echo   "%BUILD_DIR%\ZenithDAW.exe"
) else if exist "%BUILD_DIR%\x64\%BUILD_TYPE%\Zenith\ DAW.exe" (
    for %%A in ("%BUILD_DIR%\x64\%BUILD_TYPE%\Zenith\ DAW.exe") do set "size=%%~zA"
    set /a "size_mb=!size!/1024/1024"
    echo   Executable: %BUILD_DIR%\x64\%BUILD_TYPE%\Zenith\ DAW.exe (!size_mb! MB)
    echo.
    call :print_success "You can run the application with:"
    echo   "%BUILD_DIR%\x64\%BUILD_TYPE%\Zenith\ DAW.exe"
) else if exist "%BUILD_DIR%\Release\Zenith DAW.exe" (
    for %%A in ("%BUILD_DIR%\Release\Zenith DAW.exe") do set "size=%%~zA"
    set /a "size_mb=!size!/1024/1024"
    echo   Executable: %BUILD_DIR%\Release\Zenith DAW.exe (!size_mb! MB)
    echo.
    call :print_success "You can run the application with:"
    echo   "%BUILD_DIR%\Release\Zenith DAW.exe"
) else (
    call :print_warning "Build completed but executable not found"
)

exit /b 0

REM Main function
:main
REM Change to project root
cd /d "%PROJECT_ROOT%"

REM Check prerequisites
call :check_prerequisites

REM Build in Docker if requested
if "%DOCKER_BUILD%"=="ON" (
    call :build_in_docker
    call :show_summary
    exit /b 0
)

REM Clean build directory if requested
call :clean_build

REM Fetch dependencies if not skipped
if "%SKIP_DEPS%"=="OFF" (
    call :print_status "Fetching dependencies..."
    if exist "%BUILD_DIR%\CMakeCache.txt" (
        cmake --build "%BUILD_DIR%" --target deps 2>nul
        if %errorlevel% neq 0 (
            call :print_warning "Dependency fetching failed, continuing with build..."
        )
    )
)

REM Configure CMake
call :configure_cmake

REM Build project
call :build_project

REM Run tests if enabled
call :run_tests

REM Show summary
call :show_summary

exit /b 0

REM Main entry point
call :main %*

REM Exit with error code if any
if %errorlevel% neq 0 exit /b %errorlevel%