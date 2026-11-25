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

namespace
{
    // The instant a bake's tables go into use on a running engine (the lead's question, 01:33): first.json baked, then
    // both tables set aside again on a running SimHost (the build attached, as the application's is), settled at idle
    // and at a held 3000 rpm part load, and both Auto switches turned off: the commanded and actual intake phase, the
    // spark and knock's retard printed every block around that instant. Prints only (nightly).
    BUILD_TEST(bakeJump, "build.bake.jump.pictures")
    {
        std::vector<std::string> warnings;
        std::string error;
        CHECK_MSG(rig.build->loadFile(std::filesystem::path(TEE_UI_BUILDS_DIR) / "first.json", tee::BuildPart::Whole, warnings, error), error);
        {
            tee::ui::ServiceBake bake(rig.sim, *rig.build);
            const auto t0 = std::chrono::steady_clock::now();
            while (bake.state() != "baked" && std::chrono::steady_clock::now() - t0 < std::chrono::seconds(400))
            {
                bake.tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            std::printf("  baked: %s after %.0f s\n", bake.state().c_str(),
                        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
        }
        const int kla = rig.doc->find("ecu.maps.knockLimitAuto"), cia = rig.doc->find("ecu.maps.camInAuto");
        CHECK(kla >= 0 && cia >= 0);
        const auto autos = [&](double v) {
            rig.doc->beginGesture();
            rig.doc->set(kla, v);
            rig.doc->set(cia, v);
            rig.doc->endGesture();
        };
        autos(1.0);   // set aside, as after an edit
        tee::SimHost host;
        tee::LoadOptions lo;
        lo.worker = false;
        CHECK_MSG(host.attach(rig.sim, lo, error), error);
        if (!host.loaded())
            return 0;
        host.prepare(lo.sampleRate, 512);
        rig.build->attach(&host);
        const tee::Fields& fields = rig.sim->fields();
        const char* names[] = { "engine.speed", "ecu.advance", "ecu.knockRetard", "valve.inPhaseTarget", "valve.inPhase", "ecu.knockTable", "engine.manifold", "engine.throttle" };
        std::vector<const tee::FieldInfo*> tel;
        for (const char* n : names)
            tel.push_back(fields.find(std::string("tel.") + n));
        std::vector<float> l(512), r(512);
        tee::SimControls ctl;
        double t = 0.0;
        const auto run = [&](double seconds, double printFrom, double printEvery) {
            double next = printFrom;
            for (double end = t + seconds; t < end; t += 512.0 / lo.sampleRate)
            {
                host.setControls(ctl);
                host.process(l.data(), r.data(), 512);
                host.service();
                if (printEvery > 0.0 && t >= next)
                {
                    next = t + printEvery;
                    tee::TelemetryFrame f;
                    host.telemetry(f);
                    std::printf("    %7.3f s", t);
                    for (std::size_t i = 0; i < tel.size(); ++i)
                    {
                        const double v = tel[i] != nullptr ? f.get(*tel[i]) : std::nan("");
                        std::printf("  %s %.2f", names[i], i == 0 ? v * 30.0 / 3.14159265358979 : v);
                    }
                    std::printf("\n");
                }
            }
        };
        ctl.ignition = 2;
        run(1.5, 0.0, 0.0);
        ctl.ignition = 1;
        std::printf("  -- idle, tables set aside; at the flip both Auto switches go off (tables in use)\n");
        run(8.0, t + 7.6, 0.05);
        autos(0.0);
        std::printf("  -- flipped at %.3f s\n", t);
        run(1.0, t, 0.025);
        std::printf("  -- 3000 rpm held by the bench's driver (ctl.holdSpeed, the pedal his), tables set aside again\n");
        autos(1.0);
        ctl.holdSpeed = static_cast<float>(3000.0 * 3.14159265358979 / 30.0);
        run(8.0, t + 7.6, 0.05);
        autos(0.0);
        std::printf("  -- flipped at %.3f s\n", t);
        run(1.0, t, 0.025);
        rig.build->attach(nullptr);
        return 0;
    }
}
