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
  
  for (auto* type : deviceManager_->getAvailableDeviceTypes())
  {
    type->scanForDevices();

    auto inputNames = type->getDeviceNames(true);
    auto outputNames = type->getDeviceNames(false);
    
    // Combine lists to find unique devices
    juce::StringArray allNames;
    allNames.addArray(inputNames);
    for (const auto& name : outputNames)
    {
      if (!allNames.contains(name))
        allNames.add(name);
    }

    for (const auto& name : allNames)
    {
      DeviceInfo info;
      info.name = name;
      info.id = name;
      info.apiType = type->getTypeName();

      bool hasInput = inputNames.contains(name);
      bool hasOutput = outputNames.contains(name);

      // Estimate channels (exact count requires opening the device)
      info.numInputChannels = hasInput ? 2 : 0;
      info.numOutputChannels = hasOutput ? 2 : 0;

      // Populate more details if this matches the currently open device
      auto* currentDevice = deviceManager_->getCurrentAudioDevice();
      if (currentDevice != nullptr &&
          currentDevice->getName() == name &&
          currentDevice->getTypeName() == type->getTypeName())
      {
        info.supportedSampleRates = currentDevice->getAvailableSampleRates();
        info.supportedBufferSizes = currentDevice->getAvailableBufferSizes();
        info.numInputChannels = currentDevice->getActiveInputChannels().countNumberOfSetBits();
        info.numOutputChannels = currentDevice->getActiveOutputChannels().countNumberOfSetBits();
        info.isDefault = true;
      }

      devices.push_back(info);
    }
  }
  
  return devices;
}

TransportProtocolAgent::DeviceInfo TransportProtocolAgent::getDefaultInputDevice() {
  // TODO: Get platform default input device
  DeviceInfo info;
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
  
  for (auto* type : deviceManager_->getAvailableDeviceTypes())
  {
    type->scanForDevices();

    auto inputNames = type->getDeviceNames(true);
    auto outputNames = type->getDeviceNames(false);

    bool hasInput = inputNames.contains(deviceId);
    bool hasOutput = outputNames.contains(deviceId);

    if (hasInput || hasOutput)
    {
      // Switch to the correct backend type
      deviceManager_->setCurrentAudioDeviceType(type->getTypeName(), true);

      // Configure the device
      juce::AudioDeviceManager::AudioDeviceSetup setup;
      deviceManager_->getAudioDeviceSetup(setup);

      setup.inputDeviceName = hasInput ? deviceId : juce::String();
      setup.outputDeviceName = hasOutput ? deviceId : juce::String();
      setup.sampleRate = sampleRate;
      setup.bufferSize = bufferSize;
      setup.useDefaultInputChannels = hasInput;
      setup.useDefaultOutputChannels = hasOutput;

      juce::String error = deviceManager_->setAudioDeviceSetup(setup, true);

      if (error.isNotEmpty())
      {
        currentState_ = DeviceState::Error;
        return false;
      }

      currentState_ = DeviceState::Active;
      return true;
    }
  }
  
  // Device not found
  return false;
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

//==============================================================================
// AudioBufferConverter Implementation

void AudioBufferConverter::convertToPlanarFloat(const void* sourceData,
                                                juce::AudioBuffer<float>& destBuffer,
                                                int numSamples,
                                                int numChannels,
                                                BitDepth sourceFormat)
{
    // Resize buffer if needed (though usually caller handles this)
    destBuffer.setSize(numChannels, numSamples, false, false, true);

    const int bytesPerSample = getBytesPerSample(sourceFormat);
    const int strideBytes = bytesPerSample * numChannels;
    const char* rawSrc = static_cast<const char*>(sourceData);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* destChannel = destBuffer.getWritePointer(ch);
        const void* channelSrc = rawSrc + (ch * bytesPerSample);

        switch (sourceFormat)
        {
            case BitDepth::Int16:
                // srcBytesPerSample arg handles the stride for us
                juce::AudioDataConverters::convertInt16LEToFloat(
                    channelSrc, destChannel, numSamples, strideBytes);
                break;

            case BitDepth::Int24:
                juce::AudioDataConverters::convertInt24LEToFloat(
                    channelSrc, destChannel, numSamples, strideBytes);
                break;

            case BitDepth::Int32:
                juce::AudioDataConverters::convertInt32LEToFloat(
                    channelSrc, destChannel, numSamples, strideBytes);
                break;

            case BitDepth::Float32:
                juce::AudioDataConverters::convertFloat32LEToFloat(
                    channelSrc, destChannel, numSamples, strideBytes);
                break;
        }
    }
}

void AudioBufferConverter::convertFromPlanarFloat(const juce::AudioBuffer<float>& sourceBuffer,
                                                  void* destData,
                                                  int numSamples,
                                                  int numChannels,
                                                  BitDepth destFormat)
{
    const int bytesPerSample = getBytesPerSample(destFormat);
    const int strideBytes = bytesPerSample * numChannels;
    char* rawDest = static_cast<char*>(destData);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (ch >= sourceBuffer.getNumChannels()) break;

        const float* srcChannel = sourceBuffer.getReadPointer(ch);
        void* channelDest = rawDest + (ch * bytesPerSample);

        switch (destFormat)
        {
            case BitDepth::Int16:
                juce::AudioDataConverters::convertFloatToInt16LE(
                    srcChannel, channelDest, numSamples, strideBytes);
                break;

            case BitDepth::Int24:
                juce::AudioDataConverters::convertFloatToInt24LE(
                    srcChannel, channelDest, numSamples, strideBytes);
                break;

            case BitDepth::Int32:
                juce::AudioDataConverters::convertFloatToInt32LE(
                    srcChannel, channelDest, numSamples, strideBytes);
                break;

            case BitDepth::Float32:
                juce::AudioDataConverters::convertFloatToFloat32LE(
                    srcChannel, channelDest, numSamples, strideBytes);
                break;
        }
    }
}

} // namespace agents
} // namespace zenith
