// tests/ui/MockSpec.h: a made-up Spec for J2's tests (card J, slice J2): the sections the real one will have (K3 writes
// it: sim/spec/*.fm), each with fields of every kind a generated page must handle: a number with a quantity, a number
// with a fixed unit, an integer, a choice, a switch, a hidden field, groups, a Curve16 and a Map16. Nothing here is the
// real Spec; the generated pages must work for any.
#pragma once

#include "kit/bind/Fields.h"

#include <string>
#include <vector>

namespace tee::test
{
    inline std::vector<tee::ui::FieldInfo> mockFields()
    {
        using tee::ui::FieldInfo;
        using tee::ui::FieldKind;
        using tee::ui::Quantity;
        std::vector<FieldInfo> out;
        std::string group;
        const auto add = [&](std::string path, FieldKind kind, double lo, double hi, double def, std::string label,
                             Quantity q = Quantity::none) -> FieldInfo& {
            FieldInfo f;
            f.path = std::move(path);
            f.kind = kind;
            f.min = lo;
            f.max = hi;
            f.def = def;
            f.label = std::move(label);
            f.quantity = q;
            f.group = group;
            out.push_back(std::move(f));
            return out.back();
        };
        const auto choice = [&](std::string path, std::string label, std::vector<std::string> names, int def = 0) -> FieldInfo& {
            FieldInfo& f = add(std::move(path), FieldKind::choice, 0, static_cast<double>(names.size() - 1), def, std::move(label));
            f.choices = std::move(names);
            return f;
        };
        constexpr double kRpm = 3.14159265358979323846 / 30.0;   // rad/s in one rpm

        group = "Block";
        add("engine.rev", FieldKind::integer, 0, 1e9, 0, "Revision").hidden = true;
        choice("engine.layout", "Layout", { "I4", "I6", "V6", "V8", "V10", "V12" }, 3);
        add("engine.cylinders", FieldKind::integer, 1, 16, 8, "Cylinders");
        add("engine.displacement", FieldKind::number, 0.0005, 0.008, 0.0057, "Displacement", Quantity::volume);
        add("engine.boreStroke", FieldKind::number, 0.6, 1.4, 1.0, "Bore/Stroke").decimals = 2;
        add("engine.compression", FieldKind::number, 7.0, 15.0, 10.5, "Compression", Quantity::ratio);
        group = "Rotating";
        add("engine.flywheel", FieldKind::number, 0.05, 1.0, 0.25, "Flywheel", Quantity::inertia).logTaper = true;
        add("engine.idleTrim", FieldKind::number, -200 * kRpm, 200 * kRpm, 0, "Idle trim", Quantity::rotation);
        add("engine.redlineTrim", FieldKind::number, -1500 * kRpm, 1500 * kRpm, 0, "Redline trim", Quantity::rotation);
        add("engine.rough", FieldKind::number, 0, 1, 0.1, "Roughness", Quantity::fraction);

        group = "";
        add("valve.rev", FieldKind::integer, 0, 1e9, 0, "Revision").hidden = true;
        add("valve.cam", FieldKind::number, 0, 1, 0.3, "Cam", Quantity::fraction);
        {
            FieldInfo& lift = add("valve.lift", FieldKind::curve, 0, 0.016, 0.008, "Intake lift", Quantity::length);
            lift.axes = { Quantity::angle, Quantity::none };
            lift.axisLo = { 0, 0 };
            lift.axisHi = { 720, 0 };
        }

        group = "Turbo";
        choice("boost.kind", "Aspiration", { "NA", "Turbo", "Twin turbo", "Supercharger" });
        add("boost.level", FieldKind::number, 0, 200e3, 80e3, "Boost", Quantity::pressure);
        add("boost.size", FieldKind::number, 0, 1, 0.5, "Turbo size", Quantity::fraction);
        add("boost.blowOff", FieldKind::toggle, 0, 1, 1, "Blow-off");
        group = "Wastegate";
        add("boost.wastegateSpring", FieldKind::number, 20e3, 150e3, 50e3, "Spring", Quantity::pressure);
        {
            FieldInfo& duty = add("boost.duty", FieldKind::map, 0, 1, 0.4, "Duty", Quantity::fraction);
            duty.axes = { Quantity::rotation, Quantity::pressure };
            duty.axisLo = { 1000 * kRpm, 0 };
            duty.axisHi = { 8000 * kRpm, 200e3 };
        }

        group = "";
        choice("exhaust.headers", "Headers", { "Log", "Shorty", "Long tube" }, 1);
        add("exhaust.pipe", FieldKind::number, 0.035, 0.09, 0.0635, "Pipe", Quantity::length);
        add("exhaust.crossover", FieldKind::toggle, 0, 1, 0, "X-pipe");
        add("exhaust.muffler", FieldKind::number, 0, 1, 0.5, "Muffler", Quantity::fraction);
        add("exhaust.tipAngle", FieldKind::number, -45, 45, 0, "Tip angle").unit = "deg";

        choice("fuel.kind", "Fuel", { "Petrol", "E85", "Methanol", "Diesel" });

        group = "Ignition";
        add("ecu.timing", FieldKind::number, -10, 10, 0, "Timing", Quantity::angle);
        {
            FieldInfo& spark = add("ecu.spark", FieldKind::map, -10, 50, 20, "Spark advance", Quantity::angle);
            spark.axes = { Quantity::rotation, Quantity::pressure };
            spark.axisLo = { 800 * kRpm, 20e3 };
            spark.axisHi = { 7500 * kRpm, 250e3 };
        }
        group = "Limiter";
        add("ecu.limiter", FieldKind::number, 3000 * kRpm, 9500 * kRpm, 6500 * kRpm, "Limiter", Quantity::rotation);
        add("ecu.pops", FieldKind::toggle, 0, 1, 0, "Overrun pops");
        add("ecu.starter", FieldKind::number, 0, 1, 0.5, "Starter", Quantity::fraction);
        {
            FieldInfo& curve = add("ecu.idleCurve", FieldKind::curve, 600 * kRpm, 1400 * kRpm, 850 * kRpm, "Idle by coolant",
                                   Quantity::rotation);
            curve.axes = { Quantity::temperature, Quantity::none };
            curve.axisLo = { 233.15, 0 };
            curve.axisHi = { 383.15, 0 };
        }

        group = "";
        choice("car.kind", "Vehicle", { "Car", "Truck" });
        add("car.mass", FieldKind::number, 600, 4000, 1500, "Mass", Quantity::mass);
        add("car.finalDrive", FieldKind::number, 2.0, 5.5, 3.55, "Final drive", Quantity::ratio);
        add("car.tyre", FieldKind::number, 0.25, 0.45, 0.33, "Tyre radius", Quantity::length);

        add("env.temperature", FieldKind::number, 233.15, 323.15, 293.15, "Air temperature", Quantity::temperature);
        add("env.pressure", FieldKind::number, 60e3, 105e3, 101.325e3, "Air pressure", Quantity::pressure);

        choice("sound.view", "View", { "Exhaust", "Cabin", "Engine bay", "Trackside" });
        add("sound.distance", FieldKind::number, 0.5, 100, 5, "Distance", Quantity::length).logTaper = true;
        add("sound.room", FieldKind::number, 0, 1, 0.2, "Room", Quantity::fraction);
        add("sound.level", FieldKind::number, 0, 1, 0.7, "Level", Quantity::fraction);
        return out;
    }
}
