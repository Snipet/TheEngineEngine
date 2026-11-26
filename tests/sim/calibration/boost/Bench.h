// tests/sim/calibration/boost/Bench.h: card N2's engine dyno for tests: a build file on MODE_DYNO_ENGINE, full
// throttle, held warm (tee::warmEngine, as tee::runDyno holds it), at 48 kHz; at each speed after settling, the means
// over a window of some telemetry lines and of cylinder 1's exhaust and intake port pressures over whole cycles
// (scope.portExhaust, scope.portIntake). The harness (harness/main.cpp) is the same thing with a table.
#pragma once

#include "tee/Build.h"
#include "tee/Fields.h"
#include "tee/Sim.h"
#include "tee/SimHost.h"
#include "tee/Telemetry.h"
#include "tee/Warm.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tee::calib
{
    using Row = std::map<std::string, double>;

    // The simulator compiled once for every test in this binary (some 6 CPU-s a compile; the cases compiled it nine
    // times, the binary 143 test-s against its 80: the steward, 26 Nov). Kept through a pointer that is never
    // destroyed, as core's Bench keeps its player (01-rules.md section 4: nothing owning a compiled program in a static
    // with a destructor); each case's SimHost is its own, so no state passes between cases.
    inline std::shared_ptr<Sim> compileSim(std::string& errors)
    {
        static std::shared_ptr<Sim>* kept = nullptr;
        if (kept != nullptr && *kept != nullptr)
            return *kept;
        Sim::Options options;
        std::shared_ptr<Sim> sim = Sim::compile(std::filesystem::path(TEE_SOURCE_DIR) / "sim", options, errors);
        if (sim != nullptr)
            kept = new std::shared_ptr<Sim>(sim);
        return sim;
    }

    // Runs `buildFile` (relative to the source tree) with `sets` applied, warm-up `warmup` s at the first speed (the
    // throttle open for its second half), then each speed for `settle` s and measured over `measure` s.
    inline bool dyno(const std::shared_ptr<Sim>& sim, const std::string& buildFile, const std::vector<double>& rpms,
                     const std::vector<std::pair<std::string, double>>& sets, const std::vector<std::string>& lines,
                     std::vector<Row>& out, std::string& error, double settle = 5.0, double measure = 1.0,
                     double warmup = 8.0, bool heatSettle = false)
    {
        const Fields& fields = sim->fields();
        SimHost host;
        LoadOptions load;
        load.sim = sim->options();
        load.sampleRate = 48000.0;
        load.maxBlock = 512;
        load.worker = false;
        load.hold = warmEngine();
        if (!host.attach(sim, load, error))
            return false;
        Build build(fields);
        std::vector<std::string> warnings;
        if (!build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / buildFile, BuildPart::Whole, warnings, error))
            return false;
        for (const auto& [p0, v] : sets)
        {
            // ("path[i]" sets element i of an array field)
            std::string p = p0;
            std::size_t index = 0;
            const auto open = p.find('[');
            if (open != std::string::npos)
            {
                index = static_cast<std::size_t>(std::atoi(p.c_str() + open + 1));
                p = p.substr(0, open);
            }
            if (!build.set(p, v, index))
            {
                error = "cannot set " + p0;
                return false;
            }
        }
        build.attach(&host);

        struct Bound
        {
            std::string name;
            const FieldInfo* info;
            std::size_t index;
        };
        const auto bind = [&](const std::string& prefix, const std::string& name, Bound& b) {
            std::string path = name;
            b.name = name;
            b.index = 0;
            const auto open = path.find('[');
            if (open != std::string::npos)
            {
                b.index = static_cast<std::size_t>(std::atoi(path.c_str() + open + 1));
                path = path.substr(0, open);
            }
            b.info = fields.find(prefix + path);
            return b.info != nullptr;
        };
        std::vector<Bound> tel;
        for (const auto& n : lines)
        {
            Bound b;
            if (!bind("tel.", n, b))
            {
                error = "no tel." + n;
                return false;
            }
            tel.push_back(b);
        }
        Bound pe, pi, cyc, deg;
        if (!bind("scope.", "portExhaust", pe) || !bind("scope.", "portIntake", pi) || !bind("scope.", "cycles", cyc) ||
            !bind("scope.", "degrees", deg))
        {
            error = "no scope port pressures";
            return false;
        }

        std::vector<float> l(512), r(512);
        SimControls c;
        c.mode = 2;
        c.dynoSpeed = static_cast<float>(rpms.front() * 0.10471976);
        c.ignition = 1;              // (the key on first, then to start: off straight to start was card C's dyno fault, 8098b36)
        c.scopeOn = 1;
        const auto play = [&](double seconds, auto&& each) {
            const int blocks = static_cast<int>(seconds * 48000.0 / 512.0 + 0.5);
            for (int k = 0; k < blocks; ++k)
            {
                host.setControls(c);
                host.process(l.data(), r.data(), 512);
                host.service();
                each();
            }
        };
        const auto nothing = [] {};
        // (as tee::runDyno starts: the key on 0.25 s, cranked until max(1, min(2, warmup / 2)) s)
        const double crankEnd = std::max(1.0, std::min(2.0, 0.5 * warmup));
        play(0.25, nothing);
        c.ignition = 2;
        play(crankEnd - 0.25, nothing);
        c.ignition = 1;
        play(0.5 * warmup - crankEnd, nothing);
        c.throttle = 1.0f;
        play(0.5 * warmup, nothing);
        for (const double rpm : rpms)
        {
            c.dynoSpeed = static_cast<float>(rpm * 0.10471976);
            if (heatSettle)
            {
                // (card C's settle, as tee-calib-boost --heat-settle: 1 s into each speed the exhaust's walls and the
                // intercooler's core are put where the present flow holds them, so that a speed reached from another
                // does not carry that one's heat for minutes)
                static int presses = 0;
                const double first = std::min(1.0, settle);
                play(first, nothing);
                ++presses;
                build.set("exhaust.heatSettle", presses);
                build.set("boost.heatSettle", presses);
                play(settle - first, nothing);
            }
            else
                play(settle, nothing);
            std::vector<double> sums(tel.size(), 0.0);
            int n = 0;
            double ex = 0.0, in = 0.0, last = -1.0;
            int cycles = 0;
            TelemetryFrame frame;
            ScopeFrame scope;
            play(measure, [&] {
                if (host.telemetry(frame))
                {
                    for (std::size_t k = 0; k < tel.size(); ++k)
                        sums[k] += frame.get(*tel[k].info, tel[k].index);
                    ++n;
                }
                if (host.scope(scope) && scope.get(*cyc.info) != last)
                {
                    last = scope.get(*cyc.info);
                    const int d = static_cast<int>(scope.get(*deg.info));
                    if (d > 0)
                    {
                        double e = 0.0, i = 0.0;
                        for (int k = 0; k < d; ++k)
                        {
                            e += scope.get(*pe.info, k);
                            i += scope.get(*pi.info, k);
                        }
                        ex += e / d;
                        in += i / d;
                        ++cycles;
                    }
                }
            });
            Row row;
            row["rpm"] = rpm;
            for (std::size_t k = 0; k < tel.size(); ++k)
                row[tel[k].name] = n > 0 ? sums[k] / n : 0.0;
            row["portExhaust"] = cycles > 0 ? ex / cycles : 0.0;
            row["portIntake"] = cycles > 0 ? in / cycles : 0.0;
            out.push_back(row);
        }
        return true;
    }
} // namespace tee::calib
