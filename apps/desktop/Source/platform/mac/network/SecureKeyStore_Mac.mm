/*
  ==============================================================================

    SecureKeyStore_Mac.mm
    Created: 2025-12-28
    Author:  Zenith DAW

  ==============================================================================
*/

#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include "../../../network/SecureKeyStore.h"

namespace zenith {

static NSString* getServiceName() {
    return @"com.zenithaudio.ZenithDAW";
}

bool SecureKeyStore::storeKey(const juce::String& keyName, const juce::String& keyValue) {
    @autoreleasepool {
        NSString* account = [NSString stringWithUTF8String:keyName.toRawUTF8()];
        NSData* password = [[NSString stringWithUTF8String:keyValue.toRawUTF8()] dataUsingEncoding:NSUTF8StringEncoding];
        
        NSDictionary* query = @{
            (id)kSecClass: (id)kSecClassGenericPassword,
            (id)kSecAttrService: getServiceName(),
            (id)kSecAttrAccount: account
        };
        
        // Delete existing if any
        SecItemDelete((CFDictionaryRef)query);
        
        NSMutableDictionary* attributes = [query mutableCopy];
        [attributes setObject:password forKey:(id)kSecValueData];
        [attributes setObject:(id)kSecAttrAccessibleAfterFirstUnlock forKey:(id)kSecAttrAccessible];
        
        OSStatus status = SecItemAdd((CFDictionaryRef)attributes, NULL);
        return status == errSecSuccess;
    }
}

bool SecureKeyStore::retrieveKey(const juce::String& keyName, juce::String& outKey) {
    @autoreleasepool {
        NSString* account = [NSString stringWithUTF8String:keyName.toRawUTF8()];
        
        NSDictionary* query = @{
            (id)kSecClass: (id)kSecClassGenericPassword,
            (id)kSecAttrService: getServiceName(),
            (id)kSecAttrAccount: account,
            (id)kSecReturnData: @YES,
            (id)kSecMatchLimit: (id)kSecMatchLimitOne
        };
        
        CFTypeRef result = NULL;
        OSStatus status = SecItemCopyMatching((CFDictionaryRef)query, &result);
        
        if (status == errSecSuccess && result != NULL) {
            NSData* data = (NSData*)CFBridgingRelease(result);
            NSString* pw = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            if (pw) {
                outKey = juce::String([pw UTF8String]);
                return true;
            }
        }
        
        return false;
    }
}

bool SecureKeyStore::hasKey(const juce::String& keyName) {
    @autoreleasepool {
        NSString* account = [NSString stringWithUTF8String:keyName.toRawUTF8()];
        
        NSDictionary* query = @{
            (id)kSecClass: (id)kSecClassGenericPassword,
            (id)kSecAttrService: getServiceName(),
            (id)kSecAttrAccount: account,
            (id)kSecMatchLimit: (id)kSecMatchLimitOne
        };
        
        OSStatus status = SecItemCopyMatching((CFDictionaryRef)query, NULL);
        return status == errSecSuccess;
    }
}

bool SecureKeyStore::deleteKey(const juce::String& keyName) {
    @autoreleasepool {
        NSString* account = [NSString stringWithUTF8String:keyName.toRawUTF8()];
        
        NSDictionary* query = @{
            (id)kSecClass: (id)kSecClassGenericPassword,
            (id)kSecAttrService: getServiceName(),
            (id)kSecAttrAccount: account
        };
        
        OSStatus status = SecItemDelete((CFDictionaryRef)query);
        return (status == errSecSuccess || status == errSecItemNotFound);
    }
}

bool SecureKeyStore::clearAllKeys() {
    @autoreleasepool {
        NSDictionary* query = @{
            (id)kSecClass: (id)kSecClassGenericPassword,
            (id)kSecAttrService: getServiceName()
        };
        
        OSStatus status = SecItemDelete((CFDictionaryRef)query);
        return (status == errSecSuccess || status == errSecItemNotFound);
    }
}

juce::String SecureKeyStore::getServiceName() {
    return "ZenithDAW";
}

} // namespace zenith
