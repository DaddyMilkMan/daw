# Zenith DAW Testing Guide

This document describes the comprehensive testing infrastructure for Zenith DAW, designed to ensure stability, performance, and security before public release.

## Overview

Zenith DAW includes multiple layers of testing:

- **Unit Tests**: Component-level testing using JUCE UnitTest framework
- **Performance Benchmarks**: Real-time audio performance validation
- **Stress Tests**: WCET (Worst-Case Execution Time) testing for real-time constraints
- **Integration Tests**: End-to-end testing of features like collaboration
- **Security Scans**: CodeQL and Bandit analysis
- **Code Coverage**: Line and function coverage tracking

## Quick Start

### Running Tests Locally

```bash
# Build tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build --target ZenithDAWTests

# Run all tests
python3 scripts/run_tests.py --all

# Quick smoke tests only
python3 scripts/run_tests.py --quick

# Performance benchmarks only
python3 scripts/run_tests.py --performance

# Generate coverage report
python3 scripts/run_tests.py --coverage
```

### Running Specific Test Categories

```bash
cd build
./ZenithDAWTests AudioEngine    # Audio engine tests only
./ZenithDAWTests UI             # UI tests only
./ZenithDAWTests Collaboration  # Collaboration tests only
```

## CI/CD Pipeline

The project includes a comprehensive GitHub Actions workflow (`.github/workflows/ci.yml`) that:

1. **Builds** on Linux (GCC/Clang), macOS, and Windows
2. **Runs** all unit tests across platforms
3. **Performs** stress testing with WCET monitoring
4. **Executes** performance benchmarks
5. **Generates** code coverage reports
6. **Runs** security scans (CodeQL, Bandit)
7. **Publishes** test results as PR comments
8. **Creates** release artifacts on tagged releases

### Manual CI Trigger

```bash
# Trigger workflow manually via GitHub CLI
gh workflow run ci.yml
```

## Test Infrastructure

### Test Scripts

Located in `scripts/`:

- **`run_tests.py`**: Main test runner with multiple modes
- **`test_reporter.py`**: Comprehensive report generator

### Test Executables

Built by CMake:

- **`ZenithDAWTests`**: Main unit test runner
- **`GrokAITests`**: AI system tests
- **`WCETStressTests`**: Real-time stress tests
- **`MixerChannelBenchmark`**: Mixer performance benchmarks
- **`WavetableBenchmark`**: Wavetable loader benchmarks
- **`SampleGeneratorBenchmarkApp`**: Sample generation benchmarks

## Test Categories

### Unit Tests (60+ test files)

Located in `apps/desktop/Source/tests/`:

- **AudioEngineTests**: Core audio engine, tracks, clips, mixer
- **UITests**: UI components and rendering
- **ProjectStateTests**: Project save/load state management
- **ConcurrencyTests**: Threading and synchronization
- **ThemeTests**: UI theming system
- **AccessibilityTests**: Accessibility compliance
- **Midi2Tests**: MIDI 2.0 support
- **AITests**: AI integration and Grok API
- **CRDTSyncTests**: Real-time collaboration CRDT
- **PluginAutomationTests**: VST3 plugin hosting
- **OnnxPipelineTests**: ONNX runtime integration
- **And more...**

### Performance Benchmarks

Tests located in `apps/desktop/Source/tests/`:

- **MixerChannelBenchmark**: Mixer channel processing performance
- **WavetableLoaderBenchmark**: Wavetable synthesis performance
- **SampleGeneratorBenchmark**: Sample generation performance

Run with:
```bash
cd build
./MixerChannelBenchmark --json-output results.json
```

### Stress Tests (WCET)

**WCETStressTests** validates real-time audio constraints:

- Measures callback execution times
- Tracks missed deadlines
- Reports P99 latencies
- Validates buffer processing

Run with:
```bash
cd build
./WCETStressTests --duration 60 --json-output wcet_results.json
```

## Test Reporting

### Generate Comprehensive Reports

```bash
python3 scripts/test_reporter.py --input-dir build --output-dir test_reports
```

This generates:
- `test_report.json`: Machine-readable JSON
- `test_report.html`: Beautiful HTML dashboard
- `test_summary.md`: Markdown summary for PRs
- `ci_summary.txt`: Plain text CI summary

### HTML Report Features

The HTML report includes:
- Test summary with success rates
- Platform-specific results
- Performance benchmarks with charts
- Code coverage metrics
- WCET stress test analysis
- Security scan results

## Code Coverage

### Generate Coverage Reports

```bash
# Configure with coverage
cmake -B build -DZENITH_ENABLE_COVERAGE=ON

# Build and run tests
cmake --build build --target ZenithDAWTests
./build/ZenithDAWTests

# Generate report
genhtml build/coverage.info --output-directory coverage_report
```

### Coverage Targets

- **Minimum**: 60% line coverage
- **Target**: 80% line coverage
- **Functions**: 70% function coverage

## Security Testing

### Automated Security Scans

The CI pipeline includes:

1. **CodeQL Analysis**: C++ and Python security scanning
2. **Bandit**: Python security linting

### Manual Security Scan

```bash
# Python security
pip3 install bandit
bandit -r services/ai/runtime/

# C++ security (requires CodeQL CLI)
codeql database create cpp-database --language=cpp
codeql analyze cpp-database --format=sarifv2.1.0 --output=results.sarif
```

## Performance Testing

### Benchmarking Best Practices

1. **Use Release builds**: Debug builds are significantly slower
2. **Run on target hardware**: Test on representative systems
3. **Multiple runs**: Run benchmarks 3+ times and use average
4. **Control variables**: Close other applications

### Performance Thresholds

- **Mixer Channel**: < 1ms average processing
- **Wavetable Loading**: < 50ms for large wavetables
- **Sample Generation**: > 100,000 samples/second
- **WCET**: < 5ms max, < 2ms average

## Continuous Testing

### Pre-Commit Checks

Recommended local checks before committing:

```bash
#!/bin/bash
# .git/hooks/pre-commit

echo "Running tests..."
python3 scripts/run_tests.py --quick || exit 1

echo "Checking coverage..."
# (optional) coverage check
echo "All checks passed!"
```

### Nightly Tests

The CI runs daily at 2 AM UTC with:
- Full test suite on all platforms
- Extended stress testing (5 minutes)
- Coverage report generation
- Security scan updates

## Debugging Test Failures

### Enable Verbose Output

```bash
./ZenithDAWTests --verbose
```

### Generate Detailed Logs

```bash
./ZenithDAWTests 2>&1 | tee test_debug.log
```

### Run Single Test

```bash
./ZenithDAWTests AudioEngine.TrackProcessing
```

### Debug Specific Category

```bash
# Run with JUCE debug output
./ZenithDAWTests --juce-debug AudioEngine
```

## Platform-Specific Notes

### Linux
- Ensure ALSA or JACK is installed for audio tests
- Some tests may require X11 display

### macOS
- Tests may require microphone permission
- Some UI tests require screen recording permission

### Windows
- Tests should run on Windows 10/11
- May require audio device selection

## Release Readiness Checklist

Before shipping to public, ensure:

- [ ] All unit tests pass on all platforms
- [ ] Performance benchmarks meet targets
- [ ] WCET stress tests show 0 missed deadlines
- [ ] Code coverage >= 70%
- [ ] Security scans show 0 high severity issues
- [ ] Integration tests pass (collaboration, plugins)
- [ ] Documentation is complete
- [ ] Beta testing with real users completed
- [ ] Crash recovery tested
- [ ] Memory leaks checked (AddressSanitizer)

## Troubleshooting

### Common Issues

**Tests fail with "Audio device not found"**
- Install virtual audio driver (e.g., snd-dummy on Linux)
- Tests use mock devices when available

**Performance benchmarks are slow**
- Ensure Release build: `cmake -DCMAKE_BUILD_TYPE=Release`
- Close other applications
- Check power settings (not in battery saver mode)

**Coverage report generation fails**
- Ensure lcov/gcovr are installed
- Check that tests were built with coverage: `-DZENITH_ENABLE_COVERAGE=ON`

## Contributing Tests

### Adding New Tests

1. Create test file in `apps/desktop/Source/tests/`
2. Inherit from `juce::UnitTest`
3. Register test with static instance
4. Add to CMakeLists.txt if needed

Example:
```cpp
class MyNewTest : public juce::UnitTest {
public:
    MyNewTest() : juce::UnitTest("My New Test", "Category") {}
    
    void runTest() override {
        beginTest("Feature works");
        expect(myFeature.work());
    }
};

static MyNewTest myNewTest;
```

## Resources

- [JUCE UnitTest Documentation](https://docs.juce.com/master/classUnitTest.html)
- [Google Test Primer](https://google.github.io/googletest/primer.html) (for some tests)
- [CMake Testing Guide](https://cmake.org/cmake/help/latest/manual/ctest.1.html)

## Contact

For testing questions or issues:
- Open an issue on GitHub
- Contact the testing team
