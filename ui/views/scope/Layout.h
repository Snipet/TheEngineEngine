// ui/views/scope/Layout.h: what an engine's layout says, from the Spec's words for it (engine.layout's choices,
// sim/spec/engine.fm): how many cylinders (or rotors), whether it is a rotary, its cylinders' arrangement in words.
// SCOPE numbers the firing orders with it; GARAGE says the engine in a sentence with it. Plain C++.
#pragma once

#include "kit/bind/Fields.h"

#include <cctype>
#include <cmath>
#include <string>
#include <string_view>

namespace tee::ui::layout
{
    inline bool hasWord(std::string_view s, std::string_view w) noexcept
    {
        if (w.empty() || s.size() < w.size())
            return false;
        for (std::size_t i = 0; i + w.size() <= s.size(); ++i)
        {
            bool same = true;
            for (std::size_t k = 0; k < w.size() && same; ++k)
                same = std::tolower(static_cast<unsigned char>(s[i + k])) == std::tolower(static_cast<unsigned char>(w[k]));
            if (same)
                return true;
        }
        return false;
    }

    // Cylinders (a rotary's rotors) from a layout's name: "V8 crossplane" 8, "Inline 4" 4, "Single" 1, "V-twin 45" 2,
    // "Rotary" 2. 0: no number in it.
    inline int cylindersOf(std::string_view name) noexcept
    {
        if (hasWord(name, "single"))
            return 1;
        if (hasWord(name, "twin"))
            return 2;
        if (hasWord(name, "rotary"))
            return 2;
        for (std::size_t i = 0; i < name.size(); ++i)
            if (std::isdigit(static_cast<unsigned char>(name[i])))
            {
                int v = 0;
                while (i < name.size() && std::isdigit(static_cast<unsigned char>(name[i])))
                    v = v * 10 + (name[i++] - '0');
                // "V-twin 45" is caught above; a number after the cylinders' (an angle) is not the count.
                return v;
            }
        return 0;
    }

    inline bool isRotary(std::string_view name) noexcept { return hasWord(name, "rotary"); }

    // The layout's choice name in a document, "" when it has none.
    inline std::string layoutName(const FieldDoc& d)
    {
        const int f = d.find("engine.layout");
        if (f < 0)
            return {};
        const FieldInfo& fi = d.info(f);
        const long i = std::lround(d.get(f) - fi.min);
        return i >= 0 && i < static_cast<long>(fi.choices.size()) ? fi.choices[static_cast<std::size_t>(i)] : std::string();
    }

    // Cylinders in a document: engine.cylinders when it has one, else from its layout; 0 when neither says.
    inline int cylinders(const FieldDoc& d)
    {
        const int f = d.find("engine.cylinders");
        if (f >= 0)
            return static_cast<int>(std::lround(d.get(f)));
        return cylindersOf(layoutName(d));
    }

    // Firing events per crank revolution: a four-stroke's cylinders over two (a cycle of 720 degrees), a two-stroke's
    // cylinders (360), a rotary's rotors (each fires once a turn of its eccentric shaft). 0 when unknown.
    inline double firingPerRev(int cylinders, int cycleDegrees, bool rotary) noexcept
    {
        if (cylinders <= 0)
            return 0.0;
        if (rotary)
            return cylinders;
        const int deg = cycleDegrees == 360 ? 360 : 720;
        return cylinders * 360.0 / deg;
    }
}
