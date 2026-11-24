// ui/views/valves/ValvesModel.cpp: see ValvesModel.h.
#include "views/valves/ValvesModel.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tee::ui::valves
{
    namespace
    {
        constexpr double kSeat = 0.0001524;   // m: the seat's checking height, 0.006 in (cam.fm)
        constexpr double kOneMm = 0.001;
        using engine::wrap;

        double raw(double u, double p, double k)
        {
            const double s = std::sin(kPi * u);
            if (s <= 0.0)
                return 0.0;
            return std::pow(s, p) * (1.0 - std::exp(-k * k * s * s));
        }
        // where the rising side reaches h (a share of its most): u in [0, 0.5] (cam.fm riseTo)
        double riseTo(double h, double p, double k)
        {
            const double top = 1.0 - std::exp(-k * k);
            double lo = 0.0, hi = 0.5;
            for (int i = 0; i < 40; ++i)
            {
                const double mid = 0.5 * (lo + hi);
                (raw(mid, p, k) < h * top ? lo : hi) = mid;
            }
            return 0.5 * (lo + hi);
        }
    }

    double fieldValue(const FieldDoc& doc, VF f) { return engine::fieldValue(doc, path(f)); }

    // ---- a lobe (cam.fm's generator) ---------------------------------------------------------------------------------
    void Lobe::build()
    {
        if (!known())
            return;
        if (free)
        {
            span = durSeat;
            open = wrap(centre - 0.5 * span, 720.0);
            return;
        }
        const double liftUse = std::max(std::isfinite(lift) ? lift : 0.010, 0.0012);
        p = 2.0 - 1.5 * std::clamp(std::isfinite(fat) ? fat : 0.5, 0.0, 1.0);
        const double hs = kSeat / liftUse, h1 = kOneMm / liftUse;
        // no 1 mm duration in the build: the reference cam's ramp (231 degrees at the seat, 212 at 1 mm)
        const double d1 = std::isfinite(dur1mm) ? dur1mm : durSeat * 212.0 / 231.0;
        const double want = std::clamp(d1 / std::max(durSeat, 1.0), 0.05, 1.0);
        double lo = -3.0, hi = 4.0;
        for (int i = 0; i < 32; ++i)
        {
            const double mid = 0.5 * (lo + hi), kk = std::exp(mid);
            const double r = (1.0 - 2.0 * riseTo(h1, p, kk)) / (1.0 - 2.0 * riseTo(hs, p, kk));
            (r < want ? lo : hi) = mid;
        }
        k = std::exp(0.5 * (lo + hi));
        kNorm = 1.0 / (1.0 - std::exp(-k * k));
        span = durSeat / (1.0 - 2.0 * riseTo(hs, p, k));
        open = wrap(centre - 0.5 * span, 720.0);
    }

    double Lobe::lift01(double a) const
    {
        if (!std::isfinite(span) || !std::isfinite(open))
            return 0.0;
        const double s = wrap(a - open, 720.0);
        if (s <= 0.0 || s >= span)
            return 0.0;
        const double u = s / span;
        if (free)
        {
            const double f = std::min(flank / span, 0.5), t = std::min(std::min(u, 1.0 - u) / f, 1.0);
            return t * t * t * (10.0 + t * (-15.0 + 6.0 * t));
        }
        return raw(u, p, k) * kNorm;
    }

    double Lobe::liftAt(double a) const { return lift01(a) * (std::isfinite(lift) ? lift : 1.0); }

    double Lobe::seatOpen() const { return known() ? wrap(centre - 0.5 * durSeat, 720.0) : kNaN; }
    double Lobe::seatClose() const { return known() ? wrap(centre + 0.5 * durSeat, 720.0) : kNaN; }

    // ---- the train -------------------------------------------------------------------------------------------------
    Train trainOf(const FieldDoc& doc)
    {
        Train t;
        const auto v = [&](VF f) { return fieldValue(doc, f); };
        const auto lobe = [&](VF dur, VF d1, VF lift, VF centre, VF fat, bool intake, VF kind) {
            Lobe l;
            l.durSeat = v(dur);
            l.dur1mm = v(d1);
            l.lift = v(lift);
            const double c = v(centre);   // a cam card's: the intake's after the overlap's top, the exhaust's before
            l.centre = std::isfinite(c) ? (intake ? 360.0 + c : 360.0 - c) : kNaN;
            const double f = v(fat);
            l.fat = std::isfinite(f) ? f : 0.5;
            l.free = v(kind) >= 0.5;
            if (l.free)
            {
                // a free valve: where it opens and closes outright (the cycle's degrees), over its flanks
                const double o = v(intake ? VF::freeInOpen : VF::freeExOpen), cl = v(intake ? VF::freeInClose : VF::freeExClose);
                if (std::isfinite(o) && std::isfinite(cl))
                {
                    l.durSeat = wrap(cl - o, 720.0);
                    l.centre = wrap(o + 0.5 * l.durSeat, 720.0);
                }
                const double fl = v(VF::freeFlank);
                if (std::isfinite(fl))
                    l.flank = fl;
            }
            l.build();
            return l;
        };
        t.in = lobe(VF::inDurSeat, VF::inDur1mm, VF::inLift, VF::inCentre, VF::inFat, true, VF::inKind);
        t.ex = lobe(VF::exDurSeat, VF::exDur1mm, VF::exLift, VF::exCentre, VF::exFat, false, VF::exKind);
        t.editable = t.in.known() && t.ex.known();
        const double model = v(VF::model);
        t.labOff = std::isfinite(model) && model < 0.5;
        if (!t.editable)
        {
            // the reference's dial: 0 a mild cam (224 degrees each, 112 apart), 1 a wild one (300, 106)
            const double cam = v(VF::cam);
            if (std::isfinite(cam))
            {
                t.reference = true;
                const double dur = 224.0 + 76.0 * cam, lsa = 112.0 - 6.0 * cam;
                t.in.durSeat = t.ex.durSeat = dur;
                t.in.centre = 360.0 + lsa;
                t.ex.centre = 360.0 - lsa;
                t.in.build();
                t.ex.build();
            }
        }
        t.stages = v(VF::stages) >= 0.5;
        if (t.stages)
        {
            t.inB = lobe(VF::inDurSeatB, VF::inDur1mmB, VF::inLiftB, VF::inCentreB, VF::inFat, true, VF::inKind);
            t.exB = lobe(VF::exDurSeatB, VF::exDur1mmB, VF::exLiftB, VF::exCentreB, VF::exFat, false, VF::exKind);
        }
        if (v(VF::auxOn) >= 0.5)
        {
            const double o = v(VF::auxOpen), d = v(VF::auxDur);
            if (std::isfinite(o) && std::isfinite(d))
            {
                t.aux.durSeat = d;
                t.aux.centre = wrap(o + 0.5 * d, 720.0);
                t.aux.lift = v(VF::auxLift);
                t.aux.build();
            }
        }
        t.inPhaseRange = v(VF::inPhaseRange);
        t.exPhaseRange = v(VF::exPhaseRange);
        t.stageSpeed = v(VF::stageSpeed);
        return t;
    }

    Events eventsOf(const Lobe& in, const Lobe& ex)
    {
        Events e;
        if (in.known())
        {
            e.ivo = 360.0 - in.seatOpen();
            e.ivc = in.seatClose() - 540.0;
        }
        if (ex.known())
        {
            e.evo = 180.0 - ex.seatOpen();
            e.evc = ex.seatClose() - 360.0;
        }
        if (in.known() && ex.known())
        {
            e.overlap = std::max(0.0, e.ivo + e.evc);
            e.lsa = 0.5 * (in.centre - ex.centre);
        }
        return e;
    }

    // ---- the flow bench ----------------------------------------------------------------------------------------------
    double benchFlow(int n, double d, double q, double l)
    {
        if (l <= 0.0 || !(d > 0.0))
            return 0.0;
        const double area = n * q * 0.4 * 0.785398 * d * d * std::tanh(6.8 * l / d) / 0.9354;   // area.fm areaOf
        return area * std::sqrt(2.0 * 6974.0 / 1.204);                                              // bench.fm
    }

    Bench benchOf(const FieldDoc& doc, double bore)
    {
        Bench b;
        const double valves = fieldValue(doc, VF::valves);
        if (!std::isfinite(valves))
            return b;
        const int nv = std::clamp(static_cast<int>(std::lround(valves)), 2, 5);
        double fi = 0.48, fe = 0.39;
        b.nIn = b.nEx = 1;
        if (nv == 3) { b.nIn = 2; fi = 0.37; fe = 0.44; }
        else if (nv == 4) { b.nIn = b.nEx = 2; fi = 0.39; fe = 0.33; }
        else if (nv == 5) { b.nIn = 3; b.nEx = 2; fi = 0.32; fe = 0.33; }
        const double di = fieldValue(doc, VF::dIn), de = fieldValue(doc, VF::dEx);
        b.dIn = std::isfinite(di) && di > 0.0 ? di : fi * bore;
        b.dEx = std::isfinite(de) && de > 0.0 ? de : fe * bore;
        if (!std::isfinite(b.dIn) || !std::isfinite(b.dEx))
            return b;
        const double qi = fieldValue(doc, VF::portIn), qe = fieldValue(doc, VF::portEx);
        const double qIn = std::isfinite(qi) && qi > 0.0 ? qi : 1.0, qEx = std::isfinite(qe) && qe > 0.0 ? qe : 1.0;
        for (int i = 0; i < 16; ++i)
        {
            const double l = 0.00127 * (i + 1);
            b.lift[static_cast<std::size_t>(i)] = l;
            b.flowIn[static_cast<std::size_t>(i)] = benchFlow(b.nIn, b.dIn, qIn, l);
            b.flowEx[static_cast<std::size_t>(i)] = benchFlow(b.nEx, b.dEx, qEx, l);
        }
        b.known = true;
        return b;
    }

    Bench benchOf(const FieldDoc& doc, double bore, const Live& live)
    {
        Bench b = benchOf(doc, bore);
        Bench sim = b;
        for (int i = 0; i < 16; ++i)
        {
            const auto at = [&](const char* path) {
                std::string n(path);
                n.replace(n.find("%d"), 2, std::to_string(i));
                return live.get(n);
            };
            const double l = at(kTelBenchLift), qi = at(kTelBenchIn), qe = at(kTelBenchEx);
            if (!(std::isfinite(l) && std::isfinite(qi) && std::isfinite(qe) && l > 0.0))
                return b;
            sim.lift[static_cast<std::size_t>(i)] = l;
            sim.flowIn[static_cast<std::size_t>(i)] = qi;
            sim.flowEx[static_cast<std::size_t>(i)] = qe;
        }
        sim.known = sim.simulated = true;
        return sim;
    }
}
