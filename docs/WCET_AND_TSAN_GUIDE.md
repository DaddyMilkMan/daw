# WCET Analysis and ThreadSanitizer Guide

This guide explains how to use the new WCET (Worst-Case Execution Time) monitoring and ThreadSanitizer (TSan) data race detection features in Zenith DAW.

---

## Table of Contents

1. [WCET Monitoring](#wcet-monitoring)
2. [ThreadSanitizer](#threadsanitizer)
3. [CI/CD Integration](#cicd-integration)
4. [Interpreting Results](#interpreting-results)

---

## WCET Monitoring

### What is WCET?

WCET (Worst-Case Execution Time) monitoring tracks how long your audio callback takes to execute. This helps detect potential xruns (audio dropouts) before they affect users.

### How It Works

```
Audio Callback Budget: 5ms (256 samples @ 48kHz)

Your Processing:    |=======|     3ms avg (good)
                    |=============| 6ms worst case (BAD - xrun!)
```

### Enabling WCET

WCET is enabled by default. To disable:

```bash
cmake -DZENITH_ENABLE_WCET=OFF ..
```

### Accessing WCET Data

```cpp
auto& wcet = engine.getWCETMonitor();
auto stats = wcet.getStatistics();

std::cout << "Average: " << stats.avgExecutionTimeUs << " us\n";
std::cout << "Worst case: " << stats.maxExecutionTimeUs << " us\n";
std::cout << "Overruns: " << stats.overruns << "\n";
```

### Running WCET Stress Tests

```bash
# Build and run
cmake --build build --target WCETStressTests
./build/WCETStressTests_artefacts/Release/WCETStressTests \
    --duration 60 \
    --max-tracks 50 \
    --report-file results.json
```

---

## ThreadSanitizer

### What is ThreadSanitizer?

ThreadSanitizer (TSan) detects data races - when two threads access the same memory without synchronization.

### Running TSan

```bash
mkdir build-tsan && cd build-tsan
cmake .. \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
    -DBUILD_TESTS=ON

cmake --build . --target ZenithDAWTests
TSAN_OPTIONS=detect_deadlocks=1:halt_on_error=1 \
    ./ZenithDAWTests_artefacts/Debug/ZenithDAWTests
```

### TSan Performance

- Slows execution by 5-15x
- CI runs nightly (not on every push)
- Use `-O1` for better performance

---

## CI/CD Integration

- **WCET**: Runs on every push to main/develop branches
- **TSan**: Runs nightly at 2 AM UTC
- Results uploaded as artifacts
- PR comments show WCET summary

---

## Quick Reference

### WCET Status

| Utilization | Status | Action |
|-------------|--------|--------|
| < 80% | Healthy | None |
| 80-95% | Warning | Monitor |
| > 95% | Critical | Optimize |

### Common TSan Fixes

- Data race on variable -> Use `std::atomic`
- Lock inversion -> Establish lock ordering
- Deadlock -> Use lock hierarchy

---

For detailed information, see the full code documentation in the source files.
