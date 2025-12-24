/*
  ==============================================================================

    InternalPluginFormat.cpp
    Created: 2025-12-19
    Author:  Zenith DAW

  ==============================================================================
*/

#include "InternalPluginFormat.h"
#include "../effects/ZenithChannelStrip.h"
#include "../effects/ZenithDeEsser.h"
#include "../effects/ZenithTransientShaper.h"
#include "../effects/ZenithVoiceChanger.h"
#include "../modulation/ZenithTremolo.h"

namespace zenith {

InternalPluginFormat::InternalPluginFormat() { registerBuiltInPlugins(); }

InternalPluginFormat::~InternalPluginFormat() {}

InternalPluginFormat &InternalPluginFormat::getInstance() {
  static InternalPluginFormat instance;
  return instance;
}

void InternalPluginFormat::registerPlugin(const juce::String &identifier,
                                          Creator creator) {
  if (creator) {
    // Create a temporary instance to get the description
    auto instance = creator();
    if (instance) {
      juce::PluginDescription desc;
      instance->fillInPluginDescription(desc);

      // Ensure ID matches
      desc.fileOrIdentifier = identifier;

      creators[identifier] = creator;
      descriptions[identifier] = desc;
    }
  }
}

void InternalPluginFormat::registerBuiltInPlugins() {
  // Phase 1: Tracer Bullet
  registerPlugin("zenith.internal.tremolo",
                 []() { return std::make_unique<ZenithTremolo>(); });

  // Phase 2: Mixing Suite
  registerPlugin("zenith.internal.channelstrip",
                 []() { return std::make_unique<ZenithChannelStrip>(); });
  registerPlugin("zenith.internal.deesser",
                 []() { return std::make_unique<ZenithDeEsser>(); });
  registerPlugin("zenith.internal.transientshaper",
                 []() { return std::make_unique<ZenithTransientShaper>(); });

  // Phase 3: AI/Creative
  registerPlugin("zenith.internal.voicechanger",
                 []() { return std::make_unique<ZenithVoiceChanger>(); });
}

//==============================================================================
void InternalPluginFormat::findAllTypesForFile(
    juce::OwnedArray<juce::PluginDescription> &results,
    const juce::String &fileOrIdentifier) {
  // If identifier is passed, find specific
  auto it = descriptions.find(fileOrIdentifier);
  if (it != descriptions.end()) {
    results.add(new juce::PluginDescription(it->second));
    return;
  }
    return;
  }

  // If empty or special "internal", return all
  if (fileOrIdentifier.isEmpty() || fileOrIdentifier == "Zenith Internal") {
    for (const auto &pair : descriptions)
      results.add(new juce::PluginDescription(pair.second));
  }
}

bool InternalPluginFormat::fileMightContainThisPluginType(
    const juce::String &fileOrIdentifier) {
  return fileOrIdentifier.startsWith("zenith.internal.");
}

juce::String InternalPluginFormat::getNameOfPluginFromIdentifier(
    const juce::String &fileOrIdentifier) {
  auto it = descriptions.find(fileOrIdentifier);
  if (it != descriptions.end())
    return it->second.name;
  return {};
}

bool InternalPluginFormat::doesPluginStillExist(
    const juce::PluginDescription &desc) {
  return descriptions.count(desc.fileOrIdentifier) > 0;
}

void InternalPluginFormat::createPluginInstance(
    const juce::PluginDescription &desc, double initialSampleRate,
    int initialBlockSize, PluginCreationCallback callback) {
  auto it = creators.find(desc.fileOrIdentifier);
  if (it != creators.end()) {
    auto instance = it->second();
    if (instance) {
      instance->prepareToPlay(initialSampleRate, initialBlockSize);
      callback(std::move(instance), {});
      return;
    }
  }
  callback(nullptr, "Internal plugin not found");
}

std::unique_ptr<juce::AudioPluginInstance>
InternalPluginFormat::createInstanceFromDescription(
    const juce::PluginDescription &desc, double initialSampleRate,
    int initialBlockSize, juce::String &errorMessage) {
  auto it = creators.find(desc.fileOrIdentifier);
  if (it != creators.end()) {
    auto instance = it->second();
    if (instance) {
      instance->prepareToPlay(initialSampleRate, initialBlockSize);
      return instance;
    }
  }

  errorMessage = "Internal plugin not found: " + desc.fileOrIdentifier;
  return nullptr;
}

} // namespace zenith
