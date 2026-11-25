// ui/views/drive/Dash.cpp: see Dash.h.
#include "views/drive/Dash.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tee::ui::dash
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kToRpm = 30.0 / kPi;
        constexpr double kG = 9.80665;
        constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
        const char* const kNames[kStyles] = { "Standard", "Classic", "Digital", "Race" };
        const char* const kKeys[kStyles] = { "standard", "classic", "digital", "race" };
        const char* const kHelp[kStyles] = {
            "The first look: the tachometer, the speedometer and a column of readouts",
            "An analog cluster: the two large dials and eight small ones with needles, a gear window and lamps",
            "A modern digital dash: a segmented rev bar, the speed in large digits, tiles with bars, torque, power and g",
            "A race dash: shift lights, a huge gear, the revs in digits and alarm boxes that fill and say why",
        };

        // The alarms' limits. Judgement, not the simulator's: where a driver would want to be told (a road engine's
        // water past 110 C, oil past 130 C, oil pressure under 70 kPa while it runs, 
        // lambda past 1.08 (DRIVE's first rule), any knock retard over half a degree). The exhaust's is the port gas's mass
        // mean (hotter than a thermocouple reads, 1000 C at full load is ordinary): past 1050 C.
        constexpr double kWaterHot = 383.15, kOilHot = 403.15, kOilLow = 70e3, kExhaustHot = 1323.15, kLean = 1.08,
                         kKnock = 0.5;

        constexpr Dial kDialTable[kDials] = {
            { "Boost", "boost.pressure", Quantity::pressure, -100e3, 200e3, kNaN, 0,
              "The manifold's pressure over the air outside: below zero a vacuum, above it boost" },
            { "Oil press", "engine.oilPressure", Quantity::pressure, 0.0, 700e3, kNaN, 0,
              "The oil's pressure in the gallery, over the air outside" },
            { "Oil temp", "engine.oilTemperature", Quantity::temperature, 313.15, 423.15, kOilHot, 0,
              "The oil's temperature in the sump; the red band from 130 C" },
            { "Water", "engine.coolant", Quantity::temperature, 313.15, 403.15, kWaterHot, 0,
              "The coolant's temperature leaving the head; the red band from 110 C" },
            { "Exhaust", "exhaust.portTemperature", Quantity::temperature, 473.15, 1373.15, kExhaustHot, 0,
              "The exhaust gas at the port: its mass mean, hotter than a thermocouple reads; the red band from 1050 C" },
            { "Lambda", "ecu.lambda", Quantity::ratio, 0.6, 1.4, kNaN, 2,
              "The air-fuel ratio over stoichiometric, as the wide-band sensor in the collector reads it: under 1 rich, "
              "over 1 lean" },
            { "Fuel", "ecu.fuelFlow", Quantity::flow, 0.0, 0.04, kNaN, 1, "The fuel the injectors deliver now" },
            { "Knock", "ecu.knockRetard", Quantity::angle, 0.0, 10.0, kNaN, 1, "The spark taken back for knock, degrees" },
        };

        bool fin(double v) { return std::isfinite(v); }
        double frac(double v, double lo, double hi) { return fin(v) && hi > lo ? std::clamp((v - lo) / (hi - lo), 0.0, 1.0) : kNaN; }

        funkgui::Col alarmInk(const look::Palette& p, const Channel& ch) { return ch.hot ? p.hotter : p.warning; }

        // A text style at `px` (tabular digits), for the large numbers.
        funkgui::TextStyle big(float px) { return { px, 0.0f, 0.02f, true }; }

        // Text fitted into a width: the largest px (down to `lo`) at which `text` is no wider than `w`.
        funkgui::TextStyle fit(funkgui::Canvas& c, const char* text, float w, float px, float lo)
        {
            funkgui::TextStyle s = big(px);
            while (s.px > lo && c.textWidth(text, s) > w)
                s.px = std::max(lo, s.px - 2.0f);
            return s;
        }
    }

    const char* styleName(int s) { return kNames[std::clamp(s, 0, kStyles - 1)]; }
    const char* styleKey(int s) { return kKeys[std::clamp(s, 0, kStyles - 1)]; }
    const char* styleHelp(int s) { return kHelp[std::clamp(s, 0, kStyles - 1)]; }
    int styleOf(std::string_view key)
    {
        for (int i = 0; i < kStyles; ++i)
            if (key == kKeys[i])
                return i;
        return -1;
    }

    int takeStyle(std::string& text)
    {
        int style = -1;
        std::string rest;
        std::size_t i = 0;
        while (i < text.size())
        {
            std::size_t e = text.find('\n', i);
            if (e == std::string::npos)
                e = text.size();
            const std::string line = text.substr(i, e - i);
            i = e + 1;
            const std::size_t eq = line.find('=');
            if (eq != std::string::npos && std::string_view(line).substr(0, eq) == kSettingsKey)
            {
                std::string word = line.substr(eq + 1);
                while (!word.empty() && (word.back() == ' ' || word.back() == '\r'))
                    word.pop_back();
                style = styleOf(word);
                continue;
            }
            rest += line + "\n";
        }
        text = rest;
        return style;
    }

    std::string styleLine(int style)
    {
        if (style <= kStandard || style >= kStyles)
            return {};
        return std::string(kSettingsKey) + "=" + styleKey(style) + "\n";
    }

    const Dial& dial(int i) { return kDialTable[std::clamp(i, 0, kDials - 1)]; }

    Scale roundScale(double lo, double hi, int maxSteps, bool even)
    {
        if (!(hi > lo) || !fin(lo) || !fin(hi))
            return { lo, hi, std::max(1, maxSteps) };
        const double span = hi - lo, raw = span / std::max(1, maxSteps);
        double mag = std::pow(10.0, std::floor(std::log10(raw)));
        for (int tries = 0; tries < 8; ++tries, mag *= 10.0)
            for (const double m : { 1.0, 2.0, 2.5, 5.0 })
            {
                const double step = m * mag;
                const double a = std::floor(lo / step + 1e-9) * step, b = std::ceil(hi / step - 1e-9) * step;
                const int n = static_cast<int>(std::lround((b - a) / step));
                if (n <= maxSteps && !(even && n % 2 != 0))
                    return { a, b, std::max(1, n) };
                if (even && n % 2 != 0 && n + 1 <= maxSteps)   // one step more at the top: an even count
                    return { a, b + step, n + 1 };
            }
        return { lo, hi, std::max(1, maxSteps) };
    }

    Values Values::read(const Live& live)
    {
        Values v;
        v.rpm = live.get(telemetry::kSpeed) * kToRpm;
        v.redline = live.get(telemetry::kRedline) * kToRpm;
        v.speed = live.get(telemetry::kCarSpeed);
        v.gear = live.get(telemetry::kGear);
        v.boost = live.get(telemetry::kBoost);
        v.lambda = live.lambda();
        v.coolant = live.get(telemetry::kCoolant);
        v.oilT = live.get("engine.oilTemperature");
        v.oilP = live.get(telemetry::kOilPressure);
        v.egt = live.get("exhaust.portTemperature");
        v.fuel = live.get("ecu.fuelFlow");
        v.knockRetard = live.get("ecu.knockRetard");
        v.torque = live.get("engine.torque");
        v.power = live.get("engine.power");
        v.accel = live.get("car.accel");
        const double flags = live.get(telemetry::kFlags);
        v.limiter = fin(flags) && (std::lround(flags) & 1) != 0;
        if (fin(v.rpm) && fin(v.redline) && v.redline > 0.0)
            for (int k = 0; k < 5; ++k)
                if (v.rpm >= v.redline - look::shiftLightBefore(v.redline, k))
                    v.lights = k + 1;
        v.shift = v.lights == 5;
        return v;
    }

    std::string gearText(double g)
    {
        return !fin(g) ? "-" : g < -0.5 ? "R" : g < 0.5 ? "N" : std::to_string(std::lround(g));
    }

    std::vector<Channel> channels(const Values& v, const Units& u)
    {
        const auto num = [&](Quantity q, double si, int dec = -1) { return fin(si) ? u.number(q, si, dec) : std::string("-"); };
        const auto unit = [&](Quantity q) { return std::string(u.unit(q).name); };
        const bool running = fin(v.rpm) && v.rpm > 400.0;
        std::vector<Channel> out;
        out.reserve(8);
        const bool vacuum = fin(v.boost) && v.boost < 0.0;
        out.push_back({ vacuum ? "VACUUM" : "BOOST", num(Quantity::pressure, fin(v.boost) ? std::fabs(v.boost) : v.boost),
                        unit(Quantity::pressure), frac(v.boost, -100e3, 200e3), 1.0 / 3.0, nullptr, false });
        out.push_back({ "LAMBDA", look::num(v.lambda, "%.2f"), "", frac(v.lambda, 0.6, 1.4), 0.5,
                        fin(v.lambda) && v.lambda > kLean ? "LEAN" : nullptr, false });
        out.push_back({ "OIL PRESS", num(Quantity::pressure, v.oilP), unit(Quantity::pressure), frac(v.oilP, 0.0, 700e3), -1.0,
                        running && fin(v.oilP) && v.oilP < kOilLow ? "LOW" : nullptr, false });
        out.push_back({ "OIL TEMP", num(Quantity::temperature, v.oilT, 0), unit(Quantity::temperature),
                        frac(v.oilT, 313.15, 423.15), frac(kOilHot, 313.15, 423.15), fin(v.oilT) && v.oilT > kOilHot ? "HOT" : nullptr,
                        true });
        out.push_back({ "WATER", num(Quantity::temperature, v.coolant, 0), unit(Quantity::temperature),
                        frac(v.coolant, 313.15, 403.15), frac(kWaterHot, 313.15, 403.15),
                        fin(v.coolant) && v.coolant > kWaterHot ? "HOT" : nullptr, true });
        out.push_back({ "EXHAUST", num(Quantity::temperature, v.egt, 0), unit(Quantity::temperature), frac(v.egt, 473.15, 1373.15),
                        frac(kExhaustHot, 473.15, 1373.15), fin(v.egt) && v.egt > kExhaustHot ? "HOT" : nullptr, true });
        out.push_back({ "FUEL", num(Quantity::flow, v.fuel), unit(Quantity::flow), frac(v.fuel, 0.0, 0.04), -1.0, nullptr, false });
        out.push_back({ "KNOCK", num(Quantity::angle, v.knockRetard, 1), unit(Quantity::angle), frac(v.knockRetard, 0.0, 10.0), -1.0,
                        fin(v.knockRetard) && v.knockRetard > kKnock ? "KNOCK" : nullptr, false });
        return out;
    }

    void shiftLights(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Values& v, int count)
    {
        const int n = count >= 10 ? 10 : 5;
        const float wordW = c.textWidth("SHIFT", look::kCaps) + 12.0f;
        const float rad = std::clamp(std::min(r.h * 0.5f - 1.0f, (r.w - wordW) / (static_cast<float>(n) * 2.8f)), 3.0f, 12.0f);
        const float step = rad * 2.8f, w = step * static_cast<float>(n - 1);
        const float x0 = r.centreX() - (w + wordW) * 0.5f, cy = r.y + r.h * 0.5f;
        const funkgui::Col inks[5] = { p.ok, p.ok, p.warning, p.warning, p.hotter };
        for (int i = 0; i < n; ++i)
        {
            // Five in a row; ten close from both ends to the middle.
            const int k = n == 5 ? i : (i < 5 ? i : 9 - i);
            const bool lit = k < v.lights;
            const funkgui::Col ink = v.shift ? p.hotter : inks[k];
            const float x = x0 + step * static_cast<float>(i);
            if (lit)
                c.disc(x, cy, rad, ink);
            else
                c.disc(x, cy, rad, look::kClear, 1.25f, p.dim);
        }
        c.text("SHIFT", x0 + w + rad + 10.0f, cy - 5.0f, look::kCaps, v.shift ? p.hotter : p.faint);
    }

    void gMeter(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, double accel)
    {
        const double g = fin(accel) ? accel / kG : kNaN;
        c.text("LONG G", r.x, r.y, look::kCaps, p.label);
        c.text((look::num(g, "%+.2f") + " G").c_str(), r.right(), r.y - 2.0f, look::kNumberS, fin(g) ? p.value : p.dim,
               funkgui::Align::right);
        const float y = r.y + 20.0f, h = 6.0f, cx = r.centreX();
        c.rrect(r.x, y, r.w, h, 3.0f, p.over(p.faint, 0.9f));
        for (int k = -2; k <= 2; ++k)
        {
            const float x = cx + static_cast<float>(k) * r.w * 0.25f;
            c.segment(x, y + h + 2.0f, x, y + h + (k == 0 ? 7.0f : 5.0f), 1.0f, p.label);
        }
        if (fin(g))
        {
            const float x = cx + static_cast<float>(std::clamp(g, -1.0, 1.0)) * r.w * 0.5f;
            c.rrect(std::min(cx, x), y, std::fabs(x - cx), h, 2.0f, g < 0.0 ? p.cold : p.value);
            c.segment(x, y - 3.0f, x, y + h + 3.0f, 2.0f, p.value);
        }
        if (r.h >= 40.0f)
        {
            c.text("BRAKE", r.x, y + h + 8.0f, look::kUnit, p.dim);
            c.text("PULL", r.right(), y + h + 8.0f, look::kUnit, p.dim, funkgui::Align::right);
        }
    }

    namespace
    {
        // A tile: its label (and alarm word), the number with its unit, a bar on the channel's span; outlined in the
        // alarm's ink while it is out of range.
        void tile(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Channel& ch)
        {
            const bool alarm = ch.alarm != nullptr;
            c.rrect(r.x, r.y, r.w, r.h, 5.0f, p.raised, alarm ? 2.0f : 1.0f, alarm ? alarmInk(p, ch) : p.edge);
            const float x = r.x + 8.0f;
            if (r.h < 50.0f)
            {
                // Low: the label (and the alarm's word) at the left, the number at the right, on one line.
                const std::string label = alarm ? std::string(ch.label) + "  " + ch.alarm : std::string(ch.label);
                c.text(label.c_str(), x, r.y + 8.0f, look::kCaps, alarm ? alarmInk(p, ch) : p.label);
                const float uw = ch.unit.empty() ? 0.0f : c.textWidth(ch.unit.c_str(), look::kUnit) + 4.0f;
                if (!ch.unit.empty())
                    c.text(ch.unit.c_str(), r.right() - 8.0f, r.y + 10.0f, look::kUnit, p.label, funkgui::Align::right);
                c.text(ch.value.c_str(), r.right() - 8.0f - uw, r.y + 5.0f, look::kNumberS,
                       ch.value == "-" ? p.dim : alarm ? alarmInk(p, ch) : p.value, funkgui::Align::right);
                look::level(c, p, { x, r.bottom() - 9.0f, r.w - 16.0f, 4.0f }, ch.level, alarm ? alarmInk(p, ch) : p.text, ch.mark);
                return;
            }
            c.text(ch.label, x, r.y + 6.0f, look::kCaps, p.label);
            if (alarm)
                c.text(ch.alarm, r.right() - 8.0f, r.y + 6.0f, look::kCaps, alarmInk(p, ch), funkgui::Align::right);
            const funkgui::TextStyle s = r.h >= 50.0f ? look::kNumberM : look::kNumberS;
            const float ny = r.y + 20.0f;
            c.text(ch.value.c_str(), x, ny, s, ch.value == "-" ? p.dim : alarm ? alarmInk(p, ch) : p.value);
            if (!ch.unit.empty())
                c.text(ch.unit.c_str(), x + c.textWidth(ch.value.c_str(), s) + 4.0f, ny + s.px - look::kUnit.px - 1.0f,
                       look::kUnit, p.label);
            look::level(c, p, { x, r.bottom() - 9.0f, r.w - 16.0f, 4.0f }, ch.level, alarm ? alarmInk(p, ch) : p.text, ch.mark);
        }

        // An alarm box (RACE): the label over the number; within range the word OK beside it, out of range the box
        // filled in the alarm's ink with the word in the ground's.
        void alarmBox(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Channel& ch)
        {
            const bool alarm = ch.alarm != nullptr;
            const funkgui::Col fill = alarm ? alarmInk(p, ch) : p.raised, ink = alarm ? p.ground : p.value;
            c.rrect(r.x, r.y, r.w, r.h, 4.0f, fill, 1.0f, alarm ? fill : p.edge);
            const float x = r.x + 7.0f;
            c.text(ch.label, x, r.y + 5.0f, look::kCaps, alarm ? p.ground : p.label);
            const bool known = ch.value != "-";
            c.text(alarm ? ch.alarm : known ? "OK" : "", r.right() - 7.0f, r.y + 5.0f, look::kCaps, alarm ? p.ground : p.ok,
                   funkgui::Align::right);
            const funkgui::TextStyle s = r.h >= 46.0f ? look::kNumberM : look::kNumberS;
            const std::string text = ch.unit.empty() ? ch.value : ch.value + " " + ch.unit;
            c.text(text.c_str(), x, r.y + 19.0f, s, known ? ink : p.dim);
        }
    }

    void drawDigital(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Values& v, const Units& u)
    {
        c.rrect(r.x, r.y, r.w, r.h, look::kRadius, p.panel, 1.0f, p.edge);
        const funkgui::Rect in = r.reduced(look::kPad);
        // The rev bar: segments to the revs, those past the shift point amber, past the redline red; the scale's
        // numerals under it.
        const look::RpmScale sc = look::rpmScale(v.redline);
        constexpr int kSegs = 48;
        const float gap = 2.0f, sw = (in.w - gap * (kSegs - 1)) / kSegs, bh = 16.0f;
        const double t = fin(v.rpm) ? v.rpm / sc.top : 0.0;
        const double shiftAt = fin(v.redline) ? (v.redline - look::shiftLightBefore(v.redline, 0)) / sc.top : 2.0;
        const double redAt = fin(v.redline) ? v.redline / sc.top : 2.0;
        for (int i = 0; i < kSegs; ++i)
        {
            const double at = (i + 0.5) / kSegs;
            const funkgui::Col on = at >= redAt ? p.hotter : at >= shiftAt ? p.warning : p.value;
            const bool lit = at <= t;
            const float x = in.x + static_cast<float>(i) * (sw + gap);
            c.rrect(x, in.y, sw, bh, 1.5f, lit ? on : at >= redAt ? p.over(p.hotter, 0.25f) : p.over(p.faint, 0.9f));
        }
        const int steps = static_cast<int>(std::lround(sc.top / sc.step));
        for (int k = 0; k <= steps; ++k)
        {
            const double rpm = k * sc.step;
            const float x = in.x + in.w * static_cast<float>(rpm / sc.top);
            const std::string n = std::to_string(static_cast<int>(std::lround(rpm / sc.divisor)));
            c.text(n.c_str(), std::clamp(x, in.x + 4.0f, in.right() - 4.0f), in.y + bh + 4.0f, look::kUnit,
                   fin(v.redline) && rpm >= v.redline ? p.hotter : p.label, funkgui::Align::centre);
        }
        c.text(sc.unit, in.right(), in.y + bh + 16.0f, look::kUnit, p.dim, funkgui::Align::right);
        // The middle row: the gear in its box, the speed in large digits, the revs with the shift lights.
        const float my = in.y + bh + 34.0f;
        const auto ch = channels(v, u);
        const float bottomH = 40.0f, tilesTop = my + 78.0f;
        const int cols = in.w >= 860.0f ? 8 : 4, rows = 8 / cols;
        const float tileH = std::clamp((in.bottom() - bottomH - 8.0f - tilesTop) / static_cast<float>(rows) - 6.0f, 36.0f, 58.0f);
        const float rowH = std::min(70.0f, std::max(48.0f, in.bottom() - bottomH - 8.0f - rows * (tileH + 6.0f) - my));
        const float gw = std::min(84.0f, in.w * 0.16f);
        c.rrect(in.x, my, gw, rowH, 6.0f, p.raised, 1.5f, p.text);
        c.text("GEAR", in.x + gw * 0.5f, my + 5.0f, look::kCaps, p.label, funkgui::Align::centre);
        const std::string gear = gearText(v.gear);
        c.text(gear.c_str(), in.x + gw * 0.5f, my + 16.0f, big(std::min(46.0f, rowH - 22.0f)), gear == "-" ? p.dim : p.value,
               funkgui::Align::centre);
        const std::string speed = fin(v.speed) ? u.number(Quantity::speed, v.speed, 0) : "-";
        const funkgui::TextStyle ss = big(std::min(58.0f, rowH - 8.0f));
        const float sx = in.x + in.w * 0.42f;
        c.text(speed.c_str(), sx, my + (rowH - ss.px) * 0.5f - 2.0f, ss, speed == "-" ? p.dim : p.value, funkgui::Align::centre);
        c.text(upper(u.unit(Quantity::speed).name).c_str(), sx + c.textWidth(speed.c_str(), ss) * 0.5f + 6.0f,
               my + (rowH + ss.px) * 0.5f - 14.0f, look::kCaps, p.label);
        const float rx = in.x + in.w * 0.62f, rw = in.right() - rx;
        shiftLights(c, p, { rx, my, rw, 22.0f }, v, 5);
        look::readout(c, p, in.right(), my + 26.0f, v.limiter ? "ENGINE  LIMITER" : "ENGINE", look::num(v.rpm, "%.0f"), "rpm",
                      look::kNumberL, v.limiter ? p.warning : look::kClear, funkgui::Align::right);
        // The tiles.
        const float tw = (in.w - 6.0f * static_cast<float>(cols - 1)) / static_cast<float>(cols);
        for (int i = 0; i < 8; ++i)
        {
            const int col = i % cols, row = i / cols;
            tile(c, p, { in.x + static_cast<float>(col) * (tw + 6.0f), my + rowH + 8.0f + static_cast<float>(row) * (tileH + 6.0f), tw, tileH },
                 ch[static_cast<std::size_t>(i)]);
        }
        // Torque, power, the g meter.
        const float by = in.bottom() - bottomH + 4.0f;
        look::readout(c, p, in.x, by, "TORQUE", fin(v.torque) ? u.number(Quantity::torque, v.torque, 0) : "-",
                      u.unit(Quantity::torque).name);
        look::readout(c, p, in.x + in.w * 0.25f, by, "POWER", fin(v.power) ? u.number(Quantity::power, v.power, 0) : "-",
                      u.unit(Quantity::power).name);
        gMeter(c, p, { in.x + in.w * 0.52f, by, in.w * 0.48f, bottomH - 4.0f }, v.accel);
    }

    void drawRace(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Values& v, const Units& u)
    {
        c.rrect(r.x, r.y, r.w, r.h, look::kRadius, p.panel, 1.0f, p.edge);
        const funkgui::Rect in = r.reduced(look::kPad);
        shiftLights(c, p, { in.x, in.y, in.w, 26.0f }, v, 10);
        const auto ch = channels(v, u);
        const int cols = in.w >= 760.0f ? 8 : 4, rows = 8 / cols;
        const float boxH = std::clamp((in.h - 26.0f - 110.0f) / static_cast<float>(rows) - 6.0f, 38.0f, 52.0f);
        const float boxesTop = in.bottom() - static_cast<float>(rows) * (boxH + 6.0f) + 6.0f;
        // The middle: the gear huge in its box, the revs and the speed at its left, torque, power, g at its right.
        const float my = in.y + 34.0f, mh = boxesTop - 10.0f - my;
        const float gw = std::clamp(mh * 0.85f, 70.0f, 150.0f), gx = in.centreX() - gw * 0.5f;
        c.rrect(gx, my, gw, mh, 8.0f, p.raised, 2.0f, v.shift ? p.hotter : p.text);
        const std::string gear = gearText(v.gear);
        const funkgui::TextStyle gs = big(std::clamp(mh * 0.8f, 30.0f, 120.0f));
        c.text(gear.c_str(), gx + gw * 0.5f, my + (mh - gs.px) * 0.5f, gs, gear == "-" ? p.dim : v.shift ? p.hotter : p.value,
               funkgui::Align::centre);
        c.text("GEAR", gx + gw * 0.5f, my + mh - 15.0f, look::kCaps, p.label, funkgui::Align::centre);
        // Left: the revs large, the speed under them.
        const float lw = gx - in.x - 16.0f, lx = gx - 16.0f;
        const std::string rpm = look::num(v.rpm, "%.0f");
        const funkgui::TextStyle rs = fit(c, "00000", lw, std::clamp(mh * 0.42f, 24.0f, 64.0f), 18.0f);
        c.text(v.limiter ? "RPM  LIMITER" : "RPM", lx, my, look::kCaps, v.limiter ? p.warning : p.label, funkgui::Align::right);
        c.text(rpm.c_str(), lx, my + 14.0f, rs, rpm == "-" ? p.dim : v.limiter ? p.warning : p.value, funkgui::Align::right);
        const std::string speed = fin(v.speed) ? u.number(Quantity::speed, v.speed, 0) : "-";
        look::readout(c, p, lx, my + 22.0f + rs.px, "SPEED", speed, u.unit(Quantity::speed).name, look::kNumberL, look::kClear,
                      funkgui::Align::right);
        // Right: torque, power, the g meter.
        const float rx = gx + gw + 16.0f, rw = in.right() - rx;
        look::readout(c, p, rx, my, "TORQUE", fin(v.torque) ? u.number(Quantity::torque, v.torque, 0) : "-",
                      u.unit(Quantity::torque).name, look::kNumberL);
        look::readout(c, p, rx, my + 46.0f, "POWER", fin(v.power) ? u.number(Quantity::power, v.power, 0) : "-",
                      u.unit(Quantity::power).name, look::kNumberL);
        if (my + 92.0f + 30.0f <= boxesTop)
            gMeter(c, p, { rx, my + 94.0f, std::min(rw, 220.0f), 30.0f }, v.accel);
        else if (rw >= 250.0f)   // beside them
            gMeter(c, p, { rx + 120.0f, my, std::min(rw - 120.0f, 200.0f), 30.0f }, v.accel);
        // The alarm boxes.
        const float bw = (in.w - 6.0f * static_cast<float>(cols - 1)) / static_cast<float>(cols);
        for (int i = 0; i < 8; ++i)
        {
            const int col = i % cols, row = i / cols;
            alarmBox(c, p, { in.x + static_cast<float>(col) * (bw + 6.0f), boxesTop + static_cast<float>(row) * (boxH + 6.0f), bw, boxH },
                     ch[static_cast<std::size_t>(i)]);
        }
    }

    void drawClassicBand(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& r, const Values& v, const Units& u)
    {
        // The gear window, torque and power as a cluster's small displays, the g meter, then a row of lamps with words.
        const float gw = 46.0f;
        c.rrect(r.x, r.y, gw, r.h, 5.0f, p.raised, 1.5f, p.text);
        const std::string gear = gearText(v.gear);
        c.text("GEAR", r.x + gw * 0.5f, r.y + 4.0f, look::kCaps, p.label, funkgui::Align::centre);
        c.text(gear.c_str(), r.x + gw * 0.5f, r.y + 16.0f, big(std::min(30.0f, r.h - 20.0f)), gear == "-" ? p.dim : p.value,
               funkgui::Align::centre);
        float x = r.x + gw + 14.0f;
        x += look::readout(c, p, x, r.y, "TORQUE", fin(v.torque) ? u.number(Quantity::torque, v.torque, 0) : "-",
                           u.unit(Quantity::torque).name, look::kNumberS) + 16.0f;
        x += look::readout(c, p, x, r.y, "POWER", fin(v.power) ? u.number(Quantity::power, v.power, 0) : "-",
                           u.unit(Quantity::power).name, look::kNumberS) + 16.0f;
        const float gmW = std::clamp(r.right() - x, 0.0f, 180.0f);
        if (gmW >= 110.0f)
        {
            gMeter(c, p, { x, r.y, gmW, 30.0f }, v.accel);
            x += gmW + 18.0f;
        }
        // The lamps, on a line of their own under the displays.
        const auto ch = channels(v, u);
        float lx = r.x + gw + 14.0f;
        const float ly = r.bottom() - 6.0f;
        const auto lampOf = [&](const char* word, bool on, funkgui::Col ink) {
            if (lx + 15.0f + c.textWidth(word, look::kCaps) <= r.right())
                lx += look::lamp(c, p, lx, ly, word, on, ink) + 14.0f;
        };
        lampOf("SHIFT", v.shift, p.hotter);
        lampOf("LIMITER", v.limiter, p.warning);
        lampOf("KNOCK", ch[7].alarm != nullptr, p.warning);
        lampOf("LEAN", ch[1].alarm != nullptr, p.warning);
        lampOf("OIL", ch[2].alarm != nullptr || ch[3].alarm != nullptr, p.hotter);
        lampOf("WATER", ch[4].alarm != nullptr, p.hotter);
        lampOf("EXHAUST", ch[5].alarm != nullptr, p.hotter);
    }
}
