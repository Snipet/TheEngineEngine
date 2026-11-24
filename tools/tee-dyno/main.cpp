// tools/tee-dyno: a build on the engine dyno (tee::runDyno): a sweep or steps, the curve as CSV and a text chart.
//
//   tee-dyno --build content/builds/v8.json                 sweep 150 to 650 rad/s at 60 rad/s^2, full throttle
//   tee-dyno --build b.json --from 100 --to 700 --rate 40 --pedal 0.5 --fields boost.pressure,engine.manifold
//   tee-dyno --build b.json --steps 200,300,400,500 --settle 0.8
//   --sim DIR (the simulator; sim/ by default), --entry FILE, --csv FILE (else out/<build>-dyno.csv), --quiet
//
//   tee-dyno --predict --build content/builds/v8.json       the ribbon's curve (tee::PredictService): steady points on
//            [--threads N] [--points N] [--coarse N]        several threads, coarse first; the times to the first and
//            [--tolerance X] [--min-cycles N] [--max-cycles N] [--cold] [--history] [--repeat N]
//            [--brake auto|host|program] [--hold S] [--no-settle] [--warmup-seconds S] [--inner-rate HZ]
//   tee-dyno --bake-cam --build B [--from W --to W --points N]   the intake cam swept, best advance by speed (Bake.h)
//            [--ramp A]                                     and against a slow sweep at A rad/s^2 with the same brake
#include "common/Paths.h"

#include "tee/Bake.h"
#include "tee/Dyno.h"
#include "tee/Predict.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <thread>
#include <sstream>

namespace
{
    int usage()
    {
        std::fprintf(stderr,
                     "usage: tee-dyno [--build FILE] [--sim DIR] [--entry FILE] [--from W] [--to W] [--rate A] "
                     "[--spacing W] [--steps W,W,...] [--settle S] [--measure S] [--pedal P] [--warmup S] "
                     "[--fields a,b] [--torque FIELD] [--sample-rate HZ] [--block N] [--chassis] [--csv FILE] "
                     "[--quiet]\n  speeds in rad/s\n");
        return 2;
    }

    std::vector<std::string> list(const std::string& text)
    {
        std::vector<std::string> out;
        std::stringstream in(text);
        std::string item;
        while (std::getline(in, item, ','))
            if (!item.empty())
                out.push_back(item);
        return out;
    }

    double rpm(double w)
    {
        return w * 30.0 / 3.14159265358979323846;
    }

    // The ribbon's path, timed as the interface would see it: requested, then polled every millisecond.
    int predict(const std::shared_ptr<tee::Sim>& sim, const tee::Build& build, const tee::PredictOptions& options,
                double rampRate, bool history, int repeat)
    {
        tee::PredictService service(sim, options);
        std::printf("predict: %d worker threads, %d speeds (%d coarse), tolerance %.3g, %d to %d cycles%s\n",
                    service.threads(), options.points, options.coarse, options.tolerance, options.minCycles,
                    options.maxCycles, options.warm ? ", warm" : ", cold");
        tee::PredictCurve last;
        for (int r = 0; r < std::max(repeat, 1); ++r)
        {
            tee::Build b = build;
            if (r > 0)   // another build each time, so the cache does not answer: the idle trim nudged
                b.set("engine.idleTrim", b.get("engine.idleTrim") + 1e-4 * r);
            const auto t0 = std::chrono::steady_clock::now();
            service.request(b);
            double first = -1.0;
            while (true)
            {
                tee::PredictCurve c;
                if (service.take(c))
                {
                    const double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                    if (first < 0.0)
                    {
                        first = t;
                        std::printf("  run %d: first curve (%zu points) at %.3f s\n", r + 1, c.points.size(), t);
                    }
                    if (c.refined)
                    {
                        std::printf("  run %d: refined curve (%zu points) at %.3f s; %.2f s of engine time\n", r + 1,
                                    c.points.size(), t, c.simulatedSeconds);
                        last = c;
                        break;
                    }
                }
                if (!service.error().empty())
                {
                    std::fprintf(stderr, "tee-dyno: %s\n", service.error().c_str());
                    return 1;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        std::printf("brake: %s, inside at %.0f Hz\n%8s %10s %10s %7s %8s\n", last.brake.c_str(), last.innerRate, "rpm", "torque Nm", "power kW", "cycles",
                    "settled");
        for (const tee::PredictPoint& p : last.points)
        {
            std::printf("%8.0f %10.1f %10.1f %7d %8s", rpm(p.speed), p.torque, p.power / 1000.0, p.cycles,
                        p.settled ? "yes" : "NO");
            if (p.knockRetard > 0.0)
                std::printf("  knock %.1f deg", p.knockRetard);
            for (std::size_t k = 0; k < p.fields.size() && k < options.fields.size(); ++k)
                std::printf("  %s %.5g", options.fields[k].c_str(), p.fields[k]);
            if (history)
                for (const double t : p.history)
                    std::printf(" %.1f", t);
            std::printf("\n");
        }
        if (rampRate > 0.0)
        {
            tee::PredictCurve ramp;
            std::string error;
            if (!tee::predictRamp(sim, build, options, rampRate, ramp, error))
            {
                std::fprintf(stderr, "tee-dyno: %s\n", error.c_str());
                return 1;
            }
            std::printf("against a sweep at %.0f rad/s^2 (%.1f s of engine time in %.1f s):\n%8s %10s %10s %8s\n",
                        rampRate, ramp.simulatedSeconds, ramp.wallSeconds, "rpm", "sweep Nm", "points Nm", "differ");
            double worst = 0.0, sum = 0.0, peak = 0.0;
            for (const tee::PredictPoint& p : ramp.points)
                peak = std::max(peak, std::fabs(p.torque));
            for (std::size_t i = 0; i < ramp.points.size() && i < last.points.size(); ++i)
            {
                const double d = (last.points[i].torque - ramp.points[i].torque) / std::max(peak, 1.0);
                worst = std::max(worst, std::fabs(d));
                sum += std::fabs(d);
                std::printf("%8.0f %10.1f %10.1f %7.1f%%\n", rpm(ramp.points[i].speed), ramp.points[i].torque,
                            last.points[i].torque, 100.0 * d);
            }
            std::printf("difference over the peak torque: mean %.2f %%, worst %.2f %%\n",
                        100.0 * sum / std::max<std::size_t>(ramp.points.size(), 1), 100.0 * worst);
        }
        if (last.faults > 0)
        {
            std::fprintf(stderr, "tee-dyno: the engine diverged (FLAG_FAULT) at %d point(s): they are NaN, not measures\n",
                         last.faults);
            return 1;
        }
        return 0;
    }
} // namespace

int main(int argc, char** argv)
{
    tee::tools::Arguments args(argc, argv);
    std::string simFlag, buildFile, csv, value;
    tee::Sim::Options options;
    tee::DynoPlan plan;
    tee::PredictOptions predictOptions;
    bool quiet = false, predictMode = false, history = false, bakeCam = false;
    double rampRate = 0.0, repeat = 1.0;
    const auto number = [&](double& out) {
        if (!args.value(value))
            return false;
        out = std::strtod(value.c_str(), nullptr);
        return true;
    };
    const auto number0 = [](double& out, double v) {
        out = v;
        return true;
    };
    const auto whole = [&](int& out) {
        double v = 0.0;
        if (!number(v))
            return false;
        out = static_cast<int>(v);
        return true;
    };
    while (!args.done())
    {
        bool ok = true;
        if (args.is("--sim"))
            ok = args.value(simFlag);
        else if (args.is("--entry"))
            ok = args.value(options.entry);
        else if (args.is("--build"))
            ok = args.value(buildFile);
        else if (args.is("--csv"))
            ok = args.value(csv);
        else if (args.is("--from"))
            ok = number(plan.from) && number0(predictOptions.from, plan.from);
        else if (args.is("--to"))
            ok = number(plan.to) && number0(predictOptions.to, plan.to);
        else if (args.is("--rate"))
            ok = number(plan.rate);
        else if (args.is("--spacing"))
            ok = number(plan.spacing);
        else if (args.is("--settle"))
            ok = number(plan.settle);
        else if (args.is("--measure"))
            ok = number(plan.measure);
        else if (args.is("--pedal"))
            ok = number(plan.pedal);
        else if (args.is("--warmup"))
            ok = number(plan.warmup);
        else if (args.is("--sample-rate"))
            ok = number(plan.sampleRate);
        else if (args.is("--block"))
        {
            double b = 0;
            ok = number(b);
            plan.block = static_cast<int>(b);
        }
        else if (args.is("--steps"))
        {
            ok = args.value(value);
            for (const std::string& s : list(value))
                plan.steps.push_back(std::strtod(s.c_str(), nullptr));
        }
        else if (args.is("--fields"))
        {
            ok = args.value(value);
            plan.fields = list(value);
            predictOptions.fields = plan.fields;
        }
        else if (args.is("--torque"))
            ok = args.value(plan.torque);
        else if (args.is("--chassis"))
            plan.chassis = true;
        else if (args.is("--quiet"))
            quiet = true;
        else if (args.is("--predict"))
            predictMode = true;
        else if (args.is("--bake-cam"))
            bakeCam = true;
        else if (args.is("--history"))
            history = true;
        else if (args.is("--cold"))
            predictOptions.warm = false;
        else if (args.is("--ramp"))
            ok = number(rampRate);
        else if (args.is("--repeat"))
            ok = number(repeat);
        else if (args.is("--no-settle"))
        {
            predictOptions.settleExhaust = false;
            plan.settleExhaust = false;
        }
        else if (args.is("--inner-rate"))
            ok = number(predictOptions.innerRate);
        else if (args.is("--brake"))
            ok = args.value(predictOptions.brake);
        else if (args.is("--hold"))
            ok = number(predictOptions.holdSeconds);
        else if (args.is("--min-hold"))
            ok = number(predictOptions.minHoldSeconds);
        else if (args.is("--warmup-seconds"))
            ok = number(predictOptions.warmupSeconds);
        else if (args.is("--tolerance"))
            ok = number(predictOptions.tolerance);
        else if (args.is("--threads"))
            ok = whole(predictOptions.threads);
        else if (args.is("--points"))
            ok = whole(predictOptions.points);
        else if (args.is("--coarse"))
            ok = whole(predictOptions.coarse);
        else if (args.is("--min-cycles"))
            ok = whole(predictOptions.minCycles);
        else if (args.is("--max-cycles"))
            ok = whole(predictOptions.maxCycles);
        else
            return usage();
        if (!ok)
            return usage();
    }
    const std::filesystem::path sim =
        tee::tools::findDirectory(simFlag, "TEE_SIM_DIR", argv[0], "sim", TEE_SOURCE_SIM_DIR "/..");
    std::string errors;
    auto compiled = tee::Sim::compile(sim, options, errors);
    if (!compiled)
    {
        std::fprintf(stderr, "%s", errors.c_str());
        return 1;
    }
    tee::Build build(compiled->fields());
    if (!buildFile.empty())
    {
        std::vector<std::string> warnings;
        if (!build.loadFile(buildFile, tee::BuildPart::Whole, warnings, errors))
        {
            std::fprintf(stderr, "tee-dyno: %s\n", errors.c_str());
            return 1;
        }
        for (const std::string& w : warnings)
            std::fprintf(stderr, "tee-dyno: warning: %s\n", w.c_str());
    }
    if (bakeCam)
    {
        tee::CamBakeOptions bake;
        bake.predict = predictOptions;
        if (bake.predict.points == tee::PredictOptions{}.points)
            bake.predict.points = bake.predict.coarse = 8;
        tee::CamBakeResult r;
        if (!tee::bakeIntakeCam(compiled, build, bake, r, errors))
        {
            std::fprintf(stderr, "tee-dyno: %s\n", errors.c_str());
            return 1;
        }
        std::printf("intake cam baked by %zu curves in %.1f s (parked centreline %.1f deg); torque N m by advance:\n%8s",
                    r.advances.size(), r.wallSeconds, r.parkedCentre, "rpm");
        for (const double a : r.advances)
            std::printf(" %7.0f", a);
        std::printf("   best\n");
        for (std::size_t s2 = 0; s2 < r.speeds.size(); ++s2)
        {
            std::printf("%8.0f", rpm(r.speeds[s2]));
            for (std::size_t a = 0; a < r.advances.size(); ++a)
                std::printf(" %7.1f", r.torque[a][s2]);
            std::printf("   %.0f deg (%.1f N m)\n", r.best[s2], r.bestTorque[s2]);
        }
        std::printf("knock retard, degrees, by advance:\n");
        for (std::size_t s2 = 0; s2 < r.speeds.size(); ++s2)
        {
            std::printf("%8.0f", rpm(r.speeds[s2]));
            for (std::size_t a = 0; a < r.advances.size(); ++a)
                std::printf(" %7.1f", r.knock[a][s2]);
            std::printf("\n");
        }
        return 0;
    }
    if (predictMode)
        return predict(compiled, build, predictOptions, rampRate, history, static_cast<int>(repeat));
    tee::DynoCurve curve;
    if (!tee::runDyno(compiled, build, plan, curve, errors))
    {
        std::fprintf(stderr, "tee-dyno: %s\n", errors.c_str());
        return 1;
    }
    if (csv.empty())
    {
        const std::string stem = buildFile.empty() ? "defaults" : std::filesystem::path(buildFile).stem().generic_string();
        std::filesystem::create_directories("out");
        csv = "out/" + stem + "-dyno.csv";
    }
    std::ofstream out(csv, std::ios::binary);
    out << curve.csv();
    if (!out)
    {
        std::fprintf(stderr, "tee-dyno: cannot write %s\n", csv.c_str());
        return 1;
    }
    if (!quiet)
    {
        std::printf("%s", curve.chart().c_str());
        std::printf("%s\n", curve.settled ? "the exhaust's heat settled at each step (steady state)"
                                           : "the exhaust's heat not settled: a step's first seconds, its walls warming");
        std::printf("%zu points, %.1f s simulated in %.2f s: %.1f times real time; %s\n", curve.points.size(),
                    curve.simulatedSeconds, curve.wallSeconds, curve.realTime(), csv.c_str());
    }
    if (curve.faults > 0)
    {
        std::fprintf(stderr, "tee-dyno: the engine diverged (FLAG_FAULT) and was put at rest: the run stopped there\n");
        return 1;
    }
    return 0;
}
