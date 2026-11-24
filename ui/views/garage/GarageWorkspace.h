// ui/views/garage/GarageWorkspace.h: GARAGE, the builds (docs/plan/ui-design.md section 3; docs/ui/garage.md).
//
//   BUILDS     every build and part in the library as a card: its name, what part it is, the engine in a sentence,
//              peak torque and power with the speeds they fall at, a small power curve; the current build marked
//              (a tag and a bar) and whether it has unsaved changes. A search field (name, sentence, notes) and
//              sorting by name, power, displacement or layout; a filter by part.
//   THE CARD   (the column at the right, the stage's own inspector of the card chosen): its notes, and what can be
//              done with it: load it whole, or one of its parts into the build you have (its engine into this car, its
//              car under this engine, its tune on this engine); compare it with the current build; duplicate; delete
//              (one of yours, after a confirmation). For the current build: its notes to edit; save, save as a name,
//              revert.
//   COMPARE    the current build and the chosen one side by side: torque and power on one plot (torque heavy, power
//              light; the current solid, the other dashed; a legend), and every field that differs, grouped by section,
//              both values in the user's units.
//
// The builds come from a Library (Library.h): the application's, or a DemoLibrary of made-up builds when none is
// given (the stage then says DEMO). Nothing here reaches the file system.
#pragma once

#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/widgets/Basic.h"
#include "views/garage/Describe.h"
#include "views/garage/Library.h"
#include "views/scope/ScopePlot.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    class GarageWorkspace final : public Workspace
    {
    public:
        enum class Sort : std::uint8_t { name, power, displacement, layout };
        enum class Filter : std::uint8_t { all, whole, engine, car, tune };

        // `library` nullptr: its own, over the frame's services when they list builds (ServicesLibrary), else a
        // DemoLibrary of made-up builds over the document.
        explicit GarageWorkspace(WorkspaceContext& c, garage::Library* library = nullptr);
        ~GarageWorkspace() override;

        const char* id() const override { return "garage"; }
        const char* title() const override { return "GARAGE"; }
        std::vector<std::string> sections() const override { return {}; }
        bool moving() const override;
        std::uint64_t drawKey() const override;

        // ---- for tests ----------------------------------------------------------------------------------------------
        garage::Library& library() const noexcept { return *lib_; }
        bool demo() const noexcept { return demo_; }
        // The cards shown, in order: indexes into library().builds().
        const std::vector<int>& shown() const noexcept { return shown_; }
        const std::string& chosen() const noexcept { return chosen_; }
        void choose(const std::string& id);
        void setSearch(const std::string& text);
        void setSort(Sort s);
        void setFilter(Filter f);
        bool comparing() const noexcept { return comparing_; }
        void compareWith(const std::string& id);   // "" : back to the cards
        const std::vector<garage::Difference>& differences() const noexcept { return diffs_; }
        const std::string& message() const noexcept { return message_; }
        // Where a card is (window px) while shown; empty when scrolled out or not shown.
        funkgui::Rect cardRect(int k) const noexcept;

        // funkgui::Panel
        void pointerDown(const funkgui::PointerEvent& e) override;
        bool wheel(const funkgui::WheelEvent& e) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;

    protected:
        void layout(const funkgui::Rect& stage) override;
        void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override;
        void step(float dt) override;

    private:
        const garage::BuildSummary* find(const std::string& id) const;
        const garage::BuildSummary* currentBuild() const;
        void refilter();
        void refreshCompare();
        void act(const char* what, bool ok, const std::string& error);
        void relayout();
        int columns() const noexcept;
        float contentHeight() const noexcept;
        void drawCard(funkgui::Canvas& c, const look::Palette& p, const garage::BuildSummary& b, const funkgui::Rect& r) const;
        void drawDetail(funkgui::Canvas& c, const look::Palette& p) const;
        void drawDiffs(funkgui::Canvas& c, const look::Palette& p) const;

        std::unique_ptr<garage::Library> own_;
        garage::Library* lib_;

        // the bar
        TextField* search_ = nullptr;
        std::array<Button*, 4> sorts_{};
        std::array<Button*, 5> filters_{};
        Button* back_ = nullptr;
        // the card's column
        Button *loadWhole_ = nullptr, *loadEngine_ = nullptr, *loadCar_ = nullptr, *loadTune_ = nullptr;
        Button *compare_ = nullptr, *duplicate_ = nullptr, *delete_ = nullptr, *keep_ = nullptr;
        Button *save_ = nullptr, *revert_ = nullptr;
        TextField *saveAs_ = nullptr, *notes_ = nullptr;
        // compare
        ScopePlot* curves_ = nullptr;

        funkgui::Rect bar_{}, grid_{}, detail_{}, diffRect_{}, actions_{};
        std::string query_, chosen_, compareId_, armed_, message_;
        bool messageBad_ = false;
        Sort sort_ = Sort::name;
        Filter filter_ = Filter::all;
        bool comparing_ = false, demo_ = false;
        std::vector<int> shown_;
        std::vector<garage::Difference> diffs_;
        float scroll_ = 0.0f, scrollShown_ = 0.0f, diffScroll_ = 0.0f;
        std::uint64_t libSeen_ = 0, revision_ = 1;
    };

    std::unique_ptr<Workspace> makeGarageWorkspace(WorkspaceContext& c);
}
