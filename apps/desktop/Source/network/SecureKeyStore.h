/*
  ==============================================================================

    SecureKeyStore.h
    Created: 2025-11-29


    Platform-specific secure storage for API keys
    - Windows: DPAPI (Data Protection API)
    - macOS: Keychain Services
    - Linux: Secret Service API (libsecret)

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>

namespace zenith {

/**
    Secure storage for API keys using platform-specific encryption
    
    Keys are never stored in plaintext. Each platform uses its native
    secure storage mechanism:
    - Windows: DPAPI encrypts data with user's login credentials
    - macOS: Keychain provides hardware-backed encryption
    - Linux: Secret Service API with keyring integration
*/
class SecureKeyStore
{
public:
    //==========================================================================
    /**
        Store an API key securely
        
        @param keyName Identifier for the key (e.g., "grok_api_key")
        @param keyValue The API key to store (will be encrypted)
        @return true if successfully stored, false on error
    */
    static bool storeKey(const juce::String& keyName, const juce::String& keyValue);
    
    /**
        Retrieve an API key
        
        @param keyName Identifier for the key
        @param outKey Output parameter for the decrypted key
        @return true if key exists and was retrieved, false otherwise
    */
    static bool retrieveKey(const juce::String& keyName, juce::String& outKey);
    
    /**
        Check if a key exists
        
        @param keyName Identifier for the key
        @return true if key exists in secure storage
    */
    static bool hasKey(const juce::String& keyName);
    
    /**
        Delete a stored key
        
        @param keyName Identifier for the key to delete
        @return true if successfully deleted
    */
    static bool deleteKey(const juce::String& keyName);
    
    /**
        Clear all stored keys (use with caution!)
        
        @return true if all keys cleared successfully
    */
    static bool clearAllKeys();
    
    //==========================================================================
    // Predefined key names for common API keys
    static const juce::String GrokAPIKey;
    static const juce::String OpenAIAPIKey;
    static const juce::String AnthropicAPIKey;
    
private:
    //==========================================================================
    // Platform-specific implementations
    
    // Platform specific implementations are now handled in separate files

    
    // Service name for keychain/credential manager
    static juce::String getServiceName();
    
    JUCE_DECLARE_NON_COPYABLE(SecureKeyStore)
};

} // namespace zenith
