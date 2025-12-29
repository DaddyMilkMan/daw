/*
  ==============================================================================
    RealTimeAudioBuffer.h
    Production real-time audio buffer management - no shortcuts
    Phase 2: Audio I/O & Processing
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <chrono>
#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace zenith {
namespace audio {

// Lock-free ring buffer for real-time audio
template<typename T, size_t Size>
class LockFreeRingBuffer {
public:
    LockFreeRingBuffer() : writePos(0), readPos(0) {}
    
    bool push(const T& item) {
        size_t nextWrite = (writePos.load() + 1) % Size;
        if (nextWrite == readPos.load()) {
            return false;  // Buffer full
        }
        
        buffer[writePos.load()] = item;
        writePos.store(nextWrite);
        return true;
    }
    
    bool pop(T& item) {
        if (readPos.load() == writePos.load()) {
            return false;  // Buffer empty
        }
        
        item = buffer[readPos.load()];
        readPos.store((readPos.load() + 1) % Size);
        return true;
    }
    
    size_t size() const {
        size_t w = writePos.load();
        size_t r = readPos.load();
        return (w >= r) ? (w - r) : (Size - r + w);
    }
    
    bool isEmpty() const {
        return readPos.load() == writePos.load();
    }
    
    bool isFull() const {
        size_t nextWrite = (writePos.load() + 1) % Size;
        return nextWrite == readPos.load();
    }
    
    void clear() {
        writePos.store(0);
        readPos.store(0);
    }
    
private:
    std::array<T, Size> buffer;
    std::atomic<size_t> writePos;
    std::atomic<size_t> readPos;
};

// Real-time audio buffer with channel management
class RealTimeAudioBuffer {
public:
    RealTimeAudioBuffer(int numChannels, int bufferSize);
    ~RealTimeAudioBuffer();
    
    // Buffer operations (real-time safe)
    bool writeAudio(const juce::AudioBuffer<float>& source);
    bool readAudio(juce::AudioBuffer<float>& destination);
    
    // Channel management
    void setNumChannels(int channels);
    int getNumChannels() const { return numChannels; }
    
    // Buffer management
    void setBufferSize(int size);
    int getBufferSize() const { return bufferSize; }
    
    // Monitoring
    float getLevel(int channel) const;
    bool isClipping(int channel) const;
    void resetLevels();
    
    // Statistics
    float getAverageLatency() const;
    int getDropouts() const { return dropouts; }
    void resetStatistics();
    
private:
    int numChannels;
    int bufferSize;
    std::atomic<int> dropouts{0};
    
    // Lock-free buffers for each channel
    std::vector<std::unique_ptr<LockFreeRingBuffer<float, 65536>>> channelBuffers;
    
    // Level monitoring - direct atomics, no unnecessary indirection
    // Level monitoring - atomics wrapped in unique_ptr to allow vector resizing
    std::vector<std::unique_ptr<std::atomic<float>>> channelLevels;
    std::vector<std::unique_ptr<std::atomic<bool>>> channelClipping;
    
    // Latency tracking
    std::atomic<float> averageLatency{0.0f};
    std::queue<juce::Time> writeTimes;
    std::queue<juce::Time> readTimes;
    mutable std::mutex timingMutex;
    
    void updateLevels(const juce::AudioBuffer<float>& buffer);
    void updateLatency();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeAudioBuffer)
};

// Audio device manager with hardware support
class AudioDeviceManager {
public:
    AudioDeviceManager();
    ~AudioDeviceManager();
    
    // Device management
    bool initialize(double sampleRate = 44100.0, int bufferSize = 512);
    void shutdown();
    
    // Device selection
    std::vector<juce::String> getAvailableInputDevices() const;
    std::vector<juce::String> getAvailableOutputDevices() const;
    bool setInputDevice(const juce::String& deviceName);
    bool setOutputDevice(const juce::String& deviceName);
    
    // Configuration
    bool setSampleRate(double sampleRate);
    bool setBufferSize(int bufferSize);
    double getCurrentSampleRate() const;
    int getCurrentBufferSize() const;
    
    // Real-time audio callback
    void setAudioCallback(juce::AudioIODeviceCallback* callback);
    void removeAudioCallback();
    
    // Monitoring
    float getInputLevel(int channel) const;
    float getOutputLevel(int channel) const;
    bool isDeviceActive() const;
    
    // Error handling
    juce::String getLastError() const;
    bool hasErrors() const;
    
private:
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    // OWNERSHIP: NON-OWNING pointer to device managed by juce::AudioDeviceManager
    // Valid only while device is open. Always check isDeviceActive() before use.
    // Becomes invalid after shutdown() or device change.
    juce::AudioIODevice* currentDevice = nullptr;
    juce::String lastError;
    
    // Device state
    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;
    bool isActive = false;
    
    // Level monitoring
    std::vector<float> inputLevels;
    std::vector<float> outputLevels;
    
    bool selectBestDevice();
    void updateDeviceList();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceManager)
};

// Sample rate converter with high quality
class SampleRateConverter {
public:
    enum class Quality {
        Fast,
        Good,
        Best,
        Linear,
        Sinc
    };
    
    SampleRateConverter();
    ~SampleRateConverter();
    
    // Conversion
    bool convert(const juce::AudioBuffer<float>& input,
                juce::AudioBuffer<float>& output,
                double inputSampleRate,
                double outputSampleRate,
                Quality quality = Quality::Good);
    
    // Configuration
    void setQuality(Quality quality);
    Quality getQuality() const { return currentQuality; }
    
    // Information
    double getLatency() const;
    bool isInPlaceSupported() const { return false; }
    
private:
    Quality currentQuality = Quality::Good;
    
    // Conversion algorithms
    bool convertLinear(const juce::AudioBuffer<float>& input,
                      juce::AudioBuffer<float>& output,
                      double ratio);
    
    bool convertSinc(const juce::AudioBuffer<float>& input,
                    juce::AudioBuffer<float>& output,
                    double ratio);
    
    // Sinc filter
    std::vector<float> sincKernel;
    int kernelSize = 64;
    void buildSincKernel(double cutoff);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRateConverter)
};

// Real-time audio processor with low latency
class RealTimeAudioProcessor {
public:
    RealTimeAudioProcessor();
    ~RealTimeAudioProcessor();
    
    // Initialization
    bool initialize(int numChannels, double sampleRate, int bufferSize);
    void shutdown();
    
    // Processing
    void processAudio(juce::AudioBuffer<float>& buffer);
    
    // RT-SAFE: Template version eliminates std::function heap allocation
    // C++20 constraint prevents accidentally passing std::function which allocates
    template<typename ProcessorFunc>
        requires std::invocable<ProcessorFunc, juce::AudioBuffer<float>&> &&
                 (!std::is_same_v<std::decay_t<ProcessorFunc>, 
                                  std::function<void(juce::AudioBuffer<float>&)>>)
    void processAudioWithCallback(juce::AudioBuffer<float>& buffer, ProcessorFunc&& processor) {
        // Check real-time safety
        if (!checkRealTimeSafety()) {
            underruns.fetch_add(1);
        }
        
        // Execute the actual processing callback - INLINED, no allocation
        std::forward<ProcessorFunc>(processor)(buffer);
        
        // Update CPU usage metrics with atomic operations only
        // Note: For precise timing, consider using a separate metrics thread
        updatePerformanceMetrics();
    }
    
    // Buffer management
    bool setInputBuffer(const juce::AudioBuffer<float>& buffer);
    bool getOutputBuffer(juce::AudioBuffer<float>& buffer);
    
    // Latency management
    int getLatencySamples() const;
    double getLatencySeconds() const;
    void setTargetLatency(double seconds);
    
    // Performance monitoring
    float getCpuUsage() const;
    int getUnderruns() const;
    int getOverruns() const;
    void resetPerformanceCounters();
    
    // Real-time safety
    bool isRealTimeSafe() const;
    void setRealTimePriority(bool enabled);
    
private:
    std::unique_ptr<RealTimeAudioBuffer> inputBuffer;
    std::unique_ptr<RealTimeAudioBuffer> outputBuffer;
    std::unique_ptr<SampleRateConverter> sampleRateConverter;
    
    int numChannels = 2;
    double sampleRate = 44100.0;
    int bufferSize = 512;
    double targetLatency = 0.01;  // 10ms
    
    // Performance monitoring
    std::atomic<float> cpuUsage{0.0f};
    std::atomic<int> underruns{0};
    std::atomic<int> overruns{0};
    
    // Timing
    juce::Time lastProcessTime;
    std::atomic<bool> realTimePriority{false};
    
    void updatePerformanceMetrics();
    bool checkRealTimeSafety() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RealTimeAudioProcessor)
};

// Audio interface for hardware integration
class AudioInterface {
public:
    struct DeviceInfo {
        juce::String name;
        juce::String driver;
        int inputChannels;
        int outputChannels;
        std::vector<double> supportedSampleRates;
        std::vector<int> supportedBufferSizes;
        bool isDefaultInput;
        bool isDefaultOutput;
    };
    
    AudioInterface();
    ~AudioInterface();
    
    // Device discovery
    std::vector<DeviceInfo> getAvailableDevices() const;
    DeviceInfo getCurrentDevice() const;
    bool selectDevice(const juce::String& deviceName);
    
    // Configuration
    bool configureDevice(double sampleRate, int bufferSize, int inputChannels, int outputChannels);
    bool startDevice();
    bool stopDevice();
    
    // Real-time I/O
    void setAudioCallback(std::function<void(const juce::AudioBuffer<float>&, juce::AudioBuffer<float>&)> callback);
    void removeAudioCallback();
    
    // Monitoring
    float getInputLevel(int channel) const;
    float getOutputLevel(int channel) const;
    bool isDeviceActive() const;
    
    // Error handling
    juce::String getLastError() const;
    
private:
    std::unique_ptr<AudioDeviceManager> deviceManager;
    std::unique_ptr<RealTimeAudioProcessor> processor;
    
    DeviceInfo currentDevice;
    juce::String lastError;
    bool isActive = false;
    
    // Callback management
    std::function<void(const juce::AudioBuffer<float>&, juce::AudioBuffer<float>&)> audioCallback;
    
    // Internal audio callback
    void audioDeviceIOCallback(const juce::AudioBuffer<float>& inputBuffer,
                               int numInputChannels,
                               juce::AudioBuffer<float>& outputBuffer,
                               int numOutputChannels);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioInterface)
};

} // namespace audio
} // namespace zenith
