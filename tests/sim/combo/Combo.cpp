// tests/sim/combo/Combo.cpp: package X's harness (Combo.h).
#include "Combo.h"

#include "tee/Fields.h"
#include "tee/Sim.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <map>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <vector>

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
            // the turbo six's fair twin, a mild cam (its dial and the lab's cams stated equal; the lead's ruling, 26 Nov
            // 11:46): the lab's default-length cam judged model against model. Every loop over this list is nightly.
            {"turbo-six-mild", 47.1, 188.5, false},
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

        // drive: the build's box is an automatic or a CVT, whose gear the lever alone sets (R, N, D: gearbox.fm), not the
        // shift counters: the pull and the launch put it in D as they pull away and back in N as the car stops, as a driver
        // does (the owner's report, the lead's ruling 27 Nov 11:37: with the lever in N those boxes ran the pull in
        // neutral). A manual's lever is its H-pattern (1 is first), so no other box's lever is moved.
        std::vector<Step> steps(Kind kind, std::uint32_t mask, Times& tm, Driver driver)
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
                if (driver == Driver::Lever)
                    ctl(3.0, "lever", 1);
                if (driver == Driver::Truck)
                {
                    // the eighteen-speed's pull-away in one gear (the lead's ruling 3(b), 27 Nov 12:07): the clutch
                    // floored at rest (the clutch brake stops the input shaft), the lever to the L slot (gear 1:
                    // roadranger.fm), the clutch let up over truckClutchRamp() as the throttle opens; no shifts (a
                    // shift wants a float or a double clutch: a driver, not a timeline; the presses do nothing to it)
                    ctl(2.5, "clutch", 1.0, 0.2);
                    ctl(2.8, "lever", truckSlot());
                    ctl(3.4, "clutch", 0.0, truckClutchRamp());
                }
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
                if (driver != Driver::Presses)
                    ctl(13.5, "lever", 0);
                ctl(14.0, "ignition", 0);
                tm.off = 14.0;
                tm.end = 17.0;
                break;
            case Kind::Launch:
                tm.idleFrom = 2.5;
                tm.idleTo = 3.0;
                ctl(3.0, "brake", 1.0);
                press(3.0, "shiftUp");
                if (driver == Driver::Lever)
                    ctl(3.0, "lever", 1);
                ctl(3.5, "throttle", 0.6, 0.3);   // held on the brake
                ctl(5.0, "brake", 0.0, 0.05);
                ctl(5.0, "throttle", 1.0, 0.1);
                press(7.5, "shiftUp");
                tm.driveFrom = 5.0;
                tm.driveTo = 9.0;
                ctl(9.0, "throttle", 0.0, 0.05);
                ctl(9.2, "brake", 0.8, 0.2);
                ctl(11.0, "clutch", 1.0, 0.2);
                if (driver == Driver::Lever)
                    ctl(11.0, "lever", 0);
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

    namespace
    {
        tee::test::Scenario scenarioOn(Kind kind, const std::string& build, std::uint32_t mask, Times& times,
                                       double shiftPhase, bool reference);
    }

    tee::test::Scenario scenario(Kind kind, const std::string& build, std::uint32_t mask, Times& times,
                                 double shiftPhase)
    {
        return scenarioOn(kind, build, mask, times, shiftPhase, false);
    }

    tee::test::Scenario referenceScenario(Kind kind, const std::string& build, Times& times)
    {
        return scenarioOn(kind, build, 0u, times, 0.0, true);
    }

    void requireReferenceRuns(const tee::test::Run& run, const Times& times, const std::string& what)
    {
        if (times.driveFrom < 0.0)
            return;
        const double low = run.tel("engine.state").during(times.driveFrom, times.driveTo).min();
        if (low < 1.5)
        {
            char b[160];
            std::snprintf(b, sizeof b, "the reference stalled while driven (engine.state %.0f between %.1f and %.1f s): ",
                          low, times.driveFrom, times.driveTo);
            throw std::runtime_error(b + what);
        }
    }

    namespace
    {
    tee::test::Scenario scenarioOn(Kind kind, const std::string& build, std::uint32_t mask, Times& times,
                                   double shiftPhase, bool reference)
    {
        times = Times{};
        const std::string file = build.find('/') != std::string::npos ? build : buildPath(build);
        tee::test::Scenario sc(file);
        for (const Step& st : steps(kind, mask, times, reference ? Driver::Presses : driverFor(file)))
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
    } // namespace

    std::filesystem::path scenarioDirectory()
    {
        return tee::test::contentDirectory() / "scenarios" / "combo";
    }

    double truckClutchRamp()
    {
        // COMBO_TRUCK_CLUTCH_RAMP: a measurement's override (the ramp is picked by 17 nudges without a stall)
        if (const char* r = std::getenv("COMBO_TRUCK_CLUTCH_RAMP"); r != nullptr)
            return std::atof(r);
        return kTruckClutchRamp;
    }

    int truckSlot()
    {
        // COMBO_TRUCK_SLOT: a measurement's override (5 the L slot, gear 1; 1 the low range's first position, gear 3)
        if (const char* r = std::getenv("COMBO_TRUCK_SLOT"); r != nullptr)
            return std::atoi(r);
        return kTruckSlot;
    }

    namespace
    {
        // the build's "gearbox" as written (the tee-build format's spec.car.gearbox, by its choice's name); empty if none
        std::string gearboxName(const std::string& buildFile)
        {
            std::ifstream in(buildFile, std::ios::binary);
            const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            static const std::regex box("\"gearbox\"\\s*:\\s*\"([^\"]*)\"");
            std::smatch m;
            return std::regex_search(text, m, box) ? m[1].str() : std::string();
        }
    }

    Driver driverFor(const std::string& buildFile)
    {
        const std::string box = gearboxName(buildFile);
        if (box == "Automatic" || box == "CVT")
            return Driver::Lever;
        return box == "Eighteen-speed" ? Driver::Truck : Driver::Presses;
    }

    bool leverDrives(const std::string& buildFile)
    {
        return driverFor(buildFile) == Driver::Lever;
    }

    std::string scenarioJson(Kind kind, std::uint32_t mask, Driver driver)
    {
        Times tm;
        const auto s = steps(kind, mask, tm, driver);
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

    // The reference's step and excess (judge's bounds) from one run of the reference: a case that asks for both, as every
    // judged case does, plays the reference once per process (the fast tier's grouping, 26 Nov)
    struct Reference
    {
        double step = 0.0;
        double excess = -1.0;
    };
    const Reference& reference(const std::string& build, Kind kind);

    double referenceStep(const std::string& build, Kind kind)
    {
        return reference(build, kind).step;
    }

    // The excess of the step at `at` on one channel over its neighbouring firing periods, the neighbours inside
    // [from, to] (the running stretch)
    double excessAt(const tee::test::Run& run, bool left, double at, double from, double to, double* periodOut)
    {
        const double hz = run.firingHz(std::max(0.0, at - 0.25), at + 0.25);
        if (!(hz > 1.0))
            return -1.0;
        const double T = 1.0 / hz;
        if (periodOut != nullptr)
            *periodOut = T;
        const auto whole = left ? run.left() : run.right();
        auto stepIn = [&](double centre) {
            const double a = centre - 0.5 * T, b = centre + 0.5 * T;
            if (a < from || b > to)
                return -1.0;
            return whole.during(a, b).largestStep();
        };
        const double own = stepIn(at);
        std::vector<double> around;
        for (int k = 1; k <= 4; ++k)
            for (double c : {at - k * T, at + k * T})
                if (const double x = stepIn(c); x > 0.0)
                    around.push_back(x);
        if (around.size() < 4 || !(own > 0.0))
            return -1.0;
        std::sort(around.begin(), around.end());
        const std::size_t m = around.size() / 2;
        const double median = around.size() % 2 != 0 ? around[m] : 0.5 * (around[m - 1] + around[m]);
        return own / std::max(median, 1e-9);
    }

    double stepExcess(const tee::test::Run& run, double from, double to, double* atOut, double* periodOut)
    {
        const auto l = run.left().during(from, to), r = run.right().during(from, to);
        const bool left = l.largestStep() >= r.largestStep();
        const auto& ch = left ? l : r;
        const double at = from + largestStepTime(ch.samples(), ch.rate());
        if (atOut != nullptr)
            *atOut = at;
        return excessAt(run, left, at, from, to, periodOut);
    }

    std::vector<float> stepProfile(std::span<const float> x, double rate)
    {
        // largestStepTime's measure at every sample (the step over its local level), so a window picks its worst
        // sample from the run's own figures and its edges move no figure
        const std::size_t n = x.size();
        std::vector<float> out(n, 0.0f);
        if (n < 3)
            return out;
        const std::size_t half = std::max<std::size_t>(8, static_cast<std::size_t>(0.01 * rate));
        std::vector<double> prefix(n, 0.0);
        for (std::size_t i = 1; i < n; ++i)
        {
            const double d = static_cast<double>(x[i]) - x[i - 1];
            prefix[i] = prefix[i - 1] + d * d;
        }
        for (std::size_t i = 1; i < n; ++i)
        {
            const double d = static_cast<double>(x[i]) - x[i - 1];
            const std::size_t lo = i > half ? i - half : 1;
            const std::size_t hi = std::min(n - 1, i + half);
            const double around = prefix[hi] - prefix[lo - 1] - d * d;
            const double count = static_cast<double>(hi - lo + 1) - 1.0;
            const double local = count > 0.0 ? std::sqrt(std::max(around, 0.0) / count) : 0.0;
            out[i] = static_cast<float>(std::fabs(d) / std::max(local, 1e-4));
        }
        return out;
    }

    std::vector<Event> events(const tee::test::Run& run, const Times& tm, double from, double to)
    {
        // The run's events from what the scenario and the telemetry report (the lead's ruling, 26 Nov 08:07, part B):
        // each shift (each gear change, 0.3 s either side; and car.flags 16 shifting or 32 cut asked, from its rise to
        // 0.3 s after its fall; named by the gear before and after), each ECU cut outside a shift (ecu.flags 2 fuel, 4 spark, 8 overrun; the reference's
        // engine.flags 2), each limiter episode (ecu.flags 1, engine.flags 1), each lift (the throttle falling
        // through 0.1: 0.05 s before to 0.5 s after); the start (to 2 s after the key is let go) first of all; a
        // sample in two windows is the first's in that order (start, flip, shift, limiter, cut, lift); the rest of the running stretch is "the rest"
        std::vector<Event> out;
        if (tm.started + 2.0 > from)
            out.push_back({"start", from, std::min(to, tm.started + 2.0)});
        // a switch thrown while running (the scenario's flip times): 0.3 s from each (the lead's ruling, 26 Nov 11:11)
        for (double t : tm.flips)
            if (t + 0.3 > from && t < to)
                out.push_back({"flip", std::max(from, t), std::min(to, t + 0.3)});
        auto episodes = [&](const char* name, std::initializer_list<std::pair<const char*, int>> bits, double after) {
            std::vector<tee::test::Series> s;
            for (const auto& [path, bit] : bits)
                s.push_back(run.tel(path));
            if (s.empty() || s[0].empty())
                return;
            const auto& t = s[0].times();
            bool on = false;
            double rise = 0.0;
            for (std::size_t i = 0; i <= t.size(); ++i)
            {
                bool now = false;
                if (i < t.size())
                {
                    std::size_t k = 0;
                    for (const auto& [path, bit] : bits)
                    {
                        const auto& vals = s[k++].values();
                        if (i < vals.size() && (static_cast<int>(vals[i]) & bit) != 0)
                            now = true;
                    }
                }
                if (now && !on)
                    rise = t[i];
                if (!now && on)
                {
                    const double fall = i < t.size() ? t[i] : to;
                    Event e{name, std::max(from, rise - 0.05), std::min(to, fall + after)};
                    if (std::string(name) == "shift")
                    {
                        const auto gear = run.tel("car.gear");
                        char g[48];
                        std::snprintf(g, sizeof g, "shift %.0f-%.0f", gear.at(rise - 0.05), gear.at(fall + after));
                        e.name = g;
                    }
                    if (e.to > e.from)
                        out.push_back(e);
                }
                on = now;
            }
        };
        {
            // each gear change is a shift (the classic car raises no car.flags: the ECU cuts the spark while the gear
            // rises): 0.3 s either side of it, named by the gear before and after
            const auto gear = run.tel("car.gear");
            const auto& t = gear.times();
            const auto& x = gear.values();
            for (std::size_t i = 1; i < t.size(); ++i)
                if (x[i] != x[i - 1])
                {
                    char g[48];
                    std::snprintf(g, sizeof g, "shift %.0f-%.0f", x[i - 1], x[i]);
                    Event e{g, std::max(from, t[i] - 0.3), std::min(to, t[i] + 0.3)};
                    if (e.to > e.from)
                        out.push_back(e);
                }
        }
        episodes("shift", {{"car.flags", 16 | 32}}, 0.3);
        episodes("limiter", {{"ecu.flags", 1}, {"engine.flags", 1}}, 0.3);
        episodes("cut", {{"ecu.flags", 2 | 4 | 8}, {"engine.flags", 2}}, 0.3);
        {
            const auto th = run.tel("engine.throttle");
            const auto& t = th.times();
            const auto& x = th.values();
            for (std::size_t i = 1; i < t.size(); ++i)
                if (x[i - 1] >= 0.1 && x[i] < 0.1)
                {
                    Event e{"lift", std::max(from, t[i] - 0.05), std::min(to, t[i] + 0.5)};
                    if (e.to > e.from)
                        out.push_back(e);
                }
        }
        out.push_back({"the rest", from, to});
        return out;
    }

    double lowestExcess(const std::vector<double>& excess)
    {
        double lowest = -1.0;
        for (double e : excess)
            if (e >= 0.0 && (lowest < 0.0 || e < lowest))
                lowest = e;
        return lowest;
    }

    namespace
    {
        bool gNightlyCase = false;
    } // namespace

    void setNightlyCase(bool nightly)
    {
        gNightlyCase = nightly;
    }

    bool nightlyCase()
    {
        return gNightlyCase;
    }

    // ---- the reference copies kept across processes (the lead's ruling, 27 Nov 03:44) ----
    // CI runs each Catch case as its own process, and each judged its references afresh: the five copies behind every
    // nightly bound were played again in every entry (360 reference sets in one soak for 57 distinct). A set is kept in
    // a file named by a hash of everything that decides it: the test executable's bytes (any rebuild is a new key),
    // every file of the sim tree and of the content tree in use, the build's name, the scenario (its kind and its
    // timeline as tee-scenario JSON), the nudges, and whether copies are taken. The values are written exactly (hex
    // floats), so a kept set reads the same figures to the digit; a missing, partial or unreadable file is a miss and
    // the set is played. Written to a temporary name and renamed, so a reader never sees half a file.
    namespace refcache
    {
        std::uint64_t mix(std::uint64_t h, const void* data, std::size_t n)
        {
            const auto* p = static_cast<const unsigned char*>(data);
            for (std::size_t i = 0; i < n; ++i)
            {
                h ^= p[i];
                h *= 1099511628211ull;   // FNV-1a, 64 bits
            }
            return h;
        }
        std::uint64_t mixString(std::uint64_t h, const std::string& s)
        {
            const std::uint64_t n = s.size();
            h = mix(h, &n, sizeof n);
            return mix(h, s.data(), s.size());
        }
        // every regular file under dir, by its path relative to dir, in order; 0 when dir is not a directory
        std::uint64_t treeHash(const std::filesystem::path& dir)
        {
            std::error_code ec;
            if (!std::filesystem::is_directory(dir, ec))
                return 0;
            std::vector<std::filesystem::path> files;
            for (auto it = std::filesystem::recursive_directory_iterator(dir, ec);
                 !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
                if (it->is_regular_file(ec))
                    files.push_back(it->path());
            if (ec)
                return 0;
            std::sort(files.begin(), files.end());
            std::uint64_t h = 14695981039346656037ull;
            std::vector<char> buf;
            for (const auto& f : files)
            {
                h = mixString(h, std::filesystem::relative(f, dir, ec).generic_string());
                std::ifstream in(f, std::ios::binary);
                buf.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
                h = mixString(h, std::string(buf.begin(), buf.end()));
            }
            return h;
        }
        std::uint64_t fileHash(const std::filesystem::path& f)
        {
            std::ifstream in(f, std::ios::binary);
            if (!in)
                return 0;
            std::uint64_t h = 14695981039346656037ull;
            char buf[1 << 16];
            while (in.read(buf, sizeof buf) || in.gcount() > 0)
                h = mix(h, buf, static_cast<std::size_t>(in.gcount()));
            return h;
        }
        // the directory the sets are kept in, or empty: none (TEE_COMBO_REFERENCES=off, or no executable to key on)
        const std::filesystem::path& directory()
        {
            static const std::filesystem::path dir = [] {
                const char* e = std::getenv("TEE_COMBO_REFERENCES");
                if (e != nullptr && std::strcmp(e, "off") == 0)
                    return std::filesystem::path();
                if (e != nullptr && *e != '\0')
                    return std::filesystem::path(e);
#ifdef TEE_COMBO_REFERENCE_DIR
                return std::filesystem::path(TEE_COMBO_REFERENCE_DIR);
#else
                return std::filesystem::path();
#endif
            }();
            return dir;
        }
        // the hash of what every set of this process shares (the executable, the trees); 0: keep nothing
        std::uint64_t processKey()
        {
            static const std::uint64_t key = [] {
                std::error_code ec;
                const auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
                if (ec)
                    return std::uint64_t(0);   // no executable to key on (not Linux): every set is played
                const std::uint64_t e = fileHash(exe), sim = treeHash(tee::test::simDirectory()),
                                    content = treeHash(tee::test::contentDirectory());
                if (e == 0 || sim == 0 || content == 0)
                    return std::uint64_t(0);
                std::uint64_t h = 14695981039346656037ull;
                for (std::uint64_t x : {e, sim, content})
                    h = mix(h, &x, sizeof x);
                return h;
            }();
            return key;
        }
    } // namespace refcache

    std::filesystem::path referenceDirectory()
    {
        return refcache::directory();
    }

    std::uint64_t referenceTreeHash(const std::filesystem::path& dir)
    {
        return refcache::treeHash(dir);
    }

    std::string referenceKey(const std::string& build, Kind kind, bool copies)
    {
        const std::uint64_t p = refcache::processKey();
        if (p == 0 || refcache::directory().empty())
            return std::string();
        std::uint64_t h = p;
        h = refcache::mixString(h, build);
        h = refcache::mixString(h, kindName(kind));
        h = refcache::mixString(h, scenarioJson(kind, 0u));   // a reference never moves the lever (referenceScenario)
        for (double d : kBoundNudges)
            h = refcache::mix(h, &d, sizeof d);
        const int c = copies ? 1 : 0;
        h = refcache::mix(h, &c, sizeof c);
        char name[32];
        std::snprintf(name, sizeof name, "%016llx", static_cast<unsigned long long>(h));
        return name;
    }

    bool readKeptReference(const std::string& key, double& step, std::vector<double>& excess)
    {
        if (key.empty())
            return false;
        std::ifstream in(refcache::directory() / (key + ".ref"));
        std::string tag;
        std::size_t n = 0;
        if (!(in >> tag >> n) || tag != "tee-combo-reference-1" || n == 0 || n > 16)
            return false;
        std::string word;
        if (!(in >> word))
            return false;
        step = std::strtod(word.c_str(), nullptr);
        excess.clear();
        for (std::size_t i = 0; i < n; ++i)
        {
            if (!(in >> word))
                return false;
            excess.push_back(std::strtod(word.c_str(), nullptr));
        }
        return (in >> word) && word == "end";
    }

    void keepReference(const std::string& key, double step, const std::vector<double>& excess)
    {
        if (key.empty())
            return;
        std::error_code ec;
        std::filesystem::create_directories(refcache::directory(), ec);
        const auto final = refcache::directory() / (key + ".ref");
        auto tmp = final;
        tmp += ".tmp." + std::to_string(std::random_device{}()) + std::to_string(std::random_device{}());   // a name of its own
        {
            std::ofstream out(tmp);
            if (!out)
                return;
            char b[64];
            out << "tee-combo-reference-1 " << excess.size();
            std::snprintf(b, sizeof b, " %a", step);
            out << b;
            for (double e : excess)
            {
                std::snprintf(b, sizeof b, " %a", e);
                out << b;
            }
            out << " end\n";
            if (!out)
                return;
        }
        std::filesystem::rename(tmp, final, ec);
        if (ec)
            std::filesystem::remove(tmp, ec);
    }

    const Reference& reference(const std::string& build, Kind kind)
    {
        // (the default tier's single draw and the nightly's copies kept apart)
        static std::map<std::tuple<std::string, int, bool>, Reference>* cache =
            new std::map<std::tuple<std::string, int, bool>, Reference>();
        const bool copies = nightlyCase();
        const auto key = std::make_tuple(build, static_cast<int>(kind), copies);
        if (auto it = cache->find(key); it != cache->end())
            return it->second;
        Reference ref;
        std::vector<double> excess;
        const std::string kept = referenceKey(build, kind, copies);
        const bool fromFile = readKeptReference(kept, ref.step, excess);
        for (double dT : kBoundNudges)
        {
            if (fromFile)
                break;
            Times tm;
            tee::test::Scenario sc = referenceScenario(kind, build, tm);
            if (dT != 0.0)
                sc.at(0.0).spec("env.temperature", 293.15 + dT);
            const tee::test::Run r = sc.run(tm.end);
            requireReferenceRuns(r, tm, build + " " + kindName(kind));
            const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
            if (dT == 0.0)   // the step's bound stays the single draw's (the ruling is the excess's)
                ref.step = std::max(r.left().during(from, to).largestStep(), r.right().during(from, to).largestStep());
            excess.push_back(stepExcess(r, from, to));
            if (!copies)
                break;
        }
        if (!fromFile)
            keepReference(kept, ref.step, excess);
        ref.excess = lowestExcess(excess);
        if (copies)
        {
            std::printf("reference | %-13s | %-6s | excess over its copies (dT 0, -0.07, -0.03, +0.03, +0.07 K)%s:", build.c_str(),
                        kindName(kind), fromFile ? " (kept)" : "");
            for (double e : excess)
                std::printf(" %.2f", e);
            std::printf("; the bound twice the lowest, %.2f\n", 2.0 * ref.excess);
        }
        return (*cache)[key] = ref;
    }

    double referenceExcess(const std::string& build, Kind kind)
    {
        return reference(build, kind).excess;
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
            tee::test::Scenario sc = referenceScenario(kind, build, tm);
            sc.at(0.0).spec("ecu.pops", 0.3).spec("env.temperature", 293.15 + dT);
            const tee::test::Run r = sc.run(tm.end);
            requireReferenceRuns(r, tm, build + " " + kindName(kind) + " (pops)");
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

    Verdict judge(const tee::test::Run& run, const Times& tm, const Archetype* a, bool expectRunning, double refStep,
                  double refExcess)
    {
        Verdict v;
        char b[512];
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
        v.stepExcess = stepExcess(run, from, to, &v.stepAt, &v.firingPeriod);
        const double clickBound = std::max(tee::test::clickLimit, 2.0 * refStep);
        // The lead's ruling (25 Nov 17:24): a click is a step its neighbouring firing periods do not have. Where the
        // engine fires and a period is defined (stepExcess >= 0) and the caller gives the reference's own excess, the
        // excess decides, against twice the reference's on the same build and scenario; where no period exists
        // (stopped, cranking before the first firing, the starter) or no reference excess is given, the step over
        // the local level decides as before. Both are printed in a click's detail for the next two windows. This
        // restates the check on its property; it is not a loosened bound: it still sees the cured clicks on their
        // old tree (X-015: 7.33 against 3.19 on the pull, 4.07 against 3.64 on the tour, its weakest proof; X-014:
        // 7.77 against 4.60) and clears the diesel's firing trains (X-033, X-034, X-035: 1.17 to 2.59 against 3.19).
        v.excessBound = refExcess >= 0.0 ? 2.0 * refExcess : -1.0;
        // Per event (the lead's ruling, 26 Nov 08:07, part B): each event window of the run (events(): each shift,
        // cut, limiter episode, lift, the start, the rest) is judged at its own worst step, so a row is "this event:
        // n of N" and never the run's one worst. The window's edges move no figure: a window only picks its worst
        // sample, from the run's own step profile (the step over its local level, stepProfile, computed on the whole
        // running stretch), and that step's excess is taken over neighbouring firing periods anywhere in the running
        // stretch, never clipped to the window (the steward read 2.64 in a window against 2.50 over the run, k = -4,
        // when edges clipped). A sample in two windows is judged in the first (start, flip, shift, limiter, cut, lift): a limiter's cuts are
        // the limiter's.
        const auto lch = run.left().during(from, to), rch = run.right().during(from, to);
        const std::vector<float> lp = stepProfile(lch.samples(), lch.rate());
        const std::vector<float> rp = stepProfile(rch.samples(), rch.rate());
        const double rate = lch.rate();
        const std::vector<Event> evs = events(run, tm, from, to);
        std::vector<int> owner(lp.size(), -1);
        for (std::size_t e = 0; e < evs.size(); ++e)
        {
            const auto i0 = static_cast<std::size_t>(std::max(0.0, (evs[e].from - from) * rate));
            const auto i1 = std::min(lp.size(), static_cast<std::size_t>(std::max(0.0, (evs[e].to - from) * rate)));
            for (std::size_t k = i0; k < i1; ++k)
                if (owner[k] < 0)
                    owner[k] = static_cast<int>(e);
        }
        std::vector<std::size_t> worst(evs.size(), 0);
        std::vector<float> worstR(evs.size(), -1.0f);
        std::vector<bool> worstLeft(evs.size(), true);
        for (std::size_t k = 0; k < lp.size(); ++k)
        {
            const int e = owner[k];
            if (e < 0)
                continue;
            const bool left = lp[k] >= (k < rp.size() ? rp[k] : 0.0f);
            const float r = left ? lp[k] : rp[k];
            if (r > worstR[static_cast<std::size_t>(e)])
            {
                worstR[static_cast<std::size_t>(e)] = r;
                worst[static_cast<std::size_t>(e)] = k;
                worstLeft[static_cast<std::size_t>(e)] = left;
            }
        }
        for (std::size_t e = 0; e < evs.size(); ++e)
        {
            if (worstR[e] < 0.0f)
                continue;
            const double at = from + static_cast<double>(worst[e]) / rate;
            const double step = worstR[e];
            const double excess = excessAt(run, worstLeft[e], at, from, to, nullptr);
            v.windows.push_back({evs[e].name, excess, step, at});
            const bool byExcess = v.excessBound >= 0.0 && excess >= 0.0;
            if (!(byExcess ? excess > v.excessBound : step > clickBound))
                continue;
            // The lead's ruling (23:15, through the steward): an upshift's spark cut under the reference's burn
            // (ecu.model 1, fuel.model 0) is an event of the engine: about 70 ms of near silence and the charge sent
            // to the pipe lights after it (D's measure: 15 to 29 times the local level by the cut's phase; the small
            // four's pull 33.6 after the steward's 0cb535ae). At an upshift's cut (car.flags 32, cut asked) and the
            // 0.15 s after it (with the gear rising), the bound is that step and a margin, 42, and the failure is
            // "click-shift-cut", which report() accepts for that combination only (Report.cpp, shiftCutRuled); over
            // 42, or anywhere else, it is a click.
            constexpr double kShiftCutBound = 42.0;
            // An upshift's cut: the car asks it (car.flags 32, car.model 1) or the ECU cuts the spark (ecu.flags 4,
            // with the classic car), in the 0.15 s before the step, while the gear rises from 0.3 s before it to 0.15 s
            // after (so a limiter's spark cut is not one)
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
            char both[160];
            std::snprintf(both, sizeof both, "excess over its firing periods %.2f (bound %.2f%s)", excess,
                          v.excessBound, byExcess ? ", which decides" : ", not deciding: no period or no reference's");
            if (atShiftCut && step <= kShiftCutBound)
            {
                std::snprintf(b, sizeof b,
                              "%s: largest step %.1f times the local level at %.3f s, at an upshift's cut (the bound "
                              "there %.0f, the lead's ruling for ecu.model 1 under fuel.model 0; elsewhere %.1f); %s",
                              evs[e].name.c_str(), step, at, kShiftCutBound, clickBound, both);
                v.failures.push_back({"click-shift-cut", b, evs[e].name});
            }
            else
            {
                std::snprintf(b, sizeof b,
                              "%s: largest step %.1f times the local level at %.3f s (bound %.1f; the reference's "
                              "%.1f%s); %s",
                              evs[e].name.c_str(), step, at, clickBound, refStep,
                              atShiftCut ? "; at an upshift's cut, over 42" : "", both);
                v.failures.push_back({"click", b, evs[e].name});
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
                       const std::string& check, const std::string& event)
    {
        for (const Known& k : kKnown)
            if (k.id != nullptr && comboMatches(k.combo, c) && matches(k.build, build) && matches(k.scenario, scenario) &&
                matches(k.check, check) && (k.event == nullptr || event == k.event))
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

    // Each field draws from its own generator, seeded by the seed and the field's path (FNV-1a), so a field added to
    // the Schema or un-hidden leaves every other field's draw as it was: a seed names the same Spec across commits
    // (on 25 Nov un-hiding boost.seqRpm, 9b0bab54, redrew every seed, and four "new" soak reds were new territory).
    std::vector<Value> randomSpec(std::uint64_t seed, bool corners)
    {
        std::vector<Value> out;
        for (const Field& f : fuzzFields())
        {
            std::uint64_t h = 0xCBF29CE484222325ull;
            for (const char c : f.path)
                h = (h ^ static_cast<unsigned char>(c)) * 0x100000001B3ull;
            std::mt19937_64 g((seed * 0x9E3779B97F4A7C15ull + 1) ^ h);
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
