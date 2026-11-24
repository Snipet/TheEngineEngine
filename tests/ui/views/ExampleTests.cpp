// tests/ui/views/ExampleTests.cpp: the example workspace (ui/views/_example/), the look and the demo source: the
// pattern every workspace's tests follow (docs/handoffs/u1-interface.md).
#include "ViewKit.h"

#include "app/Look.h"
#include "kit/bind/Binding.h"

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
        CHECK(first.settled == -1 && std::string(first.wallsWord()).empty());   // a prediction says nothing of the walls
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
        // A run says its exhaust walls: a sweep not at all, a stepped run settled or cold as asked.
        const auto runOf = [&](std::vector<double> steps, bool settle) {
            tee::ui::DynoRequest r;
            r.steps = std::move(steps);
            r.settleExhaust = settle;
            a.engine.startRun(r);
            for (int i = 0; i < 200 && a.engine.running(); ++i)
                a.engine.tick(1.0f / 60.0f);
            return a.engine.run(a.engine.runCount() - 1);
        };
        CHECK(runOf({}, true).settled == -1);
        CHECK(runOf({ 300.0, 400.0 }, true).settled == 1 && std::string(a.engine.run(a.engine.runCount() - 1).wallsWord()) == "walls settled");
        CHECK(runOf({ 300.0, 400.0 }, false).settled == 0);
        return 0;
    }

    // Every registered workspace's demo hook defines names of its own: none twice (the base demo's or another hook's),
    // the conflicts named with both definers.
    VIEW_TEST(testDemoHooks, "demo.hooks")
    {
        tee::ui::DemoRig all;   // every hook
        CHECK_MSG(all.engine.conflicts().empty(), "\n" + all.engine.conflicts());
        const std::string fields = tee::ui::demoFieldConflicts();
        CHECK_MSG(fields.empty(), "\n" + fields);
        std::printf("  %zu hooks: %s\n", tee::ui::registeredDemos().size(), all.engine.conflicts().empty() && fields.empty() ? "no name twice" : "conflicts above");
        return 0;
    }

    // The base demo's names are a contract (frozen 2026-11-24): every package's goldens see them, so a new name comes
    // through its workspace's hook, never here. Changing this list is a change for every package to re-bless.
    VIEW_TEST(testDemoBase, "demo.base")
    {
        const std::vector<std::string> tel = {
            "engine.state", "engine.speed", "engine.redline", "engine.idle", "engine.throttle", "engine.manifold",
            "engine.coolant", "engine.oilPressure", "engine.flags", "engine.cycle", "engine.crank",
            "boost.pressure", "exhaust.portTemperature", "ecu.knockRetard",
            "car.speed", "car.gear", "car.brake", "car.clutch", "sound.peak",
            "ecu.advance", "sound.rate",
            "engine.torque", "engine.power", "engine.firing", "engine.imep", "engine.bmep", "engine.veAir",
        };
        const std::vector<std::string> scope = { "degrees", "cycles", "pressure", "volume", "liftExhaust", "liftIntake",
                                                 "pipePoints", "pipePressure", "portIntake", "portExhaust" };
        CHECK(tee::ui::DemoEngine::baseTelemetry() == tel);
        CHECK(tee::ui::DemoEngine::baseScope() == scope);
        CHECK_MSG(tee::ui::demoFields(tee::ui::DemoHooks::none()).size() == 57,
                  std::to_string(tee::ui::demoFields(tee::ui::DemoHooks::none()).size()));
        return 0;
    }

    // A length field that prefers metres (a listener's distance) reads in m while the person's length unit is mm or
    // cm; inches stay inches; a field with no preference (a bore) keeps the chosen unit. The inspector's parameter
    // (FieldParams) carries it: its unit, its range and its value.
    VIEW_TEST(testUnitsPrefer, "units.prefer")
    {
        using tee::ui::Quantity;
        tee::ui::Units u;
        CHECK(std::string(u.unit(Quantity::length).name) == "mm");
        CHECK(std::string(u.unit(Quantity::length, "m").name) == "m");
        CHECK(u.format(Quantity::length, 5.0, -1, "m") == "5.000 m");
        CHECK(u.format(Quantity::length, 0.1016) == "101.6 mm");
        u.choose(Quantity::length, "cm");
        CHECK(std::string(u.unit(Quantity::length, "m").name) == "m");
        u.choose(Quantity::length, "in");
        CHECK(std::string(u.unit(Quantity::length, "m").name) == "in");   // another system: as chosen
        CHECK(std::string(u.unit(Quantity::length, "furlong").name) == "in");   // a name it has not: as chosen
        u.choose(Quantity::length, "mm");
        double si = 0.0;
        CHECK(u.parse(Quantity::length, "3", si, "m") && std::fabs(si - 3.0) < 1e-12);
        tee::ui::FieldInfo d;
        d.path = "sound.distance";
        d.kind = tee::ui::FieldKind::number;
        d.min = 0.5;
        d.max = 100.0;
        d.def = 5.0;
        d.label = "Distance";
        d.quantity = Quantity::length;
        d.preferUnit = "m";
        tee::ui::MemoryDoc doc({ d });
        tee::ui::FieldParams params(doc, u, { 0 });
        CHECK(params.info(0).unit == "m");
        CHECK_NEAR(params.get(0), 5.0, 1e-12);
        CHECK_NEAR(params.info(0).max, 100.0, 1e-12);
        CHECK(tee::ui::fieldText(d, 5.0, u) == "5.000 m");
        return 0;
    }
}
