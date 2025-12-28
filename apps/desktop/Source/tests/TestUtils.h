/*
  ==============================================================================

    TestUtils.h
    Created: 2025-12-03
    Author:  Zenith DAW

    Shared utilities for unit testing, including mock objects.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace zenith {
namespace tests {

/**
 * @class StubAudioPlugin
 * @brief A base class for mock plugins that implements all pure virtual methods
 * with no-ops.
 *
 * Inherit from this and override only the methods you need to test.
 */
class StubAudioPlugin : public juce::AudioPluginInstance {
public:
  StubAudioPlugin()
      : juce::AudioPluginInstance(
            juce::AudioProcessor::BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

  //==============================================================================
  // Required AudioProcessor overrides (No-ops)
  void prepareToPlay(double, int) override {}
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override {}

  //==============================================================================
  // Metadata overrides (Defaults)
  const juce::String getName() const override { return "Stub Plugin"; }
  double getTailLengthSeconds() const override { return 0.0; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }

  //==============================================================================
  // Editor overrides (No editor)
  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  //==============================================================================
  // Program/State overrides (No-ops)
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return "Default"; }
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override {}
  void setStateInformation(const void *, int) override {}

  //==============================================================================
  // AudioPluginInstance pure virtual override
  void fillInPluginDescription(juce::PluginDescription &desc) const override {
    desc.name = getName();
    desc.pluginFormatName = "Stub";
    desc.category = "Test";
    desc.manufacturerName = "Zenith Tests";
    desc.version = "1.0";
    desc.version = "1.0";
    desc.uniqueId = 0;
  }

  // Helper to expose addParameter for test setups
  void addTestParameter(juce::AudioProcessorParameter* param) {
      juce::AudioProcessor::addParameter(param);
  }
};

/**
 * @class MockAudioIODevice
 * @brief A stub audio I/O device for testing AudioSource callbacks without hardware.
 */
class MockAudioIODevice : public juce::AudioIODevice {
public:
    MockAudioIODevice(const juce::String& deviceName) 
        : AudioIODevice(deviceName, "Mock"), name(deviceName) {}

    bool isOpen() override { return isOpen_; }
    
    juce::String open(const juce::BigInteger&, const juce::BigInteger&, double sampleRate, int bufferSizeSamples) override {
        currentSampleRate = sampleRate;
        currentBufferSize = bufferSizeSamples;
        isOpen_ = true;
        return {};
    }
    
    void close() override { isOpen_ = false; }
    
    void start(juce::AudioIODeviceCallback*) override { isStarted_ = true; }
    void stop() override { isStarted_ = false; }
    bool isPlaying() override { return isStarted_; }
    
    int getCurrentBufferSizeSamples() override { return currentBufferSize; }
    double getCurrentSampleRate() override { return currentSampleRate; }
    int getCurrentBitDepth() override { return 16; }
    
    juce::BigInteger getActiveOutputChannels() const override { return juce::BigInteger(3); } // Ch 0, 1
    juce::BigInteger getActiveInputChannels() const override { return juce::BigInteger(3); }  // Ch 0, 1
    
    juce::StringArray getOutputChannelNames() override {
        juce::StringArray names;
        names.add("Out L"); names.add("Out R");
        return names;
    }
    
    juce::StringArray getInputChannelNames() override {
        juce::StringArray names;
        names.add("In L"); names.add("In R");
        return names;
    }
    
    juce::Array<double> getAvailableSampleRates() override {
        return { 44100.0, 48000.0 };
    }
    
    juce::Array<int> getAvailableBufferSizes() override {
        return { 128, 256, 512, 1024 };
    }
    
    int getDefaultBufferSize() override { return 512; }
    
    juce::String getLastError() override { return {}; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }

    juce::String name;
    bool isOpen_ = false;
    bool isStarted_ = false;
    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;
};

} // namespace tests
} // namespace zenith
