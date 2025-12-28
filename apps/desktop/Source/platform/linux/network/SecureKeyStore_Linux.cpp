/*
  ==============================================================================

    SecureKeyStore_Linux.cpp
    Created: 2025-11-29

    Linux implementation using libsecret (GNOME Keyring/KWallet) with fallback.

  ==============================================================================
*/

#include "SecureKeyStore.h"
#include <juce_cryptography/juce_cryptography.h>
#include <sys/stat.h>

#ifdef HAVE_LIBSECRET
#include <libsecret/secret.h>
#endif

namespace zenith {

// Internal helpers
namespace {

#ifdef HAVE_LIBSECRET
    const SecretSchema* getZenithSchema() noexcept
    {
        static const SecretSchema schema = {
            "org.zenith.daw.keystore", SECRET_SCHEMA_NONE,
            {
                {  "service", SECRET_SCHEMA_ATTRIBUTE_STRING },
                {  "key_name", SECRET_SCHEMA_ATTRIBUTE_STRING },
                {  nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING }
            }
        };
        return &schema;
    }

    bool libsecretAvailable() {
        // Runtime check could be added here, but for now assume build-time config is enough
        return true; 
    }
#endif

    juce::File getKeystoreFile()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("ZenithDAW").getChildFile("keystore.dat");
    }

    // Machine-specific key for encryption
    juce::String getMachineKey()
    {
        return juce::SystemStats::getComputerName() + "_" + 
               juce::SystemStats::getUserId() + "_" +
               "ZenithLinuxKey";
    }

    juce::MemoryBlock performCrypto(const void* data, size_t size, bool encrypt)
    {
        if (data == nullptr || size == 0) return {};

        auto keyStr = getMachineKey();
        juce::SHA256 sha(keyStr.toRawUTF8(), keyStr.getNumBytesAsUTF8());
        auto hash = sha.getRawData();
        juce::MemoryBlock keyData = hash;

        // Prepare buffer
        juce::MemoryBlock processedData;
        processedData.append(data, size);

        if (encrypt) {
            // PKCS7 padding
            int paddingNeeded = 8 - (processedData.getSize() % 8);
            juce::uint8 padByte = (juce::uint8)paddingNeeded;
            for (int i = 0; i < paddingNeeded; ++i)
                processedData.append(&padByte, 1);
        }

        juce::BlowFish bf(keyData.getData(), (int)keyData.getSize());
        auto* rawData = static_cast<juce::uint8*>(processedData.getData());
        int numBlocks = (int)processedData.getSize() / 8;

        auto writeLE = [](void* d, juce::uint32 v) {
            juce::uint8* p = static_cast<juce::uint8*>(d);
            p[0] = static_cast<juce::uint8>(v & 0xFF);
            p[1] = static_cast<juce::uint8>((v >> 8) & 0xFF);
            p[2] = static_cast<juce::uint8>((v >> 16) & 0xFF);
            p[3] = static_cast<juce::uint8>((v >> 24) & 0xFF);
        };

        for (int i = 0; i < numBlocks; ++i) {
            juce::uint32 l = juce::ByteOrder::littleEndianInt(rawData + i * 8);
            juce::uint32 r = juce::ByteOrder::littleEndianInt(rawData + i * 8 + 4);

            if (encrypt) bf.encrypt(l, r);
            else bf.decrypt(l, r);

            writeLE(rawData + i * 8, l);
            writeLE(rawData + i * 8 + 4, r);
        }

        if (!encrypt) {
            // Unpad
            if (processedData.getSize() < 8) return {};
            juce::uint8 paddingVal = rawData[processedData.getSize() - 1];
            if (paddingVal < 1 || paddingVal > 8 || paddingVal > processedData.getSize()) return {};
            processedData.setSize(processedData.getSize() - paddingVal);
        }

        return processedData;
    }

    std::unique_ptr<juce::DynamicObject> loadKeystorePropSet()
    {
        auto file = getKeystoreFile();
        if (!file.existsAsFile())
            return std::make_unique<juce::DynamicObject>();

        juce::MemoryBlock encryptedData;
        file.loadFileAsData(encryptedData);

        if (encryptedData.getSize() == 0)
             return std::make_unique<juce::DynamicObject>();

        auto decryptedData = performCrypto(encryptedData.getData(), encryptedData.getSize(), false);
        if (decryptedData.getSize() == 0)
             return std::make_unique<juce::DynamicObject>();
        
        juce::MemoryInputStream input(decryptedData, false);
        auto v = juce::var::readFromStream(input);

        if (auto* obj = v.getDynamicObject()) {
            auto cloned = std::make_unique<juce::DynamicObject>();
            for (const auto& prop : obj->getProperties())
                cloned->setProperty(prop.name, prop.value);
            return cloned;
        }
                
        return std::make_unique<juce::DynamicObject>();
    }

    void saveKeystorePropSet(juce::DynamicObject* props)
    {
        if (props == nullptr) return;

        juce::MemoryOutputStream mos;
        juce::var propsVar(props->clone().release());
        propsVar.writeToStream(mos);

        auto encrypted = performCrypto(mos.getData(), mos.getDataSize(), true);

        auto file = getKeystoreFile();
        if (!file.getParentDirectory().exists())
            file.getParentDirectory().createDirectory();

        if (file.replaceWithData(encrypted.getData(), encrypted.getSize())) {
            // Set permissions to 0600
            chmod(file.getFullPathName().toRawUTF8(), S_IRUSR | S_IWUSR);
        }
    }

} // namespace

juce::String SecureKeyStore::getServiceName()
{
    return "ZenithDAW";
}

bool SecureKeyStore::storeKey(const juce::String& keyName, const juce::String& keyValue)
{
#ifdef HAVE_LIBSECRET
    if (libsecretAvailable())
    {
        GError* error = nullptr;
        // Check for existing key first? Not strictly necessary with secret_password_store_sync
        gboolean result = secret_password_store_sync(
            getZenithSchema(),
            SECRET_COLLECTION_DEFAULT,
            ("Zenith DAW: " + keyName).toRawUTF8(),
            keyValue.toRawUTF8(),
            nullptr,
            &error,
            "service", "ZenithDAW",
            "key_name", keyName.toRawUTF8(),
            nullptr);
        
        if (error) {
            juce::Logger::writeToLog("libsecret store error: " + juce::String(error->message));
            g_error_free(error);
            // Fall through to fallback
        } else if (result) {
            return true;
        }
    }
#endif

    // Fallback
    auto props = loadKeystorePropSet();
    props->setProperty(keyName, keyValue);
    saveKeystorePropSet(props.get());
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
            nullptr,
            &error,
            "service", "ZenithDAW",
            "key_name", keyName.toRawUTF8(),
            nullptr);
        
        if (error) {
            juce::Logger::writeToLog("libsecret lookup error: " + juce::String(error->message));
            g_error_free(error);
        } else if (password) {
            outKey = juce::String::fromUTF8(password);
            secret_password_free(password);
            return true;
        }
    }
#endif
    
    // Fallback
    auto props = loadKeystorePropSet();
    if (props->hasProperty(keyName)) {
        outKey = props->getProperty(keyName).toString();
        return true;
    }
    return false;
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
            return true;
        }
    }
#endif
    
    // Fallback
    auto props = loadKeystorePropSet();
    if (props->hasProperty(keyName)) {
        props->removeProperty(keyName);
        saveKeystorePropSet(props.get());
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
    return props->hasProperty(keyName);
}

bool SecureKeyStore::clearAllKeys()
{
    // Clearing libsecret items programmatically without knowing them all is tricky
    // usually requires iterating attributes. For now, we rely on user action or single deletes.
    // We CAN clear the file fallback though.
    auto file = getKeystoreFile();
    return file.deleteFile();
}

} // namespace zenith