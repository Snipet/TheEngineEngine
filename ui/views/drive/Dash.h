// ui/views/drive/Dash.h: DRIVE's dashboards (the owner's ask, 25 Nov: "Enhance the Drive view to show more
// analog/digital gauges and offer different styles"). The styles the driver picks between, kept in the settings
// (FrameServices::driveStyle, a line of the interface's own settings text):
//   STANDARD  the first look: the tachometer, the speedometer, a column of readouts (unchanged)
//   CLASSIC   an analog cluster: the two large dials, eight small ones with needles, a gear window and lamps
//   DIGITAL   a modern dash: a segmented rev bar, the speed in large digits, tiles with bars, torque, power, g
//   RACE      a race dash: shift lights, a huge gear, the revs in digits, alarm boxes that fill and say why
// Every value is telemetry that the simulator publishes today (sim/spec/telemetry.fm); a dash a name it lacks reads a
// dash ("-"). Colour is never the only sign: an alarm has its word (HOT, LOW, LEAN, KNOCK), a shift light its count
// and the word SHIFT. Canvas primitives only; nothing here moves by itself (time is the workspace's tick).
#pragma once

#include "app/Look.h"
#include "app/Workspace.h"

#include <string>
#include <string_view>
#include <vector>

namespace tee::ui::dash
{
    enum Style : int
    {
        kStandard = 0,
        kClassic,
        kDigital,
        kRace,
        kStyles,
    };
    const char* styleName(int style);          // "Standard", "Classic", "Digital", "Race"
    const char* styleKey(int style);           // the settings' word: "standard" ...
    int styleOf(std::string_view key);         // back; -1 for a word that is none
    const char* styleHelp(int style);          // its hover help

    // The settings text's line for DRIVE's style ("drive.style=race"), and its reading: `text` the interface's own
    // settings lines; the style's line taken out of it (the rest left for the units), -1 when it has none.
    inline constexpr std::string_view kSettingsKey = "drive.style";
    int takeStyle(std::string& text);
    std::string styleLine(int style);          // "" for STANDARD (the default is not written)

    // What the dashes read, in SI, NaN where the program has no such name.
    struct Values
    {
        double rpm, redline, speed, gear, boost, lambda, coolant, oilT, oilP, egt, fuel, knockRetard, torque, power,
            accel;
        bool limiter = false;
        int lights = 0;                        // shift lights lit, 0..5 (look::shiftLightBefore)
        bool shift = false;                    // the last of them: time to change up
        static Values read(const Live& live);
    };

    // One of the eight channels a dash shows beside the revs and the speed: BOOST (VACUUM below the air outside),
    // LAMBDA, OIL PRESS, OIL TEMP, WATER, EXHAUST, FUEL, KNOCK. `alarm` is the word when it is out of its range (null:
    // within it); `level` 0..1 on the channel's span and `mark` the span's normal point (-1: none), for a bar.
    struct Channel
    {
        const char* label;
        std::string value, unit;
        double level, mark;
        const char* alarm;
        bool hot;                              // the alarm is a heat's (hotter ink) rather than a warning's
    };
    std::vector<Channel> channels(const Values& v, const Units& u);
    std::string gearText(double gear);

    // The styles drawn by hand into the dash's room `r` (CLASSIC's dials are Gauge items; its band is drawn here).
    void drawDigital(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Values& v, const Units& u);
    void drawRace(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Values& v, const Units& u);
    void drawClassicBand(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Values& v, const Units& u);

    // Parts: the shift lights (5 in a row, or 10 closing from both ends) centred in r, the word SHIFT at their right
    // when the last is lit; a longitudinal g meter (car.accel over g, -1 to 1) in r with its caption and number.
    void shiftLights(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Values& v, int count);
    void gMeter(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, double accel);

    // The CLASSIC cluster's small dials: their titles, the channel each reads, its quantity and span in SI, where its
    // red band starts (NaN: none), and the help line.
    struct Dial
    {
        const char* title;
        const char* name;                      // telemetry name
        Quantity quantity;
        double lo, hi, red;
        int decimals;                          // the digits' (in the user's unit)
        const char* help;
    };
    // A span re-chosen to round steps in the unit shown: a step of 1, 2, 2.5 or 5 times a power of ten, `maxSteps` steps
    // at most (`even`: two or four, so a step marks the middle), the ends on steps and covering [lo, hi].
    struct Scale
    {
        double lo, hi;
        int steps;
    };
    Scale roundScale(double lo, double hi, int maxSteps, bool even = false);
    inline constexpr int kDials = 8;
    const Dial& dial(int i);
}
