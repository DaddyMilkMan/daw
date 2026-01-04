# RT-Safety Code Review Checklist

Use this checklist when reviewing audio-related code changes.

## File Information
- **File(s):** ___________________________
- **Author:** ___________________________
- **Reviewer:** ___________________________
- **Date:** ___________________________

---

## Critical: Audio Thread Safety

### Memory Allocation ❌ FORBIDDEN
- [ ] No `new` or `delete` in processBlock
- [ ] No `malloc` or `free` in processBlock
- [ ] No `std::make_unique` or `std::make_shared`
- [ ] No `std::vector::push_back()` or `resize()`
- [ ] No `std::string` operations (concatenation, construction)
- [ ] No `juce::String` operations
- [ ] No `juce::AudioBuffer` construction

### Blocking Operations ❌ FORBIDDEN
- [ ] No `std::mutex` or `std::lock_guard`
- [ ] No `juce::CriticalSection` or `juce::ScopedLock`
- [ ] No `std::condition_variable::wait()`
- [ ] No `sleep()` or `std::this_thread::sleep_for()`

### System Calls ❌ FORBIDDEN
- [ ] No `std::cout`, `printf`, or `fprintf`
- [ ] No `juce::Logger::writeToLog()`
- [ ] No `juce::Time::getCurrentTime()`
- [ ] No file I/O (`open`, `read`, `write`, `fstream`)
- [ ] No network I/O

### RTTI ❌ AVOID
- [ ] No `dynamic_cast` in hot paths
- [ ] No `throw`/`catch` exceptions in processBlock

---

## Required: Pre-Allocation

### Buffers and Resources ✅ REQUIRED
- [ ] All buffers allocated in `prepareToPlay()`
- [ ] Vector capacity reserved in `prepareToPlay()`
- [ ] No runtime container resizing
- [ ] Temporary buffers pre-allocated as member variables

---

## Lock-Free Patterns

### Atomic Operations ✅ IF NEEDED
- [ ] Uses `std::atomic<T>` for simple values
- [ ] Specifies memory ordering explicitly
  - [ ] `memory_order_relaxed` for independent ops
  - [ ] `memory_order_acquire` for reading shared data
  - [ ] `memory_order_release` for writing shared data
  - [ ] Avoids `memory_order_seq_cst` (too slow)

### Lock-Free Structures ✅ IF NEEDED
- [ ] Uses `juce::AbstractFifo` or `LockFreeCircularBuffer`
- [ ] Single-producer/single-consumer verified
- [ ] Memory ordering correct

---

## Code Quality

### Documentation
- [ ] Functions marked with RT-safety status
  - [ ] `// RT-SAFE:` for RT-safe functions
  - [ ] `// NOT RT-SAFE:` for unsafe functions
- [ ] Complex algorithms explained
- [ ] Memory ordering choices documented

### Style
- [ ] Follows project coding standards
- [ ] No unnecessary complexity
- [ ] Clear variable names
- [ ] Reasonable function length

---

## Testing

### Verification
- [ ] Code compiles without warnings
- [ ] Unit tests pass (if applicable)
- [ ] Tested under high CPU load
- [ ] No audio glitches observed

### Recommended
- [ ] ThreadSanitizer (TSan) clean
- [ ] AddressSanitizer (ASan) clean
- [ ] Profiled with real-time monitoring

---

## Common Patterns to Check

### String Handling
```cpp
// ❌ BAD
juce::String name = "reverb_" + juce::String(id);

// ✅ GOOD
char name[32];
std::snprintf(name, sizeof(name), "reverb_%d", id);
```

### Parameter Updates
```cpp
// ❌ BAD
std::mutex paramMutex;
float gain;
void setGain(float g) { 
    std::lock_guard lock(paramMutex);
    gain = g; 
}

// ✅ GOOD
std::atomic<float> gain{1.0f};
void setGain(float g) { 
    gain.store(g, std::memory_order_relaxed); 
}
```

### Buffer Copies
```cpp
// ❌ BAD
void processBlock(const AudioBuffer& buffer) {
    AudioBuffer temp(buffer);  // Allocates!
}

// ✅ GOOD
AudioBuffer tempBuffer;  // Member variable
void prepareToPlay(double sr, int maxBlock) {
    tempBuffer.setSize(2, maxBlock);
}
void processBlock(const AudioBuffer& buffer) {
    tempBuffer.copyFrom(buffer);  // No allocation
}
```

---

## Red Flags 🚩

Look out for these common issues:

- [ ] ⚠️ `new` or `malloc` anywhere near processBlock
- [ ] ⚠️ `std::vector` or `std::string` in audio callback
- [ ] ⚠️ Mutex locks in audio path
- [ ] ⚠️ File or network operations
- [ ] ⚠️ `juce::String` concatenation
- [ ] ⚠️ System time calls
- [ ] ⚠️ Exceptions thrown/caught
- [ ] ⚠️ Virtual function calls in tight loops
- [ ] ⚠️ `dynamic_cast` in hot path
- [ ] ⚠️ Missing memory ordering on atomics

---

## Decision

### Approve ✅
- [ ] All critical checks passed
- [ ] No RT-safety violations found
- [ ] Code is well-documented
- [ ] Tests pass

**Signature:** ___________________________

### Request Changes ⚠️
- [ ] RT-safety violations found (list below)
- [ ] Missing documentation
- [ ] Tests needed

**Issues to fix:**
1. ___________________________________________
2. ___________________________________________
3. ___________________________________________

**Signature:** ___________________________

### Reject ❌
- [ ] Critical RT-safety violations
- [ ] Would cause audio glitches
- [ ] Needs redesign

**Reason:** ___________________________________________

**Signature:** ___________________________

---

## Resources

- **Full guidelines:** `docs/RT_SAFETY.md`
- **Quick reference:** `docs/RT_SAFETY_QUICK_REF.md`
- **Audit report:** `docs/THREAD_SAFETY_AUDIT.md`

## Notes

Additional comments or concerns:

___________________________________________
___________________________________________
___________________________________________
___________________________________________

---

**Review completed:** ___________________________  
**Date:** ___________________________
