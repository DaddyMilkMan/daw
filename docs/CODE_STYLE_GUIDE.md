# Zenith DAW Code Style Guide

Consistent code style is crucial for maintainability and collaboration in the Zenith DAW project. This guide ensures all contributors write clean, readable, and efficient code that matches our high standards.

## 🎯 Why We Have Style Guidelines

- **Readability**: Code is read more than it's written
- **Maintainability**: Consistent styles make debugging and adding features easier
- **Collaboration**: Team members can understand each other's code quickly
- **Professionalism**: High-quality code reflects our commitment to excellence
- **Performance**: Some guidelines help with optimization and real-time safety

---

## 📝 Core Principles

### 1. Clarity Over Cleverness
```cpp
// Good: Clear and self-documenting
const auto maxBufferSize = 8192;

// Bad: Clever but unclear
const auto mbs = 8192;
```

### 2. Safety First
```cpp
// Good: Thread-safe and safe
std::atomic<bool> isPlaying_{false};

// Bad: Unsafe for audio threads
bool isPlaying = false;  // Could cause data races
```

### 3. Performance Conscious
```cpp
// Good: Cache-friendly and vectorized
for (size_t i = 0; i < numSamples; ++i) {
    output[i] = input[i] * gain;
}

// Bad: Cache-unfriendly with random access
for (size_t i = 0; i < numChannels; ++i) {
    for (size_t j = 0; j < numSamples; ++j) {
        output[i][j] = input[i][j] * gain;
    }
}
```

---

## 🏗️ File Structure and Naming

### File Naming Conventions

| File Type | Pattern | Example |
|-----------|---------|---------|
| Classes | `PascalCase.cpp/.h` | `AudioProcessor.cpp` |
| Test Files | `TestName.cpp` | `AudioProcessorTest.cpp` |
| Documentation | `SCREAMING_SNAKE_CASE.md` | `AUDIO_PROCESSING_GUIDE.md` |
| Utilities | `camelCase.cpp/.h` | `audioUtils.cpp` |

### Directory Structure

```
apps/desktop/Source/
├── components/          # UI components
├── tests/              # Unit tests
├── ai_client/          # AI service client
└── platform/           # OS-specific code

modules/
├── zenith_core/
│   ├── engine/         # Core audio engine
│   ├── dsp/           # Digital signal processing
│   └── instruments/   # Built-in instruments
├── zenith_ui/
│   ├── ui/            # UI components
│   └── rendering/     # Skia rendering
└── zenith_commands/
    └── commands/      # Command API
```

---

## 🎨 C++ Coding Style

### Formatting Rules

#### Indentation and Braces
```cpp
// Use 4 spaces, no tabs
class AudioProcessor : public juce::AudioProcessor
{
public:
    AudioProcessor()  // Constructor on new line
        : sampleRate_(44100),  // Member initializer
          bufferSize_(512)
    {
        // Open brace on same line as control statement
        if (isValid())
        {
            processAudio();
        }
        else
        {
            handleError();
        }
    }

private:
    int sampleRate_;  // Trailing underscore for members
    int bufferSize_;
};
```

#### Line Length
- **Maximum 120 characters** per line
- **Wrap long lines** at logical boundaries
- **No hard line breaks** in the middle of expressions

**Good:**
```cpp
auto result = std::make_unique<juce::AudioBuffer<float>>(
    numChannels,
    numSamples
);
```

**Bad:**
```cpp
auto result = std::make_unique<juce::AudioBuffer<float>>(numChannels, numSamples);
```

### Naming Conventions

| Category | Style | Examples |
|----------|-------|----------|
| Classes and structs | PascalCase | `AudioEngine`, `MixerChannel` |
| Functions and methods | camelCase | `processAudio()`, `getGain()` |
| Variables | camelCase_ | `sampleRate_`, `bufferSize_` |
| Constants | SCREAMING_SNAKE_CASE | `MAX_BUFFER_SIZE`, `SAMPLE_RATE` |
| Enums | PascalCase | `enum class TransportState` |
| Template parameters | T or TPascalCase | `typename TValue`, `template <typename TSample>` |

#### Example Usage
```cpp
class ZenithPolySynth : public juce::AudioProcessor
{
public:
    enum class SynthMode
    {
        Polyphonic,
        Monophonic,
        Legato
    };

    ZenithPolySynth()
        : sampleRate_(44100),
          cutoffFrequency_(440.0f),
          resonance_(0.5f)
    {
    }

    void processAudio(juce::AudioBuffer<float>& buffer) override
    {
        // Process each sample
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                channelData[sample] = applyFilter(channelData[sample]);
            }
        }
    }

    float applyFilter(float inputSample)
    {
        // Simple filter implementation
        const float output = inputSample * cutoffFrequency_ +
                           previousSample_ * resonance_;
        previousSample_ = inputSample;
        return output;
    }

private:
    int sampleRate_;
    float cutoffFrequency_;
    float resonance_;
    float previousSample_ = 0.0f;
};
```

### Comments and Documentation

#### Doxygen Style for Public APIs
```cpp
/**
 * @brief Processes audio buffer with gain adjustment
 *
 * This method processes the input audio buffer, applying gain and ensuring
 * the output doesn't clip. Designed for real-time audio processing.
 *
 * @param inputBuffer The input audio buffer to process
 * @param gain The gain factor to apply (0.0 to 1.0)
 * @return Processed audio buffer with gain applied
 */
juce::AudioBuffer<float> processWithGain(
    const juce::AudioBuffer<float>& inputBuffer,
    float gain
);
```

#### Implementation Comments
```cpp
// Use // for implementation comments
// Cache the filter coefficients for performance
const float alpha = 1.0f - std::exp(-2.0f * PI * cutoffFrequency_ / sampleRate_);

// Use TODO: for future improvements
// TODO: Add SIMD optimization for this loop
for (int i = 0; i < numSamples; ++i)
{
    output[i] = input[i] * alpha + output[i-1] * (1.0f - alpha);
}
```

---

## 🎵 Audio Programming Guidelines

### Real-Time Safety Rules

#### NEVER in Audio Thread
```cpp
// ✗ Never allocate memory
auto* buffer = new float[1024];  // FORBIDDEN!

// ✗ Never use locks
std::lock_guard<std::mutex> lock(audioMutex);  // FORBIDDEN!

// ✗ Never make system calls
logMessage("Processing audio");  // FORBIDDEN!

// ✗ Never do I/O
file.read(buffer, 1024);  // FORBIDDEN!
```

#### ALWAYS OK in Audio Thread
```cpp
// ✓ Use pre-allocated buffers
float stackBuffer[1024];

// ✓ Use atomic variables
std::atomic<float> gain_;

// ✓ Use SIMD operations
for (int i = 0; i < numSamples; i += 4)
{
    // Process 4 samples at once
    __m128 samples = _mm_load_ps(&input[i]);
    __m128 scaled = _mm_mul_ps(samples, _mm_set1_ps(gain_));
    _mm_store_ps(&output[i], scaled);
}

// ✓ Use template functions
template<typename T>
void processBuffer(T* buffer, int numSamples) {
    // Type-safe processing
}
```

### Audio Buffer Handling

#### Good Practices
```cpp
class AudioEngine
{
public:
    void processAudio(juce::AudioBuffer<float>& buffer)
    {
        // Ensure buffer is valid
        jassert(buffer.getNumSamples() > 0);
        jassert(buffer.getNumChannels() > 0);

        // Use getWritePointer for safe access
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            // Clear buffer if needed
            if (shouldClearBuffer_)
            {
                juce::FloatVectorOperations::clear(
                    channelData,
                    buffer.getNumSamples()
                );
            }
            else
            {
                processChannel(channelData, buffer.getNumSamples());
            }
        }
    }

private:
    void processChannel(float* channelData, int numSamples)
    {
        // Process with zero-latency
        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] = applyEffects(channelData[sample]);
        }
    }

    bool shouldClearBuffer_ = false;
};
```

### Threading Model

#### Thread Types
```cpp
// Audio Thread - Real-time processing
void audioDeviceIOCallback(
    const float** inputChannelData,
    float** outputChannelData,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context
) override
{
    // Real-time audio processing here
    processAudio(outputChannelData, numSamples);
}

// Message Thread - UI and user interaction
void buttonClicked(juce::Button* button) override
{
    // Handle user input
    if (button == &playButton_)
    {
        transport_.play();
    }
}

// Background Thread - File I/O and heavy processing
void loadAudioFile(const juce::String& filePath)
{
    // Load on background thread
    auto* thread = new juce::Thread("Audio Loader");
    thread->start([this, filePath]() {
        // File loading here
        auto buffer = loadFile(filePath);

        // Update UI on message thread
        juce::MessageManager::callAsync([this, buffer]() {
            loadedBuffer_ = buffer;
            updateUI();
        });
    });
}
```

---

## 🖥️ UI Development Guidelines

### JUCE Component Best Practices

#### Component Hierarchy
```cpp
class MainPanel : public juce::Component
{
public:
    MainPanel()
    {
        // Use addAndMakeOwned for memory management
        addAndMakeOwned(&transportBar_);
        addAndMakeOwned(&mixer_);
        addAndMakeOwned(&pianoRoll_);

        // Set sizes and positions
        setSize(1200, 800);
    }

    void resized() override
    {
        // Use RectangleList for layout
        auto bounds = getLocalBounds();

        // Transport bar at top
        transportBar_.setBounds(bounds.withHeight(60));
        bounds = bounds.withTrimmedTop(60);

        // Mixer on right
        mixer_.setBounds(bounds.getRight() - 300, bounds.getY(), 300, bounds.getHeight());
        bounds = bounds.withTrimmedRight(300);

        // Piano roll taking remaining space
        pianoRoll_.setBounds(bounds);
    }

private:
    TransportBar transportBar_;
    MixerPanel mixer_;
    PianoRoll pianoRoll_;
};
```

#### Styling and Theming
```cpp
class StyledButton : public juce::TextButton
{
public:
    StyledButton(const juce::String& name)
        : juce::TextButton(name)
    {
        // Set colors using Zenith theme
        setColour(juce::TextButton::buttonColourId, ZenithColours::buttonBackground);
        setColour(juce::TextButton::textColourId, ZenithColours::text);
        setColour(juce::TextButton::buttonOnColourId, ZenithColours::buttonHover);
    }

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = getLocalBounds();

        // Draw rounded rectangle background
        g.setColour(findColour(juce::TextButton::buttonColourId));
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

        // Add hover effect
        if (isMouseOverButton)
        {
            g.setColour(findColour(juce::TextButton::buttonOnColourId).withAlpha(0.3f));
            g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
        }

        // Draw text
        g.setColour(findColour(juce::TextButton::textColourId));
        g.setFont(juce::Font(14.0f));
        g.drawText(getButtonText(), bounds, juce::Justification::centred, true);
    }
};
```

### Responsive UI Design

#### Adaptive Layout
```cpp
class ResponsiveLayout
{
public:
    static void applyLayout(juce::Component* component, int width, int height)
    {
        if (width < 800)
        {
            // Compact layout for mobile
            component->setBounds(0, 0, width, height);
            applyCompactLayout(component);
        }
        else
        {
            // Full layout for desktop
            component->setBounds(0, 0, width, height);
            applyDesktopLayout(component);
        }
    }

private:
    static void applyCompactLayout(juce::Component* component)
    {
        // Stack panels vertically on small screens
        // Implementation...
    }

    static void applyDesktopLayout(juce::Component* component)
    {
        // Side-by-side panels on desktop
        // Implementation...
    }
};
```

---

## 🧪 Testing Guidelines

### Unit Test Structure
```cpp
class AudioProcessorTest : public juce::UnitTest
{
public:
    AudioProcessorTest() : juce::UnitTest("AudioProcessor", "Audio") {}

    void runTest() override
    {
        beginTest("Initialization");
        testInitialization();

        beginTest("Parameter Changes");
        testParameterChanges();

        beginTest("Audio Processing");
        testAudioProcessing();
    }

    void testInitialization()
    {
        AudioProcessor processor;

        // Test initial state
        expectEquals(processor.getNumInputChannels(), 2);
        expectEquals(processor.getNumOutputChannels(), 2);
        expect(!processor.isPlaying());
    }

    void testParameterChanges()
    {
        AudioProcessor processor;
        processor.prepareToPlay(44100, 512);

        // Test parameter setting
        processor.setParameter(0, 0.5f);  // Gain parameter
        expectEquals(processor.getParameter(0), 0.5f);

        // Test parameter bounds
        processor.setParameter(0, 2.0f);  // Should clamp to 1.0
        expectEquals(processor.getParameter(0), 1.0f);
    }

    void testAudioProcessing()
    {
        AudioProcessor processor;
        processor.prepareToPlay(44100, 512);

        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();

        // Test processing doesn't crash
        juce::MidiBuffer midiBuffer;
        expect(!processor.processBlock(buffer, midiBuffer));

        // Test output isn't silent when input has signal
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Impulse at start of channel 0

        processor.processBlock(buffer, midiBuffer);
        expect(buffer.getRMSLevel(0, 0, buffer.getNumSamples()) > 0.0f);
    }
};

// Create static test instance
static AudioProcessorTest audioProcessorTest;
```

### Integration Tests
```cpp
class TransportIntegrationTest : public juce::UnitTest
{
public:
    TransportIntegrationTest()
        : juce::UnitTest("Transport Integration", "Transport") {}

    void runTest() override
    {
        beginTest("Play-Stop Cycle");
        testPlayStopCycle();

        beginTest("Transport Synchronization");
        testTransportSync();
    }

    void testPlayStopCycle()
    {
        Transport transport;
        TransportUI ui;

        // Connect transport to UI
        ui.setTransport(&transport);

        // Test play button
        ui.getPlayButton().triggerClick();
        expect(transport.isPlaying());

        // Test stop button
        ui.getStopButton().triggerClick();
        expect(!transport.isPlaying());
    }

    void testTransportSync()
    {
        Transport transport;
        TransportUI ui;

        // Test state synchronization
        transport.play();
        expect(ui.getPlayButton().getToggleState());

        transport.stop();
        expect(!ui.getPlayButton().getToggleState());
    }
};

static TransportIntegrationTest transportIntegrationTest;
```

### Performance Tests
```cpp
class PerformanceTest : public juce::UnitTest
{
public:
    PerformanceTest() : juce::UnitTest("Performance", "Audio") {}

    void runTest() override
    {
        beginTest("Audio Processing Latency");
        testProcessingLatency();

        beginTest("Memory Usage");
        testMemoryUsage();
    }

    void testProcessingLatency()
    {
        AudioProcessor processor;
        processor.prepareToPlay(44100, 512);

        auto start = juce::Time::getMillisecondCounterHiRes();

        // Process multiple buffers
        for (int i = 0; i < 1000; ++i)
        {
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            processor.processBlock(buffer, juce::MidiBuffer());
        }

        auto end = juce::Time::getMillisecondCounterHiRes();
        auto duration = end - start;

        // Should process 1000 buffers in under 100ms
        expect(duration < 100.0,
               "Processing too slow: " + juce::String(duration) + "ms");
    }

    void testMemoryUsage()
    {
        AudioProcessor processor;

        // Check for memory leaks
        size_t initialMemory = getCurrentMemoryUsage();

        // Create and destroy many instances
        for (int i = 0; i < 1000; ++i)
        {
            AudioProcessor* temp = new AudioProcessor();
            delete temp;
        }

        size_t finalMemory = getCurrentMemoryUsage();

        // Memory usage should not grow significantly
        expect(finalMemory - initialMemory < 1024 * 1024,  // Less than 1MB
               "Memory leak detected: " + juce::String(finalMemory - initialMemory) + " bytes");
    }

    size_t getCurrentMemoryUsage()
    {
        #if JUCE_MAC
        struct task_basic_info info;
        mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
            return info.resident_size;
        #endif

        return 0;
    }
};

static PerformanceTest performanceTest;
```

---

## 🔧 Build and CI Guidelines

### CMake Configuration

#### Required Build Options
```cmake
# Required: Enable Skia for GPU rendering
option(ZENITH_ENABLE_SKIA "Enable Skia GPU rendering" ON)

# Required: Enable real-time audio priority
option(ZENITH_ENABLE_MMCSS "Enable Windows Multimedia Class Scheduler" ON)

# Optional: Enable tests
option(BUILD_TESTS "Build unit tests" OFF)
```

#### Compiler Flags
```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # Visual Studio flags
    add_compile_options(/W4 /WX)  # Treat warnings as errors

    # Enable C++20
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

else()
    # GCC/Clang flags
    add_compile_options(-Wall -Wextra -Werror -pedantic)

    # Enable modern C++ features
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Additional warnings
    add_compile_options(-Wshadow -Woverloaded-virtual -Wnon-virtual-dtor)
endif()
```

### Code Quality Checks

#### Static Analysis
```cmake
# Enable address sanitizer for debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ZenithCore PRIVATE "DEBUG=1")

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(ZenithCore PRIVATE
            -fsanitize=address
            -fsanitize=leak
            -fsanitize=undefined
        )
        target_link_libraries(ZenithCore
            -fsanitize=address
            -fsanitize=leak
            -fsanitize=undefined
        )
    endif()
endif()
```

### Pre-commit Hook

#### .git/hooks/pre-commit
```bash
#!/bin/bash
# Pre-commit script for code quality checks

echo "Running pre-commit checks..."

# Build in Debug mode
echo "Building in Debug mode..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
if cmake --build build --config Debug; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

# Run tests
echo "Running tests..."
if ./build/ZenithDAWTests_artefacts/Debug/ZenithDAWTests; then
    echo "✅ All tests passed"
else
    echo "❌ Tests failed"
    exit 1
fi

# Check for common issues
echo "Checking for common issues..."

# Check for TODO comments in code
if grep -r "TODO:" apps/desktop/Source/ --include="*.cpp" --include="*.h"; then
    echo "⚠️  Found TODO comments - are they intentional?"
fi

# Check for memory allocations in audio thread
if grep -r "new\|delete\|malloc\|free" modules/zenith_core/engine/ --include="*.cpp" --include="*.h"; then
    echo "⚠️  Found potential memory allocations in audio thread - check thread safety!"
fi

echo "✅ All checks passed"
exit 0
```

---

## 🚫 Common Anti-Patterns to Avoid

### Memory Management
```cpp
// ✗ Bad: Raw pointers without ownership
AudioProcessor* processor = new AudioProcessor();  // Who owns this?

// ✓ Good: Smart pointers
auto processor = std::make_unique<AudioProcessor>();

// ✓ Good: RAII with JUCE components
addAndMakeOwned(&myComponent);
```

### Threading Issues
```cpp
// ✗ Bad: Blocking audio thread
juce::Thread::sleep(10);  // NEVER!

// ✓ Good: Use message thread for blocking operations
juce::MessageManager::callAsync([]() {
    // Safe blocking operations here
});
```

### Resource Management
```cpp
// ✗ Bad: File handles not closed
std::ifstream file("audio.wav");
// ... processing ...
// File might not be closed if exception thrown

// ✓ Good: RAII file handling
auto file = std::make_unique<std::ifstream>("audio.wav");
if (file->is_open()) {
    // ... processing ...
}  // File automatically closed
```

### Error Handling
```cpp
// ✗ Bad: Silent failures
if (error) {
    // Do nothing - hoping it works
}

// ✓ Good: Proper error handling
if (error) {
    handleError(error);  // Log and handle gracefully
}
```

---

## 🎯 Review Checklist

### Self-Review Checklist
Before submitting a pull request:

- [ ] Code follows all style guidelines
- [ ] All tests pass (unit and integration)
- [ ] Performance benchmarks are met
- [ ] Audio thread safety is maintained
- [ ] Documentation is updated
- [ ] Code is reviewed by another contributor

### Reviewer Checklist
When reviewing code:

- [ ] Code follows style guidelines
- [ ] Logic is correct and efficient
- [ ] Edge cases are handled
- [ ] Performance is acceptable
- [ ] Thread safety is maintained
- [ ] Tests are comprehensive
- [ ] Documentation is accurate

---

## 📚 Additional Resources

### Tools for Code Quality
- **clang-format**: Automated code formatting
- **clang-tidy**: Static analysis
- **Valgrind**: Memory leak detection (Linux)
- **AddressSanitizer**: Memory error detection
- **JUCE Unit Test Framework**: Testing framework

### Recommended Reading
- **Effective C++** by Scott Meyers
- **Clean Code** by Robert Martin
- **JUCE Documentation**: https://docs.juce.com/
- **Real-time Audio Programming**: https://github.com/mbrucher/rt-audio

### Community Standards
- **Code of Conduct**: Available in repository
- **Issue Guidelines**: Follow issue templates
- **Pull Request Templates**: Use provided templates
- **Communication**: Be respectful and constructive

---

## 🎉 Conclusion

Following these style guidelines ensures that Zenith DAW remains a high-quality, maintainable project that we can all be proud of. Remember:

- **Consistency is key** - Follow the established patterns
- **Quality matters** - Every line of code counts
- **Collaboration is important** - Make it easy for others to understand your work
- **Learning is ongoing** - Continue to improve your skills

Thank you for your dedication to creating amazing audio software! 🎵✨

---

**Questions about style?** Ask in [GitHub Discussions](https://github.com/zenith-daw/zenith/discussions) or join our [Discord server](https://discord.gg/zenith-daw)