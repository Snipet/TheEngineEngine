// tests/ui/build/EditTableTests.cpp: a game controller's commands on a table (kit/gui/EditCommand.h; the owner's card,
// 26 Nov), in the application's frame over the real Spec, whose rows hold tables (EXHAUST's primaries' bores, an array):
// activate on the table's row opens its page with a cell in focus (MapEdit::focusFirstCell); a value command moves that
// cell; pageNext moves the cell (the table takes it), not the page; back returns to the rows the table was opened from,
// the table's row in focus (RowsPage::focusTable).
#include "build/BuildKit.h"

#include "kit/inspect/FieldInspector.h"
#include "kit/inspect/RowsPage.h"
#include "kit/widgets/Tables.h"

namespace
{
    using tee::test::build::Rig;
    using tee::ui::EditCommand;
    using tee::ui::FieldInspector;

    BUILD_TEST(editTableTest, "build.edit.table")
    {
        tee::ui::AppFrame& f = *rig.frame;
        rig.host->resize(1100, 700);
        f.show(f.find("exhaust"));
        rig.host->tick(5);
        // The first Rows page of EXHAUST holding a table, and the controller's focus walked down to its row.
        bool found = false;
        int rowsPage = -1, first = -1;
        for (int p = 0; p < f.inspectorPageCount() && !found; ++p)
        {
            f.showInspectorPage(p);
            rig.host->tick(2);
            tee::ui::RowsPage* rows = f.inspectorRows();
            if (rows == nullptr || rows->tables().empty())
                continue;
            first = rows->tables().front().fields.front();
            CHECK(rows->focusTable(first));
            const int row = rows->rows().focusedRow();
            found = row >= 0 && rows->rows().rows()[static_cast<std::size_t>(row)].kind == FieldInspector::Row::Kind::table;
            rowsPage = p;
        }
        CHECK_MSG(found, "no Rows page of EXHAUST holds a table");
        if (!found)
            return 0;
        // Activate: the table's page, a cell in focus.
        CHECK(f.edit(EditCommand::activate));
        rig.host->tick(2);
        CHECK_MSG(f.inspectorRows() == nullptr && f.inspectorPageIndex() != rowsPage, "activate on the table's row opened no page");
        const int tablePage = f.inspectorPageIndex();
        // A value command moves the cell's value; pageNext moves the cell, not the page.
        const double was = rig.doc->get(first);
        CHECK(f.edit(EditCommand::valueUpCoarse));
        rig.host->tick(2);
        CHECK_MSG(rig.doc->get(first) != was, rig.doc->info(first).path + ": the table's first cell did not move");
        CHECK(f.edit(EditCommand::pageNext) && f.inspectorPageIndex() == tablePage);
        // Back: the rows it was opened from, the table's row in focus.
        CHECK(f.edit(EditCommand::back));
        rig.host->tick(2);
        CHECK(f.inspectorPageIndex() == rowsPage);
        tee::ui::RowsPage* rows = f.inspectorRows();
        CHECK(rows != nullptr);
        if (rows != nullptr)
        {
            const int row = rows->rows().focusedRow();
            CHECK_MSG(row >= 0 && rows->rows().rows()[static_cast<std::size_t>(row)].kind == FieldInspector::Row::Kind::table,
                      "back from the table's page: its row not in focus");
        }
        std::printf("  %s: opened, a cell moved, back to its row\n", rig.doc->info(first).path.c_str());
        return 0;
    }
}
