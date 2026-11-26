// ui/views/air/AirDemo.cpp: see AirDemo.h. The fields mirror machinespec.fm and lab-spec.fm (ranges, defaults, labels,
// quantities); the operating point is placed on the build's own map at the flow the demo engine would take.
#include "AirDemo.h"

#include "AirModel.h"

#include "views/exhaust/ExhaustDemo.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tee::ui::air
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
        FieldInfo choice(std::string path, std::string label, std::vector<std::string> names, int def, std::string group = {})
        {
            FieldInfo f = field(std::move(path), FieldKind::choice, 0, static_cast<double>(names.size()) - 1.0, def,
                                std::move(label), Quantity::none, std::move(group));
            f.choices = std::move(names);
            return f;
        }
        void set(FieldDoc& doc, std::string_view path, double v)
        {
            const int f = doc.find(path);
            if (f >= 0)
                doc.set(f, v);
        }
        double get(const DemoEngine& e, const char* name, double fallback)
        {
            const int h = e.findValue(name);
            const double v = h >= 0 ? e.value(h) : fallback;
            return std::isfinite(v) ? v : fallback;
        }
    }

    std::vector<FieldInfo> airDemoFields()
    {
        std::vector<FieldInfo> out = exhaust::exhaustDemoFields();
        // the real Spec's boost.size (a ratio) and blow-off in place of the demo's dials
        out.erase(std::remove_if(out.begin(), out.end(), [](const FieldInfo& f) { return f.path == "boost.size"; }), out.end());
        using Q = Quantity;
        out.push_back(field("engine.displacement", FieldKind::number, 0.00005, 0.03, 0.00576, "Displacement", Q::volume));
        out.push_back(field("engine.redlineTrim", FieldKind::number, -3000, 3000, 0, "Redline trim"));
        const std::string g = "Compressor", t = "Turbine", ctl = "Control", ch = "Charge", sc = "Supercharger", ex = "Extras";
        out.push_back(field("boost.size", FieldKind::number, 0.5, 2, 1, "Size over the match", Q::ratio));
        out.push_back(field("boost.compInducer", FieldKind::number, 0, 0.12, 0, "Inducer diameter", Q::length, g));
        out.push_back(field("boost.compExducer", FieldKind::number, 0, 0.16, 0, "Exducer diameter", Q::length, g));
        out.push_back(field("boost.compAR", FieldKind::number, 0.3, 1.2, 0.6, "Compressor A/R", Q::ratio, g));
        out.push_back(field("boost.compEta", FieldKind::number, 0.55, 0.85, 0.76, "Peak efficiency", Q::fraction, g));
        out.push_back(field("boost.turbWheel", FieldKind::number, 0, 0.14, 0, "Turbine wheel", Q::length, t));
        out.push_back(choice("boost.scrolls", "Housing", { "Single scroll", "Twin scroll" }, 0, t));
        out.push_back(choice("boost.vanes", "Nozzle", { "Fixed", "Variable vanes" }, 0, t));
        out.push_back(field("boost.vaneMin", FieldKind::number, 0.15, 0.8, 0.3, "Vanes shut to", Q::fraction, t));
        out.push_back(field("boost.maxSpeed", FieldKind::number, 0, 40000, 0, "Shaft speed limit", Q::rotation, "Shaft"));
        out.push_back(choice("boost.gate", "Wastegate", { "Internal", "External" }, 0, ctl));
        out.push_back(field("boost.gateSpring", FieldKind::number, 10000, 300000, 70000, "Gate spring", Q::pressure, ctl));
        out.push_back(field("boost.target", FieldKind::number, 0, 300000, 0, "Boost target", Q::pressure, ctl));
        out.push_back(choice("boost.blowOffKind", "Blow-off valve", { "None", "To the air", "Recirculating" }, 1, ch));
        out.push_back(field("boost.blowOffSpring", FieldKind::number, 5000, 100000, 15000, "Blow-off opens at", Q::pressure, ch));
        out.push_back(field("boost.icEffect", FieldKind::number, 0, 0.95, 0.7, "Intercooler effectiveness", Q::fraction, ch));
        out.push_back(field("boost.icDrop", FieldKind::number, 0, 50000, 12000, "Intercooler drop", Q::pressure, ch));
        out.push_back(choice("boost.charger", "Supercharger", { "Roots", "Twin-screw", "Centrifugal" }, 0, sc));
        out.push_back(field("boost.scRatio", FieldKind::number, 1, 4, 2.4, "Pulley ratio", Q::ratio, sc));
        out.push_back(field("boost.scStepUp", FieldKind::number, 2, 6, 3.6, "Step-up gear", Q::ratio, sc));
        out.push_back(field("boost.scBypass", FieldKind::toggle, 0, 1, 1, "Bypass at part throttle", Q::none, sc));
        out.push_back(field("boost.antiLag", FieldKind::toggle, 0, 1, 0, "Anti-lag", Q::none, ex));
        out.push_back(field("boost.nitrousShot", FieldKind::number, 0, 300000, 0, "Nitrous shot", Q::power, ex));
        out.push_back(field("boost.nitrousBottle", FieldKind::number, 0.5, 10, 4.5, "Nitrous bottle", Q::mass, ex));
        out.push_back(choice("intake.throttleKind", "Throttle", { "One body and a plenum", "A body a runner" }, 0, "Throttle"));
        out.push_back(choice("intake.trumpets", "Trumpets open to", { "The air box", "The air" }, 0, "Throttle"));
        out.push_back(field("intake.plenumVolume", FieldKind::number, 0, 0.03, 0, "Plenum volume", Q::volume, "Manifold"));
        out.push_back(field("intake.runnerLength", FieldKind::number, 0, 1.2, 0, "Runner length", Q::length, "Manifold"));
        out.push_back(field("intake.runnerLength2", FieldKind::number, 0, 1.2, 0, "Second runner length", Q::length, "Manifold"));
        out.push_back(field("intake.runnerSwitch", FieldKind::number, 0, 1100, 0, "Second length from", Q::rotation, "Manifold"));
        out.push_back(field("intake.boxVolume", FieldKind::number, 0, 0.05, 0, "Air box volume", Q::volume, "Air box"));
        return out;
    }

    bool setAirDemoBuild(FieldDoc& doc, std::string_view name)
    {
        doc.beginGesture();
        for (int i = 0; i < doc.count(); ++i)
        {
            const FieldInfo& f = doc.info(i);
            if (f.section() == "boost" || f.section() == "intake")
                doc.set(i, f.def);
        }
        bool known = true;
        if (name == "na4-itb")
        {
            set(doc, "engine.layout", 5);
            set(doc, "engine.displacement", 0.002);
            set(doc, "boost.kind", 0);
            set(doc, "intake.throttleKind", 1);
            set(doc, "intake.trumpets", 1);
        }
        else if (name == "turbo4")
        {
            set(doc, "engine.layout", 5);
            set(doc, "engine.displacement", 0.002);
            set(doc, "boost.kind", 1);
            set(doc, "boost.vanes", 1);
            set(doc, "boost.gate", 1);
            set(doc, "boost.target", 120e3);
            set(doc, "boost.antiLag", 1);
            set(doc, "boost.nitrousShot", 60e3);
        }
        else if (name == "tt-v8")
        {
            set(doc, "engine.layout", 12);
            set(doc, "engine.displacement", 0.00576);
            set(doc, "boost.kind", 2);
            set(doc, "boost.target", 90e3);
        }
        else if (name == "sc-v8")
        {
            set(doc, "engine.layout", 12);
            set(doc, "engine.displacement", 0.0062);
            set(doc, "boost.kind", 4);
            set(doc, "boost.charger", 0);
            set(doc, "boost.icEffect", 0.6);
        }
        else if (name == "seq6")
        {
            set(doc, "engine.layout", 10);
            set(doc, "engine.displacement", 0.003);
            set(doc, "boost.kind", 2);
            set(doc, "boost.layout", 3);
            set(doc, "boost.blowOffKind", 2);
        }
        else if (name == "cmp6")
        {
            set(doc, "engine.layout", 10);
            set(doc, "engine.displacement", 0.003);
            set(doc, "boost.kind", 2);
            set(doc, "boost.layout", 4);
        }
        else if (name == "var-v8")   // a natural V8 on a variable manifold: long runners low, short ones above 5000 rpm
        {
            set(doc, "engine.layout", 13);
            set(doc, "engine.displacement", 0.0045);
            set(doc, "boost.kind", 0);
            set(doc, "intake.runnerLength", 0.45);
            set(doc, "intake.runnerLength2", 0.22);
            set(doc, "intake.runnerSwitch", 5000.0 * 0.10471976);
        }
        else if (name == "cf-v8")
        {
            set(doc, "engine.layout", 13);
            set(doc, "engine.displacement", 0.005);
            set(doc, "boost.kind", 4);
            set(doc, "boost.charger", 2);
        }
        else
            known = false;
        doc.endGesture();
        return known;
    }

    std::vector<std::pair<const char*, double>> airLiveValues(const AirBuild& b, double rpm, double load)
    {
        std::vector<std::pair<const char*, double>> out;
        const auto put = [&](const char* n, double v) { out.emplace_back(n, v); };
        rpm = std::max(600.0, rpm);
        load = std::clamp(load, 0.0, 1.0);
        const double amb = b.ambientP, tAmb = b.ambientT, nan = std::nan("");
        // the boost this build would make: a turbo spools with speed and load, a displacement blower follows speed
        double boost = 0.0;
        if (b.charger == Charger::turbo)
            boost = 110e3 * load * std::clamp((rpm - 1800.0) / 2600.0, 0.0, 1.0);
        else if (b.charger == Charger::supercharger)
            boost = (b.blower == Blower::centrifugal ? 75e3 * std::pow(rpm / b.redRpm, 2.1) : 60e3 * std::min(1.0, rpm / 2500.0)) * load;
        const double pr = (amb + boost + 8e3 * (boost > 0)) / amb;
        put("boost.pressure", boost);
        const double mani = b.charger == Charger::none ? amb * (0.3 + 0.68 * load) : amb + boost - 3e3;
        put("engine.manifold", mani);
        put("intake.manifold", mani);
        put("intake.box", -1500.0 * load * rpm / 6000.0);
        // the pulsing: a throttle a runner feels each cylinder (most at idle); a plenum calms it
        put("intake.ripple", b.itb ? 3e3 + 20e3 * (1.0 - load) : 1.2e3 + 1.5e3 * load);
        put("intake.runnerLength", b.runner2 > 0.0 && b.runnerSwitch > 0.0 && rpm * 0.10471976 > b.runnerSwitch ? b.runner2 : b.runner);
        const double t2 = tAmb * (1.0 + (std::pow(pr, 1.0 / 3.5) - 1.0) / 0.72);
        const double tIc = b.intercooler ? t2 - 0.7 * (t2 - tAmb) : t2;
        const bool forced = b.charger != Charger::none;
        put("boost.turbos", b.turbos);
        put("boost.chargePipe", forced ? pr * amb - amb : nan);
        put("boost.compressorOut[0]", forced ? t2 : nan);
        put("boost.compressorOut[1]", b.turbos > 1 ? t2 - 4.0 : nan);
        put("boost.intercoolerOut", b.intercooler ? tIc : nan);
        put("boost.throttleIn", forced ? tIc : 0.0);   // the simulator's 0 K with no machine in the path
        put("boost.blowOffValve", forced ? (load < 0.05 && boost > 5e3 ? 1.0 : 0.0) : nan);
        double flags = 0.0;
        if (b.hasMap())
        {
            // the flow the engine takes, a turbo's share, and the shaft's speed that makes this ratio on its own map
            const double share = b.turbos > 1 && b.arrangement != Arrangement::compound ? 1.0 / b.turbos : 1.0;
            const double mdot = b.displacement * rpm / 120.0 * 1.204 * pr * 0.88 * share * (0.25 + 0.75 * load);
            const double tau = std::pow(pr, 1.0 / 3.5) - 1.0;
            const double u = std::sqrt(std::max(tau, 0.0) * 1005.0 * 293.15 / (0.45 * 0.86));
            const double w = std::max(u / b.map.rTip, 0.12 * b.omegaMax);
            put("boost.shaftSpeed[0]", w);
            put("boost.shaftSpeed[1]", b.turbos > 1 ? w * 0.93 : nan);
            put("boost.ratio[0]", pr);
            put("boost.ratio[1]", b.turbos > 1 ? 1.0 + (pr - 1.0) * 0.9 : nan);
            put("boost.flow[0]", mdot);
            put("boost.flow[1]", b.turbos > 1 ? mdot * 0.9 : nan);
            // its efficiency there, from the same map (compressor.fm efficiency())
            const CompressorMap::Line line = b.map.at(static_cast<float>(w));
            put("boost.efficiency[0]", b.map.efficiency(line, static_cast<float>(mdot / std::max(1e-6f, line.mc))));
        }
        if (b.charger == Charger::turbo)
        {
            put("boost.gate[0]", std::clamp((boost - 0.8 * (b.target > 0 ? b.target : b.gateSpring)) / 30e3, 0.0, 1.0));
            put("boost.vanes[0]", b.vanes ? std::clamp(1.0 - load * (1.0 - rpm / b.redRpm), 0.2, 1.0) : 1.0);
            put("boost.drive[0]", 1.0 + 0.45 * pr * load);
        }
        if (b.charger == Charger::supercharger)
            put("boost.superchargerPower", 32e3 * load * rpm / 6000.0 + 2e3);
        if (b.nitrous && load > 0.9 && rpm > 3000.0)
            flags += 32.0;
        put("boost.flags", forced ? flags : nan);
        // card N2's mismatch: none in the demo's builds (all 0, as the simulator writes for a matched system)
        for (const char* n : { "boost.mismatch", "boost.coolerHas", "boost.coolerWants", "boost.springHas", "boost.springWants",
                               "boost.volumeHas", "boost.volumeWants", "boost.compressorHas", "boost.compressorWants",
                               "boost.turbineHas", "boost.turbineWants" })
            put(n, 0.0);
        return out;
    }

    void airDemoLive(DemoEngine& engine, const FieldDoc& doc)
    {
        const double load = get(engine, "engine.throttle", 0.2);
        const double rpm = get(engine, "engine.speed", 90.0) * 30.0 / 3.14159265358979;
        for (const auto& [name, v] : airLiveValues(resolveAir(doc), rpm, load))
            engine.set(name, v);
    }

    // ---- U1's demo hook (TEE_UI_DEMO in register.h) -----------------------------------------------------------------
    namespace
    {
        // The demo's fields read through the engine (it watches the document): enough of a FieldDoc for resolveAir.
        class EngineFields final : public FieldDoc
        {
        public:
            EngineFields(const DemoEngine& e, const std::vector<FieldInfo>& infos) : e_(e), infos_(infos) {}
            int count() const override { return static_cast<int>(infos_.size()); }
            const FieldInfo& info(int i) const override { return infos_[static_cast<std::size_t>(i)]; }
            double get(int i) const override { return e_.field(infos_[static_cast<std::size_t>(i)].path, infos_[static_cast<std::size_t>(i)].def); }
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
        const std::vector<FieldInfo>& hookInfos()
        {
            // the fields as the demo's document has them: U1's, then this package's that it lacks (as the hook adds them)
            static const std::vector<FieldInfo> infos = [] {
                std::vector<FieldInfo> v = demoFields(false);
                for (const FieldInfo& f : airDemoFields())
                    if ((f.section() == "boost" || f.section() == "intake")
                        && std::none_of(v.begin(), v.end(), [&](const FieldInfo& g) { return g.path == f.path; }))
                        v.push_back(f);
                return v;
            }();
            return infos;
        }
    }
}

namespace tee::ui
{
    void airDemo(DemoExtras& x)
    {
        if (x.fields != nullptr)
        {
            // the boost and intake fields the demo lacks (those it has, as it has them)
            for (const FieldInfo& f : air::airDemoFields())
                if ((f.section() == "boost" || f.section() == "intake")
                    && std::none_of(x.fields->begin(), x.fields->end(), [&](const FieldInfo& g) { return g.path == f.path; }))
                    x.fields->push_back(f);
        }
        if (x.engine != nullptr)
        {
            static const char* const kNames[] = {
                "boost.turbos", "boost.chargePipe", "boost.compressorOut[0]", "boost.compressorOut[1]",
                "boost.intercoolerOut", "boost.throttleIn", "intake.manifold", "intake.box", "intake.ripple", "intake.runnerLength",
                "boost.blowOffValve", "boost.shaftSpeed[0]", "boost.shaftSpeed[1]", "boost.ratio[0]", "boost.ratio[1]",
                "boost.flow[0]", "boost.flow[1]", "boost.efficiency[0]", "boost.gate[0]", "boost.vanes[0]",
                "boost.drive[0]", "boost.superchargerPower", "boost.flags",
            };
            for (const char* name : kNames)
                x.engine->addTelemetry(name, [name](const DemoEngine& e) {
                    const air::EngineFields doc(e, air::hookInfos());
                    for (const auto& [n, v] : air::airLiveValues(air::resolveAir(doc), e.rpm(), e.throttle()))
                        if (std::string_view(n) == name)
                            return v;
                    return std::nan("");
                });
        }
    }
}
