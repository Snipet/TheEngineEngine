// tests/sim/calibration/na/Probe.cpp: card N1's probe of a cylinder's gas exchange on the engine dyno (hidden: run by
// hand). It plays an archetype at full throttle, warm, held at each speed, one sample a call at 96 kHz, and reads
// cylinder 1's own state at each intake closing, so the volumetric efficiency can be split into its factors:
//
//     VE (air) = (p_ivc / p_air) x (T_air / T_ivc) x (V_ivc / V_d) x (1 - x_burned)
//
// p_ivc, T_ivc, V_ivc the gas's at intake closing; x_burned the burned share of what is trapped (the residual and what
// came back). Also the cylinder's mean pressure over the exhaust and the intake strokes (the pumping loop's two sides)
// and the Scope's port pressures. The gas constant is taken as air's for every gas (as the cylinder does).
//
//   PROBE_BUILD=straight-six-32-high-rev PROBE_RPM=3000,7600 PROBE_SET="valve.inPhaseRange=0" \
//     build-dev/bin/tee-tests-calibration-na "[.probe]"     (PROBE_SIM=DIR: another tree's sim/)
#include "tee/EnginePlayer.h"
#include "tee/Warm.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::vector<std::string> split(const std::string& s, char c)
    {
        std::vector<std::string> out;
        std::stringstream in(s);
        std::string item;
        while (std::getline(in, item, c))
            if (!item.empty())
                out.push_back(item);
        return out;
    }

    // tee::loadBuild reads only spec.<section>.<field>: a nested section (ecu.idle, ecu.limiter, ecu.knock, ecu.maps'
    // scalars) is dropped (the lead, 19:48). Writes the nested numbers and booleans by path after the load.
    void applyNested(tee::EnginePlayer& p, const std::filesystem::path& file)
    {
        std::ifstream in(file);
        const nlohmann::json doc = nlohmann::json::parse(in, nullptr, false);
        if (doc.is_discarded() || !doc.contains("spec"))
            return;
        std::function<void(const nlohmann::json&, const std::string&, int)> walk = [&](const nlohmann::json& j,
                                                                                         const std::string& path, int depth) {
            for (auto it = j.begin(); it != j.end(); ++it)
            {
                const std::string sub = path.empty() ? it.key() : path + "." + it.key();
                std::string e;
                if (it->is_object())
                    walk(*it, sub, depth + 1);
                else if (depth >= 2 && (it->is_number() || it->is_boolean()))
                    p.setSpec(sub, it->is_boolean() ? (it->get<bool>() ? 1.0 : 0.0) : it->get<double>(), e);
            }
        };
        walk(doc["spec"], "", 0);
    }

    struct Probe
    {
        std::unique_ptr<tee::EnginePlayer> p;
        double get(const std::string& path) const
        {
            double v = 0.0;
            std::string e;
            if (!p->read(path, v, e))
                return std::nan("");
            return v;
        }
        double tel(const char* name) const
        {
            const int i = p->telemetryIndex(name);
            return i < 0 ? std::nan("") : p->telemetry(std::size_t(i));
        }
        void warm()
        {
            std::string e;
            for (const tee::HeldValue& h : tee::warmEngine())
                p->instance().write(p->schema(), h.path, h.value, e);
        }
        void play(double seconds, const std::function<void()>& each = {})
        {
            float l = 0.0f, r = 0.0f;
            const long n = long(seconds * p->sampleRate());
            for (long i = 0; i < n; ++i)
            {
                if ((i & 63) == 0)
                    warm();
                p->process(&l, &r, 1);
                if (each)
                    each();
            }
        }
    };
} // namespace

TEST_CASE("probe: a cylinder's gas exchange at intake closing", "[.probe]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "economy-four-15";
    const char* rpmEnv = std::getenv("PROBE_RPM");
    const char* setEnv = std::getenv("PROBE_SET");
    const char* secEnv = std::getenv("PROBE_SECONDS");
    const double measure = secEnv ? std::atof(secEnv) : 1.0;

    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM")) : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 96000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    INFO(errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    std::filesystem::path file = slug;
    if (!std::filesystem::exists(file))
        file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
    REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    if (setEnv)
        for (const std::string& kv : split(setEnv, ';'))
        {
            const auto eq = kv.find('=');
            REQUIRE(eq != std::string::npos);
            std::string e;
            INFO(kv << ": " << e);
            REQUIRE(pr.p->setSpec(kv.substr(0, eq), std::atof(kv.c_str() + eq + 1), e));
        }

    std::vector<double> rpms;
    for (const std::string& s : split(rpmEnv ? rpmEnv : "2500,4000,6000", ','))
        rpms.push_back(std::atof(s.c_str()));

    auto& c = pr.p->controls();
    c.mode = 2;
    c.dynoRate = 0.0f;
    c.dynoSpeed = float(rpms.front() * M_PI / 30.0);
    c.ignition = 2;
    pr.play(1.0);
    c.ignition = 1;
    c.throttle = 1.0f;
    c.scopeOn = 1;
    pr.play(1.0);

    const double pAir = 101325.0, tAir = 288.15, R = 287.0;
    const double vd = pr.get("cyl[0].vd");
    std::printf("%s%s%s: cylinder 1, V_d %.1f cc; the air %.0f Pa, %.1f K\n", slug.c_str(), setEnv ? " with " : "",
                setEnv ? setEnv : "", vd * 1e6, pAir, tAir);
    std::printf("%6s %6s %6s | %6s %6s %6s %6s %6s = %6s | %7s %7s %7s %7s | %6s %6s %6s\n", "rpm", "T Nm", "VEtel",
                "p/pa", "Ta/T", "V/Vd", "1-xb", "", "VE", "pIVC", "TIVC", "pExStr", "pInStr", "man", "portI", "portE");
    std::printf("(walls: the heat cylinder 1 gave its walls over the cycles measured, over the heat its burns released)\n");
    for (const double rpm : rpms)
    {
        c.dynoSpeed = float(rpm * M_PI / 30.0);
        pr.play(1.2);
        double n = 0, sp = 0, st = 0, sv = 0, sx = 0, sm = 0;
        double exSum = 0, exN = 0, inSum = 0, inN = 0, torque = 0, tN = 0, man = 0, wall = 0, heat = 0, thr = 0, trap = 0;
        const int cyls = int(pr.tel("engine.cylinders"));
        const double dt = 1.0 / pr.p->sampleRate();
        std::vector<double> halves, burns;
        double tEvo = 0, pEvo = 0, nEvo = 0;
        pr.play(measure, [&] {
            const double ang = pr.get("cyl[0].ang");
            const double pc = pr.get("cyl[0].p");
            if (ang >= 180.0 && ang < 360.0)
                exSum += pc, exN += 1;
            else if (ang >= 360.0 && ang < 540.0)
                inSum += pc, inN += 1;
            const double tg = pr.get("cyl[0].t");
            wall += pr.get("cyl[0].hA") * (tg - pr.get("cyl[0].tw")) * dt;
            if (pr.get("cyl[0].sparkNow") != 0.0)
                heat += pr.get("cyl[0].qTot");
            if (pr.get("cyl[0].evoNow") != 0.0)
            {
                tEvo += pr.get("cyl[0].t"), pEvo += pc, nEvo += 1;
                halves.push_back(pr.tel("ecu.halfBurn"));
                burns.push_back(pr.get("cyl[0].qTot"));
            }
            if (pr.get("cyl[0].ivcNow") != 0.0)
            {
                const double m = pr.get("cyl[0].m");
                sp += pc;
                st += pr.get("cyl[0].t");
                sv += pr.get("cyl[0].v");
                sx += pr.get("cyl[0].mInert") / std::max(m, 1e-12);
                sm += m;
                n += 1;
            }
            thr += pr.get("intk.qThr") * pr.get("intk.pBox") / (287.0 * pr.get("intk.tAir")) * dt;
            for (int k = 0; k < cyls; ++k)
                if (pr.get("cyl[" + std::to_string(k) + "].ivcNow") != 0.0)
                    trap += pr.get("cyl[" + std::to_string(k) + "].m") - pr.get("cyl[" + std::to_string(k) + "].mInert");
            torque += pr.tel("engine.torque");
            man += pr.tel("engine.manifold");
            tN += 1;
        });
        if (n == 0)
        {
            std::printf("%6.0f: no intake closing seen\n", rpm);
            continue;
        }
        const double pI = sp / n, tI = st / n, vI = sv / n, xb = sx / n;
        const double f1 = pI / pAir, f2 = tAir / tI, f3 = vI / vd, f4 = 1.0 - xb;
        double portI = 0, portE = 0, piLo = 1e9, piHi = 0;
        const int deg = int(pr.get("scope.degrees"));
        for (int d = 0; d < deg; ++d)
        {
            const double pi = pr.get("scope.portIntake[" + std::to_string(d) + "]");
            piLo = std::min(piLo, pi);
            piHi = std::max(piHi, pi);
            portI += pi;
            portE += pr.get("scope.portExhaust[" + std::to_string(d) + "]");
        }
        portI /= std::max(deg, 1);
        portE /= std::max(deg, 1);
        (void)R;
        std::printf("%6.0f %6.0f %6.3f | %6.3f %6.3f %6.3f %6.3f %6s = %6.3f | %7.0f %7.1f %7.0f %7.0f | %6.1f %6.1f %6.1f\n", rpm,
                    torque / tN, pr.tel("engine.veAir"), f1, f2, f3, f4, "", f1 * f2 * f3 * f4, pI, tI, exSum / std::max(exN, 1.0),
                    inSum / std::max(inN, 1.0), man / tN / 1e3, portI / 1e3, portE / 1e3);
        std::printf("        air: through the throttle %.2f g/s, trapped fresh %.2f g/s (%.3f of it)\n", 1e3 * thr / measure,
                    1e3 * trap / measure, trap / std::max(thr, 1e-12));
        {
            std::vector<double> h = halves;
            std::sort(h.begin(), h.end());
            int over20 = 0, weak = 0;
            double qMean = 0;
            for (double q : burns)
                qMean += q / std::max<std::size_t>(burns.size(), 1);
            for (std::size_t k = 0; k < halves.size(); ++k)
                over20 += halves[k] > 20.0, weak += burns[k] < 0.5 * qMean;
            if (!h.empty())
                std::printf("        half-burn (tel, at each exhaust opening): min %.1f median %.1f max %.1f; %d of %zu over 20 deg; "
                            "%d burns under half the mean heat\n", h.front(), h[h.size() / 2], h.back(), over20, h.size(), weak);
        }
        std::printf("        intake port over the cycle: %.0f to %.0f kPa (swing +-%.0f)\n", piLo / 1e3, piHi / 1e3, (piHi - piLo) / 2e3);
        std::printf("        at exhaust opening: gas %.0f K, %.0f kPa (cylinder 1's mean over %d openings)\n", tEvo / std::max(nEvo, 1.0),
                    pEvo / std::max(nEvo, 1.0) / 1e3, int(nEvo));
        std::printf("        walls %.3f of the burns' heat (%.0f J of %.0f J), port gas %.0f K\n", wall / std::max(heat, 1e-9), wall, heat,
                    pr.tel("exhaust.portTemperature"));
    }
    SUCCEED();
}

// Idle, warm, on the bench: what the idle controller asks and gets. PROBE_BUILD, PROBE_SET as above.
TEST_CASE("probe: a warm idle", "[.probe-idle]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "v12-65";
    const char* setEnv = std::getenv("PROBE_SET");
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM")) : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 96000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    INFO(errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    std::filesystem::path file = slug;
    if (!std::filesystem::exists(file))
        file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
    REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    if (setEnv)
        for (const std::string& kv : split(setEnv, ';'))
        {
            const auto eq = kv.find('=');
            std::string e;
            REQUIRE(pr.p->setSpec(kv.substr(0, eq), std::atof(kv.c_str() + eq + 1), e));
        }
    auto& c = pr.p->controls();
    c.ignition = 2;
    pr.play(1.0);
    c.ignition = 1;
    std::printf("%s%s%s: warm idle on the bench\n%5s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %7s %7s\n", slug.c_str(), setEnv ? " with " : "",
                setEnv ? setEnv : "", "t s", "rpm", "target", "valve", "plate", "man kPa", "spark", "lambda", "IMEPg", "PMEP", "1-xb", "thr g/s", "trap g/s");
    for (int i = 0; i < 6; ++i)
    {
        double rpm = 0, n = 0, xb = 0, nx = 0, thr = 0, trap = 0;
        const int cyls = int(pr.tel("engine.cylinders"));
        pr.play(1.0, [&] {
            rpm += pr.tel("engine.speed") * 30.0 / M_PI;
            n += 1;
            thr += pr.get("intk.qThr") * pr.get("intk.pBox") / (287.0 * pr.get("intk.tAir"));
            for (int k = 0; k < cyls; ++k)
                if (pr.get("cyl[" + std::to_string(k) + "].ivcNow") != 0.0)
                    trap += pr.get("cyl[" + std::to_string(k) + "].m") - pr.get("cyl[" + std::to_string(k) + "].mInert");
        });
        for (int k = 0; k < 4; ++k)
            xb += pr.get("comb.dilution[" + std::to_string(k) + "]"), nx += 1;
        std::printf("%5d %6.0f %6.0f %6.3f %6.3f %6.1f %6.1f %6.3f %6.2f %6.2f %6.3f %7.2f %7.2f\n", i + 2, rpm / n, pr.tel("ecu.idleTarget") * 30.0 / M_PI,
                    pr.tel("ecu.idleValve"), pr.tel("engine.throttle"), pr.tel("engine.manifold") / 1e3, pr.tel("ecu.advance"),
                    pr.tel("ecu.lambda"), pr.tel("engine.imep") / 1e5, pr.tel("engine.pmep") / 1e5, 1.0 - xb / nx, 1e3 * thr / n, 1e3 * trap);
    }
    SUCCEED();
}

// The air's books over a window (Engine's running totals, card D): the throttle's air, the net mass in through the
// intake valves and the exhaust valves, the unburned gas trapped; at a warm idle on the bench and on the dyno at full
// load. PROBE_BUILD, PROBE_RPM (the dyno's speeds), PROBE_SET, PROBE_SIM as above.
TEST_CASE("probe: the air's books", "[.probe-books]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "flat-plane-v8-45";
    const char* rpmEnv = std::getenv("PROBE_RPM");
    const char* setEnv = std::getenv("PROBE_SET");
    std::vector<double> rpms{0.0};
    for (const std::string& s : split(rpmEnv ? rpmEnv : "3000,6000", ','))
        rpms.push_back(std::atof(s.c_str()));
    std::printf("%s%s%s: the air's books over 2 s (g/s); rpm 0 is a warm idle on the bench\n%7s %9s %9s %9s %9s %8s %8s\n", slug.c_str(),
                setEnv ? " with " : "", setEnv ? setEnv : "", "rpm", "throttle", "valves", "exhaust", "trapped", "val/thr", "plenum");
    for (const double rpm : rpms)
    {
        Probe pr;
        tee::EnginePlayer::Options options;
        options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                      : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 48000.0;
        std::string errors, warnings;
        pr.p = tee::EnginePlayer::create(options, errors);
        INFO(errors);
        REQUIRE(pr.p != nullptr);
        tee::EngineBuild b;
        std::filesystem::path file = slug;
        if (!std::filesystem::exists(file))
            file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
        REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
        tee::applyBuild(*pr.p, b);
        if (setEnv)
            for (const std::string& kv : split(setEnv, ';'))
            {
                const auto eq = kv.find('=');
                std::string e;
                REQUIRE(pr.p->setSpec(kv.substr(0, eq), std::atof(kv.c_str() + eq + 1), e));
            }
        auto& c = pr.p->controls();
        c.mode = rpm > 0.0 ? 2 : 0;
        c.dynoSpeed = float(rpm * M_PI / 30.0);
        c.ignition = 2;
        pr.play(1.0);
        c.ignition = 1;
        c.throttle = rpm > 0.0 ? 1.0f : 0.0f;
        pr.play(3.0);
        const double t0 = pr.get("airThrottle"), v0 = pr.get("airValves"), e0 = pr.get("gasExhaust"), a0 = pr.get("airTrapped"),
                     m0 = pr.get("intk.mPlenum");
        pr.play(2.0);
        const double k = 1e3 / 2.0;
        const double thr = (pr.get("airThrottle") - t0) * k, val = (pr.get("airValves") - v0) * k;
        std::printf("%7.0f %9.2f %9.2f %9.2f %9.2f %8.3f %8.3f\n", rpm, thr, val, (pr.get("gasExhaust") - e0) * k,
                    (pr.get("airTrapped") - a0) * k, val / thr, (pr.get("intk.mPlenum") - m0) * k);
    }
    SUCCEED();
}

// H9's overrun, watched: the V8 preset in neutral (both models on, warm air), on the limiter 1.5 s, lifted with a
// fuelled overrun; the collector's gas (where the bangs' store lights), the port's at the valve, cylinder 1's at its
// exhaust opening, and the bangs. PROBE_SIM as above.
TEST_CASE("probe: an overrun's gas temperatures", "[.probe-overrun]")
{
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 96000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    INFO(errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    REQUIRE(tee::loadBuild(*pr.p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "v8.json", b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    for (const char* kv : {"ecu.model=1", "fuel.model=1", "engine.wallHeat=1", "env.temperature=300", "ecu.overrun.fuel=0.25",
                           "ecu.overrun.retard=30"})
    {
        const std::string s = kv;
        const auto eq = s.find('=');
        std::string e;
        REQUIRE(pr.p->setSpec(s.substr(0, eq), std::atof(s.c_str() + eq + 1), e));
    }
    auto& c = pr.p->controls();
    c.mode = 1;
    c.ignition = 2;
    pr.play(1.5);
    c.ignition = 1;
    pr.play(1.0);
    c.throttle = 1.0f;
    pr.play(1.5);
    {
        double hold = 0, n = 0;
        pr.play(0.5, [&] { hold += pr.get("ecu.blow[0]"), n += 1; });
        std::printf("on the limiter at full throttle: the blowdown hold %.0f K (mean over 0.5 s)\n", hold / n);
    }
    c.throttle = 0.0f;
    std::printf("%6s %6s %8s %8s %8s %8s %6s\n", "t s", "rpm", "blowHold", "hdrWall", "EVO gas", "bangs", "flags");
    for (int i = 0; i < 10; ++i)
    {
        double tEvo = 0, nEvo = 0, burning = 0, fired = 0;
        const double b0 = pr.get("ecu.one.account.bangs");
        const double pj0 = pr.get("comb.popJ");
        double exMax = 0.0, lastMax = 0.0;
        pr.play(0.1, [&] {
            exMax = std::max(exMax, pr.get("cyl[0].exUnburnedJ"));
            lastMax = std::max(lastMax, pr.get("cyl[0].exUnburnedLast"));
            for (int k = 0; k < 8; ++k)
                if (pr.get("cyl[" + std::to_string(k) + "].evoNow") != 0.0)
                {
                    if (k == 0)
                        tEvo += pr.get("cyl[0].t"), nEvo += 1;
                    if (pr.get("cyl[" + std::to_string(k) + "].qTot") > 0.0)
                        fired += 1, burning += pr.get("cyl[" + std::to_string(k) + "].burnAt") >= 0.0 ? 1 : 0;
                }
        });
        std::printf("       dilution (burned share at intake closing) cyl 0 %.3f, cyl 1 %.3f; manifold %.1f kPa\n", pr.get("comb.dilution[0]"), pr.get("comb.dilution[1]"), pr.get("intk.pPlenum") * 1e-3);
        std::printf("       cyl 0: conserve %.0f, exUnburnedJ most %.2f J, its last most %.2f J, burnOut %.2f; burn %.0f deg, spark %.1f deg BTDC, EVO %.0f\n", pr.get("cyl[0].conserve"), exMax, lastMax, pr.get("cyl[0].bookOut"), pr.get("cyl[0].burnDur"), pr.tel("ecu.advance"), pr.get("cyl[0].evo"));
        std::printf("       exhaust openings of fired charges %.0f, still burning %.0f; to the store %.1f J (popJ), the store's fuel %.2f mg\n",
                    fired, burning, pr.get("comb.popJ") - pj0, (pr.get("ecu.one.account.fuel[0]") + pr.get("ecu.one.account.fuel[1]")) * 1e6);
        std::printf("%6.1f %6.0f %8.0f %8.0f %8.0f %8.0f %6d\n", 0.1 * (i + 1), pr.tel("engine.speed") * 30.0 / M_PI,
                    pr.get("ecu.blow[0]"), pr.get("tel.exhaust.wallTemperature[0]"), nEvo > 0 ? tEvo / nEvo : 0.0,
                    pr.get("ecu.one.account.bangs") - b0, int(pr.tel("ecu.flags")));
    }
    SUCCEED();
}

// A start on the engine dyno as card N2's harness makes it (the key on, then start, held warm): speed, state, the
// intake's air temperature and the charge heating's rise, every 0.2 s. PROBE_BUILD, PROBE_RPM (the dyno's), PROBE_SIM.
TEST_CASE("probe: the first second of a crank", "[.probe-crank]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "tractor-15-inline-six";
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 48000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    REQUIRE(tee::loadBuild(*pr.p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json"), b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    auto& c = pr.p->controls();
    c.mode = 2;
    c.dynoSpeed = float(1300.0 * M_PI / 30.0);
    c.ignition = 1;
    pr.play(0.1);
    c.ignition = 2;
    std::printf("%5s %6s %7s %8s %8s %8s %8s %7s\n", "t s", "rpm", "cyl1 t", "cyl1 p", "qTot J", "pPlen", "pBox", "tAir");
    for (int i = 0; i < 24; ++i)
    {
        double qMax = 0, tMax = 0, pMax = 0;
        pr.play(0.05, [&] {
            qMax = std::max(qMax, pr.get("cyl[0].qTot"));
            tMax = std::max(tMax, pr.get("cyl[0].t"));
            pMax = std::max(pMax, pr.get("cyl[0].p"));
        });
        std::printf("%5.2f %6.0f %7.0f %8.0f %8.1f %8.0f %8.0f %7.1f\n", 0.1 + 0.05 * (i + 1), pr.tel("engine.speed") * 30.0 / M_PI, tMax, pMax / 1e3,
                    qMax, pr.get("intk.pPlenum") / 1e3, pr.get("intk.pBox") / 1e3, pr.get("intk.tAir"));
    }
    SUCCEED();
}

TEST_CASE("probe: a start on the dyno", "[.probe-start]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "tractor-15-inline-six";
    const char* rpmEnv = std::getenv("PROBE_RPM");
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 48000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    INFO(errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    std::filesystem::path file = slug;
    if (!std::filesystem::exists(file))
        file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
    REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    auto& c = pr.p->controls();
    c.mode = 2;
    c.dynoSpeed = float((rpmEnv ? std::atof(rpmEnv) : 1300.0) * M_PI / 30.0);
    c.ignition = 1;
    std::printf("%5s %6s %5s %7s %7s %7s %7s %8s\n", "t s", "rpm", "state", "tAir", "rise", "tCool", "charge", "fuel g/s");
    for (int i = 0; i < 30; ++i)
    {
        const double t = 0.2 * i;
        c.ignition = t < 0.1 ? 1 : (t < 1.1 ? 2 : 1);
        c.throttle = t > 4.0 ? 1.0f : 0.0f;
        pr.play(0.2);
        std::printf("%5.1f %6.0f %5.0f %7.1f %7.2f %7.1f %7.1f %8.3f\n", t + 0.2, pr.tel("engine.speed") * 30.0 / M_PI,
                    pr.tel("engine.state"), pr.get("intk.tAir"), pr.get("intk.heatRise"), pr.get("intk.tCoolant"),
                    pr.tel("engine.chargeTemperature"), pr.tel("ecu.fuelFlow") * 1e3);
    }
    SUCCEED();
}

// A build's idle on the bench for PROBE_SECONDS (10), printing every 0.25 s what an intake with throttles a runner and a
// boost machine hold (X-022). PROBE_BUILD (a file), PROBE_SIM.
TEST_CASE("probe: an idle's intake and boost, every quarter second", "[.probe-x022]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string file = buildEnv ? buildEnv : std::string(TEE_SOURCE_DIR) + "/content/builds/combo/findings/x022-supercharger-itb-small-charge.json";
    const char* secEnv = std::getenv("PROBE_SECONDS");
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 48000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    auto& c = pr.p->controls();
    c.ignition = 2;
    std::printf("%5s %6s %5s %8s %8s %8s %8s %7s %7s\n", "t s", "rpm", "flags", "pEnds", "pPlen", "pBox", "boostP", "valve", "qThr");
    const int n = int((secEnv ? std::atof(secEnv) : 10.0) / 0.25);
    for (int i = 0; i < n; ++i)
    {
        if (i == 4)
            c.ignition = 1;
        if (std::getenv("PROBE_BLIP"))
            c.throttle = i >= 8 && (i % 4) < 2 ? 1.0f : 0.0f;
        if (std::getenv("PROBE_FINE") && i == 8)
        {
            for (int k = 0; k < 50; ++k)
            {
                pr.play(0.005);
                std::printf("  %6.3f %6.0f %5.0f pEnds %7.0f pBox %8.0f boost %8.0f qThr %8.4f mValves %8.5f\n", 2.0 + 0.005 * (k + 1),
                            pr.tel("engine.speed") * 30.0 / M_PI, pr.tel("engine.flags"), pr.get("intk.pEnds"), pr.get("intk.pBox"),
                            pr.tel("boost.pressure"), pr.get("intk.qThr"), pr.get("intk.mValves"));
            }
            continue;
        }
        pr.play(0.25);
        std::printf("%5.2f %6.0f %5.0f %8.0f %8.0f %8.0f %8.0f %7.3f %7.4f\n", 0.25 * (i + 1), pr.tel("engine.speed") * 30.0 / M_PI,
                    pr.tel("engine.flags"), pr.get("intk.pEnds"), pr.get("intk.pPlenum"), pr.get("intk.pBox"),
                    pr.tel("boost.pressure"), pr.tel("ecu.idleValve"), pr.get("intk.qThr"));
    }
    SUCCEED();
}

// p-idle-warm on the probe: in the car in neutral, held warm, the key at 0.25 s until 2 s, printing every PROBE_STEP
// (0.05) s through PROBE_SECONDS (3) the flare: the plenum, the knock retard held and the knocks heard, the spark's
// trim, the charge, the idle valve. PROBE_BUILD (slug), PROBE_SIM. (Not tee-render's run to the sample: the hold is
// written every 64 samples here, and a marginal start can go either way; WarmStartTest.cpp plays it as tee-render does)
TEST_CASE("probe: a warm start's flare", "[.probe-warm]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "big-bore-70-pushrod-v8";
    const double step = std::getenv("PROBE_STEP") ? std::atof(std::getenv("PROBE_STEP")) : 0.05;
    const double secs = std::getenv("PROBE_SECONDS") ? std::atof(std::getenv("PROBE_SECONDS")) : 3.0;
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 48000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    INFO(errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    std::filesystem::path file = slug;
    if (!std::filesystem::exists(file))
        file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
    REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    auto& c = pr.p->controls();
    c.mode = 1;
    c.lever = 0;
    c.clutch = 0;
    c.throttle = 0;
    std::printf("%5s %6s %3s %7s %7s %7s %8s %8s %6s %6s\n", "t s", "rpm", "st", "pPlen", "knock", "knocks", "trim",
                "veAir", "valve", "flags");
    const int n = int(secs / step + 0.5);
    for (int i = 0; i < n; ++i)
    {
        const double t = step * i;
        c.ignition = t < 0.25 ? 1 : (t < 2.0 ? 2 : 1);
        pr.play(step, [&] { pr.warm(); });
        std::printf("%5.2f %6.0f %3.0f %7.1f %7.3f %7.3f %8.4f %8.3f %6.3f %6.0f\n", t + step,
                    pr.tel("engine.speed") * 30.0 / M_PI, pr.tel("engine.state"), pr.get("intk.pPlenum") * 1e-3,
                    pr.get("ecu.one.knock.most"), double(pr.get("ecu.one.knock.count")), pr.get("ecu.trim"), pr.tel("engine.veAir"),
                    pr.tel("ecu.idleValve"), pr.tel("engine.flags"));
    }
    SUCCEED();
}

// The two books on the bench, shut throttle and warm: what the intake valves passed back into the runners (backflow)
// and drew from them, each sample's intk.mValves split by sign, against the plenum correction's ratio (valves' mass over
// the waves', intk.kWant + 1) and the plenum. Over 2 s after 3 s at each PROBE_RPM (700,1100). PROBE_BUILD (slug),
// PROBE_SIM, PROBE_THROTTLE (0).
TEST_CASE("probe: backflow into the runners against the plenum's books", "[.probe-backflow]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "big-bore-70-pushrod-v8";
    const char* rpmEnv = std::getenv("PROBE_RPM");
    const float thr = std::getenv("PROBE_THROTTLE") ? float(std::atof(std::getenv("PROBE_THROTTLE"))) : 0.0f;
    std::printf("%s, throttle %.2f; mass g/s, and the same volumes at the plenum's density\n%7s %8s %8s %8s %8s %7s %7s %7s\n",
                slug.c_str(), thr, "rpm", "draw", "back", "drawVol", "backVol", "m/vol", "ratio", "kPa");
    for (const std::string& s : split(rpmEnv ? rpmEnv : "700,1100", ','))
    {
        const double rpm = std::atof(s.c_str());
        Probe pr;
        tee::EnginePlayer::Options options;
        options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                      : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 48000.0;
        std::string errors, warnings;
        pr.p = tee::EnginePlayer::create(options, errors);
        REQUIRE(pr.p != nullptr);
        tee::EngineBuild b;
        std::filesystem::path file = slug;
        if (!std::filesystem::exists(file))
            file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
        REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
        tee::applyBuild(*pr.p, b);
        auto& c = pr.p->controls();
        c.mode = 2;
        c.dynoSpeed = float(rpm * M_PI / 30.0);
        c.ignition = 2;
        pr.play(1.0);
        c.ignition = 1;
        c.throttle = thr;
        pr.play(3.0);
        int nc = 0;
        while (nc < 16 && std::isfinite(pr.get("cyl[" + std::to_string(nc) + "].mI")))
            ++nc;
        std::vector<std::string> mPath, qPath, aPath;
        for (int i = 0; i < nc; ++i)
            mPath.push_back("cyl[" + std::to_string(i) + "].mI"), qPath.push_back("cyl[" + std::to_string(i) + "].qI"), aPath.push_back("cyl[" + std::to_string(i) + "].qIa");
        double back = 0.0, draw = 0.0, backV = 0.0, drawV = 0.0, ratio = 0.0, plen = 0.0, netA = 0.0;
        long n = 0;
        pr.play(2.0, [&] {
            const double rho = pr.get("intk.pPlenum") / (287.0 * pr.get("intk.tAir"));
            for (int i = 0; i < nc; ++i)
            {
                const double m = pr.get(mPath[std::size_t(i)]), q = pr.get(qPath[std::size_t(i)]) * rho;
                (m > 0.0 ? back : draw) += m;
                (q > 0.0 ? backV : drawV) += q;
                netA += pr.get(aPath[std::size_t(i)]) * rho;
            }
            ratio += pr.get("intk.kWant") + 1.0;
            plen += pr.get("intk.pPlenum");
            ++n;
        });
        const double k = 1e3 / n;
        std::printf("%7.0f %8.2f %8.2f %8.2f %8.2f %7.3f %7.3f %7.1f  netQIa %.3f netQI %.3f\n", rpm, -draw * k, back * k, -drawV * k, backV * k,
                    (draw + back) / (drawV + backV), ratio / n, plen / n * 1e-3, netA * k, (drawV + backV) * k);
    }
    SUCCEED();
}

// The idle loop's plant (the lead, 20:14): in the car in neutral, held warm, the key, 8 s of the ECU's own idle; then
// the valve held at its mean over the last 2 s (needs a simulator with ecu.holdOn and ecu.holdValve: a scratch tree),
// 3 s; then stepped by PROBE_STEP (0.02), 4 s. Prints the speed every 0.1 s after the hold. PROBE_BUILD (slug),
// PROBE_SIM.
TEST_CASE("probe: the idle valve's step on a warm idle", "[.probe-idlestep]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "big-bore-70-pushrod-v8";
    const double dv = std::getenv("PROBE_STEP") ? std::atof(std::getenv("PROBE_STEP")) : 0.02;
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 48000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    INFO(errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    const std::filesystem::path file =
        std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
    REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    applyNested(*pr.p, file);
    auto& c = pr.p->controls();
    c.mode = 1;
    c.ignition = 1;
    pr.play(0.25, [&] { pr.warm(); });
    c.ignition = 2;
    pr.play(1.75, [&] { pr.warm(); });
    c.ignition = 1;
    pr.play(4.0, [&] { pr.warm(); });
    double vs = 0.0, rs = 0.0;
    long n = 0;
    pr.play(2.0, [&] {
        pr.warm();
        vs += pr.tel("ecu.idleValve");
        rs += pr.tel("engine.speed");
        ++n;
    });
    const double v0 = vs / n;
    std::printf("%s: the ECU's idle over 6-8 s: valve %.4f, %.0f rpm; idle target %.0f rpm\n", slug.c_str(), v0,
                rs / n * 30.0 / M_PI, pr.tel("ecu.idleTarget") * 30.0 / M_PI);
    std::string e;
    pr.p->instance().write(pr.p->schema(), "ecu.holdOn", std::getenv("PROBE_HOLD") ? std::atof(std::getenv("PROBE_HOLD")) : 1.0, e);
    pr.p->instance().write(pr.p->schema(), "ecu.holdValve", v0, e);
    INFO(e);
    std::printf("%5s %6s %7s\n", "t s", "rpm", "kPa");
    for (int i = 0; i < 70; ++i)
    {
        if (i == 30)
            pr.p->instance().write(pr.p->schema(), "ecu.holdValve", v0 + dv, e);
        pr.play(0.1, [&] { pr.warm(); });
        std::printf("%5.1f %6.0f %7.1f\n", 8.1 + 0.1 * i, pr.tel("engine.speed") * 30.0 / M_PI, pr.get("intk.pPlenum") * 1e-3);
    }
    SUCCEED();
}

// The idle's torque against speed on the dyno, the idle valve held at PROBE_VALVE (0.1) and the spark's trim at 1
// (ecu.holdOn 2: a scratch tree), warm, 3 s at each of PROBE_RPM (500,600,700,800,900,1000), the mean of the last
// second. A torque that rises with speed is an idle the engine cannot hold by itself. PROBE_BUILD, PROBE_SIM.
TEST_CASE("probe: the idle's torque against speed, valve and spark held", "[.probe-idletorque]")
{
    const char* buildEnv = std::getenv("PROBE_BUILD");
    const std::string slug = buildEnv ? buildEnv : "big-bore-70-pushrod-v8";
    const double valve = std::getenv("PROBE_VALVE") ? std::atof(std::getenv("PROBE_VALVE")) : 0.1;
    const char* rpmEnv = std::getenv("PROBE_RPM");
    std::printf("%s, valve %.3f, trim 1\n%6s %9s %8s %8s\n", slug.c_str(), valve, "rpm", "torque", "kPa", "veAir");
    for (const std::string& s : split(rpmEnv ? rpmEnv : "500,600,700,800,900,1000", ','))
    {
        Probe pr;
        tee::EnginePlayer::Options options;
        options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                      : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 48000.0;
        std::string errors, warnings;
        pr.p = tee::EnginePlayer::create(options, errors);
        REQUIRE(pr.p != nullptr);
        tee::EngineBuild b;
        const std::filesystem::path file =
            std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
        REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
        tee::applyBuild(*pr.p, b);
        applyNested(*pr.p, file);
        std::string e;
        pr.p->instance().write(pr.p->schema(), "ecu.holdOn", 2.0, e);
        pr.p->instance().write(pr.p->schema(), "ecu.holdValve", valve, e);
        auto& c = pr.p->controls();
        c.mode = 2;
        c.dynoSpeed = float(std::atof(s.c_str()) * M_PI / 30.0);
        c.ignition = 2;
        pr.play(1.0, [&] { pr.warm(); });
        c.ignition = 1;
        pr.play(2.0, [&] { pr.warm(); });
        double tq = 0.0, kp = 0.0, ve = 0.0;
        long n = 0;
        pr.play(1.0, [&] {
            pr.warm();
            tq += pr.tel("engine.torque");
            kp += pr.get("intk.pPlenum");
            ve += pr.tel("engine.veAir");
            ++n;
        });
        std::printf("%6s %9.1f %8.1f %8.3f\n", s.c_str(), tq / n, kp / n * 1e-3, ve / n);
    }
    SUCCEED();
}

// The boost machine's charge path from the key, every PROBE_STEP (0.001) s for PROBE_SECONDS (0.5), until anything is
// non-finite (X-024, X-026: an explicit step around a small volume). PROBE_BUILD (a file), PROBE_SIM, PROBE_THROTTLE (0).
TEST_CASE("probe: the boost charge path from the key", "[.probe-boost]")
{
    const std::string file = std::getenv("PROBE_BUILD") ? std::getenv("PROBE_BUILD")
                                                        : std::string(TEE_SOURCE_DIR) + "/content/builds/combo/findings/x026-single-big-blower.json";
    const double step = std::getenv("PROBE_STEP") ? std::atof(std::getenv("PROBE_STEP")) : 0.001;
    const double secs = std::getenv("PROBE_SECONDS") ? std::atof(std::getenv("PROBE_SECONDS")) : 0.5;
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 48000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    auto& c = pr.p->controls();
    c.ignition = 2;
    c.throttle = std::getenv("PROBE_THROTTLE") ? float(std::atof(std::getenv("PROBE_THROTTLE"))) : 0.0f;
    std::printf("%6s %6s %9s %9s %9s %9s %9s %9s %9s\n", "t s", "rpm", "v1 kPa", "v2 kPa", "mIc g/s", "pdDel", "mDraw", "v1 K",
                "vol1 L");
    const int n = int(secs / step + 0.5);
    const bool blips = std::getenv("PROBE_BLIPS") != nullptr;
    const double from = std::getenv("PROBE_FROM") ? std::atof(std::getenv("PROBE_FROM")) : 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double tNow = step * i;
        if (blips)
        {
            c.ignition = tNow < 1.0 ? 2 : 1;
            c.throttle = tNow > 2.0 && std::fmod(tNow, 1.0) < 0.5 ? 1.0f : 0.0f;
        }
        pr.play(step);
        if (tNow + step < from)
            continue;
        const double v1 = pr.get("boost.mach.v1.p"), v2 = pr.get("boost.mach.v2.p");
        std::printf("%6.3f %6.0f %9.2f %9.2f %9.3f %9.3f %9.3f %9.1f %9.4f\n", step * (i + 1), pr.tel("engine.speed") * 30.0 / M_PI,
                    v1 * 1e-3, v2 * 1e-3, pr.get("boost.mach.mIc") * 1e3, pr.get("boost.mach.pd.mDel") * 1e3,
                    pr.get("boost.mach.mDraw") * 1e3, pr.get("boost.mach.v1.t"), pr.get("boost.mach.v1.vol") * 1e3);
        std::printf("       p1 %.2f kPa, leak %.3f g/s, inlet net %.3f g/s, bypass open %.3f, scBypass %.0f, v1 mass %.4f g\n",
                    pr.get("boost.mach.p1") * 1e-3, pr.get("boost.mach.pd.mLeak") * 1e3, pr.get("boost.mach.mInlet") * 1e3,
                    pr.get("boost.mach.bypassOpen"), pr.get("boost.mach.scBypass"), pr.get("boost.mach.v1.m") * 1e3);
        std::printf("       c %.4g m %.4g a %.4g b %.4g kF %.4g sc %.0f turbos %.0f\n", pr.get("boost.mach.dbgC"), pr.get("boost.mach.dbgM"), pr.get("boost.mach.dbgA"), pr.get("boost.mach.dbgB"), pr.get("boost.mach.kFilter"), pr.get("boost.mach.sc"), pr.get("boost.mach.turbos"));
        if (!std::isfinite(v1) || !std::isfinite(v2) || std::fabs(v1) > 1e8)
            break;
    }
    SUCCEED();
}

// X022Test's fast form against PROBE_SIM (a bisect): the X-022 build, the key, two blips, 4 s, the guard's faults
TEST_CASE("probe: X-022's blips against a tree", "[.probe-x022t]")
{
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 48000.0;
    std::string errors, warnings;
    auto p = tee::EnginePlayer::create(options, errors);
    REQUIRE(p != nullptr);
    tee::EngineBuild b;
    REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "combo" / "findings" /
                                   "x022-supercharger-itb-small-charge.json", b, errors, warnings));
    tee::applyBuild(*p, b);
    auto& c = p->controls();
    const int block = 256;
    float l[block], r[block];
    const int flags = p->telemetryIndex("engine.flags");
    int faults = 0;
    double first = -1.0;
    for (int i = 0; i < int(4.0 * options.sampleRate / block); ++i)
    {
        const double t = i * block / options.sampleRate;
        c.ignition = t < 1.0 ? 2 : 1;
        c.throttle = t > 2.0 && std::fmod(t, 1.0) < 0.5 ? 1.0f : 0.0f;
        p->process(l, r, block);
        if ((int(p->telemetry(std::size_t(flags))) & 4) != 0)
        {
            if (first < 0.0)
                first = t;
            ++faults;
        }
    }
    std::printf("X-022 blips: faults %d, the first at %.3f s\n", faults, first);
    SUCCEED();
}

// The boosted books on the dyno (N3's finding: the machine draws ~8 % more than the throttle and the valves book):
// PROBE_BUILD (slug), PROBE_RPM (1700), full load, 6 s to settle then 2 s booked: the machine's draw (sumMDraw), the
// throttle's (airThrottle), the valves' (airValves), and what each is taken at: pBox, the box's depression, the
// machine's v2, the intake's tAir and its heating
TEST_CASE("probe: the boosted books", "[.probe-boostbooks]")
{
    const std::string slug = std::getenv("PROBE_BUILD") ? std::getenv("PROBE_BUILD") : "pickup-diesel-67-inline-six";
    const double rpm = std::getenv("PROBE_RPM") ? std::atof(std::getenv("PROBE_RPM")) : 1700.0;
    Probe pr;
    tee::EnginePlayer::Options options;
    options.directory = std::getenv("PROBE_SIM") ? std::filesystem::path(std::getenv("PROBE_SIM"))
                                                  : std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    options.sampleRate = 48000.0;
    std::string errors, warnings;
    pr.p = tee::EnginePlayer::create(options, errors);
    REQUIRE(pr.p != nullptr);
    tee::EngineBuild b;
    const std::filesystem::path file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
    REQUIRE(tee::loadBuild(*pr.p, file, b, errors, warnings));
    tee::applyBuild(*pr.p, b);
    auto& c = pr.p->controls();
    c.mode = 2;
    c.dynoSpeed = float(rpm * M_PI / 30.0);
    c.ignition = 2;
    pr.play(1.0, [&] { pr.warm(); });
    c.ignition = 1;
    c.throttle = 1.0f;
    const double settle = std::getenv("PROBE_SETTLE") ? std::atof(std::getenv("PROBE_SETTLE")) : 5.0;
    const double meas = std::getenv("PROBE_MEASURE") ? std::atof(std::getenv("PROBE_MEASURE")) : 2.0;
    pr.play(settle, [&] { pr.warm(); });
    const double d0 = pr.get("boost.mach.sumMDraw"), t0 = pr.get("airThrottle"), v0 = pr.get("airValves"),
                 c0 = pr.get("boost.mach.sumMComp"), m0 = pr.get("intk.mPlenum"), vm0 = pr.get("boost.mach.v2.m") + pr.get("boost.mach.v1.m");
    double pBox = 0, boxP = 0, v2p = 0, v2t = 0, tAir = 0, rise = 0;
    long n = 0;
    pr.play(meas, [&] {
        pr.warm();
        pBox += pr.get("intk.pBox"); boxP += pr.get("intk.box.p"); v2p += pr.get("boost.mach.v2.p"); v2t += pr.get("boost.mach.v2.t");
        tAir += pr.get("intk.tAir"); rise += pr.get("intk.heatRise"); ++n;
    });
    const double k = 1e3 / meas;
    std::printf("%s at %.0f rpm, full load, booked over the window (g/s): compressor %.2f, machine's draw %.2f, throttle %.2f, valves %.2f; "
                "plenum store %.3f, the machine's v1+v2 store %.3f\n",
                slug.c_str(), rpm, (pr.get("boost.mach.sumMComp") - c0) * k, (pr.get("boost.mach.sumMDraw") - d0) * k,
                (pr.get("airThrottle") - t0) * k, (pr.get("airValves") - v0) * k, (pr.get("intk.mPlenum") - m0) * k,
                (pr.get("boost.mach.v2.m") + pr.get("boost.mach.v1.m") - vm0) * k);
    std::printf("  means: intk.pBox %.2f kPa, box.p %.2f kPa, machine's v2.p %.2f kPa; intk.tAir %.1f K (heatRise %.1f), v2.t %.1f K; "
                "intake.model %.0f, itbs %.0f\n", pBox / n * 1e-3, boxP / n * 1e-3, v2p / n * 1e-3, tAir / n, rise / n, v2t / n,
                pr.get("intk.model"), pr.get("intk.itbs"));
    SUCCEED();
}
