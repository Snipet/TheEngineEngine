// ui/views/engine/EngineDemo.cpp: card T's two-stroke fields for the demo's document (sim/spec/engine.fm's marked
// block, its ranges, defaults and groups), through U1's demo hook: `engine.cycle` stays 0 (a four-stroke), so the demo
// is as before until a test or a person sets it.
#include "app/Demo.h"

namespace tee::ui
{
    void engineDemo(DemoExtras& x)
    {
        if (x.engine != nullptr)
        {
            // card T's telemetry as its 125 cc single gives it at 11 000 rpm (0 for a four-stroke: the stages read
            // these only for a two-stroke)
            x.engine->addTelemetry("engine.layoutRefused", [](const DemoEngine&) { return 0.0; });   // D's: none refused
            x.engine->addTelemetry("engine.tsPurity", [](const DemoEngine&) { return 0.95; });
            x.engine->addTelemetry("engine.tsDelivery", [](const DemoEngine&) { return 0.85; });
            x.engine->addTelemetry("engine.tsTrapping", [](const DemoEngine&) { return 0.65; });
            x.engine->addTelemetry("engine.tsPowerValve", [](const DemoEngine&) { return 1.0; });
            x.engine->addTelemetry("engine.tsCrankcase", [](const DemoEngine& e) { return 101325.0 * (0.98 + 0.2 * e.phase()); });
            x.engine->addTelemetry("engine.tsReed", [](const DemoEngine& e) { return e.phase() < 0.5 ? 0.6 : 0.0; });
        }
        if (x.fields == nullptr)
            return;
        std::string group;
        const auto add = [&](std::string path, FieldKind kind, double lo, double hi, double def, std::string label,
                             Quantity q = Quantity::none, std::string unit = {}) -> FieldInfo& {
            FieldInfo f;
            f.path = std::move(path);
            f.kind = kind;
            f.min = lo;
            f.max = hi;
            f.def = def;
            f.label = std::move(label);
            f.quantity = q;
            f.unit = std::move(unit);
            f.group = group;
            x.fields->push_back(std::move(f));
            return x.fields->back();
        };
        const auto choice = [&](std::string path, std::string label, std::vector<std::string> names, int def) {
            FieldInfo& f = add(std::move(path), FieldKind::choice, 0, static_cast<double>(names.size() - 1), def, std::move(label));
            f.choices = std::move(names);
        };
        // package D's moving masses (the demo's made-up Spec lacks them): 0 is "from the bore", as D's are
        group = "Moving masses";
        add("engine.pistonMass", FieldKind::number, 0, 20, 0, "Piston, pin and rings", Quantity::mass);
        add("engine.rodMass", FieldKind::number, 0, 20, 0, "Rod", Quantity::mass);
        group = "Two-stroke";
        choice("engine.cycle", "Cycle", { "Four-stroke", "Two-stroke" }, 0);
        add("engine.tsCylinders", FieldKind::integer, 1, 4, 1, "Cylinders", Quantity::none, "cyl");
        choice("engine.tsFiring", "Firing", { "Even", "Together" }, 0);
        group = "Ports";
        add("engine.tsExhaustDur", FieldKind::number, 140, 220, 192, "Exhaust port open", Quantity::none, "deg");
        add("engine.tsExhaustWidth", FieldKind::number, 0.3, 1.0, 0.70, "Exhaust port width", Quantity::fraction);
        add("engine.tsTransferDur", FieldKind::number, 100, 150, 130, "Transfer ports open", Quantity::none, "deg");
        add("engine.tsExhaustRadius", FieldKind::number, 0, 0.5, 0.25, "Exhaust port's corner radius", Quantity::fraction);
        add("engine.tsTransferWidth", FieldKind::number, 0.3, 1.6, 1.10, "Transfer ports' width", Quantity::fraction);
        add("engine.tsTransferRadius", FieldKind::number, 0, 0.5, 0.15, "Transfer ports' corner radius", Quantity::fraction);
        add("engine.tsPortCd", FieldKind::number, 0.4, 0.9, 0.70, "Ports' discharge coefficient", Quantity::ratio);
        group = "Power valve";
        add("engine.tsValveDur", FieldKind::number, 0, 220, 0, "Power valve closed: exhaust open", Quantity::none, "deg");
        add("engine.tsValveFrom", FieldKind::number, 0, 2000, 900, "Opens from", Quantity::rotation);
        add("engine.tsValveTo", FieldKind::number, 0, 2000, 1150, "Fully open at", Quantity::rotation);
        group = "Crankcase";
        add("engine.tsCrankcaseRatio", FieldKind::number, 1.1, 2.0, 1.40, "Crankcase compression", Quantity::ratio);
        choice("engine.tsInletKind", "Inlet", { "Reed valve", "Rotary disc", "Piston port" }, 0);
        add("engine.tsInletArea", FieldKind::number, 0.0001, 0.003, 0.0008, "Inlet's open area", Quantity::area);
        add("engine.tsReedFull", FieldKind::number, 1000, 50000, 15000, "Reed fully open at", Quantity::pressure);
        add("engine.tsReedHz", FieldKind::number, 100, 2000, 550, "Reed's natural frequency", Quantity::frequency);
        add("engine.tsReedDamping", FieldKind::number, 0.02, 1.0, 0.15, "Reed's damping", Quantity::ratio);
        add("engine.tsDiscOpen", FieldKind::number, 0, 360, 220, "Disc opens", Quantity::none, "deg");
        add("engine.tsDiscDur", FieldKind::number, 90, 250, 200, "Disc open for", Quantity::none, "deg");
        add("engine.tsPistonDur", FieldKind::number, 90, 200, 140, "Piston port open for", Quantity::none, "deg");
        add("engine.tsCarbArea", FieldKind::number, 0.0001, 0.003, 0.00057, "Carburettor bore's area", Quantity::area);
        group = "Scavenging";
        add("engine.tsMixing", FieldKind::number, 0, 1, 0.55, "Scavenging: mixing", Quantity::fraction);
        add("engine.tsShortCircuit", FieldKind::number, 0, 0.5, 0.10, "Short-circuited share", Quantity::fraction);
        group = "Burn";
        add("engine.tsSpark", FieldKind::number, 0, 40, 18, "Spark", Quantity::none, "deg");
        add("engine.tsBurnDur", FieldKind::number, 20, 90, 45, "Burn's duration", Quantity::none, "deg");
        add("engine.tsAfr", FieldKind::number, 10, 16, 12.8, "Air-fuel ratio", Quantity::ratio);
    }
}
