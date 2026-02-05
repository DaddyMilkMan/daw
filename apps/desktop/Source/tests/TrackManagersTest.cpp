/**
 * @file TrackManagersTest.cpp
 * @brief Unit tests for Track Managers (Sidechain, Send, Plugin)
 * @author Zenith DAW Testing Team
 *
 * Framework: JUCE UnitTest
 */

#include "TestUtils.h"
#include "../engine/AudioTrack.h"
#include "../engine/MixerChannel.h"
#include "../engine/TrackPluginManager.h"
#include "../engine/TrackProcessor.h"
#include "../engine/TrackSendManager.h"
#include "../engine/TrackSidechain.h"
#include <juce_core/juce_core.h>

namespace zenith {
namespace tests {

/**
 * @class TrackSidechainTests
 * @brief Tests for TrackSidechain functionality
 */
class TrackSidechainTests : public juce::UnitTest {
public:
  TrackSidechainTests() : juce::UnitTest("Track Sidechain", "TrackManagers") {}

  void runTest() override {
    beginTest("Stores and clears source");
    {
      TrackProcessor processor;
      TrackSidechain sidechain(processor);
      auto source = std::make_shared<AudioTrack>("Source");

      sidechain.setPluginSidechainSource(0, source);
      expect(sidechain.getSidechainSource().get() == source.get());

      sidechain.setPluginSidechainSource(0, nullptr);
      expect(sidechain.getSidechainSource() == nullptr);
    }
  }
};

/**
 * @class TrackSendManagerTests
 * @brief Tests for TrackSendManager functionality
 */
class TrackSendManagerTests : public juce::UnitTest {
public:
  TrackSendManagerTests() : juce::UnitTest("Track Send Manager", "TrackManagers") {}

  void runTest() override {
    beginTest("Destinations and levels");
    {
      MixerChannel mixerChannel;
      TrackSendManager sendManager(mixerChannel);

      expectEquals(sendManager.getSendDestination(0), -1);
      sendManager.setSendDestination(0, 2);
      expectEquals(sendManager.getSendDestination(0), 2);

      sendManager.setSendLevel(0, 0.5f);
      expect(std::abs(sendManager.getSendLevel(0) - 0.5f) < 1e-6f);

      sendManager.setSendPreFader(0, true);
      expect(sendManager.isSendPreFader(0));
    }
  }
};

/**
 * @class TrackPluginManagerTests
 * @brief Tests for TrackPluginManager functionality
 */
class TrackPluginManagerTests : public juce::UnitTest {
public:
  TrackPluginManagerTests() : juce::UnitTest("Track Plugin Manager", "TrackManagers") {}

  void runTest() override {
    beginTest("Add and remove plugins");
    {
      TrackProcessor processor;
      double sampleRate = 48000.0;
      int blockSize = 512;
      TrackPluginManager pluginManager(processor, sampleRate, blockSize);

      auto plugin = std::make_unique<StubAudioPlugin>();
      pluginManager.addPlugin(std::move(plugin));
      expectEquals(pluginManager.getNumPlugins(), 1);
      expect(pluginManager.getPlugin(0) != nullptr);

      pluginManager.clearPlugins();
      expectEquals(pluginManager.getNumPlugins(), 0);
    }
  }
};

// Static test registration instances
static TrackSidechainTests trackSidechainTests;
static TrackSendManagerTests trackSendManagerTests;
static TrackPluginManagerTests trackPluginManagerTests;

} // namespace tests
} // namespace zenith
