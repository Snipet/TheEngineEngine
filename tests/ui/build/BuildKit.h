// tests/ui/build/BuildKit.h: what tee-ui-build-tests' tests share (the real simulator under the frame, in the root's
// build only). A test file tests/ui/build/<Name>Tests.cpp (or tests/ui/BuildTests.cpp) defines tests with
//
//     BUILD_TEST(myIdent, "build.mything") { ... CHECK(...); return 0; }     // int myIdent(Rig& rig)
//
// and CMake reads the names (each a CTest test "ui.<name>"), so a package adds one without editing a CMake file. The
// Rig (sim/ compiled, a Build, a BuildDoc, the frame over them) is made once for the run.
#pragma once

#include "app/AppFrame.h"
#include "kit/bind/Binding.h"
#include "kit/bind/host/BuildDoc.h"

#include "tee/Build.h"
#include "tee/Sim.h"

#include <funkgui/panel/HeadlessHost.h>

#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tee::test::build
{
    int& fails();
    void failAt(const char* file, int line, const std::string& what);

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

    using Test = int (*)(Rig&);
    std::vector<std::pair<std::string, Test>>& tests();
    struct Add
    {
        Add(const char* name, Test t) { tests().emplace_back(name, t); }
    };
}

#define BUILD_CHECK_FAIL(what) ::tee::test::build::failAt(__FILE__, __LINE__, what)
#define CHECK(cond) do { if (!(cond)) BUILD_CHECK_FAIL(#cond); } while (false)
#define CHECK_MSG(cond, msg) do { if (!(cond)) BUILD_CHECK_FAIL(std::string(#cond) + ": " + (msg)); } while (false)
#define BUILD_TEST(ident, name)                                                                                        \
    static int ident(::tee::test::build::Rig& rig);                                                                    \
    static const ::tee::test::build::Add add_##ident(name, ident);                                                     \
    static int ident([[maybe_unused]] ::tee::test::build::Rig& rig)
