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

#if HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif

namespace zenith {

//==============================================================================
// Internal Helpers
//==============================================================================

namespace {

#if HAVE_LIBSECRET
    const SecretSchema* getZenithSchema()
    {
        static const SecretSchema schema = {
            "com.zenithaudio.ZenithDAW.ApiKey", SECRET_SCHEMA_NONE,
            {
                { "key_name", SECRET_SCHEMA_ATTRIBUTE_STRING },
                { NULL, SECRET_SCHEMA_ATTRIBUTE_STRING }
            }
        };
        return &schema;
    }
#endif

    // Helper to get the machine-specific key for encryption (fallback)
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

    // Get the storage file location (fallback)
    juce::File getKeystoreFile()
    {
        auto appDataDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
        return appDataDir.getChildFile ("ZenithDAW").getChildFile ("keystore.dat");
    }

    // Encrypt/Decrypt helper (fallback)
    // Returns empty block on failure
    juce::MemoryBlock performCrypto(const void* data, size_t size, bool encrypt)
    {
        if (data == nullptr || size == 0)
            return {};

        auto keyStr = getMachineKey();
        juce::MemoryBlock keyData (keyStr.toRawUTF8(), keyStr.getNumBytesAsUTF8());
        
        juce::MemoryBlock processedData;
        processedData.append (data, size);

        if (encrypt)
        {
            // PKCS7 Padding
            int paddingBytes = 8 - (processedData.getSize() % 8);
            for (int i = 0; i < paddingBytes; ++i)
            {
                juce::uint8 p = (juce::uint8)paddingBytes;
                processedData.append (&p, 1);
            }
        }

        juce::BlowFish bf (keyData.getData(), (int)keyData.getSize());
        auto* rawData = static_cast<juce::uint8*> (processedData.getData());
        int numBlocks = (int)processedData.getSize() / 8;

        for (int i = 0; i < numBlocks; ++i)
        {
            // Endianness Fix: Always process as Little Endian for portable storage
            juce::uint32 l = juce::ByteOrder::littleEndianInt (rawData + i * 8);
            juce::uint32 r = juce::ByteOrder::littleEndianInt (rawData + i * 8 + 4);

            if (encrypt)
                bf.encrypt (l, r);
            else
                bf.decrypt (l, r);

            // Write back in Little Endian
            rawData[i * 8 + 0] = (juce::uint8)(l);
            rawData[i * 8 + 1] = (juce::uint8)(l >> 8);
            rawData[i * 8 + 2] = (juce::uint8)(l >> 16);
            rawData[i * 8 + 3] = (juce::uint8)(l >> 24);

            rawData[i * 8 + 4] = (juce::uint8)(r);
            rawData[i * 8 + 5] = (juce::uint8)(r >> 8);
            rawData[i * 8 + 6] = (juce::uint8)(r >> 16);
            rawData[i * 8 + 7] = (juce::uint8)(r >> 24);
        }

        if (!encrypt)
        {
            // Unpad PKCS7
            if (processedData.getSize() < 8) return {};
            juce::uint8 paddingVal = rawData[processedData.getSize() - 1];
            if (paddingVal < 1 || paddingVal > 8) return {}; // Invalid padding
            
            // Verify padding
            for (int i = 0; i < (int)paddingVal; ++i)
            {
                if (processedData.getSize() < (size_t)(i + 1)) return {};
                if (rawData[processedData.getSize() - 1 - i] != paddingVal)
                    return {}; // Invalid padding
            }
            
            processedData.setSize (processedData.getSize() - paddingVal);
        }

        return processedData;
    }

    // Fallback implementation using file + Blowfish
    namespace fallback {
        std::unique_ptr<juce::DynamicObject> loadKeystorePropSet()
        {
            auto file = getKeystoreFile();
            if (!file.existsAsFile())
                return std::make_unique<juce::DynamicObject>();

            juce::MemoryBlock encryptedData;
            file.loadFileAsData (encryptedData);

            if (encryptedData.getSize() == 0 || encryptedData.getSize() % 8 != 0)
                 return std::make_unique<juce::DynamicObject>();

            auto decryptedData = performCrypto (encryptedData.getData(), encryptedData.getSize(), false);
            if (decryptedData.getSize() == 0)
                return std::make_unique<juce::DynamicObject>();
            
            juce::MemoryInputStream input (decryptedData, false);
            auto v = juce::var::readFromStream (input);

            if (auto* obj = v.getDynamicObject())
            {
                auto cloned = std::make_unique<juce::DynamicObject>();
                for (const auto& prop : obj->getProperties())
                    cloned->setProperty(prop.name, prop.value);
                return cloned;
            }
                
            return std::make_unique<juce::DynamicObject>();
        }

        void saveKeystorePropSet(const juce::DynamicObject* props)
        {
            if (props == nullptr) return;

            juce::MemoryOutputStream mos;
            juce::var propsVar(props->clone().release());
            propsVar.writeToStream (mos);

            auto encrypted = performCrypto (mos.getData(), mos.getDataSize(), true);

            auto file = getKeystoreFile();
            if (!file.getParentDirectory().exists())
                file.getParentDirectory().createDirectory();

            if (file.replaceWithData (encrypted.getData(), encrypted.getSize()))
                chmod (file.getFullPathName().toRawUTF8(), S_IRUSR | S_IWUSR);
        }
    }

} // namespace

//==============================================================================
// Public API
//==============================================================================

bool SecureKeyStore::storeKey(const juce::String& keyName, const juce::String& keyValue)
{
#if HAVE_LIBSECRET
    GError* error = NULL;
    secret_password_store_sync (getZenithSchema(), SECRET_COLLECTION_DEFAULT,
                                keyName.toRawUTF8(), keyValue.toRawUTF8(), NULL, &error,
                                "key_name", keyName.toRawUTF8(),
                                NULL);

    if (error != NULL) {
        g_error_free (error);
        return false;
    }
    return true;
#else
    auto props = fallback::loadKeystorePropSet();
    props->setProperty (keyName, keyValue);
    fallback::saveKeystorePropSet (props.get());
    return true;
#endif
}

bool SecureKeyStore::retrieveKey(const juce::String& keyName, juce::String& outKey)
{
#if HAVE_LIBSECRET
    GError* error = NULL;
    gchar* password = secret_password_lookup_sync (getZenithSchema(), NULL, &error,
                                                   "key_name", keyName.toRawUTF8(),
                                                   NULL);
    if (error != NULL) {
        g_error_free (error);
        return false;
    }

    if (password != NULL) {
        outKey = juce::String::fromUTF8 (password);
        secret_password_free (password);
        return true;
    }
    return false;
#else
    auto props = fallback::loadKeystorePropSet();
    if (props->hasProperty (keyName))
    {
        outKey = props->getProperty (keyName).toString();
        return true;
    }
    return false;
#endif
}

bool SecureKeyStore::hasKey(const juce::String& keyName)
{
    juce::String dummy;
    return retrieveKey (keyName, dummy);
}

bool SecureKeyStore::deleteKey(const juce::String& keyName)
{
#if HAVE_LIBSECRET
    GError* error = NULL;
    secret_password_clear_sync (getZenithSchema(), NULL, &error,
                                "key_name", keyName.toRawUTF8(),
                                NULL);
    if (error != NULL) {
        g_error_free (error);
        return false;
    }
    return true;
#else
    auto props = fallback::loadKeystorePropSet();
    if (props->hasProperty (keyName))
    {
        props->removeProperty (keyName);
        fallback::saveKeystorePropSet (props.get());
        return true;
    }
    return false;
#endif
}

bool SecureKeyStore::clearAllKeys()
{
#if HAVE_LIBSECRET
    // libsecret doesn't have a direct "clear all" for a schema easily without iterating
    // But we can clear our known keys
    deleteKey (GrokAPIKey);
    deleteKey (OpenAIAPIKey);
    deleteKey (AnthropicAPIKey);
#endif
    auto file = getKeystoreFile();
    file.deleteFile();
    return true;
}

juce::String SecureKeyStore::getServiceName()
{
    return "ZenithDAW";
}

} // namespace zenith
