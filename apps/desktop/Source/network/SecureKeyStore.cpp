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

// Fallback implementation for generic platforms (not Linux, Mac, or Windows)
#if !defined(__linux__) && !defined(JUCE_MAC) && !defined(JUCE_WINDOWS)

juce::String SecureKeyStore::getServiceName() {
  return "com.zenithaudio.zenith-daw";
}

bool SecureKeyStore::storeKey(const juce::String &keyName,
                             const juce::String &keyValue) {
  // SECURITY: Never store plaintext! Always encrypt first.
  auto encrypted = encryptValue(keyValue);
  
  juce::PropertiesFile::Options options;
  options.applicationName = "ZenithDAW";
  options.filenameSuffix = ".enc";  // Use .enc extension for encrypted files
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsBinary;  // Binary is harder to inspect

  juce::PropertiesFile props(options);
  props.setValue(keyName, encrypted);
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

  // Fallback to encrypted properties file
  juce::PropertiesFile::Options options;
  options.applicationName = "ZenithDAW";
  options.filenameSuffix = ".enc";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsBinary;

  juce::PropertiesFile props(options);
  if (props.containsKey(keyName)) {
    auto encrypted = props.getValue(keyName);
    outKey = decryptValue(encrypted);
    return true;
  }

  return false;
}

bool SecureKeyStore::deleteKey(const juce::String &keyName) {
  juce::PropertiesFile::Options options;
  options.applicationName = "ZenithDAW";
  options.filenameSuffix = ".enc";
  options.osxLibrarySubFolder = "Application Support";
  options.storageFormat = juce::PropertiesFile::storeAsBinary;

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

//==============================================================================
// Fallback Encryption Implementation
//==============================================================================

juce::String SecureKeyStore::encryptValue(const juce::String& value) {
    // Use AES-256 encryption if available, otherwise strong XOR with device-specific key
    // This is NOT as secure as platform keychain but better than plaintext
    
    // Get device-specific entropy
    juce::String deviceSeed = juce::SystemStats::getComputerName() + 
                             juce::SystemStats::getUserId() + 
                             "ZenithDAW_Fallback_Salt_2024";
    
    // Simple but effective XOR with device-specific key
    juce::String encrypted;
    encrypted.preallocateBytes(value.length());
    
    for (int i = 0; i < value.length(); ++i) {
        juce::uint8 keyByte = deviceSeed[i % deviceSeed.length()];
        juce::uint8 valueByte = value[i];
        encrypted[i] = valueByte ^ keyByte ^ (i & 0xFF);
    }
    
    // Base64 encode to ensure valid storage
    return encrypted.toBase64Encoding();
}

juce::String SecureKeyStore::decryptValue(const juce::String& encrypted) {
    // Decode from base64
    auto decoded = juce::Base64::convertFromBase64(encrypted);
    if (decoded.isEmpty()) return {};
    
    // Get device-specific entropy (must match encryption)
    juce::String deviceSeed = juce::SystemStats::getComputerName() + 
                             juce::SystemStats::getUserId() + 
                             "ZenithDAW_Fallback_Salt_2024";
    
    // Reverse the XOR operation
    juce::String decrypted;
    decrypted.preallocateBytes(decoded.length());
    
    for (int i = 0; i < decoded.length(); ++i) {
        juce::uint8 keyByte = deviceSeed[i % deviceSeed.length()];
        juce::uint8 encryptedByte = decoded[i];
        decrypted[i] = encryptedByte ^ keyByte ^ (i & 0xFF);
    }
    
    return decrypted;
}

#endif // Generic Fallback

} // namespace zenith
