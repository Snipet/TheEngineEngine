// tests/ui/BuildTests.cpp: tee-ui-build-tests, J2's binding over card C's tee::Build with the real Spec (sim/), in the
// root's build only (it needs the host and FMinus). The frame is driven headless, as tee-ui-tests drives it.
//
//   ui.build.frame     every Spec field that is not hidden has its control or widget on a page of the frame, and every
//                      page reads with no notes
//   ui.build.set       a knob dragged on a page sets its field in the Build (one undo step); a curve and a map written
//                      through the binding come back the same (the map's z transposed both ways)
//   ui.build.files     a build saved by the garage and loaded back is the same build (Build::diff is empty)
#include "app/AppFrame.h"
#include "app/host/HostSources.h"

#include <chrono>
#include <thread>
#include "kit/bind/Binding.h"
#include "kit/bind/host/BuildDoc.h"
#include "views/valves/ValvesWorkspace.h"

#include "tee/Build.h"
#include "tee/Sim.h"

#include <funkgui/panel/HeadlessGuiScope.h>
#include <funkgui/panel/HeadlessHost.h>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

namespace
{
    int g_fails = 0;
    void failAt(int line, const std::string& what)
    {
        ++g_fails;
        std::printf("  FAIL BuildTests.cpp:%d: %s\n", line, what.c_str());
    }
#define CHECK(cond) do { if (!(cond)) failAt(__LINE__, #cond); } while (false)
#define CHECK_MSG(cond, msg) do { if (!(cond)) failAt(__LINE__, std::string(#cond) + ": " + (msg)); } while (false)

    struct NoTelemetry final : tee::TelemetrySource
    {
        int findValue(std::string_view) const override { return -1; }
        double value(int) const override { return 0.0; }
        float outputPeak() const override { return 0.0f; }
    };

    struct Rig
    {
        std::shared_ptr<tee::Sim> sim;
        std::unique_ptr<tee::Build> build;
        std::unique_ptr<tee::ui::BuildDoc> doc;
        tee::ui::Units units;
        NoTelemetry telemetry;
        std::filesystem::path user = std::filesystem::path(TEE_UI_OUT_DIR) / "builds";
        std::unique_ptr<tee::ui::AppFrame> frame;
        std::unique_ptr<funkgui::HeadlessHost> host;

        bool open()
        {
            std::string errors;
            sim = tee::Sim::compile(TEE_UI_SIM_DIR, tee::Sim::Options{}, errors);
            if (sim == nullptr)
            {
                std::printf("  sim/ did not compile:\n%s\n", errors.c_str());
                return false;
            }
            build = std::make_unique<tee::Build>(sim->fields());
            build->name = "Test build";
            doc = std::make_unique<tee::ui::BuildDoc>(*build);
            frame = std::make_unique<tee::ui::AppFrame>(*doc, units, telemetry,
                                                        tee::ui::buildServices(*build, TEE_UI_BUILDS_DIR, user, {}));
            host = std::make_unique<funkgui::HeadlessHost>(*frame, 0, 2.0f);
            host->tick(10);
            return true;
        }
    };

    int frameTest(Rig& rig)
    {
        tee::ui::AppFrame& f = *rig.frame;
        int pages = 0;
        for (int w = 0; w < f.workspaceCount(); ++w)
        {
            f.show(w);
            for (int p = 0; p < f.inspectorPageCount(); ++p)
            {
                ++pages;
                f.showInspectorPage(p);
                // U1's pages (the placeholders', DRIVE's) must read clean; a registered workspace's own page is its
                // package's to check, and is only said here.
                if (f.placeholder(w) || f.slot(w).id == "drive")
                    CHECK_MSG(f.inspector()->notes().empty(), f.slot(w).id + " / " + f.inspectorTitle(p) + ": " + f.inspector()->notes());
                else if (!f.inspector()->notes().empty())
                    std::printf("  note (%s's own page): %s", f.slot(w).id.c_str(), f.inspector()->notes().c_str());
            }
        }
        const std::vector<int> shown = f.fieldsShown();
        int fields = 0, hidden = 0;
        for (int i = 0; i < rig.doc->count(); ++i)
        {
            const tee::ui::FieldInfo& info = rig.doc->info(i);
            const bool there = std::find(shown.begin(), shown.end(), i) != shown.end();
            (info.hidden ? hidden : fields) += 1;
            CHECK_MSG(there != info.hidden, info.path + (info.hidden ? " is hidden and shown" : " is on no page"));
        }
        std::printf("  %d Spec fields a person sets (and %d hidden) in %zu sections, on %d inspector pages of %d workspaces\n",
                    fields, hidden, tee::ui::sectionsOf(*rig.doc).size(), pages, f.workspaceCount());
        return 0;
    }

    // U2: the VALVES cam editor's handles on the real Spec: the intake cam's peak dragged (its centreline and lift)
    // and its closing seat mark (its seat and 1 mm durations), each one step of undo that puts every field it moved
    // back exactly. Run as part of build.set (a drag sets its fields, one undo step).
    void valvesDragTest(Rig& rig)
    {
        tee::ui::AppFrame& f = *rig.frame;
        const int w = f.find("valves");
        CHECK(w >= 0);
        if (w < 0)
            return;
        f.show(w);
        rig.host->tick(5);
        const char* paths[] = { "valve.inCentre", "valve.inLift", "valve.inDurSeat", "valve.inDur1mm" };
        int idx[4];
        for (int i = 0; i < 4; ++i)
        {
            idx[i] = rig.doc->find(paths[i]);
            CHECK_MSG(idx[i] >= 0, paths[i]);
            if (idx[i] < 0)
                return;
        }
        const auto values = [&] {
            std::vector<double> v;
            for (int i : idx)
                v.push_back(rig.doc->get(i));
            return v;
        };
        for (const int h : { 0, 2 })   // the peak, the closing seat mark
        {
            const funkgui::Point at = tee::ui::valvesHandle(f.workspace(w), h);
            CHECK_MSG(at.x >= 0.0f, "no handle " + std::to_string(h));
            if (at.x < 0.0f)
                return;
            const std::vector<double> before = values();
            rig.host->drag(at.x, at.y, at.x + 24.0f, at.y - (h == 0 ? 20.0f : 0.0f), 8);
            const std::vector<double> after = values();
            if (h == 0)
                CHECK(after[0] > before[0] && after[1] > before[1] && after[2] == before[2] && after[3] == before[3]);
            else
                CHECK(after[2] > before[2] && after[3] > before[3] && after[0] == before[0] && after[1] == before[1]
                      && std::fabs((after[2] - after[3]) - (before[2] - before[3])) < 1e-6);
            CHECK(rig.build->undo());
            CHECK_MSG(values() == before, std::string("handle ") + std::to_string(h) + ": one undo puts every field back");
            std::printf("  VALVES handle %d: dragged and undone in one step\n", h);
        }
    }

    int setTest(Rig& rig)
    {
        tee::ui::AppFrame& f = *rig.frame;
        // The first knob of a number field on any inspector page, in sight: dragged up, its field in the Build moves;
        // undo puts it back.
        bool dragged = false;
        for (int w = 0; w < f.workspaceCount() && !dragged; ++w)
        {
            f.show(w);
            for (int p = 0; p < f.inspectorPageCount() && !dragged; ++p)
            {
                f.showInspectorPage(p);
                rig.host->tick(5);
                tee::ui::FieldPage& page = *f.inspector();
                for (int k = 0; k < page.params().count() && !dragged; ++k)
                {
                    const tee::ui::FieldInfo& info = rig.doc->info(page.params().field(k));
                    if (info.kind != tee::ui::FieldKind::number || info.def >= info.max)
                        continue;
                    const funkgui::Rect r = page.controlRect(info.path);
                    if (r.w <= 0.0f || r.y < f.inspectorRect().y || r.bottom() > f.inspectorRect().bottom())
                        continue;
                    const double before = rig.doc->get(page.params().field(k));
                    rig.host->drag(r.centreX(), r.centreY(), r.centreX(), r.centreY() - 50.0f, 10);
                    const double after = rig.doc->get(page.params().field(k));
                    CHECK_MSG(after > before, info.path);
                    CHECK(rig.build->undo());
                    CHECK_MSG(rig.doc->get(page.params().field(k)) == before, info.path);
                    std::printf("  dragged %s from %g to %g and back\n", info.path.c_str(), before, after);
                    dragged = true;
                }
            }
        }
        CHECK(dragged);
        valvesDragTest(rig);
        // Tables through the binding, both ways.
        for (int i = 0; i < rig.doc->count(); ++i)
        {
            const tee::ui::FieldInfo& info = rig.doc->info(i);
            if (info.kind == tee::ui::FieldKind::curve)
            {
                tee::ui::Curve16 c;
                c.n = 4;
                for (std::size_t k = 0; k < 4; ++k)
                {
                    c.x[k] = info.axisLo[0] + (info.axisHi[0] - info.axisLo[0]) * static_cast<double>(k) / 3.0;
                    c.y[k] = info.min + (info.max - info.min) * 0.1 * static_cast<double>(k + 1);
                }
                rig.doc->setCurve(i, c);
                const tee::ui::Curve16 back = rig.doc->curve(i);
                bool same = back.n == 4;
                for (std::size_t k = 0; k < 4; ++k)
                    same = same && std::fabs(back.x[k] - c.x[k]) < 1e-3 * (1.0 + std::fabs(c.x[k]))
                        && std::fabs(back.y[k] - c.y[k]) < 1e-3 * (1.0 + std::fabs(c.y[k]));
                CHECK_MSG(same, info.path);
            }
            if (info.kind == tee::ui::FieldKind::map)
            {
                tee::ui::Map16 m;
                m.nx = 3;
                m.ny = 2;
                m.x = { 1, 2, 3 };
                m.y = { 10, 20 };
                for (std::size_t j = 0; j < 2; ++j)
                    for (std::size_t k = 0; k < 3; ++k)
                        m.z[j][k] = info.min + (info.max - info.min) * (0.1 * static_cast<double>(k) + 0.3 * static_cast<double>(j));
                rig.doc->setMap(i, m);
                const tee::ui::Map16 back = rig.doc->map(i);
                bool same = back.nx == 3 && back.ny == 2;
                for (std::size_t j = 0; j < 2; ++j)
                    for (std::size_t k = 0; k < 3; ++k)
                        same = same && std::fabs(back.z[j][k] - m.z[j][k]) < 1e-3 * (1.0 + std::fabs(m.z[j][k]));
                CHECK_MSG(same, info.path);
                // And the Build's own layout: z[i][j] at x[i], y[j] (x of 3 points, y of 2: z[2][1] after x's and y's).
                const std::vector<double>& v = rig.build->values(rig.build->fieldOf("spec." + info.path));
                CHECK_MSG(v.size() > 32 + 2 * 16 + 1 && std::fabs(v[32 + 2 * 16 + 1] - m.z[1][2]) < 1e-3 * (1.0 + std::fabs(m.z[1][2])),
                          info.path);
            }
        }
        return 0;
    }

    int filesTest(Rig& rig)
    {
        // Change a few values, save as a name, change them again, load it back: the same build.
        for (int i = 0; i < rig.doc->count() && i < 40; ++i)
        {
            const tee::ui::FieldInfo& info = rig.doc->info(i);
            if (!info.table() && !info.hidden)
                rig.doc->set(i, info.min + (info.max - info.min) * 0.37);
        }
        const tee::Build saved = *rig.build;
        tee::ui::FrameServices s = tee::ui::buildServices(*rig.build, TEE_UI_BUILDS_DIR, rig.user, {});
        std::string error;
        CHECK_MSG(s.save("ui-round-trip", error), error);
        for (int i = 0; i < rig.doc->count() && i < 40; ++i)
            if (!rig.doc->info(i).table() && !rig.doc->info(i).hidden)
                rig.doc->set(i, rig.doc->info(i).def);
        const auto names = s.builds();
        CHECK(std::find(names.begin(), names.end(), "ui-round-trip") != names.end());
        CHECK_MSG(s.load("ui-round-trip", error), error);
        const auto diff = tee::Build::diff(saved, *rig.build);
        CHECK_MSG(diff.empty(), diff.empty() ? std::string() : diff.front().path);
        std::filesystem::remove(rig.user / "ui-round-trip.json");
        return 0;
    }
}

namespace
{
    // U1: the frame's dyno over card C's DynoService and the real simulator: a curve comes, an edit brings another with
    // the first kept as the ghost.
    int dynoTest(Rig& rig)
    {
        tee::DynoPlan plan;
        plan.from = 200.0;
        plan.to = 420.0;
        plan.rate = 220.0;
        plan.spacing = 20.0;
        plan.warmup = 0.6;
        tee::ui::ServiceDyno dyno(rig.sim, *rig.build, plan);
        const auto waitFor = [&](auto done) {
            for (int i = 0; i < 6000 && !done() && dyno.error().empty(); ++i)   // up to a minute
            {
                dyno.tick(0.01f);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            return done();
        };
        // The predicted curve (card C's PredictService): coarse first, then refined.
        bool coarse = false;
        waitFor([&] {
            coarse = coarse || (!dyno.predicted().empty() && !dyno.refined());
            return !dyno.predicted().empty() && dyno.refined();
        });
        std::printf("  a coarse curve came first: %s\n", coarse ? "yes" : "no (from the cache, or at once)");
        if (dyno.predicted().empty() && dyno.error().find("lacks") != std::string::npos)
        {
            // The program has no absorbed torque yet (tel.engine.torque, card D/K3): the ribbon says the runner's words.
            std::printf("  not run here: %s\n", dyno.error().c_str());
            return 0;
        }
        CHECK_MSG(!dyno.predicted().empty(), dyno.error());
        CHECK(dyno.before().empty());
        const tee::ui::DynoTrace first = dyno.predicted();
        std::printf("  predicted: %zu points, peak torque %.1f N m at %.0f rad/s\n", first.speed.size(),
                    first.peakTorque().value, first.peakTorque().speed);
        CHECK(first.speed.size() >= 5 && first.peakTorque().value > 0.0);
        // An edit of a number field: a new curve, the old one ghosted.
        for (int i = 0; i < rig.doc->count(); ++i)
        {
            const tee::ui::FieldInfo& info = rig.doc->info(i);
            if (info.kind != tee::ui::FieldKind::number || info.hidden || info.path.rfind("engine.", 0) != 0)
                continue;
            rig.doc->beginGesture();
            rig.doc->set(i, info.min + (info.max - info.min) * 0.7);
            rig.doc->endGesture();
            std::printf("  edited %s\n", info.path.c_str());
            break;
        }
        CHECK(waitFor([&] { return !dyno.before().empty() && dyno.refined(); }));
        CHECK(dyno.before().peakTorque().value == first.peakTorque().value);
        rig.build->undo();
        return 0;
    }
}

namespace
{
    // U1: the Scope over a real SimHost: a reader that reads only when generation() moves (as ENGINE does) gets data.
    int scopeTest(Rig& rig)
    {
        tee::SimHost host;
        tee::LoadOptions lo;
        lo.worker = false;
        std::string errors;
        CHECK_MSG(host.attach(rig.sim, lo, errors), errors);
        if (!host.loaded())
            return 0;
        host.prepare(lo.sampleRate, 512);
        tee::SimControls ctl;
        ctl.ignition = 2;                         // the key held: the starter turns it, then it runs
        host.setControls(ctl);
        tee::ui::SimScope scope(host);
        const int pressure = scope.find("pressure");
        CHECK(pressure >= 0);
        std::vector<float> left(512), right(512), buf(2048);
        std::uint64_t seen = 0;
        int reads = 0, longest = 0;
        for (int block = 0; block < 400; ++block)   // 4.3 s at 48 kHz
        {
            if (block == 120)
            {
                ctl.ignition = 1;
                host.setControls(ctl);
            }
            host.process(left.data(), right.data(), 512);
            host.service();
            if (block % 8 == 0 && scope.generation() != seen)   // a frame now and then, gated as ENGINE gates
            {
                seen = scope.generation();
                longest = std::max(longest, scope.read(pressure, buf.data(), static_cast<int>(buf.size())));
                ++reads;
            }
        }
        std::printf("  %d reads gated on generation(), the longest %d entries\n", reads, longest);
        CHECK(reads >= 2 && longest >= 360);
        return 0;
    }
}

int main(int argc, char** argv)
{
    const funkgui::HeadlessGuiScope gui;
    const std::string name = argc > 1 ? argv[1] : "all";
    Rig rig;
    if (!rig.open())
        return 1;
    const struct
    {
        const char* name;
        int (*run)(Rig&);
    } tests[] = { { "build.frame", frameTest }, { "build.set", setTest }, { "build.files", filesTest },
                  { "build.dyno", dynoTest }, { "build.scope", scopeTest } };
    for (const auto& t : tests)
        if (name == "all" || name == t.name)
        {
            const int before = g_fails;
            std::printf("ui.%s\n", t.name);
            t.run(rig);
            std::printf("  %s\n", g_fails == before ? "ok" : "FAILED");
        }
    return g_fails == 0 ? 0 : 1;
}
