/*
  ==============================================================================

    SecureKeyStore_Mac.cpp
    Created: 2025-12-22

    macOS implementation for SecureKeyStore using Security.framework Keychain.

  ==============================================================================
*/

#include "network/SecureKeyStore.h"

#if JUCE_MAC
#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>

namespace zenith {

namespace {
    // Service name for Keychain queries
    static const char* kServiceName = "com.zenithdaw.app";
    
    // Helper to create CFString from juce::String
    CFStringRef createCFString(const juce::String& str) {
        return CFStringCreateWithCString(kCFAllocatorDefault, 
                                          str.toRawUTF8(), 
                                          kCFStringEncodingUTF8);
    }
}

bool SecureKeyStore::storeKey(const juce::String &keyName,
                              const juce::String &keyValue) {
    // First delete any existing key with this name
    deleteKey(keyName);
    
    CFStringRef service = CFStringCreateWithCString(kCFAllocatorDefault, 
                                                     kServiceName, 
                                                     kCFStringEncodingUTF8);
    CFStringRef account = createCFString(keyName);
    CFDataRef passwordData = CFDataCreate(kCFAllocatorDefault, 
                                           (const UInt8*)keyValue.toRawUTF8(), 
                                           (CFIndex)keyValue.length());
    
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 4,
        &kCFTypeDictionaryKeyCallBacks, 
        &kCFTypeDictionaryValueCallBacks);
    
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    CFDictionarySetValue(query, kSecValueData, passwordData);
    
    OSStatus status = SecItemAdd(query, nullptr);
    
    CFRelease(query);
    CFRelease(passwordData);
    CFRelease(account);
    CFRelease(service);
    
    if (status != errSecSuccess) {
        DBG("[SecureKeyStore_Mac] Failed to store key: " + keyName + 
            " (OSStatus: " + juce::String((int)status) + ")");
    }
    
    return status == errSecSuccess;
}

bool SecureKeyStore::retrieveKey(const juce::String &keyName,
                                 juce::String &outKey) {
    CFStringRef service = CFStringCreateWithCString(kCFAllocatorDefault, 
                                                     kServiceName, 
                                                     kCFStringEncodingUTF8);
    CFStringRef account = createCFString(keyName);
    
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 5,
        &kCFTypeDictionaryKeyCallBacks, 
        &kCFTypeDictionaryValueCallBacks);
    
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);
    
    CFDataRef resultData = nullptr;
    OSStatus status = SecItemCopyMatching(query, (CFTypeRef*)&resultData);
    
    CFRelease(query);
    CFRelease(account);
    CFRelease(service);
    
    if (status == errSecSuccess && resultData != nullptr) {
        const UInt8* bytes = CFDataGetBytePtr(resultData);
        CFIndex length = CFDataGetLength(resultData);
        outKey = juce::String::fromUTF8((const char*)bytes, (int)length);
        CFRelease(resultData);
        return true;
    }
    
    if (status != errSecItemNotFound) {
        DBG("[SecureKeyStore_Mac] Failed to retrieve key: " + keyName + 
            " (OSStatus: " + juce::String((int)status) + ")");
    }
    
    return false;
}

bool SecureKeyStore::deleteKey(const juce::String &keyName) {
    CFStringRef service = CFStringCreateWithCString(kCFAllocatorDefault, 
                                                     kServiceName, 
                                                     kCFStringEncodingUTF8);
    CFStringRef account = createCFString(keyName);
    
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 3,
        &kCFTypeDictionaryKeyCallBacks, 
        &kCFTypeDictionaryValueCallBacks);
    
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    
    OSStatus status = SecItemDelete(query);
    
    CFRelease(query);
    CFRelease(account);
    CFRelease(service);
    
    // errSecItemNotFound is OK - means key didn't exist
    return status == errSecSuccess || status == errSecItemNotFound;
}

} // namespace zenith
#endif

