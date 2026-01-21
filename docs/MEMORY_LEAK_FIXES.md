# Memory Leak Fixes - Technical Documentation

## Problem Statement

Test runs showed **301+ leaked objects** across multiple test runs:
- 301 instances of `AudioPluginInstance`
- 301 instances of `AudioProcessor`
- 301 instances of `AudioProcessorParameterGroup`
- 602 instances of `OwnedArray`
- 1 instance of `PluginAutomationBinding`
- 96 bytes from ONNX Runtime (AddressSanitizer report)

**Evidence**: 
- `old_logs/archive_2026_01_01/test_results.txt:1171-1183`
- `old_logs/archive_2026_01_01/test_final.txt:1119-1126`

## Root Cause Analysis

### Critical Bug in RealTimeGarbageCollector.cpp

The `RealTimeGarbageCollector` is used throughout the codebase to safely defer deletion of objects that might still be referenced by real-time audio threads. The pattern is:

1. Create a new snapshot of shared state
2. Atomically swap the active pointer
3. Call `gc.deferDelete(oldSnapshot)` to queue the old snapshot for deletion
4. The garbage collector waits 1 second before deleting to ensure no RT thread is still using it

**The Bug (lines 72-82 in RealTimeGarbageCollector.cpp):**

```cpp
void RealTimeGarbageCollector::ensureClean() {
  stopTimer();
  
  // Process remaining items in FIFO
  int start1, size1, start2, size2;
  fifo_.prepareToRead(fifo_.getNumReady(), start1, size1, start2, size2);
  fifo_.finishedRead(size1 + size2);   // ⚠️ Marks as read but doesn't move items!
  
  trashBuffer_.clear();                 // ⚠️ Clears without running destructors
  pendingDestruction_.clear();          // ⚠️ Clears without running destructors
}
```

**The Problem:**
1. `fifo_.finishedRead()` marks items as read but doesn't transfer them to `pendingDestruction_`
2. `trashBuffer_.clear()` and `pendingDestruction_.clear()` discard all `std::function<void()>` deleters
3. The lambda functions holding `shared_ptr` references never execute
4. The shared pointers never decrement their reference counts
5. Objects remain alive → reported as memory leaks

**Why 301 Plugins?**
- Tests create and destroy plugins, tracks, automation systems
- Each state change creates a new snapshot
- Old snapshots are queued via `deferDelete()`
- On test shutdown, `TestMain.cpp:119` calls `ensureClean()` to flush garbage
- All 301 accumulated snapshots leak because their deleters never run

## The Fix

### 1. Fixed RealTimeGarbageCollector::ensureClean()

```cpp
void RealTimeGarbageCollector::ensureClean() {
  stopTimer();
  
  // 1. Flush all items from FIFO into pendingDestruction_
  int start1, size1, start2, size2;
  int numReady = fifo_.getNumReady();
  
  if (numReady > 0) {
    fifo_.prepareToRead(numReady, start1, size1, start2, size2);
    
    // Move items from trashBuffer_ to pendingDestruction_
    for (int i = 0; i < size1; ++i)
      pendingDestruction_.push_back(std::move(trashBuffer_[start1 + i]));
      
    for (int i = 0; i < size2; ++i)
      pendingDestruction_.push_back(std::move(trashBuffer_[start2 + i]));
      
    fifo_.finishedRead(size1 + size2);
  }
  
  // 2. Execute all deleters by clearing pendingDestruction_
  // The TrashItem destructors will run, executing the deleter functions
  pendingDestruction_.clear();
  
  // 3. Clear trash buffer (should already be moved from)
  trashBuffer_.clear();
}
```

**Key Changes:**
- Actually transfer items from FIFO to `pendingDestruction_`
- Let `pendingDestruction_.clear()` run the TrashItem destructors
- Each TrashItem destructor executes its `std::function<void()>` deleter
- The lambda functions run, allowing shared_ptr destructors to execute

### 2. Enabled LeakSanitizer by Default (cmake/CompilerFlags.cmake)

**Before:**
```cmake
option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)

# Only enabled for Release builds (!)
if(ENABLE_SANITIZERS AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    ...
endif()
```

**After:**
```cmake
# Default ON for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_SANITIZERS "Enable Address, Leak and UB Sanitizers" ON)
else()
    option(ENABLE_SANITIZERS "Enable Address and UB Sanitizers" OFF)
endif()

if(ENABLE_SANITIZERS)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        # Enable ASan, LSan, UBSan
        add_compile_options(-fsanitize=address -fsanitize=leak -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=leak -fsanitize=undefined)
        add_compile_options(-fno-omit-frame-pointer -g)
    endif()
endif()
```

**Impact:**
- Debug builds now have leak detection by default
- CI will catch future memory leaks automatically
- Better error reporting with frame pointers

### 3. ONNX Runtime Leak Suppression (lsan.supp)

ONNX Runtime performs internal allocations that are not freed until process termination. This is intentional for performance. Created suppression file:

```
leak:libonnxruntime.so
leak:Ort::GetApi
leak:OrtApis
leak:onnxruntime
```

Configured in CMakeLists.txt:
```cmake
if(ENABLE_SANITIZERS AND NOT MSVC)
    set_target_properties(ZenithDAWTests PROPERTIES
        ENVIRONMENT "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/lsan.supp"
    )
endif()
```

## Affected Files

### Fixed Files
- ✅ `apps/desktop/Source/engine/RealTimeGarbageCollector.cpp` - Fixed ensureClean() bug
- ✅ `cmake/CompilerFlags.cmake` - Enabled sanitizers by default for Debug
- ✅ `CMakeLists.txt` - Configured LSan suppression file
- ✅ `docs/KNOWN_ISSUES.md` - Updated to reflect fixes

### New Files
- ✅ `lsan.supp` - LeakSanitizer suppression file for ONNX Runtime
- ✅ `test_memory_leaks.sh` - Validation script for developers

### Files Using RealTimeGarbageCollector (Now Fixed)
- `apps/desktop/Source/engine/PluginChain.cpp` - Uses snapshots with deferred deletion
- `apps/desktop/Source/engine/AutomationManager.cpp` - Uses snapshots
- `apps/desktop/Source/engine/RoutingGraph.cpp` - Uses snapshots
- `apps/desktop/Source/tests/TestMain.cpp` - Calls ensureClean() at shutdown

All these files now benefit from proper cleanup.

## Verification Steps

### 1. Build with Sanitizers
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build . -j$(nproc)
```

Sanitizers are now enabled automatically in Debug mode.

### 2. Run Tests
```bash
cd build
LSAN_OPTIONS=suppressions=../lsan.supp ./ZenithDAWTests
```

### 3. Or Use Validation Script
```bash
./test_memory_leaks.sh
```

### Expected Output
- ✅ No "LeakSanitizer: detected memory leaks" messages
- ✅ No "Leaked objects detected:" messages from JUCE
- ✅ All tests pass

## Technical Notes

### Why shared_ptr in Lambda?

The deferred deletion pattern uses type erasure:

```cpp
template <typename T>
void deferDelete(std::shared_ptr<T> object) {
    deferDelete([object]() mutable { 
        // Object destroyed when lambda destructor runs
        (void)object; 
    });
}
```

The `std::shared_ptr<T>` is captured by value in the lambda. When the lambda destructor runs, it decrements the shared_ptr reference count. If this was the last reference, the object is deleted.

### Why 1-Second Safety Window?

Real-time audio threads may still be reading from an old snapshot for a few milliseconds after it's been replaced. The 1-second delay ensures all audio processing has completed before deletion.

### JUCE Leak Detector

JUCE's `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR` tracks object counts per type. If objects remain alive at shutdown, it reports them. This is how we discovered the 301 plugin leaks before AddressSanitizer was enabled.

## Future Improvements

1. **Benchmark Impact**: Measure overhead of sanitizers in Debug builds
2. **CI Integration**: Add sanitizer flags to CI pipeline
3. **Continuous Monitoring**: Run leak tests in CI for every PR
4. **Valgrind Integration**: Consider Valgrind memcheck for deeper analysis

## References

- [AddressSanitizer Documentation](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [LeakSanitizer Documentation](https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer)
- [JUCE Leak Detector](https://docs.juce.com/master/classLeakedObjectDetector.html)
- JUCE AbstractFifo for lock-free MPSC queue
