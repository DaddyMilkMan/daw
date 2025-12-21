/*
  ==============================================================================

    SecureKeyStore.cpp
    Created: 2025-11-29


    Platform-specific secure storage implementation

  ==============================================================================
*/

#include "SecureKeyStore.h"
#include "../engine/ZenithLogger.h"

#if JUCE_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
#elif JUCE_MAC
#include <Security/Security.h>
#elif JUCE_LINUX
// Will use JUCE's built-in encryption as fallback for Linux
#include <juce_cryptography/juce_cryptography.h>
#endif

namespace zenith {

// Predefined key names
const juce::String SecureKeyStore::GrokAPIKey = "zenith_grok_api_key";
const juce::String SecureKeyStore::OpenAIAPIKey = "zenith_openai_api_key";
const juce::String SecureKeyStore::AnthropicAPIKey = "zenith_anthropic_api_key";
const juce::String SecureKeyStore::ZenithAuthToken = "zenith_auth_token";
const juce::String SecureKeyStore::ZenithLicenseData = "zenith_license_data";

juce::String SecureKeyStore::getServiceName() {
  return "com.zenithaudio.zenith-daw";
}

//==============================================================================
// Public API
//==============================================================================

bool SecureKeyStore::storeKey(const juce::String &keyName,
                              const juce::String &keyValue) {
  if (keyName.isEmpty() || keyValue.isEmpty())
    return false;

#if JUCE_WINDOWS
  ZENITH_LOG_INFO("SecureKeyStore: Calling storeKeyWindows");
  return storeKeyWindows(keyName, keyValue);
#elif JUCE_MAC
  return storeKeyMac(keyName, keyValue);
#elif JUCE_LINUX
  return storeKeyLinux(keyName, keyValue);
#else
  return false;
#endif
}

bool SecureKeyStore::retrieveKey(const juce::String &keyName,
                                 juce::String &outKey) {
  if (keyName.isEmpty())
    return false;

#if JUCE_WINDOWS
  ZENITH_LOG_INFO("SecureKeyStore: Calling retrieveKeyWindows for " + keyName);
  return retrieveKeyWindows(keyName, outKey);
#elif JUCE_MAC
  return retrieveKeyMac(keyName, outKey);
#elif JUCE_LINUX
  return retrieveKeyLinux(keyName, outKey);
#else
  return false;
#endif
}

bool SecureKeyStore::hasKey(const juce::String &keyName) {
  juce::String dummy;
  return retrieveKey(keyName, dummy);
}

bool SecureKeyStore::deleteKey(const juce::String &keyName) {
  if (keyName.isEmpty())
    return false;

#if JUCE_WINDOWS
  return deleteKeyWindows(keyName);
#elif JUCE_MAC
  return deleteKeyMac(keyName);
#elif JUCE_LINUX
  return deleteKeyLinux(keyName);
#else
  return false;
#endif
}

bool SecureKeyStore::clearAllKeys() {
  bool success = true;
  success &= deleteKey(GrokAPIKey);
  success &= deleteKey(OpenAIAPIKey);
  success &= deleteKey(AnthropicAPIKey);
  return success;
}

//==============================================================================
// Windows Implementation (DPAPI)
//==============================================================================

#if JUCE_WINDOWS

bool SecureKeyStore::storeKeyWindows(const juce::String &keyName,
                                     const juce::String &keyValue) {
  ZENITH_LOG_INFO("SecureKeyStore: storeKeyWindows started");
  // Convert to UTF-8
  auto utf8Data = keyValue.toUTF8();

  // Prepare data for encryption
  DATA_BLOB dataIn;
  dataIn.pbData = (BYTE *)utf8Data.getAddress();
  dataIn.cbData = (DWORD)utf8Data.sizeInBytes();

  DATA_BLOB dataOut;

  // Encrypt using DPAPI (user-specific encryption)
  BOOL result = CryptProtectData(&dataIn,
                                 L"Zenith DAW API Key",     // Description
                                 nullptr,                   // Optional entropy
                                 nullptr,                   // Reserved
                                 nullptr,                   // Prompt struct
                                 CRYPTPROTECT_UI_FORBIDDEN, // Flags
                                 &dataOut);

  if (!result) {
    ZENITH_LOG_ERROR("SecureKeyStore: Failed to encrypt key: " + keyName);
    return false;
  }

  // Convert encrypted data to base64 for storage
  juce::MemoryBlock encryptedBlock(dataOut.pbData, dataOut.cbData);
  auto base64 = encryptedBlock.toBase64Encoding();

  // Free the encrypted data
  LocalFree(dataOut.pbData);

  // Store in application data directory
  auto appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("ZenithDAW")
          .getChildFile("keys");

  appDataDir.createDirectory();

  auto keyFile = appDataDir.getChildFile(keyName + ".key");

  // Write encrypted data to file
  return keyFile.replaceWithText(base64);
}

bool SecureKeyStore::retrieveKeyWindows(const juce::String &keyName,
                                        juce::String &outKey) {
  ZENITH_LOG_INFO("SecureKeyStore: retrieveKeyWindows started");
  // Read encrypted data from file
  auto appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("ZenithDAW")
          .getChildFile("keys");

  ZENITH_LOG_INFO("SecureKeyStore: Checking keys dir: " + appDataDir.getFullPathName());
  auto keyFile = appDataDir.getChildFile(keyName + ".key");

  if (!keyFile.existsAsFile()) {
    ZENITH_LOG_WARNING("SecureKeyStore: Key file does not exist: " +
               keyFile.getFullPathName());
    return false;
  }

  ZENITH_LOG_INFO("SecureKeyStore: Loading key file...");
  auto base64 = keyFile.loadFileAsString();

  // Decode from base64
  juce::MemoryBlock encryptedBlock;
  if (!encryptedBlock.fromBase64Encoding(base64))
    return false;

  // Prepare data for decryption
  DATA_BLOB dataIn;
  dataIn.pbData = (BYTE *)encryptedBlock.getData();
  dataIn.cbData = (DWORD)encryptedBlock.getSize();

  DATA_BLOB dataOut;

  // Decrypt using DPAPI
  BOOL result = CryptUnprotectData(&dataIn,
                                   nullptr, // Description (output)
                                   nullptr, // Optional entropy
                                   nullptr, // Reserved
                                   nullptr, // Prompt struct
                                   CRYPTPROTECT_UI_FORBIDDEN, &dataOut);

  if (!result) {
    ZENITH_LOG_ERROR("SecureKeyStore: Failed to decrypt key: " + keyName);
    return false;
  }

  // Convert decrypted data to string
  outKey =
      juce::String::fromUTF8((const char *)dataOut.pbData, (int)dataOut.cbData);

  // Securely clear the decrypted data from memory
  SecureZeroMemory(dataOut.pbData, dataOut.cbData);

  // Free the decrypted data
  LocalFree(dataOut.pbData);

  return true;
}

bool SecureKeyStore::deleteKeyWindows(const juce::String &keyName) {
  auto appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("ZenithDAW")
          .getChildFile("keys");

  auto keyFile = appDataDir.getChildFile(keyName + ".key");

  if (keyFile.existsAsFile())
    return keyFile.deleteFile();

  return true; // Already doesn't exist
}

#endif // JUCE_WINDOWS

//==============================================================================
// macOS Implementation (Keychain)
//==============================================================================

#if JUCE_MAC

bool SecureKeyStore::storeKeyMac(const juce::String &keyName,
                                 const juce::String &keyValue) {
  // Delete existing key if present
  deleteKeyMac(keyName);

  auto serviceName = getServiceName();
  auto utf8Value = keyValue.toUTF8();

  OSStatus status = SecKeychainAddGenericPassword(
      nullptr,                         // Default keychain
      (UInt32)serviceName.length(),    // Service name length
      serviceName.toRawUTF8(),         // Service name
      (UInt32)keyName.length(),        // Account name length
      keyName.toRawUTF8(),             // Account name
      (UInt32)utf8Value.sizeInBytes(), // Password length
      utf8Value.getAddress(),          // Password data
      nullptr                          // Item ref (not needed)
  );

  if (status != errSecSuccess) {
    DBG("SecureKeyStore: Failed to store key in Keychain: " +
        juce::String(status));
    return false;
  }

  return true;
}

bool SecureKeyStore::retrieveKeyMac(const juce::String &keyName,
                                    juce::String &outKey) {
  auto serviceName = getServiceName();

  UInt32 passwordLength = 0;
  void *passwordData = nullptr;

  OSStatus status = SecKeychainFindGenericPassword(
      nullptr,                      // Default keychain
      (UInt32)serviceName.length(), // Service name length
      serviceName.toRawUTF8(),      // Service name
      (UInt32)keyName.length(),     // Account name length
      keyName.toRawUTF8(),          // Account name
      &passwordLength,              // Password length (output)
      &passwordData,                // Password data (output)
      nullptr                       // Item ref (not needed)
  );

  if (status != errSecSuccess)
    return false;

  // Convert to string
  outKey =
      juce::String::fromUTF8((const char *)passwordData, (int)passwordLength);

  // Free the password data
  SecKeychainItemFreeContent(nullptr, passwordData);

  return true;
}

bool SecureKeyStore::deleteKeyMac(const juce::String &keyName) {
  auto serviceName = getServiceName();

  SecKeychainItemRef itemRef = nullptr;

  OSStatus status = SecKeychainFindGenericPassword(
      nullptr, (UInt32)serviceName.length(), serviceName.toRawUTF8(),
      (UInt32)keyName.length(), keyName.toRawUTF8(), nullptr, nullptr,
      &itemRef);

  if (status == errSecSuccess && itemRef != nullptr) {
    status = SecKeychainItemDelete(itemRef);
    CFRelease(itemRef);
    return status == errSecSuccess;
  }

  return true; // Already doesn't exist
}

#endif // JUCE_MAC

//==============================================================================
// Linux Implementation (File-based with JUCE encryption)
//==============================================================================

#if JUCE_LINUX

bool SecureKeyStore::storeKeyLinux(const juce::String &keyName,
                                   const juce::String &keyValue) {
  // Use JUCE's Blowfish encryption with machine-specific key
  auto machineID =
      juce::SystemStats::getComputerName() + juce::SystemStats::getLogonName();
  juce::BlowFish blowfish(machineID.toUTF8(), machineID.length());

  // Encrypt the key value
  juce::MemoryBlock encrypted;
  auto utf8Data = keyValue.toUTF8();
  encrypted.setSize(utf8Data.sizeInBytes() + 8); // Add padding for Blowfish

  blowfish.encrypt((const uint32 *)utf8Data.getAddress(),
                   (uint32 *)encrypted.getData(), (int)utf8Data.sizeInBytes());

  // Convert to base64
  auto base64 = encrypted.toBase64Encoding();

  // Store in user config directory
  auto configDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile(".zenith-daw")
          .getChildFile("keys");

  configDir.createDirectory();

  auto keyFile = configDir.getChildFile(keyName + ".key");

  return keyFile.replaceWithText(base64);
}

bool SecureKeyStore::retrieveKeyLinux(const juce::String &keyName,
                                      juce::String &outKey) {
  auto configDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile(".zenith-daw")
          .getChildFile("keys");

  auto keyFile = configDir.getChildFile(keyName + ".key");

  if (!keyFile.existsAsFile())
    return false;

  auto base64 = keyFile.loadFileAsString();

  // Decode from base64
  juce::MemoryBlock encrypted;
  if (!encrypted.fromBase64Encoding(base64))
    return false;

  // Decrypt using machine-specific key
  auto machineID =
      juce::SystemStats::getComputerName() + juce::SystemStats::getLogonName();
  juce::BlowFish blowfish(machineID.toUTF8(), machineID.length());

  juce::MemoryBlock decrypted;
  decrypted.setSize(encrypted.getSize());

  blowfish.decrypt((const uint32 *)encrypted.getData(),
                   (uint32 *)decrypted.getData(), (int)encrypted.getSize());

  outKey = juce::String::fromUTF8((const char *)decrypted.getData(),
                                  (int)decrypted.getSize());

  return true;
}

bool SecureKeyStore::deleteKeyLinux(const juce::String &keyName) {
  auto configDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile(".zenith-daw")
          .getChildFile("keys");

  auto keyFile = configDir.getChildFile(keyName + ".key");

  if (keyFile.existsAsFile())
    return keyFile.deleteFile();

  return true;
}

#endif // JUCE_LINUX

} // namespace zenith
