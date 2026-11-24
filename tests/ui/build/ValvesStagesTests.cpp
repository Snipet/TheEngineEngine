// tests/ui/build/ValvesStagesTests.cpp: U2's ENGINE section and VALVES on the two-stage archetype (content/builds/
// archetypes/two-stage-cam-20.json: stage A 8.5 mm / 236 deg, stage B 12.65 mm / 280 deg at 6000 rpm), through the real
// Spec's document and the application's frame, with telemetry stepping valve.stage 0 to 1 as engine.speed passes the
// stage speed and a Scope that gives the lifts as the simulator does (valvetrain.fm liftIn: a share of the taller of
// the two stages' cams).
//
//   ui.build.stages.archetype   ENGINE's section scales the Scope's share to the taller cam, so stage A is drawn
//                               at A's lift and B at B's (A's alone drew A short and B no taller than A); VALVES
//                               draws the stage in use (its peak handle on B's taller lobe once B is in); pictures of
//                               both stages at density 1 and 2 under out/ui/
#include "build/BuildKit.h"

#include "views/engine/EngineWorkspace.h"
#include "views/valves/ValvesModel.h"
#include "views/valves/ValvesWorkspace.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
    using tee::test::build::Rig;
    namespace vv = tee::ui::valves;

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

    // The simulator's Scope for cylinder 1 as the lab fills it: each degree's lift a share of the taller cam of the
    // two stages, from the stage in use; a pressure and volume so the section draws.
    struct StageScope final : tee::ui::ScopeSource
    {
        const tee::ui::FieldDoc& doc;
        int stage = 0;
        std::uint64_t gen = 1;
        explicit StageScope(const tee::ui::FieldDoc& d) : doc(d) {}
        int find(std::string_view n) const override
        {
            const char* names[] = { "pressure", "volume", "liftIntake", "liftExhaust" };
            for (int i = 0; i < 4; ++i)
                if (n == names[i])
                    return i;
            return -1;
        }
        int read(int h, float* out, int max) override
        {
            const vv::Train t = vv::trainOf(doc);
            const vv::Lobe& in = stage == 1 ? t.inB : t.in;
            const vv::Lobe& ex = stage == 1 ? t.exB : t.ex;
            const double topIn = std::max(t.in.lift, t.inB.lift), topEx = std::max(t.ex.lift, t.exB.lift);
            const int n = std::min(max, 720);
            for (int d = 0; d < n; ++d)
            {
                const double a = d;
                double v = 0.0;
                if (h == 0)
                    v = 101325.0 * (1.0 + 20.0 * std::exp(-((a - 370.0) * (a - 370.0)) / 800.0));
                else if (h == 1)
                    v = 5.0e-5 + 4.5e-4 * 0.5 * (1.0 - std::cos(a * 3.14159265358979 / 180.0));
                else if (h == 2)
                    v = in.liftAt(a) / topIn;
                else
                    v = ex.liftAt(a) / topEx;
                out[d] = static_cast<float>(v);
            }
            return n;
        }
        std::uint64_t generation() const override { return gen; }
    };

    BUILD_TEST(stagesArchetypeTest, "build.stages.archetype")
    {
        std::vector<std::string> warnings;
        std::string error;
        const auto file = std::filesystem::path(TEE_UI_BUILDS_DIR) / "archetypes" / "two-stage-cam-20.json";
        CHECK_MSG(rig.build->loadFile(file, tee::BuildPart::Whole, warnings, error), error);
        tee::ui::FieldDoc& doc = *rig.doc;
        const vv::Train t = vv::trainOf(doc);
        CHECK(t.stages && t.inB.known() && t.exB.known());
        CHECK_MSG(std::fabs(t.in.lift - 0.0085) < 1e-6 && std::fabs(t.inB.lift - 0.01265) < 1e-6,
                  std::to_string(t.in.lift) + " " + std::to_string(t.inB.lift));

        for (const float dpi : { 1.0f, 2.0f })
        {
            SetTelemetry tel;
            tel.set("engine.state", 2.0);
            tel.set("engine.oilTemperature", 360.0);
            tel.set("engine.oilPressure", 400000.0);
            tel.set("engine.speed", 5500.0 * 3.14159265358979 / 30.0);
            tel.set("valve.stage", 0.0);
            tel.set("valve.inPhase", 0.0);
            // the crank where the intake is at its peak (cylinder 1's angle in its cycle)
            tel.set("engine.crank", t.in.centre);
            StageScope scope(doc);
            tee::ui::FrameServices services = tee::ui::buildServices(*rig.build, TEE_UI_BUILDS_DIR, rig.user, {});
            tee::ui::Units units;
            tee::ui::AppFrame frame(doc, units, tel, services, nullptr, &scope);
            funkgui::HeadlessHost host(frame, 0, dpi);
            host.resize(1100, 700);
            host.tick(10);
            const std::string d = dpi == 1.0f ? ".dpi1" : "";
            const auto shot = [&](const std::string& name) {
                const std::string path = std::string(TEE_UI_OUT_DIR) + "/" + name + d + ".png";
                host.draw();   // (the picture is the last frame drawn)
                CHECK_MSG(host.writePng(path.c_str(), 1), path);
            };

            // ENGINE: the full lift the section scales to is B's, the taller; the Scope's share peaks at A's lift
            // with A in and at B's with B in, so the two stages are drawn at their own lifts
            const int we = frame.find("engine");
            CHECK(we >= 0);
            if (we < 0)
                return 0;
            frame.show(we);
            host.tick(10);
            const auto [fullIn, fullEx] = tee::ui::engineFullLifts(frame.workspace(we));
            CHECK_MSG(std::fabs(fullIn - 0.01265) < 1e-6, std::to_string(fullIn));
            CHECK(std::isfinite(fullEx) && fullEx >= t.ex.lift);
            const auto peakOf = [&](int stage) {
                scope.stage = stage;
                std::vector<float> v(720);
                scope.read(2, v.data(), 720);
                return static_cast<double>(*std::max_element(v.begin(), v.end())) * fullIn;
            };
            const double drawnA = peakOf(0), drawnB = peakOf(1);
            CHECK_MSG(std::fabs(drawnA - 0.0085) < 2e-5 && std::fabs(drawnB - 0.01265) < 2e-5,
                      std::to_string(drawnA) + " " + std::to_string(drawnB));
            scope.stage = 0;
            ++scope.gen;
            host.tick(5);
            const std::uint64_t kA = frame.workspace(we).drawKey();
            CHECK(tee::ui::engineStageWords(frame.workspace(we)).first == "STAGE A");
            CHECK(tee::ui::engineStageWords(frame.workspace(we)).second.empty());   // below the stage speed: no reason
            shot("stages.engine.A");
            // past 6000 rpm: stage B (the Scope's next copy is B's)
            tel.set("engine.speed", 6500.0 * 3.14159265358979 / 30.0);
            tel.set("valve.stage", 1.0);
            scope.stage = 1;
            ++scope.gen;
            host.tick(5);
            CHECK(frame.workspace(we).drawKey() != kA);
            CHECK(tee::ui::engineStageWords(frame.workspace(we)).first == "STAGE B");
            shot("stages.engine.B");
            // past the stage speed with cold oil: A stays in, and ENGINE says why in VALVES' words
            tel.set("valve.stage", 0.0);
            tel.set("engine.oilTemperature", 300.0);
            scope.stage = 0;
            ++scope.gen;
            host.tick(5);
            const auto held = tee::ui::engineStageWords(frame.workspace(we));
            CHECK_MSG(held.first == "STAGE A" && held.second.find("B WAITS: OIL") != std::string::npos, held.first + " / " + held.second);
            shot("stages.engine.held");
            tel.set("engine.oilTemperature", 360.0);
            tel.set("valve.stage", 1.0);
            scope.stage = 1;
            ++scope.gen;
            host.tick(5);

            // VALVES: the stage in use is the one drawn and edited: its peak handle stands on B's taller lobe
            const int wv = frame.find("valves");
            CHECK(wv >= 0);
            if (wv < 0)
                return 0;
            frame.show(wv);
            host.tick(10);
            // the same reason on VALVES, word for word
            tel.set("engine.oilTemperature", 300.0);
            CHECK_MSG(tee::ui::valvesOilWords(frame.workspace(wv)).find(held.second) != std::string::npos,
                      tee::ui::valvesOilWords(frame.workspace(wv)) + " / " + held.second);
            tel.set("engine.oilTemperature", 360.0);
            host.tick(2);
            const float yB = tee::ui::valvesHandle(frame.workspace(wv), 0).y;
            shot("stages.valves.B");
            tel.set("valve.stage", 0.0);
            tel.set("engine.speed", 5500.0 * 3.14159265358979 / 30.0);
            scope.stage = 0;
            ++scope.gen;
            host.tick(5);
            const float yA = tee::ui::valvesHandle(frame.workspace(wv), 0).y;
            CHECK_MSG(yB < yA - 10.0f, std::to_string(yA) + " " + std::to_string(yB));   // B's peak drawn higher
            shot("stages.valves.A");
        }
        std::printf("  two-stage archetype: ENGINE's section at A's 8.5 mm then B's 12.65 mm; VALVES on the stage in use\n");
        return 0;
    }
}
