// shell/settings/Settings.h: what the application remembers between runs: the audio device, rate and buffer, the key
// bindings, the pedals' easing times, the units, the last build, the window's size. One JSON file,
// TheEngineEngine/settings.json under the user's configuration folder:
//
//   Windows   %APPDATA%
//   macOS     ~/Library/Application Support
//   Linux     $XDG_CONFIG_HOME, else ~/.config
//
// (TEE_SETTINGS_DIR in the environment names another folder: the tests use it, and so can a person who wants two
// setups.) The file has a schema version. A file that cannot be read as this version's (not JSON, a newer version, a
// value of the wrong kind) is renamed aside (settings.json.bad, then .bad2, ...) and the defaults are used: the
// application always starts, and nothing a person set is deleted. A field the file lacks takes its default; a field
// this version does not know is dropped at the next save.
//
// Not for the audio thread: load() and save() touch the disk.
#pragma once

#include "input/Driver.h"
#include "input/Keys.h"

#include <filesystem>
#include <string>
#include <vector>

namespace tee::settings
{
    inline constexpr int kVersion = 1;

    enum class Units { metric, imperial };      // km/h, kW, N m, bar; or mph, hp, lb ft, psi

    struct Settings
    {
        // the audio device: empty and 0 mean the system's default device, its own rate
        std::string audioDevice;
        int audioRate = 0;
        int audioBuffer = 256;                  // frames asked of the device
        // The simulator's quality: "full" (the engine at 96 kHz inside: the default) or "economy" (48 kHz inside: about
        // half the CPU, the top octave lost). Never changed by itself.
        std::string audioQuality = "full";

        std::vector<input::Binding> bindings = input::defaultBindings();
        input::DriverSettings driver;           // the easing times, the automatic clutch

        Units units = Units::metric;
        // The interface's own choice of a unit a quantity, as it writes it: lines of "quantity=unit" (those not at
        // their default). The shell keeps the text and hands it back; the interface package reads it.
        std::string unitChoices;
        std::string lastBuild;                  // the file of the build last open; empty: none
        std::vector<std::string> recentBuilds;  // the files opened or saved last, newest first (files::RecentFiles)
        bool tryFirstSeen = false;              // the first build's TRY FIRST card was closed (it is not shown again)
        int windowWidth = 0, windowHeight = 0;  // logical px; 0: the Panel's own size
        // (the theme and the zoom are FunkGui's own preferences, in its file beside this one)
    };

    // The folder the file lives in (it may not exist yet), and the file.
    std::filesystem::path directory();
    std::filesystem::path file();

    struct Loaded
    {
        Settings settings;                      // the defaults where the file said nothing
        bool found = false;                     // there was a file
        std::string note;                       // not empty: what was wrong with it, and where it was moved to
    };

    // Reads the file at `path` (file() when empty). Never fails: see the header comment.
    Loaded load(const std::filesystem::path& path = {});

    // Writes the file (its folder is made): beside its place first, then renamed over it, so a crash mid-write leaves
    // the old file whole. False, and why, when it cannot be written.
    bool save(const Settings&, std::string& why, const std::filesystem::path& path = {});

    // The settings as the file's text, and back. parse() answers false, and why, for text that is not this version's.
    std::string toJson(const Settings&);
    bool parse(const std::string& json, Settings& out, std::string& why);
}
