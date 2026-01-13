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

private:
  //==============================================================================
  std::unique_ptr<juce::AudioDeviceManager> deviceManager_;
  DeviceState currentState_{DeviceState::Disconnected};
  
  // TODO: Add platform-specific backend implementations
  // TODO: Add device hot-plug detection
  // TODO: Add buffer format conversion layer
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportProtocolAgent)
};

} // namespace agents
} // namespace zenith
