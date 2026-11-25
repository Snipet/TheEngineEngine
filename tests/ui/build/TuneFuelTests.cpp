// tests/ui/build/TuneFuelTests.cpp: TUNE's fuel control (ui/views/tune/, U4) over the real Spec and simulator: the grade
// stepped in TUNE's band on first.json (Premium) reaches the build, its knock-limit table is then stale under the
// host's own key (knockLimitKey: the grade is one of its fields), and the band's words carry the
// octane numbers of the simulator's fuel table (sim/combustion/fuels.fm, served as the application serves it).
#include "build/BuildKit.h"

#include "tee/Bake.h"
#include "views/tune/Tune.h"

#include <fstream>
#include <iterator>

namespace
{
    std::string simText(const std::string& file)
    {
        std::ifstream in(std::filesystem::path(TEE_UI_SIM_DIR) / file, std::ios::binary);
        return in ? std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()) : std::string();
    }

    BUILD_TEST(tuneFuel, "build.tune.fuel")
    {
        std::vector<std::string> warnings;
        std::string error;
        CHECK_MSG(rig.build->loadFile(std::filesystem::path(TEE_UI_BUILDS_DIR) / "first.json", tee::BuildPart::Whole, warnings, error), error);
        CHECK(static_cast<int>(rig.build->get("fuel.grade")) == 2);
        CHECK(static_cast<int>(rig.build->get("fuel.model")) == 1);
        // (whether the shipped table is current at this tip is P's bake's business: not asked here)
        const std::int64_t key = tee::knockLimitKey(*rig.build, *rig.sim);
        tee::ui::FrameServices services = tee::ui::buildServices(*rig.build, TEE_UI_BUILDS_DIR, rig.user, {});
        services.simSource = simText;
        tee::ui::AppFrame frame(*rig.doc, rig.units, rig.telemetry, services, nullptr, nullptr, tee::ui::defaultSlots(), 1280, 800);
        funkgui::HeadlessHost host(frame, 0, 1.0f);
        host.tick(5);
        const int tune = frame.find("tune");
        CHECK(tune >= 0);
        if (tune < 0)
            return 0;
        frame.show(tune);
        host.tick(5);
        tee::ui::Workspace& ws = frame.workspace(tune);
        std::string t = tee::ui::tuneFuelText(ws);
        CHECK_MSG(t.rfind("Premium (91) | AKI 91   RON 96   MON 86 |", 0) == 0, t);
        // Next fuel: Race, through the band's button.
        const funkgui::Rect next = tee::ui::tuneFuelButton(ws, 1);
        CHECK(!next.isEmpty());
        host.click(next.centreX(), next.centreY());
        host.tick(5);
        CHECK(static_cast<int>(rig.build->get("fuel.grade")) == 3);
        CHECK(tee::knockLimitKey(*rig.build, *rig.sim) != key);     // re-keyed: the shipped table is stale
        CHECK(!tee::knockLimitCurrent(*rig.build, *rig.sim));
        CHECK(tee::ui::tuneShownMap(ws) == 7);                      // the knock-limit table shown, where it lands
        t = tee::ui::tuneFuelText(ws);
        CHECK_MSG(t.rfind("Race | AKI 100   RON 104   MON 96 | Premium (91) to Race: the knock-limit table re-bakes", 0) == 0, t);
        const std::string shot = std::string(TEE_UI_OUT_DIR) + "/build.tune.fuel.png";
        host.draw();
        if (!host.writePng(shot.c_str(), 1))
            std::printf("  (no picture %s)\n", shot.c_str());
        // Undone: Premium again, under the same key.
        CHECK(static_cast<bool>(services.undo));
        if (services.undo)
            services.undo();
        host.tick(5);
        CHECK(static_cast<int>(rig.build->get("fuel.grade")) == 2);
        CHECK(tee::knockLimitKey(*rig.build, *rig.sim) == key);
        return 0;
    }
}
