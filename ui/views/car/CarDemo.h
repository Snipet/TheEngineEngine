// ui/views/car/CarDemo.h: what the CAR workspace's tests and pictures add to U1's demo (ui/app/Demo.h) until its hook
// for a workspace's own demo data is there: the vehicle's live values the demo's telemetry does not have yet (each
// wheel's load, slip, force and torque; the acceleration), made from the demo's own speed and pedal at the script's
// time with plain sums, so the pictures show a car pulling in third. Kept here so it moves into the hook in a minute.
#pragma once

#include "app/Demo.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tee::ui::cardemo
{
    inline void setLive(DemoEngine& e, double mass = 1550.0, double wheelbase = 2.7, double cg = 0.5, double front = 0.53)
    {
        const double plate = e.value(e.findValue("engine.throttle"));
        const double accel = plate > 0.5 ? 4.2 * plate : -1.2;          // m/s^2: pulling in third, or lifting
        const double weight = mass * 9.81;
        const double moved = mass * accel * cg / wheelbase;              // load moved to the rear
        const double f = weight * front - moved, r = weight * (1.0 - front) + moved;
        const double slip = plate > 0.9 ? 0.16 : 0.03 * plate;           // the rear wheels, driven
        const char* w[] = { "[0]", "[1]", "[2]", "[3]" };
        for (int i = 0; i < 4; ++i)
        {
            const bool rear = i >= 2;
            const double load = (rear ? r : f) * 0.5 + (i == 3 ? -180.0 : i == 2 ? 180.0 : 0.0) * plate;   // the shaft's twist
            e.set(std::string("car.wheelLoad") + w[i], load);
            e.set(std::string("car.wheelSlip") + w[i], rear ? slip * (i == 3 ? 1.25 : 1.0) : 0.004);
            e.set(std::string("car.wheelForce") + w[i], rear ? mass * std::max(accel, 0.0) * 0.5 : 0.0);
            e.set(std::string("car.wheelTorque") + w[i], rear ? mass * std::max(accel, 0.0) * 0.5 * 0.33 : 0.0);
        }
        e.set("car.accel", accel);
        e.set("car.flags", slip > 0.1 ? 1 : 0);
    }
}
