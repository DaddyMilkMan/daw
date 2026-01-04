# Audio Real-Time Safety Rules

CRITICAL: The audio thread (processBlock, getNextAudioBlock) must be strictly real-time safe. Violations cause glitches.

## Core Rules

1. **NO Allocations**: Never use new, malloc, or resizing containers (e.g., std::vector::push_back, std::string creation) inside the audio callback.
   - ❌ FORBIDDEN: `new`, `delete`, `malloc`, `free`, `std::vector::push_back()`, `std::string` ops, `juce::String` ops
   - ✅ ALLOWED: `std::array<T, N>`, pre-allocated `std::vector` with `reserve()`, fixed-size char arrays

2. **NO Blocking**: Do not use std::mutex, juce::CriticalSection, or sleep. Use std::atomic parameters or lock-free FIFOs (juce::AbstractFifo) for thread communication.
   - ❌ FORBIDDEN: `std::mutex`, `juce::CriticalSection`, `std::lock_guard`, `std::unique_lock`, `sleep()`
   - ✅ ALLOWED: `std::atomic` (with proper memory ordering), lock-free FIFOs, seqlock pattern

3. **NO System Calls**: Strictly avoid std::cout, printf, file I/O, or OS event logging.
   - ❌ FORBIDDEN: `std::cout`, `printf`, `juce::Logger`, `juce::Time::getCurrentTime()`, file I/O
   - ✅ ALLOWED: Pre-computed values, atomic counters, lookup tables

4. **NO RTTI**: Do not use dynamic_cast in the hot path. Use static polymorphism or unchecked casts if the type is guaranteed.
   - ❌ FORBIDDEN: `dynamic_cast<T*>`, `throw`/`catch` exceptions
   - ✅ ALLOWED: `static_cast<T*>` (when type is known), templates, direct function calls

5. **Pre-Allocation**: All buffers and objects must be allocated in prepareToPlay or the constructor. Capacity must be reserved upfront.
   - Allocate in: `prepareToPlay()`, constructor, or non-RT initialization functions
   - Use in RT context: Only pre-allocated resources

## Memory Ordering for Atomics

Always specify memory ordering for `std::atomic` operations:

- `memory_order_relaxed` - Local operations, counters
- `memory_order_acquire` - Reading shared data (pairs with release)
- `memory_order_release` - Writing shared data (pairs with acquire)
- `memory_order_acq_rel` - Read-modify-write operations
- Avoid `memory_order_seq_cst` - Too slow for RT

## Documentation

For detailed guidelines, see:
- `docs/RT_SAFETY.md` - Comprehensive guidelines and patterns
- `docs/RT_SAFETY_QUICK_REF.md` - Quick reference for developers
- `docs/THREAD_SAFETY_AUDIT.md` - Audit results and recommendations

## Code Annotations

Mark functions with RT-safety status:

```cpp
// RT-SAFE: This function is real-time safe
void processAudio(const juce::AudioBuffer<float>& buffer);

// NOT RT-SAFE: This function may allocate or block
void loadPreset(const juce::File& file);
```
