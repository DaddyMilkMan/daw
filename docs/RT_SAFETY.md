# Real-Time Safety Guidelines for Zenith DAW

## Overview

Real-time audio processing requires deterministic, bounded execution time. Any unbounded operation can cause audio dropouts, glitches, or clicks. This document defines what is allowed and forbidden on the audio thread.

---

## The Golden Rule

**On the audio thread: Never do anything that might take an unknown amount of time.**

---

## Audio Thread Entry Points

These functions run on the **REAL-TIME AUDIO THREAD** and must follow strict safety rules:

### Engine (zenith-core/src/Engine.cpp)
- `Engine::audioDeviceAboutToStart()` - Called from audio thread before streaming
- `Engine::audioDeviceIOCallbackWithContext()` - Main audio callback
- `Engine::processAudio()` - Audio processing helper

### Track (zenith-core/src/Track.cpp)
- `Track::processAudioBlock()` - Per-track audio processing
- `Track::Clip::getNextAudioBlock()` - Per-clip audio rendering

All code paths called from these functions **must be RT-safe**.

---

## ❌ FORBIDDEN on Audio Thread

### 1. Memory Allocation

**Never allocate on the heap:**
- `new` / `delete`
- `malloc()` / `free()`
- `std::vector::push_back()` (when capacity would grow)
- `std::vector::emplace_back()` (when capacity would grow)
- `std::string` construction
- `juce::String` construction
- `std::function` creation
- `std::shared_ptr` / `std::unique_ptr` construction

**Why:** Memory allocation can trigger system calls, page faults, or unbounded wait times for the allocator lock.

**Fix:** Pre-allocate all buffers in `audioDeviceAboutToStart()` or constructors. Use fixed-size containers with `reserve()`.

### 2. Logging and String Operations

**Never log or build strings:**
- `DBG()` - Allocates strings
- `juce::Logger::writeToLog()` - I/O and allocation
- `std::cout` / `printf` - I/O operations
- `juce::String` operations with `+`, `<<` - Allocation
- String formatting with `juce::String(number)` - Allocation

**Why:** String operations allocate memory and DBG/logging perform I/O which blocks.

**Fix:**
- Use `jassert()` for debug builds (compiled out in release)
- Set atomic flags and poll from message thread
- Use lock-free ring buffer for debugging (advanced)

### 3. Locking / Blocking

**Never use blocking synchronization:**
- `std::mutex::lock()` - Can block indefinitely
- `std::lock_guard` / `std::unique_lock` - Same as above
- `std::condition_variable` - Blocks by design
- File I/O - Unbounded wait time
- Network I/O - Unpredictable latency

**Why:** Any blocking operation violates real-time guarantees.

**Fix:**
- Use `std::atomic` for single values
- Use `juce::AbstractFifo` for lock-free queues
- Use `juce::SpinLock` (last resort, use carefully)
- Move I/O to message thread

### 4. UI Calls

**Never touch UI from audio thread:**
- `Component::repaint()`
- `Component::addAndMakeVisible()`
- Any `juce::Component` methods
- `juce::MessageManager::callAsync()`

**Why:** UI operations allocate, lock, and call into windowing systems with unpredictable timing.

**Fix:** Set atomic flags, use `AsyncUpdater`, or post messages to the message thread.

### 5. System Calls

**Never make system calls:**
- File operations (`fopen`, `read`, `write`)
- Time queries (except fast clocks like `rdtsc`)
- Process/thread management
- Memory mapping

**Why:** System calls trap to kernel mode with unbounded execution time.

**Fix:** Do this work on message thread and pass results via atomics.

---

## ✅ ALLOWED on Audio Thread

### 1. Audio Processing

- Reading/writing samples from/to audio buffers
- DSP math operations (add, multiply, sin, cos, etc.)
- `juce::FloatVectorOperations::clear()`
- `juce::FloatVectorOperations::add()`
- `AudioBuffer::applyGain()`
- Fixed-point arithmetic

### 2. Lock-Free Data Structures

- `std::atomic<T>` with built-in numeric types
  - `.load()` / `.store()` operations
  - `.fetch_add()` / `.fetch_sub()`
- `juce::AbstractFifo` - Lock-free FIFO queue
- Pre-allocated fixed-size arrays
- Pre-reserved `std::vector` (no growth!)

### 3. Pre-Allocated Buffers

- Reading from `AudioFormatReader` (if file is cached)
- Writing to pre-allocated `AudioBuffer`
- Accessing fixed-size lookup tables
- Using pre-computed wave tables

### 4. Control Flow

- Conditionals (`if`, `switch`)
- Fixed-iteration loops (avoid unbounded loops)
- Function calls to RT-safe functions
- Early returns

---

## Common Patterns

### Pattern 1: Pre-Allocate in Prepare

```cpp
// In audioDeviceAboutToStart() or prepareToPlay()
void Engine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    // Pre-allocate buffers (MESSAGE THREAD - allocation OK here)
    tempBuffer.setSize(2, 4096);  // Max channels, max buffer size

    // Reserve fixed capacity for containers
    clipQueue.reserve(128);  // Max clips

    // RT-safe operations only after this point
}

// In audio callback (AUDIO THREAD)
void Engine::processAudio(...)
{
    // Use pre-allocated buffers - NO allocation here!
    tempBuffer.clear();
    // Process...
}
```

### Pattern 2: Atomic Flags for State

```cpp
// Shared state
std::atomic<bool> isPlaying_{false};
std::atomic<float> masterVolume_{0.8f};

// Message thread writes
void Engine::play()
{
    isPlaying_.store(true);  // RT-safe write
}

// Audio thread reads
void Engine::processAudio(...)
{
    bool playing = isPlaying_.load();  // RT-safe read
    if (playing) {
        // Process audio...
    }
}
```

### Pattern 3: Lock-Free Queue for Events

```cpp
// Message thread → Audio thread
juce::AbstractFifo midiEvents{128};  // Fixed size
std::array<MidiEvent, 128> midiBuffer;

// Message thread writes
void addMidiEvent(const MidiEvent& event)
{
    int start1, size1, start2, size2;
    midiEvents.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0)
    {
        midiBuffer[start1] = event;
        midiEvents.finishedWrite(1);
    }
}

// Audio thread reads (RT-safe!)
void processAudio(...)
{
    int start1, size1, start2, size2;
    midiEvents.prepareToRead(available, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i)
        processMidi(midiBuffer[start1 + i]);

    midiEvents.finishedRead(size1);
}
```

### Pattern 4: No Logging - Use Asserts

```cpp
// ❌ WRONG - Logs on audio thread
void Track::Clip::getNextAudioBlock(...)
{
    if (reader == nullptr)
    {
        DBG("Clip reader is null!");  // RT VIOLATION!
        return;
    }
}

// ✅ RIGHT - Assert in debug, silent in release
void Track::Clip::getNextAudioBlock(...)
{
    jassert(reader != nullptr);  // Debug only, no cost in release
    if (reader == nullptr)
        return;
}
```

---

## Verification Tools

### JUCE Built-In

Use `JUCE_CHECK_MEMORY_LEAKS` and run with sanitizers:

```bash
# Address Sanitizer (detects allocations)
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address" ..

# Thread Sanitizer (detects data races)
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..
```

### Community Tools

- **AudioThreadGuard** - JUCE forum tool that asserts on audio thread allocations
- **pluginval** - Validates plugins for RT safety violations

### Manual Testing

Run your DAW with:
- Very small buffer sizes (32-64 samples)
- High CPU load
- Watch for glitches, clicks, dropouts

If audio glitches with small buffers, you likely have RT violations.

---

## Common Mistakes

### Mistake 1: "Just One Allocation Won't Hurt"

```cpp
// ❌ WRONG - "Small" allocation still breaks RT guarantee
void processAudio(...)
{
    auto tempArray = std::make_unique<float[]>(16);  // RT VIOLATION!
    // Process...
}
```

**Fix:** Pre-allocate `std::array<float, 16>` as member variable.

### Mistake 2: Hidden Allocations in Standard Library

```cpp
// ❌ WRONG - std::function allocates for captures
void processAudio(...)
{
    auto processor = [volume](float sample) {  // RT VIOLATION!
        return sample * volume;
    };
}
```

**Fix:** Use plain function pointers or inline lambdas without captures.

### Mistake 3: "Logging is Just for Debug"

```cpp
// ❌ WRONG - DBG still allocates in release builds!
void processAudio(...)
{
    #ifdef DEBUG
        DBG("Processing audio");  // Still RT VIOLATION in debug!
    #endif
}
```

**Fix:** Use `jassert()` which compiles to nothing in release, or don't log at all.

---

## Threading Model

```
┌─────────────────────────────────────┐
│         MESSAGE THREAD              │
│  - UI updates                       │
│  - File I/O                         │
│  - Allocations OK                   │
│  - Logging OK                       │
│  - Calls: play(), stop(), load()    │
└────────────┬────────────────────────┘
             │
             │ (Atomic variables)
             │
┌────────────▼────────────────────────┐
│       AUDIO THREAD                  │
│  - Real-time processing             │
│  - NO allocations                   │
│  - NO logging                       │
│  - NO locks (except SpinLock)       │
│  - Calls: processAudio()            │
└─────────────────────────────────────┘
```

---

## References

### JUCE Resources
- [JUCE Audio Thread Safety](https://docs.juce.com/master/classAudioIODeviceCallback.html)
- [AudioThreadGuard - JUCE Forum](https://forum.juce.com/t/audiothreadguard-keep-your-audio-thread-clean/28532)
- [Real-Time Safety Checking in pluginval](https://forum.juce.com/t/pluginval-real-time-safety-checking/67439)

### External Resources
- [Using Locks in Real-Time Audio Processing Safely](https://timur.audio/using-locks-in-real-time-audio-processing-safely)
- [Real-Time Programming Best Practices](https://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing)

---

## Summary

### On the audio thread:

**NEVER:**
- Allocate (`new`, `malloc`, `String`)
- Log (`DBG`, `std::cout`)
- Lock (`std::mutex`)
- Do I/O (files, network)
- Call UI

**ALWAYS:**
- Use pre-allocated buffers
- Use `std::atomic` for state
- Use `jassert` for debug checks
- Keep execution bounded and deterministic
- Document RT functions with warning comments

**Remember:** One RT violation can cause glitches for all your users. When in doubt, move it to the message thread.
