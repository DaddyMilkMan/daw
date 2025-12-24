/*
  ==============================================================================

    SecureKeyStore_Mac.cpp
    Created: 2025-12-22

    macOS implementations for SecureKeyStore.
    (Empty implementation as requested)

  ==============================================================================
*/

#include "network/SecureKeyStore.h"

#if JUCE_MAC
namespace zenith {

bool SecureKeyStore::storeKey(const juce::String &keyName,
                              const juce::String &keyValue) {
  // Not implemented
  return false;
}

bool SecureKeyStore::retrieveKey(const juce::String &keyName,
                                 juce::String &outKey) {
  // Not implemented
  return false;
}

bool SecureKeyStore::deleteKey(const juce::String &keyName) {
  // Not implemented
  return false;
}

} // namespace zenith
#endif
