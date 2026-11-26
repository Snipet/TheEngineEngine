// tests/sim/calibration/na/FastTest.cpp: card N1's fast tier in one process (the steward's budget, 26 Nov: calibration-na
// at 115 test-s against its 80 on CI's runner, of which some 6 s a case was the program's compile). Each section is a
// case that stood alone until then, under its old name, on Player.h's one compiled player, reset for each build. The
// warm starts keep their own cases (their compiled Sim is kept the same way, one a process: WarmStartTest.cpp).
#include "Player.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("calibration-na's fast tier on one compiled player: the air's account, individual throttles, starts on the "
          "dyno, tiny charge paths",
          "[calibration][na]")
{
    SECTION("the fresh air trapped at a warm idle is the air through the throttle")
    n1::freshAirIsThrottleAir();
    SECTION("individual throttles: the manifold reading stays above 5 kPa through a limiter and a lift")
    n1::itbManifoldAbove5kPa();
    SECTION("a start on the engine dyno: the 15 L diesel and a boosted engine fire and pull")
    n1::dynoStartTractorAndTurbo();
    SECTION("a start on the engine dyno: the 2.0 common-rail diesel fires and pulls")
    n1::dynoStartCommonRail();
    SECTION("X-022 X-024 and X-026 together: tiny charge paths start idle and blip without a fault or a vacuum")
    n1::tinyChargePathsHold();
}
