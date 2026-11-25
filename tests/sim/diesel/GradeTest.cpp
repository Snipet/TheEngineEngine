// tests/sim/diesel/GradeTest.cpp: card D2: the 36 t rig (content/builds/truck-15l-six.json, the vehicle model, its
// eighteen gears worked by the paddles' robotised clutch; 300 K, 101325 Pa) pulling away loaded, up a 6 % grade and
// held on the way down by its brakes.
// (The cases "the paddles' clutch holds the compression brake's overrun at low revs" and "...stays shut under a floored
// pedal at low revs" are card D's as integrator, with the lead's leave, 25 Nov.)
#include "Diesel.h"

#include <cstdio>
#include <string>

using d2::Engine;

namespace
{
    // A driver: the pedal held; up a gear past 1900 rpm, down one under 1150 (by the road's speed and the gear's ratio) (not more often than every 1.5 s)
    struct Driver
    {
        Engine& e;
        double since = 10.0;
        int lo = 1;     // the lowest gear it shifts down to
        int hi = 18;

        void step(double dt)
        {
            REQUIRE(e.run(dt));
            since += dt;
            // (the speed the gear in gives the crank from the road's speed: a slipping clutch does not fool it)
            const double rpm = e.tel("car.speed") / 0.5 * e.tel("car.ratio") / 0.10471976;
            const int gear = static_cast<int>(e.tel("car.gear"));
            if (since < 1.5)
                return;
            if (gear == 0 || (rpm > 1900.0 && gear < hi))
            {
                e.ctl().shiftUp += 1;
                since = 0.0;
            }
            else if (rpm < 1150.0 && gear > lo)
            {
                e.ctl().shiftDown += 1;
                since = 0.0;
            }
        }
    };

    // (these drive the paddles' robotised clutch: the truck's gearbox set to card I's Manual with eighteen ratios; the
    // eighteen-speed's own lever is EighteenTest.cpp's)
    void startDriving(Engine& e)
    {
        e.spec("car.gearbox", 0);
        e.spec("car.gears", 18);
        e.spec("car.autoGearing", 0);
        const double r[18] = {14.40, 12.29, 8.56, 7.30, 6.05, 5.16, 4.38, 3.74, 3.20, 2.73, 2.29, 1.95, 1.62, 1.38, 1.17, 1.00, 0.86, 0.73};
        const int ratio = e.player->specIndex("car.ratio");
        REQUIRE(ratio >= 0);
        for (int i = 0; i < 18; ++i)
            e.player->setSpec(static_cast<std::size_t>(ratio), r[i], static_cast<std::size_t>(i));
        e.start();
        auto& c = e.ctl();
        c.mode = 1;
        c.lever = 0;
        c.clutch = 0.0f;
        c.brake = 0.0f;
        c.throttle = 0.0f;
        REQUIRE(e.run(1.0));
    }
} // namespace

TEST_CASE("diesel truck: a 36 t rig pulls away loaded on the paddles' gears", "[diesel][truck]")
{
    Engine e("truck-15l-six");
    startDriving(e);
    Driver d{e};
    e.ctl().throttle = 1.0f;
    double t = 0.0;
    while (t < 30.0 && e.tel("car.speed") < 30.0 / 3.6)
    {
        d.step(0.1);
        t += 0.1;
    }
    std::printf("loaded: 0 to 30 km/h in %.1f s, gear %.0f\n", t, e.tel("car.gear"));
    CHECK(t < 20.0);
    CHECK(e.tel("car.gear") >= 4.0);
}

TEST_CASE("diesel truck, full: a 36 t rig pulls away loaded and climbs a 6 % grade at the speed its power gives",
          "[diesel][truck][nightly]")
{
    Engine e("truck-15l-six");
    startDriving(e);
    Driver d{e};
    e.ctl().throttle = 1.0f;
    double t = 0.0;
    double t50 = -1.0;
    while (t < 90.0)
    {
        d.step(0.1);
        t += 0.1;
        if (t50 < 0.0 && e.tel("car.speed") > 50.0 / 3.6)
            t50 = t;
    }
    const double vLevel = e.tel("car.speed");
    std::printf("level: 0 to 50 km/h in %.1f s; after 90 s %.1f km/h in gear %.0f at %.0f rpm\n", t50, vLevel * 3.6,
                e.tel("car.gear"), e.tel("engine.speed") / 0.10471976);
    CHECK(t50 > 0.0);
    // the grade
    e.spec("env.grade", 0.06);
    double v = 0.0, p = 0.0;
    int n = 0;
    t = 0.0;
    while (t < 120.0)
    {
        d.step(0.1);
        t += 0.1;
        if (t > 90.0)
        {
            v += e.tel("car.speed");
            p += e.tel("engine.power");
            ++n;
        }
    }
    v /= n;
    p /= n;
    // what that power holds on the grade: P eta = m g (grade + rolling) v + 1/2 rho CdA v^3 (eta 0.92 through the
    // gearbox and axle; rolling 0.006, truck tyres' published 0.005 to 0.008)
    const double m = 36000.0, g = 9.81, eta = 0.92, crr = 0.006, cda = 6.0, rho = 1.177;
    double vp = 10.0;
    for (int i = 0; i < 60; ++i)
        vp = p * eta / (m * g * (0.06 + crr) + 0.5 * rho * cda * vp * vp);
    std::printf("6 %% grade: %.1f km/h in gear %.0f at %.0f rpm, the engine's %.0f kW (power and weight give %.1f km/h)\n", v * 3.6,
                e.tel("car.gear"), e.tel("engine.speed") / 0.10471976, p / 1000.0, vp * 3.6);
    // published: a 36 to 40 t rig of 400 to 450 kW holds some 45 to 60 km/h on 6 % (judgement from hauliers' and makers' figures)
    CHECK(v > 0.85 * vp);
    CHECK(v < 1.15 * vp);
    CHECK(v * 3.6 > 30.0);
    CHECK(v * 3.6 < 70.0);
}

TEST_CASE("diesel truck, full: on the way down a 6 % grade the brakes hold the rig's speed", "[diesel][truck][brake][nightly]")
{
    // Down 6 %, pedal up, a gear held: free, the rig runs away; with the compression brake on High and the exhaust
    // flap, its speed settles where the brakes' power is the grade's m g s v (less rolling and air)
    double vEnd[2] = {0.0, 0.0};
    for (int k = 0; k < 2; ++k)
    {
        Engine e("truck-15l-six");
        e.spec("ecu.brakeSwitch", k == 0 ? 0 : 3);
        e.spec("ecu.exBrakeSwitch", k == 0 ? 0 : 1);
        startDriving(e);
        Driver d{e};
        e.ctl().throttle = 1.0f;
        double t = 0.0;
        while (t < 60.0 && e.tel("car.speed") < 45.0 / 3.6)
        {
            d.step(0.1);
            t += 0.1;
        }
        e.ctl().throttle = 0.0f;
        e.spec("env.grade", -0.06);
        const int gear = static_cast<int>(e.tel("car.gear"));
        double v0 = e.tel("car.speed");
        for (int i = 0; i < 400; ++i)
        {
            REQUIRE(e.run(0.1));
            if (e.tel("engine.speed") / 0.10471976 > 2300.0)
                break;
        }
        vEnd[k] = e.tel("car.speed");
        std::printf("down 6 %%, gear %d, brakes %s: %.1f km/h -> %.1f km/h after 40 s at %.0f rpm, %.0f N m on the crank\n", gear,
                    k == 0 ? "off" : "on (High, flap)", v0 * 3.6, vEnd[k] * 3.6, e.tel("engine.speed") / 0.10471976, e.tel("engine.torque"));
    }
    CHECK(vEnd[1] < vEnd[0] - 3.0);
    CHECK(vEnd[1] * 3.6 < 70.0);
}

TEST_CASE("diesel truck: the paddles' clutch holds the compression brake's overrun at low revs, and the brake stays on",
          "[diesel][truck][brake]")
{
    // (card D2's finding, 25 Nov: in gear at some 1150 rpm, down 6 % with the compression brake on High and the flap, the
    // robotised clutch, closing with the engine's own revs, passed some 0.4 of its torque there; the brake's overrun (on
    // D2's corrected valves some -1450 N m) slipped it, the engine fell to its idle, the brake cut out below its 900 rpm
    // and the clutch opened for good with the gear in: the rig ran away. Now a clutch that slips backwards is held closed
    // by the road's speed: the brake stays on and the slip small. The fixture needs no diesel package: the brake as it
    // is on the branch (some -1170 N m here) against the clutch scaled to 2500 N m from 3500, so overrun over capacity is
    // D2's (measured 25 Nov without the hold: 891 N m of capacity at 1116 rpm, slip -150 rad/s by 13 s))
    Engine e("truck-15l-six");
    e.spec("ecu.brakeSwitch", 3);
    e.spec("ecu.exBrakeSwitch", 1);
    e.spec("car.clutchTorque", 2500.0);
    startDriving(e);
    Driver d{e};
    e.ctl().throttle = 1.0f;
    double t = 0.0;
    while (t < 60.0 && e.tel("car.speed") < 28.0 / 3.6)
    {
        d.step(0.1);
        t += 0.1;
    }
    // (up the gears with the pedal light until the road's speed through the gear is under 1250 rpm)
    e.ctl().throttle = 0.2f;
    auto roadRpm = [&] { return e.tel("car.speed") / 0.5 * e.tel("car.ratio") / 0.10471976; };
    for (int k = 0; k < 6 && roadRpm() > 1250.0; ++k)
    {
        e.ctl().shiftUp += 1;
        REQUIRE(e.run(1.5));
    }
    e.ctl().throttle = 0.0f;
    e.spec("env.grade", -0.06);
    const int gear = static_cast<int>(e.tel("car.gear"));
    const double r0 = roadRpm();
    double worst = 0.0;
    for (int i = 0; i < 100; ++i)
    {
        REQUIRE(e.run(0.1));
        worst = std::max(worst, std::fabs(e.get("car.m1.slipC")));
    }
    std::printf("down 6 %% in gear %d from %.0f rpm, the brake on High and the flap: after 10 s %.0f rpm (road %.0f), %.0f N m on the "
                "crank, the clutch's slip %.1f rad/s at worst, %.1f km/h\n",
                gear, r0, e.tel("engine.speed") / 0.10471976, roadRpm(), e.tel("engine.torque"), worst, e.tel("car.speed") * 3.6);
    CHECK(r0 < 1300.0);                                        // (the case's own: it starts where the clutch was half closed)
    CHECK(static_cast<int>(e.tel("car.gear")) == gear);
    CHECK(e.tel("engine.torque") < -500.0);                    // the brake still on
    CHECK(std::fabs(e.tel("engine.speed") / 0.10471976 - roadRpm()) < 30.0);   // the clutch closed: the engine at the road's
    CHECK(worst < 20.0);                                       // the slip bounded (the run-away reached 150 rad/s)
}

TEST_CASE("diesel truck: the paddles' clutch, once synchronised, stays shut under a floored pedal at low revs",
          "[diesel][truck]")
{
    // (the lead's ruling, 25 Nov: the robotised clutch closing with the engine's own revs, once locked in a gear, slipped
    // forward again under a floored pedal at 1070 rpm: the engine flared 130 rpm over the road for 3.9 s on the flat
    // (measured 25 Nov on main's law). A real automated box's clutch, once synchronised, stays clamped: the engine pulls
    // from the road's speed)
    Engine e("truck-15l-six");
    startDriving(e);
    Driver d{e};
    e.ctl().throttle = 1.0f;
    double t = 0.0;
    while (t < 60.0 && e.tel("car.speed") < 25.0 / 3.6)
    {
        d.step(0.1);
        t += 0.1;
    }
    auto roadRpm = [&] { return e.tel("car.speed") / 0.5 * e.tel("car.ratio") / 0.10471976; };
    e.ctl().throttle = 0.3f;
    for (int k = 0; k < 8 && roadRpm() > 1150.0; ++k)
    {
        e.ctl().shiftUp += 1;
        REQUIRE(e.run(1.5));
    }
    const int gear = static_cast<int>(e.tel("car.gear"));
    const double r0 = roadRpm();
    e.ctl().throttle = 1.0f;
    double worst = 0.0, gap = 0.0;
    for (int i = 0; i < 60; ++i)
    {
        REQUIRE(e.run(0.1));
        worst = std::max(worst, std::fabs(e.get("car.m1.slipC")));
        gap = std::max(gap, std::fabs(e.tel("engine.speed") / 0.10471976 - roadRpm()));
    }
    std::printf("floored in gear %d from %.0f rpm: after 6 s %.0f rpm (road %.0f), the clutch's slip %.2f rad/s at worst, "
                "the engine %.0f rpm from the road at worst, %.0f N m\n",
                gear, r0, e.tel("engine.speed") / 0.10471976, roadRpm(), worst, gap, e.tel("engine.torque"));
    CHECK(r0 < 1150.0);                                         // (the case's own: where the clutch was half closed)
    CHECK(static_cast<int>(e.tel("car.gear")) == gear);
    CHECK(worst < 3.0);                                         // shut: no flare (main's law: 13.8 rad/s)
    CHECK(gap < 30.0);                                          // the engine at the road's speed (main's: 132 rpm over it)
}
