/*
  ==============================================================================
    agents/TransportProtocolAgent/TransportProtocolAgent.cpp
    Cross-platform audio transport protocol implementation.
  ==============================================================================
*/

#include "TransportProtocolAgent.h"
#include <cstdint>

namespace zenith {
namespace agents {

//==============================================================================
TransportProtocolAgent::TransportProtocolAgent() 
  : deviceManager_(std::make_unique<juce::AudioDeviceManager>()) {
  // Initialize device manager
  deviceManager_->initialiseWithDefaultDevices(2, 2);
}

TransportProtocolAgent::TransportProtocolAgent(std::unique_ptr<juce::AudioDeviceManager> manager)
  : deviceManager_(std::move(manager)) {
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

  // Iterate through all available device types (ASIO, WASAPI, ALSA, etc.)
  for (auto* type : deviceManager_->getAvailableDeviceTypes()) {
    type->scanForDevices();

    juce::StringArray inputNames = type->getDeviceNames(true);
    juce::StringArray outputNames = type->getDeviceNames(false);

    // Identify defaults by name
    juce::String defaultInputName;
    int defInIdx = type->getDefaultDeviceIndex(true);
    if (defInIdx >= 0 && defInIdx < inputNames.size())
      defaultInputName = inputNames[defInIdx];

    juce::String defaultOutputName;
    int defOutIdx = type->getDefaultDeviceIndex(false);
    if (defOutIdx >= 0 && defOutIdx < outputNames.size())
      defaultOutputName = outputNames[defOutIdx];

    // Merge unique device names from inputs and outputs
    juce::StringArray allNames;
    allNames.addArray(inputNames);
    
    for (const auto& outName : outputNames) {
      if (!allNames.contains(outName)) {
        allNames.add(outName);
      }
    }

    for (const auto& name : allNames) {
      DeviceInfo info;
      info.name = name;
      info.id = name; // Using name as ID is standard for simple device types
      info.apiType = type->getTypeName();

      // Mark as default if it matches either default input or output
      info.isDefault = (name == defaultInputName || name == defaultOutputName);

      // Check if this is the currently active device
      bool isActive = (currentDevice != nullptr &&
                       currentDevice->getName() == name &&
                       currentDevice->getTypeName() == info.apiType);

      if (isActive) {
        // For active device: Query full capabilities
        info.numInputChannels = currentDevice->getActiveInputChannels().countNumberOfSetBits();
        info.numOutputChannels = currentDevice->getActiveOutputChannels().countNumberOfSetBits();
        info.supportedSampleRates = currentDevice->getAvailableSampleRates();
        info.supportedBufferSizes = currentDevice->getAvailableBufferSizes();
      } else {
        // For inactive devices: Return empty/zero to avoid opening the device (performance)
        info.numInputChannels = 0;
        info.numOutputChannels = 0;
      }

      devices.push_back(info);
    }
  }
  
  return devices;
}

TransportProtocolAgent::DeviceInfo TransportProtocolAgent::getDefaultInputDevice() {
  DeviceInfo info;
  info.name = "None";
  info.id = "";
  info.isDefault = false;

  if (deviceManager_ == nullptr)
      return info;

  // Define priority order based on platform
  juce::StringArray searchOrder;

  // Check currently active type first
  juce::String activeType = deviceManager_->getCurrentAudioDeviceType();
  if (activeType.isNotEmpty())
      searchOrder.add(activeType);

#if JUCE_WINDOWS
  searchOrder.add("ASIO");
  searchOrder.add("Windows Audio");
  searchOrder.add("DirectSound");
#elif JUCE_LINUX
  searchOrder.add("JACK");
  searchOrder.add("ALSA");
#elif JUCE_MAC
  searchOrder.add("CoreAudio");
#endif

  // Remove duplicates (keep first occurrence - effectively active type stays first)
  for (int i = searchOrder.size() - 1; i > 0; --i)
  {
      if (searchOrder.indexOf(searchOrder[i]) < i)
          searchOrder.remove(i);
  }

  const auto& availableTypes = deviceManager_->getAvailableDeviceTypes();

  for (const auto& typeName : searchOrder)
  {
      for (auto* type : availableTypes)
      {
          if (type != nullptr && type->getTypeName() == typeName)
          {
              type->scanForDevices();
              juce::StringArray deviceNames = type->getDeviceNames(true); // true for input
              int defaultIndex = type->getDefaultDeviceIndex(true);

              if (defaultIndex >= 0 && defaultIndex < deviceNames.size())
              {
                  info.name = deviceNames[defaultIndex];
                  info.id = info.name; // In JUCE, name is typically used as ID
                  info.apiType = typeName;
                  info.isDefault = true;
                  
                  // Enrich with details if it matches current device (Best effort)
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
  }

  return info;
}

TransportProtocolAgent::DeviceInfo TransportProtocolAgent::getDefaultOutputDevice() {
  DeviceInfo info;
  info.name = "None";
  info.id = "";
  info.isDefault = false;

  if (deviceManager_ == nullptr)
      return info;

  // Define priority order based on platform
  juce::StringArray searchOrder;

  // Check currently active type first
  juce::String activeType = deviceManager_->getCurrentAudioDeviceType();
  if (activeType.isNotEmpty())
      searchOrder.add(activeType);

#if JUCE_WINDOWS
  searchOrder.add("ASIO");
  searchOrder.add("Windows Audio");
  searchOrder.add("DirectSound");
#elif JUCE_LINUX
  searchOrder.add("JACK");
  searchOrder.add("ALSA");
#elif JUCE_MAC
  searchOrder.add("CoreAudio");
#endif

  // Remove duplicates (keep first occurrence - effectively active type stays first)
  for (int i = searchOrder.size() - 1; i > 0; --i)
  {
      if (searchOrder.indexOf(searchOrder[i]) < i)
          searchOrder.remove(i);
  }

  const auto& availableTypes = deviceManager_->getAvailableDeviceTypes();

  for (const auto& typeName : searchOrder)
  {
      for (auto* type : availableTypes)
      {
          if (type != nullptr && type->getTypeName() == typeName)
          {
              type->scanForDevices();
              juce::StringArray deviceNames = type->getDeviceNames(false); // false for output
              int defaultIndex = type->getDefaultDeviceIndex(false);

              if (defaultIndex >= 0 && defaultIndex < deviceNames.size())
              {
                  info.name = deviceNames[defaultIndex];
                  info.id = info.name;
                  info.apiType = typeName;
                  info.isDefault = true;

                  // Enrich with details if it matches current device
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
  }

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

    using DestFormat = juce::AudioData::Format<juce::AudioData::Float32,
                                               juce::AudioData::NativeEndian>;
    auto* const* destPtrs = destBuffer.getArrayOfWritePointers();

    switch (sourceFormat)
    {
        case BitDepth::Int16: {
            using SourceFormat = juce::AudioData::Format<juce::AudioData::Int16,
                                                         juce::AudioData::LittleEndian>;
            juce::AudioData::deinterleaveSamples(
                juce::AudioData::InterleavedSource<SourceFormat>{
                    static_cast<const std::uint16_t*>(sourceData), numChannels},
                juce::AudioData::NonInterleavedDest<DestFormat>{destPtrs, numChannels},
                numSamples);
            break;
        }
        case BitDepth::Int24: {
            using SourceFormat = juce::AudioData::Format<juce::AudioData::Int24,
                                                         juce::AudioData::LittleEndian>;
            juce::AudioData::deinterleaveSamples(
                juce::AudioData::InterleavedSource<SourceFormat>{
                    static_cast<const char*>(sourceData), numChannels},
                juce::AudioData::NonInterleavedDest<DestFormat>{destPtrs, numChannels},
                numSamples);
            break;
        }
        case BitDepth::Int32: {
            using SourceFormat = juce::AudioData::Format<juce::AudioData::Int32,
                                                         juce::AudioData::LittleEndian>;
            juce::AudioData::deinterleaveSamples(
                juce::AudioData::InterleavedSource<SourceFormat>{
                    static_cast<const std::uint32_t*>(sourceData), numChannels},
                juce::AudioData::NonInterleavedDest<DestFormat>{destPtrs, numChannels},
                numSamples);
            break;
        }
        case BitDepth::Float32: {
            using SourceFormat = juce::AudioData::Format<juce::AudioData::Float32,
                                                         juce::AudioData::LittleEndian>;
            juce::AudioData::deinterleaveSamples(
                juce::AudioData::InterleavedSource<SourceFormat>{
                    static_cast<const float*>(sourceData), numChannels},
                juce::AudioData::NonInterleavedDest<DestFormat>{destPtrs, numChannels},
                numSamples);
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
    using SourceFormat = juce::AudioData::Format<juce::AudioData::Float32,
                                                 juce::AudioData::NativeEndian>;
    auto* const* sourcePtrs = sourceBuffer.getArrayOfReadPointers();

    switch (destFormat)
    {
        case BitDepth::Int16: {
            using DestFormat = juce::AudioData::Format<juce::AudioData::Int16,
                                                       juce::AudioData::LittleEndian>;
            juce::AudioData::interleaveSamples(
                juce::AudioData::NonInterleavedSource<SourceFormat>{sourcePtrs, numChannels},
                juce::AudioData::InterleavedDest<DestFormat>{
                    static_cast<std::uint16_t*>(destData), numChannels},
                numSamples);
            break;
        }
        case BitDepth::Int24: {
            using DestFormat = juce::AudioData::Format<juce::AudioData::Int24,
                                                       juce::AudioData::LittleEndian>;
            juce::AudioData::interleaveSamples(
                juce::AudioData::NonInterleavedSource<SourceFormat>{sourcePtrs, numChannels},
                juce::AudioData::InterleavedDest<DestFormat>{
                    static_cast<char*>(destData), numChannels},
                numSamples);
            break;
        }
        case BitDepth::Int32: {
            using DestFormat = juce::AudioData::Format<juce::AudioData::Int32,
                                                       juce::AudioData::LittleEndian>;
            juce::AudioData::interleaveSamples(
                juce::AudioData::NonInterleavedSource<SourceFormat>{sourcePtrs, numChannels},
                juce::AudioData::InterleavedDest<DestFormat>{
                    static_cast<std::uint32_t*>(destData), numChannels},
                numSamples);
            break;
        }
        case BitDepth::Float32: {
            using DestFormat = juce::AudioData::Format<juce::AudioData::Float32,
                                                       juce::AudioData::LittleEndian>;
            juce::AudioData::interleaveSamples(
                juce::AudioData::NonInterleavedSource<SourceFormat>{sourcePtrs, numChannels},
                juce::AudioData::InterleavedDest<DestFormat>{
                    static_cast<float*>(destData), numChannels},
                numSamples);
            break;
        }
    }
}

} // namespace agents
} // namespace zenith
