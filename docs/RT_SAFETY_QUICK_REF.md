# Real-Time Safety Quick Reference

**Print this out and keep it near your desk when writing audio code!**

## ❌ FORBIDDEN in Audio Thread (processBlock/getNextAudioBlock)

### Memory Allocation
- `new`, `delete`, `malloc`, `free`
- `std::vector::push_back()`, `resize()`
- `std::string` operations
- `juce::String` operations
- `std::make_unique()`, `std::make_shared()`
- `juce::AudioBuffer` constructor

### Blocking Operations
- `std::mutex`, `std::lock_guard`, `std::unique_lock`
- `juce::CriticalSection`, `juce::ScopedLock`
- `std::condition_variable::wait()`
- `sleep()`, `std::this_thread::sleep_for()`
- File I/O (`open`, `read`, `write`, `fstream`)
- Network I/O (sockets, HTTP)

### System Calls
- `std::cout`, `printf`, `fprintf`
- `juce::Logger::writeToLog()`
- `juce::Time::getCurrentTime()`
- `assert()` with side effects

### Expensive Operations
- `dynamic_cast<T*>` (RTTI)
- `throw` / `catch` exceptions
- Virtual function calls in tight loops

## ✅ ALLOWED in Audio Thread

### Data Types
- `std::atomic<T>` (with proper memory ordering)
- `std::array<T, N>` (fixed size)
- Pre-allocated `std::vector` (with `reserve()` called in prepareToPlay)
- POD types on the stack
- Fixed-size char arrays

### Synchronization
- `std::atomic` load/store
- Lock-free FIFOs (`juce::AbstractFifo`)
- Seqlock pattern (for larger structs)
- Spinlocks (ONLY for < 100 CPU cycles)

### Operations
- Math functions (`std::sin`, `std::cos`, `std::sqrt`)
- Buffer operations (copy, add, multiply)
- Pre-computed lookup tables
- Simple control flow

## 🔧 Common Patterns

### Parameter Updates (UI → Audio)
```cpp
class MyProcessor {
    std::atomic<float> gain{1.0f};
    
    void setGain(float g) {  // UI thread
        gain.store(g, std::memory_order_relaxed);
    }
    
    void processBlock(AudioBuffer& buffer) {  // Audio thread
        float g = gain.load(std::memory_order_relaxed);
        buffer.applyGain(g);
    }
};
```

### Audio Data → UI (with FIFO)
```cpp
class MyAnalyzer {
    LockFreeCircularBuffer<float, 1024> fifo;
    
    void processBlock(AudioBuffer& buffer) {  // Audio thread
        float peak = buffer.getMagnitude(0, buffer.getNumSamples());
        fifo.push(peak);  // Lock-free
    }
    
    void updateUI() {  // UI thread
        float peak;
        if (fifo.pop(peak)) {
            // Update UI with peak value
        }
    }
};
```

### Pre-Allocation in prepareToPlay
```cpp
class MyEffect {
    std::vector<float> delayBuffer;
    
    void prepareToPlay(double sampleRate, int maxBlockSize) {
        // Allocate 1 second of delay
        delayBuffer.resize(static_cast<size_t>(sampleRate));
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
    }
    
    void processBlock(AudioBuffer& buffer) {
        // Use delayBuffer - already allocated
    }
};
```

## 📊 Memory Ordering Quick Guide

| Operation | Memory Order | Use Case |
|-----------|--------------|----------|
| Local read | `relaxed` | Counter, status flag |
| Read shared data | `acquire` | Reading from FIFO |
| Write shared data | `release` | Writing to FIFO |
| Read-modify-write | `acq_rel` | `fetch_add`, `compare_exchange` |
| Avoid | `seq_cst` | Too slow for RT |

## 🧪 Testing Checklist

Before committing audio code:

- [ ] No `new`/`malloc` in processBlock
- [ ] No mutex locks in audio path
- [ ] All buffers pre-allocated
- [ ] Used `std::atomic` with correct memory_order
- [ ] No string allocations
- [ ] No system calls
- [ ] Added RT-safe comment
- [ ] Tested under load

## 🚨 Quick Fixes

### Problem: Need to pass string to audio thread
```cpp
// ❌ BAD
juce::String name;
void processBlock() { 
    if (name == "reverb") { ... }  // Allocates!
}

// ✅ GOOD
char name[32];
void setName(const char* n) {
    std::strncpy(name, n, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
}
void processBlock() {
    if (std::strcmp(name, "reverb") == 0) { ... }  // No allocation
}
```

### Problem: Need timestamp
```cpp
// ❌ BAD
auto time = juce::Time::getCurrentTime();  // Syscall!

// ✅ GOOD
static std::atomic<uint64_t> counter{0};
uint64_t time = counter.fetch_add(1, std::memory_order_relaxed);
```

### Problem: Need to copy buffer
```cpp
// ❌ BAD
void processBlock(const AudioBuffer& buffer) {
    AudioBuffer copy(buffer);  // Allocates!
}

// ✅ GOOD
AudioBuffer preallocatedBuffer;  // Member variable
void prepareToPlay(double sr, int maxBlock) {
    preallocatedBuffer.setSize(2, maxBlock);
}
void processBlock(const AudioBuffer& buffer) {
    preallocatedBuffer.copyFrom(buffer);  // No allocation
}
```

## 📚 More Info

- Full guidelines: `docs/RT_SAFETY.md`
- Audit report: `docs/THREAD_SAFETY_AUDIT.md`
- JUCE docs: https://docs.juce.com/
- Lock-free programming: https://www.1024cores.net/

## 🎯 Remember

**When in doubt, DON'T do it in the audio thread!**

Use a background thread or defer to the next prepareToPlay() call.
