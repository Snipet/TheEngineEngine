// tests/ui/views/DynoTests.cpp: the DYNO workspace (ui/views/dyno/) over the demo engine (ui/app/Demo.h): its stage at
// both densities, both themes and two sizes; a run started, watched, landed, renamed, hidden and copied as CSV; the
// cursor reading the traces; a quantity the runs did not record drawn as a dash, not invented.
#include "ViewKit.h"

#include "app/AppFrame.h"
#include "views/car/Car.h"
#include "views/dyno/Dyno.h"
#include "views/tune/Tune.h"
#include "views/tune/TuneDemo.h"

#include <funkgui/core/Theme.h>
#include <funkgui/text/FontService.h>

#include <algorithm>


namespace tee::test::u4
{
    // The stage U1's frame gives a workspace in a window of w x h (the frame over the demo).
    funkgui::Rect measure(int w, int h)
    {
        tee::ui::DemoRig demo(4.6);
        tee::ui::FrameServices s;
        s.app = demo.app;
        tee::ui::AppFrame frame(demo.doc, demo.units, demo.engine, std::move(s), &demo.engine, &demo.engine,
                                tee::ui::defaultSlots(), w, h);
        funkgui::HeadlessHost host(frame, 0, 1.0f);
        host.resize(w, h);
        host.tick(2);
        return frame.stageRect();
    }
    funkgui::Rect frameStage(int w, int h)
    {
        static funkgui::Rect small = measure(1100, 700), large = measure(1600, 1000);
        return w >= 1600 ? large : w == 1100 && h == 700 ? small : measure(w, h);
    }
    int stageW(int window) { const funkgui::Rect r = frameStage(window, window >= 1600 ? 1000 : 700); return static_cast<int>(r.w); }
    int stageH(int window) { const funkgui::Rect r = frameStage(window, window >= 1600 ? 1000 : 700); return static_cast<int>(r.h); }
}

namespace
{
    using namespace tee::test::views;

    // The same rig on another theme (index 1: paper).
    // An edit made and the predicted curve answered, so "before last edit" is there to overlay.
    void editBoost(StageRig& rig, double pa)
    {
        auto& doc = rig.demo.doc;
        const int f = doc.find("boost.level");
        doc.beginGesture();
        doc.set(f, pa);
        doc.endGesture();
        for (int i = 0; i < 90; ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            rig.host->tick(1);
        }
    }

    // The three workspaces of U4 by row (0 DYNO, 1 TUNE, 2 CAR), and each brought to the moment its pictures show.
    tee::ui::WorkspaceFactory factoryOf(int row)
    {
        return row == 0 ? tee::ui::makeDynoWorkspace : row == 1 ? tee::ui::makeTuneWorkspace : tee::ui::makeCarWorkspace;
    }
    void prepare(int row, StageRig& rig, int w, int h)
    {
        if (row == 0)
        {
            editBoost(rig, 110e3);
            tee::ui::DynoRequest r;                      // the stage's START button is tested in dyno.run
            r.name = "Engine 1";
            rig.demo.engine.startRun(r);
            for (int i = 0; i < 200 && rig.demo.engine.running(); ++i)
            {
                rig.demo.engine.tick(1.0f / 60.0f);
                rig.host->tick(1);
            }
            editBoost(rig, 85e3);
            rig.host->move(static_cast<float>(w) * 0.45f, static_cast<float>(h) * 0.4f);
        }
        else if (row == 1)
        {
            tee::ui::tunedemo::fillDocument(rig.demo.doc);
            for (int i = 0; i < 300; ++i)
            {
                rig.demo.at(i / 60.0);
                rig.host->tick(1);
            }
        }
        rig.settle();
    }

    // The stage at the frame's: in a 1100 x 700 window (dyno.stage) and a 1600 x 1000 one (dyno.large), density 2 and 1;
    // the predicted curve, the one before an edit, a run, the cursor; and the small one on paper.
    void stageGoldens(int row, const char* name)
    {
        for (const int window : { 1100, 1600 })
            for (float dpi : { 2.0f, 1.0f })
            {
                const int w = tee::test::u4::stageW(window), h = tee::test::u4::stageH(window);
                StageRig rig(factoryOf(row), w, h, dpi, row == 1 ? 0.0 : 4.6);
                prepare(row, rig, w, h);
                golden(std::string(name) + (window == 1100 ? ".stage" : ".large") + (dpi > 1.5f ? "" : ".dpi1"), *rig.host);
            }
        const int w = tee::test::u4::stageW(1100), h = tee::test::u4::stageH(1100);
        StageRig paper(factoryOf(row), w, h, 2.0f, row == 1 ? 0.0 : 4.6);
        prepare(row, paper, w, h);
        paper.host = std::make_unique<funkgui::HeadlessHost>(*paper.ws, 1, 2.0f);
        paper.host->resize(w, h);
        paper.ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
        if (row == 0)
            paper.host->move(static_cast<float>(w) * 0.45f, static_cast<float>(h) * 0.4f);
        paper.settle();
        golden(std::string(name) + ".paper", *paper.host);
    }

    VIEW_TEST(testDynoStage, "dyno.stage")
    {
        stageGoldens(0, "dyno");
        return 0;
    }
    // TUNE's and CAR's are made here too, where the three share prepare() and the frame's stage rects.
    VIEW_TEST(testTuneStage, "tune.stage")
    {
        stageGoldens(1, "tune");
        return 0;
    }
    VIEW_TEST(testCarStage, "car.stage")
    {
        stageGoldens(2, "car");
        return 0;
    }

    // A run: started from the plan, in progress (the words and the swept span), landed as a row chosen at once, renamed,
    // hidden by its box, copied as CSV. Stop ends one early.
    VIEW_TEST(testDynoRun, "dyno.run")
    {
        StageRig rig(tee::ui::makeDynoWorkspace, 1100, 700, 1.0f);
        CHECK(!rig.ws->moving());
        const std::uint64_t still = rig.ws->drawKey();
        rig.settle();
        CHECK(rig.ws->drawKey() == still);                   // at rest it asks for nothing
        // START RUN (the plan's button under the steppers).
        const funkgui::Rect startR = tee::ui::dynoControl(*rig.ws, "start"), stopR = tee::ui::dynoControl(*rig.ws, "stop");
        const float sx = startR.centreX(), sy = startR.centreY();
        rig.host->click(sx, sy);
        CHECK(rig.demo.engine.running());
        CHECK(rig.ws->moving());
        for (int i = 0; i < 30; ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            rig.host->tick(1);
        }
        picture("dyno.running", *rig.host);
        for (int i = 0; i < 200 && rig.demo.engine.running(); ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            rig.host->tick(1);
        }
        rig.settle();
        CHECK(rig.demo.engine.runCount() == 1);
        // The newest run is chosen; rename it through the NAME field.
        const funkgui::Rect nameR = tee::ui::dynoControl(*rig.ws, "name"), copyR = tee::ui::dynoControl(*rig.ws, "copy");
        const float nameX = nameR.centreX(), nameY = nameR.centreY();
        rig.host->click(nameX, nameY);
        typeKeys(*rig.host, "B a s e space p u l l enter");
        rig.settle();
        picture("dyno.renamed", *rig.host);
        // Copy CSV.
        rig.host->click(copyR.centreX(), copyR.centreY());
        const std::string csv = rig.host->clipboardText();
        CHECK_MSG(csv.rfind("speed_rad_s,rpm,torque_N_m,power_kW,boost.pressure\n", 0) == 0, csv.substr(0, 80));
        CHECK(std::count(csv.begin(), csv.end(), '\n') == 1 + static_cast<long>(rig.demo.engine.run(0).speed.size()));
        // Stop: a second run ended early lands nothing in the demo.
        rig.host->click(sx, sy);
        CHECK(rig.demo.engine.running());
        rig.host->click(stopR.centreX(), stopR.centreY());
        CHECK(!rig.demo.engine.running());
        return 0;
    }

    // A quantity the runs did not record: the band says so with a dash; nothing is invented. A dyno with no curve says
    // why instead of drawing axes over nothing.
    VIEW_TEST(testDynoAbsent, "dyno.absent")
    {
        StageRig rig(tee::ui::makeDynoWorkspace, 1100, 700, 1.0f);
        // LAMBDA, chosen by its button in the band's title row.
        const funkgui::Rect lambda = tee::ui::dynoControl(*rig.ws, "lambda");
        CHECK(!lambda.isEmpty());
        rig.host->click(lambda.centreX(), lambda.centreY());
        rig.settle();
        picture("dyno.absent", *rig.host);
        return 0;
    }

    // The contact sheet of package U4: DYNO, TUNE and CAR a row each; across, each at 1100 x 700 and 1600 x 1000, at
    // density 1 then 2 (out/ui/sheet.u4.png). Every tile is the software rasteriser's, shrunk to half its logical size.
    VIEW_TEST(testU4Sheet, "dyno.sheet.pictures")
    {
        struct Col
        {
            int w, h;
            float dpi;
        };
        // The frame's stages in a 1100 x 700 and a 1600 x 1000 window.
        const int sw = tee::test::u4::stageW(1100), sh = tee::test::u4::stageH(1100);
        const int lw = tee::test::u4::stageW(1600), lh = tee::test::u4::stageH(1600);
        const Col cols[] = { { sw, sh, 1.0f }, { lw, lh, 1.0f }, { sw, sh, 2.0f }, { lw, lh, 2.0f } };
        constexpr int kGap = 24;
        int sheetW = kGap;
        for (const Col& c : cols)
            sheetW += c.w / 2 + kGap;
        const int rowH = lh / 2 + kGap, sheetH = kGap + 3 * rowH;
        funkgui::Image out;
        out.w = sheetW;
        out.h = sheetH;
        out.rgba.assign(static_cast<std::size_t>(sheetW) * static_cast<std::size_t>(sheetH) * 4, 255);
        for (std::size_t i = 0; i < out.rgba.size(); i += 4)
            out.rgba[i] = out.rgba[i + 1] = out.rgba[i + 2] = 8;
        for (int row = 0; row < 3; ++row)
        {
            int ox = kGap;
            for (const Col& col : cols)
            {
                StageRig rig(factoryOf(row), col.w, col.h, col.dpi, row == 1 ? 0.0 : 4.6);
                prepare(row, rig, col.w, col.h);
                const funkgui::Image img = funkgui::rasterise(rig.host->draw(), funkgui::FontService::get().atlas(), 1);
                const int tw = col.w / 2, th = col.h / 2, oy = kGap + row * rowH;
                const double sc = static_cast<double>(img.w) / tw;
                for (int y = 0; y < th; ++y)
                    for (int x = 0; x < tw; ++x)
                    {
                        const int x0 = static_cast<int>(x * sc), x1 = std::max(x0 + 1, static_cast<int>((x + 1) * sc));
                        const int y0 = static_cast<int>(y * sc), y1 = std::max(y0 + 1, static_cast<int>((y + 1) * sc));
                        unsigned sum[3] = { 0, 0, 0 }, n = 0;
                        for (int yy = y0; yy < y1 && yy < img.h; ++yy)
                            for (int xx = x0; xx < x1 && xx < img.w; ++xx)
                            {
                                const std::size_t k = (static_cast<std::size_t>(yy) * static_cast<std::size_t>(img.w) + static_cast<std::size_t>(xx)) * 4;
                                for (int ch = 0; ch < 3; ++ch)
                                    sum[ch] += img.rgba[k + static_cast<std::size_t>(ch)];
                                ++n;
                            }
                        const std::size_t d = (static_cast<std::size_t>(oy + y) * static_cast<std::size_t>(sheetW) + static_cast<std::size_t>(ox + x)) * 4;
                        for (int ch = 0; ch < 3; ++ch)
                            out.rgba[d + static_cast<std::size_t>(ch)] = static_cast<uint8_t>(n > 0 ? sum[ch] / n : 0);
                    }
                ox += tw + kGap;
            }
        }
        const std::string path = (outDir() / "sheet.u4.png").string();
        CHECK_MSG(tee::test::writePng(out, path), path);
        std::printf("  contact sheet: %s (software rasteriser; rows DYNO, TUNE, CAR; columns the frame's stages in "
                    "1100 x 700 and 1600 x 1000 windows at density 1, then at 2)\n", path.c_str());
        return 0;
    }

    // At the frame's stage: about 660 x 570 in a 1100 x 700 window, about 1100 x 860 in a 1600 x 1000 one.
    VIEW_TEST(testU4FrameStages, "dyno.framestages.pictures")
    {
        const char* names[] = { "dyno", "tune", "car" };
        for (int row = 0; row < 3; ++row)
            for (const auto& [w, h] : { std::pair{ 660, 570 }, std::pair{ 1100, 860 } })
            {
                StageRig rig(factoryOf(row), w, h, 1.0f, row == 1 ? 0.0 : 4.6);
                prepare(row, rig, w, h);
                picture(std::string(names[row]) + ".frame" + std::to_string(w) + ".dpi1", *rig.host);
            }
        return 0;
    }

    // Text that collides: the frame's glyphs grouped into strings (a string is a run of glyph quads in drawing order on
    // one line, left to right), then every two glyphs of different strings that cover each other by more than a quarter
    // of the smaller, and every glyph that leaves the stage. Each is said once, with where.
    std::vector<std::string> textClashes(const funkgui::PrimList& list, float w, float h)
    {
        struct G
        {
            float x0, y0, x1, y1;
            int run;
        };
        std::vector<G> gs;
        int run = -1;
        const funkgui::Prim* prev = nullptr;
        for (const funkgui::Prim& p : list.prims)
        {
            const bool text = static_cast<int>(p.d2[2] + 0.5f) == static_cast<int>(funkgui::PrimKind::text);
            if (!text)
            {
                prev = nullptr;
                continue;
            }
            const float gh = p.y1 - p.y0;
            if (prev == nullptr || std::fabs(p.y0 - prev->y0) > 0.5f || p.x0 < prev->x0 || p.x0 - prev->x1 > gh * 1.6f)
                ++run;
            gs.push_back({ p.x0 + 1.0f, p.y0 + 1.0f, p.x1 - 1.0f, p.y1 - 1.0f, run });
            prev = &p;
        }
        std::vector<std::string> out;
        // Each string inside the smallest box (a panel, a card, a button, a cell: a rounded rect at least 30 x 16)
        // that holds the middle of its first glyph.
        struct Box
        {
            float x0, y0, x1, y1;
        };
        std::vector<Box> boxes;
        for (const funkgui::Prim& p : list.prims)
            if (static_cast<int>(p.d2[2] + 0.5f) == static_cast<int>(funkgui::PrimKind::rrect) && p.x1 - p.x0 >= 30.0f &&
                p.y1 - p.y0 >= 16.0f)
                boxes.push_back({ p.x0, p.y0, p.x1, p.y1 });
        {
            std::size_t i = 0;
            char m[200];
            while (i < gs.size())
            {
                std::size_t j = i;
                Box sb{ gs[i].x0, gs[i].y0, gs[i].x1, gs[i].y1 };
                while (j < gs.size() && gs[j].run == gs[i].run)
                {
                    sb = { std::min(sb.x0, gs[j].x0), std::min(sb.y0, gs[j].y0), std::max(sb.x1, gs[j].x1), std::max(sb.y1, gs[j].y1) };
                    ++j;
                }
                // Its box: the smallest holding its first glyph's middle, else (centred text that overflows on both
                // sides) the smallest holding the string's middle.
                const auto smallest = [&](float cx, float cy) {
                    const Box* in = nullptr;
                    for (const Box& b : boxes)
                        if (cx > b.x0 && cx < b.x1 && cy > b.y0 && cy < b.y1 &&
                            (in == nullptr || (b.x1 - b.x0) * (b.y1 - b.y0) < (in->x1 - in->x0) * (in->y1 - in->y0)))
                            in = &b;
                    return in;
                };
                const Box* in = smallest((gs[i].x0 + gs[i].x1) * 0.5f, (gs[i].y0 + gs[i].y1) * 0.5f);
                if (in == nullptr)
                    in = smallest((sb.x0 + sb.x1) * 0.5f, (sb.y0 + sb.y1) * 0.5f);
                if (in != nullptr && (sb.x0 < in->x0 - 1.0f || sb.x1 > in->x1 + 1.0f || sb.y0 < in->y0 - 1.0f || sb.y1 > in->y1 + 1.0f))
                {
                    std::snprintf(m, sizeof m, "text at %.0f, %.0f (to %.0f, %.0f) leaves its box %.0f, %.0f to %.0f, %.0f", sb.x0,
                                  sb.y0, sb.x1, sb.y1, in->x0, in->y0, in->x1, in->y1);
                    out.push_back(m);
                }
                i = j;
            }
        }
        if (std::getenv("U4_TEXT_DEBUG") != nullptr)
            std::printf("  %zu glyphs in %d strings\n", gs.size(), run + 1);
        std::vector<std::pair<int, int>> said;
        char b[160];
        for (std::size_t i = 0; i < gs.size(); ++i)
        {
            const G& a = gs[i];
            if (a.x0 < -0.5f || a.y0 < -0.5f || a.x1 > w + 0.5f || a.y1 > h + 0.5f)
            {
                std::snprintf(b, sizeof b, "text off the stage at %.0f, %.0f", a.x0, a.y0);
                if (std::find(said.begin(), said.end(), std::pair{ a.run, -1 }) == said.end())
                {
                    said.push_back({ a.run, -1 });
                    out.push_back(b);
                }
            }
            for (std::size_t j = i + 1; j < gs.size(); ++j)
            {
                const G& c = gs[j];
                if (c.run == a.run)
                    continue;
                const float ix = std::min(a.x1, c.x1) - std::max(a.x0, c.x0), iy = std::min(a.y1, c.y1) - std::max(a.y0, c.y0);
                if (ix <= 0.0f || iy <= 0.0f)
                    continue;
                const float small = std::min((a.x1 - a.x0) * (a.y1 - a.y0), (c.x1 - c.x0) * (c.y1 - c.y0));
                if (ix * iy > 0.25f * small && std::find(said.begin(), said.end(), std::pair{ a.run, c.run }) == said.end())
                {
                    said.push_back({ a.run, c.run });
                    std::snprintf(b, sizeof b, "text over text at %.0f, %.0f and %.0f, %.0f", a.x0, a.y0, c.x0, c.y0);
                    out.push_back(b);
                }
            }
        }
        return out;
    }

    // No text over text and none off the stage, for each workspace at the frame's stages and the windows' sizes, at
    // density 1 (the frame's small stage ~660 x 570, its large ~1100 x 860; the 1100 x 700 and 1600 x 1000 windows).
    VIEW_TEST(testU4Text, "dyno.text")
    {
        // The check itself: two strings, one over the other, and one off the stage.
        {
            funkgui::PrimList made;
            funkgui::Prim g{};
            g.d2[2] = static_cast<float>(funkgui::PrimKind::text);
            g.x0 = 10.0f, g.y0 = 10.0f, g.x1 = 18.0f, g.y1 = 22.0f;
            made.prims.push_back(g);
            funkgui::Prim r{};
            made.prims.push_back(r);
            g.x0 = 12.0f, g.x1 = 20.0f;
            made.prims.push_back(g);
            made.prims.push_back(r);
            g.x0 = 95.0f, g.x1 = 103.0f;
            made.prims.push_back(g);
            CHECK(textClashes(made, 100.0f, 100.0f).size() == 2);
            // A string that starts in a box and runs out of it.
            funkgui::PrimList box;
            funkgui::Prim b{};
            b.d2[2] = static_cast<float>(funkgui::PrimKind::rrect);
            b.x0 = 0.0f, b.y0 = 0.0f, b.x1 = 40.0f, b.y1 = 30.0f;
            box.prims.push_back(b);
            g.y0 = 5.0f, g.y1 = 17.0f;
            for (float x : { 30.0f, 38.0f, 46.0f })
            {
                g.x0 = x, g.x1 = x + 8.0f;
                box.prims.push_back(g);
            }
            CHECK(textClashes(box, 100.0f, 100.0f).size() == 1);
        }
        const char* names[] = { "dyno", "tune", "car" };
        for (int row = 0; row < 3; ++row)
            for (const int window : { 1100, 1600 })
                for (const float dpi : { 1.0f, 2.0f })
            {
                const int w = tee::test::u4::stageW(window), h = tee::test::u4::stageH(window);
                StageRig rig(factoryOf(row), w, h, dpi, row == 1 ? 0.0 : 4.6);
                prepare(row, rig, w, h);
                rig.host->move(-10.0f, -10.0f);          // no cursor card: it covers what is under it, by design
                rig.settle();
                for (const std::string& clash : textClashes(rig.host->draw(), static_cast<float>(w), static_cast<float>(h)))
                    CHECK_MSG(false, std::string(names[row]) + " at " + std::to_string(w) + " x " + std::to_string(h) + " d" +
                                         std::to_string(static_cast<int>(dpi)) + ": " + clash);
            }
        return 0;
    }

    VIEW_TEST(testU4StageRects, "dyno.stagerects")
    {
        for (const auto& [w, h] : { std::pair{ 1100, 700 }, std::pair{ 1600, 1000 } })
        {
            const funkgui::Rect r = tee::test::u4::measure(w, h);
            std::printf("  window %d x %d: stage %.0f x %.0f at %.0f, %.0f\n", w, h, r.w, r.h, r.x, r.y);
            CHECK(r.w > 300.0f && r.h > 300.0f);
        }
        return 0;
    }
}

namespace
{
    // Two runs of the demo's curve, one with its exhaust walls settled and one with cold walls (a step a few percent
    // low), the rest of the demo's dyno as it is: what card C's runner reports through DynoTrace::walls.
    class WallsDyno final : public tee::ui::DynoSource
    {
    public:
        explicit WallsDyno(tee::ui::DemoEngine& e) : e_(e)
        {
            settled_ = e.predicted();
            settled_.name = "Settled";
            settled_.walls = 1;
            cold_ = e.predicted();
            cold_.name = "Cold";
            cold_.walls = 0;
            for (double& t : cold_.torque)
                t *= 0.97;
            for (double& p : cold_.power)
                p *= 0.97;
        }
        void tick(float dt) override { e_.tick(dt); }
        const tee::ui::DynoTrace& predicted() const override { return e_.predicted(); }
        const tee::ui::DynoTrace& before() const override { return e_.before(); }
        bool busy() const override { return false; }
        double progress() const override { return 0.0; }
        std::uint64_t revision() const override { return e_.revision(); }
        int runCount() const override { return 2; }
        const tee::ui::DynoTrace& run(int i) const override { return i == 0 ? settled_ : cold_; }

    private:
        tee::ui::DemoEngine& e_;
        tee::ui::DynoTrace settled_, cold_;
    };

    // Each run says its walls in words on its row and in its CSV; runs of both kinds shown together say so over the plot.
    VIEW_TEST(testDynoWalls, "dyno.walls")
    {
        tee::ui::DemoRig demo(4.6);
        WallsDyno dyno(demo.engine);
        tee::ui::WorkspaceContext ctx{ demo.doc, demo.engine, demo.engine, dyno, demo.units, demo.selection, demo.app };
        auto ws = tee::ui::makeDynoWorkspace(ctx);
        for (const int window : { 1100, 1600 })
            for (float dpi : { 2.0f, 1.0f })
            {
                const int w = tee::test::u4::stageW(window), h = tee::test::u4::stageH(window);
                funkgui::HeadlessHost host(*ws, 0, dpi);
                host.resize(w, h);
                ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
                host.tick(90);
                for (const std::string& clash : textClashes(host.draw(), static_cast<float>(w), static_cast<float>(h)))
                    CHECK_MSG(false, std::to_string(w) + ": " + clash);
                picture(std::string("dyno.walls") + (window == 1100 ? "" : ".large") + (dpi > 1.5f ? "" : ".dpi1"), host);
                if (window == 1100 && dpi < 1.5f)
                {
                    // The newest run (cold walls) is chosen: its CSV says so on its first line.
                    const funkgui::Rect copy = tee::ui::dynoControl(*ws, "copy");
                    host.click(copy.centreX(), copy.centreY());
                    const std::string csv = host.clipboardText();
                    CHECK_MSG(csv.rfind("# exhaust cold walls\nspeed_rad_s,", 0) == 0, csv.substr(0, 60));
                }
            }
        return 0;
    }
}
