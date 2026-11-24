// ui/views/garage/ServicesLibrary.h: a Library (Library.h) over what the frame already has today (FrameServices in
// ui/app/Workspace.h: the builds' names, load one, save as a name, the build's name, unsaved), so GARAGE lists the
// application's real builds before the shell implements Library itself. It knows less than a Library should: every
// entry is a whole build with no values and no curve (only the current one has the document's values and the dyno's
// predicted curve), and duplicate, delete, revert and notes are refused with words saying so.
#pragma once

#include "views/garage/Library.h"

#include <string>
#include <vector>

namespace tee::ui::garage
{
    class ServicesLibrary final : public Library
    {
    public:
        explicit ServicesLibrary(WorkspaceContext& ctx);

        const std::vector<BuildSummary>& builds() const override { return builds_; }
        std::uint64_t revision() const override { return revision_; }
        void tick(float dt) override;
        bool load(const std::string& id, Part part, std::string& error) override;
        bool save(std::string& error) override;
        bool saveAs(const std::string& name, std::string& error) override;
        bool duplicate(const std::string& id, std::string& newId, std::string& error) override;
        bool remove(const std::string& id, std::string& error) override;
        bool revert(std::string& error) override;
        bool unsaved() const override;
        std::string notes() const override { return {}; }
        void setNotes(const std::string&) override {}

        // Whether the frame's services can list builds at all.
        static bool usable(const WorkspaceContext& ctx) noexcept;

    private:
        void refresh();

        WorkspaceContext& ctx_;
        std::vector<BuildSummary> builds_;
        std::vector<std::string> names_;
        std::string current_;
        std::uint64_t revision_ = 1, docSeen_ = 0, dynoSeen_ = 0;
    };
}
