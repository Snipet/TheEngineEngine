// ui/views/engine/TwoStroke.cpp: see TwoStroke.h.
#include "views/engine/TwoStroke.h"

#include <algorithm>
#include <cmath>

namespace tee::ui::engine
{
    namespace
    {
        double field(const FieldDoc& doc, const char* path)
        {
            const int i = doc.find(path);
            return i < 0 ? kNaN : doc.get(i);
        }
    }

    TwoStroke twoStrokeOf(const FieldDoc& doc)
    {
        TwoStroke t;
        const double cycle = fieldValue(doc, EF::cycle);
        t.on = std::isfinite(cycle) && cycle >= 0.5;
        if (!t.on)
            return t;
        const double n = field(doc, "engine.tsCylinders");
        t.cylinders = std::isfinite(n) ? std::clamp(static_cast<int>(std::lround(n)), 1, 4) : 1;
        t.together = field(doc, "engine.tsFiring") >= 0.5;
        t.exDur = field(doc, "engine.tsExhaustDur");
        t.trDur = field(doc, "engine.tsTransferDur");
        t.exWidth = field(doc, "engine.tsExhaustWidth");
        t.trWidth = field(doc, "engine.tsTransferWidth");
        t.valveDur = field(doc, "engine.tsValveDur");
        t.valveFrom = field(doc, "engine.tsValveFrom");
        t.valveTo = field(doc, "engine.tsValveTo");
        const double k = field(doc, "engine.tsInletKind");
        t.inlet = !std::isfinite(k) ? Inlet::reed : static_cast<Inlet>(std::clamp(static_cast<int>(std::lround(k)), 0, 2));
        t.discOpen = field(doc, "engine.tsDiscOpen");
        t.discDur = field(doc, "engine.tsDiscDur");
        t.pistonDur = field(doc, "engine.tsPistonDur");
        t.inletArea = field(doc, "engine.tsInletArea");
        t.carbArea = field(doc, "engine.tsCarbArea");
        t.reedFull = field(doc, "engine.tsReedFull");
        t.crankcaseRatio = field(doc, "engine.tsCrankcaseRatio");
        t.spark = field(doc, "engine.tsSpark");
        return t;
    }

    double TwoStroke::exDurAt(double speed) const
    {
        if (!hasValve() || !std::isfinite(speed))
            return exDur;
        if (!(valveTo > valveFrom))
            return speed >= valveTo ? exDur : valveDur;
        const double u = std::clamp((speed - valveFrom) / (valveTo - valveFrom), 0.0, 1.0);
        return valveDur + (exDur - valveDur) * u;
    }

    bool TwoStroke::inletWindow(double& from, double& dur) const
    {
        if (inlet == Inlet::disc && std::isfinite(discOpen) && std::isfinite(discDur))
        {
            from = discOpen;
            dur = discDur;
            return true;
        }
        if (inlet == Inlet::pistonPort && std::isfinite(pistonDur))
        {
            from = 360.0 - 0.5 * pistonDur;
            dur = pistonDur;
            return true;
        }
        return false;
    }

    const char* TwoStroke::inletName() const
    {
        return inlet == Inlet::disc ? "ROTARY DISC" : inlet == Inlet::pistonPort ? "PISTON PORT" : "REED VALVE";
    }

    double portHeight(double dur, double radius, double rod)
    {
        if (!std::isfinite(dur))
            return kNaN;
        return 2.0 * radius - pistonDrop(180.0 - 0.5 * dur, radius, rod);
    }

    double portShare(double deg, double dur, double radius, double rod)
    {
        const double h = portHeight(dur, radius, rod);
        if (!(h > 0.0))
            return 0.0;
        const double top = 2.0 * radius - h;   // the port's top edge, as the crown's drop from top dead centre
        return std::clamp((pistonDrop(deg, radius, rod) - top) / h, 0.0, 1.0);
    }

    const char* twoStrokePhase(double deg, const TwoStroke& t)
    {
        if (!std::isfinite(deg) || !std::isfinite(t.exDur))
            return "-";
        const double a = wrap(deg, 360.0);
        const bool tr = std::isfinite(t.trDur);
        if (a < t.exOpens() || a >= t.exCloses())
            return a < 180.0 ? "POWER" : "COMPRESSION";
        if (tr && a >= t.trOpens() && a < t.trCloses())
            return "SCAVENGE";
        return a < 180.0 ? "BLOWDOWN" : "TRAPPING";
    }
}
