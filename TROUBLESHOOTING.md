# Zenith DAW Troubleshooting Guide

This comprehensive troubleshooting guide helps you resolve common issues when developing and using Zenith DAW. From build failures to runtime problems, we've got you covered!

## 🚨 Quick Reference - Most Common Issues

| Symptom | Solution | Status |
|---------|----------|--------|
| **CMake can't find JUCE** | Delete build/ and reconfigure | 🔴 Common |
| **Audio device initialization fails** | Close other audio apps, check permissions | 🔴 Common |
| **Build fails with linker errors** | Clean rebuild with `cmake --build build --clean-first` | 🟡 Medium |
| **GUI doesn't render** | Check Skia configuration, GPU drivers | 🟡 Medium |
| **Audio clicks/pops** | Check buffer size, thread safety | 🟢 Advanced |

---

## 🔧 Build Issues

### CMake Configuration Problems

#### Error: "Could not find JUCE"
```bash
# Problem: CMake can't locate JUCE framework
# Solution: Clean build and reconfigure

rm -rf build/
mkdir build && cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
```

#### Error: "No CMAKE_CXX_COMPILER found"
```bash
# Problem: C++ compiler not installed or not in PATH
# Solution: Install compiler

# Ubuntu/Debian
sudo apt install build-essential

# macOS
xcode-select --install

# Windows
# Install Visual Studio 2022 with C++ workload
```

#### Error: "CMake Error: The source directory does not exist"
```bash
# Problem: Wrong working directory
# Solution: Navigate to project root

pwd  # Should show /path/to/zenith-daw
ls   # Should show CMakeLists.txt
```

### Build Failures

#### Error: "LNK1104: cannot open file 'juce_*.lib'"
```bash
# Problem: Linker can't find JUCE libraries
# Solution: Clean rebuild

cmake --build build --clean-first --config Release
```

#### Error: "undefined reference to `juce::Something`"
```bash
# Problem: Missing JUCE module in CMakeLists.txt
# Solution: Check module requirements

# Check CMakeLists.txt in your component
find . -name "CMakeLists.txt" -exec grep -l "juce_" {} \;
```

#### Error: "fatal error: JuceHeader.h: No such file or directory"
```cpp
// Problem: JUCE headers not found
// Solution: Check include paths

// In your .cpp file
#include <JuceHeader.h>  // Should be at top of includes

// Check that JUCE is properly configured
cmake --build build --verbose 2>&1 | grep JUCE
```

### Cross-Platform Build Issues

#### Windows Specific Issues
```powershell
# Problem: Visual Studio can't open CMake project
# Solution: Use CMake GUI or open folder

# Method 1: CMake GUI
cmake-gui.exe
  - Where is the source code: C:\path\to\zenith-daw
  - Where to build the binaries: C:\path\to\zenith-daw\build
  - Configure → Generate → Open Project

# Method 2: Open folder in VS Code
# Open CMake: F1 → CMake: Open
```

#### macOS Specific Issues
```bash
# Problem: "Xcode-select: error: command line tools are not installed"
# Solution: Install command line tools

xcode-select --install

# If issues persist, reset Xcode
sudo xcode-select -switch /Library/Developer/CommandLineTools
```

#### Linux Specific Issues
```bash
# Problem: "libasound2-dev: Depends: libasound2 (>= 1.0.16) but 1.0.15 is to be installed"
# Solution: Update package list and install dependencies

sudo apt update
sudo apt install --fix-broken
sudo apt install build-essential cmake ninja-build \
                libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
                libfreetype6-dev libx11-dev libxinerama-dev libxext-dev \
                libxrandr-dev libxcursor-dev libwebkit2gtk-4.0-dev \
                libglu1-mesa-dev mesa-common-dev
```

---

## 🎵 Audio Issues

### Device Initialization Problems

#### Error: "Audio device initialization failed"
```cpp
// Problem: Can't initialize audio output
// Solution: Check audio devices and permissions

// In your code, check available devices
auto* deviceManager = juce::AudioDeviceManager::getInstance();
auto devices = deviceManager->getAvailableDeviceTypes();

for (auto* deviceType : devices)
{
    DBG("Available device: " << deviceType->getTypeName());
    auto deviceNames = deviceType->getDeviceNames();
    for (auto& name : deviceNames)
    {
        DBG("  - " << name);
    }
}
```

#### Error: "Device not available"
```bash
# Problem: Audio device in use by another application
# Solution: Close other audio apps

# Linux: Check audio processes
ps aux | grep -E "(pulseaudio|jack|alsad)"

# Close PulseAudio (if safe)
pulseaudio -k

# macOS: Check Audio MIDI Setup
# Applications → Utilities → Audio MIDI Setup

# Windows: Check Task Manager for audio apps
```

### Audio Thread Problems

#### Clicks and Pops in Audio
```cpp
// Problem: Audio buffer underruns or timing issues
// Solution: Optimize audio thread and check buffer sizes

class OptimizedAudioProcessor {
public:
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock)
    {
        // Use appropriate buffer size
        bufferSize_ = juce::jlimit(64, 2048, estimatedSamplesPerBlock);

        // Pre-allocate all buffers
        tempBuffer_.setSize(2, bufferSize_);

        // Set sample rate
        sampleRate_ = sampleRate;
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        jassert(buffer.getNumSamples() <= bufferSize_);

        // Process audio - NO allocations, NO locks
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            processChannel(buffer.getWritePointer(channel), buffer.getNumSamples());
        }
    }

private:
    void processChannel(float* channelData, int numSamples)
    {
        // Optimized processing with SIMD
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Audio processing here - NO memory operations
            channelData[sample] = applyEffects(channelData[sample]);
        }
    }

    int bufferSize_ = 512;
    double sampleRate_ = 44100.0;
    juce::AudioBuffer<float> tempBuffer_;
};
```

#### Error: "Audio callback took too long"
```cpp
// Problem: Audio processing exceeds buffer time
// Solution: Optimize and profile

// Before: Slow processing
void processSlow(float* data, int numSamples)
{
    // BAD: Allocations in audio thread
    auto* temp = new float[numSamples];  // CRASH!

    // Complex algorithm that takes too long
    for (int i = 0; i < numSamples; ++i)
    {
        data[i] = expensiveAlgorithm(data[i]);
    }

    delete[] temp;  // Double bad!
}

// After: Optimized processing
void processFast(float* data, int numSamples)
{
    // GOOD: Pre-allocated and optimized
    // Process in chunks for better cache usage
    const int chunkSize = 64;

    for (int chunk = 0; chunk < numSamples; chunk += chunkSize)
    {
        int chunkEnd = juce::jmin(chunk + chunkSize, numSamples);

        // Process chunk with SIMD
        for (int i = chunk; i < chunkEnd; ++i)
        {
            data[i] = fastAlgorithm(data[i]);
        }
    }
}
```

### MIDI Input Issues

#### MIDI Not Working
```cpp
// Problem: MIDI input not recognized
// Solution: Check MIDI device setup and enable input

class MidiProcessor : public juce::MidiInputCallback
{
public:
    void setupMidiInput(juce::AudioDeviceManager& deviceManager)
    {
        // Enable MIDI input
        deviceManager.addMidiInputCallback({}, this);

        // List available MIDI devices
        auto devices = deviceManager.getMidiInputDevices();
        for (auto& device : devices)
        {
            DBG("MIDI Device: " << device);
        }
    }

    void handleIncomingMidiMessage(juce::MidiInput* source,
                                 const juce::MidiMessage& message) override
    {
        // Process MIDI messages
        if (message.isNoteOn())
        {
            int note = message.getNoteNumber();
            float velocity = message.getFloatVelocity();

            DBG("Note On: " << note << " velocity: " << velocity);
            handleNoteOn(note, velocity);
        }
        else if (message.isNoteOff())
        {
            int note = message.getNoteNumber();
            DBG("Note Off: " << note);
            handleNoteOff(note);
        }
    }

private:
    void handleNoteOn(int note, float velocity)
    {
        // Handle note on - don't allocate!
        playNote(note, velocity);
    }
};
```

---

## 🖥️ GUI and Rendering Issues

### Skia Rendering Problems

#### GUI Not Visible or Corrupted
```cpp
// Problem: Skia components not rendering
// Solution: Check Skia configuration and OpenGL support

class SkiaComponent : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        // Check if OpenGL context is available
        if (auto* glContext = dynamic_cast<juce::OpenGLContext*>(getOpenGLContext()))
        {
            if (glContext->isAttached())
            {
                // Render with Skia
                auto surface = createSkiaSurface(g);
                if (surface)
                {
                    SkCanvas canvas(*surface);
                    renderSkiaContent(canvas);
                }
            }
            else
            {
                // Fallback to native rendering
                g.fillAll(juce::Colours::grey);
                g.drawText("OpenGL not available", getLocalBounds(),
                          juce::Justification::centred, true);
            }
        }
        else
        {
            // No OpenGL context available
            g.fillAll(juce::Colours::red);
            g.drawText("No OpenGL context", getLocalBounds(),
                      juce::Justification::centred, true);
        }
    }

private:
    std::unique_ptr<SkSurface> createSkiaSurface(juce::Graphics& g)
    {
        try
        {
            // Create Skia surface from graphics context
            auto image = g.getImageNative();
            if (image.isValid())
            {
                return SkSurface::MakeRaster(
                    SkImageInfo::Make(image.getWidth(), image.getHeight(),
                                     kN32_SkColorType, kPremul_SkAlphaType),
                    nullptr);
            }
        }
        catch (const std::exception& e)
        {
            DBG("Skia error: " << e.what());
        }

        return nullptr;
    }
};
```

#### Component Flickering
```cpp
// Problem: UI flickering during updates
// Solution: Optimize rendering and use dirty regions

class FlickerFreeComponent : public juce::Component
{
public:
    FlickerFreeComponent()
    {
        // Enable double buffering
        setBufferedToImage(true);

        // Set paint method
        setPaintingIsUnclipped(true);
    }

    void resized() override
    {
        // Only repaint when necessary
        repaint(getLocalBounds());
    }

    void paint(juce::Graphics& g) override
    {
        // Only paint visible area
        auto bounds = getLocalBounds();

        // Clear background efficiently
        g.reduceClipRegion(bounds);
        g.fillAll(juce::Colours::black);

        // Only paint what's visible
        paintVisibleContent(g, bounds);
    }

    void paintVisibleContent(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        // Only paint visible components
        for (auto* child : getChildren())
        {
            auto childBounds = child->getBounds();

            // Check if child is visible
            if (bounds.intersects(childBounds))
            {
                g.saveState();
                g.reduceClipRegion(childBounds);
                child->paintWithin(g, childBounds.getX(), childBounds.getY());
                g.restoreState();
            }
        }
    }
};
```

### Layout and Sizing Issues

#### Component Layout Problems
```cpp
// Problem: Components not resizing correctly
// Solution: Implement proper layout management

class ResponsiveLayoutComponent : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Dynamic layout based on size
        if (bounds.getWidth() < 800)
        {
            // Mobile layout
            applyMobileLayout(bounds);
        }
        else
        {
            // Desktop layout
            applyDesktopLayout(bounds);
        }
    }

private:
    void applyMobileLayout(juce::Rectangle<int> bounds)
    {
        // Stack components vertically
        int y = bounds.getY();

        header_.setBounds(bounds.withHeight(60));
        y += 60;

        content_.setBounds(bounds.withTrimmedTop(60).withTrimmedBottom(100));
        y += content_.getHeight();

        footer_.setBounds(bounds.withTrimmedTop(bounds.getHeight() - 100));
    }

    void applyDesktopLayout(juce::Rectangle<int> bounds)
    {
        // Side-by-side layout
        auto contentBounds = bounds.reduced(20);

        sidebar_.setBounds(contentBounds.withWidth(250));
        mainContent_.setBounds(contentBounds.withTrimmedLeft(250));
    }

    juce::Component header_;
    juce::Component sidebar_;
    juce::Component mainContent_;
    juce::Component footer_;
};
```

---

## 🐛 Runtime Issues

### Crash Issues

#### Segmentation Fault (Linux/macOS)
```bash
# Problem: Application crashes with segmentation fault
# Solution: Use debugger to find crash location

# Run with debugger
gdb ./build/Zenith
(gdb) run
# Wait for crash
(gdb) backtrace
(gdb) info locals
(gdb) quit

# Or run with core dump
ulimit -c unlimited
./build/Zenith
gdb ./build/Zenith core.0
```

#### Access Violation (Windows)
```powershell
# Problem: Application crashes with access violation
# Solution: Use Visual Studio debugger

# Run with debugging
Start-Process "devenv.exe" -ArgumentList "Zenith.sln /debug"

# Or use command line
devenv /debugexe build\Release\Zenith.exe

# In debugger, check call stack and variables
```

#### Memory Issues
```cpp
// Problem: Memory leaks or corruption
// Solution: Use memory sanitizers

// Build with address sanitizer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"

# Run with leak detection
./build/Zenith

# Check for allocations
grep -r "new\|delete\|malloc" Source/ | head -10
```

### Performance Issues

#### High CPU Usage
```cpp
// Problem: High CPU usage during audio processing
// Solution: Optimize and profile

class OptimizedAudioProcessor {
public:
    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        // Profile performance
        auto start = juce::Time::getHighResolutionTicks();

        // Optimize processing
        processOptimized(buffer);

        auto end = juce::Time::getHighResolutionTicks();
        auto duration = end - start;

        // Log if too slow
        if (duration > maxProcessingTime_)
        {
            DBG("Slow processing: " << duration << " ticks");
            maxProcessingTime_ = duration;
        }
    }

private:
    void processOptimized(juce::AudioBuffer<float>& buffer)
    {
        // Use SIMD where possible
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* data = buffer.getWritePointer(channel);

            // Process in chunks for better cache
            const int chunkSize = 64;
            for (int i = 0; i < buffer.getNumSamples(); i += chunkSize)
            {
                int end = juce::jmin(i + chunkSize, buffer.getNumSamples());

                // Process chunk
                for (int j = i; j < end; ++j)
                {
                    data[j] = processSample(data[j]);
                }
            }
        }
    }

    int maxProcessingTime_ = 0;
};
```

#### UI Freezing
```cpp
// Problem: UI becomes unresponsive
// Solution: Move heavy operations to background threads

class ResponsiveUI : public juce::Component
{
public:
    void loadHeavyResource()
    {
        // Don't block UI thread
        auto* thread = new juce::Thread("Resource Loader");
        thread->start([this]() {
            // Heavy operation in background
            auto resource = loadResource();

            // Update UI on message thread
            juce::MessageManager::callAsync([this, resource]() {
                loadedResource_ = resource;
                updateUI();
            });

            delete thread;  // Clean up
        });
    }

    void paint(juce::Graphics& g) override
    {
        // Fast painting only
        g.fillAll(juce::Colours::white);

        if (isLoading_)
        {
            g.drawText("Loading...", getLocalBounds(), juce::Justification::centred);
        }
        else
        {
            g.drawText("Ready", getLocalBounds(), juce::Justification::centred);
        }
    }

private:
    std::unique_ptr<LoadedResource> loadedResource_;
    bool isLoading_ = false;
};
```

---

## 🔍 Debugging Techniques

### Logging for Debugging

#### Debug Logging System
```cpp
class ZenithDebugLogger : public juce::Logger
{
public:
    static void log(const juce::String& message)
    {
        // Log to file
        juce::File logFile = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW")
            .getChildFile("debug.log");

        if (auto logStream = logFile.createOutputStream(true))
        {
            logStream << juce::Time::getCurrentTime().toString(true, true)
                      << " - " << message << juce::newLine;
        }

        // Also log to console in debug builds
        #if DEBUG
        std::cout << "[DEBUG] " << message.toStdString() << std::endl;
        #endif
    }

    void logMessage(const juce::String& message) override
    {
        log(message);
    }
};

// Usage
ZenithDebugLogger::log("Audio buffer size: " + juce::String(bufferSize));
ZenithDebugLogger::log("Error loading plugin: " + pluginName);
```

#### Conditional Debugging
```cpp
// Debug macros
#ifdef DEBUG
    #define DBG(x) juce::Logger::writeToLog(x)
#else
    #define DBG(x) ((void)0)
#endif

// Usage
DBG("Processing audio buffer with " << numSamples << " samples");

// Performance debugging
class ScopedPerformanceTimer
{
public:
    ScopedPerformanceTimer(const juce::String& operation)
        : operation_(operation), startTime_(juce::Time::getMillisecondCounterHiRes())
    {
        DBG("Starting: " << operation_);
    }

    ~ScopedPerformanceTimer()
    {
        auto duration = juce::Time::getMillisecondCounterHiRes() - startTime_;
        DBG("Completed: " << operation_ << " in " << duration << "ms");
    }

private:
    juce::String operation_;
    double startTime_;
};

// Usage
{
    ScopedPerformanceTimer timer("Audio Processing");
    // ... audio processing code ...
}
```

### Memory Debugging

#### Memory Leak Detection
```cpp
// Memory tracking class
class MemoryTracker
{
public:
    static void allocate(size_t size, const juce::String& context)
    {
        allocations_[context] += size;
        totalAllocated_ += size;

        DBG("Allocated " << size << " bytes for " << context);
    }

    static void deallocate(size_t size, const juce::String& context)
    {
        allocations_[context] -= size;
        totalDeallocated_ += size;

        DBG("Deallocated " << size << " bytes for " << context);
    }

    static void report()
    {
        DBG("=== Memory Report ===");
        DBG("Total Allocated: " << totalAllocated_);
        DBG("Total Deallocated: " << totalDeallocated_);
        DBG("Net: " << (totalAllocated_ - totalDeallocated_));

        for (auto& allocation : allocations_)
        {
            if (allocation.value > 0)
            {
                DBG("Leak in " << allocation.key << ": " << allocation.value << " bytes");
            }
        }
    }

private:
    static juce::HashMap<juce::String, size_t> allocations_;
    static size_t totalAllocated_;
    static size_t totalDeallocated_;
};

// Usage
MemoryTracker::allocate(1024, "Audio Buffer");
// ... use memory ...
MemoryTracker::deallocate(1024, "Audio Buffer");
```

---

## 🔧 Development Environment Issues

### IDE Configuration Problems

#### Visual Studio Issues
```json
// .vs/settings.json
{
  "cmake.configureOnOpen": true,
  "cmake.configureArgs": [
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DBUILD_TESTS=ON"
  ],
  "cmake.generator": "Ninja",
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "editor.formatOnSave": true,
  "files.associations": {
    "*.h": "cpp",
    "*.cpp": "cpp"
  }
}
```

#### VS Code Issues
```bash
# Problem: CMake not working in VS Code
# Solution: Install extensions and configure

# Install extensions
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
code --install-extension ms-vscode.makefile-tools

# Select CMake kit
# Ctrl+Shift+P → CMake: Select a Kit
# Choose "Visual Studio Community 2022 Release - amd64"
```

### Git Issues

#### Authentication Problems
```bash
# Problem: Git authentication fails
# Solution: Use personal access token

# Method 1: HTTPS with token
git remote set-url origin https://YOUR_TOKEN@github.com/your-username/zenith-daw.git

# Method 2: SSH
git remote set-url origin git@github.com:your-username/zenith-daw.git

# Method 3: Git credential manager
git config --global credential.helper store
```

#### Merge Conflicts
```bash
# Problem: Git merge conflicts
# Solution: Resolve conflicts and continue

# See conflicts
git status

# Edit conflicting files
#<<<<<<< HEAD
// your changes
//=======
// their changes
//>>>>>>> branch-name

# Mark resolved
git add resolved-file.cpp

# Continue merge
git commit
```

---

## 🚀 Performance Optimization

### Build Performance

#### Faster CMake Configuration
```bash
# Use pre-compiled binaries
cmake -S . -B build -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS="-march=native -O3"

# Enable parallel builds
cmake --build build -j$(nproc)
```

### Runtime Performance

#### Audio Optimization
```cpp
class OptimizedAudioEngine
{
public:
    void processAudio(juce::AudioBuffer<float>& buffer)
    {
        // Use SIMD instructions
        processSIMD(buffer);

        // Cache-friendly processing
        processCacheFriendly(buffer);

        // Branch prediction optimization
        processWithBranchPrediction(buffer);
    }

private:
    void processSIMD(juce::AudioBuffer<float>& buffer)
    {
        // Use SIMD for better performance
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* data = buffer.getWritePointer(channel);

            #ifdef __SSE__
            // Process 4 samples at once with SSE
            for (int i = 0; i < buffer.getNumSamples(); i += 4)
            {
                __m128 samples = _mm_load_ps(&data[i]);
                __m128 processed = _mm_mul_ps(samples, gain_);
                _mm_store_ps(&data[i], processed);
            }
            #endif
        }
    }

    void processCacheFriendly(juce::AudioBuffer<float>& buffer)
    {
        // Process channels separately for better cache usage
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* data = buffer.getWritePointer(channel);

            // Process in larger chunks
            const int chunkSize = 256;
            for (int i = 0; i < buffer.getNumSamples(); i += chunkSize)
            {
                int end = juce::jmin(i + chunkSize, buffer.getNumSamples());

                // Process chunk
                for (int j = i; j < end; ++j)
                {
                    data[j] = processSample(data[j]);
                }
            }
        }
    }

    void processWithBranchPrediction(juce::AudioBuffer<float>& buffer)
    {
        // Avoid branches in hot loops
        bool hasEffects = effectsEnabled_;

        if (hasEffects)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                auto* data = buffer.getWritePointer(channel);
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                {
                    data[sample] = applyEffects(data[sample]);
                }
            }
        }
        else
        {
            // Fast path for no effects
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                auto* data = buffer.getWritePointer(channel);
                for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                {
                    data[sample] = data[sample] * gain_;
                }
            }
        }
    }

    bool effectsEnabled_ = false;
    float gain_ = 1.0f;
};
```

---

## 📊 Monitoring and Profiling

### System Monitoring

#### Audio Performance Monitoring
```cpp
class AudioPerformanceMonitor
{
public:
    void startProcessing()
    {
        processingStart_ = juce::Time::getHighResolutionTicks();
        bufferCount_++;
    }

    void endProcessing()
    {
        auto processingEnd = juce::Time::getHighResolutionTicks();
        auto processingTime = processingEnd - processingStart_;

        // Track maximum processing time
        if (processingTime > maxProcessingTime_)
        {
            maxProcessingTime_ = processingTime;
            maxBufferCount_ = bufferCount_;
        }

        // Calculate average
        totalProcessingTime_ += processingTime;
        averageProcessingTime_ = totalProcessingTime_ / bufferCount_;

        // Check for underruns
        if (processingTime > targetProcessingTime_)
        {
            underrunCount_++;
            DBG("Underrun detected: " << processingTime << " ticks");
        }
    }

    void reportPerformance()
    {
        DBG("=== Audio Performance Report ===");
        DBG("Average processing time: " << averageProcessingTime_ << " ticks");
        DBG("Max processing time: " << maxProcessingTime_ << " ticks (buffer " << maxBufferCount_ << ")");
        DBG("Underruns: " << underrunCount_);
        DBG("Target: " << targetProcessingTime_ << " ticks per buffer");
    }

private:
    int64 processingStart_ = 0;
    int64 maxProcessingTime_ = 0;
    int64 targetProcessingTime_ = 5000;  // 5ms at 48kHz
    int bufferCount_ = 0;
    int maxBufferCount_ = 0;
    int underrunCount_ = 0;
    int64 totalProcessingTime_ = 0;
    double averageProcessingTime_ = 0.0;
};
```

### Memory Usage Tracking
```cpp
class MemoryMonitor
{
public:
    static void start()
    {
        initialMemory_ = getCurrentMemoryUsage();
        peakMemory_ = initialMemory_;
    }

    static void checkpoint(const juce::String& name)
    {
        size_t current = getCurrentMemoryUsage();
        size_t used = current - initialMemory_;

        if (current > peakMemory_)
        {
            peakMemory_ = current;
        }

        DBG("Memory checkpoint: " << name
            << " - Current: " << formatBytes(current)
            << " - Used: " << formatBytes(used)
            << " - Peak: " << formatBytes(peakMemory_));
    }

private:
    static size_t getCurrentMemoryUsage()
    {
        #if defined(__linux__)
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        return usage.ru_maxrss * 1024;  // Convert to bytes
        #elif defined(__APPLE__)
        struct task_basic_info info;
        mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
            return info.resident_size;
        #endif
        return 0;
    }

    static juce::String formatBytes(size_t bytes)
    {
        const char* units[] = {"B", "KB", "MB", "GB"};
        size_t unit = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024 && unit < 3)
        {
            size /= 1024;
            unit++;
        }

        return juce::String(size, 1) + " " + units[unit];
    }

    static size_t initialMemory_;
    static size_t peakMemory_;
};
```

---

## 🎯 Solutions by Symptom

### Won't Build
```bash
# 1. Clean and reconfigure
rm -rf build/
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# 2. Install missing dependencies
# Follow OS-specific instructions above

# 3. Check CMake version
cmake --version

# 4. Update dependencies
git pull upstream main
```

### Audio Not Working
```cpp
// 1. Check device initialization
auto* deviceManager = juce::AudioDeviceManager::getInstance();
auto devices = deviceManager->getAvailableDeviceTypes();

// 2. Test with different buffer sizes
for (int bufferSize : [64, 128, 256, 512])
{
    deviceManager->initialise(2, 2, nullptr, true, "", bufferSize);
}

// 3. Check for other audio apps
// Close other DAWs, browsers, media players
```

### GUI Issues
```cpp
// 1. Check OpenGL support
if (!juce::OpenGLContext::isOpenGLAvailable())
{
    DBG("OpenGL not available!");
    // Use fallback rendering
}

// 2. Test with different graphics APIs
// Try Vulkan, Metal, or DirectX alternatives

// 3. Check component hierarchy
// Use debug renderer to visualize component tree
```

### Performance Problems
```bash
# 1. Profile with tools
# Linux: perf record ./build/Zenith && perf report
# macOS: Instruments.app
# Windows: Visual Studio Performance Profiler

# 2. Enable logging
# Set log level to debug for detailed performance info

# 3. Check audio thread priority
# Verify MMCSS is enabled on Windows
```

---

## 🆘 Getting Additional Help

### When to Ask for Help

**Ask when:**
- 🔴 Issue blocks your development
- 🔴 You've tried the solutions above
- 🔴 You need clarification on the codebase
- 🔴 You want to suggest improvements

**Don't ask when:**
- 🟡 Issue is in documentation (check first)
- 🟡 You haven't tried basic troubleshooting
- 🟡 Question is already answered in issues/discussions

### How to Ask Effectively

#### Good Issue Template
```markdown
## Issue Title: Clear and specific problem description

### Environment
- OS: [Ubuntu 22.04 / macOS 13.0 / Windows 11]
- Build: [Debug / Release]
- Version: [git hash or version number]

### Expected Behavior
[What should happen]

### Actual Behavior
[What actually happens]

### Steps to Reproduce
1. [First step]
2. [Second step]
3. [Third step]

### Error Messages
[Copy and paste error messages]

### Additional Context
[Code snippets, screenshots, or logs]
```

### Support Channels
1. **GitHub Issues**: Bug reports and feature requests
2. **GitHub Discussions**: Questions and brainstorming
3. **Discord**: Real-time chat with developers
4. **Discussions**: Community Q&A and help

---

## 🎉 Conclusion

This troubleshooting guide covers the most common issues you'll encounter while developing with Zenith DAW. Remember:

- **Start with basics** - Check environment and dependencies first
- **Use debug tools** - Profilers and loggers save time
- **Ask for help** - Community is here to support you
- **Document solutions** - Help future developers

Every challenge you overcome makes you a better audio software developer! 🎵✨

---

**Need more help?** Join our [Discord server](https://discord.gg/zenith-daw) or ask in [GitHub Discussions](https://github.com/zenith-daw/zenith/discussions)