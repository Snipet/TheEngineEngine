// docs/measurements/tools/qbench.cpp: package Q's measuring tool (not part of the build; build.sh compiles it against
// a configured build-dev tree). Two copies of the simulator, A and B (directories, each compiled with its own FMinus
// options through the environment variables patch-program.sh's copy of host/src/Program.cpp reads), played on the same
// builds and scenarios: for each pair, whether B's output is A's to the bit, and if not the difference's level under
// A's signal (dB, as the null test's nullDepth) and the worst sample's difference; and each one's cost, ns a 96 kHz
// sample: the wall time of the calls in which the engine sounded (as tee-bench --engine reads it: measure on a
// quiet machine), the least of --repeat runs and the
// spread (most over least), and the ratio of the least processor time of the whole renders (which a busy host's
// stolen time does not lengthen), the runs alternated A, B, A, B so that a drift of the machine falls on both. The engine is
// a chaotic system: a change in the last bit of one number makes the two waveforms part company within a second or
// two, though they sound alike, so beside the difference sample for sample (which then reads near 0 dB) it reports
// how long the two stayed within -80 dB of each other, the long-term spectrum's third-octave bands' worst difference,
// the worst difference of a quarter second's level, and the engine's speed apart (rpm).
//
//   qbench [--a <dir>] [--b <dir>] [--aenv K=V,K=V] [--benv K=V,...] [--builds a,b] [--scenarios a,b]
//          [--repeat n] [--rate hz] [--block n] [--seconds s] [--bands 1 (each band's difference, B over A)] [--sizes 1 (A's code by module)]
//   TEE_Q_OPT=0..3, TEE_Q_MATH=vector|libm|fast, TEE_Q_DENORM=flush|preserve, TEE_Q_KNOBS=n, TEE_Q_IR=<file>, TEE_Q_DEFINES=NAME[=v];NAME (the program's macros: TEE_FAST)
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
#include <map>
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

    const char* kVars[] = {"TEE_Q_OPT", "TEE_Q_MATH", "TEE_Q_DENORM", "TEE_Q_KNOBS", "TEE_Q_IR", "TEE_Q_DEFINES"};

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

    // An in-place radix-2 FFT (n a power of 2), for the band levels below
    void fft(std::vector<double>& re, std::vector<double>& im)
    {
        const std::size_t n = re.size();
        for (std::size_t i = 1, j = 0; i < n; ++i)
        {
            std::size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
            {
                std::swap(re[i], re[j]);
                std::swap(im[i], im[j]);
            }
        }
        for (std::size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = -2.0 * M_PI / (double)len;
            for (std::size_t i = 0; i < n; i += len)
                for (std::size_t k = 0; k < len / 2; ++k)
                {
                    const double c = std::cos(ang * k), sn = std::sin(ang * k);
                    const double xr = re[i + k + len / 2] * c - im[i + k + len / 2] * sn;
                    const double xi = re[i + k + len / 2] * sn + im[i + k + len / 2] * c;
                    re[i + k + len / 2] = re[i + k] - xr;
                    im[i + k + len / 2] = im[i + k] - xi;
                    re[i + k] += xr;
                    im[i + k] += xi;
                }
        }
    }

    // The long-term spectrum (Welch: 8192-point Hann windows, half overlapped) in third-octave bands from 25 Hz to
    // 20 kHz, dB
    std::vector<double> bands(const std::vector<float>& x, double rate)
    {
        const std::size_t n = 8192;
        std::vector<double> power(n / 2, 0.0);
        for (std::size_t at = 0; at + n <= x.size(); at += n / 2)
        {
            std::vector<double> re(n), im(n, 0.0);
            for (std::size_t i = 0; i < n; ++i)
                re[i] = x[at + i] * (0.5 - 0.5 * std::cos(2.0 * M_PI * (double)i / (double)n));
            fft(re, im);
            for (std::size_t k = 0; k < n / 2; ++k)
                power[k] += re[k] * re[k] + im[k] * im[k];
        }
        std::vector<double> out;
        for (double fc = 25.0; fc < 20500.0; fc *= std::pow(2.0, 1.0 / 3.0))
        {
            const double lo = fc / std::pow(2.0, 1.0 / 6.0), hi = fc * std::pow(2.0, 1.0 / 6.0);
            double sum = 1e-30;
            for (std::size_t k = 1; k < n / 2; ++k)
            {
                const double f = (double)k * rate / (double)n;
                if (f >= lo && f < hi)
                    sum += power[k];
            }
            out.push_back(10.0 * std::log10(sum));
        }
        return out;
    }

    // The largest difference of a band's level, over the bands within 50 dB of the loudest
    double bandDifference(const std::vector<float>& a, const std::vector<float>& b, double rate)
    {
        const std::vector<double> x = bands(a, rate), y = bands(b, rate);
        const double top = *std::max_element(x.begin(), x.end());
        double worst = 0.0;
        for (std::size_t i = 0; i < x.size(); ++i)
            if (x[i] > top - 50.0)
                worst = std::max(worst, std::fabs(x[i] - y[i]));
        return worst;
    }

    // The largest difference of a quarter second's level (dB), over the quarter seconds within 40 dB of the loudest
    double levelDifference(const std::vector<float>& a, const std::vector<float>& b, double rate)
    {
        const std::size_t w = (std::size_t)(0.25 * rate);
        std::vector<double> la, lb;
        for (std::size_t at = 0; at + w <= std::min(a.size(), b.size()); at += w)
        {
            double sa = 1e-30, sb = 1e-30;
            for (std::size_t i = at; i < at + w; ++i)
            {
                sa += (double)a[i] * a[i];
                sb += (double)b[i] * b[i];
            }
            la.push_back(10.0 * std::log10(sa / (double)w));
            lb.push_back(10.0 * std::log10(sb / (double)w));
        }
        if (la.empty())
            return 0.0;
        const double top = *std::max_element(la.begin(), la.end());
        double worst = 0.0;
        for (std::size_t i = 0; i < la.size(); ++i)
            if (la[i] > top - 40.0)
                worst = std::max(worst, std::fabs(la[i] - lb[i]));
        return worst;
    }

    // How long the two stay together: the first 10 ms window (after the first sound) whose difference is within 80 dB
    // of its signal no longer, in seconds from the start (the length if never)
    double together(const std::vector<float>& a, const std::vector<float>& b, double rate)
    {
        const std::size_t w = (std::size_t)(0.01 * rate);
        const std::size_t n = std::min(a.size(), b.size());
        for (std::size_t at = 0; at + w <= n; at += w)
        {
            double sig = 0.0, dif = 0.0;
            for (std::size_t i = at; i < at + w; ++i)
            {
                sig += (double)a[i] * a[i];
                const double d = (double)a[i] - (double)b[i];
                dif += d * d;
            }
            if (dif > 0.0 && (sig == 0.0 || 10.0 * std::log10(sig / dif) < 80.0))
                return (double)at / rate;
        }
        return (double)n / rate;
    }

    // The engine's speed (telemetry engine.speed) apart: the largest difference of a call's row, rpm, and the mean
    void speedDifference(const tee::RenderResult& a, const tee::RenderResult& b, double& worst, double& mean)
    {
        worst = mean = 0.0;
        const int ca = a.column("engine.speed"), cb = b.column("engine.speed");
        if (ca < 0 || cb < 0)
            return;
        const std::size_t n = std::min(a.rowCount(), b.rowCount());
        for (std::size_t r = 0; r < n; ++r)
        {
            const double d = std::fabs((double)a.at(r, (std::size_t)ca) - (double)b.at(r, (std::size_t)cb)) * 9.5492966;
            worst = std::max(worst, d);
            mean += d;
        }
        if (n > 0)
            mean /= (double)n;
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
    bool showBands = false, showSizes = false;
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
        else if (k == "--bands") showBands = v == "1";
        else if (k == "--sizes") showSizes = v == "1";
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
    if (showSizes)
    {
        // A's machine code by module (FMinus's code ranges), largest first
        std::string err;
        std::map<std::string, std::size_t> bytes;
        std::map<std::string, int> count;
        for (const auto& r : A.player->program().fminus().codeRanges(err))
        {
            bytes[r.module.empty() ? "(functions)" : r.module] += r.size;
            count[r.module.empty() ? "(functions)" : r.module] += 1;
        }
        std::vector<std::pair<std::size_t, std::string>> v;
        for (const auto& [m, b] : bytes)
            v.push_back({b, m});
        std::sort(v.rbegin(), v.rend());
        std::printf("| module | bytes of code | functions |\n|---|---:|---:|\n");
        for (const auto& [b, m] : v)
            std::printf("| %s | %zu | %d |\n", m.c_str(), b, count[m]);
        std::printf("\n");
    }
    std::printf("| build | scenario | A ns | A spread | B ns | B spread | B / A | to the bit | difference under the signal, dB | together for, s | bands, worst dB | quarter-second level, worst dB | speed apart, worst / mean rpm | B / A, processor time |\n");
    std::printf("|---|---|---:|---:|---:|---:|---:|---|---:|---:|---:|---:|---:|---:|\n");
    double sumA = 0.0, sumB = 0.0, sumCa = 0.0, sumCb = 0.0;
    for (const std::string& bn : split(builds))
    {
        const std::string bfile = bn.find('/') != std::string::npos ? bn : root + "/content/builds/" + bn + ".json";
        for (const std::string& sn : split(scenarios))
        {
            std::string err, warn;
            tee::EngineBuild buildA, buildB;
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
            double bestA = 1e30, worstA = 0.0, bestB = 1e30, worstB = 0.0, cpuA = 1e30, cpuB = 1e30;
            tee::RenderResult ra, rb;
            for (int r = 0; r < repeat; ++r)
            {
                ro.keepAudio = r == 0;
                for (int side = 0; side < 2; ++side)
                {
                    tee::RenderResult res;
                    tee::EnginePlayer& p = side == 0 ? *A.player : *B.player;
                    const double c0 = tee::threadCpuSeconds();
                    const bool ok = tee::renderEngine(p, side == 0 ? buildA : buildB, sc, ro, res, err);
                    const double cpu = tee::threadCpuSeconds() - c0;
                    (side == 0 ? cpuA : cpuB) = std::min(side == 0 ? cpuA : cpuB, cpu);
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
            const double tog = same ? (double)ra.audio.left.size() / rate : std::min(together(ra.audio.left, rb.audio.left, rate), together(ra.audio.right, rb.audio.right, rate));
            const double bd = same ? 0.0 : std::max(bandDifference(ra.audio.left, rb.audio.left, rate), bandDifference(ra.audio.right, rb.audio.right, rate));
            const double ld = same ? 0.0 : std::max(levelDifference(ra.audio.left, rb.audio.left, rate), levelDifference(ra.audio.right, rb.audio.right, rate));
            if (showBands && !same)
            {
                // each third-octave band's level, B over A (dB), left and right averaged in power
                std::vector<double> xl = bands(ra.audio.left, rate), xr = bands(ra.audio.right, rate);
                std::vector<double> yl = bands(rb.audio.left, rate), yr = bands(rb.audio.right, rate);
                std::printf("bands %s %s:", bn.c_str(), sn.c_str());
                double fc = 25.0;
                for (std::size_t i = 0; i < xl.size(); ++i, fc *= std::pow(2.0, 1.0 / 3.0))
                {
                    const double a = 10.0 * std::log10(std::pow(10.0, xl[i] / 10.0) + std::pow(10.0, xr[i] / 10.0));
                    const double b2 = 10.0 * std::log10(std::pow(10.0, yl[i] / 10.0) + std::pow(10.0, yr[i] / 10.0));
                    std::printf(" %.0f:%+.2f", fc, b2 - a);
                }
                std::printf("\n");
            }
            double sw = 0.0, sm = 0.0;
            speedDifference(ra, rb, sw, sm);
            sumA += bestA;
            sumB += bestB;
            char dtext[32];
            std::snprintf(dtext, sizeof dtext, "%.1f", std::min(dl, dr));
            std::printf("| %s | %s | %.0f | %.1f %% | %.0f | %.1f %% | %.3f | %s | %s | %.2f | %.2f | %.2f | %.0f / %.1f | %.3f |\n", bn.c_str(), sn.c_str(), bestA,
                        100.0 * (worstA / bestA - 1.0), bestB, 100.0 * (worstB / bestB - 1.0), bestB / bestA,
                        same ? "yes" : "no", same ? "-" : dtext, tog, bd, ld, sw, sm, cpuB / cpuA);
            sumCa += cpuA;
            sumCb += cpuB;
            (void)worst;
            std::fflush(stdout);
        }
    }
    std::printf("| all | | %.0f | | %.0f | | %.3f | | | | | | | %.3f |\n", sumA, sumB, sumB / sumA, sumCb / sumCa);
    return 0;
}
