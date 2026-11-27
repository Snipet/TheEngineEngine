// tests/ui/build/BakeWatch.cpp: the calibration bake (card C's CalibrationBakeService through ServiceBake) as a person
// sees it, in real time on first.json under the frame with TUNE showing: a timeline of TUNE's words, the set-aside
// switches and the ribbon's peak (printed), a picture at each change of state (out/ui/bake.watch.<n>.png); then one
// edit, its first seconds. A ".pictures" case: nightly only, about two and a half minutes on four cores (the lead's ask,
// 01:12). Its waits are on the bake's state, not a clock (the lead's ask, 27 Nov: hosted runners differ by up to about
// 1.9x; the 26 Nov soak's slower one failed a 300 s and a 10 s clock): a wait ends when the state is reached, or fails
// when the state stops moving for kStall; kCap only keeps a hung run from holding the nightly.
#include "build/BuildKit.h"

#include "app/host/HostSources.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <thread>

namespace
{
    BUILD_TEST(bakeWatch, "build.bake.pictures")
    {
        std::vector<std::string> warnings;
        std::string error;
        // first.json, its spark and cam tables then put back to the program's defaults (as an unbaked build: content
        // ships baked since P's 066165a0, and this case watches a bake), as card C's unbaked() in tests/host.
        const auto loadFirst = [&] {
            CHECK_MSG(rig.build->loadFile(std::filesystem::path(TEE_UI_BUILDS_DIR) / "first.json", tee::BuildPart::Whole, warnings, error), error);
            const tee::Build fresh(rig.build->fields());
            const auto& all = rig.build->fields().all();
            for (std::size_t i = 0; i < all.size(); ++i)
                if (all[i].block == "spec" && (all[i].path.find("ecu.maps.knockLimit") != std::string::npos
                                               || all[i].path.find("ecu.maps.camIn") != std::string::npos))
                    CHECK(rig.build->setValues(static_cast<int>(i), fresh.values(static_cast<int>(i))));
        };
        loadFirst();
        // A cache folder of the test's own, empty at the start: the second load below finds its bake there.
        const std::filesystem::path cache = std::filesystem::path(TEE_UI_OUT_DIR) / "bake-cache";
        std::filesystem::remove_all(cache);
        std::filesystem::create_directories(cache);
        tee::CalibrationBakeOptions options;
        options.cacheDirectory = cache;
        tee::ui::ServiceDyno dyno(rig.sim, *rig.build);
        auto bake = std::make_unique<tee::ui::ServiceBake>(rig.sim, *rig.build, options);
        bake->yieldTo(dyno);   // the ribbon's curve before the bake's points
        tee::ui::FrameServices services = tee::ui::buildServices(*rig.build, TEE_UI_BUILDS_DIR, rig.user, {});
        services.sparkMap = [&bake] { return bake ? bake->state() : std::string(); };
        services.bakeStatus = [&bake] { return bake ? bake->status() : tee::ui::BakeStatus{}; };
        tee::ui::AppFrame frame(*rig.doc, rig.units, rig.telemetry, services, &dyno, nullptr, tee::ui::defaultSlots(), 1280, 800);
        funkgui::HeadlessHost host(frame, 0, 1.0f);
        host.tick(5);
        frame.show(frame.find("tune"));
        host.tick(5);
        const int kla = rig.doc->find("ecu.maps.knockLimitAuto"), cia = rig.doc->find("ecu.maps.camInAuto");
        auto t0 = std::chrono::steady_clock::now();
        auto now = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); };
        std::string lastStage, firstCurveState;
        int shots = 0;
        double lastPeak = -1.0, lastPrint = -100.0, firstCurve = -1.0;
        bool bakedAgain = false;   // a stage of baking seen (the second launch must take its tables from the cache)
        double longestStill = 0.0;   // s: the longest the state stood unmoved in a watch to the end (printed: kStall's room)
        constexpr double kStall = 120.0, kCap = 1500.0;   // s: the state unmoved this long is a stall; the run's bound
        std::function<void()> watchEach;   // called each tick of a watch, when set
        const auto stageOf = [](const std::string& st) {   // "baking cam 23 of 42" -> "baking cam"
            const std::size_t d = st.find_first_of("0123456789");
            return d == std::string::npos ? st : st.substr(0, d - 1);
        };
        // Watches for `until` seconds; with `stopAtBaked`, until the bake is done instead, for as long as its state moves
        // (each point changes its words). False when it stopped moving for kStall (or ran past kCap).
        const auto watch = [&](double until, const char* phase, bool stopAtBaked) {
            const double end = now() + until;
            std::string moved = bake->state();
            double movedAt = now();
            while (stopAtBaked ? now() < kCap : now() < end)
            {
                bake->tick();
                host.tick(1);
                if (watchEach)
                    watchEach();
                const std::string s = bake->state();
                if (s != moved)
                {
                    longestStill = std::max(longestStill, now() - movedAt);
                    moved = s;
                    movedAt = now();
                }
                if (s.rfind("baking ", 0) == 0)
                    bakedAgain = true;
                if (stopAtBaked && now() - movedAt > kStall)
                {
                    std::printf("  %7.2f s  [%s]  state '%s' unmoved for %.0f s: a stall\n", now(), phase, s.c_str(), kStall);
                    return false;
                }
                const double pk = dyno.predicted().peakTorque().value;
                if (firstCurve < 0.0 && !dyno.predicted().empty())
                {
                    firstCurve = now();
                    firstCurveState = s;
                    std::printf("  %7.2f s  [%s]  the ribbon's first curve (state '%s')\n", firstCurve, phase, s.c_str());
                }
                const bool newStage = stageOf(s) != lastStage;
                if (newStage || now() - lastPrint >= 10.0 || (std::isfinite(pk) && std::fabs(pk - lastPeak) > 0.05))
                {
                    std::printf("  %7.2f s  [%s]  state '%s'  knockLimitAuto %.0f  camInAuto %.0f  peak %.1f N m (refined %d)\n", now(),
                                phase, s.c_str(), kla >= 0 ? rig.doc->get(kla) : -1.0, cia >= 0 ? rig.doc->get(cia) : -1.0, pk,
                                dyno.refined() ? 1 : 0);
                    if (newStage && shots < 12)
                    {
                        const std::string path = std::string(TEE_UI_OUT_DIR) + "/bake.watch." + std::to_string(shots++) + ".png";
                        host.draw();
                        if (!host.writePng(path.c_str(), 1))
                            std::printf("  (no picture %s)\n", path.c_str());
                    }
                    lastStage = stageOf(s);
                    lastPeak = pk;
                    lastPrint = now();
                }
                if (stopAtBaked && (s == "baked" || s == "calibration checked"))
                    return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            return !stopAtBaked;
        };
        // "baking spark map 3 of 80" -> 3; no count yet ("waiting to bake", "baking"): 0.
        const auto pointsDone = [](const std::string& st) {
            const std::size_t of = st.find(" of ");
            if (of == std::string::npos)
                return 0;
            const std::size_t d = st.rfind(' ', of - 1);
            return std::atoi(st.c_str() + (d == std::string::npos ? 0 : d + 1));
        };
        std::printf("  -- load (the bake yields to the ribbon's predictor; cache %s, empty)\n", cache.string().c_str());
        CHECK_MSG(watch(0.0, "load", true), "the load's bake stalled at '" + bake->state() + "'");
        std::printf("  the state stood still at most %.1f s (a stall: %.0f s)\n", longestStill, kStall);
        CHECK(bake->state() == "baked" || bake->state() == "calibration checked");
        // A promptness check, not a guard of the bake's yield: the ribbon's first curve comes a few points into the bake's
        // first stage, whatever the machine's speed (both slow together). The yield is no longer what makes it prompt
        // (U1, 27 Nov, the lead's proof asked for): pinned to two cores and to one, the first curve came at point 3 with the
        // yield and point 4 without it (2 cores 10.0 s / 13.4 s; 1 core 21.1 s / 24.4 s); on four, 4.9 to 6.8 s at point 2
        // or 3. Before card C's 7f2ddfa1 it came some thirty points in (31 to 35 s); a check of 10 points would catch that.
        CHECK_MSG(firstCurve >= 0.0 && pointsDone(firstCurveState) <= 10,
                  "first curve at " + std::to_string(firstCurve) + " s, the bake at '" + firstCurveState + "'");
        std::printf("  still knocking after the bake: %d cells\n", bake->stillKnockingCells());
        // One edit (card C's bake across edits, the owner's options 1 and 2; restated by U1, 27 Nov, on the bake's state):
        // the old tables stay in use, marked stale, while the bake rests for the edits; it starts by itself once they
        // rest; the old tables play all through it; the new ones land and nothing is stale. Was "re-bakes within 12 s".
        std::printf("  -- edit compression +0.5\n");
        const int comp = rig.doc->find("engine.compression"), kl = rig.doc->find("ecu.maps.knockLimit");
        const tee::ui::Map16 before = kl >= 0 ? rig.doc->map(kl) : tee::ui::Map16{};
        rig.doc->beginGesture();
        rig.doc->set(comp, rig.doc->get(comp) + 0.5);
        rig.doc->endGesture();
        bake->tick();
        host.tick(1);
        tee::ui::BakeStatus st = bake->status();
        std::printf("  at once: '%s' | '%s'\n", tee::ui::bakeLine(st).c_str(), tee::ui::bakeStaleLine(st).c_str());
        CHECK_MSG(st.phase == tee::ui::BakeStatus::Phase::resting && st.staleSpark && st.staleCam && !st.setAside, bake->state());
        CHECK(rig.doc->get(kla) < 0.5 && rig.doc->get(cia) < 0.5);   // in use, not set aside
        // To the new tables, for as long as the state moves; each tick while it works, the old ones in use and said stale.
        const double editAt = now();
        double bakingAt = -1.0;
        int notStale = 0;
        watchEach = [&] {
            const tee::ui::BakeStatus w = bake->status();
            if (bakingAt < 0.0 && w.phase == tee::ui::BakeStatus::Phase::baking)
                bakingAt = now();
            if (w.working() && (!w.staleSpark || !w.staleCam || rig.doc->get(kla) > 0.5 || rig.doc->get(cia) > 0.5))
                ++notStale;
        };
        CHECK_MSG(watch(0.0, "edit1", true), "the edit's bake stalled at '" + bake->state() + "'");
        watchEach = nullptr;
        st = bake->status();
        std::printf("  rested %.1f s, then baked in %.1f s; ticks working without the old tables in use: %d\n",
                    bakingAt >= 0.0 ? bakingAt - editAt : -1.0, bakingAt >= 0.0 ? now() - bakingAt : -1.0, notStale);
        CHECK_MSG(bakingAt >= 0.0, "no stage of baking after the edit");
        CHECK(notStale == 0);
        CHECK_MSG((st.phase == tee::ui::BakeStatus::Phase::baked || st.phase == tee::ui::BakeStatus::Phase::checked) && !st.stale(),
                  bake->state());
        CHECK(rig.doc->get(kla) < 0.5 && rig.doc->get(cia) < 0.5);
        CHECK_MSG(kl < 0 || !(rig.doc->map(kl).z == before.z), "the knock-limit table did not move with the compression");
        CHECK_MSG(bake->error().empty(), bake->error());
        // A second launch: a fresh service over the same cache, first.json loaded again: "baked" at once.
        bake.reset();
        loadFirst();
        bake = std::make_unique<tee::ui::ServiceBake>(rig.sim, *rig.build, options);
        bake->yieldTo(dyno);
        std::printf("  -- a second launch (first.json again, the cache from the first)\n");
        t0 = std::chrono::steady_clock::now();
        lastStage.clear();
        bakedAgain = false;
        // Done from the cache: no stage of baking on the way (checking it on this program may run points; 1.5 s here).
        CHECK_MSG(watch(0.0, "second", true), "the second launch stalled at '" + bake->state() + "'");
        CHECK_MSG((bake->state() == "baked" || bake->state() == "calibration checked") && !bakedAgain,
                  bake->state() + (bakedAgain ? ", baked again" : "") + " after " + std::to_string(now()) + " s");
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
            while (bake.state() != "baked" && bake.state() != "calibration checked" && std::chrono::steady_clock::now() - t0 < std::chrono::seconds(400))
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
