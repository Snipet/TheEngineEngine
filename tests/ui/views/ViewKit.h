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

    // A workspace alone in a headless host over the demo (ui/app/Demo.h): its stage the whole window. `t` is the demo
    // script's time; at(t) moves it and ticks the host so the eases land.
    struct StageRig
    {
        tee::ui::DemoRig demo;
        std::unique_ptr<tee::ui::Workspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;

        StageRig(tee::ui::WorkspaceFactory make, int w = 1100, int h = 700, float dpi = 2.0f, double t = 4.6)
            : demo(t)
        {
            ws = make(demo.ctx);
            host = std::make_unique<funkgui::HeadlessHost>(*ws, 0, dpi);
            host->resize(w, h);
            ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
            settle();
        }
        void at(double t)
        {
            demo.at(t);
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
