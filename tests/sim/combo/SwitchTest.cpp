// tests/sim/combo/SwitchTest.cpp: package X's switching while running. Every model switch flipped 0 to 1 and back
// while the engine pulls in gear; every choice and switch of the Spec stepped through its options while it revs; a
// build loaded over another while it runs (the fade). Judged as the combinations are, and it must still run.
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <map>

using namespace combo;

namespace
{
    // The run must be running over [from, to] (state 2, or cranking again by itself: state never 0 or 3 for long).
    void mustRun(const tee::test::Run& r, double from, double to, Verdict& v)
    {
        const auto st = r.tel("engine.state").during(from, to);
        double down = 0.0;
        const auto& t = st.times();
        const auto& x = st.values();
        for (std::size_t i = 1; i < t.size(); ++i)
            if (x[i] == 0.0 || x[i] == 3.0)
                down += t[i] - t[i - 1];
        if (down > 0.5)
            v.failures.push_back({"stall", "not running for " + std::to_string(down) + " s of the switching"});
    }

    void flipCase(std::size_t i, const char* build)
    {
        const auto& s = switches();
        if (i >= s.size())
            SKIP("the program has " << s.size() << " switches");
        const Switch& w = s[i];
        Times tm;
        tm.idleFrom = 2.5;
        tm.idleTo = 3.0;
        tm.off = 12.0;
        tm.end = 15.0;
        tee::test::Scenario sc(buildPath(build));
        sc.at(0.0).spec("env.temperature", 293.15).control("mode", 1).lever(0).clutch(0).ignition(tee::On);
        sc.at(tm.start).ignition(tee::Start);
        sc.at(tm.started).ignition(tee::On);
        sc.at(3.0).shiftUp();
        sc.at(3.3).throttle(0.6).ramp(0.3);
        sc.at(5.0).spec(w.path, w.on);
        sc.at(6.5).spec(w.path, 0);
        sc.at(7.0).shiftUp();
        sc.at(8.0).spec(w.path, w.on);
        sc.at(9.0).throttle(1.0).ramp(0.1);
        sc.at(9.5).spec(w.path, 0);
        sc.at(10.5).throttle(0.0).ramp(0.05);
        sc.at(10.6).brake(0.7).ramp(0.2);
        sc.at(12.0).ignition(tee::Off);
        const tee::test::Run r = sc.run(tm.end);
        Verdict v = judge(r, tm, archetype(build), true, referenceStep(build, Kind::Pull));
        mustRun(r, 3.0, 11.0, v);
        report(v, "flip " + w.path, build, "flip");
    }

    // Every choice and switch of one section stepped through its options while the engine revs in neutral: each
    // option held 0.4 s, then the field back to its default.
    void stepCase(const std::string& section)
    {
        std::vector<const Field*> list;
        for (const Field& f : fuzzFields())
            if (f.choices > 0 && f.path.rfind(section + ".", 0) == 0 && f.path.find('[') == std::string::npos &&
                f.path != section + ".model" && f.path != "engine.custom")
                list.push_back(&f);
        if (list.empty())
            SKIP("no choices in " << section);
        Times tm;
        tm.idleFrom = -1.0;
        tee::test::Scenario sc(buildPath("v8"));
        sc.at(0.0).spec("env.temperature", 293.15).control("mode", 1).lever(0).clutch(0).ignition(tee::On);
        sc.at(tm.start).ignition(tee::Start);
        sc.at(tm.started).ignition(tee::On);
        sc.at(3.0).throttle(0.3).ramp(0.2);
        double t = 4.0;
        std::string names;
        for (const Field* f : list)
        {
            for (double x = f->lo; x <= f->hi + 1e-9; x += 1.0)
            {
                sc.at(t).spec(f->path, x);
                t += 0.4;
            }
            sc.at(t).spec(f->path, f->def);
            t += 0.4;
            names += " " + f->path;
        }
        tm.end = t + 1.5;
        std::printf("combo: stepping %zu fields of %s over %.1f s:%s\n", list.size(), section.c_str(), t - 4.0,
                    names.c_str());
        const tee::test::Run r = sc.run(tm.end);
        Verdict v = judge(r, tm, nullptr, false, referenceStep("v8", Kind::Tour));
        mustRun(r, 3.0, tm.end, v);
        // the key stays on: a layout or a fuel swapped under the fade must leave it running (the reference's fade does)
        report(v, "step " + section, "v8", "step");
    }

    // A build file's values as Spec writes: every field a fuzzer knows, the file's value or its default (a load).
    std::vector<std::pair<std::string, nlohmann::json>> loadOf(const std::string& build)
    {
        std::ifstream in(buildPath(build));
        nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
        std::map<std::string, nlohmann::json> given;
        if (j.is_object() && j.contains("spec"))
            for (auto& [sec, fields] : j["spec"].items())
                for (auto& [name, value] : fields.items())
                {
                    if (value.is_array())
                        for (std::size_t i = 0; i < value.size(); ++i)
                            given[sec + "." + name + "[" + std::to_string(i) + "]"] = value[i];
                    else
                        given[sec + "." + name] = value;
                }
        std::vector<std::pair<std::string, nlohmann::json>> out;
        for (const Field& f : fuzzFields())
        {
            auto it = given.find(f.path);
            out.emplace_back(f.path, it != given.end() ? it->second : nlohmann::json(f.def));
        }
        return out;
    }

    void load(tee::test::Scenario& sc, double t, const std::string& build)
    {
        sc.at(t);
        for (const auto& [path, value] : loadOf(build))
        {
            if (value.is_string())
                sc.spec(path, value.get<std::string>());
            else if (value.is_boolean())
                sc.spec(path, value.get<bool>() ? 1.0 : 0.0);
            else if (value.is_number())
                sc.spec(path, value.get<double>());
        }
    }
} // namespace

#define FLIP_CASES(i)                                                                                                  \
    TEST_CASE("switch " #i " flipped on and off while the V8 pulls in gear", "[nightly][flip]")                          \
    {                                                                                                                  \
        flipCase(i, "v8");                                                                                             \
    }                                                                                                                  \
    TEST_CASE("switch " #i " flipped on and off while the turbo six pulls in gear", "[nightly][flip]")                   \
    {                                                                                                                  \
        flipCase(i, "turbo-six");                                                                                      \
    }
FLIP_CASES(0)
FLIP_CASES(1)
FLIP_CASES(2)
FLIP_CASES(3)
FLIP_CASES(4)
FLIP_CASES(5)
FLIP_CASES(6)
FLIP_CASES(7)
FLIP_CASES(8)
FLIP_CASES(9)
FLIP_CASES(10)
FLIP_CASES(11)
FLIP_CASES(12)
FLIP_CASES(13)

#define STEP_CASE(section)                                                                                             \
    TEST_CASE("every choice and switch of spec." section " stepped while the V8 revs", "[nightly][step]")                \
    {                                                                                                                  \
        stepCase(section);                                                                                             \
    }
STEP_CASE("engine")
STEP_CASE("valve")
STEP_CASE("intake")
STEP_CASE("boost")
STEP_CASE("exhaust")
STEP_CASE("fuel")
STEP_CASE("ecu")
STEP_CASE("car")
STEP_CASE("env")
STEP_CASE("sound")

namespace
{
    void loadCase(bool all)
    {
        Times tm;
        tm.idleFrom = -1.0;
        tm.end = 17.0;
        tee::test::Scenario sc(buildPath("v8"));
        sc.at(0.0).spec("env.temperature", 293.15).control("mode", 1).lever(0).clutch(0).ignition(tee::On);
        sc.at(tm.start).ignition(tee::Start);
        sc.at(tm.started).ignition(tee::On);
        sc.at(3.0).throttle(0.4).ramp(0.2);
        const char* order[] = {"small-four", "large-diesel", "turbo-six", "v8"};
        double t = 5.0;
        for (const char* b : order)
        {
            load(sc, t, b);
            if (all)
                for (const Switch& w : switches())
                    sc.at(t).spec(w.path, w.on);
            t += 3.0;
        }
        const tee::test::Run r = sc.run(tm.end);
        double ref = 0.0;
        for (const char* b : {"v8", "small-four", "large-diesel", "turbo-six"})
            ref = std::max(ref, referenceStep(b, Kind::Tour));
        Verdict v = judge(r, tm, nullptr, false, ref);
        mustRun(r, 3.0, tm.end, v);
        report(v, all ? "load, all on" : "load", "v8", "load");
    }
} // namespace

TEST_CASE("a build loaded over another while it runs: V8 then small four then large diesel then turbo six then V8",
          "[nightly][load]")
{
    loadCase(false);
}

TEST_CASE("a build loaded over another while it runs with every switch on", "[nightly][combo][load]")
{
    loadCase(true);
}

namespace
{
    // the integrator holds a changed model until the fade's quiet (Engine::holdModel, 02bf4db): all at once must be
    // one clean change, and the engine still running after each
    void flipAllCase(const char* build)
    {
        Times tm;
        tm.idleFrom = 2.5;
        tm.idleTo = 3.0;
        tm.off = 12.0;
        tm.end = 15.0;
        tee::test::Scenario sc(buildPath(build));
        sc.at(0.0).spec("env.temperature", 293.15).control("mode", 1).lever(0).clutch(0).ignition(tee::On);
        sc.at(tm.start).ignition(tee::Start);
        sc.at(tm.started).ignition(tee::On);
        sc.at(3.0).shiftUp();
        sc.at(3.3).throttle(0.6).ramp(0.3);
        for (double t : {5.0, 6.5, 8.0, 9.5})
        {
            const bool on = t == 5.0 || t == 8.0;
            sc.at(t);
            for (const Switch& w : switches())
                if (w.path != "engine.cycle")   // a two-stroke needs its own build (card T's): not over a V8
                    sc.spec(w.path, on ? w.on : 0);
        }
        sc.at(7.0).shiftUp();
        sc.at(9.0).throttle(1.0).ramp(0.1);
        sc.at(10.5).throttle(0.0).ramp(0.05);
        sc.at(10.6).brake(0.7).ramp(0.2);
        sc.at(12.0).ignition(tee::Off);
        const tee::test::Run r = sc.run(tm.end);
        Verdict v = judge(r, tm, archetype(build), true, referenceStep(build, Kind::Pull));
        mustRun(r, 3.0, 11.0, v);
        report(v, "flip all at once", build, "flip");
    }
} // namespace

TEST_CASE("every switch flipped in the same block while the V8 pulls in gear", "[fast][fast-2][combo][flip]")
{
    flipAllCase("v8");
}

TEST_CASE("every switch flipped in the same block while the turbo six pulls in gear", "[nightly][flip]")
{
    flipAllCase("turbo-six");
}
