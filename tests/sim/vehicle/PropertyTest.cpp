// tests/sim/vehicle/PropertyTest.cpp: the vehicle model is passive at its junctions with the crank (finding X-007: a
// heavy piston on a light flywheel ran away through an automatic's converter in neutral). Two properties:
//
//   standalone: a crank whose inertia varies round the turn, as a custom engine's moving masses make it (rig.fm's
//   `pistons`: J = jE + jp sin^2, the inertia torque -jp sin cos w^2 passed in the torque and J passed as the crank's
//   inertia), at the corners of light and heavy flywheels and pistons, with each gearbox, in neutral and in gear, under
//   random pedals: the crank's and the vehicle's kinetic energy never rises above where the engine's own work (its
//   torque less its friction, the inertia torque left out: over a turn it does none) would put it, and nothing turns
//   faster than a stated absurd speed;
//
//   in the engine: custom singles at the corners of their Spec's ranges (bore, stroke, piston mass, flywheel) in
//   car.model 1 with each gearbox, in neutral and in gear, random pedals, the development build's checks on: no check
//   stops a call, no block is refused, the engine's guard never puts a run-away crank at rest (FLAG_FAULT in
//   tel.engine.flags, HostStats::engineFaults), the crank stays under 3000 rad/s, and the sound is finite.
#include "Rig.h"

#include "tee/test/Scenario.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string>

using namespace tee::vehicle;

TEST_CASE("passive: a crank of varying inertia gains no energy from the vehicle, whatever the gearbox", "[vehicle][property]")
{
    int runs = 0;
    for (int box = 0; box < 5; ++box)
        for (double jE : {0.01, 0.5})
            for (double jp : {0.0, 0.3})
                for (int geared = 0; geared < 2; ++geared)
                {
                    INFO("gearbox " << box << ", jE " << jE << ", jp " << jp << ", in gear " << geared);
                    Rig rig;
                    rig.set("car.spec.gearbox", box);
                    rig.set("car.spec.drive", (box + geared) % 3);
                    rig.set("jE", jE);
                    rig.set("jp", jp);
                    rig.set("pistons", 1);
                    rig.set("noStall", 1);
                    rig.settle();
                    rig.seconds(0.3);
                    if (geared != 0)
                    {
                        if (box == Manual || box == Sequential || box == DualClutch)
                            rig.set("up", 1);
                        else
                            rig.set("lever", 1);
                    }
                    rig.function<void(void*)>("Rig::openBooks")(rig.instance().data());
                    for (int step = 0; step < 60; ++step)
                    {
                        rig.set("throttle", rig.random() < 0.2f ? 0.0 : rig.random());
                        rig.set("brake", rig.random() < 0.85f ? 0.0 : rig.random());
                        if (box == Manual && geared != 0)
                            rig.set("clutch", rig.random() < 0.7f ? 0.0 : rig.random());
                        rig.seconds(0.1);
                    }
                    const double excess = rig.get("excessMax"), top = rig.get("eMax");
                    // (the books close to 2 %: the crank's speed is a float, as the engine's is, and its rounding at a
                    // limiter that chatters each sample is not zero on the mean; a run-away is a thousandfold)
                    if (excess > 0.02 * top + 5.0 || rig.get("wMax") > 3000.0)
                        std::printf("passive: gearbox %d, jE %.2f, jp %.1f, in gear %d: energy up to %.0f J, above the "
                                    "engine's work by %.3f J; crank up to %.0f rad/s\n",
                                    box, jE, jp, geared, top, excess, rig.get("wMax"));
                    CHECK(rig.get("finite") == 1.0);
                    CHECK(excess <= 0.02 * top + 5.0);
                    CHECK(rig.get("wMax") <= 3000.0);     // (a light crank on heavy pistons swings to 1200 at 680 mean)
                    runs += 1;
                }
    std::printf("passive: %d runs of 6 s, every one within its books\n", runs);
}

namespace
{
    // custom singles at the given corners of their Spec's ranges, with the given gearboxes, in neutral and in gear:
    // random pedals, the checks on; no check stops a call, no block is refused, the engine's guard never finds a fault
    // (FLAG_FAULT, HostStats::engineFaults), the crank stays under 3000 rad/s
    int passiveEngine(std::initializer_list<const char*> boxes, std::initializer_list<int> corners)
    {
        int runs = 0;
        for (const char* box : boxes)
            for (int corner : corners)
                for (int geared = 0; geared < 2; ++geared)
                {
                    // a heavy piston on a long stroke and a light flywheel (X-007's), a light one on a short stroke and
                    // a heavy flywheel, and the two between
                    const double bore[4] = {0.037, 0.12, 0.037, 0.12};
                    const double stroke[4] = {0.153, 0.05, 0.05, 0.153};
                    const double piston[4] = {18.4, 0.3, 20.0, 5.0};
                    const double flywheel[4] = {0.24, 1.0, 0.0, 0.5};
                    INFO(box << ", corner " << corner << ", in gear " << geared);
                    tee::test::Scenario s;
                    s.checks();
                    s.at(0.0).spec("engine.custom", 1.0).spec("engine.cylinders", 1.0).spec("engine.layout", "Single");
                    s.at(0.0).spec("engine.bore", bore[corner]).spec("engine.stroke", stroke[corner]);
                    s.at(0.0).spec("engine.pistonMass", piston[corner]).spec("engine.flywheel", flywheel[corner]);
                    s.at(0.0).spec("car.model", "Vehicle").spec("car.gearbox", std::string(box));
                    s.at(0.0).mode(1).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On);
                    s.at(0.25).ignition(tee::Start).until(1.5).ignition(tee::On);
                    if (geared != 0)
                    {
                        if (std::string(box) == "Automatic" || std::string(box) == "CVT")
                            s.at(1.6).lever(1);
                        else
                            s.at(1.6).shiftUp();
                    }
                    unsigned seed = 12345u + static_cast<unsigned>(corner * 7 + geared);
                    auto rnd = [&seed] {
                        seed = seed * 1664525u + 1013904223u;
                        return static_cast<double>(seed >> 8) / 16777216.0;
                    };
                    for (double t = 1.8; t < 5.0; t += 0.2)
                        s.at(t).throttle(rnd() < 0.2 ? 0.0 : rnd()).brake(rnd() < 0.8 ? 0.0 : rnd());
                    const tee::test::Run r = s.run(5.0);
                    REQUIRE(r.ok());
                    const double top = r.tel("engine.speed").max();
                    // (the engine's guard, ae5c99b, puts a crank past five times its redline or not finite at rest and
                    // sets FLAG_FAULT for that call: a run-away it caught is a run-away all the same)
                    int faults = 0;
                    for (double f : r.tel("engine.flags").values())
                        faults += (static_cast<int>(f) & 4) != 0 ? 1 : 0;
                    CHECK(faults == 0);
                    CHECK(r.host().engineFaults == 0);
                    if (r.host().stops != 0 || r.host().guardTrips != 0 || top > 3000.0 || faults != 0
                        || r.host().engineFaults != 0)
                        std::printf("passive.engine: %s, corner %d, in gear %d: stops %u, guard trips %u, faults %d (%u), "
                                    "crank up to %.0f rad/s (%s)\n",
                                    box, corner, geared, r.host().stops, r.host().guardTrips, faults,
                                    r.host().engineFaults, top,
                                    r.stopReason().c_str());
                    CHECK(r.host().stops == 0);
                    CHECK(r.host().guardTrips == 0);
                    CHECK(top < 3000.0);
                    CHECK(r.audio().finite());
                    runs += 1;
                }
        return runs;
    }
} // namespace

// The quick form, in every run (01-rules.md §3: under 30 s): X-007's own corner (a heavy piston on a long stroke and a
// light flywheel), which ran away through an automatic's converter, with the Automatic and a Manual, in neutral and in
// gear. Re-proven after the engine's guard landed (ae5c99b), in a scratch copy of today's tree: with f9e26f1's two
// fixes taken out (the crank stepped with configure's inertia, the converter's torque unlimited) the automatic's runs
// fail, the guard restarting the engine 392 times in 5 s (FLAG_FAULT); with them in, all four pass.
TEST_CASE("passive.engine: X-007's single, automatic and manual, never runs away", "[vehicle][property]")
{
    const int runs = passiveEngine({"Automatic", "Manual"}, {0});
    std::printf("passive.engine (quick): %d runs of 5 s with the checks on\n", runs);
}

// The full form, nightly (ctest --preset nightly; CI's soak tier): every gearbox at every corner.
TEST_CASE("passive.engine: custom singles at their Spec's corners with each gearbox never run away",
          "[vehicle][property][nightly]")
{
    const int runs = passiveEngine({"Manual", "Sequential", "Dual clutch", "Automatic", "CVT"}, {0, 1, 2, 3});
    std::printf("passive.engine: %d runs of 5 s with the checks on\n", runs);
}
