// tools/tee-fuzz: package X's fuzzer, the same harness as tee-tests-combo (tests/sim/combo/Combo.h). Named .cc so
// that tools/CMakeLists.txt's glob (*.cpp, on Tee::host alone) passes it by: tests/sim/combo/CMakeLists.txt builds it.
//
//   tee-fuzz --seeds 1-50 [--corners] [--seconds 6]     random Specs inside the declared ranges, random driving
//   tee-fuzz --seed 17 --reduce                         a failing seed reduced to the fewest fields that still fail;
//                                                       writes out/combo/fuzz-<seed>.json (a build) and prints the
//                                                       one-line command that shows it
//   tee-fuzz ... --checks                               the program compiled with FMinus's bounds checks and loop
//                                                       budget: an access out of bounds stops the call with
//                                                       FMinus's words (a crash without them)
//   tee-fuzz --seeds 100-110 --dump                     each seed's whole Spec to out/combo/spec-<seed>.json, no run
//   tee-fuzz --build file.json --seed 17                that build (and nothing random) under seed 17's driving
//   tee-fuzz --play v8 tour car.model+engine.custom     one combination (switch paths joined by +, or none, or all)
//                                                       on an archetype through a scenario, judged as the tests do
//                                                       (against the reference's own step); --wav f.wav keeps it
//   tee-fuzz --write-scenarios                          content/scenarios/combo/*.json from the harness
//   tee-fuzz --match -20 a.wav b.wav ...                the files to one loudness, in place (the listening pack's
//                                                       pairs): each file's gated RMS (100 ms windows, those within
//                                                       30 dB of the loudest) brought to the target in dBFS; when a
//                                                       file's peak would pass -0.5 dBFS, every file of the call
//                                                       is lowered alike, so the set stays matched
//
// Exit 0 when every run passed, 3 when one failed, 2 on a usage error.
#include "Combo.h"

#include "tee/Wav.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

using namespace combo;

namespace
{
    struct Options
    {
        std::uint64_t first = 1, last = 1;
        bool corners = false;
        bool reduce = false;
        bool dump = false;   // --dump: write each seed's whole Spec to out/combo/spec-<seed>.json and run nothing
        double seconds = 6.0;
        std::string build;
    };

    std::vector<Value> fromFile(const std::string& path)
    {
        std::vector<Value> out;
        std::ifstream in(path);
        nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
        if (!j.is_object() || !j.contains("spec"))
        {
            std::fprintf(stderr, "tee-fuzz: %s is no build\n", path.c_str());
            return out;
        }
        for (auto& [sec, fields] : j["spec"].items())
            for (auto& [name, value] : fields.items())
            {
                auto put = [&](const std::string& p, const nlohmann::json& v) {
                    if (v.is_number())
                        out.push_back({p, v.get<double>()});
                    else if (v.is_boolean())
                        out.push_back({p, v.get<bool>() ? 1.0 : 0.0});
                };
                if (value.is_array())
                    for (std::size_t i = 0; i < value.size(); ++i)
                        put(sec + "." + name + "[" + std::to_string(i) + "]", value[i]);
                else
                    put(sec + "." + name, value);
            }
        return out;
    }

    bool gChecks = false;   // --checks: the program compiled with its loop budget and bounds checks

    Verdict once(const std::vector<Value>& spec, std::uint64_t seed, double seconds)
    {
        Times tm;
        tee::test::Scenario sc = fuzzScenario(spec, seed, seconds, tm);
        if (gChecks)
            sc.checks();
        const tee::test::Run r = sc.run(tm.end);
        return judgeFuzz(r, tm);
    }

    bool hasCheck(const Verdict& v, const std::string& check)
    {
        for (const Failure& f : v.failures)
            if (f.check == check)
                return true;
        return false;
    }

    // ddmin over the fields that differ from their defaults: the fewest whose random values still fail `check`.
    std::vector<Value> reduce(const std::vector<Value>& spec, std::uint64_t seed, double seconds, const std::string& check)
    {
        std::map<std::string, double> def;
        for (const Field& f : fuzzFields())
            def[f.path] = f.def;
        std::vector<Value> keep;
        for (const Value& v : spec)
            if (!def.count(v.path) || def[v.path] != v.value)
                keep.push_back(v);
        int runs = 0;
        std::size_t n = 2;
        while (keep.size() >= 2)
        {
            const std::size_t chunk = (keep.size() + n - 1) / n;
            bool reduced = false;
            for (std::size_t start = 0; start < keep.size(); start += chunk)
            {
                std::vector<Value> without;
                for (std::size_t i = 0; i < keep.size(); ++i)
                    if (i < start || i >= start + chunk)
                        without.push_back(keep[i]);
                ++runs;
                if (hasCheck(once(without, seed, seconds), check))
                {
                    keep = without;
                    n = std::max<std::size_t>(n - 1, 2);
                    reduced = true;
                    std::printf("  reduce: %zu fields left (%d runs)\n", keep.size(), runs);
                    std::fflush(stdout);
                    break;
                }
            }
            if (!reduced)
            {
                if (n >= keep.size())
                    break;
                n = std::min(keep.size(), n * 2);
            }
        }
        return keep;
    }

    // The gated RMS of a file in dB (both channels): 100 ms windows, the mean power of those within 30 dB of the
    // loudest window. Not a broadcast loudness (no K-weighting): two renders of one engine are compared, not genres.
    double gatedDb(const tee::Audio& a)
    {
        const std::size_t w = static_cast<std::size_t>(a.sampleRate * 0.1);
        std::vector<double> p;
        for (std::size_t i = 0; w > 0 && i + w <= a.frames(); i += w)
        {
            double s = 0.0;
            for (std::size_t k = i; k < i + w; ++k)
                s += 0.5 * (double(a.left[k]) * a.left[k] + double(a.right[k]) * a.right[k]);
            p.push_back(s / static_cast<double>(w));
        }
        double top = 0.0;
        for (double x : p)
            top = std::max(top, x);
        double sum = 0.0;
        int n = 0;
        for (double x : p)
            if (x >= top * 1e-3)
            {
                sum += x;
                ++n;
            }
        return n ? 10.0 * std::log10(std::max(sum / n, 1e-30)) : -300.0;
    }

    int match(double target, const std::vector<std::string>& files)
    {
        std::vector<tee::Audio> audio(files.size());
        std::vector<double> gain(files.size());
        double lower = 0.0;
        for (std::size_t i = 0; i < files.size(); ++i)
        {
            std::string error;
            if (!tee::readWav(files[i], audio[i], error))
            {
                std::fprintf(stderr, "tee-fuzz: %s: %s\n", files[i].c_str(), error.c_str());
                return 2;
            }
            float peak = 0.0f;
            for (std::size_t k = 0; k < audio[i].frames(); ++k)
                peak = std::max({peak, std::fabs(audio[i].left[k]), std::fabs(audio[i].right[k])});
            gain[i] = target - gatedDb(audio[i]);
            const double headroom = -0.5 - 20.0 * std::log10(std::max(peak, 1e-9f));
            lower = std::max(lower, gain[i] - headroom);
        }
        for (std::size_t i = 0; i < files.size(); ++i)
        {
            const double g = gain[i] - lower;
            const float k = static_cast<float>(std::pow(10.0, g / 20.0));
            for (std::size_t n = 0; n < audio[i].frames(); ++n)
            {
                audio[i].left[n] *= k;
                audio[i].right[n] *= k;
            }
            std::string error;
            if (!tee::writeWav(files[i], audio[i], error))
            {
                std::fprintf(stderr, "tee-fuzz: %s: %s\n", files[i].c_str(), error.c_str());
                return 2;
            }
            std::printf("%s: %+.1f dB, to %.1f dBFS gated RMS\n", files[i].c_str(), g, target - lower);
        }
        return 0;
    }

    int usage();

    int play(const std::string& build, const std::string& kindArg, const std::string& comboArg, const std::string& wav,
             const std::vector<Value>& sets)
    {
        if (archetype(build) == nullptr)
        {
            std::fprintf(stderr, "tee-fuzz: no archetype %s\n", build.c_str());
            return 2;
        }
        Kind kind = Kind::Tour;
        bool found = false;
        for (Kind k : allKinds())
            if (kindArg == kindName(k))
            {
                kind = k;
                found = true;
            }
        if (!found)
            return usage();
        std::uint32_t mask = 0;
        const auto& sw = switches();
        if (comboArg == "all")
            mask = (1u << sw.size()) - 1u;
        else if (comboArg != "none")
        {
            std::string rest = comboArg;
            while (!rest.empty())
            {
                const auto plus = rest.find('+');
                const std::string one = rest.substr(0, plus);
                rest = plus == std::string::npos ? std::string() : rest.substr(plus + 1);
                bool hit = false;
                for (std::size_t k = 0; k < sw.size(); ++k)
                    if (sw[k].path == one)
                    {
                        mask |= 1u << k;
                        hit = true;
                    }
                if (!hit)
                {
                    std::fprintf(stderr, "tee-fuzz: no switch %s\n", one.c_str());
                    return 2;
                }
            }
        }
        Times tm;
        tee::test::Scenario sc = scenario(kind, build, mask, tm);
        // --set path=value: Spec writes at t = 0 over the switches (the bound stays the reference's own)
        for (const Value& x : sets)
            sc.at(0.0).spec(x.path, x.value);
        const tee::test::Run r = sc.run(tm.end);
        const Verdict v = judge(r, tm, archetype(build), true, mask == 0 ? 1e9 : referenceStep(build, kind));
        std::size_t ruled = 0;
        for (const Failure& f : v.failures)
            ruled += shiftCutRuled(comboName(mask), f) ? 1 : 0;
        const bool failed = v.failures.size() > ruled;
        std::printf("%s / %s / %s: %s; step %.1f, dc %.4f, idle %.0f rpm, revs %.0f rpm, %.1f %% of a core\n",
                    comboName(mask).c_str(), build.c_str(), kindName(kind),
                    failed ? "FAILED" : (ruled > 0 ? "ok (the shift cut's ruled bound)" : "ok"),
                    v.stepMax, v.dcMax, v.idle * 30.0 / 3.14159265, v.revMax * 30.0 / 3.14159265, 100.0 * v.coreShare);
        for (const Failure& f : v.failures)
            std::printf("  %s%s: %s\n", shiftCutRuled(comboName(mask), f) ? "ruled, " : "", f.check.c_str(),
                        f.detail.c_str());
        {
            const double refExcess = referenceExcess(build, kind);
            std::printf("  excess: %.2f at %.3f s (firing period %.1f ms); the reference's %.2f, so the bound %.2f: %s\n",
                        v.stepExcess, v.stepAt, 1000.0 * v.firingPeriod, refExcess, 2.0 * refExcess,
                        v.stepExcess < 0.0 || refExcess < 0.0 ? "n/a"
                        : v.stepExcess > 2.0 * refExcess    ? "OVER"
                                                            : "within");
        }
        if (!wav.empty())
        {
            tee::Audio a;
            a.sampleRate = r.result().rate;
            const auto l = r.left().samples(), rr = r.right().samples();
            a.left.assign(l.begin(), l.end());
            a.right.assign(rr.begin(), rr.end());
            std::string error;
            if (!tee::writeWav(wav, a, error))
                std::fprintf(stderr, "tee-fuzz: %s\n", error.c_str());
        }
        return failed ? 3 : 0;
    }

    int usage()
    {
        std::fprintf(stderr, "usage: tee-fuzz --seeds A-B | --seed N [--corners] [--seconds S] [--reduce] "
                             "[--build file.json] | --play <archetype> <tour|pull|launch|cold> <switches|none|all> "
                             "[--wav f.wav] | --match <dBFS> files... | --write-scenarios\n");
        return 2;
    }
} // namespace

int main(int argc, char** argv)
{
    Options o;
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : std::string(); };
        if (a == "--seeds")
        {
            const std::string s = next();
            const auto dash = s.find('-');
            if (dash == std::string::npos)
                return usage();
            o.first = std::strtoull(s.substr(0, dash).c_str(), nullptr, 10);
            o.last = std::strtoull(s.substr(dash + 1).c_str(), nullptr, 10);
        }
        else if (a == "--seed")
            o.first = o.last = std::strtoull(next().c_str(), nullptr, 10);
        else if (a == "--checks")
            gChecks = true;
        else if (a == "--dump")
            o.dump = true;
        else if (a == "--corners")
            o.corners = true;
        else if (a == "--reduce")
            o.reduce = true;
        else if (a == "--seconds")
            o.seconds = std::atof(next().c_str());
        else if (a == "--build")
            o.build = next();
        else if (a == "--play")
        {
            const std::string build = next(), kindArg = next(), comboArg = next();
            std::string wav;
            std::vector<Value> sets;
            while (i + 2 < argc && (std::string(argv[i + 1]) == "--wav" || std::string(argv[i + 1]) == "--set"))
            {
                const std::string opt = argv[++i];
                const std::string arg = next();
                if (opt == "--wav")
                    wav = arg;
                else
                {
                    const auto eq = arg.find('=');
                    if (eq == std::string::npos)
                        return usage();
                    sets.push_back({arg.substr(0, eq), std::atof(arg.substr(eq + 1).c_str())});
                }
            }
            return play(build, kindArg, comboArg, wav, sets);
        }
        else if (a == "--match")
        {
            const double target = std::atof(next().c_str());
            std::vector<std::string> files(argv + i + 1, argv + argc);
            return files.empty() ? usage() : match(target, files);
        }
        else if (a == "--write-scenarios")
        {
            const auto dir = scenarioDirectory();
            std::filesystem::create_directories(dir);
            for (Kind k : allKinds())
            {
                std::ofstream f(dir / (std::string(kindName(k)) + ".json"), std::ios::binary);
                f << scenarioJson(k, 0);
                std::printf("wrote %s\n", (dir / (std::string(kindName(k)) + ".json")).string().c_str());
            }
            return 0;
        }
        else
            return usage();
    }
    if (o.last < o.first)
        return usage();
    std::printf("tee-fuzz: %zu fields, %zu switches; the reference costs %.3f of a core here\n", fuzzFields().size(),
                switches().size(), costOfR());
    int failed = 0;
    for (std::uint64_t seed = o.first; seed <= o.last; ++seed)
    {
        const auto spec = o.build.empty() ? randomSpec(seed, o.corners) : fromFile(o.build);
        if (o.dump)
        {
            std::filesystem::create_directories("out/combo");
            const std::string file = "out/combo/spec-" + std::to_string(seed) + (o.corners ? "-corners" : "") + ".json";
            std::ofstream(file) << buildJson(spec, "seed " + std::to_string(seed), "tee-fuzz --dump: the seed's whole Spec");
            std::printf("wrote %s\n", file.c_str());
            continue;
        }
        const Verdict v = once(spec, seed, o.seconds);
        std::printf("seed %llu%s: %s, %.2f R", static_cast<unsigned long long>(seed), o.corners ? " (corners)" : "",
                    v.failures.empty() ? "ok" : "FAILED", v.costR);
        for (const Failure& f : v.failures)
            std::printf("\n  %s: %s", f.check.c_str(), f.detail.c_str());
        std::printf("\n");
        std::fflush(stdout);
        if (v.failures.empty())
            continue;
        ++failed;
        if (!o.reduce)
            continue;
        const std::string check = v.failures.front().check;
        const auto small = reduce(spec, seed, o.seconds, check);
        std::filesystem::create_directories("out/combo");
        const std::string file = "out/combo/fuzz-" + std::to_string(seed) + (o.corners ? "-corners" : "") + ".json";
        std::ofstream(file) << buildJson(small, "fuzz " + std::to_string(seed),
                                         "tee-fuzz --reduce: the fields that still fail '" + check +
                                             "' under the seed's driving; every other field at its default");
        std::printf("reduced to %zu fields:\n", small.size());
        for (const Value& x : small)
            std::printf("  %s = %g\n", x.path.c_str(), x.value);
        std::printf("shows it: tee-fuzz --build %s --seed %llu --seconds %g\n", file.c_str(),
                    static_cast<unsigned long long>(seed), o.seconds);
    }
    return failed ? 3 : 0;
}
