#!/bin/bash

# =============================================================================
# Zenith DAW Prerequisites Checker
# =============================================================================
# This script checks if all prerequisites for building Zenith DAW are installed
# and provides helpful installation instructions for missing items.
#
# Usage:
#   ./check-prerequisites.sh [--install] [--skip-tests] [--verbose]
#
# Options:
#   --install      Attempt to install missing dependencies
#   --skip-tests   Skip version compatibility tests
#   --verbose      Show detailed version information
#
# Exit codes:
#   0 - All prerequisites satisfied
#   1 - Missing prerequisites found
#   2 - Error checking prerequisites
# =============================================================================

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../" && pwd)"

# Default options
INSTALL_MISSING="OFF"
SKIP_TESTS="OFF"
VERBOSE="OFF"

# Minimum required versions
MIN_CMAKE_VERSION="3.25"
MIN_NINJA_VERSION="1.10"
MIN_GCC_VERSION="12"
MIN_CLANG_VERSION="14"

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --install)
                INSTALL_MISSING="ON"
                shift
                ;;
            --skip-tests)
                SKIP_TESTS="ON"
                shift
                ;;
            --verbose)
                VERBOSE="ON"
                shift
                ;;
            *)
                echo -e "${RED}Error: Unknown option '$1'${NC}"
                show_help
                exit 2
                ;;
        esac
    done
}

# Show help message
show_help() {
    cat << EOF
Zenith DAW Prerequisites Checker

This script checks if all prerequisites for building Zenith DAW are installed
and provides helpful installation instructions for missing items.

Usage:
    $0 [options]

Options:
    --install      Attempt to install missing dependencies
    --skip-tests   Skip version compatibility tests
    --verbose      Show detailed version information

Exit codes:
    0 - All prerequisites satisfied
    1 - Missing prerequisites found
    2 - Error checking prerequisites

Examples:
    $0                      # Check prerequisites
    $0 --install            # Install missing dependencies
    $0 --verbose            # Show detailed version info

For more information, see BUILDING.md
EOF
}

# Print colored output
print_header() {
    echo -e "${PURPLE}$1${NC}"
}

print_section() {
    echo -e "${BLUE}$1${NC}"
}

print_ok() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}!${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_info() {
    echo -e "${CYAN}•${NC} $1"
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Get command version
get_version() {
    local cmd="$1"
    local version_cmd="$2"

    if command_exists "$cmd"; then
        if [ -n "$version_cmd" ]; then
            $version_cmd 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1
        else
            $cmd --version 2>/dev/null | head -n1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n1
        fi
    fi
}

# Check version compatibility
check_version() {
    local current="$1"
    local required="$2"
    local program="$3"

    # Convert version strings to comparable format
    local current_clean=$(echo "$current" | sed 's/[^0-9.]//g' | cut -d. -f1,2)
    local required_clean=$(echo "$required" | sed 's/[^0-9.]//g' | cut -d. -f1,2)

    # Compare versions
    if [ -z "$current_clean" ]; then
        return 1
    fi

    if [ "$(printf '%s\n' "$required_clean" "$current_clean" | sort -V | head -n1)" = "$required_clean" ]; then
        return 0
    else
        return 1
    fi
}

# Install dependency on Linux (Ubuntu/Debian)
install_linux_debian() {
    local package="$1"
    local description="$2"

    if ! dpkg -l "$package" >/dev/null 2>&1; then
        print_info "Installing $description..."

        # Update package list
        sudo apt update

        # Install package
        if sudo apt install -y "$package"; then
            print_ok "$package installed successfully"
            return 0
        else
            print_error "Failed to install $package"
            return 1
        fi
    else
        print_ok "$package already installed"
        return 0
    fi
}

# Install dependency on Linux (Fedora)
install_linux_fedora() {
    local package="$1"
    local description="$2"

    if ! rpm -q "$package" >/dev/null 2>&1; then
        print_info "Installing $description..."

        # Install package
        if sudo dnf install -y "$package"; then
            print_ok "$package installed successfully"
            return 0
        else
            print_error "Failed to install $package"
            return 1
        fi
    else
        print_ok "$package already installed"
        return 0
    fi
}

# Install dependency on macOS
install_macos() {
    local package="$1"
    local description="$2"

    if command_exists brew; then
        if ! brew list "$package" >/dev/null 2>&1; then
            print_info "Installing $description..."

            if brew install "$package"; then
                print_ok "$package installed successfully"
                return 0
            else
                print_error "Failed to install $package"
                return 1
            fi
        else
            print_ok "$package already installed"
            return 0
        fi
    else
        print_error "Homebrew not found. Cannot install $package"
        return 1
    fi
}

# Check C compiler
check_compiler() {
    print_section "Checking C Compiler..."

    # Determine compiler
    if [ -n "$CC" ]; then
        CC_COMPILER="$CC"
    else
        if command_exists gcc; then
            CC_COMPILER="gcc"
        elif command_exists clang; then
            CC_COMPILER="clang"
        else
            print_error "No C compiler found"
            return 1
        fi
    fi

    if command_exists "$CC_COMPILER"; then
        local version=$(get_version "$CC_COMPILER" "--version")
        print_ok "C compiler: $CC_COMPILER (version $version)"

        if [ "$SKIP_TESTS" = "OFF" ]; then
            if [ "$CC_COMPILER" = "gcc" ]; then
                if ! check_version "$version" "$MIN_GCC_VERSION" "GCC"; then
                    print_error "GCC version $version is too old. $MIN_GCC_VERSION+ required"
                    return 1
                fi
            elif [ "$CC_COMPILER" = "clang" ]; then
                if ! check_version "$version" "$MIN_CLANG_VERSION" "Clang"; then
                    print_error "Clang version $version is too old. $MIN_CLANG_VERSION+ required"
                    return 1
                fi
            fi
        fi
    else
        print_error "C compiler not found"
        return 1
    fi
}

# Check C++ compiler
check_cpp_compiler() {
    print_section "Checking C++ Compiler..."

    # Determine compiler
    if [ -n "$CXX" ]; then
        CXX_COMPILER="$CXX"
    else
        if command_exists g++; then
            CXX_COMPILER="g++"
        elif command_exists clang++; then
            CXX_COMPILER="clang++"
        else
            print_error "No C++ compiler found"
            return 1
        fi
    fi

    if command_exists "$CXX_COMPILER"; then
        local version=$(get_version "$CXX_COMPILER" "--version")
        print_ok "C++ compiler: $CXX_COMPILER (version $version)"

        if [ "$SKIP_TESTS" = "OFF" ]; then
            if [ "$CXX_COMPILER" = "g++" ]; then
                if ! check_version "$version" "$MIN_GCC_VERSION" "G++"; then
                    print_error "G++ version $version is too old. $MIN_GCC_VERSION+ required"
                    return 1
                fi
            elif [ "$CXX_COMPILER" = "clang++" ]; then
                if ! check_version "$version" "$MIN_CLANG_VERSION" "Clang++"; then
                    print_error "Clang++ version $version is too old. $MIN_CLANG_VERSION+ required"
                    return 1
                fi
            fi
        fi
    else
        print_error "C++ compiler not found"
        return 1
    fi
}

# Check CMake
check_cmake() {
    print_section "Checking CMake..."

    if command_exists cmake; then
        local version=$(get_version "cmake")
        print_ok "CMake found (version $version)"

        if [ "$SKIP_TESTS" = "OFF" ]; then
            if ! check_version "$version" "$MIN_CMAKE_VERSION" "CMake"; then
                print_error "CMake version $version is too old. $MIN_CMAKE_VERSION+ required"
                return 1
            fi
        fi

        if [ "$VERBOSE" = "ON" ]; then
            print_info "CMake detailed info:"
            cmake --version | head -3
        fi
    else
        print_error "CMake not found"

        if [ "$INSTALL_MISSING" = "ON" ]; then
            print_info "Installing CMake..."

            # Try different package managers
            if command_exists apt; then
                install_linux_debian "cmake" "CMake"
            elif command_exists dnf; then
                install_linux_fedora "cmake" "CMake"
            elif [ "$(uname)" = "Darwin" ] && command_exists brew; then
                install_macos "cmake" "CMake"
            else
                print_error "Cannot install CMake automatically. Please install manually:"
                print_error "  Download from https://cmake.org/download/"
                return 1
            fi
        else
            print_error "Please install CMake $MIN_CMAKE_VERSION+"
            print_info "Installation instructions:"
            print_info "  Ubuntu/Debian: sudo apt install cmake"
            print_info "  Fedora: sudo dnf install cmake"
            print_info "  macOS: brew install cmake"
            print_info "  Windows: Download from https://cmake.org/download/"
            return 1
        fi
    fi
}

# Check Ninja
check_ninja() {
    print_section "Checking Ninja..."

    if command_exists ninja; then
        local version=$(get_version "ninja")
        print_ok "Ninja found (version $version)"

        if [ "$SKIP_TESTS" = "OFF" ]; then
            if ! check_version "$version" "$MIN_NINJA_VERSION" "Ninja"; then
                print_error "Ninja version $version is too old. $MIN_NINJA_VERSION+ required"
                return 1
            fi
        fi

        if [ "$VERBOSE" = "ON" ]; then
            print_info "Ninja build system available"
        fi
    elif command_exists ninja-build; then
        print_ok "Ninja found as ninja-build"

        # Create symlink for convenience
        if [ ! -L /usr/local/bin/ninja ]; then
            print_info "Creating symlink from ninja-build to ninja..."
            sudo ln -sf $(which ninja-build) /usr/local/bin/ninja 2>/dev/null || true
        fi
    else
        print_error "Ninja not found"

        if [ "$INSTALL_MISSING" = "ON" ]; then
            print_info "Installing Ninja..."

            # Try different package managers
            if command_exists apt; then
                install_linux_debian "ninja-build" "Ninja build system"
            elif command_exists dnf; then
                install_linux_fedora "ninja-build" "Ninja build system"
            elif [ "$(uname)" = "Darwin" ] && command_exists brew; then
                install_macos "ninja" "Ninja build system"
            else
                print_error "Cannot install Ninja automatically. Please install manually:"
                print_error "  Download from https://github.com/ninja-build/ninja/releases"
                return 1
            fi
        else
            print_error "Please install Ninja $MIN_NINJA_VERSION+"
            print_info "Installation instructions:"
            print_info "  Ubuntu/Debian: sudo apt install ninja-build"
            print_info "  Fedora: sudo dnf install ninja-build"
            print_info "  macOS: brew install ninja"
            print_info "  Windows: Download from https://github.com/ninja-build/ninja/releases"
            return 1
        fi
    fi
}

# Check Git
check_git() {
    print_section "Checking Git..."

    if command_exists git; then
        local version=$(get_version "git" "git --version")
        print_ok "Git found (version $version)"

        if [ "$VERBOSE" = "ON" ]; then
            print_info "Git detailed info:"
            git --version
            git config --global user.name 2>/dev/null || print_warning "Git user.name not configured"
            git config --global user.email 2>/dev/null || print_warning "Git user.email not configured"
        fi
    else
        print_error "Git not found"

        if [ "$INSTALL_MISSING" = "ON" ]; then
            print_info "Installing Git..."

            # Try different package managers
            if command_exists apt; then
                install_linux_debian "git" "Git version control"
            elif command_exists dnf; then
                install_linux_fedora "git" "Git version control"
            elif [ "$(uname)" = "Darwin" ] && command_exists brew; then
                install_macos "git" "Git version control"
            else
                print_error "Cannot install Git automatically. Please install manually:"
                return 1
            fi
        else
            print_error "Please install Git"
            print_info "Installation instructions:"
            print_info "  Ubuntu/Debian: sudo apt install git"
            print_info "  Fedora: sudo dnf install git"
            print_info "  macOS: brew install git"
            print_info "  Windows: Download from https://git-scm.com/download/win"
            return 1
        fi
    fi
}

# Check Linux-specific dependencies
check_linux_dependencies() {
    print_section "Checking Linux Dependencies..."

    local missing_packages=()
    local install_scripts=()

    # Audio development libraries
    if ! dpkg -l libasound2-dev >/dev/null 2>&1; then
        missing_packages+=("libasound2-dev (ALSA audio development)")
        install_scripts+=("install_linux_debian 'libasound2-dev' 'ALSA audio development libraries'")
    fi

    if ! dpkg -l libjack-jackd2-dev >/dev/null 2>&1; then
        missing_packages+=("libjack-jackd2-dev (JACK audio development)")
        install_scripts+=("install_linux_debian 'libjack-jackd2-dev' 'JACK audio development libraries'")
    fi

    # Graphics and UI libraries
    if ! dpkg -l libx11-dev >/dev/null 2>&1; then
        missing_packages+=("libx11-dev (X11 development)")
        install_scripts+=("install_linux_debian 'libx11-dev' 'X11 development libraries'")
    fi

    if ! dpkg -l libxinerama-dev >/dev/null 2>&1; then
        missing_packages+=("libxinerama-dev (Xinerama development)")
        install_scripts+=("install_linux_debian 'libxinerama-dev' 'Xinerama development libraries'")
    fi

    if ! dpkg -l libxext-dev >/dev/null 2>&1; then
        missing_packages+=("libxext-dev (X11 extensions development)")
        install_scripts+=("install_linux_debian 'libxext-dev' 'X11 extensions development libraries'")
    fi

    if ! dpkg -l libxrandr-dev >/dev/null 2>&1; then
        missing_packages+=("libxrandr-dev (Xrandr development)")
        install_scripts+=("install_linux_debian 'libxrandr-dev' 'Xrandr development libraries'")
    fi

    if ! dpkg -l libxcursor-dev >/dev/null 2>&1; then
        missing_packages+=("libxcursor-dev (Xcursor development)")
        install_scripts+=("install_linux_debian 'libxcursor-dev' 'Xcursor development libraries'")
    fi

    if ! dpkg -l libwebkit2gtk-4.0-dev >/dev/null 2>&1; then
        missing_packages+=("libwebkit2gtk-4.0-dev (WebKitGTK development)")
        install_scripts+=("install_linux_debian 'libwebkit2gtk-4.0-dev' 'WebKitGTK development libraries'")
    fi

    # Graphics libraries
    if ! dpkg -l libglu1-mesa-dev >/dev/null 2>&1; then
        missing_packages+=("libglu1-mesa-dev (Mesa OpenGL utilities)")
        install_scripts+=("install_linux_debian 'libglu1-mesa-dev' 'Mesa OpenGL utilities development'")
    fi

    if ! dpkg -l mesa-common-dev >/dev/null 2>&1; then
        missing_packages+=("mesa-common-dev (Mesa common development)")
        install_scripts+=("install_linux_debian 'mesa-common-dev' 'Mesa common development files'")
    fi

    # Network and crypto
    if ! dpkg -l libcurl4-openssl-dev >/dev/null 2>&1; then
        missing_packages+=("libcurl4-openssl-dev (cURL development)")
        install_scripts+=("install_linux_debian 'libcurl4-openssl-dev' 'cURL development libraries'")
    fi

    if ! dpkg -l libssl-dev >/dev/null 2>&1; then
        missing_packages+=("libssl-dev (OpenSSL development)")
        install_scripts+=("install_linux_debian 'libssl-dev' 'OpenSSL development libraries'")
    fi

    if [ ${#missing_packages[@]} -gt 0 ]; then
        print_error "Missing Linux dependencies:"
        for package in "${missing_packages[@]}"; do
            print_error "  - $package"
        done

        if [ "$INSTALL_MISSING" = "ON" ]; then
            print_info "Installing missing Linux dependencies..."

            if command_exists apt; then
                # Install all packages at once
                local package_list=()
                for script in "${install_scripts[@]}"; do
                    package_list+=($(echo "$script" | awk -F"'" '{print $2}'))
                done

                if sudo apt update && sudo apt install -y "${package_list[@]}"; then
                    print_ok "All Linux dependencies installed successfully"
                else
                    print_error "Failed to install some dependencies"
                    return 1
                fi
            else
                print_error "Automatic installation only supports Ubuntu/Debian"
                print_error "Please install missing packages manually:"
                for package in "${missing_packages[@]}"; do
                    print_info "  sudo apt install $package"
                done
                return 1
            fi
        else
            print_info "To install missing dependencies:"
            print_info "  sudo apt install ${missing_packages[*]}"
            return 1
        fi
    else
        print_ok "All Linux dependencies found"
    fi
}

# Check macOS-specific dependencies
check_macos_dependencies() {
    print_section "Checking macOS Dependencies..."

    # Check for Xcode command line tools
    if ! xcode-select -p >/dev/null 2>&1; then
        print_error "Xcode command line tools not found"

        if [ "$INSTALL_MISSING" = "ON" ]; then
            print_info "Installing Xcode command line tools..."
            xcode-select --install
            print_info "Please complete the installation and re-run this script"
            return 1
        else
            print_error "Please install Xcode command line tools:"
            print_info "  xcode-select --install"
            return 1
        fi
    else
        print_ok "Xcode command line tools found"
    fi

    # Check for Homebrew
    if command_exists brew; then
        print_ok "Homebrew found"
    else
        print_warning "Homebrew not found (optional but recommended)"
        if [ "$VERBOSE" = "ON" ]; then
            print_info "Install Homebrew from https://brew.sh/"
        fi
    fi
}

# Check Windows-specific dependencies (in PowerShell context)
check_windows_dependencies() {
    print_section "Checking Windows Dependencies..."

    # This would be implemented in the PowerShell version
    # For now, just check basic Windows requirements
    print_info "Windows build requirements:"
    print_info "  - Visual Studio 2022 with C++ workload"
    print_info "  - Windows 10 or later"
    print_info "  - Administrator privileges for installation"
}

# Check build directory
check_build_directory() {
    print_section "Checking Build Directory..."

    if [ -d "$PROJECT_ROOT/build" ]; then
        print_ok "Build directory exists"

        if [ "$(ls -A "$PROJECT_ROOT/build" 2>/dev/null)" ]; then
            print_info "Build directory contains files"
        else
            print_info "Build directory is empty"
        fi
    else
        print_info "Build directory does not exist"
        print_info "This is normal for first-time builds"
    fi
}

# Check project files
check_project_files() {
    print_section "Checking Project Files..."

    local required_files=(
        "CMakeLists.txt"
        "cmake/Dependencies.cmake"
        "external/JUCE/CMakeLists.txt"
        "apps/desktop/Source/Main.cpp"
    )

    local missing_files=()

    for file in "${required_files[@]}"; do
        if [ -f "$PROJECT_ROOT/$file" ]; then
            print_ok "$file found"
        else
            missing_files+=("$file")
        fi
    done

    if [ ${#missing_files[@]} -gt 0 ]; then
        print_error "Missing project files:"
        for file in "${missing_files[@]}"; do
            print_error "  - $file"
        done
        return 1
    else
        print_ok "All required project files found"
    fi
}

# Check Git repository
check_git_repository() {
    print_section "Checking Git Repository..."

    if [ -d "$PROJECT_ROOT/.git" ]; then
        print_ok "Git repository found"

        if [ "$VERBOSE" = "ON" ]; then
            print_info "Repository status:"
            cd "$PROJECT_ROOT"
            git status --porcelain | head -10
            cd - >/dev/null
        fi
    else
        print_warning "Not a Git repository"
        print_info "This is normal for source distributions"
    fi
}

# Generate summary report
generate_summary() {
    print_section "Prerequisites Check Summary"

    if [ "$1" = "0" ]; then
        print_header "✅ All prerequisites satisfied!"
        echo
        print_info "You can now build Zenith DAW:"
        print_info "  ./scripts/build.sh"
        print_info "Or manually:"
        print_info "  mkdir build && cd build"
        print_info "  cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release"
        print_info "  cmake --build . -j$(nproc)"
        echo
        print_info "For more information, see BUILDING.md"
    else
        print_header "❌ Missing prerequisites found"
        echo
        print_error "Please install missing dependencies before building"
        print_info "Use --install to attempt automatic installation"
        echo
        print_info "For detailed instructions, see BUILDING.md"
    fi
}

# Main function
main() {
    # Parse arguments
    parse_args "$@"

    # Print header
    print_header "Zenith DAW Prerequisites Checker"
    echo

    # Change to project root
    cd "$PROJECT_ROOT" 2>/dev/null || cd "$(dirname "$SCRIPT_DIR")"

    # Initialize error counter
    local error_count=0

    # Check prerequisites
    {
        check_git || ((error_count++))
        check_cmake || ((error_count++))
        check_ninja || ((error_count++))
        check_compiler || ((error_count++))
        check_cpp_compiler || ((error_count++))

        # Platform-specific checks
        if [ "$(uname)" = "Linux" ]; then
            check_linux_dependencies || ((error_count++))
        elif [ "$(uname)" = "Darwin" ]; then
            check_macos_dependencies || ((error_count++))
        elif [ "$(uname)" = "Linux" ]; then
            check_windows_dependencies || ((error_count++))
        fi

        check_project_files || ((error_count++))
        check_git_repository
        check_build_directory
    } || true

    # Generate summary
    echo
    generate_summary $error_count

    # Exit with error code if prerequisites not satisfied
    if [ $error_count -gt 0 ]; then
        exit 1
    else
        exit 0
    fi
}

# Run main function with all arguments
main "$@"