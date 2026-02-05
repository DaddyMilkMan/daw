/*
  ==============================================================================
    agents/TransportProtocolAgent/TransportProtocolAgent.h
    Cross-platform audio transport protocol abstraction layer.
  ==============================================================================
*/

#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

namespace zenith {
namespace agents {

//==============================================================================
/**
    TransportProtocolAgent provides unified cross-platform audio device access,
    abstracting ASIO, WASAPI, CoreAudio, ALSA, and JACK protocols.
*/
class TransportProtocolAgent {
public:
  //==============================================================================
  struct DeviceInfo {
    juce::String name;
    juce::String id;
    int numInputChannels{0};
    int numOutputChannels{0};
    juce::Array<double> supportedSampleRates;
    juce::Array<int> supportedBufferSizes;
    bool isDefault{false};
    juce::String apiType; // "ASIO", "WASAPI", "CoreAudio", "ALSA", "JACK"
  };
  
  enum class DeviceState {
    Disconnected,
    Connected,
    Active,
    Error
  };

  //==============================================================================
  TransportProtocolAgent();
  explicit TransportProtocolAgent(std::unique_ptr<juce::AudioDeviceManager> manager);
  ~TransportProtocolAgent();

  //==============================================================================
  // Device Enumeration
  
  /// Scan for available audio devices
  std::vector<DeviceInfo> enumerateDevices();
  
  /// Get default input device
  DeviceInfo getDefaultInputDevice();
  
  /// Get default output device
  DeviceInfo getDefaultOutputDevice();
  
  //==============================================================================
  // Device Management
  
  /// Open audio device with specified configuration
  bool openDevice(const juce::String& deviceId, 
                  double sampleRate,
                  int bufferSize);
  
  /// Close currently open device
  void closeDevice();
  
  /// Check if device is currently open
  bool isDeviceOpen() const;
  
  /// Get current device state
  DeviceState getDeviceState() const;
  
  //==============================================================================
  // Configuration
  
  /// Get currently active sample rate
  double getCurrentSampleRate() const;
  
  /// Get current buffer size
  int getCurrentBufferSize() const;
  
  /// Get input latency in samples
  int getInputLatencySamples() const;
  
  /// Get output latency in samples
  int getOutputLatencySamples() const;

  //==============================================================================
  // Platform-Specific Backends

  enum class BackendType {
    ASIO,
    WASAPI_Shared,
    WASAPI_Exclusive,
    ALSA,
    CoreAudio,
    JACK,
    Unknown
  };

  /// Get list of available audio backends
  std::vector<BackendType> getAvailableBackends() const;

  /// Get current active backend
  BackendType getCurrentBackend() const;

  /// Switch the audio backend (driver type)
  bool setBackend(BackendType backend, bool treatAsChosenDevice = true);

  //==============================================================================
  // Device Control Panel (ASIO/Generic)

  /// Check if current device supports a control panel
  bool currentDeviceHasControlPanel() const;

  /// Open the device control panel
  bool showCurrentDeviceControlPanel();

  //==============================================================================
  // WASAPI Specifics

  enum class WasapiMode { Shared, Exclusive };

  /// Set WASAPI mode (internally switches backend type)
  bool setWasapiMode(WasapiMode mode);

  /// Get current WASAPI mode
  WasapiMode getWasapiMode() const;

  //==============================================================================
  // ALSA Specifics

  /// Enumerate raw ALSA device names (e.g., "hw:0,0")
  std::vector<juce::String> enumerateAlsaDeviceNames(bool inputs) const;

  /// Open ALSA device by its raw name
  bool openAlsaDeviceByName(const juce::String& deviceName,
                            double sampleRate,
                            int bufferSize);

private:
  //==============================================================================
  juce::String backendTypeToJuceName(BackendType b) const;
  BackendType juceNameToBackendType(const juce::String& typeName) const;

  std::unique_ptr<juce::AudioDeviceManager> deviceManager_;
  DeviceState currentState_{DeviceState::Disconnected};
  
  // TODO: Add device hot-plug detection
  // TODO: Add buffer format conversion layer
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportProtocolAgent)
};

//==============================================================================
/**
    Audio format bit depth enumeration.
*/
enum class BitDepth {
  Int16,
  Int24,
  Int32,
  Float32
};

//==============================================================================
/**
    Utility for converting between interleaved audio data and planar JUCE AudioBuffers.
    Wraps juce::AudioDataConverters for optimized performance.
*/
struct AudioBufferConverter {
  /**
      De-interleaves raw audio data from a device into a JUCE AudioBuffer.
      @param sourceData        Pointer to the raw interleaved data (e.g., from the driver)
      @param destBuffer        The planar buffer to fill
      @param numSamples        Number of samples to process per channel
      @param numChannels       Number of channels in the source/dest
      @param sourceFormat      Enum for Int16, Int24, Int32, or Float32
  */
  static void convertToPlanarFloat(const void* sourceData,
                                   juce::AudioBuffer<float>& destBuffer,
                                   int numSamples,
                                   int numChannels,
                                   BitDepth sourceFormat);

  /**
      Interleaves a JUCE AudioBuffer into raw memory for output.
      @param sourceBuffer      The planar source buffer
      @param destData          Pointer to the raw interleaved destination memory
      @param numSamples        Number of samples to process per channel
      @param numChannels       Number of channels in the source/dest
      @param destFormat        Enum for Int16, Int24, Int32, or Float32
  */
  static void convertFromPlanarFloat(const juce::AudioBuffer<float>& sourceBuffer,
                                     void* destData,
                                     int numSamples,
                                     int numChannels,
                                     BitDepth destFormat);

  // Helper to get bytes per sample for allocation
  static int getBytesPerSample(BitDepth depth) {
      switch (depth) {
          case BitDepth::Int16: return 2;
          case BitDepth::Int24: return 3;
          case BitDepth::Int32: return 4;
          case BitDepth::Float32: return 4;
          default: return 0;
      }
  }
};

} // namespace agents
} // namespace zenith
