// tools/tee-dyno: a build on the engine dyno (tee::runDyno): a sweep or steps, the curve as CSV and a text chart.
//
//   tee-dyno --build content/builds/v8.json                 sweep 150 to 650 rad/s at 60 rad/s^2, full throttle
//   tee-dyno --build b.json --from 100 --to 700 --rate 40 --pedal 0.5 --fields boost.pressure,engine.manifold
//   tee-dyno --build b.json --steps 200,300,400,500 --settle 0.8
//   --sim DIR (the simulator; sim/ by default), --entry FILE, --csv FILE (else out/<build>-dyno.csv), --quiet
#include "common/Paths.h"

#include "tee/Dyno.h"

#include <cstdio>
#include <fstream>
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
} // namespace

int main(int argc, char** argv)
{
    tee::tools::Arguments args(argc, argv);
    std::string simFlag, buildFile, csv, value;
    tee::Sim::Options options;
    tee::DynoPlan plan;
    bool quiet = false;
    const auto number = [&](double& out) {
        if (!args.value(value))
            return false;
        out = std::strtod(value.c_str(), nullptr);
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
            ok = number(plan.from);
        else if (args.is("--to"))
            ok = number(plan.to);
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
        }
        else if (args.is("--torque"))
            ok = args.value(plan.torque);
        else if (args.is("--chassis"))
            plan.chassis = true;
        else if (args.is("--quiet"))
            quiet = true;
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
        std::printf("%zu points, %.1f s simulated in %.2f s: %.1f times real time; %s\n", curve.points.size(),
                    curve.simulatedSeconds, curve.wallSeconds, curve.realTime(), csv.c_str());
    }
    return 0;
}
