// tests/shell/SettingsTest.cpp: the settings file (shell/settings/Settings.h): what is written reads back, a file
// that lacks fields takes the defaults for them, and a file that is not ours is moved aside, never deleted.
#include "settings/Settings.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

using namespace tee;
namespace fs = std::filesystem;

namespace
{
    fs::path freshFolder(const char* name)
    {
        const fs::path dir = fs::path(TEE_SOURCE_DIR) / "out" / "shell" / name;
        fs::remove_all(dir);
        fs::create_directories(dir);
        return dir;
    }

    void write(const fs::path& path, const std::string& text)
    {
        std::ofstream out(path, std::ios::binary);
        out << text;
    }
}

TEST_CASE("shell: settings read back as they were written", "[shell][settings]")
{
    const fs::path path = freshFolder("settings-roundtrip") / "settings.json";
    settings::Settings s;
    s.audioDevice = "Speakers (a device)";
    s.audioRate = 44100;
    s.audioBuffer = 512;
    s.bindings = { { input::key::kA, input::Action::throttle }, { 200, input::Action::brake } };   // one with no name
    s.driver.throttle = { 0.5f, 0.25f };
    s.driver.autoClutch = false;
    s.driver.offHold = 2.0f;
    s.units = settings::Units::imperial;
    s.unitChoices = "pressure=psi\npower=hp\n";
    s.lastBuild = "builds/my v8.json";
    s.audioQuality = "economy";
    s.recentBuilds = { "builds/my v8.json", "C:\\Users\\me\\a diesel.json" };
    s.windowWidth = 1280;
    s.windowHeight = 800;
    std::string why;
    REQUIRE(settings::save(s, why, path));
    CHECK_FALSE(fs::exists(path.string() + ".new"));

    const settings::Loaded l = settings::load(path);
    CHECK(l.found);
    CHECK(l.note.empty());
    CHECK(l.settings.audioDevice == s.audioDevice);
    CHECK(l.settings.audioRate == 44100);
    CHECK(l.settings.audioBuffer == 512);
    REQUIRE(l.settings.bindings.size() == 2);
    CHECK(l.settings.bindings[0].key == input::key::kA);
    CHECK(l.settings.bindings[0].action == input::Action::throttle);
    CHECK(l.settings.bindings[1].key == 200);
    CHECK(l.settings.bindings[1].action == input::Action::brake);
    CHECK(l.settings.driver.throttle.rise == Catch::Approx(0.5f));
    CHECK(l.settings.driver.throttle.fall == Catch::Approx(0.25f));
    CHECK_FALSE(l.settings.driver.autoClutch);
    CHECK(l.settings.driver.offHold == Catch::Approx(2.0f));
    CHECK(l.settings.units == settings::Units::imperial);
    CHECK(l.settings.unitChoices == "pressure=psi\npower=hp\n");
    CHECK(l.settings.lastBuild == s.lastBuild);
    CHECK(l.settings.recentBuilds == s.recentBuilds);
    CHECK(l.settings.audioQuality == "economy");
    CHECK(l.settings.windowWidth == 1280);
    CHECK(l.settings.windowHeight == 800);
}

TEST_CASE("shell: no settings file, and one that says little, give the defaults", "[shell][settings]")
{
    const fs::path dir = freshFolder("settings-defaults");
    const settings::Loaded none = settings::load(dir / "settings.json");
    CHECK_FALSE(none.found);
    CHECK(none.note.empty());
    CHECK(none.settings.audioBuffer == 256);
    CHECK(none.settings.bindings.size() == input::defaultBindings().size());
    CHECK(none.settings.driver.autoClutch);

    write(dir / "settings.json", R"({"format": "tee-settings", "version": 1, "audio": {"buffer": 128}, "unknown": [1, 2]})");
    const settings::Loaded some = settings::load(dir / "settings.json");
    CHECK(some.found);
    CHECK(some.note.empty());
    CHECK(some.settings.audioBuffer == 128);
    CHECK(some.settings.audioRate == 0);
    CHECK(some.settings.bindings.size() == input::defaultBindings().size());
    CHECK(some.settings.units == settings::Units::metric);
}

TEST_CASE("shell: a settings file that is not ours is moved aside and the defaults used", "[shell][settings]")
{
    const fs::path dir = freshFolder("settings-bad");
    const fs::path path = dir / "settings.json";
    const char* bad[] = {
        "this is not JSON {",
        R"({"format": "something else", "version": 1})",
        R"({"format": "tee-settings", "version": 99})",
        R"({"format": "tee-settings", "version": 1, "audio": {"buffer": "large"}})",
        R"({"format": "tee-settings", "version": 1, "keys": [{"key": "w", "action": "fly"}]})",
        R"({"format": "tee-settings", "version": 1, "driver": {"throttle": {"rise": -1}}})",
    };
    int n = 0;
    for (const char* text : bad)
    {
        INFO(text);
        write(path, text);
        const settings::Loaded l = settings::load(path);
        ++n;
        CHECK(l.found);
        CHECK_FALSE(l.note.empty());
        CHECK(l.settings.audioBuffer == 256);               // the defaults
        CHECK_FALSE(fs::exists(path));                      // moved, not deleted
        const fs::path aside = n == 1 ? fs::path(path.string() + ".bad") : fs::path(path.string() + ".bad" + std::to_string(n));
        CHECK(fs::exists(aside));
    }
    // and the next save starts a clean file
    std::string why;
    CHECK(settings::save({}, why, path));
    CHECK(settings::load(path).note.empty());

    std::string reason;
    settings::Settings parsed;
    CHECK_FALSE(settings::parse("[]", parsed, reason));
    CHECK_FALSE(reason.empty());
    CHECK(settings::parse(settings::toJson({}), parsed, reason));
}
