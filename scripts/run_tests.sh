#!/bin/bash

# Zenith DAW Test Runner Script
# Usage: ./run_tests.sh [test_type] [options]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
TEST_EXECUTABLE="ZenithDAWTests"
COVERAGE_DIR="coverage"
LOG_FILE="test_results.log"

# Functions
print_usage() {
    echo -e "${BLUE}Zenith DAW Test Runner${NC}"
    echo ""
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  build           - Build tests only"
    echo "  run             - Run all tests"
    echo "  unit [name]    - Run specific unit test"
    echo "  integration     - Run integration tests"
    echo "  performance     - Run performance benchmarks"
    echo "  coverage        - Generate coverage report"
    echo "  clean           - Clean test artifacts"
    echo "  help            - Show this help"
    echo ""
    echo "Options:"
    echo "  -v, --verbose  - Verbose output"
    echo "  -j, --jobs N   - Number of parallel jobs (default: auto)"
    echo "  -f, --filter   - Filter tests by name pattern"
    echo "  --no-gpu       - Disable GPU for tests"
    echo "  --mem-check     - Run with memory checker (valgrind)"
}

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

# Pre-flight checks
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check if build directory exists
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory '$BUILD_DIR' not found. Run 'cmake' and 'make' first."
        exit 1
    fi
    
    # Check if test executable exists
    if [ ! -f "$BUILD_DIR/$TEST_EXECUTABLE" ]; then
        print_warning "Test executable not found. Building tests first..."
        build_tests
    fi
    
    # Check for required tools
    command -v cmake >/dev/null 2>&1 || { print_error "cmake not found. Please install cmake."; exit 1; }
    command -v make >/dev/null 2>&1 || { print_error "make not found. Please install make."; exit 1; }
    
    print_success "Prerequisites check passed"
}

# Build tests
build_tests() {
    print_status "Building tests..."
    
    cd "$BUILD_DIR"
    
    # Configure for tests
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_TESTS=ON \
        -DENABLE_COVERAGE=ON \
        -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
        -DCMAKE_C_FLAGS="--coverage -fprofile-arcs -ftest-coverage"
    
    # Build tests
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) "$TEST_EXECUTABLE"
    
    if [ $? -eq 0 ]; then
        print_success "Tests built successfully"
    else
        print_error "Failed to build tests"
        exit 1
    fi
    
    cd ..
}

# Run all tests
run_all_tests() {
    print_status "Running all unit tests..."
    
    cd "$BUILD_DIR"
    
    # Create timestamp for this test run
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    RESULTS_DIR="test_results_$TIMESTAMP"
    mkdir -p "$RESULTS_DIR"
    
    # Run tests with output capture
    ./"$TEST_EXECUTABLE" \
        --verbose \
        --output-dir="$RESULTS_DIR" \
        --junit-xml="$RESULTS_DIR/test_results.xml" \
        2>&1 | tee "$LOG_FILE"
    
    # Parse results
    if [ -f "$RESULTS_DIR/test_results.xml" ]; then
        # Extract test count and pass/fail rates
        TOTAL_TESTS=$(grep -o 'tests="[0-9]*' "$RESULTS_DIR/test_results.xml" | cut -d'"' -f2)
        FAILED_TESTS=$(grep -o 'failures="[0-9]*' "$RESULTS_DIR/test_results.xml" | cut -d'"' -f2)
        PASSED_TESTS=$((TOTAL_TESTS - FAILED_TESTS))
        
        echo ""
        print_success "Test Results:"
        echo "  Total: $TOTAL_TESTS"
        echo -e "  Passed: ${GREEN}$PASSED_TESTS${NC}"
        if [ $FAILED_TESTS -gt 0 ]; then
            echo -e "  Failed: ${RED}$FAILED_TESTS${NC}"
        else
            echo -e "  Failed: ${GREEN}0${NC}"
        fi
    else
        print_warning "No test results XML found. Check output above."
    fi
    
    cd ..
}

# Run specific test
run_unit_test() {
    local test_name="$1"
    print_status "Running unit test: $test_name"
    
    cd "$BUILD_DIR"
    ./"$TEST_EXECUTABLE" --filter="$test_name" --verbose
    cd ..
}

# Run integration tests
run_integration_tests() {
    print_status "Running integration tests..."
    
    # Test project loading
    test_project_load
    # Test plugin loading  
    test_plugin_load
    # Test audio export
    test_audio_export
}

test_project_load() {
    print_status "Testing project load/save..."
    
    # Create test project
    TEST_PROJECT="/tmp/zenith_test_project.zenith"
    cd "$BUILD_DIR"
    ./ZenithDAW --create-test-project="$TEST_PROJECT" --headless
    
    if [ -f "$TEST_PROJECT" ]; then
        print_success "Project creation test passed"
        
        # Test project loading
        ./ZenithDAW --load-project="$TEST_PROJECT" --validate --headless
        if [ $? -eq 0 ]; then
            print_success "Project loading test passed"
        else
            print_error "Project loading test failed"
        fi
        
        # Cleanup
        rm -f "$TEST_PROJECT"
    else
        print_error "Project creation test failed"
    fi
    
    cd ..
}

test_plugin_load() {
    print_status "Testing plugin loading..."
    
    cd "$BUILD_DIR"
    ./ZenithDAW --scan-plugins --validate --headless
    if [ $? -eq 0 ]; then
        print_success "Plugin loading test passed"
    else
        print_error "Plugin loading test failed"
    fi
    
    cd ..
}

test_audio_export() {
    print_status "Testing audio export..."
    
    TEST_WAV="/tmp/zenith_test_export.wav"
    cd "$BUILD_DIR"
    
    # Create test export
    ./ZenithDAW --export-test-audio="$TEST_WAV" --duration=5 --headless
    
    if [ -f "$TEST_WAV" ]; then
        print_success "Audio export test passed"
        
        # Validate WAV file
        if file "$TEST_WAV" | grep -q "WAVE audio"; then
            print_success "Exported file format is valid"
        else
            print_warning "Exported file format may be invalid"
        fi
        
        # Cleanup
        rm -f "$TEST_WAV"
    else
        print_error "Audio export test failed"
    fi
    
    cd ..
}

# Generate coverage report
generate_coverage() {
    print_status "Generating coverage report..."
    
    if ! command -v gcov >/dev/null 2>&1; then
        print_warning "gcov not found. Install gcc/clang with coverage support."
        return 1
    fi
    
    cd "$BUILD_DIR"
    
    # Generate coverage data
    gcov *.gcno
    
    # Create HTML report (lcov preferred, fallback to gcov)
    if command -v lcov >/dev/null 2>&1; then
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        lcov --remove coverage.info '*/tests/*' --output-file coverage.info
        
        # Generate HTML
        genhtml coverage.info --output-directory "$COVERAGE_DIR"
        print_success "Coverage report generated in $COVERAGE_DIR/"
        
        # Open in browser
        if command -v xdg-open >/dev/null 2>&1; then
            xdg-open "$COVERAGE_DIR/index.html"
        elif command -v open >/dev/null 2>&1; then
            open "$COVERAGE_DIR/index.html"
        fi
    else
        gcov *.gcda
        print_success "Coverage files generated. Use gcov to analyze."
    fi
    
    cd ..
}

# Performance benchmarks
run_performance() {
    print_status "Running performance benchmarks..."
    
    cd "$BUILD_DIR"
    ./ZenithDAW --benchmark --output-json="benchmark_results.json"
    
    if [ -f "benchmark_results.json" ]; then
        print_success "Performance benchmarks completed"
        
        # Show summary
        python3 -c "
import json
with open('benchmark_results.json') as f:
    data = json.load(f)
    
print('Performance Summary:')
for test, results in data.items():
    if 'fps' in results:
        print(f'  {test}: {results[\"fps\"]:.1f} FPS')
    if 'latency' in results:
        print(f'  {test}: {results[\"latency\"]:.1f} ms')
    if 'cpu_usage' in results:
        print(f'  {test}: {results[\"cpu_usage\"]:.1f}%')
"
    else
        print_error "Performance benchmarks failed"
    fi
    
    cd ..
}

# Clean test artifacts
clean_tests() {
    print_status "Cleaning test artifacts..."
    
    cd "$BUILD_DIR"
    
    # Remove test results
    rm -rf test_results_*
    rm -f *.gcda *.gcno *.gcov
    
    # Remove coverage
    rm -rf "$COVERAGE_DIR"
    
    # Remove test executable
    rm -f "$TEST_EXECUTABLE"
    
    print_success "Test artifacts cleaned"
    cd ..
}

# Parse command line arguments
case "${1:-help}" in
    "build")
        check_prerequisites
        build_tests
        ;;
    "run")
        check_prerequisites
        run_all_tests
        ;;
    "unit")
        check_prerequisites
        run_unit_test "$2"
        ;;
    "integration")
        check_prerequisites
        run_integration_tests
        ;;
    "performance")
        check_prerequisites
        run_performance
        ;;
    "coverage")
        check_prerequisites
        generate_coverage
        ;;
    "clean")
        clean_tests
        ;;
    "help"|"-h"|"--help")
        print_usage
        ;;
    *)
        print_error "Unknown command: $1"
        echo ""
        print_usage
        exit 1
        ;;
esac

# Optional: Parse additional arguments
VERBOSE=false
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
FILTER_PATTERN=""
NO_GPU=false
MEM_CHECK=false

shift
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift
            ;;
        -f|--filter)
            FILTER_PATTERN="$2"
            shift
            ;;
        --no-gpu)
            NO_GPU=true
            export SDL_VIDEODRIVER=dummy
            ;;
        --mem-check)
            MEM_CHECK=true
            ;;
    esac
    shift
done

echo ""
print_success "Zenith DAW test script completed!"