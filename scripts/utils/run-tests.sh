#!/bin/bash

# =============================================================================
# Zenith DAW Test Runner Script
# =============================================================================
# This script runs tests with various configurations and options.
#
# Usage:
#   ./run-tests.sh [options]
#
# Options:
#   -h, --help          Show this help message
#   -u, --unit          Run unit tests only
#   -i, --integration   Run integration tests only
#   -a, --all           Run all tests (default)
#   -j, --jobs N        Use N parallel jobs (default: all cores)
#   -v, --verbose       Verbose test output
#   -c, --coverage      Run with coverage reporting
#   -r, --repeat N      Repeat tests N times
#   -t, --timeout N     Set test timeout in seconds
#   --filter PATTERN    Filter tests by pattern
#   --gtest-output     Generate Google Test output
#   --junit-output     Generate JUnit XML output
#   --html-output      Generate HTML report
#   --sanitize         Run with sanitizers (AddressSanitizer, LeakSanitizer)
#   --leak-check       Run with memory leak detection
#
# Examples:
#   ./run-tests.sh                  # Run all tests
#   ./run-tests.sh --unit --jobs 4 # Run unit tests with 4 jobs
#   ./run-tests.sh --coverage      # Run with coverage
#   ./run-tests.sh --filter Audio   # Run only Audio-related tests
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
BUILD_DIR="$PROJECT_ROOT/build"
TEST_TIMEOUT=60
TEST_REPEAT=1

# Default options
RUN_UNIT="OFF"
RUN_INTEGRATION="OFF"
RUN_ALL="ON"
JOBS=$(nproc 2>/dev/null || echo 4)
VERBOSE="OFF"
COVERAGE="OFF"
SANITIZE="OFF"
LEAK_CHECK="OFF"
FILTER_PATTERN=""
GTEST_OUTPUT=""
JUNIT_OUTPUT=""
HTML_OUTPUT=""

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -u|--unit)
                RUN_UNIT="ON"
                RUN_ALL="OFF"
                shift
                ;;
            -i|--integration)
                RUN_INTEGRATION="ON"
                RUN_ALL="OFF"
                shift
                ;;
            -a|--all)
                RUN_ALL="ON"
                RUN_UNIT="OFF"
                RUN_INTEGRATION="OFF"
                shift
                ;;
            -j|--jobs)
                JOBS="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE="ON"
                shift
                ;;
            -c|--coverage)
                COVERAGE="ON"
                shift
                ;;
            -r|--repeat)
                TEST_REPEAT="$2"
                shift 2
                ;;
            -t|--timeout)
                TEST_TIMEOUT="$2"
                shift 2
                ;;
            --filter)
                FILTER_PATTERN="$2"
                shift 2
                ;;
            --gtest-output)
                GTEST_OUTPUT="$2"
                shift 2
                ;;
            --junit-output)
                JUNIT_OUTPUT="$2"
                shift 2
                ;;
            --html-output)
                HTML_OUTPUT="$2"
                shift
                ;;
            --sanitize)
                SANITIZE="ON"
                shift
                ;;
            --leak-check)
                LEAK_CHECK="ON"
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
Zenith DAW Test Runner Script

This script runs tests with various configurations and options.

Usage:
    $0 [options]

Options:
    -h, --help          Show this help message
    -u, --unit          Run unit tests only
    -i, --integration   Run integration tests only
    -a, --all           Run all tests (default)
    -j, --jobs N        Use N parallel jobs (default: all cores)
    -v, --verbose       Verbose test output
    -c, --coverage      Run with coverage reporting
    -r, --repeat N      Repeat tests N times
    -t, --timeout N     Set test timeout in seconds
    --filter PATTERN    Filter tests by pattern
    --gtest-output     Generate Google Test output
    --junit-output     Generate JUnit XML output
    --html-output      Generate HTML report
    --sanitize         Run with sanitizers (AddressSanitizer, LeakSanitizer)
    --leak-check       Run with memory leak detection

Examples:
    $0                  # Run all tests
    $0 --unit --jobs 4 # Run unit tests with 4 jobs
    $0 --coverage      # Run with coverage
    $0 --filter Audio   # Run only Audio-related tests

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

# Check if build directory exists
check_build_dir() {
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory not found"
        print_info "Please run './scripts/build.sh' first"
        exit 1
    fi
}

# Get test executable path
get_test_executable() {
    local test_name="$1"

    # Check for different possible paths
    local paths=(
        "$BUILD_DIR/$test_name"
        "$BUILD_DIR/Release/$test_name"
        "$BUILD_DIR/Debug/$test_name"
        "$BUILD_DIR/x64/Release/$test_name"
        "$BUILD_DIR/x64/Debug/$test_name"
        "$BUILD_DIR/$test_name_artefacts/Release/$test_name"
        "$BUILD_DIR/$test_name_artefacts/Debug/$test_name"
    )

    for path in "${paths[@]}"; do
        if [ -f "$path" ]; then
            echo "$path"
            return 0
        fi
    done

    return 1
}

# Run a single test with options
run_test() {
    local test_name="$1"
    local test_executable="$2"
    local test_type="$3"

    local test_args=()
    local repeat_count=1

    # Add timeout
    if [ "$TEST_TIMEOUT" -gt 0 ]; then
        test_args+=("timeout" "$TEST_TIMEOUT")
    fi

    # Add sanitizers
    if [ "$SANITIZE" = "ON" ]; then
        local sanitizer_args=("-fsanitize=address,leak" "-g" "-fno-omit-frame-pointer")
        test_args+=("${sanitizer_args[@]}")
    fi

    # Add leak check
    if [ "$LEAK_CHECK" = "ON" ]; then
        test_args+=("LSAN_OPTIONS=suppressions=$SCRIPT_DIR/../lsan.supp")
    fi

    # Add filter
    if [ -n "$FILTER_PATTERN" ]; then
        test_args+=("--gtest_filter=$FILTER_PATTERN*")
    fi

    # Add verbose
    if [ "$VERBOSE" = "ON" ]; then
        test_args+=("--verbose")
    fi

    # Add output formats
    if [ -n "$GTEST_OUTPUT" ]; then
        test_args+=("--gtest_output=$GTEST_OUTPUT")
    fi
    if [ -n "$JUNIT_OUTPUT" ]; then
        test_args+=("--gtest_output=xml:$JUNIT_OUTPUT")
    fi

    # Add test executable
    test_args+=("$test_executable")

    # Test loop for repeat count
    local test_repeat=1
    local total_tests_passed=0
    local total_tests_failed=0

    while [ $test_repeat -le $TEST_REPEAT ]; do
        print_section "Running $test_name ($test_repeat/$TEST_REPEAT)"

        if [ "$test_repeat" -gt 1 ]; then
            print_info "Repeating test run $test_repeat of $TEST_REPEAT"
        fi

        # Run the test
        if "${test_args[@]}"; then
            print_success "$test_name passed"
            ((total_tests_passed++))
        else
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                print_error "$test_name timed out after $TEST_TIMEOUT seconds"
            else
                print_error "$test_name failed (exit code: $exit_code)"
            fi
            ((total_tests_failed++))
        fi

        ((test_repeat++))
    done

    # Report results
    if [ $total_tests_failed -eq 0 ]; then
        print_success "All $test_repeat runs of $test_name passed"
        return 0
    else
        print_error "$total_tests_failed out of $test_repeat runs of $test_name failed"
        return 1
    fi
}

# Run unit tests
run_unit_tests() {
    print_section "Running Unit Tests"

    local unit_tests=(
        "ZenithTests"
        "ZenithCoreTests"
        "ZenithUITests"
        "ZenithEngineTests"
    )

    local tests_passed=0
    local tests_failed=0

    for test_name in "${unit_tests[@]}"; do
        local test_executable=$(get_test_executable "$test_name")

        if [ -n "$test_executable" ]; then
            if run_test "$test_name" "$test_executable" "unit"; then
                ((tests_passed++))
            else
                ((tests_failed++))
            fi
        else
            print_warning "Unit test not found: $test_name"
        fi
    done

    print_section "Unit Test Summary"
    echo "  Passed: $tests_passed"
    echo "  Failed: $tests_failed"

    if [ $tests_failed -eq 0 ]; then
        print_success "All unit tests passed"
        return 0
    else
        print_error "$tests_failed unit tests failed"
        return 1
    fi
}

# Run integration tests
run_integration_tests() {
    print_section "Running Integration Tests"

    local integration_tests=(
        "ZenithIntegrationTests"
        "ZenithAudioTests"
        "ZenithPluginTests"
        "ZenithProjectTests"
    )

    local tests_passed=0
    local tests_failed=0

    for test_name in "${integration_tests[@]}"; do
        local test_executable=$(get_test_executable "$test_name")

        if [ -n "$test_executable" ]; then
            if run_test "$test_name" "$test_executable" "integration"; then
                ((tests_passed++))
            else
                ((tests_failed++))
            fi
        else
            print_warning "Integration test not found: $test_name"
        fi
    done

    print_section "Integration Test Summary"
    echo "  Passed: $tests_passed"
    echo "  Failed: $tests_failed"

    if [ $tests_failed -eq 0 ]; then
        print_success "All integration tests passed"
        return 0
    else
        print_error "$tests_failed integration tests failed"
        return 1
    fi
}

# Run CTest
run_ctest() {
    print_section "Running CTest Tests"

    cd "$BUILD_DIR"

    local ctest_args=()

    if [ "$VERBOSE" = "ON" ]; then
        ctest_args+=("-V")
    fi

    if [ "$TEST_TIMEOUT" -gt 0 ]; then
        ctest_args+=("--timeout" "$TEST_TIMEOUT")
    fi

    if [ -n "$FILTER_PATTERN" ]; then
        ctest_args+=("--R" "$FILTER_PATTERN")
    fi

    if ctest "${ctest_args[@]}"; then
        print_success "All CTest tests passed"
        cd - >/dev/null
        return 0
    else
        print_error "Some CTest tests failed"
        cd - >/dev/null
        return 1
    fi
}

# Generate coverage report
generate_coverage() {
    print_section "Generating Coverage Report"

    if command_exists lcov; then
        cd "$BUILD_DIR"

        # Generate coverage data
        lcov --capture --directory . --output-file coverage.info --rc lcov_branch_coverage=1

        # Remove system and test files
        lcov --remove coverage.info '/usr/*' '*/tests/*' '*/external/*' --output-file coverage.info

        # Generate HTML report
        genhtml coverage.info --output-directory coverage-html

        print_success "Coverage report generated in $BUILD_DIR/coverage-html/index.html"
        cd - >/dev/null
    else
        print_warning "lcov not found, skipping coverage report"
    fi
}

# Generate HTML report
generate_html_report() {
    print_section "Generating HTML Report"

    if command_exists pytest; then
        # This would use pytest with pytest-html plugin
        print_info "Generating HTML report..."
        # Placeholder for HTML report generation
    else
        print_warning "pytest not found, skipping HTML report"
    fi
}

# Show test summary
show_summary() {
    print_section "Test Summary"
    echo

    echo "Test Configuration:"
    echo "  Test Type: ${RUN_ALL:+All}${RUN_UNIT:+Unit}${RUN_INTEGRATION:+Integration}"
    echo "  Jobs: $JOBS"
    echo "  Verbose: $VERBOSE"
    echo "  Coverage: $COVERAGE"
    echo "  Sanitize: $SANITIZE"
    echo "  Repeat: $TEST_REPEAT"
    echo "  Timeout: $TEST_TIMEOUT seconds"
    if [ -n "$FILTER_PATTERN" ]; then
        echo "  Filter: $FILTER_PATTERN"
    fi

    echo
    print_success "Test run completed!"
    echo
    print_info "Output files:"
    if [ -n "$GTEST_OUTPUT" ]; then
        echo "  Google Test XML: $GTEST_OUTPUT"
    fi
    if [ -n "$JUNIT_OUTPUT" ]; then
        echo "  JUnit XML: $JUNIT_OUTPUT"
    fi
    if [ -n "$HTML_OUTPUT" ]; then
        echo "  HTML Report: $HTML_OUTPUT"
    fi
    if [ "$COVERAGE" = "ON" ]; then
        echo "  Coverage Report: $BUILD_DIR/coverage-html/index.html"
    fi
}

# Main function
main() {
    # Parse arguments
    parse_args "$@"

    # Print header
    print_header "Zenith DAW Test Runner Script"
    echo

    # Check build directory
    check_build_dir

    # Set parallel jobs
    export NINJA_STATUS="[%f/%t %p] "
    export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"

    # Run tests based on configuration
    local tests_passed=0
    local tests_failed=0

    if [ "$RUN_ALL" = "ON" ]; then
        if run_ctest; then
            ((tests_passed++))
        else
            ((tests_failed++))
        fi
    fi

    if [ "$RUN_UNIT" = "ON" ]; then
        if run_unit_tests; then
            ((tests_passed++))
        else
            ((tests_failed++))
        fi
    fi

    if [ "$RUN_INTEGRATION" = "ON" ]; then
        if run_integration_tests; then
            ((tests_passed++))
        else
            ((tests_failed++))
        fi
    fi

    # Generate reports
    if [ "$COVERAGE" = "ON" ]; then
        generate_coverage
    fi

    if [ -n "$HTML_OUTPUT" ]; then
        generate_html_report
    fi

    # Show summary
    show_summary

    # Exit with appropriate code
    if [ $tests_failed -eq 0 ]; then
        print_success "All tests passed!"
        exit 0
    else
        print_error "$tests_failed test suites failed"
        exit 1
    fi
}

# Print header
print_header() {
    echo -e "${PURPLE}$1${NC}"
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Run main function with all arguments
main "$@"