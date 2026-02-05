#!/bin/bash

# =============================================================================
# Zenith DAW Dependency Update Script
# =============================================================================
# This script updates project dependencies to their latest compatible versions.
#
# Usage:
#   ./update-deps.sh [options]
#
# Options:
#   -h, --help          Show this help message
#   -a, --all           Update all dependencies
#   -j, --juce          Update JUCE only
#   -o, --onnx          Update ONNX Runtime only
#   -n, --network       Update network dependencies only
#   -v, --verbose       Verbose output
#   --test              Run tests after update
#   --force             Force update even if up to date
#   --check-only        Check for updates without installing
#
# Examples:
#   ./update-deps.sh                  # Update all dependencies
#   ./update-deps.sh --juce --test   # Update JUCE and run tests
#   ./update-deps.sh --check-only    # Check for updates
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
DEPENDENCY_MANIFEST="$PROJECT_ROOT/cmake/FetchContentVersions.cmake"

# Default options
UPDATE_ALL="OFF"
UPDATE_JUCE="OFF"
UPDATE_ONNX="OFF"
UPDATE_NETWORK="OFF"
VERBOSE="OFF"
RUN_TESTS="OFF"
FORCE_UPDATE="OFF"
CHECK_ONLY="OFF"

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -a|--all)
                UPDATE_ALL="ON"
                UPDATE_JUCE="ON"
                UPDATE_ONNX="ON"
                UPDATE_NETWORK="ON"
                shift
                ;;
            -j|--juce)
                UPDATE_JUCE="ON"
                shift
                ;;
            -o|--onnx)
                UPDATE_ONNX="ON"
                shift
                ;;
            -n|--network)
                UPDATE_NETWORK="ON"
                shift
                ;;
            -v|--verbose)
                VERBOSE="ON"
                shift
                ;;
            --test)
                RUN_TESTS="ON"
                shift
                ;;
            --force)
                FORCE_UPDATE="ON"
                shift
                ;;
            --check-only)
                CHECK_ONLY="ON"
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
Zenith DAW Dependency Update Script

This script updates project dependencies to their latest compatible versions.

Usage:
    $0 [options]

Options:
    -h, --help          Show this help message
    -a, --all           Update all dependencies
    -j, --juce          Update JUCE only
    -o, --onnx          Update ONNX Runtime only
    -n, --network       Update network dependencies only
    -v, --verbose       Verbose output
    --test              Run tests after update
    --force             Force update even if up to date
    --check-only        Check for updates without installing

Examples:
    $0                  # Update all dependencies
    $0 --juce --test   # Update JUCE and run tests
    $0 --check-only    # Check for updates

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

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Get current version from manifest
get_current_version() {
    local dep_name="$1"
    grep -A 2 "ZENITH_${dep_name}_VERSION" "$DEPENDENCY_MANIFEST" | grep set | sed 's/.*"\([^"]*\)".*/\1/'
}

# Get current git hash from manifest
get_current_hash() {
    local dep_name="$1"
    grep -A 2 "ZENITH_${dep_name}_GIT_HASH" "$DEPENDENCY_MANIFEST" | grep set | sed 's/.*"\([^"]*\)".*/\1/'
}

# Check for updates
check_for_updates() {
    local dep_name="$1"
    local check_func="$2"

    print_section "Checking for $dep_name updates"

    if command_exists "$check_func"; then
        local current_version=$(get_current_version "$dep_name" 2>/dev/null || echo "unknown")
        local latest_version=$($check_func)

        if [ -n "$latest_version" ]; then
            print_info "Current version: $current_version"
            print_info "Latest version: $latest_version"

            if [ "$latest_version" != "$current_version" ] || [ "$FORCE_UPDATE" = "ON" ]; then
                print_info "Update available: $current_version → $latest_version"
                return 0
            else
                print_info "Already up to date"
                return 1
            fi
        else
            print_warning "Could not check for updates"
            return 1
        fi
    else
        print_warning "$check_func not available"
        return 1
    fi
}

# Update JUCE
update_juce() {
    print_section "Updating JUCE"

    if [ "$CHECK_ONLY" = "ON" ]; then
        if check_for_updates "JUCE" "check_juce_version"; then
            return 0
        else
            return 1
        fi
    fi

    local juce_url="https://github.com/juce-framework/JUCE.git"
    local current_hash=$(get_current_hash "JUCE")

    if command_exists git; then
        print_status "Fetching latest JUCE commits..."
        local latest_hash=$(git ls-remote "$juce_url" refs/tags/8.0.12 | cut -f1)

        if [ -n "$latest_hash" ]; then
            if [ "$latest_hash" != "$current_hash" ] || [ "$FORCE_UPDATE" = "ON" ]; then
                print_status "Updating JUCE from $current_hash to $latest_hash"

                # Update the manifest
                sed -i "s/ZENITH_JUCE_GIT_HASH \"$current_hash\"/ZENITH_JUCE_GIT_HASH \"$latest_hash\"/" "$DEPENDENCY_MANIFEST"

                print_success "JUCE manifest updated to $latest_hash"
                return 0
            else
                print_info "JUCE is already up to date"
                return 1
            fi
        else
            print_error "Could not fetch latest JUCE hash"
            return 1
        fi
    else
        print_error "Git not available"
        return 1
    fi
}

# Check JUCE version (placeholder)
check_juce_version() {
    echo "8.0.12"  # This would normally fetch from GitHub
}

# Update ONNX Runtime
update_onnx() {
    print_section "Updating ONNX Runtime"

    if [ "$CHECK_ONLY" = "ON" ]; then
        if check_for_updates "ONNX" "check_onnx_version"; then
            return 0
        else
            return 1
        fi
    fi

    if command_exists curl; then
        local current_version=$(get_current_version "ONNX" 2>/dev/null || echo "unknown")
        local latest_version=$(curl -s https://api.github.com/repos/microsoft/onnxruntime/releases/latest | grep tag_name | cut -d'"' -f4 | sed 's/v//')

        if [ -n "$latest_version" ]; then
            if [ "$latest_version" != "$current_version" ] || [ "$FORCE_UPDATE" = "ON" ]; then
                print_status "Updating ONNX Runtime from $current_version to $latest_version"

                # Update the manifest
                sed -i "s/ZENITH_ONNX_VERSION \"$current_version\"/ZENITH_ONNX_VERSION \"$latest_version\"/" "$DEPENDENCY_MANIFEST"

                print_success "ONNX Runtime manifest updated to $latest_version"
                return 0
            else
                print_info "ONNX Runtime is already up to date"
                return 1
            fi
        else
            print_error "Could not fetch latest ONNX version"
            return 1
        fi
    else
        print_error "curl not available"
        return 1
    fi
}

# Check ONNX version (placeholder)
check_onnx_version() {
    curl -s https://api.github.com/repos/microsoft/onnxruntime/releases/latest | grep tag_name | cut -d'"' -f4 | sed 's/v//'
}

# Update network dependencies (OpenSSL, Opus)
update_network_deps() {
    print_section "Updating Network Dependencies"

    updated=false

    # Update OpenSSL
    if [ "$UPDATE_NETWORK" = "ON" ]; then
        if command_exists curl; then
            local current_openssl=$(get_current_version "OPENSSL" 2>/dev/null || echo "unknown")
            local latest_openssl=$(curl -s https://api.github.com/openssl/openssl/releases/latest | grep tag_name | cut -d'"' -f4 | sed 's/openssl-//')

            if [ -n "$latest_openssl" ] && [ "$latest_openssl" != "$current_openssl" ]; then
                print_status "Updating OpenSSL from $current_openssl to $latest_openssl"
                sed -i "s/ZENITH_OPENSSL_VERSION \"$current_openssl\"/ZENITH_OPENSSL_VERSION \"$latest_openssl\"/" "$DEPENDENCY_MANIFEST"
                updated=true
            else
                print_info "OpenSSL is already up to date"
            fi
        fi
    fi

    # Update Opus
    if [ "$UPDATE_NETWORK" = "ON" ]; then
        if command_exists curl; then
            local current_opus=$(grep "opus.*1\.3\.1" "$DEPENDENCY_MANIFEST" | cut -d'"' -f4)
            local latest_opus=$(curl -s https://api.github.com/xiph/opus/releases/latest | grep tag_name | cut -d'"' -f4)

            if [ -n "$latest_opus" ] && [ "$latest_opus" != "$current_opus" ]; then
                print_status "Updating Opus from $current_opus to $latest_opus"
                # Update Opus version in manifest
                sed -i "s/v1\.3\.1/$latest_opus/" "$DEPENDENCY_MANIFEST"
                updated=true
            else
                print_info "Opus is already up to date"
            fi
        fi
    fi

    if [ "$updated" = true ]; then
        print_success "Network dependencies updated"
        return 0
    else
        print_info "Network dependencies are up to date"
        return 1
    fi
}

# Clean and rebuild dependencies
rebuild_deps() {
    print_section "Rebuilding Dependencies"

    # Clean existing build
    if [ -d "$PROJECT_ROOT/build" ]; then
        print_status "Cleaning existing build..."
        rm -rf "$PROJECT_ROOT/build"
    fi

    # Configure CMake fresh
    print_status "Configuring CMake with fresh dependencies..."
    mkdir -p "$PROJECT_ROOT/build"
    cd "$PROJECT_ROOT/build"

    if cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DZENITH_FETCH_DEPS=ON; then
        print_success "CMake configuration successful"
    else
        print_error "CMake configuration failed"
        return 1
    fi

    # Build dependencies
    print_status "Building dependencies..."
    if cmake --build . --target deps; then
        print_success "Dependencies built successfully"
        cd - >/dev/null
        return 0
    else
        print_error "Failed to build dependencies"
        cd - >/dev/null
        return 1
    fi
}

# Run tests
run_tests() {
    print_section "Running Tests"

    if [ -d "$PROJECT_ROOT/build" ]; then
        cd "$PROJECT_ROOT/build"
        if cmake --build . --target test; then
            print_success "All tests passed"
            cd - >/dev/null
            return 0
        else
            print_error "Some tests failed"
            cd - >/dev/null
            return 1
        fi
    else
        print_warning "Build directory not found, skipping tests"
        return 0
    fi
}

# Show summary
show_summary() {
    print_section "Dependency Update Summary"
    echo

    echo "Updated dependencies:"
    if [ "$UPDATE_JUCE" = "ON" ]; then
        echo "  ✅ JUCE"
    fi
    if [ "$UPDATE_ONNX" = "ON" ]; then
        echo "  ✅ ONNX Runtime"
    fi
    if [ "$UPDATE_NETWORK" = "ON" ]; then
        echo "  ✅ Network dependencies"
    fi
    if [ "$UPDATE_ALL" = "ON" ]; then
        echo "  ✅ All dependencies"
    fi

    echo
    print_success "Dependency update completed!"
    echo
    print_info "Dependencies updated in: $DEPENDENCY_MANIFEST"
    echo
    print_info "Run './scripts/build.sh' to rebuild with new dependencies"
}

# Main function
main() {
    # Parse arguments
    parse_args "$@"

    # Print header
    print_header "Zenith DAW Dependency Update Script"
    echo

    # Check if at least one update option is specified
    if [ "$UPDATE_ALL" = "OFF" ] && [ "$UPDATE_JUCE" = "OFF" ] && [ "$UPDATE_ONNX" = "OFF" ] && [ "$UPDATE_NETWORK" = "OFF" ]; then
        print_warning "No update option specified"
        print_info "Use --help for usage information"
        echo
        print_info "Defaulting to all dependencies update"
        UPDATE_ALL="ON"
    fi

    # Update dependencies
    local updated=false

    if [ "$UPDATE_ALL" = "ON" ] || [ "$UPDATE_JUCE" = "ON" ]; then
        if update_juce; then
            updated=true
        fi
    fi

    if [ "$UPDATE_ALL" = "ON" ] || [ "$UPDATE_ONNX" = "ON" ]; then
        if update_onnx; then
            updated=true
        fi
    fi

    if [ "$UPDATE_ALL" = "ON" ] || [ "$UPDATE_NETWORK" = "ON" ]; then
        if update_network_deps; then
            updated=true
        fi
    fi

    # Check if any updates were made
    if [ "$updated" = true ] || [ "$FORCE_UPDATE" = "ON" ]; then
        print_info "Dependencies updated"

        # Rebuild dependencies if not in check-only mode
        if [ "$CHECK_ONLY" = "OFF" ]; then
            rebuild_deps
        fi

        # Run tests if requested
        if [ "$RUN_TESTS" = "ON" ]; then
            run_tests
        fi
    else
        print_info "No updates needed"
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