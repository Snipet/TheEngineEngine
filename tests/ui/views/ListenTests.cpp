// tests/ui/views/ListenTests.cpp: LISTEN (ui/views/listen/): the plan over its own demo hook (package L's fields and
// telemetry made up), the cameras and places one click each, the listener dragged (the Free camera, one undo step) and
// turned, the driver's window, the reference's listener, nothing published, at rest; at the frame's stage sizes with
// no text off its panel or over other text. Pictures by FunkGui's software rasteriser, at density 2 and 1.
#include "ViewKit.h"

#include "app/Look.h"
#include "views/listen/ListenWorkspace.h"

#include <funkgui/canvas/Prim.h>
#include <funkgui/core/Theme.h>

#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace tee::test::u5
{
    funkgui::Rect frameStage(int w, int h);   // ScopeTests.cpp
    std::string textFaults(const funkgui::PrimList& prims, const funkgui::Rect& stage, const std::vector<funkgui::Rect>& panels);
}

namespace
{
    using namespace tee::test::views;
    using tee::ui::ListenWorkspace;
    using tee::ui::WorkspaceContext;

    struct ListenRig
    {
        tee::ui::DemoRig demo;
        std::unique_ptr<ListenWorkspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;
        int w, h;

        explicit ListenRig(int width = 1100, int height = 700, float dpi = 2.0f, double t = 4.6, int theme = 0,
                           tee::ui::DemoHooks hooks = tee::ui::DemoHooks::of({ "listen" }))
            : demo(t, hooks), w(width), h(height)
        {
            ws = std::make_unique<ListenWorkspace>(demo.ctx);
            host = std::make_unique<funkgui::HeadlessHost>(*ws, theme, dpi);
            host->resize(w, h);
            ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
            host->tick(90);
        }
        double field(const char* path) const { return demo.doc.get(demo.doc.find(path)); }
        bool press(const std::string& title)
        {
            for (const funkgui::A11yItem& it : host->accessibility())
                if (it.title == title && it.visible && !it.bounds.isEmpty())
                {
                    host->click(it.bounds.centreX(), it.bounds.centreY());
                    host->tick(30);
                    return true;
                }
            return false;
        }
        std::string faults()
        {
            const funkgui::PrimList& prims = host->draw();
            std::string why = tee::test::u5::textFaults(prims, { 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) },
                                                        { ws->plan(), ws->column(), ws->bar() });
            if (!why.empty())
                return why;
            // No glyph on a wheel (the lead found "TANK / silent" over the right rear one).
            for (const funkgui::Prim& p : prims.prims)
                if (static_cast<int>(p.d2[2] + 0.5f) == static_cast<int>(funkgui::PrimKind::text))
                    for (const funkgui::Rect& r : ws->wheelRects())
                        if (p.x1 - 1.5f > r.x && p.x0 + 1.5f < r.right() && p.y1 - 1.5f > r.y && p.y0 + 1.5f < r.bottom())
                            return "a glyph at " + std::to_string(static_cast<int>(p.x0)) + "," + std::to_string(static_cast<int>(p.y0))
                                 + " lies on a wheel";
            return {};
        }
    };

    // The plan over the demo: the car, the ten sources at their places with their levels, the listener at the
    // tailpipe camera's place (DISTANCE in metres in U1's made-up Spec: 5 m behind the bumper).
    VIEW_TEST(testListenStage, "listen.stage")
    {
        ListenRig rig;
        const ListenWorkspace::Scene s = rig.ws->scene();
        CHECK(s.model == 1 && s.camera == 0 && s.place == 2);
        CHECK(s.src[0].published && std::isfinite(s.src[0].level) && s.src[2].y > 3.0);
        CHECK_NEAR(s.lx, 0.0, 1e-9);
        CHECK(s.ly < 0.0);
        const funkgui::Point l = rig.ws->listenerPx();
        CHECK(rig.ws->plan().contains(l));
        CHECK_MSG(rig.faults().empty(), rig.faults());
        golden("listen.stage", *rig.host);
        ListenRig one(1100, 700, 1.0f);
        golden("listen.stage.dpi1", *one.host);
        ListenRig paper(1100, 700, 2.0f, 4.6, 1);
        golden("listen.paper", *paper.host);
        return 0;
    }

    // At the frame's own stages (its smallest window, a 1600 x 1000 one): the plan and the column fit, no glyph off its
    // panel or over another, for every camera.
    VIEW_TEST(testListenFrameStage, "listen.framestage")
    {
        for (const auto& win : { std::pair<int, int>{ 1100, 700 }, std::pair<int, int>{ 1600, 1000 } })
        {
            const funkgui::Rect st = tee::test::u5::frameStage(win.first, win.second);
            const int w = static_cast<int>(st.w), h = static_cast<int>(st.h);
            const std::string tag = win.first == 1100 ? "small" : "large";
            for (const float dpi : { 2.0f, 1.0f })
            {
                ListenRig rig(w, h, dpi);
                CHECK_MSG(rig.faults().empty(), tag + ": " + rig.faults());
                golden("listen.frame." + tag + (dpi == 1.0f ? ".dpi1" : ""), *rig.host);
            }
            ListenRig rig(w, h, 1.0f);
            for (const char* cam : { "REAR", "SIDE", "ENGINE BAY", "DRIVER", "TRACKSIDE", "CHASE", "FREE", "TAILPIPE" })
            {
                CHECK_MSG(rig.press(cam), cam);
                CHECK_MSG(rig.faults().empty(), tag + " " + cam + ": " + rig.faults());
            }
        }
        return 0;
    }

    // One click a camera and a place; the driver's seat with its window on the picture; trackside with its road.
    VIEW_TEST(testListenCameras, "listen.cameras")
    {
        ListenRig rig;
        CHECK(rig.press("SIDE"));
        CHECK(rig.field("sound.camera") == 2.0 && rig.ws->scene().lx > 1.0);
        CHECK(rig.press("TUNNEL"));
        CHECK(rig.field("sound.place") == 3.0);
        CHECK(rig.press("DRIVER"));
        CHECK(rig.ws->scene().camera == 4);
        picture("listen.driver", *rig.host);
        // The window: dragged down its handle, it opens.
        for (const funkgui::A11yItem& it : rig.host->accessibility())
            (void) it;
        const ListenWorkspace::Scene s = rig.ws->scene();
        const funkgui::Point top = rig.ws->toPx(-s.width / 2, s.ly + 0.65), bottom = rig.ws->toPx(-s.width / 2, s.ly - 0.2);
        rig.host->drag(top.x, top.y, bottom.x, bottom.y);
        rig.host->tick(10);
        CHECK_MSG(rig.field("sound.windows") > 0.3, std::to_string(rig.field("sound.windows")));
        golden("listen.driver.open", *rig.host);
        CHECK(rig.press("TRACKSIDE"));
        CHECK(rig.press("FIELD"));
        golden("listen.trackside", *rig.host);
        CHECK(rig.press("GARAGE"));
        CHECK(rig.press("REAR"));
        golden("listen.garage", *rig.host);
        return 0;
    }

    // The listener dragged: the Free camera at the place it was dropped, in one undo step; its facing turned.
    VIEW_TEST(testListenDrag, "listen.drag")
    {
        ListenRig rig;
        const funkgui::Point l = rig.ws->listenerPx();
        double x = 0, y = 0;
        rig.ws->toCar({ l.x + 120.0f, l.y - 60.0f }, x, y);   // the plan holds its scale through a drag
        rig.host->drag(l.x, l.y, l.x + 120.0f, l.y - 60.0f);
        rig.host->tick(10);
        CHECK(rig.field("sound.camera") == 7.0);
        const ListenWorkspace::Scene s = rig.ws->scene();
        CHECK(s.camera == 7 && s.lx > 0.5 && s.ly > -5.0);
        CHECK_NEAR(s.lx, x, 0.06);
        CHECK_NEAR(s.ly, y, 0.06);
        // Its facing: the handle turned to the right turns it toward +x.
        const funkgui::Point h = rig.ws->facingHandlePx(), m = rig.ws->listenerPx();
        rig.host->drag(h.x, h.y, m.x + 40.0f, m.y);
        rig.host->tick(10);
        CHECK_NEAR(rig.field("sound.facing"), 90.0, 5.0);   // degrees, toward the right
        golden("listen.free", *rig.host);
        // Undo takes the turn back, and then the drag, a step each.
        rig.demo.doc.undo();
        CHECK(std::fabs(rig.field("sound.facing") - 90.0) > 5.0);
        rig.demo.doc.undo();
        CHECK(rig.field("sound.camera") == 0.0);
        return 0;
    }

    // The reference's listener: its five views as the choice, said in words.
    VIEW_TEST(testListenReference, "listen.reference")
    {
        ListenRig rig;
        const int f = rig.demo.doc.find("sound.model");
        rig.demo.doc.beginGesture();
        rig.demo.doc.set(f, 0.0);
        rig.demo.doc.endGesture();
        rig.ws->place({ 0.0f, 0.0f, 1100.0f, 700.0f });
        rig.host->tick(30);
        CHECK(rig.ws->scene().model == 0);
        CHECK(!rig.press("TRACKSIDE"));   // no cameras: the views
        CHECK(rig.press("SIDE"));
        CHECK(rig.field("sound.view") == 2.0);
        CHECK_MSG(rig.faults().empty(), rig.faults());
        golden("listen.reference", *rig.host);
        return 0;
    }

    // Nothing of L's: no fields of its, no telemetry. Dashes, the model's places, no crash; nothing to drag.
    VIEW_TEST(testListenAbsent, "listen.absent")
    {
        ListenRig rig(1100, 700, 2.0f, 4.6, 0, tee::ui::DemoHooks::none());
        const ListenWorkspace::Scene s = rig.ws->scene();
        CHECK(s.model == 0 && !s.src[0].published && std::isnan(s.src[0].level));
        const funkgui::Point l = rig.ws->listenerPx();
        rig.host->drag(l.x, l.y, l.x + 80.0f, l.y);
        CHECK(rig.demo.doc.find("sound.camera") < 0);
        CHECK_MSG(rig.faults().empty(), rig.faults());
        golden("listen.absent", *rig.host);
        return 0;
    }

    // At rest: nothing moves, the key stands; a telemetry value moved moves it.
    VIEW_TEST(testListenRest, "listen.rest")
    {
        ListenRig rig;
        CHECK(!rig.ws->moving());
        const std::uint64_t key = rig.ws->drawKey();
        rig.host->tick(30);
        CHECK(rig.ws->drawKey() == key);
        rig.demo.at(1.5);    // the script elsewhere: the levels move
        rig.host->tick(1);
        CHECK(rig.ws->drawKey() != key);
        bool found = false;
        for (const auto& e : tee::ui::registeredWorkspaces())
            found = found || std::string(e.id) == "listen";
        CHECK(found);
        return 0;
    }
}
