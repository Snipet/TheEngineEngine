// ui/views/car/CarDemo.cpp: CAR's own demo data (U1's hook, ui/app/Demo.h): the vehicle's fields the demo's made-up
// Spec lacks (sim/vehicle/vspec.fm's VehSpec, at its defaults but a rear limited-slip differential), and each wheel's
// load, slip, force and drive torque, the acceleration and the flags, made from the script's pedal with plain sums
// (a car pulling in third: load moved rearwards, the driven rears slipping when the pedal is down).
#include "app/Demo.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tee::ui
{
    void carDemo(DemoExtras& x)
    {
        if (x.fields != nullptr)
        {
            const auto add = [&](const char* path, FieldKind kind, double lo, double hi, double def, const char* label,
                                 Quantity q, const char* group, std::vector<std::string> choices = {}) {
                FieldInfo f;
                f.path = path;
                f.kind = kind;
                f.min = lo;
                f.max = hi;
                f.def = def;
                f.label = label;
                f.quantity = q;
                f.group = group;
                f.choices = std::move(choices);
                x.fields->push_back(std::move(f));
            };
            add("car.cgHeight", FieldKind::number, 0.2, 2.5, 0.5, "Centre of mass height", Quantity::length, "Body");
            add("car.frontWeight", FieldKind::number, 0.2, 0.8, 0.53, "Weight on the front", Quantity::fraction, "Body");
            add("car.track", FieldKind::number, 0.8, 2.6, 1.55, "Track", Quantity::length, "Body");
            add("car.cdA", FieldKind::number, 0.2, 12.0, 0.7, "Drag area (Cd A)", Quantity::area, "Aero");
            add("car.clA", FieldKind::number, -2.0, 5.0, 0.3, "Downforce area (Cl A)", Quantity::area, "Aero");
            add("car.aeroBalance", FieldKind::number, 0.0, 1.0, 0.45, "Downforce on the front", Quantity::fraction, "Aero");
            add("car.gearbox", FieldKind::choice, 0, 4, 0, "Gearbox", Quantity::none, "Gearbox",
                { "Manual", "Sequential", "Dual clutch", "Automatic", "CVT" });
            add("car.gears", FieldKind::integer, 1, 18, 6, "Forward gears", Quantity::none, "Gearbox");
            const double ratios[] = { 3.36, 2.07, 1.43, 1.0, 0.84, 0.56 };
            for (int i = 0; i < 6; ++i)
            {
                const std::string p = "car.ratio[" + std::to_string(i) + "]";
                add(p.c_str(), FieldKind::number, 0.3, 20.0, ratios[i], "Ratio", Quantity::ratio, "Gearbox");
                x.fields->back().label = "Gear " + std::to_string(i + 1);
            }
            add("car.gearEfficiency", FieldKind::number, 0.8, 1.0, 0.97, "Gearbox efficiency", Quantity::fraction, "Gearbox");
            add("car.brakeDecel", FieldKind::number, 0.3, 2.5, 1.2, "Brakes", Quantity::none, "Brakes");
            x.fields->back().unit = "g";
            add("car.brakeBias", FieldKind::number, 0.3, 0.9, 0.65, "Brake bias to the front", Quantity::fraction, "Brakes");
            add("car.abs", FieldKind::toggle, 0, 1, 1, "ABS", Quantity::none, "Brakes");
            add("car.rearPreload", FieldKind::number, 0.0, 5000.0, 150.0, "Rear preload", Quantity::torque, "Differentials");
            add("car.rearLocked", FieldKind::toggle, 0, 1, 0, "Rear locked", Quantity::none, "Differentials");
            add("car.frontPreload", FieldKind::number, 0.0, 5000.0, 0.0, "Front preload", Quantity::torque, "Differentials");
            add("car.frontLocked", FieldKind::toggle, 0, 1, 0, "Front locked", Quantity::none, "Differentials");
        }
        if (x.engine == nullptr)
            return;
        DemoEngine& e = *x.engine;
        // The sums, from the document's fields where it has them.
        const auto accel = [](const DemoEngine& d) { return d.throttle() > 0.5 ? 4.2 * d.throttle() : -1.2; };
        const auto load = [accel](const DemoEngine& d, int w) {
            const double mass = d.field("car.mass", 1550.0), wb = d.field("car.wheelbase", 2.7), cg = d.field("car.cgHeight", 0.5);
            const double front = d.field("car.frontWeight", 0.53), weight = mass * 9.81, moved = mass * accel(d) * cg / wb;
            const double axle = w < 2 ? weight * front - moved : weight * (1.0 - front) + moved;
            return axle * 0.5 + (w == 3 ? -180.0 : w == 2 ? 180.0 : 0.0) * d.throttle();
        };
        const auto slip = [](const DemoEngine& d, int w) {
            if (w < 2)
                return 0.004;
            const double s = d.throttle() > 0.9 ? 0.16 : 0.03 * d.throttle();
            return s * (w == 3 ? 1.25 : 1.0);
        };
        for (int w = 0; w < 4; ++w)
        {
            const std::string i = "[" + std::to_string(w) + "]";
            e.addTelemetry("car.wheelLoad" + i, [load, w](const DemoEngine& d) { return load(d, w); });
            e.addTelemetry("car.wheelSlip" + i, [slip, w](const DemoEngine& d) { return slip(d, w); });
            e.addTelemetry("car.wheelForce" + i, [accel, w](const DemoEngine& d) {
                return w >= 2 ? d.field("car.mass", 1550.0) * std::max(accel(d), 0.0) * 0.5 : 0.0;
            });
            e.addTelemetry("car.wheelTorque" + i, [accel, w](const DemoEngine& d) {
                return w >= 2 ? d.field("car.mass", 1550.0) * std::max(accel(d), 0.0) * 0.5 * d.field("car.tyre", 0.33) : 0.0;
            });
        }
        e.addTelemetry("car.accel", accel);
        e.addTelemetry("car.flags", [slip](const DemoEngine& d) { return slip(d, 2) > 0.1 ? 1.0 : 0.0; });
    }
}
