/*
  ==============================================================================

    PluginAutomationTests.cpp
    Created: 2025-12-25
    Author:  Zenith DAW

    Tests for plugin parameter automation binding and smoothing.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "TestUtils.h"
#include "../../engine/PluginChain.h"
#include "../../engine/PluginAutomationBinding.h"

namespace zenith {
namespace tests {

// A mock plugin that exposes a few parameters for testing
class MockPluginWithParameters : public StubAudioPlugin {
public:
  MockPluginWithParameters() {
    addTestParameter(new juce::AudioParameterFloat(juce::ParameterID("gain", 1), "Gain", 0.0f, 1.0f, 0.5f));
    addTestParameter(new juce::AudioParameterFloat(juce::ParameterID("cutoff", 1), "Cutoff", 20.0f, 20000.0f, 1000.0f));
  }
};

class PluginAutomationTests : public juce::UnitTest {
public:
  PluginAutomationTests() : juce::UnitTest("PluginAutomationTests", "Automation") {}

  void runTest() override {
    beginTest("Binding Creation");
    {
      PluginChain chain;
      auto plugin = std::make_unique<MockPluginWithParameters>();
      chain.addPlugin(std::move(plugin), 44100.0, 512);

      // Verify initial state
      auto params = chain.getAutomatableParameters(0);
      expectEquals((int)params.size(), 2);
      expectEquals(params[0].name, juce::String("Gain"));

      // Set parameter value should create binding
      chain.setParameterValue(0, 0, 0.8f);

      // We can't easily inspect internal bindings without friend access or getters,
      // but we can verify the parameter was updated on the plugin itself eventually.
      // However, PluginChain::setParameterValue updates the binding's target,
      // and the binding updates the plugin during process().
      // Actually, setParameterValue also prepares the binding.
      
      // Let's verify via the plugin parameter directly if possible, 
      // although the binding applies it during audio processing usually.
      // But PluginAutomationBinding calls setValueNotifyingHost immediately?
      // No, applyAutomation() is called during process.
      
      // Let's simulate processing
      juce::AudioBuffer<float> buffer(2, 512);
      juce::MidiBuffer midi;
      
      chain.prepareToPlay(44100.0, 512);
      
      // The binding smooths the value. 
      // If we process a block, the parameter should move towards target.
      chain.process(buffer, midi);
      
      // Check parameter value
      auto* pluginPtr = chain.getPlugin(0);
      auto* param = pluginPtr->getParameters()[0];
      float val = param->getValue();
      expectGreaterThan(val, 0.5f); // Should have moved from 0.5 towards 0.8
    }

    beginTest("Parameter Smoothing");
    {
      auto plugin = std::make_shared<MockPluginWithParameters>();
      // Directly test binding logic
      PluginAutomationBinding binding(plugin.get(), 0); // Gain param
      binding.prepare(44100.0);
      
      // Initial value is 0.5
      binding.setTargetValue(1.0f); // Jump to 1.0
      
      // Process small chunk
      binding.applyAutomation(100);
      
      float val = plugin->getParameters()[0]->getValue();
      expectGreaterThan(val, 0.5f);
      expectLessThan(val, 1.0f); // Should be ramping
      
      // Process huge chunk to finish ramp
      binding.applyAutomation(44100);
      val = plugin->getParameters()[0]->getValue();
      expectEquals(val, 1.0f);
    }
  }
};

static PluginAutomationTests pluginAutomationTests;

} // namespace tests
} // namespace zenith
