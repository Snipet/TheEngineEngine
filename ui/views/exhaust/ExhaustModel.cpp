// ui/views/exhaust/ExhaustModel.cpp: see ExhaustModel.h. The tables are sim/core/layouts.fm's; the rules are
// sim/exhaust/slots.fm's resolve() and shape() (the primaries' lengths), kept in the same order so a reader can hold
// the two side by side.
#include "ExhaustModel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace tee::ui::exhaust
{
    namespace
    {
        constexpr int kLayouts = 17;
        const char* const kLayoutNames[kLayouts] = {
            "Single", "Twin 360", "Twin 270", "V-twin 45", "Inline 3", "Inline 4", "Flat 4", "Crossplane I4", "Inline 5",
            "V6", "Inline 6", "Flat 6", "V8 crossplane", "V8 flat-plane", "V10", "V12", "Rotary",
        };
        // sim/core/layouts.fm: cylinders, each one's bank (12 a layout), the exhaust's topology, the header length (m),
        // the main pipe (m), the muffler's first chamber (m), a motorcycle's.
        const int kCyl[kLayouts] = { 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 8, 8, 10, 12, 6 };
        const int kBank[kLayouts][12] = {
            {}, {}, {}, {}, {}, {}, { 0, 1, 0, 1 }, {}, {}, { 0, 1, 0, 1, 0, 1 }, { 0, 0, 0, 1, 1, 1 },
            { 0, 0, 0, 1, 1, 1 }, { 0, 1, 0, 1, 0, 1, 0, 1 }, { 0, 0, 0, 0, 1, 1, 1, 1 }, { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 },
            { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 }, {},
        };
        const float kFire[kLayouts][12] = {
            {}, { 0, 360 }, { 0, 270 }, { 0, 315 }, { 0, 240, 480 }, { 0, 540, 180, 360 }, { 0, 360, 180, 540 },
            { 0, 450, 270, 540 }, { 0, 144, 576, 288, 432 }, { 0, 120, 240, 360, 480, 600 }, { 0, 480, 240, 600, 120, 360 },
            { 0, 240, 480, 360, 600, 120 }, { 0, 630, 270, 180, 450, 360, 540, 90 }, { 0, 540, 180, 360, 90, 630, 270, 450 },
            { 0, 288, 432, 576, 144, 72, 360, 504, 648, 216 }, { 0, 480, 240, 600, 120, 360, 60, 540, 300, 660, 180, 420 },
            { 0, 360, 720, 180, 540, 900 },
        };
        const int kTopo[kLayouts] = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 1, 2, 2, 2, 2, 2, 0 };
        const float kHdr[kLayouts] = { 0.60f, 0.55f, 0.55f, 0.95f, 0.55f, 0.60f, 0.45f, 0.50f, 0.65f, 0.55f, 0.55f, 0.60f,
                                       0.75f, 0.70f, 0.65f, 0.65f, 0.45f };
        const float kMain[kLayouts] = { 0.45f, 0.45f, 0.45f, 0.35f, 1.80f, 2.20f, 2.10f, 0.40f, 2.30f, 2.40f, 2.40f, 1.60f,
                                        2.60f, 2.20f, 2.30f, 2.40f, 2.10f };
        const float kMuff[kLayouts] = { 0.22f, 0.25f, 0.25f, 0.30f, 0.35f, 0.42f, 0.42f, 0.25f, 0.45f, 0.45f, 0.45f, 0.40f,
                                        0.50f, 0.45f, 0.45f, 0.45f, 0.45f };
        const bool kBike[kLayouts] = { true, true, true, true, false, false, false, true, false, false, false, false,
                                       false, false, false, false, false };

        bool contains(std::string_view hay, std::string_view needle)
        {
            if (needle.size() > hay.size())
                return false;
            for (std::size_t i = 0; i + needle.size() <= hay.size(); ++i)
            {
                bool same = true;
                for (std::size_t k = 0; k < needle.size() && same; ++k)
                {
                    const char a = hay[i + k], b = needle[k];
                    same = (a >= 'A' && a <= 'Z' ? a + 32 : a) == (b >= 'A' && b <= 'Z' ? b + 32 : b);
                }
                if (same)
                    return true;
            }
            return false;
        }

        // A choice field's label for its value ("" when the field is not a choice).
        std::string choiceLabel(const FieldDoc& doc, std::string_view path)
        {
            const int f = doc.find(path);
            if (f < 0)
                return {};
            const FieldInfo& info = doc.info(f);
            const int k = static_cast<int>(std::lround(doc.get(f) - info.min));
            if (info.kind != FieldKind::choice || k < 0 || k >= static_cast<int>(info.choices.size()))
                return {};
            return info.choices[static_cast<std::size_t>(k)];
        }

        int clampi(double v, int lo, int hi) { return std::clamp(static_cast<int>(std::lround(v)), lo, hi); }
    }

    const char* tagName(Tag t) noexcept
    {
        switch (t)
        {
            case Tag::rearCentre: return "REAR CENTRE";
            case Tag::rearLeft: return "REAR LEFT";
            case Tag::rearRight: return "REAR RIGHT";
            case Tag::sideLeft: return "SIDE LEFT";
            case Tag::sideRight: return "SIDE RIGHT";
            case Tag::front: return "FRONT";
            case Tag::up: return "UP";
        }
        return "-";
    }

    const char* layoutName(int layout) noexcept
    {
        return layout >= 0 && layout < kLayouts ? kLayoutNames[layout] : "-";
    }

    int layoutFromLabel(std::string_view label) noexcept
    {
        for (int k = 0; k < kLayouts; ++k)
            if (label == kLayoutNames[k])
                return k;
        // the demo's and other short names
        struct Alias
        {
            const char* name;
            int layout;
        };
        static const Alias kAliases[] = {
            { "I3", 4 }, { "I4", 5 }, { "I5", 8 }, { "I6", 10 }, { "Flat 4", 6 }, { "Flat 6", 11 }, { "Boxer 4", 6 },
            { "V6", 9 }, { "V8 flat", 13 }, { "V8", 12 }, { "V10", 14 }, { "V12", 15 }, { "Single", 0 },
        };
        for (const Alias& a : kAliases)
            if (label == a.name)
                return a.layout;
        return -1;
    }

    double fieldOr(const FieldDoc& doc, std::string_view path, int i, double fallback)
    {
        int f = -1;
        if (i >= 0)
        {
            const std::string el = std::string(path) + "[" + std::to_string(i) + "]";
            f = doc.find(el);
        }
        else
            f = doc.find(path);
        return f < 0 ? fallback : doc.get(f);
    }

    bool ExhaustBuild::openStacks() const noexcept
    {
        for (int i = 0; i < ports; ++i)
            if (portTo[static_cast<std::size_t>(i)] >= 0)
                return false;
        return ports > 0;
    }

    bool ExhaustBuild::anyMerge() const noexcept
    {
        for (int c = 0; c < kCollectors; ++c)
            if (colOn[static_cast<std::size_t>(c)] && colFed[static_cast<std::size_t>(c)])
                return true;
        return false;
    }

    std::string ExhaustBuild::sentence() const
    {
        std::string s = layoutName(layout);
        int first = 0, merges = 0;
        bool log = false;
        for (int c = 0; c < kCollectors; ++c)
        {
            const auto k = static_cast<std::size_t>(c);
            if (!colOn[k])
                continue;
            (colFed[k] ? merges : first) += 1;
            log = log || colVol[k] > 0.0f;
        }
        int stacks = 0;
        for (int i = 0; i < ports; ++i)
            stacks += portTo[static_cast<std::size_t>(i)] < 0 ? 1 : 0;
        char b[64];
        if (stacks == ports)
            s += ports == 1 ? ", an open pipe" : ", open stacks";
        else
        {
            if (log)
                s += ", log manifold";
            else
            {
                std::snprintf(b, sizeof b, ", %d into %d", ports - stacks, first);
                s += b;
                if (merges > 0)
                {
                    std::snprintf(b, sizeof b, " into %d", merges);
                    s += b;
                }
            }
            if (turbos != Turbos::none)
                s += turbos == Turbos::one ? ", a turbine" : ", turbines";
            if (sides == 2)
                s += cross == 2 ? ", dual with an X" : cross == 1 ? ", dual with an H" : ", dual";
            else
                s += ", single pipe";
            static const char* const kMuf[] = { "no muffler", "straight-through", "chambered", "three-pass" };
            s += ", ";
            s += kMuf[std::clamp(muf[0], 0, 3)];
        }
        return s;
    }

    ExhaustBuild resolveExhaust(const FieldDoc& doc)
    {
        using P = ExhaustPaths;
        ExhaustBuild b;

        // the layout: the real Spec's index, or the demo's by its label
        int layout = 12;
        if (const int f = doc.find(P::layout); f >= 0)
        {
            const std::string label = choiceLabel(doc, P::layout);
            const int byLabel = label.empty() ? -1 : layoutFromLabel(label);
            layout = byLabel >= 0 ? byLabel : clampi(doc.get(f), 0, kLayouts - 1);
        }
        b.layout = layout;
        const bool rotary = layout == 16;
        b.ports = rotary ? 2 : kCyl[layout];
        for (int i = 0; i < kPorts; ++i)
        {
            b.bank[static_cast<std::size_t>(i)] = i < 12 && !rotary ? kBank[layout][i] : 0;
            b.fire[static_cast<std::size_t>(i)] = i < 12 ? kFire[layout][i] : 0.0f;
        }
        if (rotary)   // a rotor's port: its first face's
            b.fire = { 0.0f, 180.0f };
        b.cycle = rotary ? 1080.0f : 720.0f;
        const int topo = kTopo[layout];
        const bool bike = kBike[layout];

        // the aspiration: by label where it is a choice (the demo's and the real names differ), else the index
        {
            const std::string kind = choiceLabel(doc, P::boostKind);
            int k = 0;
            if (!kind.empty())
                k = contains(kind, "super") ? 4 : contains(kind, "quad") ? 3 : contains(kind, "twin") ? 2
                    : contains(kind, "turbo") ? 1 : 0;
            else
                k = clampi(fieldOr(doc, P::boostKind, -1, 0.0), 0, 4);
            b.supercharger = k == 4;
            const int arr = clampi(fieldOr(doc, P::boostLayout, -1, 0.0), 0, 5);
            if (k >= 1 && k <= 3)
                b.turbos = arr == 1 ? Turbos::one : arr == 2 ? Turbos::perBank : arr == 3 ? Turbos::sequential
                         : arr == 4 ? Turbos::compound : arr == 5 ? Turbos::fourPerBank
                         : k == 1 ? Turbos::one : k == 2 ? Turbos::perBank : Turbos::fourPerBank;
        }

        // the dials (numbers only: the demo's "exhaust.headers" is a choice of its own)
        const auto dial = [&](const char* path) {
            const int f = doc.find(path);
            if (f < 0 || doc.info(f).kind != FieldKind::number || doc.info(f).max > 1.0)
                return 0.5;
            return std::clamp(doc.get(f), 0.0, 1.0);
        };
        const double hk = std::exp(0.9808293 * (dial(P::headersDial) - 0.5));
        const double pk = std::exp(1.3862944 * (dial(P::pipeDial) - 0.5));

        // resolve(): the sides, each port's collector, the collectors' pipes
        const int sidesField = clampi(fieldOr(doc, P::sides, -1, 0.0), 0, 2);
        b.sides = sidesField > 0 ? sidesField : (topo == 2 ? 2 : 1);
        const double spread = fieldOr(doc, P::primSpread, -1, 0.06);
        for (int i = 0; i < kPorts; ++i)
        {
            const auto k = static_cast<std::size_t>(i);
            int to = static_cast<int>(std::lround(fieldOr(doc, P::primaryTo, i, -2.0)));
            if (to == -2 || to < -2 || to >= kCollectors)
                to = topo == 0 ? 0 : b.bank[k];
            b.portTo[k] = i < b.ports ? to : -1;
            if (i < b.ports && to >= 0)
                b.colOn[static_cast<std::size_t>(to)] = true;
            // shape(): each primary a little different (the reference's pattern), the second bank's own factor
            const double own = 1.0 + spread * static_cast<double>((i * 7) % 5 - 2) / 2.0;
            const double set = fieldOr(doc, P::primLen, i, 0.0);
            const double bank1 = layout == 6 && b.bank[k] == 1 ? 1.75 : 1.0;
            b.primLen[k] = static_cast<float>(set > 0.0 ? set * own : kHdr[layout] * hk * own * bank1);
        }
        for (int c = 0; c < kCollectors; ++c)
        {
            int to = static_cast<int>(std::lround(fieldOr(doc, P::collectorTo, c, -1.0)));
            if (to < 0 || to > kCollectors + 1)
                to = b.sides == 2 ? (c & 1) : 0;
            if (to >= 2 && to - 2 == c)
                to = 0;
            b.colTo[static_cast<std::size_t>(c)] = to;
            const double len = fieldOr(doc, P::colLen, c, 0.0);
            b.colLen[static_cast<std::size_t>(c)] = static_cast<float>(len > 0.0 ? len : (bike ? 0.25 : 0.38));
            b.colVol[static_cast<std::size_t>(c)] = static_cast<float>(fieldOr(doc, P::colVol, c, 0.0));
        }
        for (int c = 0; c < kCollectors; ++c)
        {
            const int to = b.colTo[static_cast<std::size_t>(c)];
            if (to >= 2 && b.colTo[static_cast<std::size_t>(to - 2)] >= 2)
                b.colTo[static_cast<std::size_t>(to - 2)] = 0;
        }
        for (int c = 0; c < kCollectors; ++c)
        {
            const auto k = static_cast<std::size_t>(c);
            if (b.colOn[k] && b.colTo[k] >= 2)
            {
                b.colOn[static_cast<std::size_t>(b.colTo[k] - 2)] = true;
                b.colFed[static_cast<std::size_t>(b.colTo[k] - 2)] = true;
            }
        }
        for (int c = 0; c < kCollectors; ++c)
        {
            const auto k = static_cast<std::size_t>(c);
            if (b.colOn[k] && b.colTo[k] < 2)
            {
                if (b.colTo[k] >= b.sides)
                    b.colTo[k] = b.sides - 1;
                b.sideOn[static_cast<std::size_t>(b.colTo[k])] = true;
            }
        }

        // the crossover: a choice of three, or the demo's switch (an X), or the reference's dial (an H when open)
        {
            const int fk = doc.find(P::crossKind), f = doc.find(P::crossover);
            int cross = 1;
            if (fk >= 0)
                cross = clampi(doc.get(fk), 0, 2);
            else if (f >= 0)
            {
                const FieldInfo& info = doc.info(f);
                const double v = doc.get(f);
                if (info.kind == FieldKind::toggle)
                    cross = v > 0.5 ? 2 : 0;
                else if (info.kind == FieldKind::number)
                    cross = v > 0.01 ? 1 : 0;
                else
                    cross = clampi(v, 0, 2);
            }
            b.cross = b.sides == 2 ? cross : 0;
        }
        if (b.cross != 0 && (b.sideOn[0] || b.sideOn[1]))
            b.sideOn = { true, true };
        if (b.cross != 0 && !b.sideOn[0])
            b.cross = 0;
        b.crossAt = fieldOr(doc, P::crossAt, -1, 0.0) > 0.5 ? 1 : 0;

        for (int s = 0; s < kSides; ++s)
        {
            const auto k = static_cast<std::size_t>(s);
            b.conv[k] = clampi(fieldOr(doc, P::converter, s, 1.0), 0, 2);
            b.res[k] = clampi(fieldOr(doc, P::resonator, s, 0.0), 0, 3);
            b.muf[k] = clampi(fieldOr(doc, P::mufKind, s, fieldOr(doc, P::muffler, s, 2.0)), 0, 3);
            b.byp[k] = b.muf[k] != 0 ? clampi(fieldOr(doc, P::bypass, s, 0.0), 0, 4) : 0;
            b.tips[k] = fieldOr(doc, P::tips, s, 1.0) >= 1.5 ? 2 : 1;
            const int ea = static_cast<int>(std::lround(fieldOr(doc, P::exitAt, s, -1.0)));
            b.tag[k] = ea < 0 || ea > 6 ? (b.sides == 2 ? (s == 0 ? Tag::rearLeft : Tag::rearRight) : Tag::rearCentre)
                                        : static_cast<Tag>(ea);
            const double ml = fieldOr(doc, P::mainLen, s, 0.0), mu = fieldOr(doc, P::mufLen, s, 0.0);
            const double tl = fieldOr(doc, P::tailLen, s, 0.0);
            b.mainLen[k] = static_cast<float>(ml > 0.0 ? ml : kMain[layout] * pk);
            b.mufLen[k] = static_cast<float>(mu > 0.0 ? mu : kMuff[layout]);
            b.tailLen[k] = static_cast<float>(tl > 0.0 ? tl : (bike ? 0.10 : 0.40));
        }
        return b;
    }
}
