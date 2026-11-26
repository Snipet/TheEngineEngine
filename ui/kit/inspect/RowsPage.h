// ui/kit/inspect/RowsPage.h: the inspector's page as Rows (the owner's pick, 25 Nov: "As recommended"), a pane for the
// application's frame in place of the FieldPage it shows today.
//
// The frame keeps building its pages as now (a workspace's .gui page, or generated pages of a section's scalars and a
// page per table) and makes a RowsPage over each that is not a table alone (RowsPage::over). The FieldPage stays the
// frame's (its controls, its widgets, fieldsShown(), the tests that read it); the RowsPage is what the frame shows and
// gives the pointer and keys to. A table stays on a page of its own: a table's row in Rows (OPEN) calls the frame
// back with the table's field (setOnTable), and the frame shows that table's page.
//
// The board is the frame's size (as a FieldPage is, with insets): place(room) at every layout puts the rows in the
// inspector's room, in frame px. The filter searches every page of the workspace (setSearchable with the union of the
// workspace's RowsPage::fields()); Tab past the rows leaves the pane (the frame's turn), as FieldPage's tabWraps=false.
#pragma once

#include "kit/bind/Binding.h"
#include "kit/inspect/FieldInspector.h"
#include "kit/widgets/Board.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    class RowsPage final : public Board
    {
    public:
        // Rows over a page the frame built; nullptr when the page holds a table alone (that page stays as it is).
        static std::unique_ptr<RowsPage> over(const FieldPage& page, FieldDoc& doc, const Units& units);
        // Rows over the page's outline (outlineOf), in a board of the frame's size.
        RowsPage(FieldDoc& doc, const Units& units, Outline outline, int width, int height);

        // At every layout: the frame's size and the inspector's room (frame px).
        void place(int width, int height, const funkgui::Rect& room);
        // The fields the filter searches (the workspace's: the union of its pages' fields()); empty: its own.
        void setSearchable(std::vector<int> fields) { rows_->setSearchable(std::move(fields)); }
        // A table (a map, a curve, an array's elements): its title and fields. Each wants a page of the frame's, a
        // table alone, to open on: pageGui(doc, table.fields, {}) titled upper(table.title) when the frame has none.
        struct Table
        {
            std::string title;
            std::vector<int> fields;
        };
        // A table's row was pressed (or Enter on it): show the table's page.
        void setOnTable(std::function<void(const Table&)> f) { onTable_ = std::move(f); }
        // Its fields (the tables' among them), and its tables.
        const std::vector<int>& fields() const noexcept { return fields_; }
        const std::vector<Table>& tables() const noexcept { return tables_; }

        FieldInspector& rows() noexcept { return *rows_; }
        const FieldInspector& rows() const noexcept { return *rows_; }
        // For the frame's needsDraw: changes whenever what it draws does (values, hover help, typing, scroll).
        uint64_t drawKey() const;

        // A controller's command to the rows (FieldInspector::edit); false: the frame's turn.
        bool edit(EditCommand cmd, bool fine) { return rows_->edit(cmd, fine); }
        // A page just shown: the board's focus on the rows, the rows' on their first field or table.
        bool focusFirst();
        // Back from a table's page: the board's focus on the rows, the rows' on the table row whose fields include
        // `field` (the table's first, from RowsPage::Table); false when none.
        bool focusTable(int field);

        // Escape is the rows' first (it leaves typing, clears the filter and CHANGED), the board's (the ring) after.
        bool key(const funkgui::KeyEvent& e) override;

    protected:
        void arrange() override;

    private:
        FieldInspector* rows_ = nullptr;
        funkgui::Rect room_{};
        std::vector<int> fields_;
        std::vector<Table> tables_;
        std::function<void(const Table&)> onTable_;
    };
}
