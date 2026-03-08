# Testing & Debugging Guide - Concrete Steps

**Created:** 2026-02-20
**For:** User to execute on their development machine

---

## Step 1: Initial Build Attempt

### Commands

```bash
cd /path/to/zenith-daw

# Clean build
rm -rf build

# Configure
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build (capture all output)
cmake --build build -j$(nproc) 2>&1 | tee build_errors.log
```

### Expected Result

**It will fail with errors.** This is normal.

### What To Do Next

**Copy the errors and show them to me.**

Specifically:
1. Look at `build_errors.log`
2. Find the first 10-20 error messages
3. Paste them here

Example:
```
error: no matching function for call to 'downloadToFile'
error: 'downloadThread' was not declared in this scope
error: invalid use of incomplete type 'struct DownloadTask'
```

---

## Step 2: Fix Errors Iteratively

### Process

**For each batch of errors:**

1. **You show me the errors**
2. **I suggest fixes**
3. **You apply the fixes**
4. **You rebuild**
5. **Repeat until clean build**

### Example Interaction

**You paste:**
```
/home/user/zenith/ModelManager.cpp:86:15: error: no member named 'url' in 'ModelManager::DownloadTask'
```

**I respond with:**
"The issue is that the DownloadTask struct is defined inside DownloadThread class but you're trying to access it from ModelManager. Here's the fix..."

---

## Step 3: First Successful Build

Once compilation succeeds, you need to verify:

```bash
# Verify the executable exists
ls -lh build/Zenith\ DAW*

# Try to run it (may crash, that's OK)
./build/Zenith\ DAW
```

### Expected Issues

**Likely runtime errors:**
- Segmentation faults
- Missing model files
- Plugin scanning issues

**Capture these:**
```bash
# Run with debugger
gdb ./build/Zenith\ DAW

# Or get backtrace
./build/Zenith\ DAW 2>&1 | tee runtime.log
```

---

## Step 4: Testing on Real Systems

### Test Matrix

| Platform | Tests | Commands |
|----------|-------|----------|
| **Linux** | Load plugins, export audio | `valgrind --leak-check=full ./build/Zenith\ DAW` |
| **macOS** | Load plugins, verify codesign | `Instruments - Time Profiler` |
| **Windows** | Load plugins, test ASIO | `Visual Studio Profiler` |

### Stem Separation Test

```cpp
// In your app, test this:
auto modelManager = std::make_unique<ModelManager>();
modelManager->downloadModel("htdemucs",
    [](int64_t downloaded, int64_t total) {
        std::cout << "Downloaded: " << downloaded << "/" << total << std::endl;
    },
    [](bool success, juce::String message) {
        if (success) {
            std::cout << "Model downloaded!" << std::endl;
        } else {
            std::cout << "Failed: " << message << std::endl;
        }
    }
);
```

### VST3 Scanner Test

```cpp
// Test with a known good plugin
auto scanner = std::make_unique<SafePluginScanner>();

scanner->startScanning(directories,
    [](int current, int total, const juce::String& path) {
        std::cout << "Scanning: " << current << "/" << total << " - " << path << std::endl;
    },
    [](const juce::Array<ScanResult>& results) {
        for (auto& result : results) {
            std::cout << "Plugin: " << result.description.name
                      << " - " << (result.success ? "OK" : result.errorMessage) << std::endl;
        }
    }
);
```

---

## Step 5: Performance Profiling

### Linux

```bash
# CPU profiling
perf record -g ./build/Zenith\ DAW
perf report

# Memory profiling
valgrind --tool=massif ./build/Zenith\ DAW
ms_print massif.out.* massif.txt

# ThreadSanitizer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build
./build/ZenithDAWTests_artifacts/Debug/ZenithDAWTests
```

### macOS

```bash
# Time Profiler
instruments -t "Time Profiler" -D ./build/Zenith\ DAW

# Allocations
instruments -t "Allocations" -D ./build/Zenith\ DAW

# Leaks
leaks --atExit -- ./build/Zenith\ DAW
```

### Windows

```bash
# Use Visual Studio Profiler
# Or: VSPerfCmd.exe /callstack

# Memory
drmemory -- ./build/Zenith\ DAW
```

---

## Step 6: Common Issues & Solutions

### Issue: "undefined reference to `vtable`"

**Cause:** Virtual functions not implemented

**Fix:** Implement all virtual functions in the class

### Issue: "undefined reference to `juce::URL::downloadToFile`"

**Cause:** JUCE API change or missing include

**Fix:** Check JUCE version, use correct API signature

### Issue: Segmentation fault on startup

**Cause:** Uninitialized members, null pointers

**Fix:** Run in debugger:
```bash
gdb ./build/Zenith\ DAW
(gdb) run
(gdb) bt  # backtrace
```

### Issue: Plugin scanner crashes

**Cause:** Bad plugin, timeout not working

**Fix:** Test with safe plugins first, blacklist problematic ones

---

## What I Need From You

To continue helping, I need:

1. **Build errors** - Paste the actual compilation errors
2. **Runtime errors** - Paste backtraces or crash logs
3. **Platform info** - Tell me which OS you're on
4. **Compiler version** - `cmake --version`, `gcc --version`, etc.

### What I Can Do Once You Provide This

1. Fix specific compilation errors
2. Suggest solutions for runtime crashes
3. Suggest performance optimizations
4. Debug platform-specific issues

---

## Realistic Timeline (with your help)

### Week 1: Compilation
- Day 1-2: You build, show me errors
- Day 3-5: I suggest fixes, you apply them, you rebuild

### Week 2: Basic Testing
- Day 1-2: First successful build
- Day 3-5: Test basic functionality, report crashes

### Week 3-4: Platform Testing
- Test on all 3 platforms
- Fix platform-specific issues

### Week 5-6: Performance
- Profile performance
- Optimize hot paths
- Fix memory leaks

---

## Right Now - Your Action Items

1. **Build the code:**
   ```bash
   cd /home/micah/Desktop/sylorlabs\ projects/zenith-daw
   cmake --build build 2>&1 | tee build_errors.log
   ```

2. **Show me the first 20 error lines** from build_errors.log

3. **Tell me your platform** (Linux/macOS/Windows, version)

Once you do this, I can actually help fix real problems instead of guessing.

---

*This guide requires YOUR participation. I cannot do this alone because I cannot access your systems or run your code.*
