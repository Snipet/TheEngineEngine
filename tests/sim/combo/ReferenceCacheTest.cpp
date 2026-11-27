// tests/sim/combo/ReferenceCacheTest.cpp: the reference sets kept across one job's processes (Combo.cpp, the lead's
// ruling of 27 Nov 03:44) are never read where any part of their key differs, and a partial or foreign file is a miss.
// The other half of the proof, that a kept set reads the nightly's figures to the digit, is the nightly itself run with
// TEE_COMBO_REFERENCES=off and with the cache (the commit says what was compared).
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace combo;

TEST_CASE("the kept reference sets: a key per build, scenario and copies; a changed tree, file or value is a miss", "[fast][fast-1][combo]")
{
    // a key for each part that decides a set: none shared (on a platform without an executable to key on there are
    // no keys at all, and every set is played)
    const std::string a = referenceKey("v8", Kind::Tour, true);
    if (a.empty())
        SKIP("no kept sets on this platform (no executable to key on) or TEE_COMBO_REFERENCES=off");
    const std::set<std::string> keys{a,
                                     referenceKey("v8", Kind::Tour, false),
                                     referenceKey("v8", Kind::Pull, true),
                                     referenceKey("small-four", Kind::Tour, true),
                                     referenceKey("v8 ", Kind::Tour, true)};
    CHECK(keys.size() == 5);
    CHECK(referenceKey("v8", Kind::Tour, true) == a);   // the same parts, the same key

    // a tree's hash moves with any byte, any name and any added file
    const auto dir = std::filesystem::temp_directory_path() / ("tee-refcache-test-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directories(dir / "sub");
    std::ofstream(dir / "a.fm") << "float x = 1.0f;\n";
    std::ofstream(dir / "sub" / "b.fm") << "int y = 2;\n";
    const auto h0 = referenceTreeHash(dir);
    CHECK(h0 != 0);
    CHECK(referenceTreeHash(dir) == h0);
    std::ofstream(dir / "sub" / "b.fm") << "int y = 3;\n";
    const auto h1 = referenceTreeHash(dir);
    CHECK(h1 != h0);
    std::filesystem::rename(dir / "sub" / "b.fm", dir / "sub" / "c.fm");
    CHECK(referenceTreeHash(dir) != h1);
    std::ofstream(dir / "d.fm") << "";
    CHECK(referenceTreeHash(dir) != h1);
    std::filesystem::remove_all(dir);
    CHECK(referenceTreeHash(dir) == 0);   // no tree: no key, so nothing kept

    // a set written is read back exactly; a partial, foreign or unknown file is a miss
    const std::string key = "test-" + std::to_string(std::random_device{}());
    const std::vector<double> excess{2.19, 2.2632323232, 0.1 + 0.2, 2.84, 1.97};
    keepReference(key, 11.6, excess);
    double step = 0.0;
    std::vector<double> back;
    REQUIRE(readKeptReference(key, step, back));
    CHECK(step == 11.6);
    CHECK(back == excess);   // to the bit
    CHECK_FALSE(readKeptReference(key + "-other", step, back));
    const auto file = referenceDirectory() / (key + ".ref");
    REQUIRE(std::filesystem::exists(file));
    std::string whole;
    {
        std::ifstream in(file);
        std::getline(in, whole);
    }
    // cut short (a writer stopped mid-line: the rename makes this impossible, the reader holds to it anyway)
    std::ofstream(file) << whole.substr(0, whole.size() / 2);
    CHECK_FALSE(readKeptReference(key, step, back));
    // another format's tag
    std::ofstream(file) << "tee-combo-reference-0 1 0x1p+0 0x1p+0 end\n";
    CHECK_FALSE(readKeptReference(key, step, back));
    // no end mark
    std::ofstream(file) << "tee-combo-reference-1 1 0x1p+0 0x1p+0\n";
    CHECK_FALSE(readKeptReference(key, step, back));
    std::filesystem::remove(file);
    CHECK_FALSE(readKeptReference(key, step, back));
}
