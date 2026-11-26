// tests/ui/build/DynoUnsteadyTests.cpp: what card C's predictor says of each point reaches the dyno's trace, its reasons
// apart (the lead's ruling 26 Nov, on C's numbers: first.json's 1733 rpm point reads 293 N m against a steady 272, not
// settled; a settled point whose boost ripples is known to its standard error). ServiceDyno::traceOf sets
// DynoTrace::pointFlags from PredictPoint's settled, approachForced and boostHunting: not settled and forced are
// distrusted (distrustText); a ripple on a settled point is not, and says rippleText; a limiter's point is neither.
#include "build/BuildKit.h"

#include "app/host/HostSources.h"

#include "tee/Predict.h"

namespace
{
    using tee::test::build::Rig;

    BUILD_TEST(dynoUnsteadyTest, "build.dyno.unsteady")
    {
        (void)rig;
        tee::PredictCurve c;
        const auto point = [&c](double rpm, bool settled, bool hunting, bool forced, bool limited) {
            tee::PredictPoint p;
            p.speed = rpm * 3.14159265358979 / 30.0;
            p.torque = 272.0;
            p.power = p.torque * p.speed;
            p.settled = settled;
            p.settledCycles = settled ? 8 : 0;
            p.boostHunting = hunting;
            p.approachForced = forced;
            p.limited = limited;
            c.points.push_back(p);
        };
        point(1500.0, true, false, false, false);    // 0 plain
        point(1733.0, false, true, false, false);    // 1 first.json's: not settled, boost hunting
        point(2000.0, true, false, true, false);     // 2 its state forced
        point(7000.0, false, true, false, true);     // 3 the limiter's: apart, not unsteady
        const tee::ui::DynoTrace t = tee::ui::ServiceDyno::traceOf(c, "predicted");
        CHECK(t.pointFlags.size() == 4);
        CHECK(!t.distrustAt(0) && t.distrustText(0).empty() && t.rippleText(0).empty());
        // first.json's 1733 rpm kind: not settled (distrusted), and its boost ripples (said, not distrusted for it).
        CHECK(t.distrustAt(1));
        CHECK_MSG(t.distrustText(1) == "not settled within its cycles", t.distrustText(1));
        CHECK_MSG(t.rippleText(1) == "boost ripples: torque a mean over whole windows", t.rippleText(1));
        CHECK(t.distrustAt(2));
        CHECK_MSG(t.distrustText(2) == "its state forced: the approach could not release the latch", t.distrustText(2));
        CHECK(!t.distrustAt(3) && t.distrustText(3).empty() && t.rippleText(3).empty());
        // A settled point whose boost ripples: no distrust, its words only.
        tee::PredictCurve r;
        tee::PredictPoint q;
        q.speed = 6000.0 * 3.14159265358979 / 30.0;
        q.torque = 285.0;
        q.settled = true;
        q.settledCycles = 8;
        q.boostHunting = true;
        r.points.push_back(q);
        const tee::ui::DynoTrace s = tee::ui::ServiceDyno::traceOf(r, "predicted");
        CHECK(!s.distrustAt(0) && !s.rippleText(0).empty());
        std::printf("  1733 rpm: %s; %s\n", t.distrustText(1).c_str(), t.rippleText(1).c_str());
        return 0;
    }
}
