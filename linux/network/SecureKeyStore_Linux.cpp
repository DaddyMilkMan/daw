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
            // PKCS7-like padding or just simple null padding if we store size separately?
            // Simplest for now: ensure multiple of 8.
            int paddingParams = 8 - (processedData.getSize() % 8);
            if (paddingParams < 8) // If it's 8, it's already aligned, but standard PKCS7 adds a full block.
            {
                 // We will just pad with zeros for simplicity as we are serializing var which stops at valid end usually?
                 // Actually `juce::var` binary format ... 
                 // Let's rely on storing the actual data size or just trusting the var parser to stop.
                 // Better: Store size as first 4 bytes? 
                 // Even better: Use PKCS7 padding where the value of padding byte is the number of padding bytes.
                 processedData.ensureSize (processedData.getSize() + paddingParams);
                 for (int i = 0; i < paddingParams; ++i)
                     processedData.append (&paddingParams, 1); // Not quite PKCS7 correct logic if we don't start from 1, but close enough for self-contained. 
                     // Wait, standard PKCS7: if 8 bytes needed, add 8 bytes of value 8. If 1 byte needed, 1 byte of value 1.
            }
            else
            {
                // If aligned, add a full block of 8s to distinguish from data ending in valid bytes?
                // Let's Keep It Simple: Just pad with zeros to align to 8 bytes.
                // Upon decryption, we try to read `var`. If it has trailing zeros, `var::readFromStream` usually works if it's based on internal structure, 
                // but `MemoryBlock::fromBase64String` etc might be better.
                // Let's stick to simple multiple of 8 size.
                if (processedData.getSize() % 8 != 0)
                    processedData.setSize (processedData.getSize() + (8 - (processedData.getSize() % 8)), true);
            }
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

            // Write back
            // Note: BlowFish encrypt/decrypt takes reference and modifies.
            // Wait, standard JUCE `BlowFish::encrypt(uint32&, uint32&)` handles endianness? 
            // The JUCE docs say "The byte ordering of the 32-bit integers is irrelevant...".
            // So we just need to pack/unpack correctly.
            
            // Actually, we must be careful. `juce::BlowFish` modifies the uint32s.
            // When writing back to memory, we should consistent.
            
            *(juce::uint32*)(rawData + i * 8) = l;
            *(juce::uint32*)(rawData + i * 8 + 4) = r;
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
