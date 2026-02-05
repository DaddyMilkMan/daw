#!/bin/bash

# =============================================================================
# Zenith DAW Clean Script
# =============================================================================
# This script helps clean build artifacts, dependencies, and temporary files.
#
# Usage:
#   ./clean.sh [options]
#
# Options:
#   -h, --help          Show this help message
#   -a, --all           Clean all artifacts including dependencies
#   -b, --build         Clean build directory only
#   -c, --cache         Clean build cache and temporary files
#   -d, --deps          Clean downloaded dependencies
#   -i, --install       Clean installation artifacts
#   -v, --verbose       Verbose output
#   --force             Force cleaning without confirmation
#
# Examples:
#   ./clean.sh                  # Clean build directory
#   ./clean.sh --all            # Clean everything
#   ./clean.sh --deps --cache   # Clean dependencies and cache
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
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Default options
CLEAN_BUILD="OFF"
CLEAN_CACHE="OFF"
CLEAN_DEPS="OFF"
CLEAN_INSTALL="OFF"
CLEAN_ALL="OFF"
VERBOSE="OFF"
FORCE="OFF"

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -a|--all)
                CLEAN_ALL="ON"
                CLEAN_BUILD="ON"
                CLEAN_CACHE="ON"
                CLEAN_DEPS="ON"
                CLEAN_INSTALL="ON"
                shift
                ;;
            -b|--build)
                CLEAN_BUILD="ON"
                shift
                ;;
            -c|--cache)
                CLEAN_CACHE="ON"
                shift
                ;;
            -d|--deps)
                CLEAN_DEPS="ON"
                shift
                ;;
            -i|--install)
                CLEAN_INSTALL="ON"
                shift
                ;;
            -v|--verbose)
                VERBOSE="ON"
                shift
                ;;
            --force)
                FORCE="ON"
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
Zenith DAW Clean Script

This script helps clean build artifacts, dependencies, and temporary files.

Usage:
    $0 [options]

Options:
    -h, --help          Show this help message
    -a, --all           Clean all artifacts including dependencies
    -b, --build         Clean build directory only
    -c, --cache         Clean build cache and temporary files
    -d, --deps          Clean downloaded dependencies
    -i, --install       Clean installation artifacts
    -v, --verbose       Verbose output
    --force             Force cleaning without confirmation

Examples:
    $0                  # Clean build directory
    $0 --all            # Clean everything
    $0 --deps --cache   # Clean dependencies and cache

EOF
}

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_section() {
    echo -e "${PURPLE}$1${NC}"
}

print_info() {
    echo -e "${CYAN}•${NC} $1"
}

# Clean build directory
clean_build() {
    print_section "Cleaning Build Directory"

    local build_dir="$PROJECT_ROOT/build"

    if [ -d "$build_dir" ]; then
        if [ "$VERBOSE" = "ON" ]; then
            print_info "Build directory contents:"
            ls -la "$build_dir" | head -10
        fi

        print_status "Removing build directory..."
        rm -rf "$build_dir"
        if [ $? -eq 0 ]; then
            print_success "Build directory cleaned"
        else
            print_error "Failed to clean build directory"
            exit 1
        fi
    else
        print_info "Build directory not found"
    fi
}

# Clean cache and temporary files
clean_cache() {
    print_section "Cleaning Cache and Temporary Files"

    local cache_dirs=(
        "$PROJECT_ROOT/.cache"
        "$PROJECT_ROOT/tmp"
        "$PROJECT_ROOT/temp"
        "$PROJECT_ROOT/cmake-build-*"
        "$PROJECT_ROOT/ninja-build-*"
    )

    for cache_dir in "${cache_dirs[@]}"; do
        if [ -d "$cache_dir" ]; then
            print_status "Removing cache directory: $cache_dir"
            rm -rf "$cache_dir"
            if [ $? -eq 0 ]; then
                print_success "Cache cleaned: $cache_dir"
            else
                print_error "Failed to clean cache: $cache_dir"
            fi
        fi
    done

    # Clean CMake cache files
    local cmake_cache_files=(
        "$PROJECT_ROOT/CMakeCache.txt"
        "$PROJECT_ROOT/CTestCache.txt"
        "$PROJECT_ROOT/CPackConfig.cmake"
        "$PROJECT_ROOT/*.cmake"
    )

    for cache_file in "${cmake_cache_files[@]}"; do
        if [ -f "$cache_file" ]; then
            print_status "Removing cache file: $cache_file"
            rm -f "$cache_file"
            if [ $? -eq 0 ]; then
                print_success "Cache file removed: $cache_file"
            else
                print_error "Failed to remove cache file: $cache_file"
            fi
        fi
    done

    # Clean dependency downloads
    local download_dirs=(
        "$PROJECT_ROOT/_deps"
        "$PROJECT_ROOT/downloads"
        "$PROJECT_ROOT/Downloads"
    )

    for download_dir in "${download_dirs[@]}"; do
        if [ -d "$download_dir" ]; then
            print_status "Removing dependency downloads: $download_dir"
            rm -rf "$download_dir"
            if [ $? -eq 0 ]; then
                print_success "Dependency downloads cleaned: $download_dir"
            else
                print_error "Failed to clean dependency downloads: $download_dir"
            fi
        fi
    done
}

# Clean downloaded dependencies
clean_deps() {
    print_section "Cleaning Downloaded Dependencies"

    local deps=(
        "$PROJECT_ROOT/external/JUCE"
        "$PROJECT_ROOT/external/vcpkg"
        "$PROJECT_ROOT/external/onnxruntime"
        "$PROJECT_ROOT/external/openssl"
        "$PROJECT_ROOT/external/opus"
        "$PROJECT_ROOT/external/lua"
    )

    for dep in "${deps[@]}"; do
        if [ -d "$dep" ]; then
            if [ "$VERBOSE" = "ON" ]; then
                print_info "Dependency found: $dep"
                du -sh "$dep" | cut -f1
            fi

            print_status "Removing dependency: $dep"
            rm -rf "$dep"
            if [ $? -eq 0 ]; then
                print_success "Dependency cleaned: $dep"
            else
                print_error "Failed to clean dependency: $dep"
            fi
        fi
    done

    # Clean git submodules
    if [ -f "$PROJECT_ROOT/.gitmodules" ]; then
        print_info "Resetting git submodules..."
        cd "$PROJECT_ROOT"
        git submodule foreach --recursive 'git clean -xdf'
        git submodule foreach --recursive 'git reset --hard'
        cd - >/dev/null
        print_success "Git submodules cleaned"
    fi
}

# Clean installation artifacts
clean_install() {
    print_section "Cleaning Installation Artifacts"

    local install_dirs=(
        "/usr/local/bin/zenith-daw"
        "/usr/local/bin/ZenithDAW"
        "/usr/local/lib/libzenith*"
        "/usr/local/include/zenith*"
        "/usr/local/share/zenith*"
        "/usr/local/share/doc/zenith*"
    )

    for install_dir in "${install_dirs[@]}"; do
        if [ -e "$install_dir" ]; then
            print_status "Removing installation: $install_dir"
            sudo rm -rf "$install_dir"
            if [ $? -eq 0 ]; then
                print_success "Installation removed: $install_dir"
            else
                print_error "Failed to remove installation: $install_dir"
            fi
        fi
    done

    # Clean package files
    local package_files=(
        "$PROJECT_ROOT/*.deb"
        "$PROJECT_ROOT/*.rpm"
        "$PROJECT_ROOT/*.dmg"
        "$PROJECT_ROOT/*.zip"
        "$PROJECT_ROOT/*.tar.gz"
        "$PROJECT_ROOT/*.pkg"
    )

    for package_file in "${package_files[@]}"; do
        if [ -f "$package_file" ]; then
            print_status "Removing package file: $package_file"
            rm -f "$package_file"
            if [ $? -eq 0 ]; then
                print_success "Package file removed: $package_file"
            else
                print_error "Failed to remove package file: $package_file"
            fi
        fi
    done
}

# Clean other temporary files
clean_temp() {
    print_section "Cleaning Temporary Files"

    # Clean temporary files in project root
    local temp_files=(
        "$PROJECT_ROOT/*.tmp"
        "$PROJECT_ROOT/*.temp"
        "$PROJECT_ROOT/*.log"
        "$PROJECT_ROOT/*.log.*"
        "$PROJECT_ROOT/core"
        "$PROJECT_ROOT/*.core"
        "$PROJECT_ROOT/*.so"
        "$PROJECT_ROOT/*.so.*"
        "$PROJECT_ROOT/*.dylib"
        "$PROJECT_ROOT/*.dylib.*"
        "$PROJECT_ROOT/*.dll"
        "$PROJECT_ROOT/*.exe"
    )

    for temp_file in "${temp_files[@]}"; do
        if [ -f "$temp_file" ]; then
            print_status "Removing temporary file: $temp_file"
            rm -f "$temp_file"
        fi
    done

    # Clean editor files
    local editor_files=(
        "$PROJECT_ROOT/.vscode/"
        "$PROJECT_ROOT/.idea/"
        "$PROJECT_ROOT/*.swp"
        "$PROJECT_ROOT/*.swo"
        "$PROJECT_ROOT/*~"
        "$PROJECT_ROOT/Thumbs.db"
        "$PROJECT_ROOT/.DS_Store"
    )

    for editor_file in "${editor_files[@]}"; do
        if [ -e "$editor_file" ]; then
            print_status "Removing editor files: $editor_file"
            rm -rf "$editor_file"
        fi
    done
}

# Show summary
show_summary() {
    print_section "Clean Summary"
    echo

    echo "Cleaned items:"
    if [ "$CLEAN_BUILD" = "ON" ]; then
        echo "  ✅ Build directory"
    fi
    if [ "$CLEAN_CACHE" = "ON" ]; then
        echo "  ✅ Cache and temporary files"
    fi
    if [ "$CLEAN_DEPS" = "ON" ]; then
        echo "  ✅ Downloaded dependencies"
    fi
    if [ "$CLEAN_INSTALL" = "ON" ]; then
        echo "  ✅ Installation artifacts"
    fi
    if [ "$CLEAN_ALL" = "ON" ]; then
        echo "  ✅ All files"
    fi

    echo
    print_success "Cleaning completed!"
    echo
    print_info "Ready for a fresh build"
}

# Main function
main() {
    # Parse arguments
    parse_args "$@"

    # Print header
    print_header "Zenith DAW Clean Script"
    echo

    # Check if at least one clean option is specified
    if [ "$CLEAN_ALL" = "OFF" ] && [ "$CLEAN_BUILD" = "OFF" ] && [ "$CLEAN_CACHE" = "OFF" ] && [ "$CLEAN_DEPS" = "OFF" ] && [ "$CLEAN_INSTALL" = "OFF" ]; then
        print_warning "No clean option specified"
        print_info "Use --help for usage information"
        echo
        print_info "Defaulting to build directory clean"
        CLEAN_BUILD="ON"
    fi

    # Ask for confirmation unless forced
    if [ "$FORCE" = "OFF" ]; then
        echo -n "Are you sure you want to clean these artifacts? (y/N): "
        read -r response
        if [[ ! "$response" =~ ^[Yy]$ ]]; then
            print_info "Cleaning cancelled"
            exit 0
        fi
    fi

    # Perform cleaning
    if [ "$CLEAN_ALL" = "ON" ] || [ "$CLEAN_BUILD" = "ON" ]; then
        clean_build
    fi

    if [ "$CLEAN_ALL" = "ON" ] || [ "$CLEAN_CACHE" = "ON" ]; then
        clean_cache
    fi

    if [ "$CLEAN_ALL" = "ON" ] || [ "$CLEAN_DEPS" = "ON" ]; then
        clean_deps
    fi

    if [ "$CLEAN_ALL" = "ON" ] || [ "$CLEAN_INSTALL" = "ON" ]; then
        clean_install
    fi

    if [ "$CLEAN_ALL" = "ON" ]; then
        clean_temp
    fi

    # Show summary
    show_summary
}

# Print header
print_header() {
    echo -e "${PURPLE}$1${NC}"
}

# Run main function with all arguments
main "$@"