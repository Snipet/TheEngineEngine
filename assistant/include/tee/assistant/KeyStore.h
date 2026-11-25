// assistant/include/tee/assistant/KeyStore.h: where the owner's API key is kept (docs/plan/w3-a-assistant.md, "The key
// path"; the owner's answer 3): the system's own store, the Keychain on macOS and the Secret Service through libsecret on
// Linux, and nowhere else. Where neither is reachable the key is refused, never written to a file instead.
//
// The key is typed into the app's dialog (U1's), checked (checkKey), saved here, and read back when a request is made
// (Chat.h's key function). It is never logged, never put on a command line, in a build file, a session, a fixture or a
// commit; the dialog shows it masked (maskKey): at most its last four characters.
//
// A save that fails says so: a store that could not write never reports success (TestingPlayground's chat-audit
// STORE-1), and the key held before stays as it was.
//
// Threads: any one at a time (the store's own calls may block briefly: a Keychain or D-Bus round trip).
#pragma once

#include <memory>
#include <string>

namespace tee::assistant
{
    class KeyStore
    {
    public:
        virtual ~KeyStore() = default;
        // True, with the key, when one is saved; true with an empty key when none is; false, with `why`, when the store
        // could not be read.
        virtual bool load(std::string& key, std::string& why) = 0;
        virtual bool save(const std::string& key, std::string& why) = 0;
        virtual bool remove(std::string& why) = 0;
    };

    // The system's store: null, with `why` in words, where there is none in this build (Windows in this version; a
    // Linux build without libsecret) or it cannot be reached (no Secret Service running).
    std::unique_ptr<KeyStore> makeSystemKeyStore(std::string& why);

    // A key as typed, made ready to keep: surrounding spaces and line ends trimmed. False, with `why`, when it cannot
    // be one (empty, too long, a space or a control character inside: such a key could break its HTTP header).
    bool checkKey(std::string& key, std::string& why);

    // What the dialog shows of a saved key: "saved, ending ...abcd" (at most its last four characters, and none of a
    // key shorter than 20); "none saved" for an empty one.
    std::string maskKey(const std::string& key);

    // A store in memory, for tests and for nothing else: it can be told to fail its next save or read.
    class MemoryKeyStore final : public KeyStore
    {
    public:
        bool failSave = false;
        bool failLoad = false;
        bool load(std::string& key, std::string& why) override;
        bool save(const std::string& key, std::string& why) override;
        bool remove(std::string& why) override;

    private:
        std::string key_;
    };
}
