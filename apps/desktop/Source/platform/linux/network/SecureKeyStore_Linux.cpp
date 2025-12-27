/*
  ==============================================================================

    SecureKeyStore_Linux.cpp
    Created: 2025-12-23

  ==============================================================================
*/

#include "../../../network/SecureKeyStore.h"
#include <juce_cryptography/juce_cryptography.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <vector>

namespace zenith {

//==============================================================================
// Internal Helpers
//==============================================================================

namespace {

    // Helper to get the machine-specific key for encryption
    juce::String getMachineKey()
    {
        // Try to read /etc/machine-id
        juce::File machineIdFile ("/etc/machine-id");
        if (machineIdFile.existsAsFile())
        {
            auto id = machineIdFile.loadFileAsString().trim();
            if (id.isNotEmpty())
                return id;
        }

        // Fallback: Computer Name + Logon Name
        return juce::SystemStats::getComputerName() + "_" + juce::SystemStats::getLogonName();
    }

    // Get the storage file location
    juce::File getKeystoreFile()
    {
        auto appDataDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
        return appDataDir.getChildFile ("ZenithDAW").getChildFile ("keystore.dat");
    }

    // Encrypt/Decrypt helper
    // Returns empty block on failure
    // Encrypt/Decrypt helper
    // Returns empty block on failure
    juce::MemoryBlock performCrypto(const void* data, size_t size, bool encrypt)
    {
        if (data == nullptr || size == 0)
            return {};

        auto keyStr = getMachineKey();
        
        // Use SHA-256 hash of machine key to get stable 32-byte key
        juce::SHA256 sha;
        auto hash = sha.calc(keyStr.toRawUTF8(), keyStr.getNumBytesAsUTF8());
        juce::MemoryBlock keyData(hash.getData(), 32); 
        
        // Prepare buffer
        juce::MemoryBlock processedData;
        processedData.append (data, size);

        if (encrypt)
        {
            // Always apply PKCS7 padding
            // Block size is 8 bytes for Blowfish
            int paddingNeeded = 8 - (processedData.getSize() % 8);
            juce::uint8 padByte = (juce::uint8)paddingNeeded;
            
            for (int i = 0; i < paddingNeeded; ++i)
                processedData.append (&padByte, 1);
        }

        juce::BlowFish bf (keyData.getData(), (int)keyData.getSize());

        // Perform in-place encryption/decryption
        // Blowfish processes 2x 32bit ints (8 bytes) at a time.
        
        auto* rawData = static_cast<juce::uint8*> (processedData.getData());
        int numBlocks = (int)processedData.getSize() / 8;

        for (int i = 0; i < numBlocks; ++i)
        {
            // Read as Little Endian explicitly for portability
            juce::uint32 l = juce::ByteOrder::littleEndianInt (rawData + i * 8);
            juce::uint32 r = juce::ByteOrder::littleEndianInt (rawData + i * 8 + 4);

            if (encrypt)
                bf.encrypt (l, r);
            else
                bf.decrypt (l, r);

            // Write back as Little Endian
            juce::ByteOrder::littleEndianInt (rawData + i * 8, l);
            juce::ByteOrder::littleEndianInt (rawData + i * 8 + 4, r);
        }
        
        if (!encrypt)
        {
            // Remove PKCS7 padding
            if (processedData.getSize() > 0)
            {
                juce::uint8 padLen = rawData[processedData.getSize() - 1];
                if (padLen > 0 && padLen <= 8 && padLen <= processedData.getSize())
                {
                    // Verify padding bytes (optional but recommended)
                    bool paddingValid = true;
                    for (int i = 0; i < padLen; ++i) {
                         if (rawData[processedData.getSize() - 1 - i] != padLen) {
                             paddingValid = false;
                             break;
                         }
                    }
                    
                    if (paddingValid)
                        processedData.setSize(processedData.getSize() - padLen);
                    // Else: invalid padding, but we might just return data described by var? 
                    // Strict PKCS7 would fail here. We'll just keep data if invalid? 
                    // No, invalid padding usually means wrong key or corruption.
                }
            }
        }

        return processedData;
    }

    // Load entire keystore
    std::unique_ptr<juce::DynamicObject> loadKeystorePropSet()
    {
        auto file = getKeystoreFile();
        if (!file.existsAsFile())
            return std::make_unique<juce::DynamicObject>();

        juce::MemoryBlock encryptedData;
        file.loadFileAsData (encryptedData);

        if (encryptedData.getSize() == 0)
             return std::make_unique<juce::DynamicObject>();

        auto decryptedData = performCrypto (encryptedData.getData(), encryptedData.getSize(), false);
        
        // Try to read as var
        juce::MemoryInputStream input (decryptedData, false);
        auto v = juce::var::readFromStream (input);

        if (auto* obj = v.getDynamicObject())
        {
            auto cloned = std::make_unique<juce::DynamicObject>();
            for (const auto& prop : obj->getProperties())
                cloned->setProperty(prop.name, prop.value);
            return cloned;
        }
            
        // If failed or not an object, return empty
        return std::make_unique<juce::DynamicObject>();
    }

    void saveKeystorePropSet(const juce::DynamicObject* props)
    {
        if (props == nullptr) return;

        // Serialize to binary
        juce::MemoryOutputStream mos;
        juce::var propsVar(props->clone().release());
        propsVar.writeToStream (mos);

        auto encrypted = performCrypto (mos.getData(), mos.getDataSize(), true);

        auto file = getKeystoreFile();
        if (!file.getParentDirectory().exists())
            file.getParentDirectory().createDirectory();

        // Write
        if (file.replaceWithData (encrypted.getData(), encrypted.getSize()))
        {
            // Set permissions to 0600
            chmod (file.getFullPathName().toRawUTF8(), S_IRUSR | S_IWUSR);
        }
    }

} // namespace

//==============================================================================
// Public API
//==============================================================================

bool SecureKeyStore::storeKey(const juce::String& keyName, const juce::String& keyValue)
{
    auto props = loadKeystorePropSet();
    props->setProperty (keyName, keyValue);
    saveKeystorePropSet (props.get());
    return true;
}

bool SecureKeyStore::retrieveKey(const juce::String& keyName, juce::String& outKey)
{
    auto props = loadKeystorePropSet();
    if (props->hasProperty (keyName))
    {
        outKey = props->getProperty (keyName).toString();
        return true;
    }
    return false;
}

bool SecureKeyStore::hasKey(const juce::String& keyName)
{
    auto props = loadKeystorePropSet();
    return props->hasProperty (keyName);
}

bool SecureKeyStore::deleteKey(const juce::String& keyName)
{
    auto props = loadKeystorePropSet();
    if (props->hasProperty (keyName))
    {
        props->removeProperty (keyName);
        saveKeystorePropSet (props.get());
        return true;
    }
    return false;
}

bool SecureKeyStore::clearAllKeys()
{
    // Simply delete the file
    auto file = getKeystoreFile();
    return file.deleteFile();
}

juce::String SecureKeyStore::getServiceName()
{
    return "ZenithDAW";
}

} // namespace zenith
