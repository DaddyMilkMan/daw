/*
  ==============================================================================

    SecureKeyStore.cpp
    Created: 2025-11-29

    Platform-agnostic secure storage implementation.
    Platform-specific implementations are in platform/

  ==============================================================================
*/

#include "SecureKeyStore.h"
#include "../engine/ZenithLogger.h"
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

// Predefined key names
const juce::String SecureKeyStore::GrokAPIKey = "zenith_grok_api_key";
const juce::String SecureKeyStore::OpenAIAPIKey = "zenith_openai_api_key";
const juce::String SecureKeyStore::AnthropicAPIKey = "zenith_anthropic_api_key";



//==============================================================================
// Public API - Common Implementations
//==============================================================================

// Fallback implementation for non-Linux platforms
#ifndef __linux__

juce::String SecureKeyStore::getServiceName() {
  return "com.zenithaudio.zenith-daw";
}

bool SecureKeyStore::storeKey(const juce::String &keyName,
                             const juce::String &keyValue) {
  juce::PropertiesFile::Options options;
  options.applicationName = "ZenithDAW";
  options.filenameSuffix = ".settings";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsXML;

  juce::PropertiesFile props(options);
  props.setValue(keyName, keyValue);
  return props.saveIfNeeded();
}

bool SecureKeyStore::retrieveKey(const juce::String &keyName,
                                juce::String &outKey) {
  // Check environment variables first
  juce::String envVarName = keyName.toUpperCase().replace(" ", "_");
  juce::String envVal = juce::SystemStats::getEnvironmentVariable(envVarName, "");
  if (envVal.isNotEmpty()) {
    outKey = envVal;
    return true;
  }

  // Fallback to properties file
  juce::PropertiesFile::Options options;
  options.applicationName = "ZenithDAW";
  options.filenameSuffix = ".settings";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsXML;

  juce::PropertiesFile props(options);
  if (props.containsKey(keyName)) {
    outKey = props.getValue(keyName);
    return true;
  }

  return false;
}

bool SecureKeyStore::deleteKey(const juce::String &keyName) {
  juce::PropertiesFile::Options options;
  options.applicationName = "ZenithDAW";
  options.filenameSuffix = ".settings";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsXML;

  juce::PropertiesFile props(options);
  props.removeValue(keyName);
  return props.saveIfNeeded();
}

bool SecureKeyStore::hasKey(const juce::String &keyName) {
  juce::String dummy;
  return retrieveKey(keyName, dummy);
}

bool SecureKeyStore::clearAllKeys() {
  bool success = true;
  success &= deleteKey(GrokAPIKey);
  success &= deleteKey(OpenAIAPIKey);
  success &= deleteKey(AnthropicAPIKey);
  return success;
}

#endif

} // namespace zenith
