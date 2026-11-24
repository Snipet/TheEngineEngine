// ui/views/_example/ExampleWorkspace.cpp: the smallest whole workspace (docs/handoffs/u1-interface.md). A widget made
// on the Board and placed in layout(); a part drawn with the Canvas in drawStage() whose press selects its Spec
// section (the inspector then shows that section's fields); a live value read by name, in the user's units. A
// directory whose name starts with '_' is not put on the rail; a real one has a register.h (see that file's note).
#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/widgets/Gauge.h"

namespace tee::ui
{
    class ExampleWorkspace final : public Workspace
    {
    public:
        explicit ExampleWorkspace(WorkspaceContext& c) : Workspace(c)
        {
            tach_ = &make<Gauge>("Engine", [this] { return ctx_.live.get("engine.speed") * 30.0 / 3.14159265358979; }, 0.0, 8000.0);
            tach_->setRedline(6800.0);
            tach_->setUnit("RPM");
        }
        const char* id() const override { return "example"; }
        const char* title() const override { return "EXAMPLE"; }
        std::vector<std::string> sections() const override { return { "boost" }; }
        std::uint64_t drawKey() const override { return DrawKey{}.mix(boostText()).mix(ctx_.selection.revision()).h; }

        void pointerDown(const funkgui::PointerEvent& e) override
        {
            if (turbo_.contains({ e.x, e.y }))
                ctx_.selection.select("boost");   // the inspector shows boost's fields
            else
                Board::pointerDown(e);            // the widgets'
        }

    protected:
        void layout(const funkgui::Rect& s) override
        {
            tach_->setPlace({ s.x + 24.0f, s.y + 24.0f, 240.0f, 240.0f }, true);
            turbo_ = { s.x + 300.0f, s.y + 60.0f, 220.0f, 110.0f };
        }
        void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override
        {
            const look::Palette p = look::Palette::of(th);
            look::region(c, p, turbo_, "TURBO", ctx_.selection.is("boost"));
            look::readout(c, p, turbo_.x + look::kPad, turbo_.y + 34.0f, "BOOST", boostText());
        }

    private:
        std::string boostText() const   // "-" while the program has no boost.pressure
        {
            const double pa = ctx_.live.get("boost.pressure");
            return std::isfinite(pa) ? ctx_.units.format(Quantity::pressure, pa) : "-";
        }
        Gauge* tach_ = nullptr;
        funkgui::Rect turbo_{};
    };

    std::unique_ptr<Workspace> makeExampleWorkspace(WorkspaceContext& c) { return std::make_unique<ExampleWorkspace>(c); }
}
