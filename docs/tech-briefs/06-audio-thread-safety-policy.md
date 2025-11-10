# Audio Thread Safety Policy & Lock-Free Patterns

**Real-Time Rules and Implementation Patterns for Zenith DAW**
**Version:** 1.0
**Date:** 2025-11-10

---

## Executive Summary

This document establishes **hard rules** for the audio callback and provides **lock-free implementation patterns** for common DAW operations. All engineers working on Zenith DAW must follow these rules to prevent audio glitches, dropouts, and crashes.

**Golden Rule:** The audio callback must complete within **2-3ms** (at 128 samples @ 48kHz). Any operation that blocks, allocates, or calls the OS will **fail this deadline**.

---

## The Audio Thread: Hard Rules

### Official Sources

> "You cannot do anything on the audio thread that might block the thread or otherwise take an unknown amount of time, such as allocating memory, performing any system call, or doing any I/O."
> — [JUCE Forum: Real-Time Thread Discussion](https://forum.juce.com/t/real-time-thread-in-juce/43361)

> "A common default setting is a buffer size of 128 samples at a sample rate of 44,100 Hz, which translates to 2.9 ms in between callbacks. If your process does not compute its audio output and write it into the provided buffer before this deadline, you will get an audible glitch."
> — [timur.audio: Using Locks in Real-Time Audio Processing](https://timur.audio/using-locks-in-real-time-audio-processing-safely)

> "You should not use `std::mutex` on the audio thread, not even with `try_lock`. This is because if there is another thread waiting to acquire the mutex, the audio thread will have to interact with the OS thread scheduler at that point so that that other thread can then be woken up. And that's a system call. It's not realtime-safe to do that."
> — [timur.audio: Using Locks in Real-Time Audio Processing](https://timur.audio/using-locks-in-real-time-audio-processing-safely)

---

## NEVER on the Audio Thread

### Memory Allocation

❌ **DO NOT:**
```cpp
// AUDIO THREAD - WRONG!
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    auto* newBuffer = new float[1024];              // ❌ ALLOCATION
    std::vector<float> tempData;
    tempData.push_back(value);                       // ❌ DYNAMIC ALLOCATION
    String message = "Processing...";                // ❌ HEAP ALLOCATION
}
```

✅ **DO THIS INSTEAD:**
```cpp
// Pre-allocate in constructor or prepareToPlay()
class MyProcessor
{
    std::array<float, 1024> tempBuffer;  // ✅ Fixed-size, stack allocation
    AudioBuffer<float> scratchBuffer;    // ✅ Pre-allocated

    void prepareToPlay(double sampleRate, int maxBlockSize)
    {
        scratchBuffer.setSize(2, maxBlockSize); // ✅ Allocate here
    }

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
    {
        // Use pre-allocated buffers
        scratchBuffer.clear();
        // ... process ...
    }
};
```

### Mutex Locks

❌ **DO NOT:**
```cpp
// AUDIO THREAD - WRONG!
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    std::lock_guard<std::mutex> lock(parameterMutex);  // ❌ LOCK
    float gain = currentGain;                           // ❌ BLOCKS IF GUI OWNS LOCK
}
```

❌ **NOT EVEN `try_lock`:**
```cpp
// AUDIO THREAD - STILL WRONG!
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    if (parameterMutex.try_lock())  // ❌ SYSTEM CALL (wakes other thread)
    {
        // ...
        parameterMutex.unlock();
    }
}
```

✅ **DO THIS INSTEAD:**
```cpp
// Use std::atomic for simple values
std::atomic<float> currentGain { 0.8f };

void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    float gain = currentGain.load(std::memory_order_relaxed);  // ✅ LOCK-FREE
    buffer.applyGain(gain);
}

// GUI thread can safely update
void setGain(float newGain)
{
    currentGain.store(newGain, std::memory_order_relaxed);  // ✅ LOCK-FREE
}
```

### System Calls

❌ **DO NOT:**
```cpp
// AUDIO THREAD - WRONG!
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    DBG("Processing block");                         // ❌ LOGGING (file I/O)
    File::createDirectory(outputDir);                // ❌ FILE I/O
    URL("http://api.com/data").readEntireText();    // ❌ NETWORK
    MessageManager::getInstance()->callAsync(...);   // ❌ SYSTEM CALL
}
```

✅ **DO THIS INSTEAD:**
```cpp
// Use lock-free FIFO to send data to another thread
juce::AbstractFifo fifo { 1024 };
std::array<LogMessage, 1024> logBuffer;

void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    // Push log message to FIFO (lock-free)
    int start1, size1, start2, size2;
    fifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0)
    {
        logBuffer[start1] = { "Processing block", currentSample };
        fifo.finishedWrite(1);
    }
}

// Separate logging thread reads from FIFO and writes to disk
void loggingThreadFunc()
{
    while (!threadShouldExit())
    {
        if (fifo.getNumReady() > 0)
        {
            int start1, size1, start2, size2;
            fifo.prepareToRead(1, start1, size1, start2, size2);

            if (size1 > 0)
            {
                auto& msg = logBuffer[start1];
                std::cout << msg.text << std::endl;  // ✅ SAFE (not audio thread)
                fifo.finishedRead(1);
            }
        }
        Thread::sleep(10);
    }
}
```

---

## SAFE on the Audio Thread

### Fixed-Size Operations

✅ **Safe operations:**
```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    // Math operations
    float gain = std::sin(phase);            // ✅ SAFE
    buffer.applyGain(gain);                  // ✅ SAFE

    // Fixed-size arrays
    std::array<float, 8> coeffs;             // ✅ SAFE (stack allocation)
    float temp[256];                         // ✅ SAFE (stack)

    // Reading atomics
    float param = paramValue.load(std::memory_order_relaxed);  // ✅ SAFE

    // Pre-allocated buffers
    scratchBuffer.clear();                   // ✅ SAFE (no allocation)
    scratchBuffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());  // ✅ SAFE
}
```

---

## Lock-Free Communication Patterns

### Pattern 1: Simple Parameters (`std::atomic`)

**Use Case:** Single float/int values (gain, pan, filter frequency)

```cpp
class AudioProcessor
{
    // Atomic parameter (GUI → Audio)
    std::atomic<float> targetGain { 0.8f };

    // Smoothed value (to avoid clicks)
    float currentGain { 0.8f };

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
    {
        // AUDIO THREAD - Read new target
        float newTarget = targetGain.load(std::memory_order_relaxed);

        // Smooth to avoid clicks (exponential smoothing)
        currentGain += 0.05f * (newTarget - currentGain);

        // Apply
        buffer.applyGain(currentGain);
    }

    // GUI THREAD - Write new value
    void setGain(float gain)
    {
        targetGain.store(gain, std::memory_order_relaxed);
    }
};
```

**Pros:**
- ✅ Fast (no locks, no allocation)
- ✅ Simple (one line to read/write)

**Cons:**
- ❌ Only works for small, trivial types (float, int, bool)
- ❌ Can't pass complex data (strings, arrays)

---

### Pattern 2: Lock-Free FIFO (SPSC Queue)

**Use Case:** Stream of events (MIDI, meter data, log messages)

**SPSC = Single-Producer, Single-Consumer** (one writer, one reader)

```cpp
#include <juce_audio_basics/juce_audio_basics.h>

// Message struct
struct ParameterChange
{
    int parameterIndex;
    float newValue;
};

class AudioProcessor
{
    // SPSC FIFO (lock-free)
    juce::AbstractFifo fifo { 512 };
    std::array<ParameterChange, 512> changeBuffer;

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
    {
        // AUDIO THREAD - Read parameter changes from GUI
        while (fifo.getNumReady() > 0)
        {
            int start1, size1, start2, size2;
            fifo.prepareToRead(1, start1, size1, start2, size2);

            if (size1 > 0)
            {
                auto change = changeBuffer[start1];
                applyParameterChange(change);
                fifo.finishedRead(1);
            }
        }

        // Process audio...
    }

    // GUI THREAD - Write parameter change
    void setParameter(int index, float value)
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            changeBuffer[start1] = { index, value };
            fifo.finishedWrite(1);
        }
    }
};
```

**Pros:**
- ✅ Lock-free (real-time safe)
- ✅ Handles bursts (queue up to 512 changes)
- ✅ Type-safe (any struct/class)

**Cons:**
- ❌ Fixed capacity (can overflow if producer is too fast)
- ❌ Only works for SPSC (one writer, one reader)

**Alternative:** Use `boost::lockfree::spsc_queue` for even better performance.

---

### Pattern 3: Double-Buffering (Atomic Pointer Swap)

**Use Case:** Large data structures (project state, plugin presets, automation curves)

**Idea:** Keep two copies of the data. GUI writes to one, audio reads from the other. Swap atomically when ready.

```cpp
class AudioProcessor
{
    struct ProjectState
    {
        std::vector<float> automationCurve;
        float tempo;
        int timeSignatureNum;
        // ... more data ...
    };

    // Two buffers: one for audio, one for GUI
    ProjectState bufferA;
    ProjectState bufferB;

    // Atomic pointer: which buffer is audio currently reading?
    std::atomic<ProjectState*> audioReadBuffer { &bufferA };

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
    {
        // AUDIO THREAD - Read from current buffer
        auto* state = audioReadBuffer.load(std::memory_order_acquire);

        float tempo = state->tempo;
        // Use state->automationCurve, etc.
    }

    // GUI THREAD - Update state
    void setTempo(float newTempo)
    {
        // Determine which buffer audio is NOT reading
        auto* currentAudioBuffer = audioReadBuffer.load(std::memory_order_acquire);
        auto* guiWriteBuffer = (currentAudioBuffer == &bufferA) ? &bufferB : &bufferA;

        // Write to the GUI buffer (safe, audio isn't touching it)
        guiWriteBuffer->tempo = newTempo;
        guiWriteBuffer->automationCurve = /* updated curve */;

        // Atomically swap (audio now reads new buffer)
        audioReadBuffer.store(guiWriteBuffer, std::memory_order_release);
    }
};
```

**Pros:**
- ✅ Handles large data structures
- ✅ Lock-free atomic swap
- ✅ Audio thread always has consistent state (no torn reads)

**Cons:**
- ❌ Memory overhead (2x the data)
- ❌ GUI changes aren't instant (only take effect at next swap)
- ❌ Requires careful memory management

---

### Pattern 4: JUCE `ValueTree` Listeners (Safe Async Updates)

**Use Case:** Project state changes (add track, remove clip, etc.)

**JUCE Pattern:** `ValueTree` changes happen on the message thread. Audio thread reads a **snapshot**.

```cpp
class ProjectState
{
    juce::ValueTree projectTree { "PROJECT" };

    // Audio thread reads this snapshot (updated periodically)
    std::atomic<float> cachedTempo { 120.0f };

    ProjectState()
    {
        // Listen to ValueTree changes (message thread only!)
        projectTree.addListener(this);
    }

    // MESSAGE THREAD - Called when tempo changes
    void valueTreePropertyChanged(ValueTree& tree, const Identifier& property) override
    {
        if (property == "tempo")
        {
            float newTempo = tree.getProperty("tempo");
            cachedTempo.store(newTempo, std::memory_order_release);
        }
    }

    // AUDIO THREAD - Read cached value
    float getTempo() const
    {
        return cachedTempo.load(std::memory_order_acquire);
    }
};
```

**Pros:**
- ✅ Leverages JUCE's `ValueTree` (with undo/redo)
- ✅ Audio thread only reads atomics (fast, lock-free)
- ✅ Scales to complex state (tracks, clips, automation)

**Cons:**
- ⚠️ Audio sees stale data (until next update)
- ⚠️ Requires caching (manual sync between ValueTree and atomics)

---

## Common DAW Operations: Thread Assignment

| **Operation** | **Thread** | **Pattern** |
|---------------|------------|-------------|
| **Play/Stop button** | Message → Audio | `std::atomic<bool>` |
| **Set gain slider** | Message → Audio | `std::atomic<float>` + smoothing |
| **Load plugin** | Message (blocking) | N/A (blocks UI) |
| **Process audio** | Audio | Pure processing (no I/O) |
| **Update meters** | Audio → Message | Lock-free FIFO |
| **Write automation** | Audio → Message | Lock-free FIFO |
| **Save project** | Message (async) | Background thread + file I/O |
| **Load project** | Message (async) | Background thread + file I/O |
| **Scan plugins** | Background thread | `PluginDirectoryScanner` |
| **Render/Export** | Background thread | Offline rendering (no real-time) |

---

## Example: Complete Parameter System

### ParameterManager.h

```cpp
#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>

class ParameterManager
{
public:
    static constexpr int MAX_PARAMETERS = 128;

    struct Parameter
    {
        std::atomic<float> value { 0.0f };
        float minValue { 0.0f };
        float maxValue { 1.0f };
        juce::String name;
    };

    ParameterManager()
    {
        // Pre-allocate parameters
        for (int i = 0; i < MAX_PARAMETERS; ++i)
            parameters[i].value.store(0.0f, std::memory_order_relaxed);
    }

    // GUI THREAD - Set parameter
    void setParameter(int index, float normalizedValue)
    {
        jassert(index >= 0 && index < MAX_PARAMETERS);
        parameters[index].value.store(normalizedValue, std::memory_order_release);
    }

    // AUDIO THREAD - Read parameter
    float getParameter(int index) const
    {
        jassert(index >= 0 && index < MAX_PARAMETERS);
        return parameters[index].value.load(std::memory_order_acquire);
    }

private:
    std::array<Parameter, MAX_PARAMETERS> parameters;
};
```

### Usage

```cpp
class MyEffect
{
    ParameterManager params;

    enum Params { GAIN, PAN, FILTER_FREQ, COUNT };

    MyEffect()
    {
        params.setParameter(GAIN, 0.8f);
    }

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
    {
        // AUDIO THREAD - Read parameter
        float gain = params.getParameter(GAIN);
        buffer.applyGain(gain);
    }

    // GUI THREAD - User moves slider
    void sliderValueChanged(Slider* slider)
    {
        params.setParameter(GAIN, slider->getValue());
    }
};
```

---

## Testing Real-Time Safety

### 1. Enable Debug Checks

```cpp
// In your JUCE app, add this to prepareToPlay():
void MyProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    #if JUCE_DEBUG
    // Enable memory leak detection
    JUCE_CHECK_MEMORY_LEAKS = true;

    // Warn on allocations in audio thread (macOS/Linux)
    // This will crash if you allocate in processBlock()
    #endif
}
```

### 2. Use Thread Sanitizer (TSan)

**On Linux/macOS:**
```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..
make
./ZenithDAW
```

**TSan will detect:**
- Data races (non-atomic reads/writes from multiple threads)
- Lock inversions
- Use-after-free

### 3. Use `std::atomic_thread_fence` for Debugging

```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    #if JUCE_DEBUG
    std::atomic_thread_fence(std::memory_order_seq_cst);
    // Any improper memory access will be caught here
    #endif

    // Process...
}
```

---

## Sources

1. JUCE Forum: Real-Time Thread Discussion: https://forum.juce.com/t/real-time-thread-in-juce/43361
2. timur.audio: Using Locks in Real-Time Audio Processing: https://timur.audio/using-locks-in-real-time-audio-processing-safely
3. JUCE Forum: Real-Time Multi-Threading: https://forum.juce.com/t/real-time-multi-threading-in-an-audio-application/44268
4. JUCE Forum: AudioParameter Thread Safety: https://forum.juce.com/t/audioparameter-thread-safety/21097
5. JUCE API: juce::AbstractFifo: https://docs.juce.com/master/classAbstractFifo.html
6. Boost Lockfree: https://www.boost.org/doc/libs/1_83_0/doc/html/lockfree.html

---

**Document Version:** 1.0
**Last Updated:** 2025-11-10
**Policy:** Pin this document in the repo. All engineers must read before touching audio code.
