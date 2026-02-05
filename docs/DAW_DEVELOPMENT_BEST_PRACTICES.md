# Digital Audio Workstation (DAW) Development Best Practices

**Research Document** | **Version 1.0** | **January 2026**

This document compiles authoritative best practices for professional DAW development, drawing from industry experts, academic research, and proven production codebases.

---

## Table of Contents

1. [Real-time Audio Programming Best Practices](#1-real-time-audio-programming-best-practices)
2. [DAW Architecture Patterns](#2-daw-architecture-patterns)
3. [Performance Optimization](#3-performance-optimization)
4. [Modern C++ in Audio](#4-modern-c-in-audio)
5. [Testing & Quality Assurance](#5-testing--quality-assurance)
6. [Authoritative Sources](#6-authoritative-sources)

---

## 1. Real-time Audio Programming Best Practices

### 1.1 Understanding Real-time Constraints

Audio threads operate under **firm real-time constraints** - missing a deadline results in audible glitches (xruns). The CPU budget is surprisingly tight:

```
CPU Budget Formula:
CPUBudget = (clockSpeed × bufferSize) / (samplingRate × numChannels)

Example (worst case - slow embedded device):
- 100 MHz CPU, 192kHz, 2 channels
- Per-sample budget: ~260 cycles
- @ 256 samples buffer: ~1.3ms per callback

Example (modern desktop):
- 3.5 GHz CPU, 44.1kHz, 2 channels  
- Per-sample budget: ~40,000 cycles
- @ 256 samples buffer: ~5.8ms per callback
- With 10 plugins: only ~4,000 cycles per sample per plugin
```

**Key Insight**: Audio programmers must optimize for **worst-case execution time**, not average case.

### 1.2 Lock-free Programming Patterns

#### The Golden Rules for Audio Threads

| Operation | Audio Thread Safe? | Alternative |
|-----------|-------------------|-------------|
| Memory allocation (`new`/`malloc`) | ❌ NO | Pre-allocate in constructor |
| Mutex locks (`std::mutex`) | ❌ NO | Lock-free queues, atomics |
| System calls | ❌ NO | Do on message thread |
| File I/O | ❌ NO | Background thread + lock-free queue |
| Logging | ❌ NO | Lock-free ring buffer |
| `std::function` (may allocate) | ⚠️ CAREFUL | Use templates, function pointers |
| Lock-free atomics | ✅ YES | `std::atomic` with `memory_order_relaxed` |
| Pre-allocated buffers | ✅ YES | Stack or member allocation |
| JUCE FloatVectorOperations | ✅ YES | SIMD-accelerated operations |

#### Lock-free Ring Buffer Implementation

```cpp
/**
 * Single-producer, single-consumer lock-free ring buffer
 * Thread-safe for one writer and one reader without locks
 */
template<typename T, size_t Size>
class LockFreeRingBuffer {
    static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");
    
    std::array<T, Size> buffer_;
    std::atomic<size_t> writeIndex_{0};
    std::atomic<size_t> readIndex_{0};
    
    static constexpr size_t MASK = Size - 1;
    
public:
    static constexpr size_t CAPACITY = Size;
    
    bool push(const T& item) noexcept {
        const auto writeIdx = writeIndex_.load(std::memory_order_relaxed);
        const auto readIdx = readIndex_.load(std::memory_order_acquire);
        
        if ((writeIdx - readIdx) >= Size) {
            return false; // Buffer full
        }
        
        buffer_[writeIdx & MASK] = item;
        writeIndex_.store(writeIdx + 1, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) noexcept {
        const auto readIdx = readIndex_.load(std::memory_order_relaxed);
        const auto writeIdx = writeIndex_.load(std::memory_order_acquire);
        
        if (readIdx >= writeIdx) {
            return false; // Buffer empty
        }
        
        item = buffer_[readIdx & MASK];
        readIndex_.store(readIdx + 1, std::memory_order_release);
        return true;
    }
    
    size_t size() const noexcept {
        return writeIndex_.load(std::memory_order_relaxed) - 
               readIndex_.load(std::memory_order_relaxed);
    }
    
    void clear() noexcept {
        readIndex_.store(writeIndex_.load(std::memory_order_relaxed), 
                        std::memory_order_relaxed);
    }
};
```

### 1.3 Memory Management Strategies

#### Pre-allocation Pattern

```cpp
class RealTimeProcessor {
    // Pre-allocated in constructor (non-RT thread)
    juce::AudioBuffer<float> tempBuffer_;
    std::vector<float> delayLine_;
    juce::MidiBuffer midiScratch_;
    
public:
    RealTimeProcessor() 
        : tempBuffer_(2, 512),  // 2 channels, max block size
          delayLine_(44100),    // 1 second at 44.1kHz
          midiScratch_{} {
        midiScratch_.ensureSize(1024); // Pre-allocate MIDI capacity
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer, 
                      juce::MidiBuffer& midiMessages) noexcept {
        // RT-SAFE: No allocations here!
        jassert(buffer.getNumSamples() <= tempBuffer_.getNumSamples());
        
        tempBuffer_.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
        // ... process ...
    }
};
```

#### Memory Pool for Variable Sizes

```cpp
/**
 * Fixed-size memory pool for real-time allocation
 * O(1) allocation/deallocation, no system calls
 */
class MemoryPool {
    struct Block {
        alignas(64) std::array<std::byte, 1024> data;
        std::atomic<bool> inUse{false};
    };
    
    std::array<Block, 256> blocks_;  // 256KB pool
    
public:
    void* allocate() noexcept {
        for (auto& block : blocks_) {
            bool expected = false;
            if (block.inUse.compare_exchange_strong(
                expected, true, 
                std::memory_order_acquire)) {
                return block.data.data();
            }
        }
        return nullptr; // Pool exhausted
    }
    
    void deallocate(void* ptr) noexcept {
        for (auto& block : blocks_) {
            if (block.data.data() == ptr) {
                block.inUse.store(false, std::memory_order_release);
                return;
            }
        }
    }
};
```

### 1.4 Thread Synchronization: Audio ↔ UI

#### The Reader-Writer Pattern

```cpp
/**
 * Thread-safe parameter updates from UI to Audio thread
 * Uses atomic exchange for lock-free updates
 */
class SmoothedParameter {
    std::atomic<float> targetValue_{0.0f};
    std::atomic<float> currentValue_{0.0f};
    
public:
    // Called from UI thread (non-RT)
    void setTarget(float value) noexcept {
        targetValue_.store(value, std::memory_order_relaxed);
    }
    
    // Called from audio thread (RT)
    float getNextValue() noexcept {
        float target = targetValue_.load(std::memory_order_relaxed);
        float current = currentValue_.load(std::memory_order_relaxed);
        
        // One-pole smoothing (fast, branchless)
        const float coeff = 0.1f; // Smoothing coefficient
        current += (target - current) * coeff;
        
        currentValue_.store(current, std::memory_order_relaxed);
        return current;
    }
};
```

#### Double Buffering for Complex Data

```cpp
/**
 * Lock-free double buffering for transferring large data
 * (e.g., wavetables, impulse responses) to audio thread
 */
template<typename T>
class DoubleBuffer {
    std::array<std::unique_ptr<T>, 2> buffers_;
    std::atomic<int> activeIndex_{0};      // Audio thread reads from here
    std::atomic<bool> newDataReady_{false};
    
public:
    DoubleBuffer() {
        buffers_[0] = std::make_unique<T>();
        buffers_[1] = std::make_unique<T>();
    }
    
    // Audio thread (RT-safe)
    const T* getActiveBuffer() const noexcept {
        return buffers_[activeIndex_.load(std::memory_order_acquire)].get();
    }
    
    // UI thread - swap buffers
    void update(const T& newData) noexcept {
        int inactiveIndex = 1 - activeIndex_.load(std::memory_order_relaxed);
        *buffers_[inactiveIndex] = newData;
        
        // Atomic swap
        activeIndex_.store(inactiveIndex, std::memory_order_release);
        newDataReady_.store(true, std::memory_order_relaxed);
    }
};
```

### 1.5 Common Anti-patterns That Cause Xruns

| Anti-pattern | Problem | Solution |
|--------------|---------|----------|
| `std::vector::push_back` in processBlock | May reallocate | Pre-allocate, use fixed arrays |
| `std::map` lookups | Tree traversal, allocations | Use `std::array` or flat hash maps |
| `std::function` with captures | May heap-allocate | Use templates, function pointers |
| `juce::String` concatenation | Allocations | Use pre-allocated `char` buffers |
| `std::shared_ptr` copy | Atomic refcount increment | Use raw pointers, reference wrapper |
| `std::mutex::lock` | Priority inversion, blocking | Lock-free atomics, try_lock only |
| Dynamic_cast | RTTI lookup | Use static_cast with type enums |
| Virtual function calls in tight loops | Vtable indirection | CRTP (Curiously Recurring Template Pattern) |

---

## 2. DAW Architecture Patterns

### 2.1 Audio Engine Design: Graph vs Linear

#### Graph-Based Processing (Recommended for DAWs)

```cpp
/**
 * Audio processing graph with topological sorting
 * Handles arbitrary routing, feedback loops, parallel processing
 */
class AudioGraph {
    struct Node {
        std::unique_ptr<AudioProcessor> processor;
        std::vector<Node*> inputs;
        std::vector<Node*> outputs;
        int visitCount = 0;  // For cycle detection
        
        // For parallel execution
        std::atomic<int> dependenciesRemaining{0};
        std::vector<Node*> dependents;
    };
    
    std::vector<std::unique_ptr<Node>> nodes_;
    std::vector<Node*> sortedOrder_;
    
public:
    /**
     * Topological sort for deterministic processing order
     * Kahn's algorithm - O(V + E)
     */
    void updateProcessingOrder() {
        sortedOrder_.clear();
        
        std::queue<Node*> ready;
        for (auto& node : nodes_) {
            node->visitCount = static_cast<int>(node->inputs.size());
            if (node->visitCount == 0) {
                ready.push(node.get());
            }
        }
        
        while (!ready.empty()) {
            Node* current = ready.front();
            ready.pop();
            sortedOrder_.push_back(current);
            
            for (Node* output : current->outputs) {
                if (--output->visitCount == 0) {
                    ready.push(output);
                }
            }
        }
        
        // Cycle detection
        if (sortedOrder_.size() != nodes_.size()) {
            handleCycleDetected();
        }
    }
    
    void processBlock(juce::AudioBuffer<float>& output) {
        // Process in topological order
        for (Node* node : sortedOrder_) {
            node->processor->processBlock(/* ... */);
        }
    }
};
```

#### Tracktion Graph Pattern (from Tracktion Engine)

```cpp
/**
 * Modern graph processing with parallel execution support
 * Based on Tracktion Graph library (ADC 2020)
 */
class TracktionGraphProcessor {
    struct Node {
        AudioProcessor* processor;
        std::vector<Node*> inputs;
        juce::AudioBuffer<float> outputBuffer;
        
        // For parallel processing
        std::atomic<bool> processed{false};
        std::atomic<int> unprocessedInputs;
        
        void prepare(int numChannels, int maxSamples) {
            outputBuffer.setSize(numChannels, maxSamples);
        }
    };
    
public:
    /**
     * Parallel processing using work-stealing queue
     * Each thread grabs ready nodes and processes them
     */
    void processParallel(juce::AudioBuffer<float>& output,
                        juce::ThreadPool& threadPool) {
        
        std::queue<Node*> readyQueue;
        std::mutex queueMutex;
        
        // Find initially ready nodes (no inputs)
        for (auto& node : nodes_) {
            if (node->inputs.empty()) {
                readyQueue.push(node.get());
            }
        }
        
        // Worker function
        auto worker = [&]() {
            while (true) {
                Node* node = nullptr;
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if (readyQueue.empty()) break;
                    node = readyQueue.front();
                    readyQueue.pop();
                }
                
                // Process node
                processNode(node);
                
                // Mark dependents as ready if all inputs processed
                for (Node* dependent : node->dependents) {
                    if (--dependent->unprocessedInputs == 0) {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        readyQueue.push(dependent);
                    }
                }
            }
        };
        
        // Launch workers
        std::vector<std::future<void>> futures;
        for (int i = 0; i < threadPool.getNumThreads(); ++i) {
            futures.push_back(std::async(std::launch::async, worker));
        }
        
        for (auto& f : futures) f.wait();
    }
};
```

### 2.2 Plugin Hosting Architecture

#### VST3/AU/CLAP Abstraction Layer

```cpp
/**
 * Unified plugin hosting interface
 * Supports VST3, AU, CLAP formats
 */
class PluginHost {
public:
    struct PluginInstance {
        juce::AudioProcessor* processor;
        juce::AudioProcessorEditor* editor = nullptr;
        juce::MemoryBlock state;
        
        // Latency compensation
        int reportedLatency = 0;
        juce::AudioBuffer<float> latencyBuffer;
        
        // Thread safety
        std::atomic<bool> bypassed{false};
        std::atomic<bool> suspended{false};
    };
    
private:
    std::unique_ptr<juce::AudioPluginFormatManager> formatManager_;
    std::vector<std::unique_ptr<PluginInstance>> plugins_;
    
public:
    void initialize() {
        formatManager_ = std::make_unique<juce::AudioPluginFormatManager>();
        formatManager_->addDefaultFormats(); // VST3, AU, etc.
    }
    
    /**
     * Load plugin with error handling and validation
     */
    PluginInstance* loadPlugin(const juce::String& identifier,
                               double sampleRate,
                               int blockSize) {
        juce::String error;
        auto instance = formatManager_->createPluginInstance(
            identifier, sampleRate, blockSize, error);
        
        if (!instance) {
            logError("Failed to load plugin: " + error);
            return nullptr;
        }
        
        auto plugin = std::make_unique<PluginInstance>();
        plugin->processor = instance.release();
        
        // Get initial latency
        plugin->reportedLatency = plugin->processor->getLatencySamples();
        plugin->latencyBuffer.setSize(
            plugin->processor->getTotalNumOutputChannels(),
            plugin->reportedLatency + blockSize);
        
        auto* result = plugin.get();
        plugins_.push_back(std::move(plugin));
        return result;
    }
    
    /**
     * Process with latency compensation
     * Handles variable latency from plugins
     */
    void processPluginWithLatency(PluginInstance* plugin,
                                  juce::AudioBuffer<float>& buffer,
                                  juce::MidiBuffer& midi) {
        if (plugin->bypassed.load(std::memory_order_relaxed)) {
            return; // Pass-through
        }
        
        auto* proc = plugin->processor;
        const int latency = proc->getLatencySamples();
        
        if (latency > 0) {
            // Store current input for later output
            // (simplified - real implementation needs circular buffer)
            plugin->latencyBuffer.copyFrom(0, latency, buffer, 0, 0, 
                                          buffer.getNumSamples());
        }
        
        proc->processBlock(buffer, midi);
    }
};
```

#### Plugin Sandboxing Architecture

```cpp
/**
 * Plugin sandbox using separate process (future architecture)
 * Prevents crashes in plugins from affecting the DAW
 */
class PluginSandbox {
    struct SandboxProcess {
        juce::ChildProcess process;
        juce::InterprocessConnection connection;
        
        // Shared memory for audio buffers (zero-copy)
        juce::SharedMemory sharedAudioBuffer;
        juce::SharedMemory sharedMidiBuffer;
        
        std::atomic<bool> crashed{false};
    };
    
public:
    /**
     * Run plugin in separate process
     * Communication via shared memory + sockets
     */
    bool loadInSandbox(const juce::String& pluginPath) {
        // Launch sandbox host executable
        juce::StringArray args;
        args.add("--plugin-path=" + pluginPath);
        args.add("--shared-mem-id=" + generateSharedMemId());
        
        sandbox_.process.start("ZenithPluginSandbox", args);
        
        // Wait for connection with timeout
        if (!waitForConnection(5000)) {
            return false;
        }
        
        return true;
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi) {
        if (sandbox_.crashed.load()) {
            // Graceful degradation - bypass processing
            return;
        }
        
        // Write to shared memory
        copyToSharedMemory(buffer, sandbox_.sharedAudioBuffer);
        
        // Send process command
        sandbox_.connection.sendMessage("PROCESS");
        
        // Wait for completion (with watchdog)
        if (!waitForResponse(10)) { // 10ms timeout
            handlePluginTimeout();
        }
        
        // Read back result
        copyFromSharedMemory(sandbox_.sharedAudioBuffer, buffer);
    }
    
private:
    void handlePluginTimeout() {
        sandbox_.crashed.store(true);
        sandbox_.process.kill();
        
        // Notify user of crash
        // Auto-restart with last known good state if possible
    }
};
```

### 2.3 Undo/Redo System Design

#### Command Pattern with Transaction Support

```cpp
/**
 * Comprehensive undo system with transaction support
 * Based on JUCE UndoManager with audio-specific extensions
 */
class UndoManager {
    struct Command {
        virtual ~Command() = default;
        virtual void undo() = 0;
        virtual void redo() = 0;
        virtual juce::String getDescription() const = 0;
        virtual bool coalesceWith(Command* other) { return false; }
    };
    
    std::vector<std::unique_ptr<Command>> history_;
    size_t currentIndex_ = 0;
    size_t maxSize_ = 100;
    
    bool isPerformingUndoRedo_ = false;
    
public:
    void perform(std::unique_ptr<Command> command) {
        if (isPerformingUndoRedo_) {
            jassertfalse; // Don't record during undo/redo!
            return;
        }
        
        // Remove any redo history
        history_.resize(currentIndex_);
        
        // Try to coalesce with previous command (e.g., drag operations)
        if (!history_.empty() && 
            history_.back()->coalesceWith(command.get())) {
            return; // Merged, don't add new command
        }
        
        command->redo();
        history_.push_back(std::move(command));
        currentIndex_++;
        
        // Trim if exceeding max
        if (history_.size() > maxSize_) {
            history_.erase(history_.begin());
            currentIndex_--;
        }
    }
    
    void undo() {
        if (currentIndex_ == 0) return;
        
        isPerformingUndoRedo_ = true;
        history_[--currentIndex_]->undo();
        isPerformingUndoRedo_ = false;
    }
    
    void redo() {
        if (currentIndex_ >= history_.size()) return;
        
        isPerformingUndoRedo_ = true;
        history_[currentIndex_++]->redo();
        isPerformingUndoRedo_ = false;
    }
};

/**
 * Example: Audio clip edit command with coalescing
 */
class ClipMoveCommand : public UndoManager::Command {
    juce::ValueTree clipState_;
    double oldStartTime_;
    double newStartTime_;
    juce::UndoManager& undoManager_;
    
public:
    void undo() override {
        clipState_.setProperty("startTime", oldStartTime_, &undoManager_);
    }
    
    void redo() override {
        clipState_.setProperty("startTime", newStartTime_, &undoManager_);
    }
    
    /**
     * Coalesce consecutive moves of the same clip
     * User drags clip: many small moves become one command
     */
    bool coalesceWith(Command* other) override {
        if (auto* otherMove = dynamic_cast<ClipMoveCommand*>(other)) {
            if (otherMove->clipState_ == clipState_) {
                newStartTime_ = otherMove->newStartTime_;
                return true; // Absorbed into this command
            }
        }
        return false;
    }
};
```

### 2.4 Project State Management

#### ValueTree-Based State (JUCE Pattern)

```cpp
/**
 * Centralized project state using JUCE ValueTree
 * Provides serialization, undo, and change notification
 */
class ProjectState {
    juce::ValueTree state_{"Project"};
    juce::UndoManager undoManager_;
    
    // Cached references for fast access
    juce::ValueTree tracksNode_;
    juce::ValueTree mixerNode_;
    
public:
    ProjectState() {
        state_.setProperty("version", "1.0", nullptr);
        state_.setProperty("sampleRate", 44100.0, nullptr);
        
        tracksNode_ = state_.getOrCreateChildWithName("Tracks", nullptr);
        mixerNode_ = state_.getOrCreateChildWithName("Mixer", nullptr);
    }
    
    /**
     * Thread-safe: All modifications happen on message thread
     */
    juce::ValueTree addTrack(const juce::String& name) {
        juce::ValueTree track("Track");
        track.setProperty("name", name, &undoManager_);
        track.setProperty("id", generateUuid(), nullptr);
        track.setProperty("volume", 0.0f, &undoManager_); // dB
        track.setProperty("pan", 0.0f, &undoManager_);    // -1 to 1
        track.setProperty("mute", false, &undoManager_);
        track.setProperty("solo", false, &undoManager_);
        
        tracksNode_.addChild(track, -1, &undoManager_);
        return track;
    }
    
    /**
     * Serialization for save/load
     */
    void saveToFile(const juce::File& file) {
        std::unique_ptr<juce::XmlElement> xml(state_.createXml());
        xml->writeTo(file);
    }
    
    bool loadFromFile(const juce::File& file) {
        auto xml = juce::XmlDocument::parse(file);
        if (!xml) return false;
        
        juce::ValueTree newState = juce::ValueTree::fromXml(*xml);
        if (!newState.isValid()) return false;
        
        state_ = newState;
        tracksNode_ = state_.getChildWithName("Tracks");
        mixerNode_ = state_.getChildWithName("Mixer");
        
        return true;
    }
    
    /**
     * Change listening for UI updates
     */
    void addListener(juce::ValueTree::Listener* listener) {
        state_.addListener(listener);
    }
};
```

---

## 3. Performance Optimization

### 3.1 SIMD Usage in DSP Code

#### JUCE FloatVectorOperations (Cross-platform SIMD)

```cpp
#include <juce_dsp/juce_dsp.h>

/**
 * SIMD-optimized gain application
 * Automatically uses SSE/AVX on x86, NEON on ARM
 */
void applyGainSIMD(juce::AudioBuffer<float>& buffer, float gain) noexcept {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        juce::FloatVectorOperations::multiply(
            buffer.getWritePointer(ch),
            gain,
            buffer.getNumSamples()
        );
    }
}

/**
 * Manual SIMD with JUCE SIMDRegister
 * More control, still cross-platform
 */
void processSIMDManual(juce::AudioBuffer<float>& buffer) {
    using namespace juce::dsp;
    using Vec = SIMDRegister<float>;
    
    constexpr int SIMD_WIDTH = Vec::SIMDNumElements; // 4 for float32x4_t
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        int numSamples = buffer.getNumSamples();
        
        // Process SIMD-width chunks
        int simdSamples = numSamples / SIMD_WIDTH * SIMD_WIDTH;
        
        for (int i = 0; i < simdSamples; i += SIMD_WIDTH) {
            Vec vec = Vec::fromRawPointer(data + i);
            
            // Process 4 samples at once
            vec = vec * 0.5f;  // Example: gain
            vec = vec + 0.1f;  // Example: offset
            
            vec.copyToRawPointer(data + i);
        }
        
        // Handle remaining samples (scalar fallback)
        for (int i = simdSamples; i < numSamples; ++i) {
            data[i] = data[i] * 0.5f + 0.1f;
        }
    }
}
```

#### Platform-Specific Intrinsics (When Needed)

```cpp
/**
 * x86 SSE/AVX optimized FIR filter
 * Fallback to scalar for other platforms
 */
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>

void firFilterSSE(float* output, const float* input, 
                  const float* coeffs, int numSamples, int numTaps) {
    
    for (int i = 0; i < numSamples; ++i) {
        __m128 sum = _mm_setzero_ps();
        
        int j = 0;
        // Process 4 taps at a time
        for (; j <= numTaps - 4; j += 4) {
            __m128 in = _mm_loadu_ps(input + i - j);
            __m128 coef = _mm_loadu_ps(coeffs + j);
            sum = _mm_add_ps(sum, _mm_mul_ps(in, coef));
        }
        
        // Horizontal sum
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        
        float result = _mm_cvtss_f32(sum);
        
        // Handle remaining taps (scalar)
        for (; j < numTaps; ++j) {
            result += input[i - j] * coeffs[j];
        }
        
        output[i] = result;
    }
}

#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>

void firFilterNEON(float* output, const float* input,
                   const float* coeffs, int numSamples, int numTaps) {
    
    for (int i = 0; i < numSamples; ++i) {
        float32x4_t sum = vdupq_n_f32(0.0f);
        
        int j = 0;
        for (; j <= numTaps - 4; j += 4) {
            float32x4_t in = vld1q_f32(input + i - j);
            float32x4_t coef = vld1q_f32(coeffs + j);
            sum = vmlaq_f32(sum, in, coef);  // Fused multiply-add
        }
        
        // Horizontal sum
        float result = vaddvq_f32(sum);
        
        // Remaining taps
        for (; j < numTaps; ++j) {
            result += input[i - j] * coeffs[j];
        }
        
        output[i] = result;
    }
}
#endif
```

### 3.2 Cache-Friendly Data Structures

#### Structure of Arrays (SoA) vs Array of Structures (AoS)

```cpp
/**
 * BAD: Array of Structures - cache inefficient
 * Each sample access jumps around memory
 */
struct VoiceAoS {
    float frequency;
    float phase;
    float amplitude;
    float filterState;
};
std::vector<VoiceAoS> voicesAoS;  // BAD for SIMD/cache

/**
 * GOOD: Structure of Arrays - cache friendly
 * Sequential access patterns, SIMD-friendly
 */
struct VoiceSoA {
    std::vector<float> frequency;
    std::vector<float> phase;
    std::vector<float> amplitude;
    std::vector<float> filterState;
    
    void resize(size_t numVoices) {
        frequency.resize(numVoices);
        phase.resize(numVoices);
        amplitude.resize(numVoices);
        filterState.resize(numVoices);
    }
    
    /**
     * Process all voices - cache-friendly sequential access
     */
    void processBlock(float* output, int numSamples, int numVoices) {
        for (int sample = 0; sample < numSamples; ++sample) {
            float sum = 0.0f;
            
            for (int voice = 0; voice < numVoices; ++voice) {
                // Sequential memory access = cache hits
                phase[voice] += frequency[voice];
                sum += std::sin(phase[voice]) * amplitude[voice];
            }
            
            output[sample] = sum;
        }
    }
};
```

#### Pre-fetching and Branch Prediction

```cpp
/**
 * Optimize for cache and branch prediction
 */
void optimizedProcessing(float* data, int numSamples) {
    // 1. Align data for SIMD (16-byte boundary for SSE, 32 for AVX)
    alignas(32) float alignedBuffer[1024];
    
    // 2. Unroll loops to reduce branch prediction misses
    int i = 0;
    for (; i <= numSamples - 8; i += 8) {
        // Process 8 samples - predictable branch pattern
        processSample(data[i]);
        processSample(data[i+1]);
        processSample(data[i+2]);
        processSample(data[i+3]);
        processSample(data[i+4]);
        processSample(data[i+5]);
        processSample(data[i+6]);
        processSample(data[i+7]);
    }
    
    // Handle remainder (less critical path)
    for (; i < numSamples; ++i) {
        processSample(data[i]);
    }
}
```

### 3.3 Multi-threading Strategies

#### Thread Pool with Work Stealing

```cpp
/**
 * Track-level parallel processing
 * Each track processes in parallel, then mix
 */
class ParallelTrackProcessor {
    juce::ThreadPool threadPool_{std::thread::hardware_concurrency()};
    
public:
    void processAllTracks(const std::vector<Track*>& tracks,
                         juce::AudioBuffer<float>& output,
                         juce::MidiBuffer& midi) {
        
        std::vector<std::future<void>> futures;
        std::vector<juce::AudioBuffer<float>> trackOutputs;
        
        // Allocate output buffers for each track
        trackOutputs.resize(tracks.size());
        for (size_t i = 0; i < tracks.size(); ++i) {
            trackOutputs[i].setSize(output.getNumChannels(), 
                                   output.getNumSamples());
            trackOutputs[i].clear();
        }
        
        // Launch parallel track processing
        for (size_t i = 0; i < tracks.size(); ++i) {
            futures.push_back(threadPool_.addJob([&, i]() {
                tracks[i]->processBlock(trackOutputs[i], midi);
            }));
        }
        
        // Wait for all tracks
        for (auto& f : futures) {
            f.wait();
        }
        
        // Mix all track outputs (single thread - cache-friendly)
        for (auto& trackOut : trackOutputs) {
            for (int ch = 0; ch < output.getNumChannels(); ++ch) {
                juce::FloatVectorOperations::add(
                    output.getWritePointer(ch),
                    trackOut.getReadPointer(ch),
                    output.getNumSamples()
                );
            }
        }
    }
};
```

#### Lock-Free Work Queue

```cpp
/**
 * Lock-free task queue for audio work distribution
 * Based on moodycamel::ConcurrentQueue or similar
 */
template<typename T>
class LockFreeWorkQueue {
    struct Node {
        T data;
        std::atomic<Node*> next{nullptr};
    };
    
    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};
    
public:
    void push(T item) {
        auto* node = new Node{std::move(item), nullptr};
        
        Node* prevTail = tail_.exchange(node, std::memory_order_acq_rel);
        if (prevTail) {
            prevTail->next.store(node, std::memory_order_release);
        } else {
            head_.store(node, std::memory_order_release);
        }
    }
    
    bool pop(T& item) {
        Node* head = head_.load(std::memory_order_acquire);
        if (!head) return false;
        
        Node* next = head->next.load(std::memory_order_acquire);
        head_.store(next, std::memory_order_release);
        
        item = std::move(head->data);
        delete head;
        return true;
    }
};
```

### 3.4 Memory Layout for Real-time Audio

```cpp
/**
 * NUMA-aware memory allocation for large systems
 * Prevents cross-socket memory access penalties
 */
class NUMAAudioBuffer {
#if defined(__linux__)
    void* allocateNuma(size_t size, int node) {
        void* ptr = numa_alloc_onnode(size, node);
        if (!ptr) {
            ptr = std::aligned_alloc(64, size); // Fallback
        }
        return ptr;
    }
#endif
    
public:
    /**
     * Allocate audio buffers on specific NUMA node
     * Match allocation to thread's physical core
     */
    static float* allocateChannel(size_t numSamples, int preferredNode = 0) {
        const size_t bytes = numSamples * sizeof(float);
        
        // Align to cache line boundary (64 bytes) to prevent false sharing
        const size_t alignedBytes = (bytes + 63) & ~63;
        
#if defined(__linux__)
        return static_cast<float*>(numa_alloc_onnode(alignedBytes, preferredNode));
#else
        return static_cast<float*>(std::aligned_alloc(64, alignedBytes));
#endif
    }
};

/**
 * Pool allocator for frequent small allocations
 */
class AudioBufferPool {
    struct PoolBlock {
        alignas(64) std::array<float, 1024> data;  // 4KB per block
        std::atomic<bool> inUse{false};
    };
    
    // Separate pools by size class to reduce fragmentation
    std::array<PoolBlock, 256> smallBlocks_;   // 1K samples
    std::array<PoolBlock, 64> mediumBlocks_;   // 4K samples
    std::array<PoolBlock, 16> largeBlocks_;    // 16K samples
    
public:
    float* acquire(size_t numSamples) {
        if (numSamples <= 1024) {
            return acquireFromPool(smallBlocks_);
        } else if (numSamples <= 4096) {
            return acquireFromPool(mediumBlocks_);
        } else if (numSamples <= 16384) {
            return acquireFromPool(largeBlocks_);
        }
        return nullptr; // Too large - use heap (non-RT path)
    }
    
    template<size_t N>
    float* acquireFromPool(std::array<PoolBlock, N>& pool) {
        for (auto& block : pool) {
            bool expected = false;
            if (block.inUse.compare_exchange_strong(
                expected, true, std::memory_order_acquire)) {
                return block.data.data();
            }
        }
        return nullptr; // Pool exhausted
    }
};
```

---

## 4. Modern C++ in Audio

### 4.1 C++20 Features for Audio

#### Concepts for DSP Interfaces

```cpp
/**
 * C++20 concepts for compile-time interface checking
 * Ensures processors meet requirements at compile time
 */
template<typename T>
concept AudioProcessor = requires(T t, juce::AudioBuffer<float>& buffer, 
                                   juce::MidiBuffer& midi) {
    { t.prepareToPlay(44100.0, 512) } -> std::same_as<void>;
    { t.processBlock(buffer, midi) } -> std::same_as<void>;
    { t.releaseResources() } -> std::same_as<void>;
    { t.getLatencySamples() } -> std::convertible_to<int>;
};

/**
 * Use concept to constrain template parameters
 */
template<AudioProcessor Proc>
class ProcessingChain {
    std::vector<std::unique_ptr<Proc>> processors_;
    
public:
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi) {
        for (auto& proc : processors_) {
            proc->processBlock(buffer, midi);
        }
    }
};
```

#### Designated Initializers for Clarity

```cpp
/**
 * C++20 designated initializers for parameter structs
 * Clear, self-documenting code
 */
struct FilterParameters {
    float cutoff = 1000.0f;
    float resonance = 0.707f;
    FilterType type = FilterType::LowPass;
    bool enabled = true;
};

// Clear initialization at call site
void setupFilter() {
    configureFilter({
        .cutoff = 2000.0f,
        .resonance = 0.8f,
        .type = FilterType::HighPass,
        .enabled = true
    });
}
```

#### consteval for Compile-Time Constants

```cpp
/**
 * consteval ensures function is evaluated at compile time
 * No runtime cost for lookup tables
 */
consteval float computeSineTableEntry(int index, int tableSize) {
    constexpr float PI = 3.14159265359f;
    return std::sin(2.0f * PI * index / tableSize);
}

// Compile-time generated sine table
template<int N>
consteval auto generateSineTable() {
    std::array<float, N> table{};
    for (int i = 0; i < N; ++i) {
        table[i] = computeSineTableEntry(i, N);
    }
    return table;
}

constexpr auto sineTable = generateSineTable<1024>();
```

### 4.2 Smart Pointer Usage in Real-time Contexts

```cpp
/**
 * Smart pointer guidelines for real-time audio
 */

// ❌ BAD: shared_ptr in audio thread (atomic refcount)
void badProcess(std::shared_ptr<Filter> filter, float* data, int n) {
    for (int i = 0; i < n; ++i) {
        data[i] = filter->process(data[i]); // Atomic inc/dec on copy!
    }
}

// ✅ GOOD: raw pointer or reference in audio thread
void goodProcess(Filter& filter, float* data, int n) {
    for (int i = 0; i < n; ++i) {
        data[i] = filter.process(data[i]);
    }
}

// ✅ GOOD: unique_ptr for exclusive ownership
class Track {
    std::unique_ptr<Filter> filter_;  // Single owner
    
public:
    void processBlock(juce::AudioBuffer<float>& buffer) {
        if (filter_) {  // Check is cheap
            filter_->process(buffer);  // Raw pointer access
        }
    }
    
    void setFilter(std::unique_ptr<Filter> newFilter) {
        // Swap happens on message thread, not audio thread
        filter_ = std::move(newFilter);
    }
};

// ✅ GOOD: atomic shared_ptr for thread-safe swaps
class SafeFilterSwap {
    std::shared_ptr<Filter> filter_;
    std::atomic<std::shared_ptr<Filter>*> atomicFilter_{&filter_};
    
public:
    void setFilter(std::shared_ptr<Filter> newFilter) {
        // Create new storage
        auto* newStorage = new std::shared_ptr<Filter>(std::move(newFilter));
        
        // Atomic exchange
        auto* oldStorage = atomicFilter_.exchange(newStorage);
        
        // Schedule deletion of old (after audio thread sees new value)
        juce::MessageManager::callAsync([oldStorage]() {
            delete oldStorage;
        });
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer) {
        // Atomic load - no locks
        auto* storage = atomicFilter_.load(std::memory_order_acquire);
        if (storage && *storage) {
            (*storage)->process(buffer);
        }
    }
};
```

### 4.3 constexpr and Compile-Time Computation

```cpp
/**
 * constexpr DSP algorithms for zero-runtime-cost coefficient calculation
 */

// Compile-time filter coefficient calculation
class ConstexprBiquad {
public:
    template<FilterType Type, float SampleRate, float Freq, float Q>
    static consteval Coefficients calculateCoefficients() {
        const float w0 = 2.0f * 3.14159265359f * Freq / SampleRate;
        const float cosw0 = std::cos(w0);
        const float sinw0 = std::sin(w0);
        const float alpha = sinw0 / (2.0f * Q);
        
        Coefficients coeffs{};
        
        if constexpr (Type == FilterType::LowPass) {
            coeffs.b0 = (1.0f - cosw0) / 2.0f;
            coeffs.b1 = 1.0f - cosw0;
            coeffs.b2 = coeffs.b0;
            coeffs.a0 = 1.0f + alpha;
            coeffs.a1 = -2.0f * cosw0;
            coeffs.a2 = 1.0f - alpha;
        }
        // ... other filter types
        
        // Normalize
        coeffs.b0 /= coeffs.a0;
        coeffs.b1 /= coeffs.a0;
        coeffs.b2 /= coeffs.a0;
        coeffs.a1 /= coeffs.a0;
        coeffs.a2 /= coeffs.a0;
        
        return coeffs;
    }
};

// Usage: coefficients computed at compile time
constexpr auto lowPassCoeffs = ConstexprBiquad::calculateCoefficients<
    FilterType::LowPass, 48000.0f, 1000.0f, 0.707f
>();

// Runtime version for variable parameters
auto runtimeCoeffs = ConstexprBiquad::calculateCoefficients(
    FilterType::LowPass, sampleRate, cutoff, q
);
```

---

## 5. Testing & Quality Assurance

### 5.1 Testing Real-time Audio Code

#### Deterministic Testing with Mock Time

```cpp
/**
 * Deterministic testing of real-time code
 * Eliminates timing-related test flakiness
 */
class DeterministicAudioClock {
    mutable std::atomic<int64_t> sampleCount_{0};
    double sampleRate_ = 44100.0;
    
public:
    void advance(int numSamples) {
        sampleCount_ += numSamples;
    }
    
    double getTimeSeconds() const {
        return sampleCount_.load() / sampleRate_;
    }
    
    int64_t getSampleCount() const {
        return sampleCount_.load();
    }
};

class TestableProcessor {
    DeterministicAudioClock& clock_;
    
public:
    void processBlock(juce::AudioBuffer<float>& buffer) {
        // Use clock_ instead of system time
        auto currentTime = clock_.getTimeSeconds();
        
        // ... process ...
        
        clock_.advance(buffer.getNumSamples());
    }
};

// Test
TEST(ProcessorTest, ProcessesCorrectly) {
    DeterministicAudioClock clock;
    TestableProcessor proc(clock);
    
    juce::AudioBuffer<float> buffer(2, 512);
    
    // Deterministic - always same result
    proc.processBlock(buffer);
    EXPECT_FLOAT_EQ(clock.getTimeSeconds(), 512.0 / 44100.0);
}
```

#### Property-Based Testing for DSP

```cpp
/**
 * Property-based testing for DSP invariants
 * Tests mathematical properties rather than specific values
 */
TEST(DSPProperties, GainPreservesZeroCrossings) {
    // Property: Applying gain should preserve zero crossings
    juce::AudioBuffer<float> buffer(1, 1024);
    fillWithSine(buffer, 440.0f, 44100.0);
    
    // Count zero crossings before
    int zerosBefore = countZeroCrossings(buffer);
    
    // Apply gain
    applyGain(buffer, 0.5f);
    
    // Count after
    int zerosAfter = countZeroCrossings(buffer);
    
    EXPECT_EQ(zerosBefore, zerosAfter);
}

TEST(DSPProperties, ParallelProcessingEquivalent) {
    // Property: Parallel and serial processing produce same result
    juce::AudioBuffer<float> input(2, 1024);
    fillWithNoise(input);
    
    auto serialResult = processSerial(input);
    auto parallelResult = processParallel(input);
    
    // Allow for minor floating-point differences
    expectBuffersSimilar(serialResult, parallelResult, 1e-6f);
}
```

### 5.2 Static Analysis for Audio

#### Real-time Safety Annotations

```cpp
/**
 * Clang annotations for real-time safety checking
 * Can be checked with custom clang-tidy checks
 */

#define RT_SAFE [[clang::annotate("rt_safe")]]
#define NON_RT [[clang::annotate("non_rt")]]
#define AUDIO_THREAD_ONLY [[clang::annotate("audio_thread_only")]]
#define UI_THREAD_ONLY [[clang::annotate("ui_thread_only")]]

class AudioEngine {
    RT_SAFE
    void processBlock(juce::AudioBuffer<float>& buffer) AUDIO_THREAD_ONLY {
        // Compiler/analyzer verifies no non-rt calls
        applyGain(buffer, 0.5f);
    }
    
    NON_RT
    void loadPlugin(const juce::String& path) UI_THREAD_ONLY {
        // Allowed to allocate, use mutexes, etc.
        plugin_ = pluginLoader.load(path);
    }
};
```

#### Custom Clang-Tidy Check (Conceptual)

```cpp
/**
 * Example clang-tidy check for real-time violations
 * This is a conceptual implementation
 */
class RealTimeSafetyCheck : public clang::tidy::ClangTidyCheck {
public:
    void check(const clang::Decl* decl) override {
        // Find functions marked RT_SAFE
        if (hasAnnotation(decl, "rt_safe")) {
            // Traverse AST looking for violations
            traverse(decl, [](const clang::Stmt* stmt) {
                // Flag malloc/new
                if (isMemoryAllocation(stmt)) {
                    reportViolation(stmt, "Allocation in RT context");
                }
                // Flag mutex locks
                if (isMutexLock(stmt)) {
                    reportViolation(stmt, "Lock in RT context");
                }
                // Flag syscalls
                if (isSystemCall(stmt)) {
                    reportViolation(stmt, "Syscall in RT context");
                }
            });
        }
    }
};
```

### 5.3 Measuring and Preventing Xruns

#### Xrun Detection and Logging

```cpp
/**
 * Xrun (buffer underrun/overrun) detection and analysis
 */
class XrunAnalyzer {
    struct XrunEvent {
        double timestamp;
        int bufferSize;
        double callbackDuration;
        double deadline;
        juce::String context;
    };
    
    std::vector<XrunEvent> xrunHistory_;
    std::mutex historyMutex_;  // Only accessed from message thread
    
    std::atomic<int> xrunCount_{0};
    std::atomic<double> maxCallbackTime_{0.0};
    
public:
    void measureCallback(const juce::String& name,
                        std::function<void()> callback,
                        double deadlineMs) {
        auto start = std::chrono::high_resolution_clock::now();
        
        callback();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Track max time (relaxed - called from single audio thread)
        double currentMax = maxCallbackTime_.load(std::memory_order_relaxed);
        while (duration > currentMax && 
               !maxCallbackTime_.compare_exchange_weak(
                   currentMax, duration, std::memory_order_relaxed)) {
            // CAS loop
        }
        
        // Detect xrun
        if (duration > deadlineMs) {
            xrunCount_.fetch_add(1, std::memory_order_relaxed);
            
            // Queue for logging (lock-free queue to message thread)
            XrunEvent event{
                juce::Time::getMillisecondCounterHiRes() / 1000.0,
                0, // buffer size
                duration,
                deadlineMs,
                name
            };
            queueForLogging(event);
        }
    }
    
    void printReport() {
        std::cout << "=== Xrun Report ===" << std::endl;
        std::cout << "Total xruns: " << xrunCount_.load() << std::endl;
        std::cout << "Max callback time: " << maxCallbackTime_.load() << " ms" << std::endl;
        
        std::lock_guard<std::mutex> lock(historyMutex_);
        for (const auto& event : xrunHistory_) {
            std::cout << event.timestamp << ": " << event.context 
                     << " took " << event.callbackDuration 
                     << " ms (deadline: " << event.deadline << " ms)" << std::endl;
        }
    }
};
```

#### Worst-Case Execution Time (WCET) Analysis

```cpp
/**
 * WCET measurement for real-time verification
 */
class WCETAnalyzer {
    struct Measurement {
        double minTime = std::numeric_limits<double>::max();
        double maxTime = 0.0;
        double totalTime = 0.0;
        int64_t callCount = 0;
    };
    
    std::unordered_map<std::string, Measurement> measurements_;
    std::mutex mutex_;
    
public:
    class ScopedTimer {
        WCETAnalyzer& analyzer_;
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_;
        
    public:
        ScopedTimer(WCETAnalyzer& a, std::string name)
            : analyzer_(a), name_(std::move(name)),
              start_(std::chrono::high_resolution_clock::now()) {}
        
        ~ScopedTimer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::micro>(end - start_).count();
            analyzer_.recordMeasurement(name_, duration);
        }
    };
    
    void recordMeasurement(const std::string& name, double microseconds) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& m = measurements_[name];
        m.minTime = std::min(m.minTime, microseconds);
        m.maxTime = std::max(m.maxTime, microseconds);
        m.totalTime += microseconds;
        m.callCount++;
    }
    
    void printReport() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "=== WCET Report ===" << std::endl;
        for (const auto& [name, m] : measurements_) {
            double avg = m.callCount > 0 ? m.totalTime / m.callCount : 0.0;
            std::cout << name << ":" << std::endl;
            std::cout << "  Min: " << m.minTime << " μs" << std::endl;
            std::cout << "  Avg: " << avg << " μs" << std::endl;
            std::cout << "  Max: " << m.maxTime << " μs (WCET)" << std::endl;
            std::cout << "  Calls: " << m.callCount << std::endl;
        }
    }
};

// Usage in processBlock
void processBlock(juce::AudioBuffer<float>& buffer) {
    WCETAnalyzer::ScopedTimer timer(wcetAnalyzer, "processBlock");
    
    {
        WCETAnalyzer::ScopedTimer t2(wcetAnalyzer, "filterSection");
        filter_.process(buffer);
    }
    
    {
        WCETAnalyzer::ScopedTimer t3(wcetAnalyzer, "gainSection");
        applyGain(buffer, gain_.getNextValue());
    }
}
```

---

## 6. Authoritative Sources

### Key References

1. **Dave Rowland & Fabian Renn-Giles - "Real-time 101" (ADC 2019)**
   - The definitive introduction to real-time audio programming
   - Covers priority inversion, lock-free patterns, thread synchronization
   - Video: Available from Audio Developer Conference

2. **Tracktion Graph Library (ADC 2020)**
   - Open-source graph-based audio processing
   - Parallel execution, delay compensation
   - GitHub: `tracktion/tracktion_graph`

3. **Nathan Blair - "The Template Plugin" Thesis (2023)**
   - Comprehensive plugin development guide
   - Real-time safety, parameter management, testing
   - URL: https://nthnblair.com/thesis/

4. **JUCE Documentation**
   - `juce::FloatVectorOperations` for SIMD
   - `juce::AbstractFifo` for lock-free queues
   - `juce::AudioProcessor` lifecycle
   - URL: https://docs.juce.com

5. **Cycling '74 Max/MSP SDK**
   - Real-time audio programming patterns
   - Threading model documentation

6. **Steinberg VST3 SDK Documentation**
   - Plugin hosting best practices
   - Thread safety guidelines
   - URL: https://steinbergmedia.github.io/vst3_dev_portal/

7. **CLAP Plugin API**
   - Modern plugin API with thread annotations
   - URL: https://cleveraudio.org/

### Tools for Real-time Analysis

| Tool | Purpose | URL |
|------|---------|-----|
| pluginval | Plugin validation testing | github.com/Tracktion/pluginval |
| RtAudio/RtMidi | Cross-platform audio/MIDI | github.com/thestk/rtaudio |
| Tracy Profiler | Real-time profiling | github.com/wolfpld/tracy |
| Perfetto | System-wide tracing | ui.perfetto.dev |
| clang-tidy | Static analysis | llvm.org |
| Valgrind (Helgrind) | Thread error detection | valgrind.org |

### Community Resources

- **The Audio Programmer** (YouTube/Discord)
- **Audio Developer Conference (ADC)** - Annual conference
- **JUCE Forum** - forums.juce.com
- **KVR Audio Developer Forum** - kvraudio.com/forum

---

## Summary Checklist

### For New Code

- [ ] Pre-allocate all audio buffers in constructor
- [ ] No `new`/`delete` in `processBlock`
- [ ] No mutex locks on audio thread
- [ ] Use `std::atomic` with appropriate `memory_order`
- [ ] Verify SIMD alignment (16-byte for SSE, 32 for AVX)
- [ ] Profile worst-case execution time
- [ ] Test with varying buffer sizes
- [ ] Test with CPU stress (e.g., `stress -c 16`)

### For Code Review

- [ ] Check for hidden allocations (e.g., `std::function` captures)
- [ ] Verify lock-free queue implementations
- [ ] Confirm proper memory ordering semantics
- [ ] Review for priority inversion risks
- [ ] Validate SIMD fallbacks for non-aligned data
- [ ] Check thread annotations and safety comments

### For Testing

- [ ] Unit tests for all DSP algorithms
- [ ] Property-based tests for invariants
- [ ] Deterministic timing tests
- [ ] Xrun detection in CI
- [ ] Memory sanitizer runs
- [ ] Thread sanitizer runs
- [ ] Stress testing at minimum buffer size

---

*This document is a living resource. Update with new findings from ADC, JUCE updates, and project experience.*
