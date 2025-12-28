/*
  ==============================================================================

    SecureKeyStore_Linux.cpp
    Created: 2025-12-23
    
    Linux implementation with REAL libsecret Secret Service API integration
    Falls back to encrypted file storage when libsecret is unavailable

  ==============================================================================
*/

#include "../../../network/SecureKeyStore.h"
#include <juce_cryptography/juce_cryptography.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <vector>

#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif

namespace zenith {

//==============================================================================
// libsecret Schema Definition
//==============================================================================

#ifdef HAVE_LIBSECRET
namespace {
    // Define the secret schema for Zenith DAW credentials
    const SecretSchema* getZenithSchema() {
        static const SecretSchema schema = {
            "com.zenith.daw.credentials",
            SECRET_SCHEMA_NONE,
            {
                { "service", SECRET_SCHEMA_ATTRIBUTE_STRING },
                { "key_name", SECRET_SCHEMA_ATTRIBUTE_STRING },
                { nullptr, SecretSchemaAttributeType(0) }
            }
        };
        return &schema;
    }
    
    bool libsecretAvailable() {
        static int available = -1;
        if (available == -1) {
            // Try to connect to Secret Service
            GError* error = nullptr;
            SecretService* service = secret_service_get_sync(
                SECRET_SERVICE_LOAD_COLLECTIONS, nullptr, &error);
            if (error) {
                g_error_free(error);
                available = 0;
            } else {
                available = 1;
                if (service) g_object_unref(service);
            }
        }
        return available == 1;
    }
}
#endif

//==============================================================================
// Encrypted File Fallback Helpers
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

    // Encrypt/Decrypt helper using Blowfish
    juce::MemoryBlock performCrypto(const void* data, size_t size, bool encrypt)
    {
        if (data == nullptr || size == 0)
            return {};

        auto keyStr = getMachineKey();
        
        // Use SHA-256 hash of machine key to get stable 32-byte key
        juce::SHA256 sha(keyStr.toRawUTF8(), keyStr.getNumBytesAsUTF8());
        auto hash = sha.getRawData();
        juce::MemoryBlock keyData = hash;
        
        // Prepare buffer
        juce::MemoryBlock processedData;
        processedData.append (data, size);

        if (encrypt)
        {
            // PKCS7 padding (block size is 8 bytes for Blowfish)
            int paddingNeeded = 8 - (processedData.getSize() % 8);
            juce::uint8 padByte = (juce::uint8)paddingNeeded;
            
            for (int i = 0; i < paddingNeeded; ++i)
                processedData.append (&padByte, 1);
        }

        juce::BlowFish bf (keyData.getData(), (int)keyData.getSize());

        auto* rawData = static_cast<juce::uint8*> (processedData.getData());
        int numBlocks = (int)processedData.getSize() / 8;
        
        auto writeLE = [](void* d, juce::uint32 v) {
            juce::uint8* p = static_cast<juce::uint8*>(d);
            p[0] = static_cast<juce::uint8>(v & 0xFF);
            p[1] = static_cast<juce::uint8>((v >> 8) & 0xFF);
            p[2] = static_cast<juce::uint8>((v >> 16) & 0xFF);
            p[3] = static_cast<juce::uint8>((v >> 24) & 0xFF);
        };

        for (int i = 0; i < numBlocks; ++i)
        {
            juce::uint32 l = juce::ByteOrder::littleEndianInt (rawData + i * 8);
            juce::uint32 r = juce::ByteOrder::littleEndianInt (rawData + i * 8 + 4);

            if (encrypt)
                bf.encrypt (l, r);
            else
                bf.decrypt (l, r);

            writeLE (rawData + i * 8, l);
            writeLE (rawData + i * 8 + 4, r);
        }
        
        if (!encrypt)
        {
            // Remove PKCS7 padding
            if (processedData.getSize() > 0)
            {
                juce::uint8 padLen = rawData[processedData.getSize() - 1];
                if (padLen > 0 && padLen <= 8 && padLen <= processedData.getSize())
                {
                    bool paddingValid = true;
                    for (int i = 0; i < padLen; ++i) {
                         if (rawData[processedData.getSize() - 1 - i] != padLen) {
                             paddingValid = false;
                             break;
                         }
                    }
                    
                    if (paddingValid)
                        processedData.setSize(processedData.getSize() - padLen);
                }
            }
        }

        return processedData;
    }

    // Load entire keystore from encrypted file
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
        {
            // Set permissions to 0600 (owner read/write only)
            chmod (file.getFullPathName().toRawUTF8(), S_IRUSR | S_IWUSR);
        }
    }

} // namespace

//==============================================================================
// Public API Implementation
//==============================================================================

bool SecureKeyStore::storeKey(const juce::String& keyName, const juce::String& keyValue)
{
#ifdef HAVE_LIBSECRET
    if (libsecretAvailable())
    {
        GError* error = nullptr;
        gboolean result = secret_password_store_sync(
            getZenithSchema(),
            SECRET_COLLECTION_DEFAULT,
            ("ZenithDAW: " + keyName).toRawUTF8(),  // Label shown in keyring
            keyValue.toRawUTF8(),
            nullptr,  // Cancellable
            &error,
            "service", "ZenithDAW",
            "key_name", keyName.toRawUTF8(),
            nullptr);
        
        if (error) {
            juce::Logger::writeToLog("libsecret store error: " + juce::String(error->message));
            g_error_free(error);
            // Fall through to file-based storage
        } else if (result) {
            juce::Logger::writeToLog("Key stored in Secret Service: " + keyName);
            return true;
        }
    }
#endif
    
    // Fallback: Encrypted file storage
    auto props = loadKeystorePropSet();
    props->setProperty (keyName, keyValue);
    saveKeystorePropSet (props.get());
    return true;
}

bool SecureKeyStore::retrieveKey(const juce::String& keyName, juce::String& outKey)
{
#ifdef HAVE_LIBSECRET
    if (libsecretAvailable())
    {
        GError* error = nullptr;
        gchar* password = secret_password_lookup_sync(
            getZenithSchema(),
            nullptr,  // Cancellable
            &error,
            "service", "ZenithDAW",
            "key_name", keyName.toRawUTF8(),
            nullptr);
        
        if (error) {
            juce::Logger::writeToLog("libsecret lookup error: " + juce::String(error->message));
            g_error_free(error);
            // Fall through to file-based storage
        } else if (password) {
            outKey = juce::String::fromUTF8(password);
            secret_password_free(password);
            juce::Logger::writeToLog("Key retrieved from Secret Service: " + keyName);
            return true;
        }
    }
#endif
    
    // Fallback: Encrypted file storage
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
#ifdef HAVE_LIBSECRET
    if (libsecretAvailable())
    {
        GError* error = nullptr;
        gchar* password = secret_password_lookup_sync(
            getZenithSchema(),
            nullptr,
            &error,
            "service", "ZenithDAW",
            "key_name", keyName.toRawUTF8(),
            nullptr);
        
        if (error) {
            g_error_free(error);
        } else if (password) {
            secret_password_free(password);
            return true;
        }
    }
#endif
    
    auto props = loadKeystorePropSet();
    return props->hasProperty (keyName);
}

bool SecureKeyStore::deleteKey(const juce::String& keyName)
{
#ifdef HAVE_LIBSECRET
    if (libsecretAvailable())
    {
        GError* error = nullptr;
        gboolean result = secret_password_clear_sync(
            getZenithSchema(),
            nullptr,
            &error,
            "service", "ZenithDAW",
            "key_name", keyName.toRawUTF8(),
            nullptr);
        
        if (error) {
            juce::Logger::writeToLog("libsecret delete error: " + juce::String(error->message));
            g_error_free(error);
        } else if (result) {
            juce::Logger::writeToLog("Key deleted from Secret Service: " + keyName);
            return true;
        }
    }
#endif
    
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
    // For libsecret, we'd need to enumerate - just clear the file for now
    // The Secret Service keys can be cleared via Seahorse/GNOME Keyring GUI
    auto file = getKeystoreFile();
    return file.deleteFile();
}

juce::String SecureKeyStore::getServiceName()
{
    return "ZenithDAW";
}

} // namespace zenith
