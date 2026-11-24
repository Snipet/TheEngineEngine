// ui/views/air/AirModel.cpp: see AirModel.h. The constants are sim/boost/compressor.fm's and machine.fm's, in the same
// order, so the two can be read side by side.
#include "AirModel.h"

#include "views/exhaust/ExhaustModel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tee::ui::air
{
    namespace
    {
        constexpr float kPi = 3.14159265358979f;
        constexpr float kR = 287.05f, kCp = 1005.0f;
        const float kRedline[17] = { 8500, 9500, 10000, 5800, 7000, 7000, 6800, 14000, 6800, 6800, 7000, 8500, 6500, 9000,
                                     8500, 8500, 9000 };
        const int kCyl[17] = { 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 8, 8, 10, 12, 6 };

        int clampi(double v, int lo, int hi) { return std::clamp(static_cast<int>(std::lround(v)), lo, hi); }
        bool has(const std::string& hay, const char* needle)
        {
            std::string a = hay, b = needle;
            for (char& c : a)
                c = static_cast<char>(c >= 'A' && c <= 'Z' ? c + 32 : c);
            return a.find(b) != std::string::npos;
        }
        std::string choiceLabel(const FieldDoc& doc, const char* path)
        {
            const int f = doc.find(path);
            if (f < 0 || doc.info(f).kind != FieldKind::choice)
                return {};
            const FieldInfo& info = doc.info(f);
            const int k = static_cast<int>(std::lround(doc.get(f) - info.min));
            return k >= 0 && k < static_cast<int>(info.choices.size()) ? info.choices[static_cast<std::size_t>(k)] : std::string();
        }
    }

    // ---- the map ----------------------------------------------------------------------------------------------------
    void CompressorMap::size(float dInd, float dEx, float ar, float etaPeak)
    {
        const float ri = 0.5f * std::max(dInd, 0.005f);
        rTip = 0.5f * std::max(dEx, dInd * 1.05f);
        aInd = kPi * ri * ri * 0.91f;
        arWide = std::exp(0.25f * std::log(std::max(ar, 0.2f) / 0.6f));
        phiK = std::min(std::max(0.5f * std::exp(-0.3f * std::log(std::max(ar, 0.2f) / 0.6f)), 0.3f), 0.7f);
        phiB = phiK + 0.45f * (1.0f - phiK);
        etaMax = etaPeak;
    }

    CompressorMap::Line CompressorMap::at(float omega) const
    {
        Line l;
        const float p1 = 101325.0f, t1 = 293.15f;
        l.omega = omega;
        const float u = omega * rTip;
        const float a01 = std::sqrt(1.4f * kR * t1);
        const float mach = u / a01;
        const float rho01 = p1 / (kR * t1);
        l.mc = 0.45f * rho01 * a01 * aInd * arWide * (0.3f + 0.7f * std::min(mach / 1.2f, 1.0f));
        l.tauK = 0.45f * u * u / (kCp * t1);
        l.tau0 = 0.88f * l.tauK;
        l.tauC = 0.35f * l.tauK;
        const float dm = mach - 1.1f;
        l.etaM = etaMax - 0.25f * dm * dm;
        return l;
    }

    float CompressorMap::rise(const Line& l, float phiK, float phi)
    {
        if (phi < phiK)
        {
            const float x = std::max(phi, 0.0f) / phiK;
            return l.tau0 + (l.tauK - l.tau0) * x * x * (3.0f - 2.0f * x);
        }
        if (phi < 1.0f)
        {
            const float w = 1.0f - phiK, y = (phi - phiK) / w;
            return l.tauC + (l.tauK - l.tauC) * std::sqrt(std::max(1.0f - y * y, 1.0e-4f));
        }
        return l.tauC;
    }

    float CompressorMap::efficiency(const Line& l, float phi) const
    {
        const float d = phi - phiB;
        return std::max(l.etaM - 0.9f * d * d, 0.3f);
    }

    float CompressorMap::ratio(float tau)
    {
        const float c = 1.0f + tau;
        return c * c * c * std::sqrt(c);
    }

    MapLines mapLines(const CompressorMap& m)
    {
        MapLines out;
        const float a0 = std::sqrt(1.4f * kR * 293.15f);
        const float levels[MapLines::kLevels] = { 0.02f, 0.05f, 0.08f, 0.12f, 0.18f };
        for (int l = 0; l < MapLines::kLines; ++l)
        {
            const float mach = 0.4f + 0.15f * static_cast<float>(l);
            const float w = mach * a0 / std::max(m.rTip, 1.0e-4f);
            const CompressorMap::Line line = m.at(w);
            const auto li = static_cast<std::size_t>(l);
            out.speed[li] = w;
            for (int i = 0; i < MapLines::kPoints; ++i)
            {
                const float phi = m.phiK + (1.0f - m.phiK) * static_cast<float>(i) / static_cast<float>(MapLines::kPoints - 1);
                out.flow[li][static_cast<std::size_t>(i)] = phi * line.mc;
                out.pr[li][static_cast<std::size_t>(i)] = CompressorMap::ratio(CompressorMap::rise(line, m.phiK, phi));
                out.maxFlow = std::max(out.maxFlow, phi * line.mc);
                out.maxPr = std::max(out.maxPr, out.pr[li][static_cast<std::size_t>(i)]);
            }
        }
        const float macLo = 0.4f, macHi = 0.4f + 0.15f * static_cast<float>(MapLines::kLines - 1);
        for (int l = 0; l < MapLines::kIslandSteps; ++l)
        {
            const float mach = macLo + (macHi - macLo) * static_cast<float>(l) / static_cast<float>(MapLines::kIslandSteps - 1);
            const CompressorMap::Line line = m.at(mach * a0 / std::max(m.rTip, 1.0e-4f));
            for (int v = 0; v < MapLines::kLevels; ++v)
            {
                const auto vi = static_cast<std::size_t>(v);
                const float level = m.etaMax - levels[v];
                out.islandEta[vi] = level;
                const float room = line.etaM - level;
                float fl = -1, fr = -1, pl = 1, pr = 1;
                if (room > 0.0f)
                {
                    const float dphi = std::sqrt(room / 0.9f);
                    const float phl = std::max(m.phiB - dphi, m.phiK), phr = m.phiB + dphi;
                    if (phl < phr)
                    {
                        fl = phl * line.mc;
                        pl = CompressorMap::ratio(CompressorMap::rise(line, m.phiK, phl));
                    }
                    if (phr < 1.0f)
                    {
                        fr = phr * line.mc;
                        pr = CompressorMap::ratio(CompressorMap::rise(line, m.phiK, phr));
                    }
                }
                out.islandFlow[vi][static_cast<std::size_t>(l)] = fl;
                out.islandPr[vi][static_cast<std::size_t>(l)] = pl;
                out.islandFlow[vi][static_cast<std::size_t>(2 * MapLines::kIslandSteps - 1 - l)] = fr;
                out.islandPr[vi][static_cast<std::size_t>(2 * MapLines::kIslandSteps - 1 - l)] = pr;
            }
        }
        return out;
    }

    // ---- the build --------------------------------------------------------------------------------------------------
    std::string AirBuild::sentence() const
    {
        std::string s = exhaust::layoutName(layout);
        char b[48];
        std::snprintf(b, sizeof b, " %.1f L", displacement * 1000.0);
        s += b;
        if (charger == Charger::none)
            s += ", naturally aspirated";
        else if (charger == Charger::supercharger)
            s += blower == Blower::roots ? ", Roots supercharger" : blower == Blower::screw ? ", twin-screw supercharger"
                                                                                           : ", centrifugal supercharger";
        else
        {
            static const char* const kArr[] = { "", ", a turbo", ", twin turbos", ", sequential turbos", ", compound turbos",
                                                ", four turbos" };
            s += kArr[static_cast<int>(arrangement)];
            if (vanes)
                s += " with variable vanes";
        }
        if (intercooler && charger != Charger::none)
            s += ", intercooled";
        s += itb ? ", a throttle a runner" : ", one throttle and a plenum";
        if (antiLag)
            s += ", anti-lag";
        if (nitrous)
            s += ", nitrous";
        return s;
    }

    AirBuild resolveAir(const FieldDoc& doc)
    {
        using P = AirPaths;
        using exhaust::fieldOr;
        AirBuild b;
        int layout = 12;
        if (const int f = doc.find(P::layout); f >= 0)
        {
            const std::string label = choiceLabel(doc, P::layout);
            const int byLabel = label.empty() ? -1 : exhaust::layoutFromLabel(label);
            layout = byLabel >= 0 ? byLabel : clampi(doc.get(f), 0, 16);
        }
        b.layout = layout;
        b.cylinders = layout == 16 ? 2 : kCyl[layout];   // a rotary: a runner a rotor
        {
            double vd = fieldOr(doc, P::displacement, -1, -1.0);
            if (vd <= 0.0)
            {
                const double bore = fieldOr(doc, "engine.bore", -1, -1.0), stroke = fieldOr(doc, "engine.stroke", -1, -1.0);
                vd = bore > 0 && stroke > 0 ? 0.785398 * bore * bore * stroke * b.cylinders : 0.00576;
            }
            b.displacement = vd;
        }
        b.redRpm = kRedline[layout] + fieldOr(doc, P::redlineTrim, -1, 0.0);
        b.ambientP = fieldOr(doc, P::ambientP, -1, 101325.0);
        b.ambientT = fieldOr(doc, P::ambientT, -1, 293.15);

        // the aspiration, by label where it is a choice
        int kind = 0;
        if (const std::string label = choiceLabel(doc, P::kind); !label.empty())
            kind = has(label, "super") ? 4 : has(label, "quad") ? 3 : has(label, "twin") ? 2 : has(label, "turbo") ? 1 : 0;
        else
            kind = clampi(fieldOr(doc, P::kind, -1, 0.0), 0, 4);
        const int arr = clampi(fieldOr(doc, P::arrangement, -1, 0.0), 0, 5);
        if (kind >= 1 && kind <= 3)
        {
            b.charger = Charger::turbo;
            b.arrangement = arr == 1 ? Arrangement::one : arr == 2 ? Arrangement::twin : arr == 3 ? Arrangement::sequential
                          : arr == 4 ? Arrangement::compound : arr == 5 ? Arrangement::quad
                          : kind == 1 ? Arrangement::one : kind == 2 ? Arrangement::twin : Arrangement::quad;
            b.turbos = b.arrangement == Arrangement::one ? 1 : b.arrangement == Arrangement::quad ? 4 : 2;
        }
        else if (kind == 4)
        {
            b.charger = Charger::supercharger;
            b.blower = static_cast<Blower>(clampi(fieldOr(doc, P::charger, -1, 0.0), 0, 2));
        }
        b.vanes = b.charger == Charger::turbo && fieldOr(doc, P::vanes, -1, 0.0) > 0.5;
        b.externalGate = fieldOr(doc, P::gate, -1, 0.0) > 0.5;
        b.gateSpring = fieldOr(doc, P::gateSpring, -1, 70000.0);
        b.target = fieldOr(doc, P::target, -1, 0.0);
        b.blowOff = b.charger == Charger::none ? 0 : clampi(fieldOr(doc, P::blowOffKind, -1, 1.0), 0, 2);
        b.intercooler = b.charger != Charger::none && fieldOr(doc, P::icEffect, -1, 0.7) > 0.01;
        b.scRatio = fieldOr(doc, P::scRatio, -1, 2.4);
        b.scBypass = fieldOr(doc, P::scBypass, -1, 1.0) > 0.5;
        b.antiLag = b.charger == Charger::turbo && fieldOr(doc, P::antiLag, -1, 0.0) > 0.5;
        b.nitrous = fieldOr(doc, P::nitrousShot, -1, 0.0) > 0.0;
        b.nitrousBottle = fieldOr(doc, P::nitrousBottle, -1, 4.5);
        b.itb = fieldOr(doc, P::throttleKind, -1, 0.0) > 0.5;
        b.trumpetsToAir = fieldOr(doc, P::trumpets, -1, 0.0) > 0.5;

        // machine.fm match(): turbo 1's compressor, sized for its share of the engine's flow at the redline
        if (b.hasMap())
        {
            const float mMatch = std::max(static_cast<float>(b.displacement * b.redRpm / 120.0 * 1.204 * 1.9 * 0.9), 0.01f);
            const float share = b.arrangement == Arrangement::twin ? 0.5f : b.arrangement == Arrangement::quad ? 0.25f
                              : b.arrangement == Arrangement::sequential ? 0.55f : 1.0f;
            // (the demo's "boost.size" is a 0..1 dial of its own: only the real ratio, 0.5 to 2, is read)
            const int fs = doc.find(P::size);
            const float sizeK = fs >= 0 && doc.info(fs).max > 1.0 ? static_cast<float>(doc.get(fs)) : 1.0f;
            const float ar = static_cast<float>(fieldOr(doc, P::compAR, -1, 0.6));
            const float mt = mMatch * share * (sizeK > 0.0f && sizeK < 3.0f ? sizeK : 1.0f)
                           * (b.arrangement == Arrangement::compound ? 0.65f : 1.0f);
            const float wide = std::exp(0.25f * std::log(std::max(ar, 0.2f) / 0.6f));
            const float aAnn = 1.15f * mt / (0.45f * 1.204f * 343.2f * wide);
            const float ind = static_cast<float>(fieldOr(doc, P::compInducer, -1, 0.0));
            const float exd = static_cast<float>(fieldOr(doc, P::compExducer, -1, 0.0));
            const float dInd = ind > 0.0f ? ind : 2.0f * std::sqrt(aAnn / (0.91f * kPi));
            float dEx = exd > 0.0f ? exd : dInd / 0.7416f;
            if (b.charger == Charger::supercharger && exd <= 0.0f)
            {
                const float wRed = static_cast<float>(b.redRpm) * 0.10471976f * static_cast<float>(b.scRatio)
                                 * static_cast<float>(fieldOr(doc, P::scStepUp, -1, 3.6));
                dEx = std::max(dInd / 0.8f, 600.0f / std::max(wRed, 100.0f));
            }
            b.map.size(dInd, dEx, ar, static_cast<float>(fieldOr(doc, P::compEta, -1, 0.76)));
            const double ms = fieldOr(doc, P::maxSpeed, -1, 0.0);
            b.omegaMax = ms > 0.0 ? ms : 560.0 / b.map.rTip;
        }
        return b;
    }
}
