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
    juce::MemoryBlock performCrypto(const void* data, size_t size, bool encrypt)
    {
        if (data == nullptr || size == 0)
            return {};

        auto keyStr = getMachineKey();
        // Hash the key to get a fixed length key for Blowfish (max 72 bytes usually, typically 56 or less is standard, but JUCE Blowfish handles up to 448 bits / 56 bytes)
        // We'll use SHA-256 and truncate/use the first 56 bytes (or less).
        // Actually SHA-256 is 32 bytes (256 bits). That fits perfectly in Blowfish key range (32-448 bits).
        
        // However, juce::BlowFish doesn't seem to have a SHA helper built-in easily accessible without `juce_cryptography` module which might not include SHA.
        // Wait, juce_cryptography has SHA256. Assuming it's available.
        // If not, we can just use the raw bytes of the string, maybe hashed with a simple hash if we want to be fancy, but raw bytes are fine if long enough.
        // The machine-id is usually 32 hex chars (32 bytes effectively if hex decoded, or 32 chars).
        
        juce::MemoryBlock keyData (keyStr.toRawUTF8(), keyStr.getNumBytesAsUTF8());
        
        // Pad data to 8 bytes for Blowfish
        juce::MemoryBlock processedData;
        processedData.append (data, size);

        if (encrypt)
        {
            // PKCS7 padding: Always add 1 to 8 bytes.
            // If data is already a multiple of 8, add a full block of 8s.
            int paddingBytes = 8 - (static_cast<int>(processedData.getSize()) % 8);
            juce::uint8 p = static_cast<juce::uint8>(paddingBytes);
            
            for (int i = 0; i < paddingBytes; ++i)
                processedData.append (&p, 1);
        }

        juce::BlowFish bf (keyData.getData(), (int)keyData.getSize());

        // Perform in-place encryption/decryption
        // Blowfish processes 2x 32bit ints (8 bytes) at a time.
        
        auto* rawData = static_cast<juce::uint8*> (processedData.getData());
        int numBlocks = (int)processedData.getSize() / 8;

        for (int i = 0; i < numBlocks; ++i)
        {
            juce::uint32 l = juce::ByteOrder::littleEndianInt (rawData + i * 8);
            juce::uint32 r = juce::ByteOrder::littleEndianInt (rawData + i * 8 + 4);

            if (encrypt)
                bf.encrypt (l, r);
            else
                bf.decrypt (l, r);

            // Write back using portable ByteOrder methods
            juce::ByteOrder::storeLittleEndianInt (rawData + i * 8, l);
            juce::ByteOrder::storeLittleEndianInt (rawData + i * 8 + 4, r);
        }

        if (!encrypt)
        {
            // PKCS7 unpadding
            if (processedData.getSize() >= 8)
            {
                auto* data = static_cast<const juce::uint8*> (processedData.getData());
                juce::uint8 paddingVal = data[processedData.getSize() - 1];
                
                if (paddingVal > 0 && paddingVal <= 8)
                {
                    // Basic validation of padding
                    bool valid = true;
                    for (size_t i = 0; i < paddingVal; ++i)
                    {
                        if (data[processedData.getSize() - 1 - i] != paddingVal)
                        {
                            valid = false;
                            break;
                        }
                    }
                    
                    if (valid)
                        processedData.setSize (processedData.getSize() - paddingVal);
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
