// assistant/src/KeyStore.cpp: the owner's API key in the system's store (KeyStore.h): the Keychain (macOS, Security
// framework), the Secret Service (Linux, libsecret: TEE_HAS_LIBSECRET), else none.
#include "tee/assistant/KeyStore.h"

#if defined(__APPLE__)
 #include <CoreFoundation/CoreFoundation.h>
 #include <Security/Security.h>
#elif defined(TEE_HAS_LIBSECRET)
 #include <libsecret/secret.h>
#endif

namespace tee::assistant
{
#if defined(__APPLE__) || defined(TEE_HAS_LIBSECRET)
    namespace
    {
        constexpr const char* kService = "TheEngineEngine";
        constexpr const char* kAccount = "anthropic-api-key";
    }
#endif

    bool checkKey(std::string& key, std::string& why)
    {
        const auto first = key.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            why = "the key is empty";
            return false;
        }
        key = key.substr(first, key.find_last_not_of(" \t\r\n") - first + 1);
        if (key.size() > 512)
        {
            why = "that is too long to be a key";
            return false;
        }
        for (const char c : key)
            if (static_cast<unsigned char>(c) <= 32 || c == 127)
            {
                why = "a key has no spaces or control characters";
                return false;
            }
        return true;
    }

    std::string maskKey(const std::string& key)
    {
        if (key.empty())
            return "none saved";
        if (key.size() < 20)
            return "saved";
        return "saved, ending ..." + key.substr(key.size() - 4);
    }

    bool MemoryKeyStore::load(std::string& key, std::string& why)
    {
        if (failLoad)
        {
            why = "the store could not be read";
            return false;
        }
        key = key_;
        return true;
    }

    bool MemoryKeyStore::save(const std::string& key, std::string& why)
    {
        if (failSave)
        {
            why = "the store could not be written";
            return false;   // and the key held before stays
        }
        key_ = key;
        return true;
    }

    bool MemoryKeyStore::remove(std::string&)
    {
        key_.clear();
        return true;
    }

#if defined(__APPLE__)

    namespace
    {
        CFStringRef cf(const char* s)
        {
            return CFStringCreateWithCString(kCFAllocatorDefault, s, kCFStringEncodingUTF8);
        }

        // The item's query: a generic password of this service and account.
        CFMutableDictionaryRef query()
        {
            CFMutableDictionaryRef q = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
                                                                 &kCFTypeDictionaryValueCallBacks);
            CFStringRef service = cf(kService), account = cf(kAccount);
            CFDictionarySetValue(q, kSecClass, kSecClassGenericPassword);
            CFDictionarySetValue(q, kSecAttrService, service);
            CFDictionarySetValue(q, kSecAttrAccount, account);
            CFRelease(service);
            CFRelease(account);
            return q;
        }

        class Keychain final : public KeyStore
        {
        public:
            bool load(std::string& key, std::string& why) override
            {
                key.clear();
                CFMutableDictionaryRef q = query();
                CFDictionarySetValue(q, kSecReturnData, kCFBooleanTrue);
                CFDictionarySetValue(q, kSecMatchLimit, kSecMatchLimitOne);
                CFTypeRef found = nullptr;
                const OSStatus st = SecItemCopyMatching(q, &found);
                CFRelease(q);
                if (st == errSecItemNotFound)
                    return true;
                if (st != errSecSuccess || found == nullptr)
                {
                    why = "the Keychain could not be read";
                    return false;
                }
                const CFDataRef data = static_cast<CFDataRef>(found);
                key.assign(reinterpret_cast<const char*>(CFDataGetBytePtr(data)), static_cast<std::size_t>(CFDataGetLength(data)));
                CFRelease(found);
                return true;
            }

            bool save(const std::string& key, std::string& why) override
            {
                CFDataRef data = CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(key.data()),
                                              static_cast<CFIndex>(key.size()));
                CFMutableDictionaryRef q = query();
                CFMutableDictionaryRef change = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
                                                                          &kCFTypeDictionaryValueCallBacks);
                CFDictionarySetValue(change, kSecValueData, data);
                OSStatus st = SecItemUpdate(q, change);
                if (st == errSecItemNotFound)
                {
                    CFDictionarySetValue(q, kSecValueData, data);
                    CFDictionarySetValue(q, kSecAttrAccessible, kSecAttrAccessibleWhenUnlocked);
                    st = SecItemAdd(q, nullptr);
                }
                CFRelease(change);
                CFRelease(q);
                CFRelease(data);
                if (st != errSecSuccess)
                {
                    why = "the Keychain did not take the key";
                    return false;
                }
                return true;
            }

            bool remove(std::string& why) override
            {
                CFMutableDictionaryRef q = query();
                const OSStatus st = SecItemDelete(q);
                CFRelease(q);
                if (st != errSecSuccess && st != errSecItemNotFound)
                {
                    why = "the Keychain did not remove the key";
                    return false;
                }
                return true;
            }
        };
    }

    std::unique_ptr<KeyStore> makeSystemKeyStore(std::string&)
    {
        return std::make_unique<Keychain>();
    }

#elif defined(TEE_HAS_LIBSECRET)

    namespace
    {
        const SecretSchema* schema()
        {
            static const SecretSchema s = {
                "com.snipet.theengineengine.ApiKey",
                SECRET_SCHEMA_NONE,
                {{"service", SECRET_SCHEMA_ATTRIBUTE_STRING}, {"account", SECRET_SCHEMA_ATTRIBUTE_STRING}, {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING}},
                0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
            return &s;
        }

        class SecretService final : public KeyStore
        {
        public:
            bool load(std::string& key, std::string& why) override
            {
                key.clear();
                GError* error = nullptr;
                gchar* found = secret_password_lookup_sync(schema(), nullptr, &error, "service", kService, "account", kAccount,
                                                           nullptr);
                if (error != nullptr)
                {
                    g_error_free(error);
                    why = "the system's key store could not be read";
                    return false;
                }
                if (found != nullptr)
                {
                    key = found;
                    secret_password_free(found);
                }
                return true;
            }

            bool save(const std::string& key, std::string& why) override
            {
                GError* error = nullptr;
                const gboolean ok = secret_password_store_sync(schema(), SECRET_COLLECTION_DEFAULT, "TheEngineEngine: Claude API key",
                                                               key.c_str(), nullptr, &error, "service", kService, "account",
                                                               kAccount, nullptr);
                if (error != nullptr)
                    g_error_free(error);
                if (!ok)
                {
                    why = "the system's key store did not take the key";
                    return false;
                }
                return true;
            }

            bool remove(std::string& why) override
            {
                GError* error = nullptr;
                secret_password_clear_sync(schema(), nullptr, &error, "service", kService, "account", kAccount, nullptr);
                if (error != nullptr)
                {
                    g_error_free(error);
                    why = "the system's key store did not remove the key";
                    return false;
                }
                return true;
            }
        };
    }

    std::unique_ptr<KeyStore> makeSystemKeyStore(std::string&)
    {
        return std::make_unique<SecretService>();
    }

#else

    std::unique_ptr<KeyStore> makeSystemKeyStore(std::string& why)
    {
 #if defined(_WIN32)
        why = "the chat by API key is for macOS and Linux in this version";
 #else
        why = "this build has no system key store (libsecret's headers were missing when it was configured); the key is "
              "not kept in a file instead";
 #endif
        return nullptr;
    }

#endif
}
