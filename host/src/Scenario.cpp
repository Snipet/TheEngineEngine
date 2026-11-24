// host/src/Scenario.cpp: presets and scenarios from JSON (Scenario.h).
#include "tee/Scenario.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>

namespace tee
{
    namespace
    {
        using Json = nlohmann::ordered_json;   // an object keeps the file's order

        constexpr const char* kPresetFormat = "tee-ref-preset/1";
        constexpr const char* kScenarioFormat = "tee-ref-scenario/1";

        bool readFile(const std::filesystem::path& file, std::string& text, std::string& error)
        {
            std::ifstream in(file, std::ios::binary);
            if (!in)
            {
                error = file.generic_string() + ": cannot be read";
                return false;
            }
            std::ostringstream all;
            all << in.rdbuf();
            text = all.str();
            return true;
        }

        // The document, checked to be an object of the format named. Parse errors come back as text, never thrown.
        bool parseDocument(std::string_view json, std::string_view fileName, const char* format, Json& document,
                           std::string& error)
        {
            document = Json::parse(json.begin(), json.end(), nullptr, false, true);   // no exceptions; // comments allowed
            if (document.is_discarded() || !document.is_object())
            {
                error = std::string(fileName) + ": is not a JSON object";
                return false;
            }
            const auto found = document.find("format");
            if (found == document.end() || !found->is_string() || found->get<std::string>() != format)
            {
                error = std::string(fileName) + ": \"format\" is not \"" + format + "\"";
                return false;
            }
            return true;
        }

        std::string textOf(const Json& document, const char* key)
        {
            const auto found = document.find(key);
            return found != document.end() && found->is_string() ? found->get<std::string>() : std::string();
        }

        // An object of numbers (a bool counts as 0 or 1), in the file's order.
        bool numbersOf(const Json& object, std::vector<std::pair<std::string, double>>& out)
        {
            if (!object.is_object())
                return false;
            for (const auto& [key, value] : object.items())
            {
                if (value.is_boolean())
                    out.emplace_back(key, value.get<bool>() ? 1.0 : 0.0);
                else if (value.is_number())
                    out.emplace_back(key, value.get<double>());
                else
                    return false;
            }
            return true;
        }

        struct ButtonName
        {
            const char* name;
            int bit;
        };
        // host.fm's bits; paddleUp and paddleDown are the right and the left paddle, start the PS button.
        constexpr ButtonName kButtons[] = {
            {"cross", 0}, {"square", 1}, {"circle", 2}, {"triangle", 3}, {"paddleUp", 4}, {"paddleDown", 5},
            {"r2", 6}, {"l2", 7}, {"share", 8}, {"options", 9}, {"r3", 10}, {"l3", 11}, {"plus", 12}, {"minus", 13},
            {"dialRight", 14}, {"dialLeft", 15}, {"return", 16}, {"start", 17}, {"up", 18}, {"right", 19},
            {"down", 20}, {"left", 21},
        };

        // "press": ["paddleUp"] or "press": "paddleUp": the bits, or -1 when a name is no button.
        int bitsOf(const Json& value)
        {
            int bits = 0;
            const auto one = [&](const Json& name) {
                const int bit = name.is_string() ? buttonBit(name.get<std::string>()) : 0;
                if (bit == 0)
                    return false;
                bits |= bit;
                return true;
            };
            if (value.is_array())
            {
                for (const Json& name : value)
                    if (!one(name))
                        return -1;
                return bits;
            }
            return one(value) ? bits : -1;
        }

        constexpr const char* kInputs[] = {"throttle", "brake", "clutch", "steering", "gear", "modwheel", "pitchbend",
                                           "aftertouch", "sustain"};
    } // namespace

    int buttonBit(std::string_view name) noexcept
    {
        for (const ButtonName& button : kButtons)
            if (name == button.name)
                return 1 << button.bit;
        return 0;
    }

    bool parsePreset(std::string_view json, std::string_view fileName, Preset& preset, std::string& error)
    {
        Json document;
        if (!parseDocument(json, fileName, kPresetFormat, document, error))
            return false;
        preset = Preset{};
        preset.name = textOf(document, "name");
        preset.description = textOf(document, "description");
        const auto dials = document.find("dials");
        if (dials == document.end() || !numbersOf(*dials, preset.dials))
        {
            error = std::string(fileName) + ": \"dials\" is an object of numbers, a dial's name to its value";
            return false;
        }
        return true;
    }

    bool loadPreset(const std::filesystem::path& file, Preset& preset, std::string& error)
    {
        std::string text;
        if (!readFile(file, text, error))
            return false;
        if (!parsePreset(text, file.filename().generic_string(), preset, error))
            return false;
        if (preset.name.empty())
            preset.name = file.stem().generic_string();
        return true;
    }

    std::string presetJson(const Preset& preset)
    {
        // Written by hand so that the dials keep their order and each sits on a line of its own.
        const auto quoted = [](const std::string& text) { return Json(text).dump(); };
        std::string out = "{\n  \"format\": \"" + std::string(kPresetFormat) + "\",\n  \"name\": " + quoted(preset.name)
                          + ",\n  \"description\": " + quoted(preset.description) + ",\n  \"dials\": {\n";
        for (std::size_t k = 0; k < preset.dials.size(); ++k)
        {
            out += "    " + quoted(preset.dials[k].first) + ": " + Json(preset.dials[k].second).dump();
            out += k + 1 < preset.dials.size() ? ",\n" : "\n";
        }
        out += "  }\n}\n";
        return out;
    }

    bool parseScenario(std::string_view json, std::string_view fileName, Scenario& scenario, std::string& error)
    {
        Json document;
        if (!parseDocument(json, fileName, kScenarioFormat, document, error))
            return false;
        const std::string file(fileName);
        scenario = Scenario{};
        scenario.name = textOf(document, "name");
        scenario.description = textOf(document, "description");
        const auto seconds = document.find("seconds");
        if (seconds == document.end() || !seconds->is_number() || seconds->get<double>() <= 0.0
            || seconds->get<double>() > 86400.0)
        {
            error = file + ": \"seconds\" is how long the scenario lasts, a number above 0";
            return false;
        }
        scenario.seconds = seconds->get<double>();
        const auto events = document.find("events");
        if (events == document.end() || !events->is_array())
        {
            error = file + ": \"events\" is an array";
            return false;
        }

        // The file's events in time order (those of one time in the file's order), so that a ramp starts from the
        // value its input had been given by then.
        std::vector<const Json*> ordered;
        for (const Json& event : *events)
        {
            const auto t = event.is_object() ? event.find("t") : event.end();
            if (!event.is_object() || t == event.end() || !t->is_number() || t->get<double>() < 0.0)
            {
                error = file + ": each event is an object with \"t\", its time in seconds from the start";
                return false;
            }
            ordered.push_back(&event);
        }
        std::stable_sort(ordered.begin(), ordered.end(),
                         [](const Json* a, const Json* b) { return (*a)["t"].get<double>() < (*b)["t"].get<double>(); });

        std::map<std::string, double> inputNow;   // each input's last target
        for (const Json* raw : ordered)
        {
            const Json& event = *raw;
            const double t = event["t"].get<double>();
            const std::string at = file + ": the event at t = " + Json(t).dump();
            double ramp = 0.0;
            for (const auto& [key, value] : event.items())
            {
                if (key == "t" || key == "note" || key == "velocity" || key == "hold")
                    continue;   // "note" is a remark for the reader; the others belong to "key" and "tap"
                if (key == "ramp")
                {
                    if (!value.is_number() || value.get<double>() < 0.0)
                    {
                        error = at + ": \"ramp\" is a time in seconds";
                        return false;
                    }
                    ramp = value.get<double>();
                }
                else if (key != "dials" && key != "input" && key != "press" && key != "release" && key != "tap"
                         && key != "key" && key != "keyUp")
                {
                    error = at + ": \"" + key + "\" is not something an event can hold (docs/formats.md)";
                    return false;
                }
            }
            if (const auto dials = event.find("dials"); dials != event.end())
            {
                std::vector<std::pair<std::string, double>> values;
                if (!numbersOf(*dials, values))
                {
                    error = at + ": \"dials\" is an object of numbers";
                    return false;
                }
                for (const auto& [name, value] : values)
                    scenario.events.push_back({t, Event::Kind::Dial, name, value, 0.0, 0});
            }
            if (const auto input = event.find("input"); input != event.end())
            {
                std::vector<std::pair<std::string, double>> values;
                if (!numbersOf(*input, values))
                {
                    error = at + ": \"input\" is an object of numbers";
                    return false;
                }
                for (const auto& [name, value] : values)
                {
                    if (std::find_if(std::begin(kInputs), std::end(kInputs), [&](const char* known) { return name == known; })
                        == std::end(kInputs))
                    {
                        error = at + ": \"" + name + "\" is no input (throttle, brake, clutch, steering, gear, "
                                "modwheel, pitchbend, aftertouch, sustain)";
                        return false;
                    }
                    const double from = inputNow[name];
                    const int steps = ramp > 0.0 ? std::max(1, static_cast<int>(std::ceil(ramp / kRampStep - 1.0e-9))) : 1;
                    for (int k = 1; k <= steps; ++k)
                    {
                        const double share = static_cast<double>(k) / steps;
                        const double when = ramp > 0.0 ? t + ramp * share : t;
                        scenario.events.push_back({when, Event::Kind::Input, name, from + (value - from) * share, 0.0, 0});
                    }
                    inputNow[name] = value;
                }
            }
            else if (ramp > 0.0)
            {
                error = at + ": \"ramp\" goes with \"input\"";
                return false;
            }
            for (const char* which : {"press", "release", "tap"})
            {
                const auto found = event.find(which);
                if (found == event.end())
                    continue;
                const int bits = bitsOf(*found);
                if (bits <= 0)
                {
                    error = at + ": \"" + which + "\" names buttons (paddleUp, paddleDown, start, ...)";
                    return false;
                }
                if (std::string_view(which) == "tap")
                {
                    const auto hold = event.find("hold");
                    const double held = hold != event.end() && hold->is_number() ? hold->get<double>() : 0.1;
                    scenario.events.push_back({t, Event::Kind::Press, {}, 0.0, 0.0, bits});
                    scenario.events.push_back({t + std::max(held, 0.0), Event::Kind::Release, {}, 0.0, 0.0, bits});
                }
                else
                    scenario.events.push_back({t, std::string_view(which) == "press" ? Event::Kind::Press : Event::Kind::Release,
                                               {}, 0.0, 0.0, bits});
            }
            if (const auto key = event.find("key"); key != event.end())
            {
                const auto velocity = event.find("velocity");
                if (!key->is_number() || (velocity != event.end() && !velocity->is_number()))
                {
                    error = at + ": \"key\" is a note number, and \"velocity\" 0 to 1";
                    return false;
                }
                scenario.events.push_back({t, Event::Kind::KeyDown, {}, key->get<double>(),
                                           velocity != event.end() ? velocity->get<double>() : 1.0, 0});
            }
            if (const auto keyUp = event.find("keyUp"); keyUp != event.end())
            {
                if (!keyUp->is_boolean() || !keyUp->get<bool>())
                {
                    error = at + ": \"keyUp\" is true";
                    return false;
                }
                scenario.events.push_back({t, Event::Kind::KeyUp, {}, 0.0, 0.0, 0});
            }
        }
        std::stable_sort(scenario.events.begin(), scenario.events.end(),
                         [](const Event& a, const Event& b) { return a.time < b.time; });
        return true;
    }

    bool loadScenario(const std::filesystem::path& file, Scenario& scenario, std::string& error)
    {
        std::string text;
        if (!readFile(file, text, error))
            return false;
        if (!parseScenario(text, file.filename().generic_string(), scenario, error))
            return false;
        if (scenario.name.empty())
            scenario.name = file.stem().generic_string();
        return true;
    }
} // namespace tee
