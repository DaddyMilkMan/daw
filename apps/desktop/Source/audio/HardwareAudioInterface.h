/*
  ==============================================================================
    HardwareAudioInterface.h
    Hardware audio interface support - production ready
    Phase 2: Audio I/O & Processing
  ==============================================================================
*/

#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "RealTimeAudioBuffer.h"
#include <memory>
#include <vector>
#include <functional>

namespace zenith {
namespace audio {

// Hardware device information
struct HardwareDeviceInfo {
    juce::String name;
    juce::String driver;
    juce::String identifier;
    int inputChannels;
    int outputChannels;
    std::vector<double> supportedSampleRates;
    std::vector<int> supportedBufferSizes;
    bool isDefaultInput;
    bool isDefaultOutput;
    bool supportsLowLatency;
    juce::String manufacturer;
    juce::String driverVersion;
};

// Audio interface configuration
struct AudioInterfaceConfig {
    double sampleRate = 44100.0;
    int bufferSize = 512;
    int inputChannels = 2;
    int outputChannels = 2;
    double targetLatency = 10.0;  // ms
    bool enableLowLatency = false;
    bool enableExclusiveMode = false;
    juce::String inputDeviceName;
    juce::String outputDeviceName;
};

// Real-time audio callback
using AudioCallback = std::function<void(const juce::AudioBuffer<float>&, juce::AudioBuffer<float>&)>;

// Hardware audio interface manager
class HardwareAudioInterface {
public:
    HardwareAudioInterface();
    ~HardwareAudioInterface();
    
    // Device discovery
    std::vector<HardwareDeviceInfo> getAvailableDevices() const;
    HardwareDeviceInfo getCurrentInputDevice() const;
    HardwareDeviceInfo getCurrentOutputDevice() const;
    bool hasDevice(const juce::String& deviceName) const;
    
    // Device selection
    bool selectInputDevice(const juce::String& deviceName);
    bool selectOutputDevice(const juce::String& deviceName);
    bool selectDevicePair(const juce::String& inputDevice, const juce::String& outputDevice);
    
    // Configuration
    bool configure(const AudioInterfaceConfig& config);
    bool setSampleRate(double sampleRate);
    bool setBufferSize(int bufferSize);
    bool setChannelCount(int inputChannels, int outputChannels);
    bool setTargetLatency(double latencyMs);
    
    // Device control
    bool startDevice();
    bool stopDevice();
    bool restartDevice();
    bool isDeviceActive() const;
    
    // Real-time audio
    void setAudioCallback(AudioCallback callback);
    void removeAudioCallback();
    
    // Monitoring
    std::vector<float> getInputLevels() const;
    std::vector<float> getOutputLevels() const;
    bool isInputClipping(int channel) const;
    bool isOutputClipping(int channel) const;
    float getCurrentLatency() const;
    float getCPUUsage() const;
    
    // Advanced features
    bool enableExclusiveMode(bool enabled);
    bool enableLowLatencyMode(bool enabled);
    bool enableDirectMonitoring(bool enabled);
    bool setClockSource(const juce::String& source);
    
    // Error handling
    juce::String getLastError() const;
    bool hasErrors() const;
    void clearErrors();
    
    // Device capabilities
    std::vector<double> getSupportedSampleRates(const juce::String& deviceName) const;
    std::vector<int> getSupportedBufferSizes(const juce::String& deviceName) const;
    bool supportsSampleRate(const juce::String& deviceName, double sampleRate) const;
    bool supportsBufferSize(const juce::String& deviceName, int bufferSize) const;
    
private:
    std::unique_ptr<juce::AudioDeviceManager> deviceManager;
    std::unique_ptr<juce::AudioIODevice> currentInputDevice;
    std::unique_ptr<juce::AudioIODevice> currentOutputDevice;
    
    AudioInterfaceConfig currentConfig;
    HardwareDeviceInfo currentInputDeviceInfo;
    HardwareDeviceInfo currentOutputDeviceInfo;
    
    AudioCallback audioCallback;
    std::atomic<bool> deviceActive{false};
    std::atomic<bool> exclusiveMode{false};
    std::atomic<bool> lowLatencyMode{false};
    
    // Monitoring
    std::vector<float> inputLevels;
    std::vector<float> outputLevels;
    std::vector<bool> inputClipping;
    std::vector<bool> outputClipping;
    std::atomic<float> currentLatency{0.0f};
    std::atomic<float> cpuUsage{0.0f};
    
    // Error handling
    juce::String lastError;
    mutable juce::CriticalSection errorMutex;
    
    // Internal audio callback
    void internalAudioCallback(const juce::AudioBuffer<float>& inputBuffer,
                              int numInputChannels,
                              juce::AudioBuffer<float>& outputBuffer,
                              int numOutputChannels);
    
    // Device management
    bool initializeDeviceManager();
    void updateDeviceInfo();
    HardwareDeviceInfo createDeviceInfo(const juce::AudioIODevice* device) const;
    
    // Configuration validation
    bool validateConfiguration(const AudioInterfaceConfig& config) const;
    AudioInterfaceConfig getOptimalConfiguration(const AudioInterfaceConfig& requested) const;
    
    // Monitoring
    void updateLevels(const juce::AudioBuffer<float>& buffer, 
                     std::vector<float>& levels, 
                     std::vector<bool>& clipping);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HardwareAudioInterface)
};

// Audio interface factory
class AudioInterfaceFactory {
public:
    static std::unique_ptr<HardwareAudioInterface> createInterface();
    static std::vector<HardwareDeviceInfo> scanForDevices();
    static bool testDevice(const juce::String& deviceName);
    static HardwareDeviceInfo getBestDevice(int inputChannels, int outputChannels);
    
private:
    static std::vector<HardwareDeviceInfo> scanDevicesForType(juce::AudioIODeviceType* deviceType);
};

// Audio interface monitor
class AudioInterfaceMonitor {
public:
    struct MonitoringData {
        float inputLevel;
        float outputLevel;
        float cpuUsage;
        float latency;
        int underruns;
        int overruns;
        bool isClipping;
        juce::Time timestamp;
    };
    
    AudioInterfaceMonitor();
    ~AudioInterfaceMonitor();
    
    // Monitoring
    void startMonitoring(HardwareAudioInterface* interface);
    void stopMonitoring();
    bool isMonitoring() const { return monitoringActive; }
    
    // Data access
    MonitoringData getCurrentData() const;
    std::vector<MonitoringData> getHistory(int seconds = 60) const;
    float getAverageLevel(bool input = true) const;
    float getPeakLevel(bool input = true) const;
    int getDropouts() const;
    
    // Alerts
    void setLevelAlertThreshold(float thresholdDb);
    void setCPUAlertThreshold(float thresholdPercent);
    void setLatencyAlertThreshold(float thresholdMs);
    
    bool hasLevelAlert() const;
    bool hasCPUAlert() const;
    bool hasLatencyAlert() const;
    
    // Export
    void exportMonitoringData(const juce::File& filePath) const;
    
private:
    HardwareAudioInterface* audioInterface = nullptr;
    std::atomic<bool> monitoringActive{false};
    
    // Monitoring data
    std::queue<MonitoringData> dataHistory;
    mutable juce::CriticalSection dataMutex;
    static constexpr size_t MAX_HISTORY_SIZE = 3600;  // 1 hour at 1 second intervals
    
    // Alert thresholds
    std::atomic<float> levelAlertThreshold{-3.0f};  // -3dB
    std::atomic<float> cpuAlertThreshold{80.0f};    // 80%
    std::atomic<float> latencyAlertThreshold{20.0f}; // 20ms
    
    // Alert state
    std::atomic<bool> levelAlertActive{false};
    std::atomic<bool> cpuAlertActive{false};
    std::atomic<bool> latencyAlertActive{false};
    
    // Timer for monitoring
    juce::Timer monitoringTimer;
    
    void updateMonitoringData();
    void checkAlerts();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioInterfaceMonitor)
};

// Audio interface presets
class AudioInterfacePresets {
public:
    struct Preset {
        juce::String name;
        juce::String description;
        AudioInterfaceConfig config;
        juce::String category;
        bool isBuiltIn;
    };
    
    AudioInterfacePresets();
    ~AudioInterfacePresets();
    
    // Preset management
    std::vector<Preset> getPresets() const;
    std::vector<Preset> getPresetsForCategory(const juce::String& category) const;
    Preset getPreset(const juce::String& name) const;
    
    bool loadPreset(const juce::String& name, AudioInterfaceConfig& config) const;
    bool savePreset(const juce::String& name, const AudioInterfaceConfig& config, const juce::String& description = "");
    bool deletePreset(const juce::String& name);
    
    // Categories
    std::vector<juce::String> getCategories() const;
    
    // Import/Export
    bool exportPresets(const juce::File& filePath) const;
    bool importPresets(const juce::File& filePath);
    
private:
    std::unordered_map<juce::String, Preset> presets;
    juce::File presetFile;
    
    void loadBuiltInPresets();
    void loadUserPresets();
    void saveUserPresets();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioInterfacePresets)
};

} // namespace audio
} // namespace zenith
