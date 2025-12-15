# Audio Real-Time Safety Rules
CRITICAL: The audio thread (processBlock, getNextAudioBlock) must be strictly real-time safe. Violations cause glitches.

1. **NO Allocations**: Never use 
ew, malloc, or resizing containers (e.g., std::vector::push_back, std::string creation) inside the audio callback.
2. **NO Blocking**: Do not use std::mutex, juce::CriticalSection, or sleep. Use std::atomic parameters or lock-free FIFOs (juce::AbstractFifo) for thread communication.
3. **NO System Calls**: Strictly avoid std::cout, printf, file I/O, or OS event logging.
4. **NO RTTI**: Do not use dynamic_cast in the hot path. Use static polymorphism or unchecked casts if the type is guaranteed.
5. **Pre-Allocation**: All buffers and objects must be allocated in prepareToPlay or the constructor. capacity must be reserved upfront.
