// tests/ui/views/FxTests.cpp: the atmosphere helpers (ui/views/fx/Fx.h) and their shaders (shaders/fx/*.sc). On the
// software rasteriser every helper draws nothing (no shader draw recorded, so no placeholder either); the shaders'
// texts read only what FunkGui sets (u_params through in0..in15, u_rect through u_size, u_viewSize), include nothing
// and draw no text. That they compile for Metal, SPIR-V and Direct3D is proved by the build (docs/ui/fx.md).
#include "ViewKit.h"

#include "app/Look.h"
#include "views/fx/Fx.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <string>

namespace
{
    using namespace tee::test::views;
    namespace fx = tee::ui::fx;

    // A stage that asks for every effect over its whole area, and remembers what the helpers answered.
    class FxStage final : public tee::ui::Workspace
    {
    public:
        explicit FxStage(tee::ui::WorkspaceContext& c) : Workspace(c) {}
        const char* id() const override { return "fxtest"; }
        const char* title() const override { return "FX"; }
        std::vector<std::string> sections() const override { return {}; }
        mutable int drawn = 0, asked = 0;

    protected:
        void layout(const funkgui::Rect&) override {}
        void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override
        {
            const tee::ui::look::Palette p = tee::ui::look::Palette::of(th);
            const funkgui::Rect r = stage();
            asked += 4;
            drawn += fx::glow(c, r, 1.0f, p.hot, 0.25f) ? 1 : 0;
            drawn += fx::flame(c, r, 1.0f, 0.5f, true) ? 1 : 0;
            drawn += fx::smoke(c, r, 1.0f, p.dim, 0.75f) ? 1 : 0;
            drawn += fx::shimmer(c, r, 1.0f, p.text, 0.1f) ? 1 : 0;
            c.text("PRIMITIVES STILL DRAWN", r.x + 20.0f, r.y + 20.0f, tee::ui::look::kTitle, p.text);
        }
    };

    std::string readFile(const std::string& path)
    {
        std::ifstream f(path);
        std::stringstream s;
        s << f.rdbuf();
        return s.str();
    }

    VIEW_TEST(testFxSoftware, "fx.software")
    {
        tee::ui::DemoRig demo;
        FxStage stage(demo.ctx);
        funkgui::HeadlessHost host(stage, 0, 2.0f);
        host.resize(400, 300);
        stage.place({ 0.0f, 0.0f, 400.0f, 300.0f });
        host.tick(2);
        const funkgui::PrimList& prims = host.draw();
        CHECK(stage.asked >= 4 && stage.drawn == 0);
        CHECK(prims.shaders.empty());                    // nothing recorded: no placeholder box
        for (const fx::Effect e : { fx::Effect::glow, fx::Effect::flame, fx::Effect::smoke, fx::Effect::shimmer })
            CHECK(!fx::available(e));
        fx::setEnabled(false);
        CHECK(!fx::enabled());
        fx::setEnabled(true);
        CHECK(fx::enabled());
        picture("fx.software", host);                    // the primitives alone, as on any machine without the GPU
        return 0;
    }

    // The shaders read only what FunkGui sets, include nothing (no font: no text), and declare no uniform of their own.
    VIEW_TEST(testFxSources, "fx.sources")
    {
        const std::string root = std::string(TEE_VIEW_GOLDEN_DIR) + "/../../../../shaders/fx/";
        const std::regex word("\\b(u_[A-Za-z_]+|uniform|SAMPLER2D|#include|text\\s*\\()");
        for (const fx::Effect e : { fx::Effect::glow, fx::Effect::flame, fx::Effect::smoke, fx::Effect::shimmer })
        {
            const std::string src = readFile(root + fx::fileOf(e));
            CHECK_MSG(src.find("void main()") != std::string::npos, fx::fileOf(e));
            // Comments aside, the code names none of those (the in0..in15 names are the panel prelude's u_params).
            std::string code;
            std::istringstream lines(src);
            for (std::string l; std::getline(lines, l);)
                code += l.substr(0, l.find("//")) + "\n";
            std::smatch m;
            CHECK_MSG(!std::regex_search(code, m, word), std::string(fx::fileOf(e)) + ": " + (m.empty() ? "" : m.str()));
        }
        const std::string list = readFile(root + "fx.gui");
        for (const fx::Effect e : { fx::Effect::glow, fx::Effect::flame, fx::Effect::smoke, fx::Effect::shimmer })
            CHECK(list.find(std::string("\"") + fx::fileOf(e) + "\"") != std::string::npos);
        return 0;
    }
}
