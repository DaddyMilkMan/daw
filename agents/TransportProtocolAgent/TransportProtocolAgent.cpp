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
  
  auto* currentDevice = deviceManager_->getCurrentAudioDevice();
  const juce::String currentDeviceName = currentDevice ? currentDevice->getName() : "";
  const juce::String currentDeviceType = currentDevice ? currentDevice->getTypeName() : "";

  const auto& deviceTypes = deviceManager_->getAvailableDeviceTypes();

  for (auto* type : deviceTypes)
  {
      type->scanForDevices();
      juce::String typeName = type->getTypeName();

      if (typeName == "ASIO")
      {
          juce::StringArray devNames = type->getDeviceNames();
          for (const auto& name : devNames)
          {
              DeviceInfo info;
              info.name = name;
              info.id = name;
              info.apiType = typeName;

              if (currentDevice != nullptr && currentDeviceName == name && currentDeviceType == typeName)
              {
                  info.numInputChannels = currentDevice->getActiveInputChannels().countNumberOfSetBits();
                  info.numOutputChannels = currentDevice->getActiveOutputChannels().countNumberOfSetBits();
                  info.supportedSampleRates = currentDevice->getAvailableSampleRates();
                  info.supportedBufferSizes = currentDevice->getAvailableBufferSizes();
                  info.isDefault = true;
              }
              else
              {
                  // Shallow info for ASIO - assume availability
                  info.numInputChannels = 2;
                  info.numOutputChannels = 2;
                  info.isDefault = false;
              }
              devices.push_back(info);
          }
      }
      else
      {
          juce::StringArray inputNames = type->getDeviceNames(true);
          juce::StringArray outputNames = type->getDeviceNames(false);
          int defaultInputIndex = type->getDefaultDeviceIndex(true);
          int defaultOutputIndex = type->getDefaultDeviceIndex(false);

          // Inputs
          for (int i = 0; i < inputNames.size(); ++i)
          {
              DeviceInfo info;
              info.name = inputNames[i];
              info.id = inputNames[i];
              info.apiType = typeName;

              info.numInputChannels = 2; // Placeholder indicating input capability
              info.numOutputChannels = 0;

              info.isDefault = (i == defaultInputIndex);

              if (currentDevice != nullptr && currentDeviceName == info.name && currentDeviceType == typeName)
              {
                   // If this specific device is active, update with real info
                   info.numInputChannels = currentDevice->getActiveInputChannels().countNumberOfSetBits();
                   info.supportedSampleRates = currentDevice->getAvailableSampleRates();
                   info.supportedBufferSizes = currentDevice->getAvailableBufferSizes();
              }

              devices.push_back(info);
          }

          // Outputs
          for (int i = 0; i < outputNames.size(); ++i)
          {
              DeviceInfo info;
              info.name = outputNames[i];
              info.id = outputNames[i];
              info.apiType = typeName;

              info.numInputChannels = 0;
              info.numOutputChannels = 2; // Placeholder indicating output capability

              info.isDefault = (i == defaultOutputIndex);

              if (currentDevice != nullptr && currentDeviceName == info.name && currentDeviceType == typeName)
              {
                   info.numOutputChannels = currentDevice->getActiveOutputChannels().countNumberOfSetBits();
                   info.supportedSampleRates = currentDevice->getAvailableSampleRates();
                   info.supportedBufferSizes = currentDevice->getAvailableBufferSizes();
              }

              devices.push_back(info);
          }
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
