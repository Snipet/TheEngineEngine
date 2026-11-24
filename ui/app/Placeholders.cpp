// ui/app/Placeholders.cpp: see Placeholders.h.
#include "app/Placeholders.h"

#include "app/Look.h"
#include "kit/widgets/Basic.h"

namespace tee::ui
{
    namespace
    {
        class Placeholder final : public Workspace
        {
        public:
            Placeholder(WorkspaceContext& c, WorkspaceSlot slot) : Workspace(c), slot_(std::move(slot)) {}
            const char* id() const override { return slot_.id.c_str(); }
            const char* title() const override { return slot_.title.c_str(); }
            std::vector<std::string> sections() const override { return slot_.sections; }
            bool ribbon() const override { return slot_.ribbon; }

        protected:
            void layout(const funkgui::Rect&) override {}
            void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override
            {
                const look::Palette p = look::Palette::of(th);
                std::string body = slot_.promise + "\n\nUntil " + slot_.package
                                 + " lands, the inspector at the right has this workspace's fields.";
                look::placeholder(c, p, stage_, slot_.title, body);
            }

        private:
            WorkspaceSlot slot_;
        };

        std::string firstLine(const std::string& s) { return s.substr(0, s.find('\n')); }

        // J2's garage, kept until U5's: the builds as a list, load, the name, save as.
        class Garage final : public Workspace
        {
        public:
            Garage(WorkspaceContext& c, WorkspaceSlot slot) : Workspace(c), slot_(std::move(slot))
            {
                const FrameServices* s = c.services;
                list_ = &make<List>("Builds", s != nullptr && s->builds ? s->builds() : std::vector<std::string>{});
                load_ = &make<Button>("Load", [this] {
                    const FrameServices* sv = ctx_.services;
                    const int row = list_->selected();
                    if (row < 0 || row >= static_cast<int>(list_->rows().size()) || sv == nullptr || !sv->load)
                    {
                        message_ = "Choose a build to load.";
                        return;
                    }
                    const std::string name = list_->rows()[static_cast<std::size_t>(row)];
                    std::string error;
                    message_ = sv->load(name, error) ? "Loaded " + name + "." : "Not loaded: " + firstLine(error);
                });
                name_ = &make<TextField>("Name", [this] {
                    const FrameServices* sv = ctx_.services;
                    return sv != nullptr && sv->buildName ? sv->buildName() : std::string();
                }, [this](const std::string& n) {
                    const FrameServices* sv = ctx_.services;
                    return !n.empty() && sv != nullptr && sv->rename && sv->rename(n);
                });
                save_ = &make<Button>("Save as", [this] {
                    const FrameServices* sv = ctx_.services;
                    const std::string n = sv != nullptr && sv->buildName ? sv->buildName() : std::string();
                    std::string error;
                    if (sv == nullptr || !sv->save)
                        message_ = "Saving is not here yet.";
                    else if (sv->save(n, error))
                    {
                        message_ = "Saved " + n + ".";
                        if (sv->builds)
                            list_->setRows(sv->builds());
                    }
                    else
                        message_ = "Not saved: " + firstLine(error);
                });
                note_ = &make<Label>([this] { return message_; }, kWidgetText, Label::Ink::text);
            }
            const char* id() const override { return slot_.id.c_str(); }
            const char* title() const override { return slot_.title.c_str(); }
            std::vector<std::string> sections() const override { return slot_.sections; }
            bool ribbon() const override { return slot_.ribbon; }
            void refresh()
            {
                if (ctx_.services != nullptr && ctx_.services->builds)
                    list_->setRows(ctx_.services->builds());
            }

        protected:
            void layout(const funkgui::Rect& s) override
            {
                const float pad = 24.0f, colW = std::min(320.0f, s.w * 0.42f);
                list_->setPlace({ s.x + pad, s.y + pad + 20.0f, colW, std::max(120.0f, s.h - 2 * pad - 70.0f) }, true);
                load_->setPlace({ s.x + pad, s.bottom() - pad - 32.0f, 110.0f, 32.0f }, true);
                const float x = s.x + pad + colW + 28.0f, w = std::max(120.0f, std::min(380.0f, s.right() - pad - x));
                name_->setPlace({ x, s.y + pad + 20.0f, w, 30.0f }, true);
                save_->setPlace({ x, s.y + pad + 62.0f, 110.0f, 32.0f }, true);
                note_->setPlace({ x, s.y + pad + 104.0f, w, 22.0f }, true);
                promise_ = { x, s.y + pad + 140.0f, w, std::max(40.0f, s.bottom() - pad - (s.y + pad + 140.0f)) };
            }
            void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override
            {
                const look::Palette p = look::Palette::of(th);
                c.text("THIS BUILD", name_->bounds().x, stage_.y + 24.0f, look::kCaps, p.label);
                look::placeholder(c, p, promise_, "TO COME", slot_.promise + "\n\nThe garage itself comes with " + slot_.package + ".");
            }
            void shownChanged(bool shown) override
            {
                if (shown)
                    refresh();   // the services' newest list (a save elsewhere, a build added)
            }

        private:
            WorkspaceSlot slot_;
            List* list_ = nullptr;
            Button *load_ = nullptr, *save_ = nullptr;
            TextField* name_ = nullptr;
            Label* note_ = nullptr;
            std::string message_;
            funkgui::Rect promise_{};
        };
    }

    std::unique_ptr<Workspace> makePlaceholder(WorkspaceContext& ctx, const WorkspaceSlot& slot)
    {
        return std::make_unique<Placeholder>(ctx, slot);
    }

    std::unique_ptr<Workspace> makePlaceholderGarage(WorkspaceContext& ctx, const WorkspaceSlot& slot)
    {
        return std::make_unique<Garage>(ctx, slot);
    }
}
