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
#include <cstring>
#include <limits>

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
        juce::MemoryBlock keyData (keyStr.toRawUTF8(), keyStr.getNumBytesAsUTF8());
        
        // Validate key size fits in int (JUCE BlowFish API requirement)
        if (keyData.getSize() > static_cast<size_t>(std::numeric_limits<int>::max()))
        {
            DBG("SecureKeyStore: Key size too large");
            return {};
        }
        
        // Prepare data with proper PKCS7 padding
        juce::MemoryBlock processedData;
        processedData.append (data, size);

        if (encrypt)
        {
            // Implement proper PKCS7 padding
            // PKCS7 always adds padding, even if data is already aligned
            // If data is N bytes over a multiple of 8, add (8-N) bytes of value (8-N)
            // If data is exactly a multiple of 8, add 8 bytes of value 8
            size_t remainder = processedData.getSize() % 8;
            uint8_t paddingLength = static_cast<uint8_t>(8 - remainder);
            
            // Add PKCS7 padding bytes (each byte has value equal to padding length)
            for (uint8_t i = 0; i < paddingLength; ++i)
                processedData.append (&paddingLength, 1);
        }

        // Validate size is multiple of 8
        if (processedData.getSize() % 8 != 0)
        {
            jassertfalse; // Should never happen with proper padding
            return {};
        }

        juce::BlowFish bf (keyData.getData(), (int)keyData.getSize());

        // Perform in-place encryption/decryption
        auto* rawData = static_cast<juce::uint8*> (processedData.getData());
        size_t numBlocks = processedData.getSize() / 8;

        for (size_t i = 0; i < numBlocks; ++i)
        {
            // Read as little-endian integers
            juce::uint32 l = juce::ByteOrder::littleEndianInt (rawData + i * 8);
            juce::uint32 r = juce::ByteOrder::littleEndianInt (rawData + i * 8 + 4);

            if (encrypt)
                bf.encrypt (l, r);
            else
                bf.decrypt (l, r);

            // Write back as little-endian integers
            // Convert native endianness to little-endian and write byte-by-byte to avoid alignment issues
            juce::uint32 lLE = juce::ByteOrder::swapIfBigEndian (l);
            juce::uint32 rLE = juce::ByteOrder::swapIfBigEndian (r);
            
            std::memcpy (rawData + i * 8, &lLE, sizeof(juce::uint32));
            std::memcpy (rawData + i * 8 + 4, &rLE, sizeof(juce::uint32));
        }

        // Remove PKCS7 padding after decryption
        if (!encrypt && processedData.getSize() > 0)
        {
            // Extra safety check to prevent buffer underflow
            if (processedData.getSize() < 8)
            {
                DBG("SecureKeyStore: Decrypted data too small for valid PKCS7 padding");
                return {};
            }
            
            uint8_t paddingLength = static_cast<uint8_t*>(processedData.getData())[processedData.getSize() - 1];
            
            // Validate padding (PKCS7 validation)
            if (paddingLength > 0 && paddingLength <= 8)
            {
                // Ensure we have enough data for the claimed padding length (cast to size_t for safe comparison)
                if (static_cast<size_t>(paddingLength) > processedData.getSize())
                {
                    DBG("SecureKeyStore: Invalid padding length exceeds data size");
                    return {};
                }
                
                bool validPadding = true;
                size_t startIdx = processedData.getSize() - static_cast<size_t>(paddingLength);
                for (size_t i = startIdx; i < processedData.getSize(); ++i)
                {
                    if (static_cast<uint8_t*>(processedData.getData())[i] != paddingLength)
                    {
                        validPadding = false;
                        break;
                    }
                }
                
                if (validPadding)
                    processedData.setSize (processedData.getSize() - static_cast<size_t>(paddingLength));
                else
                    return {}; // Invalid padding - decryption failed
            }
            else
            {
                return {}; // Invalid padding length
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
        if (!file.loadFileAsData (encryptedData))
        {
            DBG("SecureKeyStore: Failed to load keystore file");
            return std::make_unique<juce::DynamicObject>();
        }

        if (encryptedData.getSize() == 0)
             return std::make_unique<juce::DynamicObject>();

        auto decryptedData = performCrypto (encryptedData.getData(), encryptedData.getSize(), false);
        
        // Check if decryption failed
        if (decryptedData.getSize() == 0)
        {
            DBG("SecureKeyStore: Decryption failed or invalid padding");
            return std::make_unique<juce::DynamicObject>();
        }
        
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
        DBG("SecureKeyStore: Failed to parse decrypted data as DynamicObject");
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
        
        // Check if encryption failed
        if (encrypted.getSize() == 0)
        {
            DBG("SecureKeyStore: Encryption failed");
            return;
        }

        auto file = getKeystoreFile();
        if (!file.getParentDirectory().exists())
        {
            if (!file.getParentDirectory().createDirectory())
            {
                DBG("SecureKeyStore: Failed to create keystore directory");
                return;
            }
        }

        // Write
        if (file.replaceWithData (encrypted.getData(), encrypted.getSize()))
        {
            // Set permissions to 0600
            chmod (file.getFullPathName().toRawUTF8(), S_IRUSR | S_IWUSR);
        }
        else
        {
            DBG("SecureKeyStore: Failed to write keystore file");
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
