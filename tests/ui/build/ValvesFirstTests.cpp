// tests/ui/build/ValvesFirstTests.cpp: U2's VALVES and ENGINE on the first build a person meets (content/builds/
// first.json: the 2.0 L turbo four, valve.model Lab, an intake phaser of 60 degrees, no two-stage cams), through the
// real Spec's document and the application's frame, with the first launch's TRY FIRST strip showing (the stage's rect
// then shorter) and the simulator's telemetry names set as it gives them (a cold start, then warm).
//
//   ui.build.valves.first   the cam editor (not a two-stroke's port timing), its handles hit where they are drawn
//                           with the strip showing, a drag is one undo step; the phaser's target and phase and stage B
//                           follow telemetry; what the simulator says holds them back (tel.valve.waiting) is said
//                           in words, and goes when nothing waits
#include "build/BuildKit.h"

#include "views/engine/EngineModel.h"
#include "views/engine/TwoStroke.h"
#include "views/valves/ValvesModel.h"
#include "views/valves/ValvesWorkspace.h"

#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace
{
    using tee::test::build::Rig;

    // The simulator's telemetry by its names, set by the test (what tel gives on the running engine).
    struct SetTelemetry final : tee::TelemetrySource
    {
        std::vector<std::string> names;
        std::vector<double> values;
        void set(const std::string& n, double v)
        {
            for (std::size_t i = 0; i < names.size(); ++i)
                if (names[i] == n)
                {
                    values[i] = v;
                    return;
                }
            names.push_back(n);
            values.push_back(v);
        }
        int findValue(std::string_view n) const override
        {
            for (std::size_t i = 0; i < names.size(); ++i)
                if (names[i] == n)
                    return static_cast<int>(i);
            return -1;
        }
        double value(int i) const override { return values[static_cast<std::size_t>(i)]; }
        float outputPeak() const override { return 0.0f; }
    };

    BUILD_TEST(valvesFirstTest, "build.valves.first")
    {
        // the first build, loaded as the application opens it
        std::vector<std::string> warnings;
        std::string error;
        CHECK_MSG(rig.build->loadFile(std::filesystem::path(TEE_UI_BUILDS_DIR) / "first.json", tee::BuildPart::Whole, warnings, error),
                  error);
        tee::ui::FieldDoc& doc = *rig.doc;
        namespace en = tee::ui::engine;
        namespace vv = tee::ui::valves;
        // custom on, four-stroke: the cam editor, not port timing (twoStrokeRuns, D's configure)
        CHECK(en::fieldValue(doc, en::EF::custom) >= 0.5);
        CHECK(!en::twoStrokeRuns(doc) && !en::twoStrokeOf(doc).on && !en::layoutOf(doc).twoStroke);
        const vv::Train t0 = vv::trainOf(doc);
        CHECK(t0.editable && !t0.stages);
        CHECK_MSG(std::fabs(t0.inPhaseRange - 60.0) < 1e-6, std::to_string(t0.inPhaseRange));

        // the frame over it, the strip showing, the engine cold just after a start (the oil at the air's 15 C)
        SetTelemetry tel;
        tel.set("engine.state", 2.0);
        tel.set("engine.speed", 6800.0 * en::kPi / 30.0);
        tel.set("engine.oilTemperature", 288.2);
        tel.set("engine.coolant", 289.0);
        tel.set("valve.waiting", 4.0);    // N1's: the phasers slowed by cold oil
        tel.set("valve.waitTemp", 0.0);
        tel.set("valve.inPhase", 0.0);
        tel.set("valve.inPhaseTarget", 3.0);
        tel.set("valve.stage", 0.0);
        tee::ui::FrameServices services = tee::ui::buildServices(*rig.build, TEE_UI_BUILDS_DIR, rig.user, {});
        services.tryFirst = [] { return std::string("Start it, then rev it"); };
        tee::ui::Units units;
        tee::ui::AppFrame frame(doc, units, tel, services);
        funkgui::HeadlessHost host(frame, 0, 1.0f);
        host.resize(1100, 700);
        host.tick(10);
        const int w = frame.find("valves");
        CHECK(w >= 0);
        if (w < 0)
            return 0;
        frame.show(w);
        host.tick(10);
        tee::ui::Workspace& ws = frame.workspace(w);
        // the strip takes the stage's top: the stage is shorter than the plain frame's
        rig.frame->show(rig.frame->find("valves"));
        rig.host->tick(5);
        CHECK_MSG(ws.stage().y > rig.frame->workspace(rig.frame->find("valves")).stage().y + 4.0f,
                  "the TRY FIRST strip is not over the stage");
        CHECK(tee::ui::valvesHandle(ws, 100).x < 0.0f);   // no port marks: not a two-stroke

        // the intake's peak, pressed where it is drawn: its centreline and lift move, one undo step
        const int fc = doc.find("valve.inCentre"), fl = doc.find("valve.inLift"), fd = doc.find("valve.inDurSeat");
        CHECK(fc >= 0 && fl >= 0 && fd >= 0);
        if (fc < 0 || fl < 0 || fd < 0)
            return 0;
        const double c0 = doc.get(fc), l0 = doc.get(fl), d0 = doc.get(fd);
        funkgui::Point at = tee::ui::valvesHandle(ws, 0);
        CHECK(at.x >= 0.0f && at.y >= ws.stage().y && at.y <= ws.stage().bottom());
        host.drag(at.x, at.y, at.x + 24.0f, at.y - 20.0f, 8);
        CHECK_MSG(doc.get(fc) > c0 && doc.get(fl) > l0 && doc.get(fd) == d0, "the peak's drag did not reach the build");
        CHECK(rig.build->undo());
        CHECK(doc.get(fc) == c0 && doc.get(fl) == l0);
        // its closing seat mark: the duration
        host.tick(3);
        at = tee::ui::valvesHandle(ws, 2);
        host.drag(at.x, at.y, at.x + 20.0f, at.y, 8);
        CHECK_MSG(doc.get(fd) > d0 && doc.get(fc) == c0, "the seat mark's drag did not reach the build");
        CHECK(rig.build->undo());

        // cold: the phaser parked and saying why; nothing about stages (this build has one)
        std::string words = tee::ui::valvesOilWords(ws);
        CHECK_MSG(words.find("SLOWED: COLD OIL") != std::string::npos, words);
        CHECK_MSG(words.find("B WAITS") == std::string::npos, words);
        // the phase and its target are drawn from telemetry: a move of either is something to draw
        const std::uint64_t k0 = ws.drawKey();
        tel.set("valve.inPhaseTarget", 13.0);
        CHECK(ws.drawKey() != k0);
        // warm: the phaser free, at its target; the words gone
        tel.set("engine.oilTemperature", 360.0);
        tel.set("valve.waiting", 0.0);
        tel.set("valve.inPhase", 13.0);
        host.tick(2);
        CHECK_MSG(tee::ui::valvesOilWords(ws).empty(), tee::ui::valvesOilWords(ws));

        // two-stage cams on: B drawn and its handles live; stage B waits for 60 C oil, then follows tel.valve.stage
        doc.set(doc.find("valve.stages"), 1.0);
        host.tick(3);
        const vv::Train t1 = vv::trainOf(doc);
        CHECK(t1.stages && t1.inB.known() && t1.exB.known());
        tel.set("valve.waiting", 8.0);   // the stage waiting for the coolant at 10 C
        tel.set("engine.coolant", 278.0);
        tel.set("valve.waitTemp", 283.0);
        host.tick(2);
        words = tee::ui::valvesOilWords(ws);
        CHECK_MSG(words.find("B WAITS: COOLANT") != std::string::npos && words.find("SLOWED") == std::string::npos, words);
        tel.set("valve.waiting", 0.0);
        tel.set("valve.waitTemp", 0.0);
        tel.set("valve.stage", 1.0);
        host.tick(2);
        CHECK(tee::ui::valvesOilWords(ws).empty());
        // with B engaged the editor edits B: its peak's drag moves B's centreline, not A's
        const int fcB = doc.find("valve.inCentreB");
        CHECK(fcB >= 0);
        if (fcB >= 0)
        {
            const double b0 = doc.get(fcB);
            at = tee::ui::valvesHandle(ws, 0);
            host.drag(at.x, at.y, at.x + 24.0f, at.y, 8);
            CHECK_MSG(doc.get(fcB) != b0 && doc.get(fc) == c0, "stage B's peak did not take the drag");
            CHECK(rig.build->undo());
        }
        std::printf("  VALVES on first.json: cam editor, handles hit with TRY FIRST showing, phaser and stage B follow "
                    "telemetry, cold oil said in words\n");
        return 0;
    }
}
