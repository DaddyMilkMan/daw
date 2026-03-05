# Real-Time Safety Guidelines for Zenith DAW

## Overview

This document outlines the real-time (RT) safety requirements and best practices for audio processing code in Zenith DAW. Violating these rules can cause audio glitches, dropouts, crackling, or system instability.

## Core RT-Safety Rules

### 1. NO Memory Allocations

**Never allocate or deallocate memory in the audio thread.**

❌ **Forbidden in RT context:**
- `new`, `delete`, `malloc`, `free`
- `std::make_unique`, `std::make_shared`
- Container resizing: `std::vector::push_back`, `std::vector::resize`
- String operations: `std::string` concatenation, `juce::String` operations
- `juce::AudioBuffer` construction (allocates memory)

✅ **Allowed in RT context:**
- Pre-allocated fixed-size arrays: `std::array<T, N>`
- Stack allocation of POD types
- Working with pre-allocated buffers
- Atomic operations on fixed-size data

**Solution:** Pre-allocate all buffers and data structures in `prepareToPlay()` or the constructor.

```cpp
// ❌ BAD: Allocates in RT context
void processBlock(juce::AudioBuffer<float>& buffer) {
    juce::AudioBuffer<float> tempBuffer(2, 512);  // ALLOCATES!
    std::vector<float> samples;
    samples.push_back(1.0f);  // ALLOCATES!
}

// ✅ GOOD: Pre-allocated
class MyProcessor {
    std::array<float, 512> preallocatedBuffer;  // Fixed size
    
    void processBlock(juce::AudioBuffer<float>& buffer) {
        // Use preallocatedBuffer - no allocation
    }
};
```

### 2. NO Blocking Operations

**Never block or wait in the audio thread.**

❌ **Forbidden in RT context:**
- `std::mutex`, `std::lock_guard`, `std::unique_lock`
- `juce::CriticalSection`, `juce::ScopedLock`
- `std::condition_variable::wait()`
- `std::this_thread::sleep_for()`
- File I/O operations
- Network I/O operations
- Database queries

✅ **Allowed in RT context:**
- `std::atomic` operations
- Lock-free data structures
- `juce::AbstractFifo` (lock-free FIFO)
- Spinlocks (only for very short critical sections, < 100 CPU cycles)

**Solution:** Use lock-free data structures or atomic operations for thread communication.

```cpp
// ❌ BAD: Locks in RT context
std::mutex mutex;
void processBlock(juce::AudioBuffer<float>& buffer) {
    std::lock_guard<std::mutex> lock(mutex);  // BLOCKS!
    // process audio...
}

// ✅ GOOD: Lock-free atomic
std::atomic<float> gain{1.0f};
void processBlock(juce::AudioBuffer<float>& buffer) {
    float currentGain = gain.load(std::memory_order_relaxed);
    buffer.applyGain(currentGain);
}
```

### 3. NO System Calls

**Avoid system calls and OS interactions.**

❌ **Forbidden in RT context:**
- `std::cout`, `printf`, `fprintf`
- `juce::Logger::writeToLog()`
- `juce::Time::getCurrentTime()` (may allocate or syscall)
- File operations: `open()`, `read()`, `write()`
- `assert()` (may trigger I/O on failure)

✅ **Allowed in RT context:**
- Atomic counters for profiling
- Pre-computed lookup tables
- Simple arithmetic operations

**Solution:** Use atomic counters or defer logging to a background thread.

```cpp
// ❌ BAD: System call in RT context
void processBlock(juce::AudioBuffer<float>& buffer) {
    auto timestamp = juce::Time::getCurrentTime();  // MAY SYSCALL!
    std::cout << "Processing at " << timestamp << std::endl;  // I/O!
}

// ✅ GOOD: Use atomic counter
static std::atomic<uint64_t> processCounter{0};
void processBlock(juce::AudioBuffer<float>& buffer) {
    uint64_t count = processCounter.fetch_add(1, std::memory_order_relaxed);
    // Use count for internal tracking (no I/O)
}
```

### 4. NO Dynamic Dispatch (RTTI)

**Avoid runtime type information in critical paths.**

❌ **Forbidden in RT context:**
- `dynamic_cast<T*>` (RTTI overhead)
- Virtual function calls in tight loops (cache-unfriendly)
- Exception handling with `throw`/`catch`

✅ **Allowed in RT context:**
- Static polymorphism (templates)
- `static_cast<T*>` (when type is guaranteed)
- Virtual functions (acceptable for top-level callbacks like `processBlock`)

**Solution:** Use templates or ensure type is known at compile time.

```cpp
// ❌ BAD: Dynamic cast in RT context
void processBlock(juce::AudioBuffer<float>& buffer) {
    for (auto* effect : effects) {
        if (auto* reverb = dynamic_cast<Reverb*>(effect)) {  // SLOW!
            reverb->process(buffer);
        }
    }
}

// ✅ GOOD: Static polymorphism or direct calls
void processBlock(juce::AudioBuffer<float>& buffer) {
    reverb.process(buffer);  // Direct call, type known
}
```

### 5. Pre-Allocation Strategy

**All resources must be allocated before RT processing begins.**

✅ **Required:**
- Allocate in constructor or `prepareToPlay()`
- Reserve capacity for all containers: `std::vector::reserve()`
- Pre-compute lookup tables
- Pre-allocate temporary buffers

```cpp
class RTSafeProcessor : public juce::AudioProcessor {
    std::vector<float> delayBuffer;
    std::array<float, 1024> lookupTable;
    
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override {
        // Pre-allocate all buffers
        delayBuffer.resize(static_cast<size_t>(sampleRate * 2.0));  // 2 seconds
        
        // Pre-compute lookup table
        for (size_t i = 0; i < lookupTable.size(); ++i) {
            lookupTable[i] = std::sin(2.0 * M_PI * i / lookupTable.size());
        }
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer, 
                     juce::MidiBuffer& midi) override {
        // All buffers are pre-allocated, safe to use
        // NO allocations happen here
    }
};
```

## Lock-Free Patterns

### Single-Producer/Single-Consumer Queue

Use `juce::AbstractFifo` or our custom `LockFreeCircularBuffer`:

```cpp
// Producer (RT thread)
LockFreeCircularBuffer<AudioAnalysisData, 64> fifo;

void processBlock(juce::AudioBuffer<float>& buffer) {
    AudioAnalysisData data;
    data.peakLevel = buffer.getMagnitude(0, buffer.getNumSamples());
    fifo.push(data);  // Lock-free, RT-safe
}

// Consumer (background thread)
void backgroundThread() {
    AudioAnalysisData data;
    while (fifo.pop(data)) {
        // Process data (can allocate, block, etc.)
        updateUI(data);
    }
}
```

### Atomic Parameters

Use `std::atomic` for simple value passing:

```cpp
class RTSafeEffect {
    std::atomic<float> wetDry{0.5f};
    
    // UI thread
    void setWetDry(float value) {
        wetDry.store(value, std::memory_order_relaxed);
    }
    
    // RT thread
    void processBlock(juce::AudioBuffer<float>& buffer) {
        float mix = wetDry.load(std::memory_order_relaxed);
        // Use mix...
    }
};
```

### Seqlock Pattern

For larger data structures (32-64 bytes), use seqlock:

```cpp
struct LargeData {
    float params[16];
    // ... more data
};

std::atomic<uint32_t> sequence{0};
LargeData data;

// Writer (non-RT thread)
void updateData(const LargeData& newData) {
    sequence.fetch_add(1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    data = newData;
    std::atomic_thread_fence(std::memory_order_release);
    sequence.fetch_add(1, std::memory_order_relaxed);
}

// Reader (RT thread)
LargeData readData() {
    LargeData localCopy;
    uint32_t s1, s2;
    do {
        s1 = sequence.load(std::memory_order_acquire);
        localCopy = data;
        s2 = sequence.load(std::memory_order_acquire);
    } while ((s1 & 1) != 0 || s1 != s2);
    return localCopy;
}
```

## Memory Ordering Guide

### When to Use Each Memory Order

- **`memory_order_relaxed`**: Single-threaded variable or no ordering needed
  - Reading/writing atomic counters
  - Status flags where exact timing doesn't matter
  
- **`memory_order_acquire`**: Reading shared data (pairs with release)
  - Reading from lock-free queue
  - Loading configuration
  
- **`memory_order_release`**: Writing shared data (pairs with acquire)
  - Writing to lock-free queue
  - Publishing new data
  
- **`memory_order_acq_rel`**: Read-modify-write operations
  - `fetch_add`, `compare_exchange_weak`
  
- **`memory_order_seq_cst`**: Default, strongest guarantee (AVOID in RT code - too slow)

```cpp
// Example: Lock-free queue with proper ordering
bool push(const T& item) {
    size_t currentWrite = writePos.load(std::memory_order_relaxed);
    size_t nextWrite = (currentWrite + 1) % Size;
    
    if (nextWrite == readPos.load(std::memory_order_acquire)) {  // Sync with consumer
        return false;
    }
    
    buffer[currentWrite] = item;
    writePos.store(nextWrite, std::memory_order_release);  // Publish to consumer
    return true;
}
```

## Function Annotations

Zenith DAW provides a formal macro system (`include/zenith/RTSafety.h`) to
annotate function boundaries.  Prefer these macros over freeform comments.

### Macro Quick Reference

| Macro | Meaning |
|-------|---------|
| `ZENITH_RT_THREAD` | Top-level audio callback entry point (strongest RT annotation) |
| `ZENITH_RT_SAFE` | May be called from the audio thread safely |
| `ZENITH_NONRT_THREAD` | Message-thread / UI entry point; must NOT be called from audio thread |
| `ZENITH_NONRT_SAFE` | May allocate / block; must not be called from audio thread |
| `ZENITH_RT_REGION_BEGIN` / `ZENITH_RT_REGION_END` | Inline region markers |
| `ZENITH_ASSERT_RT_THREAD()` | Debug assert: we ARE on the audio thread |
| `ZENITH_ASSERT_NONRT_THREAD()` | Debug assert: we are NOT on the audio thread |
| `ZENITH_WARN_IF_RT_THREAD()` | Non-fatal debug warning if called from audio thread |
| `ZENITH_RT_ASSERT(cond)` | Assert a condition in RT code (debug only) |

### Usage Examples

```cpp
#include <zenith/RTSafety.h>

// Mark the audio callback entry point
ZENITH_RT_THREAD
void audioDeviceIOCallbackWithContext(...) noexcept override {
    ZENITH_ASSERT_RT_THREAD();
    processAudioBlock(...);
}

// Mark an RT-safe helper
ZENITH_RT_SAFE
void processAudioBlock(...) noexcept {
    // No allocation, no locking, no system calls
}

// Mark a non-RT function
ZENITH_NONRT_THREAD
void loadPreset(const juce::File& file) {
    ZENITH_ASSERT_NONRT_THREAD();
    // Safe to allocate / block here
}
```

### Runtime Initialization

Call `zenith::rt::markAsAudioThread()` from `audioDeviceAboutToStart()` to
register the audio callback thread for debug assertions:

```cpp
void audioDeviceAboutToStart(juce::AudioIODevice*) override {
    zenith::rt::markAsAudioThread();
}

void audioDeviceStopped() override {
    zenith::rt::unmarkAudioThread();
}
```

## Lock-Free Command Queue (RT Boundary API)

The **only sanctioned mechanism** for passing structured data across the RT
boundary is one of the queues in `include/zenith/LockFreeCommandQueue.h`.

### Choosing the Right Queue

| Queue | Use when |
|-------|----------|
| `zenith::SPSCCommandQueue<T, N>` | Exactly one non-RT writer + one RT reader |
| `zenith::MPSCCommandQueue<T, N>` | Multiple non-RT writers (UI + AI + services) + one RT reader |

### API Contract

```
Producer side (non-RT)          Consumer side (RT audio thread)
──────────────────────          ───────────────────────────────
push(item)  → bool              pop(item)  → bool
Never blocks                    Never blocks
Any non-RT thread               Audio callback only
```

### Example — UI parameter change → audio thread

```cpp
#include <zenith/LockFreeCommandQueue.h>

struct ParamChange { int paramId; float value; };

// Shared between UI and audio thread
zenith::SPSCCommandQueue<ParamChange, 512> paramQueue;

// UI thread
void sliderMoved(int id, float v) {
    paramQueue.push({ id, v });   // lock-free, non-blocking
}

// Audio callback (RT)
void processBlock(...) {
    ParamChange cmd;
    while (paramQueue.pop(cmd)) {
        applyParam(cmd.paramId, cmd.value);
    }
    // render audio ...
}
```

### Example — Multiple producers (AI + UI) → RT

```cpp
zenith::MPSCCommandQueue<EngineCommand, 256> cmdQueue;

// AI service thread
void aiAgent() { cmdQueue.push({ CMD_LOAD_PRESET, presetId }); }

// UI thread
void onButtonClick() { cmdQueue.push({ CMD_PLAY }); }

// Audio callback (RT)
void processBlock(...) {
    EngineCommand cmd;
    while (cmdQueue.pop(cmd)) { dispatch(cmd); }
    // render audio ...
}
```

## Static Analysis — Forbidden Construct Check

A shell script (`tools/lint/check_rt_forbidden.sh`) scans any C++ file that
contains RT annotations for forbidden constructs such as heap allocations,
mutex locks, and I/O calls.

### Running the check locally

```bash
# Check the default source paths
tools/lint/check_rt_forbidden.sh

# Check specific files
tools/lint/check_rt_forbidden.sh modules/zenith_core/engine/Engine.h

# Warn only (don't fail), useful for gradual adoption
tools/lint/check_rt_forbidden.sh --warn-only
```

### CI integration

The check runs automatically via `.github/workflows/rt-safety-check.yml` on
push/PR to `main` and `develop`.  Violations are reported as GitHub warnings
and uploaded as the `rt-safety-report` artifact.

## Testing RT-Safety

### Static Analysis
- Use compiler warnings: `-Wall -Wextra`
- Enable JUCE assertions: `juce::ScopedNoDenormals`
- Use address sanitizer (ASan) and thread sanitizer (TSan)

### Runtime Detection
- Profile with real-time monitoring tools
- Use `juce::PerformanceCounter` in debug builds
- Test under high system load

### Tools
- **Valgrind** (Linux): Detect memory issues
- **Instruments** (macOS): Time Profiler and Allocations
- **Visual Studio Profiler** (Windows): Performance analysis

## Common Pitfalls

1. **Hidden Allocations**
   - `juce::String` operations
   - Lambda captures that allocate
   - Exception handling

2. **Unexpected Blocking**
   - Reference-counted objects (may lock on copy)
   - Shared pointers in multi-threaded context
   - Global variables with initialization

3. **Cache Thrashing**
   - False sharing (atomic variables on same cache line)
   - Excessive atomic operations
   - Large data structures

## Summary Checklist

Before marking code as RT-safe, verify:

- [ ] No `new`, `delete`, `malloc`, `free`
- [ ] No container resizing (use `std::array` or pre-allocated `std::vector`)
- [ ] No string operations (`std::string`, `juce::String`)
- [ ] No mutex locks (`std::mutex`, `juce::CriticalSection`)
- [ ] No system calls (`std::cout`, `juce::Time::getCurrentTime()`)
- [ ] No file or network I/O
- [ ] No `dynamic_cast` or RTTI
- [ ] All buffers pre-allocated in `prepareToPlay()`
- [ ] Proper atomic memory ordering
- [ ] Code paths tested under load

## References

- [JUCE Real-Time Safety](https://docs.juce.com/master/tutorial_audio_processor_value_tree_state.html)
- [Lock-Free Programming](https://www.1024cores.net/home/lock-free-algorithms)
- [C++ Memory Model](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [Real-Time Audio Programming 101](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing)
