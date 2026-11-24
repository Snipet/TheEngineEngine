// host/include/tee/Scenario.h: presets and scenarios for the reference player, read from JSON (docs/formats.md has
// both formats, with examples).
//
// A preset is the dials by name: what the reference's panel was set to. A scenario is a timeline: what the player does
// with the dials, the keys, the pedals, the lever and the buttons, at times in seconds from the start. Together with a
// sample rate and a block size they are everything a render depends on (Render.h).
//
// A scenario is flat once it is loaded: each event changes one thing at one time. What the file writes more
// comfortably is unrolled here: an object that sets several things is several events at its time; a "ramp" is the
// steps of a straight line, one every kRampStep seconds; a "tap" is a press and a release. So two renders of one
// scenario see the same inputs at the same times, whatever their block size.
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tee
{
    struct Preset
    {
        std::string name;
        std::string description;
        std::vector<std::pair<std::string, double>> dials;   // each dial named, in the file's order
    };

    // Reads a preset. False, with `error` saying what is wrong ("v8.json: 'dials' is an object of numbers"), when the
    // file cannot be read or is not one.
    bool loadPreset(const std::filesystem::path& file, Preset& preset, std::string& error);
    bool parsePreset(std::string_view json, std::string_view fileName, Preset& preset, std::string& error);
    // A preset as the JSON loadPreset reads.
    std::string presetJson(const Preset& preset);

    struct Event
    {
        enum class Kind
        {
            Dial,      // a dial takes `value`: `name` is the dial's
            Input,     // a pedal, the lever or a wheel takes `value`: `name` is the field of RefPlayer::Input
            Press,     // buttons go down: `bits`
            Release,   // buttons come up: `bits`
            KeyDown,   // a key goes down: `value` the note, `velocity` 0 to 1
            KeyUp,     // the key comes up
        };

        double time = 0.0;   // seconds from the start
        Kind kind = Kind::Dial;
        std::string name;
        double value = 0.0;
        double velocity = 0.0;
        int bits = 0;
    };

    struct Scenario
    {
        std::string name;
        std::string description;
        double seconds = 0.0;        // how long it lasts
        std::vector<Event> events;   // by time; events of one time in the file's order
    };

    // A ramp's steps are this far apart, in seconds.
    inline constexpr double kRampStep = 0.005;

    bool loadScenario(const std::filesystem::path& file, Scenario& scenario, std::string& error);
    bool parseScenario(std::string_view json, std::string_view fileName, Scenario& scenario, std::string& error);

    // A button's bit by the name a scenario writes ("paddleUp", "paddleDown", "start", and host.fm's others: cross,
    // square, circle, triangle, r2, l2, share, options, r3, l3, plus, minus, dialRight, dialLeft, return, up, right,
    // down, left); 0 when there is no button of that name.
    int buttonBit(std::string_view name) noexcept;
} // namespace tee
