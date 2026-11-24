// shell/settings/Settings.cpp: Settings.h.
#include "settings/Settings.h"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace tee::settings
{
    namespace fs = std::filesystem;
    using nlohmann::json;

    namespace
    {
        fs::path fromEnvironment(const char* name)
        {
            const char* value = std::getenv(name);
            return value != nullptr && *value != '\0' ? fs::path(value) : fs::path();
        }

        json easing(const input::Easing& e)
        {
            return json{ { "rise", e.rise }, { "fall", e.fall } };
        }

        // A number of the file into `out`, when the file has that key: false for a value that is no number, or
        // outside least..most.
        template <typename T>
        bool number(const json& object, const char* key, T& out, double least, double most)
        {
            const auto it = object.find(key);
            if (it == object.end())
                return true;
            if (!it->is_number())
                return false;
            const double v = it->get<double>();
            if (!(v >= least && v <= most))
                return false;
            out = static_cast<T>(v);
            return true;
        }

        bool readEasing(const json& object, const char* key, input::Easing& out)
        {
            const auto it = object.find(key);
            if (it == object.end())
                return true;
            return it->is_object() && number(*it, "rise", out.rise, 0.0, 10.0) && number(*it, "fall", out.fall, 0.0, 10.0);
        }

        bool text(const json& object, const char* key, std::string& out)
        {
            const auto it = object.find(key);
            if (it == object.end())
                return true;
            if (!it->is_string())
                return false;
            out = it->get<std::string>();
            return true;
        }

        bool flag(const json& object, const char* key, bool& out)
        {
            const auto it = object.find(key);
            if (it == object.end())
                return true;
            if (!it->is_boolean())
                return false;
            out = it->get<bool>();
            return true;
        }
    }

    fs::path directory()
    {
        const fs::path given = fromEnvironment("TEE_SETTINGS_DIR");
        if (!given.empty())
            return given;
       #if defined(_WIN32)
        const fs::path base = fromEnvironment("APPDATA");
       #elif defined(__APPLE__)
        const fs::path home = fromEnvironment("HOME");
        const fs::path base = home.empty() ? fs::path() : home / "Library" / "Application Support";
       #else
        fs::path base = fromEnvironment("XDG_CONFIG_HOME");
        if (base.empty())
        {
            const fs::path home = fromEnvironment("HOME");
            base = home.empty() ? fs::path() : home / ".config";
        }
       #endif
        std::error_code ec;
        return (base.empty() ? fs::temp_directory_path(ec) : base) / "TheEngineEngine";
    }

    fs::path file()
    {
        return directory() / "settings.json";
    }

    std::string toJson(const Settings& s)
    {
        json bindings = json::array();
        for (const input::Binding& b : s.bindings)
        {
            // by name where the key has one (a file a person can read), by number otherwise
            const std::string_view name = input::keyName(b.key);
            json row{ { "action", std::string(input::actionName(b.action)) } };
            if (name.empty())
                row["key"] = b.key;
            else
                row["key"] = std::string(name);
            bindings.push_back(std::move(row));
        }
        const json root{
            { "format", "tee-settings" },
            { "version", kVersion },
            { "audio", { { "device", s.audioDevice }, { "rate", s.audioRate }, { "buffer", s.audioBuffer } } },
            { "keys", std::move(bindings) },
            { "driver",
              { { "throttle", easing(s.driver.throttle) },
                { "brake", easing(s.driver.brake) },
                { "clutch", easing(s.driver.clutch) },
                { "handbrake", easing(s.driver.handbrake) },
                { "autoClutch", s.driver.autoClutch },
                { "offHold", s.driver.offHold } } },
            { "units", s.units == Units::imperial ? "imperial" : "metric" },
            { "unitChoices", s.unitChoices },
            { "lastBuild", s.lastBuild },
            { "recentBuilds", s.recentBuilds },
            { "window", { { "width", s.windowWidth }, { "height", s.windowHeight } } },
        };
        return root.dump(2) + "\n";
    }

    bool parse(const std::string& text_, Settings& out, std::string& why)
    {
        const json root = json::parse(text_, nullptr, false);       // no exceptions: a discarded value on an error
        if (root.is_discarded() || !root.is_object())
        {
            why = "it is not JSON";
            return false;
        }
        const auto format = root.find("format");
        if (format == root.end() || !format->is_string() || format->get<std::string>() != "tee-settings")
        {
            why = "it is not a settings file (no \"format\": \"tee-settings\")";
            return false;
        }
        const auto version = root.find("version");
        if (version == root.end() || !version->is_number_integer() || version->get<int>() != kVersion)
        {
            why = "its version is not " + std::to_string(kVersion);
            return false;
        }

        Settings s;
        const auto bad = [&why](const char* what) {
            why = std::string("its ") + what + " are not what this version reads";
            return false;
        };
        if (const auto audio = root.find("audio"); audio != root.end())
        {
            if (!audio->is_object() || !text(*audio, "device", s.audioDevice) || !number(*audio, "rate", s.audioRate, 0, 768000)
                || !number(*audio, "buffer", s.audioBuffer, 16, 8192))
                return bad("audio settings");
        }
        if (const auto keys = root.find("keys"); keys != root.end())
        {
            if (!keys->is_array())
                return bad("key bindings");
            std::vector<input::Binding> bindings;
            for (const json& row : *keys)
            {
                if (!row.is_object() || !row.contains("key") || !row.contains("action") || !row["action"].is_string())
                    return bad("key bindings");
                input::Binding b;
                b.action = input::actionFromName(row["action"].get<std::string>());
                if (row["key"].is_string())
                    b.key = input::keyFromName(row["key"].get<std::string>());
                else if (row["key"].is_number_integer())
                    b.key = row["key"].get<int>();
                if (b.key <= 0 || b.key > 511 || b.action == input::Action::none)
                    return bad("key bindings");
                bindings.push_back(b);
            }
            s.bindings = std::move(bindings);
        }
        if (const auto driver = root.find("driver"); driver != root.end())
        {
            if (!driver->is_object() || !readEasing(*driver, "throttle", s.driver.throttle)
                || !readEasing(*driver, "brake", s.driver.brake) || !readEasing(*driver, "clutch", s.driver.clutch)
                || !readEasing(*driver, "handbrake", s.driver.handbrake) || !flag(*driver, "autoClutch", s.driver.autoClutch)
                || !number(*driver, "offHold", s.driver.offHold, 0.1, 10.0))
                return bad("driver settings");
        }
        std::string units;
        if (!text(root, "units", units) || !(units.empty() || units == "metric" || units == "imperial"))
            return bad("units");
        s.units = units == "imperial" ? Units::imperial : Units::metric;
        if (!text(root, "unitChoices", s.unitChoices))
            return bad("unit choices");
        if (!text(root, "lastBuild", s.lastBuild))
            return bad("last build");
        if (const auto recent = root.find("recentBuilds"); recent != root.end())
        {
            if (!recent->is_array())
                return bad("recent builds");
            for (const json& f : *recent)
            {
                if (!f.is_string())
                    return bad("recent builds");
                s.recentBuilds.push_back(f.get<std::string>());
            }
        }
        if (const auto window = root.find("window"); window != root.end())
        {
            if (!window->is_object() || !number(*window, "width", s.windowWidth, 0, 16384)
                || !number(*window, "height", s.windowHeight, 0, 16384))
                return bad("window settings");
        }
        out = std::move(s);
        return true;
    }

    Loaded load(const fs::path& given)
    {
        Loaded loaded;
        const fs::path path = given.empty() ? file() : given;
        std::error_code ec;
        if (!fs::is_regular_file(path, ec))
            return loaded;
        loaded.found = true;
        std::string content;
        {
            std::ifstream in(path, std::ios::binary);
            std::ostringstream all;
            all << in.rdbuf();
            content = all.str();
        }
        std::string why;
        if (parse(content, loaded.settings, why))
            return loaded;
        // not ours to read: aside, under the first name that is free, and the defaults
        loaded.settings = {};
        fs::path aside = path;
        aside += ".bad";
        for (int n = 2; fs::exists(aside, ec) && n < 1000; ++n)
        {
            aside = path;
            aside += ".bad" + std::to_string(n);
        }
        fs::rename(path, aside, ec);
        loaded.note = path.string() + ": " + why + "; "
                      + (ec ? "it could not be moved aside, and is left as it is" : "it was moved to " + aside.string())
                      + ", and the defaults are used";
        return loaded;
    }

    bool save(const Settings& s, std::string& why, const fs::path& given)
    {
        const fs::path path = given.empty() ? file() : given;
        std::error_code ec;
        if (!path.parent_path().empty())
            fs::create_directories(path.parent_path(), ec);
        fs::path beside = path;
        beside += ".new";
        {
            std::ofstream out(beside, std::ios::binary | std::ios::trunc);
            out << toJson(s);
            out.flush();
            if (!out)
            {
                why = "cannot write " + beside.string();
                fs::remove(beside, ec);
                return false;
            }
        }
        fs::rename(beside, path, ec);
        if (ec)
        {
            why = "cannot put " + path.string() + " in place: " + ec.message();
            fs::remove(beside, ec);
            return false;
        }
        return true;
    }
}
