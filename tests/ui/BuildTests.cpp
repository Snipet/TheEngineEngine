// tests/ui/BuildTests.cpp: tee-ui-build-tests, J2's binding over card C's tee::Build with the real Spec (sim/), in the
// root's build only (it needs the host and FMinus). The frame is driven headless, as tee-ui-tests drives it.
//
//   ui.build.frame     every Spec field that is not hidden has its control or widget on a page of the frame, and every
//                      page reads with no notes
//   ui.build.set       a knob dragged on a page sets its field in the Build (one undo step); a curve and a map written
//                      through the binding come back the same (the map's z transposed both ways)
//   ui.build.files     a build saved by the garage and loaded back is the same build (Build::diff is empty)
#include "app/AppFrame.h"
#include "kit/bind/Binding.h"
#include "kit/bind/host/BuildDoc.h"

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
        for (int s = 0; s < f.screenCount(); ++s)
            for (int p = 0; p < f.pageCount(s); ++p)
            {
                ++pages;
                CHECK_MSG(f.pageOf(s, p)->notes().empty(), f.screenDef(s).name + " / " + f.pageTitle(s, p) + ": " + f.pageOf(s, p)->notes());
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
        std::printf("  %d Spec fields a person sets (and %d hidden) in %zu sections, on %d pages of %d screens\n", fields,
                    hidden, tee::ui::sectionsOf(*rig.doc).size(), pages, f.screenCount());
        return 0;
    }

    int setTest(Rig& rig)
    {
        tee::ui::AppFrame& f = *rig.frame;
        // The first knob of a number field on any page: dragged up, its field in the Build moves; undo puts it back.
        bool dragged = false;
        for (int s = 0; s < f.screenCount() && !dragged; ++s)
            for (int p = 0; p < f.pageCount(s) && !dragged; ++p)
            {
                tee::ui::FieldPage& page = *f.pageOf(s, p);
                for (int k = 0; k < page.params().count() && !dragged; ++k)
                {
                    const tee::ui::FieldInfo& info = rig.doc->info(page.params().field(k));
                    if (info.kind != tee::ui::FieldKind::number || info.def >= info.max)
                        continue;
                    f.show(s, p);
                    rig.host->tick(5);
                    const funkgui::Rect r = page.controlRect(info.path);
                    if (r.w <= 0.0f || r.bottom() > static_cast<float>(f.height()) - tee::ui::AppFrame::kBottomH)
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
        CHECK(dragged);
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
    } tests[] = { { "build.frame", frameTest }, { "build.set", setTest }, { "build.files", filesTest } };
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
