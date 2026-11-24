// docs/measurements/tools/qbench.cpp: package Q's measuring tool (not part of the build; build.sh compiles it against
// a configured build-dev tree). Two copies of the simulator, A and B (directories, each compiled with its own FMinus
// options through the environment variables patch-program.sh's copy of host/src/Program.cpp reads), played on the same
// builds and scenarios: for each pair, whether B's output is A's to the bit, and if not the difference's level under
// A's signal (dB, as the null test's nullDepth) and the worst sample's difference; and each one's cost, ns a 96 kHz
// sample: the wall time of the calls in which the engine sounded (as tee-bench --engine reads it: measure on a
// quiet machine), the least of --repeat runs and the
// spread (most over least), the runs alternated A, B, A, B so that a drift of the machine falls on both.
//
//   qbench [--a <dir>] [--b <dir>] [--aenv K=V,K=V] [--benv K=V,...] [--builds a,b] [--scenarios a,b]
//          [--repeat n] [--rate hz] [--block n] [--seconds s]
//   TEE_Q_OPT=0..3, TEE_Q_MATH=vector|libm|fast, TEE_Q_DENORM=flush|preserve, TEE_Q_KNOBS=n, TEE_Q_IR=<file>
#include "tee/Clock.h"
#include "tee/EnginePlayer.h"
#include "tee/Render.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::vector<std::string> split(const std::string& text)
    {
        std::vector<std::string> out;
        std::stringstream in(text);
        std::string item;
        while (std::getline(in, item, ','))
            if (!item.empty())
                out.push_back(item);
        return out;
    }

    const char* kVars[] = {"TEE_Q_OPT", "TEE_Q_MATH", "TEE_Q_DENORM", "TEE_Q_KNOBS", "TEE_Q_IR"};

    void applyEnv(const std::string& list)
    {
        for (const char* v : kVars)
            unsetenv(v);
        for (const std::string& kv : split(list))
        {
            const auto eq = kv.find('=');
            if (eq != std::string::npos)
                setenv(kv.substr(0, eq).c_str(), kv.substr(eq + 1).c_str(), 1);
        }
    }

    struct Side
    {
        std::unique_ptr<tee::EnginePlayer> player;
        double createSeconds = 0.0, compileSeconds = 0.0, linkSeconds = 0.0;
        std::size_t codeBytes = 0;
    };

    Side make(const std::string& dir, const std::string& env, double rate)
    {
        applyEnv(env);
        Side s;
        tee::EnginePlayer::Options o;
        o.directory = dir;
        o.sampleRate = rate;
        std::string errors;
        const auto t0 = std::chrono::steady_clock::now();
        s.player = tee::EnginePlayer::create(o, errors);
        if (!s.player)
        {
            std::fprintf(stderr, "qbench: %s: %s\n", dir.c_str(), errors.c_str());
            std::exit(1);
        }
        s.player->program().link(errors);
        s.createSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        s.compileSeconds = s.player->program().compileSeconds();
        s.linkSeconds = s.player->program().linkSeconds();
        s.codeBytes = s.player->program().codeBytes();
        applyEnv("");
        return s;
    }

    double depth(const std::vector<float>& a, const std::vector<float>& b, double& worst)
    {
        double sig = 0.0, dif = 0.0;
        const std::size_t n = std::min(a.size(), b.size());
        for (std::size_t i = 0; i < n; ++i)
        {
            const double d = (double)a[i] - (double)b[i];
            sig += (double)a[i] * a[i];
            dif += d * d;
            worst = std::max(worst, std::fabs(d));
        }
        if (dif == 0.0)
            return std::numeric_limits<double>::infinity();
        return 10.0 * std::log10(sig / dif);
    }
} // namespace

int main(int argc, char** argv)
{
    std::string root = TEE_SOURCE_DIR;
    std::string a = root + "/sim", b, aenv, benv, builds = "v8,large-diesel", scenarios = "free-revs,drive-car";
    int repeat = 3, block = 512;
    double rate = 48000.0, seconds = 0.0;
    for (int i = 1; i + 1 < argc; i += 2)
    {
        const std::string k = argv[i], v = argv[i + 1];
        if (k == "--a") a = v;
        else if (k == "--b") b = v;
        else if (k == "--aenv") aenv = v;
        else if (k == "--benv") benv = v;
        else if (k == "--builds") builds = v;
        else if (k == "--scenarios") scenarios = v;
        else if (k == "--repeat") repeat = std::max(1, std::atoi(v.c_str()));
        else if (k == "--rate") rate = std::atof(v.c_str());
        else if (k == "--block") block = std::atoi(v.c_str());
        else if (k == "--seconds") seconds = std::atof(v.c_str());
        else { std::fprintf(stderr, "qbench: unknown %s\n", k.c_str()); return 2; }
    }
    if (b.empty())
        b = a;
    Side A = make(a, aenv, rate);
    Side B = make(b, benv, rate);
    std::printf("A: %s [%s]: create %.2f s (compile %.2f, machine code %.2f), %zu bytes of code\n", a.c_str(),
                aenv.c_str(), A.createSeconds, A.compileSeconds, A.linkSeconds, A.codeBytes);
    std::printf("B: %s [%s]: create %.2f s (compile %.2f, machine code %.2f), %zu bytes of code\n\n", b.c_str(),
                benv.c_str(), B.createSeconds, B.compileSeconds, B.linkSeconds, B.codeBytes);
    std::printf("| build | scenario | A ns | A spread | B ns | B spread | B / A | to the bit | difference under the signal, dB | worst sample |\n");
    std::printf("|---|---|---:|---:|---:|---:|---:|---|---:|---:|\n");
    double sumA = 0.0, sumB = 0.0;
    for (const std::string& bn : split(builds))
    {
        const std::string bfile = bn.find('/') != std::string::npos ? bn : root + "/content/builds/" + bn + ".json";
        for (const std::string& sn : split(scenarios))
        {
            std::string err, warn;
            tee::Build buildA, buildB;
            tee::EngineScenario sc;
            if (!tee::loadBuild(*A.player, bfile, buildA, err, warn) || !tee::loadBuild(*B.player, bfile, buildB, err, warn)
                || !tee::loadEngineScenario(root + "/tests/sim/scenarios/" + sn + ".json", sc, err))
            {
                std::fprintf(stderr, "qbench: %s\n", err.c_str());
                return 1;
            }
            tee::RenderOptions ro;
            ro.blockSize = block;
            ro.seconds = seconds;
            double bestA = 1e30, worstA = 0.0, bestB = 1e30, worstB = 0.0;
            tee::RenderResult ra, rb;
            for (int r = 0; r < repeat; ++r)
            {
                ro.keepAudio = r == 0;
                for (int side = 0; side < 2; ++side)
                {
                    tee::RenderResult res;
                    tee::EnginePlayer& p = side == 0 ? *A.player : *B.player;
                    const bool ok = tee::renderEngine(p, side == 0 ? buildA : buildB, sc, ro, res, err);
                    if (!ok)
                    {
                        std::fprintf(stderr, "qbench: %s\n", err.c_str());
                        return 1;
                    }
                    const double ns = 1e9 * res.soundingSeconds / ((double)res.soundingFrames * 96000.0 / rate);
                    if (side == 0) { bestA = std::min(bestA, ns); worstA = std::max(worstA, ns); }
                    else { bestB = std::min(bestB, ns); worstB = std::max(worstB, ns); }
                    if (r == 0)
                        (side == 0 ? ra : rb) = std::move(res);
                }
            }
            double worst = 0.0;
            const double dl = depth(ra.audio.left, rb.audio.left, worst);
            const double dr = depth(ra.audio.right, rb.audio.right, worst);
            const bool same = ra.audio.left.size() == rb.audio.left.size()
                              && std::memcmp(ra.audio.left.data(), rb.audio.left.data(), ra.audio.left.size() * 4) == 0
                              && std::memcmp(ra.audio.right.data(), rb.audio.right.data(), ra.audio.right.size() * 4) == 0;
            sumA += bestA;
            sumB += bestB;
            std::printf("| %s | %s | %.0f | %.1f %% | %.0f | %.1f %% | %.3f | %s | %s | %.2g |\n", bn.c_str(), sn.c_str(), bestA,
                        100.0 * (worstA / bestA - 1.0), bestB, 100.0 * (worstB / bestB - 1.0), bestB / bestA,
                        same ? "yes" : "no", same ? "-" : (std::to_string(std::min(dl, dr)).substr(0, 6)).c_str(), worst);
            std::fflush(stdout);
        }
    }
    std::printf("| all | | %.0f | | %.0f | | %.3f | | | |\n", sumA, sumB, sumB / sumA);
    return 0;
}
