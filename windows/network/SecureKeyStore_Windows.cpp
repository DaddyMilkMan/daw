/*
  ==============================================================================

    SecureKeyStore_Windows.cpp
    Created: 2025-12-22
    
    Windows implementations for SecureKeyStore.

  ==============================================================================
*/

#include "network/SecureKeyStore.h"
#include "engine/ZenithLogger.h"

#if JUCE_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

namespace zenith {

bool SecureKeyStore::storeKey(const juce::String &keyName,
                              const juce::String &keyValue) {
  if (keyName.isEmpty() || keyValue.isEmpty())
    return false;

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

bool SecureKeyStore::retrieveKey(const juce::String &keyName,
                                 juce::String &outKey) {
  if (keyName.isEmpty())
    return false;

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

bool SecureKeyStore::deleteKey(const juce::String &keyName) {
  if (keyName.isEmpty())
    return false;

  auto appDataDir =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("ZenithDAW")
          .getChildFile("keys");

  auto keyFile = appDataDir.getChildFile(keyName + ".key");

  if (keyFile.existsAsFile())
    return keyFile.deleteFile();

  return true; // Already doesn't exist
}

} // namespace zenith
#endif
