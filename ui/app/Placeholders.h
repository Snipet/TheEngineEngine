// ui/app/Placeholders.h: what stands in for a workspace whose package has not landed (its stage says what will be
// there; its inspector, the frame's, is the generated pages of its Spec sections), the garage's stand-in (J2's garage:
// the builds listed, loaded, renamed, saved), and sources that have nothing (a frame given no dyno or Scope).
#pragma once

#include "app/AppFrame.h"
#include "app/Workspace.h"

#include <memory>
#include <string>

namespace tee::ui
{
    std::unique_ptr<Workspace> makePlaceholder(WorkspaceContext& ctx, const WorkspaceSlot& slot);
    std::unique_ptr<Workspace> makePlaceholderGarage(WorkspaceContext& ctx, const WorkspaceSlot& slot);

    class NoDyno final : public DynoSource
    {
    public:
        void tick(float) override {}
        const DynoTrace& predicted() const override { return none_; }
        const DynoTrace& before() const override { return none_; }
        bool busy() const override { return false; }
        double progress() const override { return 0.0; }
        std::string error() const override { return "no dyno runner in this build"; }
        std::uint64_t revision() const override { return 1; }

    private:
        DynoTrace none_;
    };

    class NoScope final : public ScopeSource
    {
    public:
        int find(std::string_view) const override { return -1; }
        int read(int, float*, int) override { return 0; }
        std::uint64_t generation() const override { return 0; }
    };
}
