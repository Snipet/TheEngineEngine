// ui/kit/bind/Units.cpp: see Units.h. The factors are the exact or the standard ones (NIST SP 811).
#include "kit/bind/Units.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace tee::ui
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;

        constexpr Unit kNone[] = { { "" } };
        constexpr Unit kPressure[] = { { "kPa", 1e-3, 0, 1 }, { "bar", 1e-5, 0, 3 }, { "psi", 1.0 / 6894.757293168, 0, 2 },
                                       { "Pa", 1, 0, 0 } };
        constexpr Unit kTemperature[] = { { "C", 1, -273.15, 1 }, { "F", 1.8, -459.67, 1 }, { "K", 1, 0, 1 } };
        constexpr Unit kLength[] = { { "mm", 1e3, 0, 1 }, { "cm", 1e2, 0, 2 }, { "in", 1.0 / 0.0254, 0, 3 }, { "m", 1, 0, 3 } };
        constexpr Unit kArea[] = { { "cm2", 1e4, 0, 2 }, { "mm2", 1e6, 0, 1 }, { "in2", 1.0 / (0.0254 * 0.0254), 0, 3 },
                                   { "m2", 1, 0, 4 } };
        constexpr Unit kVolume[] = { { "L", 1e3, 0, 2 }, { "cc", 1e6, 0, 0 }, { "ci", 1.0 / (0.0254 * 0.0254 * 0.0254), 0, 1 },
                                     { "m3", 1, 0, 5 } };
        constexpr Unit kMass[] = { { "kg", 1, 0, 2 }, { "g", 1e3, 0, 0 }, { "lb", 1.0 / 0.45359237, 0, 2 } };
        constexpr Unit kInertia[] = { { "kg.m2", 1, 0, 3 }, { "lb.ft2", 1.0 / (0.45359237 * 0.3048 * 0.3048), 0, 2 } };
        constexpr Unit kSpeed[] = { { "km/h", 3.6, 0, 0 }, { "mph", 1.0 / 0.44704, 0, 0 }, { "m/s", 1, 0, 1 } };
        constexpr Unit kRotation[] = { { "rpm", 60.0 / (2.0 * kPi), 0, 0 }, { "rad/s", 1, 0, 1 } };
        constexpr Unit kTorque[] = { { "Nm", 1, 0, 0 }, { "lb.ft", 1.0 / 1.3558179483314004, 0, 0 } };
        constexpr Unit kPower[] = { { "kW", 1e-3, 0, 0 }, { "hp", 1.0 / 745.69987158227022, 0, 0 },
                                    { "PS", 1.0 / 735.49875, 0, 0 }, { "W", 1, 0, 0 } };
        constexpr Unit kAngle[] = { { "deg", 1, 0, 1 } };
        constexpr Unit kTime[] = { { "ms", 1e3, 0, 1 }, { "s", 1, 0, 3 } };
        constexpr Unit kFrequency[] = { { "Hz", 1, 0, 1 } };
        constexpr Unit kFlow[] = { { "g/s", 1e3, 0, 1 }, { "kg/s", 1, 0, 3 }, { "lb/min", 60.0 / 0.45359237, 0, 1 } };
        constexpr Unit kRatio[] = { { "", 1, 0, 2 } };
        constexpr Unit kFraction[] = { { "%", 100, 0, 1 }, { "", 1, 0, 3 } };

        struct Entry
        {
            const char* name;
            std::span<const Unit> units;
        };
        constexpr Entry kTable[kQuantities] = {
            { "", kNone },           { "pressure", kPressure }, { "temperature", kTemperature }, { "length", kLength },
            { "area", kArea },       { "volume", kVolume },     { "mass", kMass },               { "inertia", kInertia },
            { "speed", kSpeed },     { "rotation", kRotation }, { "torque", kTorque },           { "power", kPower },
            { "angle", kAngle },     { "time", kTime },         { "frequency", kFrequency },     { "flow", kFlow },
            { "ratio", kRatio },     { "fraction", kFraction },
        };

        std::string_view trim(std::string_view s)
        {
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
                s.remove_prefix(1);
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
                s.remove_suffix(1);
            return s;
        }

        bool sameName(std::string_view a, std::string_view b)
        {
            return a.size() == b.size()
                   && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
                          return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
                      });
        }

        int unitIndex(Quantity q, std::string_view name)
        {
            const auto units = unitsOf(q);
            for (std::size_t i = 0; i < units.size(); ++i)         // exact first: "m" is not "M"
                if (name == units[i].name)
                    return static_cast<int>(i);
            for (std::size_t i = 0; i < units.size(); ++i)
                if (sameName(name, units[i].name))
                    return static_cast<int>(i);
            return -1;
        }
    }

    Quantity quantityOf(std::string_view name) noexcept
    {
        for (int i = 1; i < kQuantities; ++i)
            if (name == kTable[i].name)
                return static_cast<Quantity>(i);
        return Quantity::none;
    }

    const char* nameOf(Quantity q) noexcept { return kTable[static_cast<std::size_t>(q)].name; }
    std::span<const Unit> unitsOf(Quantity q) noexcept { return kTable[static_cast<std::size_t>(q)].units; }

    const Unit& Units::unit(Quantity q) const noexcept { return unitsOf(q)[static_cast<std::size_t>(chosen(q))]; }

    namespace
    {
        // Which system a unit's name is of: metric (SI prefixes on one base) or not; the preference crosses no system.
        bool metric(std::string_view name) noexcept
        {
            return name == "mm" || name == "cm" || name == "m" || name == "L" || name == "cm2" || name == "cm3"
                || name == "m2" || name == "m3" || name == "kPa" || name == "bar" || name == "Pa";
        }
    }

    int Units::chosen(Quantity q, std::string_view prefer) const noexcept
    {
        const int now = chosen(q);
        if (prefer.empty())
            return now;
        const int want = unitIndex(q, prefer);
        if (want < 0 || metric(unitsOf(q)[static_cast<std::size_t>(now)].name) != metric(prefer))
            return now;
        return want;
    }

    const Unit& Units::unit(Quantity q, std::string_view prefer) const noexcept
    {
        return unitsOf(q)[static_cast<std::size_t>(chosen(q, prefer))];
    }

    bool Units::choose(Quantity q, std::string_view unitName) noexcept
    {
        const int i = unitIndex(q, unitName);
        if (i < 0)
            return false;
        choose(q, i);
        return true;
    }

    void Units::choose(Quantity q, int index) noexcept
    {
        const int n = static_cast<int>(unitsOf(q).size());
        index = std::clamp(index, 0, n - 1);
        if (chosen_[static_cast<std::size_t>(q)] != index)
        {
            chosen_[static_cast<std::size_t>(q)] = index;
            ++revision_;
        }
    }

    double Units::toUser(Quantity q, double si, std::string_view prefer) const noexcept
    {
        const Unit& u = unit(q, prefer);
        return si * u.scale + u.offset;
    }

    double Units::toSi(Quantity q, double user, std::string_view prefer) const noexcept
    {
        const Unit& u = unit(q, prefer);
        return (user - u.offset) / u.scale;
    }

    std::string formatNumber(double v, int decimals)
    {
        if (!std::isfinite(v))
            return "-";
        const double a = std::fabs(v);
        const int d = decimals >= 0 ? std::min(decimals, 6) : (a >= 100.0 ? 0 : (a >= 10.0 ? 1 : 2));
        char buf[64];
        std::snprintf(buf, sizeof buf, "%.*f", d, v);
        std::string s = buf;
        if (s.find_first_not_of("-0.") == std::string::npos && s.front() == '-')
            s.erase(0, 1);                                       // "-0.0" is "0.0"
        return s;
    }

    std::string Units::number(Quantity q, double si, int decimals, std::string_view prefer) const
    {
        return formatNumber(toUser(q, si, prefer), decimals >= 0 ? decimals : unit(q, prefer).decimals);
    }

    std::string Units::format(Quantity q, double si, int decimals, std::string_view prefer) const
    {
        std::string s = number(q, si, decimals, prefer);
        const char* name = unit(q, prefer).name;
        if (name[0] != '\0')
            s += name[0] == '%' ? std::string(name) : std::string(" ") + name;
        return s;
    }

    bool Units::parse(Quantity q, std::string_view text, double& si, std::string_view prefer) const
    {
        text = trim(text);
        const std::string s(text);
        char* end = nullptr;
        const double v = std::strtod(s.c_str(), &end);
        if (end == s.c_str() || !std::isfinite(v))
            return false;
        const std::string_view rest = trim(std::string_view(end));
        int u = chosen(q, prefer);
        if (!rest.empty())
        {
            u = unitIndex(q, rest);
            if (u < 0)
                return false;
        }
        const Unit& unit = unitsOf(q)[static_cast<std::size_t>(u)];
        si = (v - unit.offset) / unit.scale;
        return true;
    }

    std::string Units::save() const
    {
        std::string out;
        for (int i = 1; i < kQuantities; ++i)
            if (chosen_[static_cast<std::size_t>(i)] != 0)
                out += std::string(kTable[i].name) + "=" + unit(static_cast<Quantity>(i)).name + "\n";
        return out;
    }

    void Units::load(std::string_view text, std::string& notes)
    {
        while (!text.empty())
        {
            const std::size_t nl = text.find('\n');
            const std::string_view line = trim(text.substr(0, nl));
            text = nl == std::string_view::npos ? std::string_view() : text.substr(nl + 1);
            if (line.empty() || line.front() == '#')
                continue;
            const std::size_t eq = line.find('=');
            const Quantity q = eq == std::string_view::npos ? Quantity::none : quantityOf(trim(line.substr(0, eq)));
            if (q == Quantity::none || !choose(q, trim(line.substr(eq + 1))))
                notes += "units: note: '" + std::string(line) + "' names no quantity and unit of ours: left out\n";
        }
    }
}
