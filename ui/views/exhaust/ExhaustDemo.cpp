// ui/views/exhaust/ExhaustDemo.cpp: see ExhaustDemo.h. The fields mirror sim/exhaust/buildspec.fm (ranges, defaults,
// labels, quantities); the builds are the four the package card names and two more.
#include "ExhaustDemo.h"

#include "ExhaustModel.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <iterator>

namespace tee::ui::exhaust
{
    namespace
    {
        FieldInfo field(std::string path, FieldKind kind, double lo, double hi, double def, std::string label,
                        Quantity q = Quantity::none, std::string group = {})
        {
            FieldInfo f;
            f.path = std::move(path);
            f.kind = kind;
            f.min = lo;
            f.max = hi;
            f.def = def;
            f.label = std::move(label);
            f.quantity = q;
            f.group = std::move(group);
            return f;
        }
        FieldInfo choice(std::string path, std::string label, std::vector<std::string> names, int def, double lo = 0,
                         std::string group = {})
        {
            FieldInfo f = field(std::move(path), FieldKind::choice, lo, lo + static_cast<double>(names.size()) - 1.0, def,
                                std::move(label), Quantity::none, std::move(group));
            f.choices = std::move(names);
            return f;
        }
        // An array of `n` as BuildDoc names it: "<path>[i]".
        void array(std::vector<FieldInfo>& out, const FieldInfo& f, int n)
        {
            for (int i = 0; i < n; ++i)
            {
                FieldInfo e = f;
                e.path += "[" + std::to_string(i) + "]";
                out.push_back(e);
            }
        }
        void set(FieldDoc& doc, std::string_view path, double v)
        {
            const int f = doc.find(path);
            if (f >= 0)
                doc.set(f, v);
        }
        void setAt(FieldDoc& doc, std::string_view path, int i, double v)
        {
            set(doc, std::string(path) + "[" + std::to_string(i) + "]", v);
        }
    }

    std::vector<FieldInfo> exhaustDemoFields()
    {
        std::vector<FieldInfo> out = demoFields(false);   // U1's alone (the hooks add this list to it)
        // the real Spec's layouts and aspirations (the demo's are shorter lists)
        std::vector<std::string> layouts;
        for (int k = 0; k < 17; ++k)
            layouts.emplace_back(layoutName(k));
        for (FieldInfo& f : out)
        {
            if (f.path == "engine.layout")
            {
                f.choices = layouts;
                f.max = 16;
                f.def = 12;
            }
            if (f.path == "boost.kind")
            {
                f.choices = { "Natural", "Turbo", "Twin turbo", "Quad turbo", "Supercharger" };
                f.max = 4;
                f.def = 2;
            }
        }
        out.push_back(choice("boost.layout", "Arrangement",
                             { "As the aspiration says", "One", "Two, a bank each", "Sequential", "Compound", "Four, two a bank" }, 0));
        using Q = Quantity;
        const std::string h = "Headers", c = "Collectors", ch = "Chain";
        array(out, choice("exhaust.primaryTo", "Primary joins",
                          { "The layout's", "Open stack", "Collector 1", "Collector 2", "Collector 3", "Collector 4",
                            "Collector 5", "Collector 6", "Collector 7", "Collector 8" }, -2, -2, h), kPorts);
        array(out, field("exhaust.primLen", FieldKind::number, 0, 3, 0, "Primary length", Q::length, h), kPorts);
        array(out, field("exhaust.primD", FieldKind::number, 0, 0.12, 0, "Primary bore", Q::length, h), kPorts);
        out.push_back(field("exhaust.primSpread", FieldKind::number, 0, 0.3, 0.06, "Primary differences", Q::fraction, h));
        out.push_back(field("exhaust.primStepAt", FieldKind::number, 0, 0.9, 0, "Step at", Q::fraction, h));
        out.push_back(field("exhaust.primStepD", FieldKind::number, 1, 1.5, 1.1, "Step", Q::ratio, h));
        array(out, field("exhaust.collectorTo", FieldKind::integer, -1, 9, -1, "Collector joins", Q::none, c), kCollectors);
        array(out, field("exhaust.colLen", FieldKind::number, 0, 1.5, 0, "Collector pipe length", Q::length, c), kCollectors);
        array(out, field("exhaust.colD", FieldKind::number, 0, 0.15, 0, "Collector pipe bore", Q::length, c), kCollectors);
        array(out, field("exhaust.colVol", FieldKind::number, 0, 0.004, 0, "Collector volume", Q::volume, c), kCollectors);
        out.push_back(field("exhaust.sides", FieldKind::integer, 0, 2, 0, "Sides", Q::none, ch));
        array(out, field("exhaust.mainLen", FieldKind::number, 0, 5, 0, "Main pipe length", Q::length, ch), kSides);
        array(out, field("exhaust.mainD", FieldKind::number, 0, 0.13, 0, "Main pipe bore", Q::length, ch), kSides);
        array(out, choice("exhaust.converter", "Converter", { "None", "Standard", "High-flow" }, 1, 0, ch), kSides);
        out.push_back(choice("exhaust.crossKind", "Crossover kind", { "None", "H", "X" }, 1, 0, ch));
        out.push_back(choice("exhaust.crossAt", "Crossover at", { "Head", "After main pipe" }, 0, 0, ch));
        out.push_back(field("exhaust.crossD", FieldKind::number, 0, 0.13, 0, "Crossover bore", Q::length, ch));
        out.push_back(field("exhaust.crossLen", FieldKind::number, 0.05, 1, 0.25, "H pipe length", Q::length, ch));
        array(out, choice("exhaust.resonator", "Resonator", { "None", "Quarter-wave", "Helmholtz", "Packed bottle" }, 0, 0, ch), kSides);
        array(out, field("exhaust.resHz", FieldKind::number, 40, 2000, 200, "Resonator note", Q::frequency, ch), kSides);
        array(out, choice("exhaust.mufKind", "Muffler kind", { "None", "Straight-through", "Chambered", "Three-pass" }, 2, 0, ch), kSides);
        array(out, field("exhaust.mufSize", FieldKind::number, -1, 1, -1, "Muffler size", Q::fraction, ch), kSides);
        array(out, field("exhaust.mufLen", FieldKind::number, 0, 1, 0, "Muffler chamber", Q::length, ch), kSides);
        array(out, choice("exhaust.bypass", "Bypass valve", { "None", "Shut", "Open", "By speed and load", "By the ECU" }, 0, 0, ch), kSides);
        out.push_back(field("exhaust.bypassRpm", FieldKind::number, 1000, 9000, 3500, "Valve opens above", Q::none, ch));
        array(out, field("exhaust.tailLen", FieldKind::number, 0, 2, 0, "Tailpipe length", Q::length, ch), kSides);
        array(out, field("exhaust.tailD", FieldKind::number, 0, 0.13, 0, "Tailpipe bore", Q::length, ch), kSides);
        array(out, field("exhaust.tips", FieldKind::integer, 1, 2, 1, "Tips", Q::none, ch), kSides);
        // card T's two-stroke: the cycle and the pipe (sim/spec/engine.fm), for the two-stroke builds
        if (std::none_of(out.begin(), out.end(), [](const FieldInfo& f) { return f.path == "engine.cycle"; }))
        {
            const std::string ts = "Two-stroke";
            out.push_back(choice("engine.cycle", "Cycle", { "Four-stroke", "Two-stroke" }, 0, 0, ts));
            out.push_back(field("engine.tsCylinders", FieldKind::integer, 1, 4, 1, "Cylinders", Q::none, ts));
            out.push_back(field("engine.tsExhaustDur", FieldKind::number, 140, 220, 192, "Exhaust port open", Q::none, ts));
            out.push_back(choice("engine.tsChamber", "Pipe", { "Straight pipe", "Expansion chamber" }, 1, 0, ts));
            out.push_back(field("engine.tsHeaderLen", FieldKind::number, 0.02, 0.6, 0.16, "Header", Q::length, ts));
            out.push_back(field("engine.tsDiffuserLen", FieldKind::number, 0.05, 0.8, 0.33, "Diffuser", Q::length, ts));
            out.push_back(field("engine.tsBellyLen", FieldKind::number, 0.0, 0.5, 0.12, "Belly", Q::length, ts));
            out.push_back(field("engine.tsBaffleLen", FieldKind::number, 0.05, 0.5, 0.24, "Baffle cone", Q::length, ts));
            out.push_back(field("engine.tsStingerLen", FieldKind::number, 0.05, 0.5, 0.22, "Stinger", Q::length, ts));
            out.push_back(field("engine.tsHeaderDiam", FieldKind::number, 0.015, 0.08, 0.038, "Header's bore", Q::length, ts));
            out.push_back(field("engine.tsBellyDiam", FieldKind::number, 0.03, 0.2, 0.115, "Belly's bore", Q::length, ts));
            out.push_back(field("engine.tsStingerDiam", FieldKind::number, 0.01, 0.05, 0.022, "Stinger's bore", Q::length, ts));
            out.push_back(field("engine.tsConeSteps", FieldKind::integer, 1, 16, 8, "Steps a cone", Q::none, ts));
            out.push_back(choice("engine.tsSilencer", "Silencer", { "None", "Packed" }, 0, 0, ts));
            out.push_back(field("engine.tsSilencerLen", FieldKind::number, 0.1, 0.6, 0.4, "Silencer's length", Q::length, ts));
            out.push_back(field("engine.tsSilencerDiam", FieldKind::number, 0.012, 0.12, 0.026, "Silencer's bore", Q::length, ts));
        }
        array(out, choice("exhaust.exitAt", "Exits",
                          { "The layout's", "Rear centre", "Rear left", "Rear right", "Side left", "Side right", "Front", "Up" },
                          -1, -1, ch), kSides);
        return out;
    }

    bool setExhaustDemoBuild(FieldDoc& doc, std::string_view name)
    {
        doc.beginGesture();
        // every exhaust field back to its default first
        for (int i = 0; i < doc.count(); ++i)
        {
            const FieldInfo& f = doc.info(i);
            if (f.section() == "exhaust" || f.path == "boost.layout" || f.path.rfind("engine.ts", 0) == 0 || f.path == "engine.cycle")
                doc.set(i, f.def);
        }
        bool known = true;
        if (name == "na4-421")             // an inline four, 1 and 4 paired, 2 and 3 paired, into one: 4-2-1
        {
            set(doc, "engine.layout", 5);
            set(doc, "boost.kind", 0);
            const int pair[4] = { 0, 1, 1, 0 };
            for (int i = 0; i < 4; ++i)
                setAt(doc, "exhaust.primaryTo", i, pair[i]);
            setAt(doc, "exhaust.primLen", 0, 0.78);
            setAt(doc, "exhaust.primLen", 1, 0.78);
            setAt(doc, "exhaust.primLen", 2, 0.78);
            setAt(doc, "exhaust.primLen", 3, 0.78);
            setAt(doc, "exhaust.collectorTo", 0, 4);
            setAt(doc, "exhaust.collectorTo", 1, 4);
            setAt(doc, "exhaust.collectorTo", 2, 0);
            set(doc, "exhaust.sides", 1);
            setAt(doc, "exhaust.resonator", 0, 1);
            setAt(doc, "exhaust.mufKind", 0, 2);
        }
        else if (name == "tt-v8-x")        // a twin-turbo V8, dual with an X, straight-through cans on valves, two tips
        {
            set(doc, "engine.layout", 12);
            set(doc, "boost.kind", 2);
            set(doc, "exhaust.sides", 2);
            set(doc, "exhaust.crossKind", 2);
            for (int s = 0; s < 2; ++s)
            {
                setAt(doc, "exhaust.converter", s, 2);
                setAt(doc, "exhaust.mufKind", s, 1);
                setAt(doc, "exhaust.bypass", s, 3);
                setAt(doc, "exhaust.tips", s, 2);
            }
        }
        else if (name == "sc-v8-180")      // a supercharged crossplane V8 with 180-degree headers, an H, Helmholtz, three-pass
        {
            set(doc, "engine.layout", 12);
            set(doc, "boost.kind", 4);
            // firing order 1-8-4-3-6-5-7-2: each collector takes every other firing, so across the banks
            const int to[8] = { 0, 1, 1, 0, 1, 0, 0, 1 };
            for (int i = 0; i < 8; ++i)
            {
                setAt(doc, "exhaust.primaryTo", i, to[i]);
                setAt(doc, "exhaust.primLen", i, to[i] == (i & 1) ? 0.82 : 1.02);   // the crossing ones are longer
            }
            set(doc, "exhaust.sides", 2);
            set(doc, "exhaust.crossKind", 1);
            set(doc, "exhaust.crossAt", 1);
            for (int s = 0; s < 2; ++s)
            {
                setAt(doc, "exhaust.resonator", s, 2);
                setAt(doc, "exhaust.mufKind", s, 3);
            }
        }
        else if (name == "single-stack")   // a thumper with an open stack: one port, its pipe ends in air
        {
            set(doc, "engine.layout", 0);
            set(doc, "boost.kind", 0);
            setAt(doc, "exhaust.primaryTo", 0, -1);
            setAt(doc, "exhaust.primLen", 0, 0.9);
        }
        else if (name == "v12-log")        // a V12 on log manifolds, one pipe a side, an H, bottles, chambered, side exits
        {
            set(doc, "engine.layout", 15);
            set(doc, "boost.kind", 0);
            for (int i = 0; i < 12; ++i)
                setAt(doc, "exhaust.primLen", i, 0.12);
            setAt(doc, "exhaust.colVol", 0, 0.0015);
            setAt(doc, "exhaust.colVol", 1, 0.0015);
            set(doc, "exhaust.sides", 2);
            set(doc, "exhaust.crossKind", 1);
            for (int s = 0; s < 2; ++s)
            {
                setAt(doc, "exhaust.resonator", s, 3);
                setAt(doc, "exhaust.exitAt", s, 4 + s);
                setAt(doc, "exhaust.bypass", s, 1 + s);
            }
        }
        else if (name == "ts-125")         // a 125 single on its expansion chamber, open
        {
            set(doc, "engine.cycle", 1);
            set(doc, "engine.tsCylinders", 1);
        }
        else if (name == "ts-twin")        // a twin on two chambers, silenced, a longer pipe (tuned lower)
        {
            set(doc, "engine.cycle", 1);
            set(doc, "engine.tsCylinders", 2);
            set(doc, "engine.tsSilencer", 1);
            set(doc, "engine.tsDiffuserLen", 0.40);
            set(doc, "engine.tsBellyLen", 0.16);
        }
        else if (name == "flat4")          // a boxer four's unequal-length headers into one pipe, a turbo
        {
            set(doc, "engine.layout", 6);
            set(doc, "boost.kind", 1);
        }
        else
            known = false;
        doc.endGesture();
        return known;
    }

    std::vector<std::pair<const char*, double>> exhaustLiveValues(double rpm, double load)
    {
        load = std::clamp(load, 0.0, 1.0);
        const double hot = std::clamp(0.35 + 0.65 * load * std::min(1.0, rpm / 6000.0), 0.0, 1.0);
        const double port = 820.0 + 330.0 * hot;
        // the gas cools along the system; the walls sit below it (card G's sections, in the gas's order)
        const double valve = rpm > 3500.0 && load > 0.5 ? 1.0 : 0.0;
        const double db = -24.0 + 18.0 * hot;
        return {
            { "exhaust.gasTemperature[1]", port - 70.0 },    { "exhaust.gasTemperature[6]", 420.0 + 190.0 * hot },
            { "exhaust.wallTemperature[0]", port - 120.0 },  { "exhaust.wallTemperature[1]", port - 170.0 },
            { "exhaust.wallTemperature[2]", port - 200.0 },  { "exhaust.wallTemperature[3]", port - 330.0 },
            { "exhaust.wallTemperature[4]", port - 400.0 },  { "exhaust.wallTemperature[5]", 430.0 + 160.0 * hot },
            { "exhaust.wallTemperature[6]", 390.0 + 140.0 * hot },
            // six ends: each side's tail and its two tips (slots.fm: 48 + side * 15 + 11, 12, 13); a reader takes those
            // the build has
            { "exhaust.ends", 6.0 },
            // the network as built: G's typical build (docs/models/exhaust.md, "What the network costs")
            { "exhaust.pipes", 32.0 }, { "exhaust.nodes", 9.0 }, { "exhaust.pairs", 8.0 }, { "exhaust.openEnds", 2.0 },
            { "exhaust.endSlot[0]", 59.0 }, { "exhaust.endSlot[1]", 60.0 }, { "exhaust.endSlot[2]", 61.0 },
            { "exhaust.endSlot[3]", 74.0 }, { "exhaust.endSlot[4]", 75.0 }, { "exhaust.endSlot[5]", 76.0 },
            { "exhaust.endLevel[0]", db }, { "exhaust.endLevel[1]", db - 3.0 }, { "exhaust.endLevel[2]", db - 3.2 },
            { "exhaust.endLevel[3]", db - 1.0 }, { "exhaust.endLevel[4]", db - 4.0 }, { "exhaust.endLevel[5]", db - 4.1 },
            { "exhaust.valve[0]", valve }, { "exhaust.valve[1]", valve }, { "exhaust.flame", 0.0 },
        };
    }

    namespace
    {
        // The demo's fields read through the engine (it watches the document): enough of a FieldDoc for resolveExhaust.
        class EngineFields final : public FieldDoc
        {
        public:
            EngineFields(const DemoEngine& e, const std::vector<FieldInfo>& infos) : e_(e), infos_(infos) {}
            int count() const override { return static_cast<int>(infos_.size()); }
            const FieldInfo& info(int i) const override { return infos_[static_cast<std::size_t>(i)]; }
            double get(int i) const override
            {
                const FieldInfo& f = infos_[static_cast<std::size_t>(i)];
                return e_.field(f.path, f.def);
            }
            void set(int, double) override {}
            Curve16 curve(int) const override { return {}; }
            void setCurve(int, const Curve16&) override {}
            Map16 map(int) const override { return {}; }
            void setMap(int, const Map16&) override {}
            void beginGesture() override {}
            void endGesture() override {}
            std::uint64_t revision() const override { return 1; }

        private:
            const DemoEngine& e_;
            const std::vector<FieldInfo>& infos_;
        };
        // the fields as the demo's document has them: U1's, then this package's that it lacks (as the hook adds them)
        const std::vector<FieldInfo>& hookInfos()
        {
            static const std::vector<FieldInfo> infos = [] {
                std::vector<FieldInfo> v = demoFields(false);
                for (const FieldInfo& f : exhaustDemoFields())
                    if (f.section() == "exhaust"
                        && std::none_of(v.begin(), v.end(), [&](const FieldInfo& g) { return g.path == f.path; }))
                        v.push_back(f);
                return v;
            }();
            return infos;
        }
        // A blowdown at a port: the pressure (gauge Pa) `deg` degrees after its cylinder fired.
        double blowdown(double deg, double load)
        {
            const double a = std::fmod(deg + 7200.0, 720.0);
            return (8e3 + 34e3 * load) * std::exp(-((a - 190.0) / 38.0) * ((a - 190.0) / 38.0)) - 3e3;
        }
    }

    void exhaustDucts(const ExhaustBuild& b, double phase, double rpm, double load, float* used, float* nearP, float* farP,
                      float* midP)
    {
        float midOwn[84];
        float* mid = midP != nullptr ? midP : midOwn;
        for (int i = 0; i < 84; ++i)
            used[i] = nearP[i] = farP[i] = mid[i] = 0.0f;
        const double deg = phase * b.cycle;
        const double degPerMetre = rpm * 6.0 / 520.0;   // a pulse's travel at some 520 m/s, in crank degrees a metre
        double side[2] = { 0.0, 0.0 };
        for (int i = 0; i < b.ports; ++i)
        {
            const auto k = static_cast<std::size_t>(i);
            const double a = deg - b.fire[k];
            used[i] = 1.0f;
            nearP[i] = static_cast<float>(blowdown(a, load));
            farP[i] = static_cast<float>(0.75 * blowdown(a - b.primLen[k] * degPerMetre, load));
            mid[i] = static_cast<float>(0.87 * blowdown(a - 0.5 * b.primLen[k] * degPerMetre, load));
            const int c = b.portTo[k];
            if (c >= 0)
            {
                const int to = b.colTo[static_cast<std::size_t>(c)];
                side[to >= 2 ? 0 : std::min(to, 1)] += farP[i];
            }
        }
        for (int s = 0; s < 2; ++s)
        {
            if (!b.sideOn[static_cast<std::size_t>(s)])
                continue;
            const int base = 48 + s * 15;
            used[base] = used[base + 11] = 1.0f;
            nearP[base] = static_cast<float>(0.6 * side[s]);
            farP[base] = static_cast<float>(0.35 * side[s] * std::cos(deg * 0.05));
            mid[base] = static_cast<float>(0.45 * side[s] * std::cos(deg * 0.025));
            nearP[base + 11] = static_cast<float>(0.2 * side[s] * std::sin(deg * 0.04));
            mid[base + 11] = static_cast<float>(0.1 * side[s] * std::sin(deg * 0.04 - 0.6));
            farP[base + 11] = 0.0f;   // an open end
        }
    }

    int exhaustTwoStrokePort(const ExhaustBuild& b, float* out, int max)
    {
        // card T's measures (degrees after top dead centre, bar over the air), a closed loop round the turn
        static const double kAt[] = { 84, 96, 108, 132, 156, 180, 216, 246, 264, 276, 306, 342 };
        static const double kBar[] = { -0.02, 0.45, 0.58, 0.17, -0.16, -0.21, -0.06, 0.05, 0.29, 0.18, 0.37, -0.19 };
        const int k = static_cast<int>(std::size(kAt));
        const double stretch = std::clamp(static_cast<double>(b.ts.exhaustDur), 60.0, 300.0) / 192.0;
        const auto at = [&](int i) {   // the measure's angle at this build's timing, unwrapped round the turn
            const int j = ((i % k) + k) % k;
            const double turns = std::floor(static_cast<double>(i) / k);
            return 180.0 + (kAt[j] - 180.0) * stretch + 360.0 * turns;
        };
        const auto bar = [&](int i) { return kBar[((i % k) + k) % k]; };
        const int m = std::min(max, 360);
        int seg = -1;
        for (int d = 0; d < m; ++d)
        {
            double x = d;
            if (x < at(0))
                x += 360.0;
            while (seg < 0 || !(x >= at(seg) && x < at(seg + 1)))
                seg = seg < 0 || x < at(seg) ? 0 : seg + 1;
            // Catmull-Rom through the loop's points, so the curve is smooth through each measure
            const double t = (x - at(seg)) / (at(seg + 1) - at(seg));
            const double p0 = bar(seg - 1), p1 = bar(seg), p2 = bar(seg + 1), p3 = bar(seg + 2);
            const double v = 0.5 * (2.0 * p1 + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t * t
                                    + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t * t * t);
            out[d] = static_cast<float>(101325.0 + 1e5 * v);
        }
        return m;
    }

    void exhaustDemoLive(DemoEngine& engine)
    {
        const auto get = [&](const char* name, double fallback) {
            const int h = engine.findValue(name);
            const double v = h >= 0 ? engine.value(h) : fallback;
            return std::isfinite(v) ? v : fallback;
        };
        for (const auto& [name, v] : exhaustLiveValues(get("engine.speed", 300.0) * 30.0 / 3.14159265358979, get("engine.throttle", 0.3)))
            engine.set(name, v);
    }
}

namespace tee::ui
{
    // U1's demo hook (TEE_UI_DEMO in register.h): the exhaust builder's fields, and the telemetry the simulator does
    // not write yet, following the demo's script.
    void exhaustDemo(DemoExtras& x)
    {
        if (x.fields != nullptr)
            for (const FieldInfo& f : exhaust::exhaustDemoFields())
                if (f.section() == "exhaust"
                    && std::none_of(x.fields->begin(), x.fields->end(), [&](const FieldInfo& g) { return g.path == f.path; }))
                    x.fields->push_back(f);
        if (x.engine != nullptr)
        {
            // every pipe's pressure at its two ends (card G's Scope arrays), each primary pulsing in its firing order
            const auto ducts = [](int which) {
                return [which](const DemoEngine& e, float* out, int max) {
                    const exhaust::EngineFields doc(e, exhaust::hookInfos());
                    float u[84], n[84], f[84], md[84];
                    exhaust::exhaustDucts(exhaust::resolveExhaust(doc), e.phase(), e.rpm(), e.throttle(), u, n, f, md);
                    const float* src = which == 0 ? u : which == 1 ? n : which == 2 ? f : md;
                    const int m = std::min(max, 84);
                    for (int i = 0; i < m; ++i)
                        out[i] = src[i];
                    return m;
                };
            };
            x.engine->addScope("ductUsed", ducts(0));
            x.engine->addScope("ductNear", ducts(1));
            x.engine->addScope("ductFar", ducts(2));
            x.engine->addScope("ductMid", ducts(3));
        }
        if (x.engine != nullptr)
            for (const auto& [name, v0] : exhaust::exhaustLiveValues(3000.0, 0.5))
            {
                (void)v0;
                const char* n = name;
                x.engine->addTelemetry(n, [n](const DemoEngine& e) {
                    for (const auto& [m, v] : exhaust::exhaustLiveValues(e.rpm(), e.throttle()))
                        if (std::string_view(m) == n)
                            return v;
                    return std::nan("");
                });
            }
    }
}
