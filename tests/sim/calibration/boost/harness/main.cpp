// tests/sim/calibration/boost/harness/main.cpp: tee-calib-boost, card N2's measuring harness (not a test). A build on
// the engine dyno (MODE_DYNO_ENGINE, full load, the engine held warm as tee::runDyno holds it) at steps of speed; at
// each, after settling, the means over a window of the telemetry's lines (boosted and not) and, from the Scope, the
// mean over each whole cycle of cylinder 1's exhaust and intake port pressures (the back pressure the cylinder sees).
//
//   tee-calib-boost --build content/builds/archetypes/turbo-four-20-gdi.json --rpm 2000,3500,5000,6500
//                   [--settle 6] [--measure 1] [--heat-settle] [--set boost.model=0 ...] [--hold path=value ...] [--sim DIR]
//
// Output: one line a speed, key=value, then a table. Means over cycles, never a waveform (the engine is chaotic).
#include "common/Paths.h"

#include "tee/Build.h"
#include "tee/Fields.h"
#include "tee/Sim.h"
#include "tee/SimHost.h"
#include "tee/Telemetry.h"
#include "tee/Warm.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::vector<std::string> split(const std::string& text, char sep)
    {
        std::vector<std::string> out;
        std::stringstream in(text);
        std::string item;
        while (std::getline(in, item, sep))
            if (!item.empty())
                out.push_back(item);
        return out;
    }

    // "boost.turbineIn[0]" -> ("tel.boost.turbineIn", 0)
    struct Line
    {
        std::string name;
        const tee::FieldInfo* info = nullptr;
        std::size_t index = 0;
        double sum = 0.0;
        int n = 0;
    };

    bool bind(const tee::Fields& fields, const std::string& prefix, Line& line)
    {
        std::string path = line.name;
        line.index = 0;
        const auto open = path.find('[');
        if (open != std::string::npos)
        {
            line.index = static_cast<std::size_t>(std::atoi(path.c_str() + open + 1));
            path = path.substr(0, open);
        }
        line.info = fields.find(prefix + path);
        return line.info != nullptr;
    }
} // namespace

int main(int argc, char** argv)
{
    tee::tools::Arguments args(argc, argv);
    std::string simFlag, buildFile, value;
    std::vector<double> rpms = {2000, 3500, 5000, 6500};
    double settle = 6.0, measure = 1.0, warmup = 8.0;
    bool heatSettle = false;   // (--heat-settle: at each speed, 1 s in, the exhaust's and the intercooler's heat settled)
    std::vector<std::pair<std::string, double>> sets;
    std::vector<tee::HeldValue> holds;
    std::vector<std::string> extra;
    while (!args.done())
    {
        if (args.is("--sim") && args.value(simFlag))
            continue;
        if (args.is("--heat-settle"))
        {
            heatSettle = true;
            continue;
        }
        if (args.is("--build") && args.value(buildFile))
            continue;
        if (args.is("--rpm") && args.value(value))
        {
            rpms.clear();
            for (const auto& s : split(value, ','))
                rpms.push_back(std::strtod(s.c_str(), nullptr));
            continue;
        }
        if (args.is("--settle") && args.value(value))
        {
            settle = std::strtod(value.c_str(), nullptr);
            continue;
        }
        if (args.is("--measure") && args.value(value))
        {
            measure = std::strtod(value.c_str(), nullptr);
            continue;
        }
        if (args.is("--warmup") && args.value(value))
        {
            warmup = std::strtod(value.c_str(), nullptr);
            continue;
        }
        if (args.is("--fields") && args.value(value))
        {
            for (const auto& s : split(value, ','))
                extra.push_back(s);
            continue;
        }
        const bool isSet = args.is("--set");
        const bool isHold = !isSet && args.is("--hold");
        if ((isSet || isHold) && args.value(value))
        {
            const auto eq = value.find('=');
            if (eq == std::string::npos)
                return 2;
            sets.emplace_back((isHold ? "hold:" : "") + value.substr(0, eq), std::strtod(value.c_str() + eq + 1, nullptr));
            continue;
        }
        std::fprintf(stderr, "tee-calib-boost: unknown argument %s\n", args.peek().c_str());
        return 2;
    }
    // --hold entries are written "hold:path"
    std::vector<std::pair<std::string, double>> specSets;
    for (auto& [p, v] : sets)
    {
        if (p.rfind("hold:", 0) == 0)
            holds.push_back({p.substr(5), v});
        else
            specSets.emplace_back(p, v);
    }

    const std::filesystem::path simDir =
        tee::tools::findDirectory(simFlag, "TEE_SIM_DIR", argv[0], "sim", TEE_SOURCE_SIM_DIR "/..");
    std::string errors;
    tee::Sim::Options options;
    auto sim = tee::Sim::compile(simDir, options, errors);
    if (!sim)
    {
        std::fprintf(stderr, "%s", errors.c_str());
        return 1;
    }
    const tee::Fields& fields = sim->fields();
    tee::SimHost host;
    tee::LoadOptions load;
    load.sim = sim->options();
    load.sampleRate = 48000.0;
    load.maxBlock = 512;
    load.worker = false;
    load.hold = tee::warmEngine();
    for (const auto& h : holds)
        load.hold.push_back(h);
    if (!host.attach(sim, load, errors))
    {
        std::fprintf(stderr, "%s\n", errors.c_str());
        return 1;
    }
    tee::Build build(fields);
    {
        std::vector<std::string> warnings;
        if (!buildFile.empty() && !build.loadFile(buildFile, tee::BuildPart::Whole, warnings, errors))
        {
            std::fprintf(stderr, "tee-calib-boost: %s\n", errors.c_str());
            return 1;
        }
    }
    for (const auto& [p, v] : specSets)
        if (!build.set(p, v))
        {
            std::fprintf(stderr, "tee-calib-boost: cannot set %s\n", p.c_str());
            return 1;
        }
    build.attach(&host);

    std::vector<std::string> names = {"engine.speed", "engine.torque", "engine.bmep", "engine.imep", "engine.pmep",
                                      "engine.fmep", "engine.veAir", "engine.veManifold", "engine.manifold",
                                      "boost.pressure", "boost.chargePipe", "boost.throttleIn", "boost.intercoolerOut",
                                      "boost.compressorOut[0]", "boost.ratio[0]", "boost.flow[0]",
                                      "boost.efficiency[0]", "boost.shaftSpeed[0]", "boost.turbineIn[0]",
                                      "boost.turbineInTemp[0]", "boost.gate[0]", "boost.superchargerPower",
                                      "ecu.lambda", "ecu.advance", "ecu.halfBurn", "ecu.knockRetard",
                                      "exhaust.portTemperature"};
    for (const auto& e : extra)
        names.push_back(e);
    std::vector<Line> lines;
    for (const auto& n : names)
    {
        Line l;
        l.name = n;
        if (bind(fields, "tel.", l))
            lines.push_back(l);
        else
            std::fprintf(stderr, "tee-calib-boost: no tel.%s (left out)\n", n.c_str());
    }
    Line portEx{"portExhaust"}, portIn{"portIntake"}, cylP{"pressure"}, cycles{"cycles"}, degrees{"degrees"};
    const bool haveScope = bind(fields, "scope.", portEx) && bind(fields, "scope.", portIn) &&
                           bind(fields, "scope.", cycles) && bind(fields, "scope.", degrees);

    std::vector<float> left(512), right(512);
    tee::SimControls c;
    c.mode = 2;
    c.dynoSpeed = static_cast<float>(rpms.front() * 0.10471976);
    c.ignition = 1;                  // (the key on first, then to start: card C's 8098b36)
    c.throttle = 0.0f;
    c.scopeOn = 0;                   // (on only while measuring)
    const double blockSeconds = 512.0 / 48000.0;
    double t = 0.0;
    const auto play = [&](double seconds, const std::function<void()>& each) {
        const int blocks = static_cast<int>(seconds / blockSeconds + 0.5);
        for (int k = 0; k < blocks; ++k)
        {
            host.setControls(c);
            host.process(left.data(), right.data(), 512);
            host.service();
            t += blockSeconds;
            if (each)
                each();
        }
    };
    // (as tee::runDyno starts since 8098b36: the key on 0.25 s, then cranked until max(1, min(2, warmup / 2)) s: a heavy
    // diesel needs more than 1 s of cranking to catch)
    const double crankEnd = std::max(1.0, std::min(2.0, 0.5 * warmup));
    play(0.25, {});
    c.ignition = 2;
    play(crankEnd - 0.25, {});
    c.ignition = 1;
    play(0.5 * warmup - crankEnd, {});
    c.throttle = 1.0f;
    play(0.5 * warmup, {});

    std::printf("build %s\n", buildFile.c_str());
    for (const auto& [p, v] : specSets)
        std::printf("set %s = %g\n", p.c_str(), v);
    std::vector<std::map<std::string, double>> table;
    for (const double rpm : rpms)
    {
        c.dynoSpeed = static_cast<float>(rpm * 0.10471976);
        c.scopeOn = 0;
        if (heatSettle)
        {
            static int presses = 0;
            const double first = std::min(1.0, settle);
            play(first, {});
            ++presses;
            build.set("exhaust.heatSettle", presses);
            build.set("boost.heatSettle", presses);
            play(settle - first, {});
        }
        else
            play(settle, {});
        c.scopeOn = 1;
        for (auto& l : lines)
            l.sum = 0.0, l.n = 0;
        double exSum = 0.0, inSum = 0.0;
        int nCycles = 0;
        double lastCycle = -1.0;
        tee::TelemetryFrame frame;
        tee::ScopeFrame scope;
        play(measure, [&] {
            if (host.telemetry(frame))
                for (auto& l : lines)
                {
                    l.sum += frame.get(*l.info, l.index);
                    l.n += 1;
                }
            if (haveScope && host.scope(scope))
            {
                const double cy = scope.get(*cycles.info);
                if (cy != lastCycle)
                {
                    lastCycle = cy;
                    const int deg = static_cast<int>(scope.get(*degrees.info));
                    if (deg > 0)
                    {
                        double ex = 0.0, in = 0.0;
                        for (int d = 0; d < deg; ++d)
                        {
                            ex += scope.get(*portEx.info, d);
                            in += scope.get(*portIn.info, d);
                        }
                        exSum += ex / deg;
                        inSum += in / deg;
                        nCycles += 1;
                    }
                }
            }
        });
        std::map<std::string, double> row;
        row["rpm"] = rpm;
        std::printf("rpm=%.0f", rpm);
        for (const auto& l : lines)
        {
            const double m = l.n > 0 ? l.sum / l.n : 0.0;
            row[l.name] = m;
            std::printf(" %s=%.6g", l.name.c_str(), m);
        }
        if (nCycles > 0)
        {
            row["portExhaust"] = exSum / nCycles;
            row["portIntake"] = inSum / nCycles;
            std::printf(" portExhaust=%.6g portIntake=%.6g cycles=%d", exSum / nCycles, inSum / nCycles, nCycles);
        }
        std::printf("\n");
        std::fflush(stdout);
        table.push_back(row);
    }

    // the summary: kPa absolute unless said
    std::printf("\n| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a "
                "| IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm "
                "| comp eta | SC kW | lambda | spark | 50%% burn | knock ret | port gas K |\n");
    std::printf("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n");
    for (auto& r : table)
    {
        const double ex = r.count("portExhaust") ? r["portExhaust"] : 0.0;
        const double in = r.count("portIntake") ? r["portIntake"] : 0.0;
        std::printf("| %.0f | %.0f | %.1f | %.1f | %.2f | %.2f | %.2f | %.0f | %.0f | %.0f | %.0f | %.0f | %.0f | %.2f | %.2f "
                    "| %.0f | %.2f | %.1f | %.2f | %.1f | %.1f | %.1f | %.0f |\n",
                    r["rpm"], r["engine.torque"], r["engine.bmep"] / 1e5, r["engine.imep"] / 1e5, r["engine.pmep"] / 1e5,
                    r["engine.fmep"] / 1e5, r["engine.veAir"], r["boost.chargePipe"] / 1e3, r["engine.manifold"] / 1e3,
                    r["boost.intercoolerOut"], r["boost.turbineIn[0]"] / 1e3, ex / 1e3, in / 1e3,
                    in > 0 ? ex / in : 0.0, r["boost.gate[0]"], r["boost.shaftSpeed[0]"] * 60.0 / 6.283185307 / 1000.0,
                    r["boost.efficiency[0]"], r["boost.superchargerPower"] / 1e3, r["ecu.lambda"], r["ecu.advance"],
                    r["ecu.halfBurn"], r["ecu.knockRetard"], r["exhaust.portTemperature"]);
    }
    return 0;
}
