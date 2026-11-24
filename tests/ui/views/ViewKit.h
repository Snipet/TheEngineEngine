// tests/ui/views/ViewKit.h: what the tests of the frame and of every workspace share (tee-view-tests). A test file
// tests/ui/views/<Name>Tests.cpp includes this and defines tests with
//
//     VIEW_TEST(myIdent, "drive.gauges") { ... CHECK(...); return 0; }
//
// The build finds the file and the names in it (tests/ui/CMakeLists.txt: each name is a CTest test "ui.<name>"), so a
// package adds tests without touching a CMake file. Goldens are frame fingerprints under tests/ui/views/golden/, written
// only by `tee-view-tests <name> --bless` after looking at the PNG under out/ui/. Every picture is FunkGui's software
// rasteriser's (no GPU here).
#pragma once

#include "Png.h"

#include "app/AppFrame.h"
#include "app/Demo.h"
#include "app/Workspace.h"

#include <funkgui/canvas/Fingerprint.h>
#include <funkgui/canvas/SoftRaster.h>
#include <funkgui/panel/HeadlessHost.h>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tee::test::views
{
    int& fails();
    bool blessing();
    void failAt(const char* file, int line, const std::string& what);

    using Test = std::function<int()>;
    struct Add
    {
        Add(const char* name, Test t);
    };

    // The frame's fingerprint against tests/ui/views/golden/<name>.txt (written instead under --bless), and its picture
    // to out/ui/<name>.png at the host's density.
    void golden(const std::string& name, funkgui::HeadlessHost& host);
    // Only the picture (no golden): out/ui/<name>.png.
    void picture(const std::string& name, funkgui::HeadlessHost& host);
    // Keys as words between spaces ("H o t space V 8 enter").
    void typeKeys(funkgui::HeadlessHost& host, std::string_view spec);
    // out/ui/ (made if needed).
    std::filesystem::path outDir();
    // What the .gui reader says of a workspace's inspectorPage() as it is now, over the demo's document ("" : it reads
    // clean; a workspace whose page is the frame's generated one has none). Each package checks its own page with it:
    //     CHECK_MSG(inspectorNotes(*rig.ws, rig.demo).empty(), inspectorNotes(*rig.ws, rig.demo));
    std::string inspectorNotes(const tee::ui::Workspace& ws, tee::ui::DemoRig& demo);
    // The host's accessibility items a person acts on (a button, a slider, a switch, a menu, a list's row, a text
    // field) that have no title: "" when every one is named, else a line each (its role and bounds). A package checks
    // its stage: CHECK_MSG(unnamedControls(*rig.host).empty(), unnamedControls(*rig.host));
    std::string unnamedControls(funkgui::HeadlessHost& host);

    // A workspace alone in a headless host over the demo (ui/app/Demo.h). `t` is the demo script's time; at(t) moves
    // it and ticks the host so the eases land. `hooks`: with every registered workspace's own demo data (DemoHook).
    //
    // The stage the frame gives (AppFrame::stageFor) is much smaller than the window. Lay out for the smallest and
    // look generous at the large:
    //     FrameSize::smallest   window 1100 x 700,  stage {64, 48, 696, 574}
    //     FrameSize::large      window 1600 x 1000, stage {112, 48, 1128, 874}
    // (the ribbon and its folding are in the inspector's column: the stage is the same either way). The plain
    // constructor gives the workspace the whole w x h window as its stage.
    enum class FrameSize { smallest, large };

    struct StageRig
    {
        tee::ui::DemoRig demo;
        std::unique_ptr<tee::ui::Workspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;

        StageRig(tee::ui::WorkspaceFactory make, int w = 1100, int h = 700, float dpi = 2.0f, double t = 4.6, bool hooks = true)
            : demo(t, hooks)
        {
            open(make, w, h, { 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) }, dpi);
        }
        StageRig(tee::ui::WorkspaceFactory make, FrameSize size, float dpi = 2.0f, double t = 4.6, bool hooks = true)
            : demo(t, hooks)
        {
            const int w = size == FrameSize::smallest ? 1100 : 1600, h = size == FrameSize::smallest ? 700 : 1000;
            open(make, w, h, tee::ui::AppFrame::stageFor(w, h), dpi);
        }
        void at(double t)
        {
            demo.at(t);
            settle();
        }
        void open(tee::ui::WorkspaceFactory make, int w, int h, const funkgui::Rect& stage, float dpi)
        {
            ws = make(demo.ctx);
            host = std::make_unique<funkgui::HeadlessHost>(*ws, 0, dpi);
            host->resize(w, h);
            ws->place(stage);
            settle();
        }
        // Ticks until the eases land (the demo's script does not move: ticking the host does not tick the demo).
        void settle() { host->tick(90); }
    };
}

#define CHECK(cond) do { if (!(cond)) ::tee::test::views::failAt(__FILE__, __LINE__, #cond); } while (false)
#define CHECK_MSG(cond, msg) do { if (!(cond)) ::tee::test::views::failAt(__FILE__, __LINE__, std::string(#cond) + ": " + (msg)); } while (false)
#define CHECK_NEAR(a, b, tol)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        const double va_ = static_cast<double>(a), vb_ = static_cast<double>(b);                                      \
        if (!(std::fabs(va_ - vb_) <= (tol)))                                                                          \
            ::tee::test::views::failAt(__FILE__, __LINE__, std::string(#a) + " = " + std::to_string(va_) + ", wanted "  \
                                                               + std::to_string(vb_));                                  \
    } while (false)
#define VIEW_TEST(ident, name)                                                                                         \
    static int ident();                                                                                                \
    static const ::tee::test::views::Add add_##ident(name, ident);                                                     \
    static int ident()
