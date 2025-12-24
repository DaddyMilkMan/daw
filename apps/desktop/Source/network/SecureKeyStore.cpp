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

namespace zenith {

// Predefined key names
const juce::String SecureKeyStore::GrokAPIKey = "zenith_grok_api_key";
const juce::String SecureKeyStore::OpenAIAPIKey = "zenith_openai_api_key";
const juce::String SecureKeyStore::AnthropicAPIKey = "zenith_anthropic_api_key";

#if ! (JUCE_LINUX || JUCE_MAC || JUCE_WINDOWS)
juce::String SecureKeyStore::getServiceName() {
  return "com.zenithaudio.zenith-daw";
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
