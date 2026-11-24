// ui/views/exhaust/ExhaustDemo.cpp: see ExhaustDemo.h. The fields mirror sim/exhaust/buildspec.fm (ranges, defaults,
// labels, quantities); the builds are the four the package card names and two more.
#include "ExhaustDemo.h"

#include "ExhaustModel.h"

#include <algorithm>
#include <cmath>
#include <string>

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
        std::vector<FieldInfo> out = demoFields();
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
        out.erase(std::remove_if(out.begin(), out.end(),
                                 [](const FieldInfo& f) { return f.path == "exhaust.crossover" || f.path == "exhaust.muffler"; }),
                  out.end());
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
        out.push_back(choice("exhaust.crossover", "Crossover", { "None", "H", "X" }, 1, 0, ch));
        out.push_back(choice("exhaust.crossAt", "Crossover at", { "Head", "After main pipe" }, 0, 0, ch));
        out.push_back(field("exhaust.crossD", FieldKind::number, 0, 0.13, 0, "Crossover bore", Q::length, ch));
        out.push_back(field("exhaust.crossLen", FieldKind::number, 0.05, 1, 0.25, "H pipe length", Q::length, ch));
        array(out, choice("exhaust.resonator", "Resonator", { "None", "Quarter-wave", "Helmholtz", "Packed bottle" }, 0, 0, ch), kSides);
        array(out, field("exhaust.resHz", FieldKind::number, 40, 2000, 200, "Resonator note", Q::frequency, ch), kSides);
        array(out, choice("exhaust.muffler", "Muffler", { "None", "Straight-through", "Chambered", "Three-pass" }, 2, 0, ch), kSides);
        array(out, field("exhaust.mufSize", FieldKind::number, -1, 1, -1, "Muffler size", Q::fraction, ch), kSides);
        array(out, field("exhaust.mufLen", FieldKind::number, 0, 1, 0, "Muffler chamber", Q::length, ch), kSides);
        array(out, choice("exhaust.bypass", "Bypass valve", { "None", "Shut", "Open", "By speed and load", "By the ECU" }, 0, 0, ch), kSides);
        out.push_back(field("exhaust.bypassRpm", FieldKind::number, 1000, 9000, 3500, "Valve opens above", Q::none, ch));
        array(out, field("exhaust.tailLen", FieldKind::number, 0, 2, 0, "Tailpipe length", Q::length, ch), kSides);
        array(out, field("exhaust.tailD", FieldKind::number, 0, 0.13, 0, "Tailpipe bore", Q::length, ch), kSides);
        array(out, field("exhaust.tips", FieldKind::integer, 1, 2, 1, "Tips", Q::none, ch), kSides);
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
            if (f.section() == "exhaust" || f.path == "boost.layout")
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
            setAt(doc, "exhaust.muffler", 0, 2);
        }
        else if (name == "tt-v8-x")        // a twin-turbo V8, dual with an X, straight-through cans on valves, two tips
        {
            set(doc, "engine.layout", 12);
            set(doc, "boost.kind", 2);
            set(doc, "exhaust.sides", 2);
            set(doc, "exhaust.crossover", 2);
            for (int s = 0; s < 2; ++s)
            {
                setAt(doc, "exhaust.converter", s, 2);
                setAt(doc, "exhaust.muffler", s, 1);
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
            set(doc, "exhaust.crossover", 1);
            set(doc, "exhaust.crossAt", 1);
            for (int s = 0; s < 2; ++s)
            {
                setAt(doc, "exhaust.resonator", s, 2);
                setAt(doc, "exhaust.muffler", s, 3);
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
            set(doc, "exhaust.crossover", 1);
            for (int s = 0; s < 2; ++s)
            {
                setAt(doc, "exhaust.resonator", s, 3);
                setAt(doc, "exhaust.exitAt", s, 4 + s);
                setAt(doc, "exhaust.bypass", s, 1 + s);
            }
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

    void exhaustDemoLive(DemoEngine& engine)
    {
        const auto get = [&](const char* name, double fallback) {
            const int h = engine.findValue(name);
            const double v = h >= 0 ? engine.value(h) : fallback;
            return std::isfinite(v) ? v : fallback;
        };
        const double load = std::clamp(get("engine.throttle", 0.3), 0.0, 1.0);
        const double rpm = get("engine.speed", 300.0) * 30.0 / 3.14159265358979;
        const double hot = std::clamp(0.35 + 0.65 * load * std::min(1.0, rpm / 6000.0), 0.0, 1.0);
        const double port = 820.0 + 330.0 * hot;
        // the gas cools along the system; the walls sit below it
        engine.set("exhaust.collectorTemperature", port - 70.0);
        engine.set("exhaust.tailTemperature", 420.0 + 190.0 * hot);
        const double walls[7] = { port - 120.0, port - 170.0, port - 200.0, port - 330.0, port - 400.0, 430.0 + 160.0 * hot,
                                  390.0 + 140.0 * hot };
        static const char* const kWalls[7] = { "exhaust.wallPrimaries", "exhaust.wallCollectors", "exhaust.wallMerges",
                                               "exhaust.wallMain", "exhaust.wallLink", "exhaust.wallMufflers",
                                               "exhaust.wallTails" };
        for (int i = 0; i < 7; ++i)
            engine.set(kWalls[i], walls[i]);
        engine.set("exhaust.tailLevel[0]", 0.18 + 0.55 * hot);
        engine.set("exhaust.tailLevel[1]", 0.16 + 0.52 * hot);
        const double valve = rpm > 3500.0 && load > 0.5 ? 1.0 : 0.0;
        engine.set("exhaust.valve[0]", valve);
        engine.set("exhaust.valve[1]", valve);
    }
}
