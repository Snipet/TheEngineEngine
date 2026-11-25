// tests/sim/combo/Combo.cpp: package X's harness (Combo.h).
#include "Combo.h"

#include "tee/Fields.h"
#include "tee/Sim.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <sstream>

namespace combo
{
    namespace
    {
        const tee::Fields* programFields()
        {
            std::string errors;
            // A plain pointer into tee::test's cache, which is never destroyed: no static here owns a Program.
            static const tee::Sim* sim = nullptr;
            if (sim == nullptr)
                sim = tee::test::compiled(tee::test::simDirectory(), tee::Sim::Options{}, errors).get();
            if (sim == nullptr)
            {
                std::fprintf(stderr, "combo: the simulator does not compile:\n%s\n", errors.c_str());
                return nullptr;
            }
            return &sim->fields();
        }

        std::string underSpec(const std::string& path)
        {
            return path.rfind("spec.", 0) == 0 ? path.substr(5) : path;
        }

        bool rng01(std::mt19937_64& g, double p)
        {
            return std::uniform_real_distribution<double>(0.0, 1.0)(g) < p;
        }
        double uni(std::mt19937_64& g, double lo, double hi)
        {
            return std::uniform_real_distribution<double>(lo, hi)(g);
        }
    } // namespace

    const std::vector<Switch>& switches()
    {
        static const std::vector<Switch> list = [] {
            std::vector<Switch> out;
            const tee::Fields* f = programFields();
            if (f == nullptr)
                return out;
            for (const tee::FieldInfo& fi : f->all())
            {
                if (fi.block != "spec" || fi.count != 1)
                    continue;
                const std::string p = underSpec(fi.path);
                const auto dot = p.find('.');
                if (dot == std::string::npos || p.find('.', dot + 1) != std::string::npos)
                    continue;   // only a section's own field
                const std::string name = p.substr(dot + 1);
                if (name == "model" && fi.maximum.value_or(0.0) >= 1.0)
                    out.push_back({p, 1});
                else if (p == "engine.custom" || p == "engine.wallHeat")
                    out.push_back({p, 1});   // the engine's own switches: D's custom engine, the walls' heat
                // engine.cycle (card T's two-stroke) is no switch over these builds: a two-stroke wants its own
                // (content/builds/ two-stroke builds, played by the corpus and T's tests)
            }
            return out;
        }();
        return list;
    }

    std::string comboName(std::uint32_t mask)
    {
        const auto& s = switches();
        if (mask == 0)
            return "none";
        const std::uint32_t all = s.size() >= 32 ? 0xffffffffu : ((1u << s.size()) - 1u);
        if (mask == all && s.size() > 2)
            return "all";
        std::string n;
        for (std::size_t i = 0; i < s.size(); ++i)
            if ((mask >> i) & 1u)
                n += (n.empty() ? "" : "+") + s[i].path;
        return n;
    }

    std::vector<std::uint32_t> subsetsOfSize(int k)
    {
        std::vector<std::uint32_t> out;
        const int n = static_cast<int>(switches().size());
        for (std::uint32_t m = 0; m < (1u << n); ++m)
        {
            int bits = 0;
            for (int i = 0; i < n; ++i)
                bits += static_cast<int>((m >> i) & 1u);
            if (bits == k)
                out.push_back(m);
        }
        return out;
    }

    const std::vector<Archetype>& archetypes()
    {
        // The idle bands: wide on purpose (a plausible idle, not a tuned one). Petrol 450 to 1800 rpm: the reference's
        // V8 idles fast for a few seconds after a start (1400 rpm at 0.5 s) and at -20 C (1590 rpm), and settles to
        // 1221 warm (c-host-tools.md). The diesel 350 to 1500 rpm (the reference's warm diesel idles at 1129).
        static const std::vector<Archetype> list = {
            {"small-four", 47.1, 188.5, false},
            {"v8", 47.1, 188.5, false},
            {"turbo-six", 47.1, 188.5, false},
            {"large-diesel", 36.6, 157.1, true},
            {"turbo-v8-dual", 47.1, 188.5, false},
        };
        return list;
    }

    const Archetype* archetype(const std::string& name)
    {
        for (const Archetype& a : archetypes())
            if (name == a.name)
                return &a;
        return nullptr;
    }

    std::string buildPath(const std::string& name)
    {
        return (tee::test::contentDirectory() / "builds" / "combo" / (name + ".json")).string();
    }

    const char* kindName(Kind k)
    {
        switch (k)
        {
        case Kind::Tour: return "tour";
        case Kind::Pull: return "pull";
        case Kind::Launch: return "launch";
        case Kind::Cold: return "cold";
        }
        return "?";
    }

    std::vector<Kind> allKinds()
    {
        return {Kind::Tour, Kind::Pull, Kind::Launch, Kind::Cold};
    }

    namespace
    {
        // One step of a timeline, so that a scenario is written once and both played (C++) and saved (JSON).
        struct Step
        {
            double t;
            std::string what;   // "ctl", "spec", "press"
            std::string name;
            double value;
            double ramp;
        };

        std::vector<Step> steps(Kind kind, std::uint32_t mask, Times& tm)
        {
            std::vector<Step> s;
            auto ctl = [&](double t, const char* n, double v, double ramp = 0.0) { s.push_back({t, "ctl", n, v, ramp}); };
            auto press = [&](double t, const char* n) { s.push_back({t, "press", n, 0.0, 0.0}); };
            s.push_back({0.0, "spec", "env.temperature", kind == Kind::Cold ? 253.15 : 293.15, 0.0});
            const auto& sw = switches();
            for (std::size_t i = 0; i < sw.size(); ++i)
                if ((mask >> i) & 1u)
                    s.push_back({0.0, "spec", sw[i].path, static_cast<double>(sw[i].on), 0.0});
            ctl(0.0, "mode", 1);
            ctl(0.0, "lever", 0);
            ctl(0.0, "clutch", 0);
            ctl(0.0, "throttle", 0);
            ctl(0.0, "brake", 0);
            ctl(0.0, "ignition", 1);
            if (kind == Kind::Cold)
                tm.started = 4.25;
            ctl(tm.start, "ignition", 2);
            ctl(tm.started, "ignition", 1);
            switch (kind)
            {
            case Kind::Tour:
                tm.idleFrom = 3.0;
                tm.idleTo = 5.0;
                ctl(5.0, "throttle", 1.0, 0.15);
                tm.revFrom = 6.0;
                tm.revTo = 8.5;
                ctl(8.5, "throttle", 0.0, 0.05);
                ctl(9.3, "throttle", 0.5, 0.1);   // a blip
                ctl(9.6, "throttle", 0.0, 0.05);
                ctl(11.0, "ignition", 0);
                tm.off = 11.0;
                tm.end = 14.0;
                break;
            case Kind::Pull:
                tm.idleFrom = 2.5;
                tm.idleTo = 3.0;
                press(3.0, "shiftUp");   // first, on the paddle (its own clutch pulls away)
                ctl(3.3, "throttle", 0.5, 0.2);
                ctl(4.5, "throttle", 1.0, 0.1);
                press(6.5, "shiftUp");
                press(8.5, "shiftUp");
                tm.driveFrom = 3.3;
                tm.driveTo = 10.5;
                ctl(10.5, "throttle", 0.0, 0.05);
                press(11.0, "shiftDown");
                ctl(12.0, "brake", 0.6, 0.2);
                ctl(13.5, "clutch", 1.0, 0.2);   // stopped: the clutch down before the key, as a driver does
                ctl(14.0, "ignition", 0);
                tm.off = 14.0;
                tm.end = 17.0;
                break;
            case Kind::Launch:
                tm.idleFrom = 2.5;
                tm.idleTo = 3.0;
                ctl(3.0, "brake", 1.0);
                press(3.0, "shiftUp");
                ctl(3.5, "throttle", 0.6, 0.3);   // held on the brake
                ctl(5.0, "brake", 0.0, 0.05);
                ctl(5.0, "throttle", 1.0, 0.1);
                press(7.5, "shiftUp");
                tm.driveFrom = 5.0;
                tm.driveTo = 9.0;
                ctl(9.0, "throttle", 0.0, 0.05);
                ctl(9.2, "brake", 0.8, 0.2);
                ctl(11.0, "clutch", 1.0, 0.2);
                ctl(11.5, "ignition", 0);
                tm.off = 11.5;
                tm.end = 14.5;
                break;
            case Kind::Cold:
                // cranked for up to 4 s, as a driver does at -20 C (the key is let go once it runs, by the ECU's
                // own starter logic, or held to 4.25 s)
                tm.idleFrom = 6.0;
                tm.idleTo = 9.0;
                ctl(10.0, "ignition", 0);
                tm.off = 10.0;
                tm.end = 13.0;
                break;
            }
            std::stable_sort(s.begin(), s.end(), [](const Step& a, const Step& b) { return a.t < b.t; });
            return s;
        }
    } // namespace

    tee::test::Scenario scenario(Kind kind, const std::string& build, std::uint32_t mask, Times& times,
                                 double shiftPhase)
    {
        times = Times{};
        tee::test::Scenario sc(build.find('/') != std::string::npos ? build : buildPath(build));
        for (const Step& st : steps(kind, mask, times))
        {
            // shiftPhase moves the upshifts after the pull-away (3 s) by a few ms: where a shift cut ends among the
            // firings sets its bang's size, so a case reads the worst over a firing interval
            const bool moved = st.what == "press" && st.name == "shiftUp" && st.t > 3.0;
            sc.at(moved ? st.t + shiftPhase : st.t);
            if (st.what == "ctl")
                sc.control(st.name, st.value);
            else if (st.what == "spec")
                sc.spec(st.name, st.value);
            else if (st.name == "shiftUp")
                sc.shiftUp();
            else
                sc.shiftDown();
            if (st.ramp > 0.0)
                sc.ramp(st.ramp);
        }
        return sc;
    }

    std::filesystem::path scenarioDirectory()
    {
        return tee::test::contentDirectory() / "scenarios" / "combo";
    }

    std::string scenarioJson(Kind kind, std::uint32_t mask)
    {
        Times tm;
        const auto s = steps(kind, mask, tm);
        std::ostringstream o;
        o << "{\n  \"format\": \"tee-scenario\",\n  \"version\": 1,\n  \"name\": \"" << kindName(kind)
          << "\",\n  \"description\": \"Package X's " << kindName(kind)
          << " (tests/sim/combo/Combo.cpp writes it: edit there, not here).\",\n  \"seconds\": " << tm.end
          << ",\n  \"events\": [\n";
        for (std::size_t i = 0; i < s.size(); ++i)
        {
            const Step& st = s[i];
            o << "    { \"t\": " << st.t << ", ";
            if (st.what == "press")
                o << "\"press\": [\"" << st.name << "\"]";
            else
                o << "\"" << st.what << "\": { \"" << st.name << "\": " << st.value << " }";
            if (st.ramp > 0.0)
                o << ", \"ramp\": " << st.ramp;
            o << " }" << (i + 1 < s.size() ? "," : "") << "\n";
        }
        o << "  ],\n  \"assertions\": [\n    { \"audio\": \"finite\" },\n    { \"audio\": \"peak\", \"<=\": 1 }\n  ]\n}\n";
        return o.str();
    }

    namespace
    {
        // Physical bounds by a telemetry field's quantity: generous (a sane machine is far inside), so that a value
        // outside is a model gone wrong, not a hot engine.
        bool telBounds(const tee::FieldInfo& f, double& lo, double& hi)
        {
            const std::string& q = f.quantity;
            if (q == "temperature")
            {
                lo = 150.0;    // colder than any air on earth
                hi = 3500.0;   // hotter than a flame's peak
            }
            else if (q == "rotation")
            {
                lo = -3200.0;  // a clutch's slip runs either way
                hi = 3200.0;   // 30 000 rpm: no layout's limit is near
            }
            else if (q == "fraction")
            {
                // "0 to 1" by the contract's word, but a volumetric efficiency is a fraction too and passes 1 when
                // tuned or boosted: 4 is far beyond any
                lo = -0.05;
                hi = 4.0;
            }
            else if (q == "pressure" && f.path.find("mep") != std::string::npos)
            {
                // a mean effective pressure from the brake torque carries the inertia's torque in a transient: the
                // engine dyno pulling the diesel up from idle reads -63 bar, a build swapped while running -21 bar
                lo = -1.0e7;
                hi = 1.0e7;
            }
            else if (q == "pressure")
            {
                lo = -2.0e6;   // gauge readings may be below 0; never this far
                hi = 4.0e7;    // 400 bar: a diesel's peak is 200
            }
            else if (q == "speed")
            {
                lo = -100.0;
                hi = 200.0;    // m/s
            }
            else
                return false;
            return true;
        }
    } // namespace

    void judgeTelemetry(const tee::test::Run& run, Verdict& v)
    {
        const tee::Fields* f = programFields();
        const tee::RunResult& r = run.result();
        std::string nonFinite;
        for (std::size_t c = 0; c < r.columns.size(); ++c)
        {
            const tee::FieldInfo* fi = f ? f->find("tel." + r.columns[c]) : nullptr;
            double lo = -1e300, hi = 1e300;
            if (fi != nullptr && fi->hasRange())
            {
                lo = *fi->minimum;
                hi = *fi->maximum;
            }
            else if (fi == nullptr || !telBounds(*fi, lo, hi))
            {
                lo = -1e300;
                hi = 1e300;
            }
            bool bad = false, out = false;
            double worst = 0.0, worstAt = 0.0, badAt = -1.0;
            for (std::size_t row = 0; row < r.rowCount(); ++row)
            {
                const double x = r.at(row, c);
                if (!std::isfinite(x))
                {
                    if (!bad)
                        badAt = r.rowTime[row];
                    bad = true;
                }
                else if (x < lo || x > hi)
                {
                    if (!out || std::fabs(x) > std::fabs(worst))
                    {
                        worst = x;
                        worstAt = r.rowTime[row];
                    }
                    out = true;
                }
            }
            if (bad)
            {
                char b[96];
                std::snprintf(b, sizeof b, "%s from %.2f s", r.columns[c].c_str(), badAt);
                nonFinite += (nonFinite.empty() ? "" : ", ") + std::string(b);
            }
            if (out)
            {
                // one failure a field, named "tel-range:<field>", so that a known finding names its fields alone
                char b[200];
                std::snprintf(b, sizeof b, "%s=%g at %.2f s (bounds %g to %g)", r.columns[c].c_str(), worst, worstAt,
                              lo, hi);
                v.failures.push_back({"tel-range:" + r.columns[c], b});
            }
        }
        if (!nonFinite.empty())
            v.failures.push_back({"tel-finite", nonFinite});
    }

    double largestStepTime(std::span<const float> x, double rate)
    {
        // tee::measure::largestStep's measure (Measures.cpp), giving where it is: seconds from the span's start
        const std::size_t n = x.size();
        if (n < 3)
            return 0.0;
        const std::size_t half = std::max<std::size_t>(8, static_cast<std::size_t>(0.01 * rate));
        std::vector<double> prefix(n, 0.0);
        for (std::size_t i = 1; i < n; ++i)
        {
            const double d = static_cast<double>(x[i]) - x[i - 1];
            prefix[i] = prefix[i - 1] + d * d;
        }
        double worst = 0.0;
        std::size_t at = 0;
        for (std::size_t i = 1; i < n; ++i)
        {
            const double d = static_cast<double>(x[i]) - x[i - 1];
            const std::size_t lo = i > half ? i - half : 1;
            const std::size_t hi = std::min(n - 1, i + half);
            const double around = prefix[hi] - prefix[lo - 1] - d * d;
            const double count = static_cast<double>(hi - lo + 1) - 1.0;
            const double local = count > 0.0 ? std::sqrt(std::max(around, 0.0) / count) : 0.0;
            const double r = std::fabs(d) / std::max(local, 1e-4);
            if (r > worst)
            {
                worst = r;
                at = i;
            }
        }
        return static_cast<double>(at) / rate;
    }

    double referenceStep(const std::string& build, Kind kind)
    {
        static std::map<std::pair<std::string, int>, double>* cache = new std::map<std::pair<std::string, int>, double>();
        const auto key = std::make_pair(build, static_cast<int>(kind));
        auto it = cache->find(key);
        if (it != cache->end())
            return it->second;
        Times tm;
        const tee::test::Run r = scenario(kind, build, 0u, tm).run(tm.end);
        const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
        const double step = std::max(r.left().during(from, to).largestStep(), r.right().during(from, to).largestStep());
        (*cache)[key] = step;
        return step;
    }

    namespace
    {
        double gCostOfR = 0.0;
    }

    double referenceStepWithPops(const std::string& build, Kind kind)
    {
        double worst = 0.0;
        for (double dT : {0.0, 0.01, 0.02})
        {
            Times tm;
            tee::test::Scenario sc = scenario(kind, build, 0u, tm);
            sc.at(0.0).spec("ecu.pops", 0.3).spec("env.temperature", 293.15 + dT);
            const tee::test::Run r = sc.run(tm.end);
            const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
            worst = std::max({worst, r.left().during(from, to).largestStep(), r.right().during(from, to).largestStep()});
        }
        return worst;
    }

    double costOfRIfMeasured()
    {
        return gCostOfR;
    }

    double costOfR()
    {
        double& cost = gCostOfR;
        if (cost > 0.0)
            return cost;
        std::vector<double> c;
        for (int i = 0; i < 3; ++i)
        {
            tee::test::Run r = tee::test::Scenario()
                                   .at(0.0).control("mode", 1).ignition(tee::On)
                                   .at(0.25).ignition(tee::Start)
                                   .at(2.0).ignition(tee::On)
                                   .at(3.0).throttle(1.0).ramp(0.2)
                                   .at(5.0).throttle(0.0)
                                   .run(6.0);
            if (r.result().simulatedSeconds > 0.0)
                c.push_back(r.result().wallSeconds / r.result().simulatedSeconds);
        }
        std::sort(c.begin(), c.end());
        cost = c.empty() ? 1.0 : c[c.size() / 2];
        return cost;
    }

    Verdict judge(const tee::test::Run& run, const Times& tm, const Archetype* a, bool expectRunning, double refStep)
    {
        Verdict v;
        char b[256];
        if (!run.ok())
        {
            v.failures.push_back({"run", run.error()});
            return v;
        }
        const tee::HostStats& h = run.host();
        if (h.guardTrips != 0 || h.stops != 0 || h.restarts != 0 || !run.stopReason().empty())
        {
            std::snprintf(b, sizeof b, "guard trips %u, stops %u, restarts %u: %s", h.guardTrips, h.stops, h.restarts,
                          run.stopReason().c_str());
            v.failures.push_back({"guard", b});
        }
        // the engine's own guard (D's ae5c99b): a crank not finite or past five times the redline is put at rest and
        // FLAG_FAULT (4) set in tel.engine.flags: a failure of the guard's kind
        {
            const auto flags = run.tel("engine.flags");
            for (std::size_t i = 0; i < flags.size(); ++i)
                if ((static_cast<int>(flags.values()[i]) & 4) != 0)
                {
                    std::snprintf(b, sizeof b, "the engine's guard put it at rest at %.2f s (FLAG_FAULT)",
                                  flags.times()[i]);
                    v.failures.push_back({"guard", b});
                    break;
                }
        }
        if (!run.left().finite() || !run.right().finite())
            v.failures.push_back({"finite", "a sample is not finite"});
        const double peak = std::max(run.left().peak(), run.right().peak());
        if (peak > 1.0)
        {
            std::snprintf(b, sizeof b, "peak %.3f over full scale", peak);
            v.failures.push_back({"peak", b});
        }

        // The running stretch: from half a second after the key is let go to the key off (or the end).
        const double from = tm.started + 0.5;
        const double to = tm.off > 0.0 ? tm.off : tm.end;
        // DC: the mean of the running stretch (at least 2 s) under 0.005 of full scale (-46 dBFS). A sound that has
        // a mean is a model's offset leaking to the output: inaudible as such, but it eats headroom and thumps at
        // every fade. (A 0.5 s window is no measure of it: an engine's rumble at 1 to 2 Hz moves such a mean.)
        const tee::test::Signal mid = run.audio();
        {
            const auto s = mid.during(from, to).samples();
            double m = 0.0;
            for (float x : s)
                m += x;
            v.dcMax = s.empty() ? 0.0 : std::fabs(m / static_cast<double>(s.size()));
        }
        if (v.dcMax > 0.005)
        {
            std::snprintf(b, sizeof b, "the running stretch's mean is %.4f of full scale (bound 0.005)", v.dcMax);
            v.failures.push_back({"dc", b});
        }
        // A click: the largest step over the local level (card C's measure), each channel, while running. Bound:
        // tee::test::clickLimit (12): a smooth sound stays under 4, a loud engine's firing reaches 6 to 8.
        // The reference's own sounds pass it (its turbos' and blow-off's reach 13 to 17 on the turbo builds), so the
        // bound is the larger of that and twice the reference's own largest step on the same build and scenario
        // (refStep; 0 when there is none to compare with). Twice, not less: a switch that moves the redline (the
        // custom engine's, by piston speed) moves where the limiter cuts, and the cut's steps with it (the turbo six
        // reaches 17 at 8100 rpm against the reference's 11 at 7100); a click proper is 30 to 100.
        v.stepMax = std::max(run.left().during(from, to).largestStep(), run.right().during(from, to).largestStep());
        const double clickBound = std::max(tee::test::clickLimit, 2.0 * refStep);
        if (v.stepMax > clickBound)
        {
            const auto l = run.left().during(from, to), r = run.right().during(from, to);
            const auto& ch = l.largestStep() >= r.largestStep() ? l : r;
            const double at = from + largestStepTime(ch.samples(), ch.rate());
            // The lead's ruling (23:15, through the steward): an upshift's spark cut under the reference's burn
            // (ecu.model 1, fuel.model 0) is an event of the engine: about 70 ms of near silence and the charge sent
            // to the pipe lights after it (D's measure: 15 to 29 times the local level by the cut's phase; the small
            // four's pull 33.6 after the steward's 0cb535ae). At an upshift's cut (car.flags 32, cut asked) and the
            // 0.15 s after it (with the gear rising), the bound is that step and a margin, 42, and the failure is "click-shift-cut", which
            // report() accepts for that combination only (Report.cpp, shiftCutRuled); over 42, or anywhere else, it
            // is a click.
            constexpr double kShiftCutBound = 42.0;
            // An upshift's cut: the car asks it (car.flags 32, car.model 1) or the ECU cuts the spark (ecu.flags 4,
            // with the classic car), in the 0.15 s before the step, while the gear rises from 0.3 s before it to 0.15 s after (so a
            // limiter's spark cut is not one)
            bool atShiftCut = false;
            {
                const double cutFrom = std::max(from, at - 0.15), cutTo = at + 0.005;
                const auto carFlags = run.tel("car.flags").during(cutFrom, cutTo);
                const auto ecuFlags = run.tel("ecu.flags").during(cutFrom, cutTo);
                bool cut = false;
                for (double x : carFlags.values())
                    cut = cut || (static_cast<int>(x) & 32) != 0;
                for (double x : ecuFlags.values())
                    cut = cut || (static_cast<int>(x) & 4) != 0;
                // the gear may change after a step early in the cut (the small four's at 8.518 s, the gear at about
                // 8.55): the rise is looked for up to 0.15 s after the step too
                const auto gear = run.tel("car.gear").during(std::max(from, at - 0.3), at + 0.15);
                const bool up = gear.size() > 1 && gear.values().back() > gear.values().front();
                atShiftCut = cut && up;
            }
            if (atShiftCut && v.stepMax <= kShiftCutBound)
            {
                std::snprintf(b, sizeof b,
                              "largest step %.1f times the local level at %.3f s, at an upshift's cut (the bound there "
                              "%.0f, the lead's ruling for ecu.model 1 under fuel.model 0; elsewhere %.1f)",
                              v.stepMax, at, kShiftCutBound, clickBound);
                v.failures.push_back({"click-shift-cut", b});
            }
            else
            {
                std::snprintf(b, sizeof b,
                              "largest step %.1f times the local level at %.3f s (bound %.1f; the reference's %.1f%s)",
                              v.stepMax, at, clickBound, refStep, atShiftCut ? "; at an upshift's cut, over 42" : "");
                v.failures.push_back({"click", b});
            }
        }

        judgeTelemetry(run, v);

        const auto state = run.tel("engine.state");
        const auto speed = run.tel("engine.speed");
        if (expectRunning)
        {
            const double ran = state.crossing(1.5);
            if (std::isnan(ran) || ran > tm.started + 0.5)
            {
                std::snprintf(b, sizeof b, "not running %.1f s after the key (state %.0f)", tm.started + 0.5 - tm.start,
                              state.at(tm.started + 0.5));
                v.failures.push_back({"start", b});
            }
            if (tm.idleFrom >= 0.0 && a != nullptr)
            {
                v.idle = speed.during(tm.idleFrom, tm.idleTo).mean();
                if (!(v.idle >= a->idleLow && v.idle <= a->idleHigh))
                {
                    std::snprintf(b, sizeof b, "idle %.0f rpm (bounds %.0f to %.0f)", v.idle * 30.0 / 3.14159265,
                                  a->idleLow * 30.0 / 3.14159265, a->idleHigh * 30.0 / 3.14159265);
                    v.failures.push_back({"idle", b});
                }
            }
            if (tm.revFrom >= 0.0)
            {
                v.revMax = speed.during(tm.revFrom, tm.revTo).max();
                const double red = run.tel("engine.redline").during(tm.revFrom, tm.revTo).max();
                if (!(v.revMax >= 0.85 * red))
                {
                    std::snprintf(b, sizeof b, "flat out in neutral it reached %.0f rpm, under 85 %% of its redline %.0f",
                                  v.revMax * 30.0 / 3.14159265, red * 30.0 / 3.14159265);
                    v.failures.push_back({"revs", b});
                }
            }
            if (tm.driveFrom >= 0.0)
            {
                const double vmax = run.tel("car.speed").during(tm.driveFrom, tm.driveTo).max();
                if (!(vmax > 2.0))   // it moves: a 4-tonne truck in first makes 5 m/s in 4 s
                {
                    std::snprintf(b, sizeof b, "in gear at full throttle the car reached %.1f m/s", vmax);
                    v.failures.push_back({"moves", b});
                }
                const double dead = state.during(tm.driveFrom, tm.driveTo).min();
                if (dead < 1.5)
                    v.failures.push_back({"stall", "it stopped running while driven"});
            }
            if (tm.off > 0.0 && tm.off + 3.0 <= tm.end + 1e-9)
            {
                // stopped: 3 s after the key off it no longer runs (state off, or running down: a heavy diesel's
                // flywheel still turns), and it has lost at least 40 % of its speed at the key
                const double st = state.at(tm.off + 3.0);
                const double w0 = std::fabs(speed.at(tm.off)), w = std::fabs(speed.at(tm.off + 3.0));
                if (st == 1.0 || st == 2.0 || w > 0.6 * w0 + 1.0)
                {
                    std::snprintf(b, sizeof b, "3 s after the key off: state %.0f, %.0f rpm (%.0f at the key)", st,
                                  w * 30.0 / 3.14159265, w0 * 30.0 / 3.14159265);
                    v.failures.push_back({"stop", b});
                }
            }
        }
        const double sim = run.result().simulatedSeconds;
        v.coreShare = sim > 0.0 ? run.result().wallSeconds / sim : 0.0;
        return v;
    }

    // ---- the known findings ----

    namespace
    {
#include "Known.h"

        bool matches(const char* pattern, const std::string& s)
        {
            return std::string(pattern) == "*" || s == pattern;
        }

        // A combination's pattern: "*", a comboName exactly, or "+a+b": every combination that has a and b on ("all"
        // and "load, all on" among them).
        bool comboMatches(const char* pattern, const std::string& c)
        {
            const std::string p = pattern;
            if (p.empty() || p[0] != '+')
                return matches(pattern, c);
            if (c == "all" || c.find("all on") != std::string::npos)
                return true;
            const std::string have = "+" + c + "+";
            std::size_t at = 1;
            while (at < p.size())
            {
                const auto next = p.find('+', at);
                const std::string one = p.substr(at, next == std::string::npos ? std::string::npos : next - at);
                if (have.find("+" + one + "+") == std::string::npos)
                    return false;
                if (next == std::string::npos)
                    break;
                at = next + 1;
            }
            return true;
        }
    } // namespace

    const Known* known(const std::string& c, const std::string& build, const std::string& scenario,
                       const std::string& check)
    {
        for (const Known& k : kKnown)
            if (k.id != nullptr && comboMatches(k.combo, c) && matches(k.build, build) && matches(k.scenario, scenario) &&
                matches(k.check, check))
                return &k;
        return nullptr;
    }

    // ---- fuzzing ----

    const std::vector<Field>& fuzzFields()
    {
        static const std::vector<Field> list = [] {
            std::vector<Field> out;
            const tee::Fields* f = programFields();
            if (f == nullptr)
                return out;
            for (const tee::FieldInfo& fi : f->all())
            {
                if (fi.block != "spec" || fi.hidden || fi.table() || fi.kind == tee::FieldKind::Rev)
                    continue;
                if (fi.scalar == tee::Scalar::None || fi.scalar == tee::Scalar::Char)
                    continue;
                Field x;
                x.integer = fi.scalar == tee::Scalar::Int || fi.scalar == tee::Scalar::Bool;
                if (fi.scalar == tee::Scalar::Bool)
                {
                    x.lo = 0.0;
                    x.hi = 1.0;
                }
                else if (fi.hasRange())
                {
                    x.lo = *fi.minimum;
                    x.hi = *fi.maximum;
                }
                else
                    continue;   // a number with no range: the contract's lint reports it, the fuzzer leaves it
                x.def = fi.fallback();
                x.log = fi.taper == "log" && x.lo > 0.0;
                x.choices = fi.scalar == tee::Scalar::Bool ? 2 : static_cast<int>(fi.choices.size());
                const std::string p = underSpec(fi.path);
                if (fi.count > 1)
                    for (std::size_t i = 0; i < fi.count; ++i)
                    {
                        x.path = p + "[" + std::to_string(i) + "]";
                        out.push_back(x);
                    }
                else
                {
                    x.path = p;
                    out.push_back(x);
                }
            }
            return out;
        }();
        return list;
    }

    std::vector<Value> randomSpec(std::uint64_t seed, bool corners)
    {
        std::mt19937_64 g(seed * 0x9E3779B97F4A7C15ull + 1);
        std::vector<Value> out;
        for (const Field& f : fuzzFields())
        {
            double v;
            if (corners)
                v = rng01(g, 0.5) ? f.lo : f.hi;
            else if (f.integer)
                v = std::floor(uni(g, f.lo, f.hi + 1.0 - 1e-9));
            else if (f.log)
                v = std::exp(uni(g, std::log(f.lo), std::log(f.hi)));
            else
                v = uni(g, f.lo, f.hi);
            out.push_back({f.path, std::clamp(v, f.lo, f.hi)});
        }
        return out;
    }

    tee::test::Scenario fuzzScenario(const std::vector<Value>& spec, std::uint64_t seed, double seconds, Times& tm)
    {
        tm = Times{};
        tm.idleFrom = -1.0;
        tm.end = tm.started + seconds;
        std::mt19937_64 g(seed ^ 0xD1B54A32D192ED03ull);
        tee::test::Scenario s;
        s.at(0.0);
        for (const Value& v : spec)
            s.spec(v.path, v.value);
        s.at(0.0).control("mode", 1).lever(0).clutch(0).ignition(tee::On);
        s.at(tm.start).ignition(tee::Start);
        s.at(tm.started).ignition(tee::On);
        for (double t = tm.started + 0.25; t < tm.end; t += 0.25)
        {
            s.at(t).throttle(rng01(g, 0.25) ? 0.0 : uni(g, 0.0, 1.0)).ramp(0.05);
            s.at(t).brake(rng01(g, 0.85) ? 0.0 : uni(g, 0.0, 1.0));
            s.at(t).clutch(rng01(g, 0.85) ? 0.0 : uni(g, 0.0, 1.0));
            const double r = uni(g, 0.0, 1.0);
            if (r < 0.06)
                s.at(t).shiftUp();
            else if (r < 0.10)
                s.at(t).shiftDown();
            else if (r < 0.12)
                s.at(t).lever(static_cast<int>(uni(g, -1.0, 4.0)));
        }
        return s;
    }

    Verdict judgeFuzz(const tee::test::Run& run, const Times& tm)
    {
        Verdict v = judge(run, tm, nullptr, false, 0.0);
        // the fuzz keeps only what any machine must do: finite, no guard, no blast, telemetry finite. A random build
        // may well click, idle oddly, or not start (a 1 cc single at -40 C): those are not findings by themselves.
        std::vector<Failure> keep;
        for (const Failure& f : v.failures)
            if (f.check == "run" || f.check == "guard" || f.check == "finite" || f.check == "peak" ||
                f.check == "tel-finite" || f.check == "dc")
                keep.push_back(f);
        v.failures = keep;
        // Silent while it says it runs: the whole last second running, and the output under -100 dBFS.
        const auto state = run.tel("engine.state").during(tm.end - 1.0, tm.end);
        if (!state.empty() && state.min() == 2.0)
        {
            const double db = run.audio().during(tm.end - 1.0, tm.end).level().db;
            if (db < -100.0)
            {
                char b[128];
                std::snprintf(b, sizeof b, "running (state 2) for the last second, the output at %.0f dBFS", db);
                v.failures.push_back({"silent", b});
            }
        }
        // Far over budget: four times the reference's cost (the heaviest v1 build is 1.7 R: d-mechanics.md).
        const double r = costOfR();
        v.costR = r > 0.0 ? v.coreShare / r : 0.0;
        if (v.costR > 4.0)
        {
            char b[128];
            std::snprintf(b, sizeof b, "cost %.1f R", v.costR);
            v.failures.push_back({"cost", b});
        }
        return v;
    }

    std::string buildJson(const std::vector<Value>& spec, const std::string& name, const std::string& notes)
    {
        // section -> field -> values (an array's elements by index; the rest of an array at its default)
        std::map<std::string, std::map<std::string, std::map<int, double>>> tree;
        std::map<std::string, double> defaults;
        for (const Field& f : fuzzFields())
            defaults[f.path] = f.def;
        for (const Value& v : spec)
        {
            const auto dot = v.path.find('.');
            const std::string sec = v.path.substr(0, dot);
            std::string field = v.path.substr(dot + 1);
            int index = -1;
            const auto br = field.find('[');
            if (br != std::string::npos)
            {
                index = std::stoi(field.substr(br + 1));
                field = field.substr(0, br);
            }
            tree[sec][field][index] = v.value;
        }
        std::ostringstream o;
        o.precision(9);
        o << "{\n  \"format\": \"tee-build\",\n  \"version\": 1,\n  \"name\": \"" << name << "\",\n  \"notes\": \""
          << notes << "\",\n  \"spec\": {\n";
        bool firstSec = true;
        for (const auto& [sec, fields] : tree)
        {
            o << (firstSec ? "" : ",\n") << "    \"" << sec << "\": {";
            firstSec = false;
            bool first = true;
            for (const auto& [field, values] : fields)
            {
                o << (first ? "\n" : ",\n") << "      \"" << field << "\": ";
                first = false;
                if (values.count(-1))
                    o << values.at(-1);
                else
                {
                    const int last = values.rbegin()->first;
                    o << "[";
                    for (int i = 0; i <= last; ++i)
                    {
                        const std::string p = sec + "." + field + "[" + std::to_string(i) + "]";
                        o << (i ? ", " : "") << (values.count(i) ? values.at(i) : defaults[p]);
                    }
                    o << "]";
                }
            }
            o << "\n    }";
        }
        o << "\n  }\n}\n";
        return o.str();
    }

    // c holds a switch: "all", or one of its '+'-joined names
    bool holds(const std::string& c, const std::string& sw)
    {
        if (c == "all")
            return true;
        std::size_t at = 0;
        while (at <= c.size())
        {
            const std::size_t plus = c.find('+', at);
            const std::size_t end = plus == std::string::npos ? c.size() : plus;
            if (c.compare(at, end - at, sw) == 0 && end - at == sw.size())
                return true;
            at = end + 1;
        }
        return false;
    }

    // The lead's ruling (23:15): a step at an upshift's spark cut, within 42, is the engine's own event for
    // ecu.model 1 under the reference's burn (fuel.model 0); with fuel.model 1 the steward's 0cb535ae cures it
    bool shiftCutRuled(const std::string& c, const Failure& f)
    {
        return f.check == "click-shift-cut" && holds(c, "ecu.model") && !holds(c, "fuel.model");
    }
} // namespace combo
