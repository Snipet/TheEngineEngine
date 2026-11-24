// tests/ui/views/ExampleTests.cpp: the example workspace (ui/views/_example/), the look and the demo source: the
// pattern every workspace's tests follow (docs/handoffs/u1-interface.md).
#include "ViewKit.h"

#include "app/Look.h"

#include <funkgui/core/Theme.h>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeExampleWorkspace(WorkspaceContext& c);
}

namespace
{
    using namespace tee::test::views;
    using tee::ui::look::Palette;

    // Pressing the drawn part selects its section; the gauge reads the demo's speed; a name the source lacks is a dash.
    VIEW_TEST(testExampleSelect, "example.select")
    {
        StageRig rig(tee::ui::makeExampleWorkspace, 800, 400, 2.0f, 4.6, false);
        CHECK(rig.demo.selection.empty());
        rig.host->click(400.0f, 110.0f);                 // the TURBO region
        CHECK(rig.demo.selection.path() == "boost" && rig.demo.selection.is("boost"));
        CHECK(!rig.demo.selection.is("boo") && !rig.demo.selection.is("boost.level"));
        rig.host->click(700.0f, 350.0f);                 // nothing there: the selection stays
        CHECK(rig.demo.selection.path() == "boost");
        golden("example.stage", *rig.host);
        rig.demo.engine.forget("boost.pressure");
        rig.settle();
        picture("example.absent", *rig.host);            // BOOST reads "-"
        return 0;
    }

    // Each colour of meaning differs from the others and from the ground in both themes, and is opaque; the heat tint
    // runs cold to hot.
    VIEW_TEST(testLookPalette, "look.palette")
    {
        for (int t = 0; t < funkgui::Theme::kCount; ++t)
        {
            const Palette p = Palette::of(funkgui::Theme::byIndex(t));
            CHECK(p.dark == (t == 0));
            const funkgui::Col all[] = { p.selected, p.cold, p.hot, p.hotter, p.ok, p.warning };
            for (const funkgui::Col& a : all)
            {
                CHECK(a.a == 255);
                const int dg = std::abs(a.r - p.ground.r) + std::abs(a.g - p.ground.g) + std::abs(a.b - p.ground.b);
                CHECK(dg > 150);
                for (const funkgui::Col& b : all)
                    if (&a != &b)
                        CHECK(std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b) > 30);
            }
            CHECK(p.heat(250.0).b > p.heat(250.0).r);    // cold is blue
            CHECK(p.heat(1100.0).r > p.heat(1100.0).b);  // hot is red
            CHECK(p.over(p.hot, 0.4f).a == 255);
            CHECK(tee::ui::look::num(std::nan("")) == "-" && tee::ui::look::num(12.4, "%.1f") == "12.4");
        }
        return 0;
    }

    // The demo source is repeatable, its Scope has a cycle, and its dyno answers an edit with a new curve and a ghost.
    VIEW_TEST(testDemoSource, "demo.source")
    {
        tee::ui::DemoRig a(3.2), b(3.2);
        const int h = a.engine.findValue("engine.speed");
        CHECK(h >= 0 && a.engine.value(h) == b.engine.value(h));
        CHECK(a.ctx.live.get("engine.speed") > 150.0);   // rad/s, pulling
        CHECK(std::isnan(a.ctx.live.get("car.wheelSlip")));
        a.engine.set("car.wheelSlip", 0.1);
        CHECK_NEAR(a.ctx.live.get("car.wheelSlip"), 0.1, 1e-12);
        std::vector<float> p(1080), v(1080);
        CHECK(a.engine.read(a.engine.find("pressure"), p.data(), 1080) == 720);
        CHECK(a.engine.read(a.engine.find("volume"), v.data(), 1080) == 720);
        CHECK(*std::max_element(p.begin(), p.begin() + 720) > 3e6f && v[180] > v[0] * 5.0f);
        CHECK(a.engine.find("nothing") == -1);
        const tee::ui::DynoTrace first = a.engine.predicted();
        CHECK(!first.empty() && a.engine.before().empty());
        const int f = a.doc.find("boost.level");
        a.doc.beginGesture();
        a.doc.set(f, 150e3);
        a.doc.endGesture();
        for (int i = 0; i < 30 && !a.engine.busy(); ++i)
            a.engine.tick(1.0f / 60.0f);
        CHECK(a.engine.busy());
        for (int i = 0; i < 60 && a.engine.busy(); ++i)
            a.engine.tick(1.0f / 60.0f);
        CHECK(!a.engine.busy() && !a.engine.before().empty());
        CHECK(a.engine.predicted().peakTorque().value > first.peakTorque().value);
        CHECK(a.engine.before().peakTorque().value == first.peakTorque().value);
        return 0;
    }
}
