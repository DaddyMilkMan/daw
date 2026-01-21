/*
  ==============================================================================
    agents/TransportProtocolAgent/TransportProtocolAgent.cpp
    Cross-platform audio transport protocol implementation.
  ==============================================================================
*/

#include "TransportProtocolAgent.h"

namespace zenith {
namespace agents {

//==============================================================================
TransportProtocolAgent::TransportProtocolAgent() 
  : deviceManager_(std::make_unique<juce::AudioDeviceManager>()) {
  // Initialize device manager
  deviceManager_->initialiseWithDefaultDevices(2, 2);
}

TransportProtocolAgent::~TransportProtocolAgent() {
  closeDevice();
}

//==============================================================================
// Device Enumeration

std::vector<TransportProtocolAgent::DeviceInfo> 
TransportProtocolAgent::enumerateDevices() {
  std::vector<DeviceInfo> devices;
  
  // TODO: Enumerate all available audio device types
  // TODO: Query device capabilities
  // TODO: Filter by platform-specific APIs
  
  auto* currentDevice = deviceManager_->getCurrentAudioDevice();
  if (currentDevice != nullptr) {
    DeviceInfo info;
    info.name = currentDevice->getName();
    info.id = currentDevice->getName(); // Simplified
    info.numInputChannels = currentDevice->getActiveInputChannels().countNumberOfSetBits();
    info.numOutputChannels = currentDevice->getActiveOutputChannels().countNumberOfSetBits();
    info.supportedSampleRates = currentDevice->getAvailableSampleRates();
    info.supportedBufferSizes = currentDevice->getAvailableBufferSizes();
    info.isDefault = true;
    info.apiType = currentDevice->getTypeName();
    
    devices.push_back(info);
  }
  
  return devices;
}

TransportProtocolAgent::DeviceInfo TransportProtocolAgent::getDefaultInputDevice() {
  DeviceInfo info;

  if (deviceManager_ == nullptr)
    return info;

  for (auto* type : deviceManager_->getAvailableDeviceTypes()) {
    if (type == nullptr)
      continue;

    type->scanForDevices();

    // Check for default input device (true = input)
    int defaultIndex = type->getDefaultDeviceIndex(true);

    if (defaultIndex >= 0) {
      auto deviceNames = type->getDeviceNames();

      if (defaultIndex < deviceNames.size()) {
        info.name = deviceNames[defaultIndex];
        info.id = deviceNames[defaultIndex];
        info.apiType = type->getTypeName();
        info.isDefault = true;

        // If the default device is the currently open device, we can fill in more details
        auto* currentDevice = deviceManager_->getCurrentAudioDevice();
        if (currentDevice != nullptr && currentDevice->getName() == info.name &&
            currentDevice->getTypeName() == info.apiType) {
          info.numInputChannels = currentDevice->getActiveInputChannels().countNumberOfSetBits();
          info.numOutputChannels = currentDevice->getActiveOutputChannels().countNumberOfSetBits();
          info.supportedSampleRates = currentDevice->getAvailableSampleRates();
          info.supportedBufferSizes = currentDevice->getAvailableBufferSizes();
        }

        return info;
      }
    }
  }

  // Fallback
  info.name = "Default Input";
  return info;
}

TransportProtocolAgent::DeviceInfo TransportProtocolAgent::getDefaultOutputDevice() {
  // TODO: Get platform default output device
  DeviceInfo info;
  info.name = "Default Output";
  return info;
}

//==============================================================================
// Device Management

bool TransportProtocolAgent::openDevice(const juce::String& deviceId,
                                        double sampleRate,
                                        int bufferSize) {
  jassert(sampleRate > 0.0 && bufferSize > 0);
  
  // TODO: Open specific device by ID
  // TODO: Apply sample rate and buffer size
  // TODO: Handle device open errors
  
  currentState_ = DeviceState::Active;
  return true;
}

void TransportProtocolAgent::closeDevice() {
  if (deviceManager_) {
    deviceManager_->closeAudioDevice();
  }
  currentState_ = DeviceState::Disconnected;
}

bool TransportProtocolAgent::isDeviceOpen() const {
  return currentState_ == DeviceState::Active;
}

TransportProtocolAgent::DeviceState TransportProtocolAgent::getDeviceState() const {
  return currentState_;
}

//==============================================================================
// Configuration

double TransportProtocolAgent::getCurrentSampleRate() const {
  auto* device = deviceManager_->getCurrentAudioDevice();
  return device ? device->getCurrentSampleRate() : 0.0;
}

int TransportProtocolAgent::getCurrentBufferSize() const {
  auto* device = deviceManager_->getCurrentAudioDevice();
  return device ? device->getCurrentBufferSizeSamples() : 0;
}

int TransportProtocolAgent::getInputLatencySamples() const {
  auto* device = deviceManager_->getCurrentAudioDevice();
  return device ? device->getInputLatencyInSamples() : 0;
}

int TransportProtocolAgent::getOutputLatencySamples() const {
  auto* device = deviceManager_->getCurrentAudioDevice();
  return device ? device->getOutputLatencyInSamples() : 0;
}

} // namespace agents
} // namespace zenith
