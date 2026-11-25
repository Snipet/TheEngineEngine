// tests/ui/build/BakeWatch.cpp: the calibration bake (card C's CalibrationBakeService through ServiceBake) as a person
// sees it, in real time on first.json under the frame with TUNE showing: a timeline of TUNE's words, the set-aside
// switches and the ribbon's peak (printed), a picture at each change of state (out/ui/bake.watch.<n>.png); then one
// edit, its first seconds. A ".pictures" case: nightly only, about two minutes on four cores (the lead's ask, 01:12).
#include "build/BuildKit.h"

#include "app/host/HostSources.h"

#include <chrono>
#include <cstdio>
#include <thread>

namespace
{
    BUILD_TEST(bakeWatch, "build.bake.pictures")
    {
        std::vector<std::string> warnings;
        std::string error;
        CHECK_MSG(rig.build->loadFile(std::filesystem::path(TEE_UI_BUILDS_DIR) / "first.json", tee::BuildPart::Whole, warnings, error), error);
        tee::ui::ServiceDyno dyno(rig.sim, *rig.build);
        tee::ui::ServiceBake bake(rig.sim, *rig.build);
        tee::ui::FrameServices services = tee::ui::buildServices(*rig.build, TEE_UI_BUILDS_DIR, rig.user, {});
        services.sparkMap = [&bake] { return bake.state(); };
        tee::ui::AppFrame frame(*rig.doc, rig.units, rig.telemetry, services, &dyno, nullptr, tee::ui::defaultSlots(), 1280, 800);
        funkgui::HeadlessHost host(frame, 0, 1.0f);
        host.tick(5);
        frame.show(frame.find("tune"));
        host.tick(5);
        const int kla = rig.doc->find("ecu.maps.knockLimitAuto"), cia = rig.doc->find("ecu.maps.camInAuto");
        const auto t0 = std::chrono::steady_clock::now();
        auto now = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); };
        std::string last;
        int shots = 0;
        double lastPeak = -1.0;
        const auto watch = [&](double until, const char* phase, bool stopAtBaked) {
            const double end = now() + until;
            while (now() < end)
            {
                bake.tick();
                host.tick(1);
                const std::string s = bake.state();
                const double pk = dyno.predicted().peakTorque().value;
                if (s != last || (std::isfinite(pk) && std::fabs(pk - lastPeak) > 0.05))
                {
                    std::printf("  %7.2f s  [%s]  state '%s'  knockLimitAuto %.0f  camInAuto %.0f  peak %.1f N m (refined %d)\n", now(),
                                phase, s.c_str(), kla >= 0 ? rig.doc->get(kla) : -1.0, cia >= 0 ? rig.doc->get(cia) : -1.0, pk,
                                dyno.refined() ? 1 : 0);
                    if (s != last && shots < 12)
                    {
                        const std::string path = std::string(TEE_UI_OUT_DIR) + "/bake.watch." + std::to_string(shots++) + ".png";
                        host.draw();
                        if (!host.writePng(path.c_str(), 1))
                            std::printf("  (no picture %s)\n", path.c_str());
                    }
                    last = s;
                    lastPeak = pk;
                }
                if (stopAtBaked && s == "baked")
                    return;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        };
        std::printf("  -- load\n");
        watch(200.0, "load", true);
        std::printf("  -- edit compression +0.5\n");
        const int comp = rig.doc->find("engine.compression");
        rig.doc->beginGesture();
        rig.doc->set(comp, rig.doc->get(comp) + 0.5);
        rig.doc->endGesture();
        watch(12.0, "edit1", false);
        CHECK_MSG(bake.error().empty(), bake.error());
        return 0;
    }
}
