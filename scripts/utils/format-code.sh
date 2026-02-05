#!/bin/bash

# =============================================================================
# Zenith DAW Code Formatter Script
# =============================================================================
# This script formats code according to project standards.
#
# Usage:
#   ./format-code.sh [options]
#
# Options:
#   -h, --help          Show this help message
   -c, --check         Check formatting without changing files
   -f, --fix           Fix formatting issues
   -v, --verbose       Verbose output
   --all               Format all files in the project
   --cpp               Format C++ files only
   --header            Format header files only
   --cmake             Format CMake files only
   --python            Format Python files only
   --files PATTERN     Format files matching pattern
   --exclude PATTERN   Exclude files matching pattern
   --clang-format      Use clang-format
   --uncrustify       Use uncrustify
   --autopep8         Use autopep8 for Python
   --black            Use Black for Python
   --dry-run          Show what would be formatted without doing it
   --diff             Show diff of formatting changes
   --stats            Show formatting statistics
#
# Examples:
#   ./format-code.sh --check          # Check formatting
#   ./format-code.sh --fix --verbose  # Fix formatting with verbose output
#   ./format-code.sh --all --clang-format # Format all files with clang-format
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
CHECK_ONLY="OFF"
FIX_FORMATTING="OFF"
VERBOSE="OFF"
FORMAT_ALL="OFF"
FORMAT_CPP="OFF"
FORMAT_HEADER="OFF"
FORMAT_CMAKE="OFF"
FORMAT_PYTHON="OFF"
FILE_PATTERN=""
EXCLUDE_PATTERN=""
USE_CLANG_FORMAT="OFF"
USE_UNCRUSTIFY="OFF"
USE_AUTOPEP8="OFF"
USE_BLACK="OFF"
DRY_RUN="OFF"
SHOW_DIFF="OFF"
SHOW_STATS="OFF"

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--check)
                CHECK_ONLY="ON"
                FIX_FORMATTING="OFF"
                shift
                ;;
            -f|--fix)
                FIX_FORMATTING="ON"
                CHECK_ONLY="OFF"
                shift
                ;;
            -v|--verbose)
                VERBOSE="ON"
                shift
                ;;
            --all)
                FORMAT_ALL="ON"
                shift
                ;;
            --cpp)
                FORMAT_CPP="ON"
                shift
                ;;
            --header)
                FORMAT_HEADER="ON"
                shift
                ;;
            --cmake)
                FORMAT_CMAKE="ON"
                shift
                ;;
            --python)
                FORMAT_PYTHON="ON"
                shift
                ;;
            --files)
                FILE_PATTERN="$2"
                shift 2
                ;;
            --exclude)
                EXCLUDE_PATTERN="$2"
                shift 2
                ;;
            --clang-format)
                USE_CLANG_FORMAT="ON"
                shift
                ;;
            --uncrustify)
                USE_UNCRUSTIFY="ON"
                shift
                ;;
            --autopep8)
                USE_AUTOPEP8="ON"
                shift
                ;;
            --black)
                USE_BLACK="ON"
                shift
                ;;
            --dry-run)
                DRY_RUN="ON"
                shift
                ;;
            --diff)
                SHOW_DIFF="ON"
                shift
                ;;
            --stats)
                SHOW_STATS="ON"
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
Zenith DAW Code Formatter Script

This script formats code according to project standards.

Usage:
    $0 [options]

Options:
    -h, --help          Show this help message
    -c, --check         Check formatting without changing files
    -f, --fix           Fix formatting issues
    -v, --verbose       Verbose output
    --all               Format all files in the project
    --cpp               Format C++ files only
    --header            Format header files only
    --cmake             Format CMake files only
    --python            Format Python files only
    --files PATTERN     Format files matching pattern
    --exclude PATTERN   Exclude files matching pattern
    --clang-format      Use clang-format
    --uncrustify       Use uncrustify
    --autopep8         Use autopep8 for Python
    --black            Use Black for Python
    --dry-run          Show what would be formatted without doing it
    --diff             Show diff of formatting changes
    --stats            Show formatting statistics

Examples:
    $0 --check          # Check formatting
    $0 --fix --verbose  # Fix formatting with verbose output
    $0 --all --clang-format # Format all files with clang-format
    $0 --files "*.cpp" --exclude "external/*" # Format specific files

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

# Get clang-format style
get_clang_format_style() {
    if [ -f "$PROJECT_ROOT/.clang-format" ]; then
        echo "-style=file"
    else
        echo "-style=Google"
    fi
}

# Get uncrustify config
get_uncrustify_config() {
    if [ -f "$PROJECT_ROOT/.uncrustify.cfg" ]; then
        echo "-c $PROJECT_ROOT/.uncrustify.cfg"
    else
        echo ""
    fi
}

# Format a single file
format_file() {
    local file="$1"
    local formatter="$2"
    local style="$3"

    if [ "$VERBOSE" = "ON" ]; then
        print_info "Formatting: $file"
    fi

    if [ "$DRY_RUN" = "ON" ]; then
        print_info "Would format: $file"
        return 0
    fi

    case $formatter in
        "clang-format")
            if [ "$CHECK_ONLY" = "ON" ]; then
                if clang-format --dry-run --Werror $style "$file" >/dev/null 2>&1; then
                    print_info "✓ $file"
                    return 0
                else
                    print_warning "✗ $file"
                    if [ "$SHOW_DIFF" = "ON" ]; then
                        echo "--- Before ---"
                        head -n 10 "$file"
                        echo "--- After ---"
                        clang-format $style "$file" | head -n 10
                    fi
                    return 1
                fi
            else
                if clang-format $style "$file" > "$file.tmp" && mv "$file.tmp" "$file"; then
                    print_info "✓ $file"
                    return 0
                else
                    print_warning "✗ $file"
                    return 1
                fi
            fi
            ;;
        "uncrustify")
            local config=$(get_uncrustify_config)
            if [ "$CHECK_ONLY" = "ON" ]; then
                if uncrustify --check $config "$file" >/dev/null 2>&1; then
                    print_info "✓ $file"
                    return 0
                else
                    print_warning "✗ $file"
                    return 1
                fi
            else
                if uncrustify $config "$file" -o "$file.tmp" && mv "$file.tmp" "$file"; then
                    print_info "✓ $file"
                    return 0
                else
                    print_warning "✗ $file"
                    return 1
                fi
            fi
            ;;
        "autopep8")
            if [ "$CHECK_ONLY" = "ON" ]; then
                if autopep8 --diff --exit-code "$file" >/dev/null 2>&1; then
                    print_info "✓ $file"
                    return 0
                else
                    print_warning "✗ $file"
                    return 1
                fi
            else
                if autopep8 --in-place "$file"; then
                    print_info "✓ $file"
                    return 0
                else
                    print_warning "✗ $file"
                    return 1
                fi
            fi
            ;;
        "black")
            if [ "$CHECK_ONLY" = "ON" ]; then
                if black --check --diff "$file" >/dev/null 2>&1; then
                    print_info "✓ $file"
                    return 0
                else
                    print_warning "✗ $file"
                    return 1
                fi
            else
                if black "$file"; then
                    print_info "✓ $file"
                    return 0
                else
                    print_warning "✗ $file"
                    return 1
                fi
            fi
            ;;
    esac
}

# Format C++ files
format_cpp_files() {
    print_section "Formatting C++ Files"

    local cpp_files=()
    local errors=0
    local formatted=0

    # Find C++ files
    if [ -n "$FILE_PATTERN" ]; then
        while IFS= read -r -d '' file; do
            cpp_files+=("$file")
        done < <(find "$PROJECT_ROOT" -name "$FILE_PATTERN" -print0)
    else
        while IFS= read -r -d '' file; do
            cpp_files+=("$file")
        done < <(find "$PROJECT_ROOT" -name "*.cpp" -print0)
    fi

    # Exclude files
    if [ -n "$EXCLUDE_PATTERN" ]; then
        local filtered_files=()
        for file in "${cpp_files[@]}"; do
            if [[ ! "$file" == *"$EXCLUDE_PATTERN"* ]]; then
                filtered_files+=("$file")
            fi
        done
        cpp_files=("${filtered_files[@]}")
    fi

    # Format files
    for file in "${cpp_files[@]}"; do
        if [ "$USE_CLANG_FORMAT" = "ON" ]; then
            format_file "$file" "clang-format" "$(get_clang_format_style)"
        elif [ "$USE_UNCRUSTIFY" = "ON" ]; then
            format_file "$file" "uncrustify" "$(get_uncrustify_config)"
        else
            format_file "$file" "clang-format" "$(get_clang_format_style)"
        fi

        if [ $? -eq 0 ]; then
            ((formatted++))
        else
            ((errors++))
        fi
    done

    print_section "C++ Formatting Summary"
    echo "  Files formatted: $formatted"
    echo "  Errors: $errors"

    if [ $errors -eq 0 ]; then
        print_success "All C++ files formatted successfully"
        return 0
    else
        print_error "$errors C++ files had formatting issues"
        return 1
    fi
}

# Format header files
format_header_files() {
    print_section "Formatting Header Files"

    local header_files=()
    local errors=0
    local formatted=0

    # Find header files
    while IFS= read -r -d '' file; do
        header_files+=("$file")
    done < <(find "$PROJECT_ROOT" -name "*.h" -print0)

    # Exclude files
    if [ -n "$EXCLUDE_PATTERN" ]; then
        local filtered_files=()
        for file in "${header_files[@]}"; do
            if [[ ! "$file" == *"$EXCLUDE_PATTERN"* ]]; then
                filtered_files+=("$file")
            fi
        done
        header_files=("${filtered_files[@]}")
    fi

    # Format files
    for file in "${header_files[@]}"; do
        if [ "$USE_CLANG_FORMAT" = "ON" ]; then
            format_file "$file" "clang-format" "$(get_clang_format_style)"
        elif [ "$USE_UNCRUSTIFY" = "ON" ]; then
            format_file "$file" "uncrustify" "$(get_uncrustify_config)"
        else
            format_file "$file" "clang-format" "$(get_clang_format_style)"
        fi

        if [ $? -eq 0 ]; then
            ((formatted++))
        else
            ((errors++))
        fi
    done

    print_section "Header Formatting Summary"
    echo "  Files formatted: $formatted"
    echo "  Errors: $errors"

    if [ $errors -eq 0 ]; then
        print_success "All header files formatted successfully"
        return 0
    else
        print_error "$errors header files had formatting issues"
        return 1
    fi
}

# Format CMake files
format_cmake_files() {
    print_section "Formatting CMake Files"

    local cmake_files=()
    local errors=0
    local formatted=0

    # Find CMake files
    while IFS= read -r -d '' file; do
        cmake_files+=("$file")
    done < <(find "$PROJECT_ROOT" -name "CMakeLists.txt" -print0)

    # Add additional CMake files
    while IFS= read -r -d '' file; do
        cmake_files+=("$file")
    done < <(find "$PROJECT_ROOT" -name "*.cmake" -print0)

    # Format files
    for file in "${cmake_files[@]}"; do
        if [ "$USE_UNCRUSTIFY" = "ON" ]; then
            format_file "$file" "uncrustify" "$(get_uncrustify_config)"
        else
            # Use a simple formatter for CMake files
            if [ "$CHECK_ONLY" = "ON" ]; then
                if ! cmakelint "$file" >/dev/null 2>&1; then
                    print_warning "✗ $file"
                    ((errors++))
                else
                    print_info "✓ $file"
                    ((formatted++))
                fi
            else
                if cmakelint --fix "$file" >/dev/null 2>&1; then
                    print_info "✓ $file"
                    ((formatted++))
                else
                    print_warning "✗ $file"
                    ((errors++))
                fi
            fi
        fi

        if [ $? -eq 0 ]; then
            ((formatted++))
        else
            ((errors++))
        fi
    done

    print_section "CMake Formatting Summary"
    echo "  Files formatted: $formatted"
    echo "  Errors: $errors"

    if [ $errors -eq 0 ]; then
        print_success "All CMake files formatted successfully"
        return 0
    else
        print_error "$errors CMake files had formatting issues"
        return 1
    fi
}

# Format Python files
format_python_files() {
    print_section "Formatting Python Files"

    local python_files=()
    local errors=0
    local formatted=0

    # Find Python files
    while IFS= read -r -d '' file; do
        python_files+=("$file")
    done < <(find "$PROJECT_ROOT" -name "*.py" -print0)

    # Exclude files
    if [ -n "$EXCLUDE_PATTERN" ]; then
        local filtered_files=()
        for file in "${python_files[@]}"; do
            if [[ ! "$file" == *"$EXCLUDE_PATTERN"* ]]; then
                filtered_files+=("$file")
            fi
        done
        python_files=("${filtered_files[@]}")
    fi

    # Format files
    for file in "${python_files[@]}"; do
        if [ "$USE_BLACK" = "ON" ]; then
            format_file "$file" "black"
        elif [ "$USE_AUTOPEP8" = "ON" ]; then
            format_file "$file" "autopep8"
        else
            # Default to black
            format_file "$file" "black"
        fi

        if [ $? -eq 0 ]; then
            ((formatted++))
        else
            ((errors++))
        fi
    done

    print_section "Python Formatting Summary"
    echo "  Files formatted: $formatted"
    echo "  Errors: $errors"

    if [ $errors -eq 0 ]; then
        print_success "All Python files formatted successfully"
        return 0
    else
        print_error "$errors Python files had formatting issues"
        return 1
    fi
}

# Show formatting statistics
show_stats() {
    print_section "Formatting Statistics"

    local total_files=0
    local cpp_files=0
    local header_files=0
    local cmake_files=0
    local python_files=0

    # Count files
    while IFS= read -r -d '' file; do
        total_files=$((total_files + 1))
    done < <(find "$PROJECT_ROOT" -type f -print0)

    while IFS= read -r -d '' file; do
        cpp_files=$((cpp_files + 1))
    done < <(find "$PROJECT_ROOT" -name "*.cpp" -print0)

    while IFS= read -r -d '' file; do
        header_files=$((header_files + 1))
    done < <(find "$PROJECT_ROOT" -name "*.h" -print0)

    while IFS= read -r -d '' file; do
        cmake_files=$((cmake_files + 1))
    done < <(find "$PROJECT_ROOT" -name "CMakeLists.txt" -print0)

    while IFS= read -r -d '' file; do
        python_files=$((python_files + 1))
    done < <(find "$PROJECT_ROOT" -name "*.py" -print0)

    echo "  Total files: $total_files"
    echo "  C++ files: $cpp_files"
    echo "  Header files: $header_files"
    echo "  CMake files: $cmake_files"
    echo "  Python files: $python_files"
}

# Main function
main() {
    # Parse arguments
    parse_args "$@"

    # Print header
    print_header "Zenith DAW Code Formatter Script"
    echo

    # Check which formatters are available
    if [ "$USE_CLANG_FORMAT" = "OFF" ] && [ "$USE_UNCRUSTIFY" = "OFF" ] && [ "$USE_AUTOPEP8" = "OFF" ] && [ "$USE_BLACK" = "OFF" ]; then
        # Default to clang-format for C++ and black for Python
        if command_exists clang-format; then
            USE_CLANG_FORMAT="ON"
        fi
        if command_exists black; then
            USE_BLACK="ON"
        elif command_exists autopep8; then
            USE_AUTOPEP8="ON"
        fi
    fi

    # Check if required tools are available
    if [ "$USE_CLANG_FORMAT" = "ON" ] && ! command_exists clang-format; then
        print_error "clang-format not found"
        exit 1
    fi

    if [ "$USE_UNCRUSTIFY" = "ON" ] && ! command_exists uncrustify; then
        print_error "uncrustify not found"
        exit 1
    fi

    if [ "$USE_AUTOPEP8" = "ON" ] && ! command_exists autopep8; then
        print_error "autopep8 not found"
        exit 1
    fi

    if [ "$USE_BLACK" = "ON" ] && ! command_exists black; then
        print_error "black not found"
        exit 1
    fi

    # Show available formatters
    print_info "Available formatters:"
    if [ "$USE_CLANG_FORMAT" = "ON" ]; then
        print_info "  clang-format ✓"
    fi
    if [ "$USE_UNCRUSTIFY" = "ON" ]; then
        print_info "  uncrustify ✓"
    fi
    if [ "$USE_AUTOPEP8" = "ON" ]; then
        print_info "  autopep8 ✓"
    fi
    if [ "$USE_BLACK" = "ON" ]; then
        print_info "  black ✓"
    fi

    # Show formatting statistics
    if [ "$SHOW_STATS" = "ON" ]; then
        show_stats
        echo
    fi

    # Format files based on options
    local format_errors=0

    if [ "$FORMAT_ALL" = "ON" ]; then
        if [ "$FORMAT_CPP" = "OFF" ] && [ "$FORMAT_HEADER" = "OFF" ] && [ "$FORMAT_CMAKE" = "OFF" ] && [ "$FORMAT_PYTHON" = "OFF" ]; then
            # Format all types
            format_cpp_files || ((format_errors++))
            format_header_files || ((format_errors++))
            format_cmake_files || ((format_errors++))
            format_python_files || ((format_errors++))
        else
            # Format specified types
            format_cpp_files || ((format_errors++))
            format_header_files || ((format_errors++))
            format_cmake_files || ((format_errors++))
            format_python_files || ((format_errors++))
        fi
    else
        # Format specific types
        if [ "$FORMAT_CPP" = "ON" ]; then
            format_cpp_files || ((format_errors++))
        fi
        if [ "$FORMAT_HEADER" = "ON" ]; then
            format_header_files || ((format_errors++))
        fi
        if [ "$FORMAT_CMAKE" = "ON" ]; then
            format_cmake_files || ((format_errors++))
        fi
        if [ "$FORMAT_PYTHON" = "ON" ]; then
            format_python_files || ((format_errors++))
        fi

        # If no specific type selected, format C++ files
        if [ "$FORMAT_CPP" = "OFF" ] && [ "$FORMAT_HEADER" = "OFF" ] && [ "$FORMAT_CMAKE" = "OFF" ] && [ "$FORMAT_PYTHON" = "OFF" ]; then
            format_cpp_files || ((format_errors++))
        fi
    fi

    # Show summary
    print_section "Formatting Summary"
    echo

    if [ "$CHECK_ONLY" = "ON" ]; then
        print_info "Formatting check completed"
    else
        print_info "Formatting completed"
    fi

    if [ $format_errors -eq 0 ]; then
        print_success "All files formatted successfully!"
        exit 0
    else
        print_error "$format_errors formatting errors found"
        exit 1
    fi
}

# Print header
print_header() {
    echo -e "${PURPLE}$1${NC}"
}

# Run main function with all arguments
main "$@"