/*
  ==============================================================================

    InternalPluginFormat.cpp
    Created: 2025-12-24
    Author:  Zenith DAW

  ==============================================================================
*/

#include "InternalPluginFormat.h"

namespace zenith {

InternalPluginFormat::InternalPluginFormat() {}

InternalPluginFormat::~InternalPluginFormat() {}

void InternalPluginFormat::findAllTypesForFile(
    juce::OwnedArray<juce::PluginDescription> &results,
    const juce::String &fileOrIdentifier) {
  // No external file scanning
}

bool InternalPluginFormat::fileMightContainThisPluginType(
    const juce::String &fileOrIdentifier) {
  return fileOrIdentifier.startsWith("zenith:");
}

juce::String InternalPluginFormat::getNameOfPluginFromIdentifier(
    const juce::String &fileOrIdentifier) {
  return fileOrIdentifier.fromFirstOccurrenceOf(":", false, false);
}

void InternalPluginFormat::createPluginInstance(
    const juce::PluginDescription &description, double initialSampleRate,
    int initialBufferSize, PluginCreationCallback callback) {
  // For now, fail gracefully
  callback(nullptr, "Internal plugins not yet implemented");
}

} // namespace zenith
