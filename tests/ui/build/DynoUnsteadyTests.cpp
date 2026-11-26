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
        // The predictor's fields at each point, carried by name (the cursor's BOOST, the band under the plot).
        r.points[0].fields = { 151e3, 205e3 };
        const tee::ui::DynoTrace b = tee::ui::ServiceDyno::traceOf(r, "predicted", { "boost.pressure", "engine.manifold" });
        CHECK(b.fields.size() == 2 && b.fields[0] == "boost.pressure" && b.values.size() == 2);
        CHECK(b.values[0].size() == 1 && b.values[0][0] == 151e3 && b.values[1][0] == 205e3);
        std::printf("  1733 rpm: %s; %s\n", t.distrustText(1).c_str(), t.rippleText(1).c_str());
        return 0;
    }
}

#include "Png.h"
#include "views/dyno/Dyno.h"

#include <funkgui/canvas/SoftRaster.h>
#include <funkgui/text/FontService.h>

#include <chrono>
#include <thread>

namespace
{
    // The picture for the lead (nightly): first.json's predicted curve on DYNO as the program predicts it, its distrusted
    // points marked (not settled, forced), the cursor at 1733 rpm (the point C found: 293 N m against a steady 272). out/ui/dyno/.
    BUILD_TEST(dynoUnsteadyPictures, "build.dyno.unsteady.pictures")
    {
        std::vector<std::string> warnings;
        std::string error;
        CHECK_MSG(rig.build->loadFile(std::filesystem::path(TEE_UI_BUILDS_DIR) / "first.json", tee::BuildPart::Whole, warnings, error), error);
        tee::ui::ServiceDyno dyno(rig.sim, *rig.build);
        tee::ui::FrameServices services = tee::ui::buildServices(*rig.build, TEE_UI_BUILDS_DIR, rig.user, {});
        tee::ui::AppFrame frame(*rig.doc, rig.units, rig.telemetry, services, &dyno, nullptr, tee::ui::defaultSlots(), 1280, 800);
        funkgui::HeadlessHost host(frame, 0, 2.0f);
        host.tick(5);
        frame.show(frame.find("dyno"));
        const auto t0 = std::chrono::steady_clock::now();
        while (!dyno.refined() && std::chrono::steady_clock::now() - t0 < std::chrono::seconds(120))
        {
            host.tick(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        CHECK_MSG(dyno.refined(), "no refined curve in two minutes");
        host.tick(30);
        const tee::ui::DynoTrace& t = dyno.predicted();
        int flagged = 0;
        for (std::size_t i = 0; i < t.speed.size(); ++i)
        {
            if (t.distrustAt(i))
                ++flagged;
            if (t.distrustAt(i) || !t.rippleText(i).empty())
                std::printf("  %5.0f rpm  %6.1f N m  %s%s%s\n", t.speed[i] * 30.0 / 3.14159265358979, t.torque[i],
                            t.distrustAt(i) ? "RING " : "", t.distrustText(i).c_str(),
                            t.rippleText(i).empty() ? "" : (" | " + t.rippleText(i)).c_str());
        }
        std::printf("  %d of %zu points distrusted\n", flagged, t.speed.size());
        // The boost at each point beside its marks (the lead, 26 Nov): the predictor's, read at the point's end.
        int boost = -1;
        for (std::size_t f = 0; f < t.fields.size(); ++f)
            if (t.fields[f] == "boost.pressure")
                boost = static_cast<int>(f);
        CHECK_MSG(boost >= 0, "the predicted curve carries no boost.pressure");
        const std::filesystem::path dir = std::filesystem::path(TEE_UI_OUT_DIR) / "dyno";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        const auto shot = [&](const char* name) {
            const std::string path = (dir / name).string();
            tee::test::writePng(funkgui::rasterise(host.draw(), funkgui::FontService::get().atlas(), 2), path);
            std::printf("  %s\n", path.c_str());
        };
        shot("first.unsteady.png");
        const funkgui::Rect st = frame.stageRect();
        const float x = tee::ui::dynoPlotX(frame.current(), 1733.0 * 3.14159265358979 / 30.0);
        host.move(x, st.y + st.h * 0.35f);
        host.tick(10);
        shot("first.unsteady.cursor.png");
        return 0;
    }
}
