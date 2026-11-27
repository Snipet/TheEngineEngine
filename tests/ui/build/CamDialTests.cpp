// tests/ui/build/CamDialTests.cpp: the reference's CAM DIAL (spec.valve.cam) where it is read and where it is not (N1's
// finding, the lead's word, 26 Nov): under the Lab model a piston engine's cams are the intake and exhaust pages and the
// dial does nothing, so VALVES leaves it out of its MODEL section and the frame's generated valve page (nothing
// selected) leaves it out too (Workspace::sectionOmits), with no line on either; a rotary's ports still run from it
// under Lab, so there it stays, named for them; under the Reference model it is live and the lab's fields carry their
// word (Workspace::sectionNotes). X-050.
// The pages follow valve.model as it is set, the selection unmoved. A two-stroke's VALVES shows its ports alone (N1:
// nothing reads spec.valve there). Made-up states over the real Spec (the demo's document has no valve.cam). Pictures: out/ui/valves/camdial.*.png.
#include "build/BuildKit.h"

#include "Png.h"
#include "views/engine/EngineWorkspace.h"

#include <funkgui/canvas/SoftRaster.h>
#include <funkgui/text/FontService.h>

namespace
{
    constexpr const char* kDead = "CAM DIAL IS THE REFERENCE";   // a Lab piston build's line, ruled out (the lead, 26 Nov)
    constexpr const char* kRotary = "ON A ROTARY THE CAM DIAL TIMES THE PORTS";
    constexpr const char* kLabOff = "THE LAB IS OFF: ITS FIELDS TAKE EFFECT WITH";

    BUILD_TEST(camDialTest, "build.valves.camdial")
    {
        tee::ui::AppFrame& f = *rig.frame;
        const int valves = f.find("valves");
        CHECK_MSG(valves >= 0, "no VALVES on the rail");
        if (valves < 0)
            return 0;
        const auto set = [&](const char* path, double v) {
            const int i = rig.doc->find(path);
            CHECK_MSG(i >= 0, path);
            if (i >= 0)
                rig.doc->set(i, v);
        };
        const auto show = [&](const char* sel) {
            f.show(valves);
            f.select(sel);
            rig.host->tick(5);
            std::string all;
            for (int i = 0; i < f.inspectorPageCount(); ++i)
                all += f.inspectorGui(i);
            return all;
        };
        const std::filesystem::path dir = std::filesystem::path(TEE_UI_OUT_DIR) / "valves";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        const auto shot = [&](const char* name) {
            const std::string path = (dir / name).string();
            tee::test::writePng(funkgui::rasterise(rig.host->draw(), funkgui::FontService::get().atlas(), 2), path);
            std::printf("  %s\n", path.c_str());
        };

        // Lab, a piston engine: the dial out of MODEL and out of the generated page, and no line on either (nothing is
        // promised; the cams are the next sections).
        set("valve.model", 1.0);
        set("engine.layout", 0.0);
        std::string t = show("valve.timing");
        CHECK_MSG(t.find("valve.model") != std::string::npos && t.find("valve.cam ") == std::string::npos, t);
        CHECK_MSG(t.find(kDead) == std::string::npos && t.find("color=yellow") == std::string::npos, t);
        shot("camdial.lab.timing.png");
        std::string g = show("");
        CHECK_MSG(g.find(kDead) == std::string::npos && g.find("color=yellow") == std::string::npos && g.find("valve.cam ") == std::string::npos, g);
        CHECK(g.find(kLabOff) == std::string::npos);
        shot("camdial.lab.generated.png");

        // Lab, a rotary (the reference's layout 16): the dial live, and a line naming the ports.
        set("engine.layout", 16.0);
        t = show("valve.timing");
        CHECK_MSG(t.find("valve.cam ") != std::string::npos && t.find("PORT TIMING") != std::string::npos, t);
        CHECK(t.find(kDead) == std::string::npos && t.find(kRotary) != std::string::npos);
        shot("camdial.rotary.timing.png");
        g = show("");
        CHECK_MSG(g.find(kDead) == std::string::npos && g.find("valve.cam ") != std::string::npos, g);

        // Reference, a piston engine: the dial live as the CAM DIAL; the lab's fields carry their word, the generated
        // page's too.
        set("engine.layout", 0.0);
        set("valve.model", 0.0);
        t = show("valve.timing");
        CHECK_MSG(t.find("valve.cam ") != std::string::npos && t.find("CAM DIAL") != std::string::npos, t);
        CHECK(t.find(kDead) == std::string::npos);
        g = show("");
        CHECK_MSG(g.find(kLabOff) != std::string::npos && g.find(kDead) == std::string::npos && g.find("valve.cam ") != std::string::npos, g);
        shot("camdial.reference.generated.png");
        // valve.head under Reference: none of its fields read (N1), and said so; under Lab, not.
        const std::string head = show("valve.head");
        CHECK_MSG(head.find("THE LAB IS OFF: THE REFERENCE MODEL SIZES ITS") != std::string::npos, head);
        shot("camdial.reference.head.png");
        // ENGINE under Reference: no lift figure (the lab's inLift/exLift are not the reference's; N1), and under Lab the
        // lab's lift again.
        const int engine = f.find("engine");
        CHECK(engine >= 0);
        if (engine >= 0)
        {
            f.show(engine);
            rig.host->tick(5);
            const auto [refIn, refEx] = tee::ui::engineFullLifts(f.current());
            CHECK_MSG(!std::isfinite(refIn) && !std::isfinite(refEx), std::to_string(refIn) + ", " + std::to_string(refEx));
            shot("camdial.reference.engine.png");
            set("valve.model", 1.0);
            rig.host->tick(5);
            const auto [labIn, labEx] = tee::ui::engineFullLifts(f.current());
            CHECK_MSG(std::isfinite(labIn) && std::isfinite(labEx), std::to_string(labIn) + ", " + std::to_string(labEx));
            set("valve.model", 0.0);
        }
        // The model switched with the page open (no new selection): the page follows at once (Workspace::inspectorKey),
        // the dial gone and no line.
        t = show("valve.timing");
        set("valve.model", 1.0);
        rig.host->tick(3);
        t = f.inspectorGui(0);
        CHECK_MSG(t.find("valve.cam ") == std::string::npos && t.find(kDead) == std::string::npos && t.find("color=yellow") == std::string::npos, t);
        CHECK(show("valve.head").find("THE REFERENCE MODEL SIZES") == std::string::npos);

        // A two-stroke (N1: nothing reads spec.valve there, its ports are the engine's two-stroke fields): VALVES shows
        // its ports alone, whatever is selected, so no valve field and no dial, under either model.
        set("engine.custom", 1.0);
        set("engine.cycle", 1.0);
        for (const double model : { 0.0, 1.0 })
        {
            set("valve.model", model);
            for (const char* sel : { "", "valve.timing", "valve.cam" })
            {
                const std::string x = show(sel);
                CHECK_MSG(x.find("A TWO-STROKE: PORTS IN PLACE OF VALVES") != std::string::npos && x.find("valve.") == std::string::npos,
                          std::string(sel) + ": " + x);
            }
        }
        return 0;
    }
}
