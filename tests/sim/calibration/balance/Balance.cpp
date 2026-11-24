// tests/sim/calibration/balance/Balance.cpp: the books of Balance.h, read from the simulator's own state each sample.
#include "Balance.h"

#include "tee/Build.h"
#include "tee/Fields.h"

#include <cstdint>
#include <cstring>

#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>

namespace tee::balance
{
    namespace
    {
        // the fuels' rows (sim/combustion/fuels.fm: stoichiometric ratio, lower heating value J/kg)
        constexpr double kAfr[] = {14.0, 14.0, 14.0, 14.6, 9.80, 6.47, 14.5, 15.6, 34.3};
        constexpr double kLhv[] = {42.2e6, 42.2e6, 42.2e6, 44.0e6, 29.3e6, 19.9e6, 42.5e6, 46.35e6, 120.0e6};

        // sim/core/cylinder.fm's gammaAt: the physical cylinder (conserve 1) Gatowski's with the lean term on the
        // cylinder's phi (a diesel's: phi x its burned share); the reference's otherwise. (u(T) is tabulated by phi to
        // 0.001; a diesel's phi moving through its burn makes the closure approximate there)
        struct Gas
        {
            int conserve = 1;
            double phi = 1.0;
            double gamma(double t) const
            {
                if (conserve != 0)
                    return std::min(std::max(1.392 - 8.13e-5 * t + 0.037 * (1.0 - std::min(phi, 1.0)), 1.15), 1.40);
                return std::min(std::max(1.39 - 7.5e-5 * (t - 300.0), 1.24), 1.40);
            }
        };
        // The gas's sensible energy and enthalpy from 0 K (J/kg): u(T) = integral of cv = R / (gamma(T) - 1),
        // h = u + R T. (The cylinder's step is m cv(T) dT = dQ - p dV + (h - u) dm, so these close its books; cp(T) T,
        // which the step writes, is the same thing only where gamma is constant.) A table a gas, kept by its key.
        double uAt(const Gas& g, double t)
        {
            static std::map<long, std::vector<double>> tables;
            const long key = g.conserve != 0 ? std::lround(std::min(g.phi, 1.0) * 1000.0) : -1;
            auto it = tables.find(key);
            if (it == tables.end())
            {
                std::vector<double> u(3601, 0.0);
                for (int i = 1; i <= 3600; ++i)
                    u[std::size_t(i)] = u[std::size_t(i - 1)] + 287.0 / (g.gamma(i - 0.5) - 1.0);
                it = tables.emplace(key, std::move(u)).first;
            }
            const std::vector<double>& table = it->second;
            const double x = std::clamp(t, 0.0, 3599.0);
            const auto i = std::size_t(x);
            return table[i] + (table[i + 1] - table[i]) * (x - double(i));
        }
        double hAt(const Gas& g, double t) { return uAt(g, t) + 287.0 * t; }

        struct Kept
        {
            std::unique_ptr<EnginePlayer> player;
            std::filesystem::path sim;
            double rate = 0.0;
        };
        Kept& kept()
        {
            static Kept k;   // (freed by keepPlayer(false) before main returns: 01-rules.md §4, macOS)
            return k;
        }
        bool keeping = false;

        struct H
        {
            Schema::Handle h;
            bool ok = false;
        };
    } // namespace

    bool loadWhole(EnginePlayer& p, const std::filesystem::path& file, std::string& error)
    {
        std::unique_ptr<Fields> fields = Fields::of(p.schema(), error);
        if (!fields)
            return false;
        Build build(*fields);
        std::vector<std::string> warnings;
        if (!build.loadFile(file, BuildPart::Whole, warnings, error))
            return false;
        unsigned char* base = static_cast<unsigned char*>(p.instance().data());
        const std::vector<FieldInfo>& all = fields->all();
        for (std::size_t i = 0; i < all.size(); ++i)
        {
            const FieldInfo& f = all[i];
            if (f.block != "spec")
                continue;
            const std::vector<double>& v = build.values(int(i));
            std::size_t at = 0;
            for (const FieldInfo::Leaf& leaf : f.leaves)
                for (std::size_t k = 0; k < leaf.count && at < v.size(); ++k, ++at)
                {
                    unsigned char* q = base + f.offset + leaf.offset + k * leaf.stride;
                    switch (leaf.scalar)
                    {
                    case Scalar::Float: { const float x = float(v[at]); std::memcpy(q, &x, 4); break; }
                    case Scalar::Double: { const double x = v[at]; std::memcpy(q, &x, 8); break; }
                    case Scalar::Int: { const std::int32_t x = std::int32_t(std::lround(v[at])); std::memcpy(q, &x, 4); break; }
                    case Scalar::Bool: { const bool x = v[at] != 0.0; std::memcpy(q, &x, 1); break; }
                    default: break;
                    }
                }
        }
        for (const Section& sec : fields->sections())
        {
            std::int32_t rev = 0;
            std::memcpy(&rev, base + sec.revOffset, 4);
            ++rev;
            std::memcpy(base + sec.revOffset, &rev, 4);
        }
        return true;
    }

    void keepPlayer(bool keep)
    {
        keeping = keep;
        if (!keep)
            kept().player.reset();
    }

    bool measure(const std::string& slug, const Options& o, Books& b, std::string& error)
    {
        b = Books{};
        b.slug = slug;
        const std::filesystem::path simDir = o.sim.empty() ? std::filesystem::path(TEE_SOURCE_DIR) / "sim" : o.sim;
        Kept& k = kept();
        if (!k.player || k.sim != simDir || k.rate != o.sampleRate)
        {
            EnginePlayer::Options po;
            po.directory = simDir;
            po.sampleRate = o.sampleRate;
            k.player = EnginePlayer::create(po, error);
            if (!k.player)
                return false;
            k.sim = simDir;
            k.rate = o.sampleRate;
        }
        else
            k.player->reset(o.sampleRate);
        EnginePlayer& p = *k.player;
        std::filesystem::path file = slug;
        if (!std::filesystem::exists(file))
            file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
        // The build through tee::Build, the loader tee-dyno and P's table use (it reads every nested section: the
        // ECU's idle, limiter, knock and baked maps, which the older loadBuild drops: the lead's note, 24 Nov), its
        // values written into the instance field by field and every section's rev moved
        if (!loadWhole(p, file, error))
            return false;
        for (const auto& [path, value] : o.set)
            if (!p.setSpec(path, value, error))
                return false;
        if (!o.raw.empty())
        {
            for (const auto& [path, value] : o.raw)
                if (!p.instance().write(p.schema(), path, value, error))
                    return false;
            std::unique_ptr<Fields> fields = Fields::of(p.schema(), error);
            if (!fields)
                return false;
            unsigned char* base = static_cast<unsigned char*>(p.instance().data());
            for (const Section& sec : fields->sections())
            {
                std::int32_t rev = 0;
                std::memcpy(&rev, base + sec.revOffset, 4);
                ++rev;
                std::memcpy(base + sec.revOffset, &rev, 4);
            }
        }

        const Schema& schema = p.schema();
        Instance& inst = p.instance();
        auto handle = [&](const std::string& path) {
            H h;
            std::string e;
            h.ok = schema.find(path, h.h, e);
            return h;
        };
        auto rd = [&](const H& h) { return h.ok ? inst.read(h.h) : std::nan(""); };
        auto rdPath = [&](const std::string& path) {
            double v = std::nan("");
            std::string e;
            inst.read(schema, path, v, e);
            return v;
        };
        auto tel = [&](const char* name) {
            const int i = p.telemetryIndex(name);
            return i < 0 ? std::nan("") : p.telemetry(std::size_t(i));
        };
        auto spec = [&](const char* path) {
            const int i = p.specIndex(path);
            return i < 0 ? std::nan("") : p.spec(std::size_t(i));
        };
        const int grade = std::clamp(int(spec("fuel.grade")), 0, 8);

        // the warm engine, and the heat settles tee-dyno makes
        auto warm = [&] {
            std::string e;
            for (const HeldValue& hv : warmEngine())
                inst.write(schema, hv.path, hv.value, e);
        };
        std::vector<std::pair<std::string, double>> settles;
        for (const char* section : {"exhaust", "intake", "boost"})
        {
            const std::string f = std::string(section) + ".heatSettle";
            if (p.specIndex(f) >= 0)
                settles.emplace_back(f, spec(f.c_str()));
        }
        auto settle = [&] {
            std::string e;
            for (auto& [f, count] : settles)
            {
                count += 1.0;
                p.setSpec(f, count, e);
            }
        };
        float l = 0.0f, r = 0.0f;
        long sampleNo = 0;
        auto play = [&](double seconds, const std::function<void()>& each) {
            const long n = long(seconds * o.sampleRate);
            for (long i = 0; i < n; ++i, ++sampleNo)
            {
                if ((sampleNo & 63) == 0)
                    warm();
                p.process(&l, &r, 1);
                if (each)
                    each();
            }
        };

        // the start, as tee-dyno's (host/src/Dyno.cpp): the dyno at the speed, the key on, the starter, the throttle
        auto& c = p.controls();
        c.mode = 2;
        c.dynoSpeed = float(o.rpm * M_PI / 30.0);
        c.dynoRate = 0.0f;
        c.ignition = 1;
        play(0.25, {});
        c.ignition = 2;
        play(0.75, {});
        c.ignition = 1;
        c.throttle = 1.0f;
        play(1.0, {});
        // the hold at the speed: settled at 0.3 s and at 0.6 of it
        const double s1 = std::min(0.3, 0.3 * o.settle);
        play(s1, {});
        settle();
        play(0.6 * o.settle - s1, {});
        settle();
        play(0.4 * o.settle, {});

        // the window
        const int n = int(rdPath("lay.n"));
        b.cylinders = n;
        const double dt = 1.0 / o.sampleRate;
        struct Cyl
        {
            H p, v, t, m, mInert, hA, tw, mE, mI, ang, sparkAt, evo, ivo, idur, burnAt, ivcNow, qTot, released, vd,
                exStore, exStoreMT, rvMass, rvMT, phi, conserve, lateFuel, xBurn;
            double pL, vL, tL, aL, relL, exStoreL, exStoreMTL, rvMassL, rvMTL;
        };
        std::vector<Cyl> cy(std::size_t(std::max(n, 0)));
        for (int i = 0; i < n; ++i)
        {
            const std::string q = "cyl[" + std::to_string(i) + "].";
            Cyl& x = cy[std::size_t(i)];
            x.p = handle(q + "p");
            x.v = handle(q + "v");
            x.t = handle(q + "t");
            x.m = handle(q + "m");
            x.mInert = handle(q + "mInert");
            x.hA = handle(q + "hA");
            x.tw = handle(q + "tw");
            x.mE = handle(q + "mE");
            x.mI = handle(q + "mI");
            x.ang = handle(q + "ang");
            x.sparkAt = handle(q + "sparkAt");
            x.evo = handle(q + "evo");
            x.ivo = handle(q + "ivo");
            x.idur = handle(q + "idur");
            x.burnAt = handle(q + "burnAt");
            x.ivcNow = handle(q + "ivcNow");
            x.qTot = handle(q + "qTot");
            x.released = handle(q + "burnReleased");
            x.vd = handle(q + "vd");
            x.exStore = handle(q + "exStore");
            x.exStoreMT = handle(q + "exStoreMT");
            x.rvMass = handle(q + "rvMass");
            x.rvMT = handle(q + "rvMT");
            x.phi = handle(q + "phi");
            x.conserve = handle(q + "conserve");
            x.lateFuel = handle(q + "lateFuel");
            x.xBurn = handle(q + "xBurn");
            b.vd += rd(x.vd);
        }
        const H airThr = handle("airThrottle"), airVal = handle("airValves"), gasEx = handle("gasExhaust");
        const H oneFuel = handle("comb.one.fuel"), oneLambda = handle("comb.one.lambdaOut"), mixHeat = handle("comb.one.mix.heat"),
                oxygen = handle("comb.oxygen"), tAirH = handle("intk.tAir"), pPlenH = handle("intk.pPlenum"), tAmbH = handle("heat.tAmb");
        const double pumpFrom = rdPath("work.pumpFrom"), pumpTo = rdPath("work.pumpTo"), cycDeg = rdPath("cyl[0].cyc");
        auto gasOf = [&](const Cyl& x) {
            Gas g;
            g.conserve = x.conserve.ok ? int(rd(x.conserve)) : 0;
            g.phi = x.phi.ok ? rd(x.phi) : 1.0;
            // (a late-injected charge's gas: the fuel burned so far over its air, as cylinder.fm's gammaAt)
            if (x.lateFuel.ok && rd(x.lateFuel) != 0.0 && x.xBurn.ok)
                g.phi *= rd(x.xBurn);
            return g;
        };
        auto energy = [&] {
            double u = 0.0;
            for (Cyl& x : cy)
                u += rd(x.m) * uAt(gasOf(x), rd(x.t));
            return u;
        };
        for (Cyl& x : cy)
        {
            x.pL = rd(x.p);
            x.vL = rd(x.v);
            x.tL = rd(x.t);
            x.aL = rd(x.ang);
            x.relL = rd(x.released);
            x.exStoreL = rd(x.exStore);
            x.exStoreMTL = rd(x.exStoreMT);
            x.rvMassL = rd(x.rvMass);
            x.rvMTL = rd(x.rvMT);
        }
        const double thr0 = rd(airThr), val0 = rd(airVal), ex0 = rd(gasEx);
        b.uStart = energy();
        double sumRpm = 0.0, sumTq = 0.0, sumLam = 0.0, sumTA = 0.0, sumTH = 0.0, sumPP = 0.0, sumTivc = 0.0;
        double sVeA = 0.0, sVeM = 0.0, sF = 0.0, sI = 0.0, sP = 0.0, sB = 0.0, sS = 0.0;
        long count = 0;
        std::vector<H> traceH;
        for (const std::string& path : o.trace)
            traceH.push_back(handle(path));
        b.traces.assign(traceH.size(), {});
        std::vector<H> meanH;
        for (const std::string& path : o.mean)
            meanH.push_back(handle(path));
        std::vector<double> meanSum(meanH.size(), 0.0);
        play(o.measure, [&] {
            for (std::size_t i = 0; i < traceH.size(); ++i)
                b.traces[i].push_back(rd(traceH[i]));
            for (std::size_t i = 0; i < meanH.size(); ++i)
                meanSum[i] += rd(meanH[i]);
            const double tA = rd(tAirH);
            for (Cyl& x : cy)
            {
                const Gas g = gasOf(x);
                const double pN = rd(x.p), vN = rd(x.v), tN = rd(x.t), aN = rd(x.ang), mE = rd(x.mE), mI = rd(x.mI);
                // the walls, by phase (the step took hA (T - T_wall) at the start's temperature)
                const double q = rd(x.hA) * (x.tL - rd(x.tw)) * dt;
                const double evo = rd(x.evo), ivc = std::fmod(rd(x.ivo) + rd(x.idur), cycDeg), spark = rd(x.sparkAt);
                const double a = x.aL;
                const bool exchanging = ivc > evo ? (a >= evo && a < ivc) : (a >= evo || a < ivc);
                if (rd(x.burnAt) >= 0.0 && !(a >= evo && a < ivc))
                    b.wallBurn += q;
                else if (exchanging)
                    b.wallExchange += q;
                else if (a >= ivc && a < spark)
                    b.wallCompression += q;
                else
                    b.wallExpansion += q;
                // the work, split as core/work.fm splits it
                const double dw = 0.5 * (pN + x.pL) * (vN - x.vL);
                const double mid = 0.5 * (aN + x.aL);
                if (aN >= x.aL && mid >= pumpFrom && mid < pumpTo)
                    b.workPump += dw;
                else
                    b.workGross += dw;
                // the burn's heat
                const double rel = rd(x.released);
                b.released += rel >= x.relL ? rel - x.relL : rel;
                x.relL = rel;
                // the enthalpy through the valves (out positive)
                if (mE >= 0.0)
                {
                    b.hOut += hAt(g, x.tL) * mE * dt;
                    b.mOutExhaust += mE * dt;
                }
                else
                {
                    const double tb = x.exStoreL > 1e-12 ? x.exStoreMTL / x.exStoreL : x.tL;
                    b.hOut += hAt(g, tb) * mE * dt;
                    b.mOutExhaust += mE * dt;
                }
                if (mI >= 0.0)
                    b.hIn -= hAt(g, x.tL) * mI * dt;
                else
                {
                    const double take = std::min(x.rvMassL, -mI * dt);
                    const double tr = x.rvMassL > 1e-12 ? x.rvMTL / x.rvMassL : tA;
                    b.hIn += hAt(g, tr) * take + hAt(g, tA) * (-mI * dt - take);
                }
                // an intake closing: what is trapped, and what the charge was given
                if (rd(x.ivcNow) != 0.0)
                {
                    const double m = rd(x.m), mi = rd(x.mInert);
                    b.trappedTotal += m;
                    b.trappedBurned += mi;
                    b.trappedFresh += std::max(m - mi, 0.0);
                    sumTivc += tN;
                    ++b.closings;
                    const double fuel = rd(oneFuel), lam = rd(oneLambda), ox = rd(oxygen);
                    b.fuel += fuel;
                    b.fuelAir += fuel * kAfr[grade] * lam / std::max(ox, 1e-6);
                    b.mixHeat += rd(mixHeat);
                    b.chargedHeat += rd(x.qTot);
                    sumLam += lam;
                }
                x.pL = pN;
                x.vL = vN;
                x.tL = tN;
                x.aL = aN;
                x.exStoreL = rd(x.exStore);
                x.exStoreMTL = rd(x.exStoreMT);
                x.rvMassL = rd(x.rvMass);
                x.rvMTL = rd(x.rvMT);
            }
            sumRpm += tel("engine.speed") * 30.0 / M_PI;   // (rad/s)
            sumTq += tel("engine.torque");
            sumTA += rd(tAmbH);
            sumTH += tA;
            sumPP += rd(pPlenH);
            sVeA += tel("engine.veAir");
            sVeM += tel("engine.veManifold");
            sF += tel("engine.fmep");
            sI += tel("engine.imep");
            sP += tel("engine.pmep");
            sB += tel("engine.bmep");
            sS += tel("ecu.advance");
            ++count;
        });
        b.uEnd = energy();
        for (const std::string& path : o.read)
            b.reads[path] = rdPath(path);
        for (std::size_t i = 0; i < meanH.size(); ++i)
            b.means[o.mean[i]] = meanSum[i] / double(std::max(count, 1L));
        b.airThrottle = rd(airThr) - thr0;
        b.airValves = rd(airVal) - val0;
        b.gasExhaust = rd(gasEx) - ex0;
        b.seconds = o.measure;
        const double cnt = double(std::max(count, 1L));
        b.rpm = sumRpm / cnt;
        b.torque = sumTq / cnt;
        b.tAmb = sumTA / cnt;
        b.tAirHeated = sumTH / cnt;
        b.pPlenum = sumPP / cnt;
        b.veAirTel = sVeA / cnt;
        b.veManTel = sVeM / cnt;
        b.fmepTel = sF / cnt;
        b.imepTel = sI / cnt;
        b.pmepTel = sP / cnt;
        b.bmepTel = sB / cnt;
        b.sparkTel = sS / cnt;
        b.lambda = b.closings > 0 ? sumLam / b.closings : 0.0;
        b.tIvc = b.closings > 0 ? sumTivc / b.closings : 0.0;
        b.lhv = kLhv[grade];
        b.fuelEnergy = b.fuel * b.lhv;
        b.hOutAtInlet = b.mOutExhaust * hAt(Gas{}, b.tAirHeated);
        b.cycles = b.closings / double(std::max(n, 1));
        if (!keeping)
            k.player.reset();
        return true;
    }
} // namespace tee::balance
