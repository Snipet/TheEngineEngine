// ui/views/valves/ValvesDemo.cpp: the fields of package E's valvetrain lab (sim/valvetrain/lab-spec.fm) as the demo's
// document lacks them, at a hot street cam with an intake phaser and a second stage, for tests and pictures. One
// function, so it moves into U1's demo hook (docs/handoffs/u1-interface.md) when that is there. The ranges, defaults,
// labels and groups are the lab spec's; the values set here are the picture's (ValvesDemo.h says which).
#include "views/valves/ValvesDemo.h"

#include "app/Demo.h"

#include <algorithm>

namespace tee::ui
{
    std::vector<FieldInfo> valvesDemoFields()
    {
        std::vector<FieldInfo> out;
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
            out.push_back(std::move(f));
            return out.back();
        };
        const auto choice = [&](std::string path, std::string label, std::vector<std::string> names, int def) {
            FieldInfo& f = add(std::move(path), FieldKind::choice, 0, static_cast<double>(names.size() - 1), def, std::move(label));
            f.choices = std::move(names);
        };
        const Quantity A = Quantity::angle, L = Quantity::length, R = Quantity::rotation, F = Quantity::fraction;
        const double rpm = 3.14159265358979323846 / 30.0;

        group = "Head";
        choice("valve.model", "Valvetrain model", { "Reference", "Lab" }, 1);
        add("valve.valves", FieldKind::integer, 2, 5, 2, "Valves a cylinder");
        add("valve.dIn", FieldKind::number, 0, 0.08, 0, "Intake valve diameter", L);
        add("valve.dEx", FieldKind::number, 0, 0.07, 0, "Exhaust valve diameter", L);
        add("valve.portIn", FieldKind::number, 0.5, 2, 1.35, "Intake port quality", Quantity::none, "x");
        add("valve.portEx", FieldKind::number, 0.5, 2, 1.5, "Exhaust port quality", Quantity::none, "x");
        group = "Intake cam";
        choice("valve.inKind", "Intake actuation", { "Cam", "Free valve" }, 0);
        add("valve.inDurSeat", FieldKind::number, 140, 360, 268, "Intake duration at the seat", A);
        add("valve.inDur1mm", FieldKind::number, 100, 340, 244, "Intake duration at 1 mm", A);
        add("valve.inLift", FieldKind::number, 0.002, 0.02, 0.0124, "Intake lift", L);
        add("valve.inCentre", FieldKind::number, 60, 160, 108, "Intake centreline", A);
        add("valve.inFat", FieldKind::number, 0, 1, 0.5, "Intake lobe fatness", F);
        group = "Exhaust cam";
        choice("valve.exKind", "Exhaust actuation", { "Cam", "Free valve" }, 0);
        add("valve.exDurSeat", FieldKind::number, 140, 360, 274, "Exhaust duration at the seat", A);
        add("valve.exDur1mm", FieldKind::number, 100, 340, 250, "Exhaust duration at 1 mm", A);
        add("valve.exLift", FieldKind::number, 0.002, 0.02, 0.0119, "Exhaust lift", L);
        add("valve.exCentre", FieldKind::number, 60, 160, 116, "Exhaust centreline", A);
        add("valve.exFat", FieldKind::number, 0, 1, 0.5, "Exhaust lobe fatness", F);
        group = "Variable timing";
        add("valve.inPhaseRange", FieldKind::number, 0, 80, 40, "Intake phaser range", A);
        add("valve.exPhaseRange", FieldKind::number, 0, 80, 0, "Exhaust phaser range", A);
        add("valve.phaseRate", FieldKind::number, 20, 1000, 200, "Phaser speed", Quantity::none, "deg/s");
        group = "Two stages";
        add("valve.stages", FieldKind::toggle, 0, 1, 1, "Two-stage cams");
        add("valve.inDurSeatB", FieldKind::number, 140, 360, 296, "Intake B duration at the seat", A);
        add("valve.inDur1mmB", FieldKind::number, 100, 340, 270, "Intake B duration at 1 mm", A);
        add("valve.inLiftB", FieldKind::number, 0.002, 0.02, 0.0138, "Intake B lift", L);
        add("valve.inCentreB", FieldKind::number, 60, 160, 106, "Intake B centreline", A);
        add("valve.exDurSeatB", FieldKind::number, 140, 360, 300, "Exhaust B duration at the seat", A);
        add("valve.exDur1mmB", FieldKind::number, 100, 340, 274, "Exhaust B duration at 1 mm", A);
        add("valve.exLiftB", FieldKind::number, 0.002, 0.02, 0.0130, "Exhaust B lift", L);
        add("valve.exCentreB", FieldKind::number, 60, 160, 112, "Exhaust B centreline", A);
        add("valve.stageSpeed", FieldKind::number, 100, 1100, 5500 * rpm, "Stage change speed", R);
        add("valve.stageHyst", FieldKind::number, 0, 150, 31.4, "Stage change hysteresis", R);
        group = "Lift control";
        add("valve.liftControl", FieldKind::toggle, 0, 1, 0, "Continuous lift");
        add("valve.liftMin", FieldKind::number, 0.03, 1, 0.15, "Least lift", F);
        add("valve.liftThrottle", FieldKind::toggle, 0, 1, 0, "Lift is the load control");
        group = "Deactivation";
        add("valve.deacMask", FieldKind::integer, 0, 65535, 0x66, "Cylinders that can rest");
        add("valve.deacLow", FieldKind::number, 50, 600, 115, "Deactivation from", R);
        add("valve.deacHigh", FieldKind::number, 100, 800, 340, "Deactivation to", R);
        add("valve.deacLoad", FieldKind::number, 0, 1, 0.35, "Deactivation below load", F);
        group = "Springs";
        choice("valve.train", "Valvetrain", { "Pushrod", "Rocker", "Bucket", "Finger", "Pneumatic" }, 0);
        add("valve.springRate", FieldKind::number, 0, 400000, 52500, "Spring rate", Quantity::none, "N/m");
        add("valve.springPreload", FieldKind::number, 0, 2000, 470, "Spring preload", Quantity::none, "N");
        add("valve.valveMass", FieldKind::number, 0, 0.3, 0, "Valve mass", Quantity::mass);
        return out;
    }
}

namespace tee::ui
{
    // The hook (ui/app/Demo.h): E's lab fields into the made-up Spec, and E's telemetry (named as ValvesModel.h guesses)
    // following the script: the intake phaser advancing with speed, stage B above 5500 rpm, the springs floating at
    // 7200 rpm, cylinders 2, 3, 6 and 7 resting at light load under 3000 rpm.
    void valvesDemo(DemoExtras& x)
    {
        constexpr double kRpm = 3.14159265358979323846 / 30.0;
        if (x.fields != nullptr)
            for (FieldInfo& f : valvesDemoFields())
                x.fields->push_back(std::move(f));
        if (x.engine != nullptr)
        {
            x.engine->addTelemetry("valve.inPhase", [](const DemoEngine& e) {
                return std::clamp((e.rpm() - 1200.0) / 3000.0, 0.0, 1.0) * 40.0 * std::min(1.0, 0.3 + e.throttle());
            });
            x.engine->addTelemetry("valve.exPhase", [](const DemoEngine&) { return 0.0; });
            x.engine->addTelemetry("valve.stage", [](const DemoEngine& e) { return e.rpm() > 5500.0 ? 1.0 : 0.0; });
            x.engine->addTelemetry("valve.floatSpeed", [](const DemoEngine&) { return 7200.0 * kRpm; });
            x.engine->addTelemetry("valve.active", [](const DemoEngine& e) {
                return e.rpm() < 3000.0 && e.throttle() < 0.35 ? static_cast<double>(0xFF & ~0x66) : 255.0;
            });
        }
    }
}
