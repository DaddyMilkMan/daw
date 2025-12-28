/*
  ==============================================================================

    SecureKeyStore_Windows.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

  ==============================================================================
*/

#include "../../../network/SecureKeyStore.h"
#include <juce_core/juce_core.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

namespace zenith {

namespace {
    juce::File getKeystoreFile() {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW")
            .getChildFile("keystore_win.dat");
    }

    juce::MemoryBlock encryptData(const juce::MemoryBlock& input) {
        DATA_BLOB dataIn;
        dataIn.pbData = (BYTE*)input.getData();
        dataIn.cbData = (DWORD)input.getSize();

        DATA_BLOB dataOut;
        if (CryptProtectData(&dataIn, L"ZenithDAW Key", NULL, NULL, NULL, 0, &dataOut)) {
            juce::MemoryBlock output(dataOut.pbData, dataOut.cbData);
            LocalFree(dataOut.pbData);
            return output;
        }
        return {};
    }

    juce::MemoryBlock decryptData(const juce::MemoryBlock& input) {
        DATA_BLOB dataIn;
        dataIn.pbData = (BYTE*)input.getData();
        dataIn.cbData = (DWORD)input.getSize();

        DATA_BLOB dataOut;
        if (CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) {
            juce::MemoryBlock output(dataOut.pbData, dataOut.cbData);
            LocalFree(dataOut.pbData);
            return output;
        }
        return {};
    }

    std::unique_ptr<juce::DynamicObject> loadProps() {
        auto file = getKeystoreFile();
        if (!file.existsAsFile()) return std::make_unique<juce::DynamicObject>();

        juce::MemoryBlock encrypted;
        file.loadFileAsData(encrypted);
        if (encrypted.getSize() == 0) return std::make_unique<juce::DynamicObject>();

        auto decrypted = decryptData(encrypted);
        if (decrypted.getSize() == 0) return std::make_unique<juce::DynamicObject>();

        juce::MemoryInputStream mis(decrypted, false);
        auto v = juce::var::readFromStream(mis);
        if (auto* obj = v.getDynamicObject()) {
            return std::unique_ptr<juce::DynamicObject>(obj->clone().release());
        }
        return std::make_unique<juce::DynamicObject>();
    }

    void saveProps(const juce::DynamicObject* obj) {
        if (!obj) return;
        juce::MemoryOutputStream mos;
        juce::var(obj->clone().release()).writeToStream(mos);
        
        auto encrypted = encryptData(mos.getMemoryBlock());
        if (encrypted.getSize() > 0) {
            auto file = getKeystoreFile();
            if (!file.getParentDirectory().exists()) file.getParentDirectory().createDirectory();
            file.replaceWithData(encrypted.getData(), encrypted.getSize());
        }
    }
}

bool SecureKeyStore::storeKey(const juce::String& keyName, const juce::String& keyValue) {
    auto props = loadProps();
    props->setProperty(keyName, keyValue);
    saveProps(props.get());
    return true;
}

bool SecureKeyStore::retrieveKey(const juce::String& keyName, juce::String& outKey) {
    auto props = loadProps();
    if (props->hasProperty(keyName)) {
        outKey = props->getProperty(keyName).toString();
        return true;
    }
    return false;
}

bool SecureKeyStore::hasKey(const juce::String& keyName) {
    auto props = loadProps();
    return props->hasProperty(keyName);
}

bool SecureKeyStore::deleteKey(const juce::String& keyName) {
    auto props = loadProps();
    if (props->hasProperty(keyName)) {
        props->removeProperty(keyName);
        saveProps(props.get());
        return true;
    }
    return false;
}

bool SecureKeyStore::clearAllKeys() {
    return getKeystoreFile().deleteFile();
}

juce::String SecureKeyStore::getServiceName() {
    return "ZenithDAW";
}

} // namespace zenith

#endif
