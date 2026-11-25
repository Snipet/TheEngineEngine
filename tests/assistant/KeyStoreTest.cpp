// tests/assistant/KeyStoreTest.cpp: the key's store (assistant/.../KeyStore.h) through its fake: a round trip, a failed
// save that says so and keeps the key held before (TestingPlayground's chat-audit STORE-1), the key's check (nothing that
// could break its header) and its mask (chat-audit UI-4: at most its last four characters). The system's stores
// (Keychain, Secret Service) are compiled here and exercised only by the owner: no test touches a real store or a real
// key (the strings below are placeholders).
#include "tee/assistant/KeyStore.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace tee::assistant;

TEST_CASE("assistant: the key's store: a round trip, a failure that says so, a removal", "[assistant]")
{
    MemoryKeyStore store;
    std::string key, why;
    REQUIRE(store.load(key, why));
    CHECK(key.empty());
    REQUIRE(store.save("placeholder-key-one-0123456789", why));
    REQUIRE(store.load(key, why));
    CHECK(key == "placeholder-key-one-0123456789");
    store.failSave = true;
    CHECK_FALSE(store.save("placeholder-key-two-0123456789", why));
    CHECK_FALSE(why.empty());
    store.failSave = false;
    REQUIRE(store.load(key, why));
    CHECK(key == "placeholder-key-one-0123456789");   // the key held before stays
    store.failLoad = true;
    CHECK_FALSE(store.load(key, why));
    store.failLoad = false;
    REQUIRE(store.remove(why));
    REQUIRE(store.load(key, why));
    CHECK(key.empty());
}

TEST_CASE("assistant: a key as typed: trimmed, and refused when it could break its header", "[assistant]")
{
    std::string why;
    std::string k = "  placeholder-key-0123456789\r\n";
    REQUIRE(checkKey(k, why));
    CHECK(k == "placeholder-key-0123456789");
    for (std::string bad : {std::string(), std::string("   \n"), std::string("place holder"), std::string("abc\r\nX-Evil: 1"),
                            std::string("abc\tdef"), std::string(600, 'a'), std::string("abc") + char(0) + "def"})
    {
        INFO(bad.size());
        CHECK_FALSE(checkKey(bad, why));
    }
}

TEST_CASE("assistant: a key is shown masked: at most its last four characters", "[assistant]")
{
    CHECK(maskKey("") == "none saved");
    CHECK(maskKey("short-placeholder") == "saved");
    const std::string k = "placeholder-key-ABCDEFGHIJKLMNOPwxyz";
    const std::string m = maskKey(k);
    CHECK(m == "saved, ending ...wxyz");
    // no five characters of the key in a row appear in its mask
    for (std::size_t i = 0; i + 5 <= k.size(); ++i)
        CHECK(m.find(k.substr(i, 5)) == std::string::npos);
}

TEST_CASE("assistant: the system's store exists in this build, or says in words why not", "[assistant]")
{
    std::string why;
    const auto store = makeSystemKeyStore(why);
    if (store == nullptr)
        CHECK_FALSE(why.empty());   // (never exercised here: a real store is the owner's to try)
}
