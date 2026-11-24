// ui/views/engine/EngineModel.cpp: see EngineModel.h.
#include "views/engine/EngineModel.h"

#include <algorithm>
#include <cstdio>

namespace tee::ui::engine
{
    namespace
    {
        std::string expand(const char* path, int index)
        {
            if (path == nullptr || *path == 0)
                return {};
            std::string p(path);
            const auto at = p.find("%d");
            if (at != std::string::npos)
                p.replace(at, 2, std::to_string(index < 0 ? 0 : index));
            return p;
        }

        // ---- the reference's seventeen (sim/core/layouts.fm) ----
        const char* const kNames[kPresetCount] = {
            "Single", "Twin 360", "Twin 270", "V-twin 45", "Inline 3", "Inline 4", "Flat 4", "Crossplane I4",
            "Inline 5", "V6", "Inline 6", "Flat 6", "V8 crossplane", "V8 flat-plane", "V10", "V12", "Rotary",
        };
        const int kN[kPresetCount] = { 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 8, 8, 10, 12, 6 };
        // the firing angle of each cylinder after cylinder 1's (layouts.fm's table, as it is)
        const double kFire[kPresetCount][12] = {
            { 0 },
            { 0, 360 },
            { 0, 270 },
            { 0, 315 },
            { 0, 240, 480 },
            { 0, 540, 180, 360 },
            { 0, 360, 180, 540 },
            { 0, 450, 270, 540 },
            { 0, 144, 576, 288, 432 },
            { 0, 120, 240, 360, 480, 600 },
            { 0, 480, 240, 600, 120, 360 },
            { 0, 240, 480, 360, 600, 120 },
            { 0, 630, 270, 180, 450, 360, 540, 90 },
            { 0, 540, 180, 360, 90, 630, 270, 450 },
            { 0, 288, 432, 576, 144, 72, 360, 504, 648, 216 },
            { 0, 480, 240, 600, 120, 360, 60, 540, 300, 660, 180, 420 },
            { 0, 360, 720, 180, 540, 900 },
        };
        // the bank each cylinder stands in (geometry, not layouts.fm's exhaust banks: an inline six is one bank)
        const int kBank[kPresetCount][12] = {
            { 0 },
            { 0, 0 },
            { 0, 0 },
            { 0, 1 },
            { 0, 0, 0 },
            { 0, 0, 0, 0 },
            { 0, 1, 0, 1 },
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0 },
            { 0, 1, 0, 1, 0, 1 },
            { 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 1, 1, 1 },
            { 0, 1, 0, 1, 0, 1, 0, 1 },
            { 0, 0, 0, 0, 1, 1, 1, 1 },
            { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 },
            { 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
            { 0, 0, 0, 1, 1, 1 },
        };
        // half the angle between the banks (the usual for each: a 45 degree V-twin, a 60 degree V6, a 90 degree V8 and
        // V10, a 60 degree V12, a flat engine's 180)
        const double kHalfVee[kPresetCount] = { 0, 0, 0, 22.5, 0, 0, 90, 0, 0, 30, 0, 90, 45, 45, 45, 30, 0 };

        // pins from the firing angles for the banks at +-half (cylinder 1's pin at 0); how many throws are shared
        int pinsFor(Layout& l, double half)
        {
            l.bankAngle = {};
            if (l.banks > 1)
            {
                l.bankAngle[0] = half;
                l.bankAngle[1] = -half;
            }
            const double t0 = l.bankAngle[static_cast<std::size_t>(l.bank[0])];
            for (int c = 0; c < l.n; ++c)
            {
                const auto i = static_cast<std::size_t>(c);
                l.pin[i] = wrap(l.bankAngle[static_cast<std::size_t>(l.bank[i])] - l.fire[i] - t0, 360.0);
            }
            int shared = 0;
            for (int a = 0; a < l.n; ++a)
                for (int b = a + 1; b < l.n; ++b)
                {
                    const auto ia = static_cast<std::size_t>(a), ib = static_cast<std::size_t>(b);
                    const double d = std::fabs(wrap(l.pin[ia] - l.pin[ib] + 180.0, 360.0) - 180.0);
                    if (l.bank[ia] != l.bank[ib] && std::fabs(l.along[ia] - l.along[ib]) < 0.6 && d < 0.5)
                        ++shared;
                }
            return shared;
        }

        // along the crank: each cylinder's rank in its bank, the second bank half a pitch behind
        void placeByRank(Layout& l)
        {
            std::array<int, kMaxBanks> seen{};
            for (int c = 0; c < l.n; ++c)
            {
                const auto i = static_cast<std::size_t>(c);
                const int b = std::clamp(l.bank[i], 0, kMaxBanks - 1);
                l.along[i] = seen[static_cast<std::size_t>(b)]++ + (l.banks > 1 ? 0.5 * b : 0.0);
            }
        }

        void orderAndIntervals(Layout& l)
        {
            for (int c = 0; c < l.n; ++c)
                l.order[static_cast<std::size_t>(c)] = c;
            std::stable_sort(l.order.begin(), l.order.begin() + l.n, [&](int a, int b) {
                return l.fire[static_cast<std::size_t>(a)] < l.fire[static_cast<std::size_t>(b)];
            });
            int same = 0;
            for (int i = 0; i < l.n; ++i)
            {
                const double a = l.fire[static_cast<std::size_t>(l.order[static_cast<std::size_t>(i)])];
                const double b = i + 1 < l.n ? l.fire[static_cast<std::size_t>(l.order[static_cast<std::size_t>(i + 1)])] : l.cyc;
                l.interval[static_cast<std::size_t>(i)] = b - a;
                if (i + 1 < l.n && b - a < 0.5)
                    ++same;
            }
            if (same > 0)
                l.ok = false;
        }

        std::string lower(std::string_view s)
        {
            std::string o(s);
            for (char& ch : o)
                if (ch >= 'A' && ch <= 'Z')
                    ch = static_cast<char>(ch - 'A' + 'a');
            return o;
        }
    }

    int findField(const FieldDoc& doc, const FieldPath& f, int index)
    {
        for (const char* p : { f.path, f.alt })
        {
            const std::string s = expand(p, index);
            if (s.empty())
                continue;
            const int i = doc.find(s);
            if (i >= 0)
                return i;
        }
        return -1;
    }

    double fieldValue(const FieldDoc& doc, const FieldPath& f, int index)
    {
        const int i = findField(doc, f, index);
        return i < 0 ? kNaN : doc.get(i);
    }

    std::string fieldPath(const FieldDoc& doc, const FieldPath& f, int index)
    {
        const int i = findField(doc, f, index);
        return i < 0 ? std::string() : doc.info(i).path;
    }

    // ---- the layout ----------------------------------------------------------------------------------------------------
    std::string Layout::orderText() const
    {
        std::string s;
        for (int i = 0; i < n; ++i)
        {
            if (i > 0)
                s += '-';
            s += std::to_string(order[static_cast<std::size_t>(i)] + 1);
        }
        return s;
    }

    bool Layout::even() const
    {
        for (int i = 1; i < n; ++i)
            if (std::fabs(interval[static_cast<std::size_t>(i)] - interval[0]) > 0.5)
                return false;
        return n > 0;
    }

    std::string Layout::intervalText() const
    {
        if (n <= 0)
            return "-";
        char b[24];
        if (n > 1 && interval[0] < 0.5)
        {
            std::snprintf(b, sizeof b, "together, %.0f", cyc);
            return b;
        }
        if (even())
        {
            std::snprintf(b, sizeof b, "even, %.0f", interval[0]);
            return b;
        }
        std::string s;
        for (int i = 0; i < n; ++i)
        {
            std::snprintf(b, sizeof b, i == 0 ? "%.0f" : "-%.0f", interval[static_cast<std::size_t>(i)]);
            s += b;
        }
        return s;
    }

    const char* presetName(int i) { return i >= 0 && i < kPresetCount ? kNames[i] : ""; }

    Layout presetLayout(int k)
    {
        Layout l;
        k = std::clamp(k, 0, kPresetCount - 1);
        l.name = kNames[k];
        l.n = kN[k];
        l.rotors = k == 16 ? 2 : 0;
        l.cyc = k == 16 ? 1080.0 : 720.0;
        l.banks = 1;
        for (int c = 0; c < l.n; ++c)
        {
            const auto i = static_cast<std::size_t>(c);
            l.fire[i] = kFire[k][c];
            l.bank[i] = kBank[k][c];
            l.banks = std::max(l.banks, l.bank[i] + 1);
        }
        if (l.rotors > 0)
        {
            // a rotor's three faces stand together; rotors one behind the other
            for (int c = 0; c < l.n; ++c)
                l.along[static_cast<std::size_t>(c)] = static_cast<double>(c / 3);
            l.banks = 1;
            for (int r = 0; r < l.rotors; ++r)
                l.pin[static_cast<std::size_t>(r)] = r == 0 ? 0.0 : 180.0;
        }
        else
        {
            placeByRank(l);
            // the banks' sides as they share the most throws (crossplane and flat-plane V8s number their banks the
            // opposite ways)
            const int a = pinsFor(l, kHalfVee[k]);
            Layout m = l;
            if (pinsFor(m, -kHalfVee[k]) > a)
                l = m;
            else
                pinsFor(l, kHalfVee[k]);
        }
        orderAndIntervals(l);
        return l;
    }

    int presetByName(std::string_view name)
    {
        const std::string s = lower(name);
        for (int i = 0; i < kPresetCount; ++i)
            if (s == lower(kNames[i]))
                return i;
        struct Alias
        {
            const char* name;
            int index;
        };
        static const Alias kAliases[] = {
            { "single", 0 }, { "i1", 0 }, { "i2", 1 }, { "i3", 4 }, { "i4", 5 }, { "i5", 8 }, { "i6", 10 },
            { "v6", 9 }, { "v8", 12 }, { "v8 cross", 12 }, { "v8 flat", 13 }, { "v10", 14 }, { "v12", 15 },
            { "flat 4", 6 }, { "flat four", 6 }, { "flat 6", 11 }, { "flat six", 11 }, { "rotary", 16 },
        };
        for (const Alias& a : kAliases)
            if (s == a.name)
                return a.index;
        return -1;
    }

    void deriveFiring(Layout& l, const std::array<int, kMaxCyl>& fireRev)
    {
        l.ok = true;
        if (l.rotors > 0)
        {
            l.n = 3 * l.rotors;
            l.cyc = 1080.0;
            for (int r = 0; r < l.rotors; ++r)
            {
                const double t = wrap(l.bankAngle[0] - l.pin[static_cast<std::size_t>(r)], 360.0);
                for (int f = 0; f < 3; ++f)
                {
                    const auto c = static_cast<std::size_t>(3 * r + f);
                    l.fire[c] = t + 360.0 * f;
                    l.bank[c] = 0;
                    l.along[c] = r;
                }
            }
        }
        else
        {
            l.cyc = 720.0;
            for (int c = 0; c < l.n; ++c)
            {
                const auto i = static_cast<std::size_t>(c);
                int b = l.bank[i];
                if (b < 0 || b >= kMaxBanks)
                {
                    l.ok = false;
                    b = 0;
                }
                const double tdc = wrap(l.bankAngle[static_cast<std::size_t>(b)] - l.pin[i], 360.0);
                l.fire[i] = tdc + (fireRev[i] != 0 ? 360.0 : 0.0);
            }
        }
        const double f0 = l.fire[0];
        for (int c = 0; c < l.n; ++c)
        {
            double f = wrap(l.fire[static_cast<std::size_t>(c)] - f0, l.cyc);
            if (l.cyc - f < 1e-3)
                f = 0.0;
            l.fire[static_cast<std::size_t>(c)] = f;
        }
        const bool wasOk = l.ok;
        orderAndIntervals(l);
        l.ok = l.ok && wasOk;
    }

    Layout layoutOf(const FieldDoc& doc, bool cycle2)
    {
        // a two-stroke (card T): its cylinders in a row, firing every turn, evenly or all together
        if (const double cycle = fieldValue(doc, EF::cycle); cycle2 && std::isfinite(cycle) && cycle >= 0.5)
        {
            Layout l;
            l.twoStroke = true;
            l.cyc = 360.0;
            const int nf = doc.find("engine.tsCylinders"), ff = doc.find("engine.tsFiring");
            l.n = nf >= 0 ? std::clamp(static_cast<int>(std::lround(doc.get(nf))), 1, 4) : 1;
            const bool together = ff >= 0 && doc.get(ff) >= 0.5;
            l.name = "Two-stroke " + std::string(l.n == 1 ? "single" : l.n == 2 ? "twin" : l.n == 3 ? "triple" : "four");
            for (int c = 0; c < l.n; ++c)
            {
                const auto i = static_cast<std::size_t>(c);
                l.fire[i] = together ? 0.0 : 360.0 * c / l.n;
                l.pin[i] = wrap(-l.fire[i], 360.0);
                l.along[i] = c;
            }
            for (int c = 0; c < l.n; ++c)
                l.order[static_cast<std::size_t>(c)] = c;
            std::stable_sort(l.order.begin(), l.order.begin() + l.n, [&](int a, int b) {
                return l.fire[static_cast<std::size_t>(a)] < l.fire[static_cast<std::size_t>(b)];
            });
            for (int i = 0; i < l.n; ++i)
            {
                const double a = l.fire[static_cast<std::size_t>(l.order[static_cast<std::size_t>(i)])];
                const double b = i + 1 < l.n ? l.fire[static_cast<std::size_t>(l.order[static_cast<std::size_t>(i + 1)])] : 360.0;
                l.interval[static_cast<std::size_t>(i)] = b - a;
            }
            return l;   // firing together is the build's choice, not a fault
        }
        const double custom = fieldValue(doc, EF::custom);
        if (std::isfinite(custom) && custom >= 0.5)
        {
            Layout l;
            l.name = "Custom";
            l.custom = true;
            const double rotors = fieldValue(doc, EF::rotors);
            l.rotors = std::isfinite(rotors) ? std::clamp(static_cast<int>(std::lround(rotors)), 0, 4) : 0;
            const double cyl = fieldValue(doc, EF::cylinders);
            l.n = std::isfinite(cyl) ? std::clamp(static_cast<int>(std::lround(cyl)), 1, kMaxCyl) : 1;
            for (int b = 0; b < kMaxBanks; ++b)
            {
                const double a = fieldValue(doc, EF::bankAngle, b);
                l.bankAngle[static_cast<std::size_t>(b)] = std::isfinite(a) ? a : 0.0;
            }
            std::array<int, kMaxCyl> rev{};
            std::array<double, kMaxCyl> place{};
            l.banks = 1;
            for (int c = 0; c < kMaxCyl; ++c)
            {
                const auto i = static_cast<std::size_t>(c);
                const double b = fieldValue(doc, EF::bank, c), p = fieldValue(doc, EF::pin, c);
                const double f = fieldValue(doc, EF::fireRev, c), z = fieldValue(doc, EF::place, c);
                l.bank[i] = std::isfinite(b) ? static_cast<int>(std::lround(b)) : 0;
                l.pin[i] = std::isfinite(p) ? p : 0.0;
                rev[i] = std::isfinite(f) ? static_cast<int>(std::lround(f)) : 0;
                place[i] = std::isfinite(z) ? z : 0.0;
                if (c < l.n)
                    l.banks = std::max(l.banks, std::clamp(l.bank[i], 0, kMaxBanks - 1) + 1);
            }
            deriveFiring(l, rev);
            if (l.rotors == 0)
            {
                // places along the crank when they are set (in pitches of the smallest step), else by rank
                double lo = place[0], hi = place[0];
                for (int c = 1; c < l.n; ++c)
                {
                    lo = std::min(lo, place[static_cast<std::size_t>(c)]);
                    hi = std::max(hi, place[static_cast<std::size_t>(c)]);
                }
                if (hi - lo > 1e-4)
                {
                    double step = hi - lo;
                    for (int a = 0; a < l.n; ++a)
                        for (int b = 0; b < l.n; ++b)
                        {
                            const double d = std::fabs(place[static_cast<std::size_t>(a)] - place[static_cast<std::size_t>(b)]);
                            if (d > 1e-4)
                                step = std::min(step, d);
                        }
                    for (int c = 0; c < l.n; ++c)
                        l.along[static_cast<std::size_t>(c)] = (place[static_cast<std::size_t>(c)] - lo) / step;
                }
                else
                    placeByRank(l);
            }
            return l;
        }
        return dialLayoutOf(doc);
    }

    Layout dialLayoutOf(const FieldDoc& doc)
    {
        const int f = findField(doc, kEngineFields[static_cast<int>(EF::layout)]);
        if (f < 0)
            return {};
        const FieldInfo& info = doc.info(f);
        const int v = static_cast<int>(std::lround(doc.get(f)));
        int k = -1;
        if (v >= 0 && v < static_cast<int>(info.choices.size()))
            k = presetByName(info.choices[static_cast<std::size_t>(v)]);
        else if (info.choices.empty())
            k = v;
        if (k < 0 || k >= kPresetCount)
            return {};
        Layout l = presetLayout(k);
        if (v >= 0 && v < static_cast<int>(info.choices.size()))
            l.name = info.choices[static_cast<std::size_t>(v)];
        return l;
    }

    // ---- the cylinder ----------------------------------------------------------------------------------------------
    Cylinder cylinderOf(const FieldDoc& doc, const Layout& l)
    {
        Cylinder c;
        const double bore = fieldValue(doc, EF::bore), stroke = fieldValue(doc, EF::stroke);
        if (std::isfinite(bore) && bore > 0.0 && std::isfinite(stroke) && stroke > 0.0)
        {
            c.bore = bore;
            c.stroke = stroke;
        }
        else
        {
            const double disp = fieldValue(doc, EF::displacement), k = fieldValue(doc, EF::boreStroke);
            if (std::isfinite(disp) && disp > 0.0 && std::isfinite(k) && k > 0.0 && l.n > 0 && l.rotors == 0)
            {
                const double v = disp / l.n;
                c.stroke = std::cbrt(4.0 * v / (kPi * k * k));
                c.bore = k * c.stroke;
                c.boreDerived = true;
            }
        }
        const double rod = fieldValue(doc, EF::rod);
        if (std::isfinite(rod) && rod > 0.0)
            c.rod = rod;
        else if (std::isfinite(c.stroke))
        {
            c.rod = 3.3 * c.stroke * 0.5;
            c.rodDerived = true;
        }
        c.compression = fieldValue(doc, EF::compression);
        return c;
    }

    double pistonDrop(double deg, double r, double rod)
    {
        const double a = deg * kPi / 180.0, s = r * std::sin(a);
        return r * (1.0 - std::cos(a)) + rod - std::sqrt(std::max(0.0, rod * rod - s * s));
    }

    Derived derive(const Layout& l, const Cylinder& c, double limiter)
    {
        Derived d;
        d.compression = c.compression;
        d.limiter = limiter;
        if (!c.known())
            return d;
        if (l.n > 0 && l.rotors == 0)
            d.displacement = c.swept() * l.n;
        d.boreStroke = c.bore / c.stroke;
        if (std::isfinite(c.rod))
            d.rodRatio = c.rod / c.stroke;
        if (std::isfinite(limiter) && limiter > 0.0)
            d.pistonSpeed = 2.0 * c.stroke * limiter / (2.0 * kPi);
        return d;
    }

    std::vector<Warning> geometryWarnings(const FieldDoc& doc, const Layout& l, const Cylinder& c, const Derived& d)
    {
        std::vector<Warning> w;
        if (!c.known())
            return w;
        const double piston = fieldValue(doc, EF::pistonMass);
        if (std::isfinite(piston) && piston > 1500.0 * c.bore * c.bore * c.bore)
            w.push_back({ "A PISTON THIS HEAVY FOR ITS BORE: ABOUT TWICE A REAL ONE'S", { EF::pistonMass, EF::bore } });
        const double sb = c.stroke / c.bore;
        if (sb < 0.5 || sb > 2.0)
            w.push_back({ "NO PRODUCTION ENGINE IS THIS FAR OVER- OR UNDER-SQUARE", { EF::bore, EF::stroke, EF::boreStroke } });
        if (std::isfinite(d.pistonSpeed) && d.pistonSpeed > 30.0)
            w.push_back({ "A MEAN PISTON SPEED OVER 30 M/S AT THE LIMITER: PAST ANY RACE ENGINE'S", { EF::stroke, EF::limiter } });
        if (std::isfinite(d.rodRatio) && d.rodRatio < 1.4 && !c.rodDerived)
            w.push_back({ "A ROD THIS SHORT FOR ITS STROKE: SHORTER THAN ANY PRODUCTION ENGINE'S", { EF::rod, EF::stroke } });
        (void)l;
        return w;
    }
}
