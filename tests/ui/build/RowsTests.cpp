// tests/ui/build/RowsTests.cpp: Rows, the inspector the owner chose (25 Nov), and the card on the picture, in the
// application's own frame over the real Spec (U1's hooks: AppFrame::inspectorRows, AppFrame::partCard).
//
//   build.rows            every workspace's pages: Rows or a table's own page; the filter reaches every page of the
//                         workspace; a table's OPEN shows its page; a value typed out of range is refused (one undo
//                         step when kept); a press on a part of ENGINE and EXHAUST opens its card, Escape closes it
//   build.rows.table      an array's table refuses a bore under the host's least in its cell and says why on its
//                         title's line (FieldDoc::accepts's reason, worded as Rows' refusal)
//   build.rows.pictures   the pictures for the owner, under out/ui/rows/ (named below; drawn by FunkGui's software
//                         rasteriser on Linux)
#include "build/BuildKit.h"

#include "Png.h"
#include "kit/inspect/FieldInspector.h"
#include "kit/inspect/Outline.h"
#include "kit/inspect/PartCard.h"
#include "kit/inspect/RowsPage.h"
#include "kit/widgets/Tables.h"

#include <funkgui/canvas/SoftRaster.h>
#include <funkgui/text/FontService.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace
{
    using tee::test::build::Rig;
    using tee::ui::FieldInspector;

    // The frame at a window of w x h, a workspace and one of its pages shown (by its tab's title; "" the first).
    void show(Rig& rig, int w, int h, const char* workspace, const char* page)
    {
        tee::ui::AppFrame& f = *rig.frame;
        rig.host->resize(w, h);
        f.select({});
        f.show(f.find(workspace));
        rig.host->tick(2);
        for (int p = 0; p < f.inspectorPageCount(); ++p)
            if (f.inspectorTitle(p) == page)
                f.showInspectorPage(p);
        rig.host->tick(30);
    }

    void picture(Rig& rig, const std::string& name)
    {
        const std::filesystem::path dir = std::filesystem::path(TEE_UI_OUT_DIR) / "rows";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        const std::string path = (dir / (name + ".png")).string();
        tee::test::writePng(funkgui::rasterise(rig.host->draw(), funkgui::FontService::get().atlas(), 1), path);
        std::printf("  %s\n", path.c_str());
    }

    // The first number fields the rows show, about a tenth off their defaults (the CHANGED marks in a picture).
    void changeSome(Rig& rig, int n)
    {
        const tee::ui::RowsPage* rows = rig.frame->inspectorRows();
        if (rows == nullptr)
            return;
        int done = 0;
        for (const FieldInspector::Row& r : rows->rows().rows())
            if (r.kind == FieldInspector::Row::Kind::field && done < n)
            {
                const tee::ui::FieldInfo& f = rig.doc->info(r.field);
                if (f.kind != tee::ui::FieldKind::number || !(f.max > f.min))
                    continue;
                const double d = rig.doc->get(r.field);
                rig.doc->set(r.field, std::clamp(d == 0.0 ? f.min + (f.max - f.min) * 0.1 : d * (done == 0 ? 1.1 : 0.92), f.min, f.max));
                ++done;
            }
        rig.host->tick(5);
    }

    // A part on the stage whose press opens the card: a grid of presses over the stage.
    bool pressPart(Rig& rig, funkgui::Point& at)
    {
        tee::ui::AppFrame& f = *rig.frame;
        const funkgui::Rect st = f.stageRect();
        for (float fy = 0.30f; fy < 0.95f; fy += 0.07f)
            for (float fx = 0.20f; fx < 0.95f; fx += 0.06f)
            {
                f.select({});
                rig.host->tick(2);
                at = { st.x + st.w * fx, st.y + st.h * fy };
                rig.host->move(at.x, at.y);
                rig.host->click(at.x, at.y);
                rig.host->tick(5);
                if (f.partCard().isOpen())
                    return true;
            }
        return false;
    }

    // A number field the rows show, on screen: its value's box.
    int shownNumber(const FieldInspector& col, const tee::ui::FieldDoc& doc, funkgui::Rect& value)
    {
        for (const FieldInspector::Row& r : col.rows())
            if (r.kind == FieldInspector::Row::Kind::field && doc.info(r.field).kind == tee::ui::FieldKind::number)
            {
                funkgui::Rect row;
                if (col.fieldRect(r.field, row, value))
                    return r.field;
            }
        return -1;
    }

    BUILD_TEST(rowsInFrame, "build.rows")
    {
        tee::ui::AppFrame& f = *rig.frame;
        rig.host->resize(1100, 700);
        rig.host->tick(5);
        // At launch, before any switch: the shown workspace's rows take a press (attached with the frame).
        if (tee::ui::RowsPage* first = f.inspectorRows())
        {
            funkgui::Rect value;
            const int field = shownNumber(first->rows(), *rig.doc, value);
            CHECK(field >= 0);
            rig.host->click(value.centreX(), value.centreY());
            CHECK_MSG(first->rows().typing(), "a press on the launch workspace's rows does nothing");
            rig.host->keys("escape");
        }
        else
            CHECK_MSG(false, "the launch workspace shows no rows");
        // Every workspace's pages: Rows, or a table's page of its own (a FieldPage); the filter's reach is the union.
        for (int w = 0; w < f.workspaceCount(); ++w)
        {
            f.select({});
            f.show(w);
            rig.host->tick(3);
            std::set<int> all;
            for (int p = 0; p < f.inspectorPageCount(); ++p)
            {
                f.showInspectorPage(p);
                if (const tee::ui::RowsPage* rows = f.inspectorRows())
                {
                    all.insert(rows->fields().begin(), rows->fields().end());
                    CHECK_MSG(rows->rows().shown() && rows->rows().bounds().w > 200.0f, f.slot(w).id + " " + f.inspectorTitle(p));
                }
                else if (f.inspector() != nullptr)
                {
                    const tee::ui::Outline o = tee::ui::outlineOf(*f.inspector(), *rig.doc);
                    int tables = 0, fields = 0;
                    for (const auto& e : o)
                        (e.kind == tee::ui::OutlineEntry::Kind::table ? tables : fields) += e.kind == tee::ui::OutlineEntry::Kind::field
                                                                                                    || e.kind == tee::ui::OutlineEntry::Kind::table;
                    CHECK_MSG(tables <= 1 && fields == 0, f.slot(w).id + " " + f.inspectorTitle(p) + ": a FieldPage that is not a table alone");
                }
            }
            // Each table of each Rows page opens on a page of the frame's.
            for (int p = 0; p < f.inspectorPageCount(); ++p)
            {
                f.showInspectorPage(p);
                const tee::ui::RowsPage* rows = f.inspectorRows();
                if (rows == nullptr)
                    continue;
                for (const tee::ui::RowsPage::Table& t : rows->tables())
                {
                    bool found = false;
                    for (int q = 0; q < f.inspectorPageCount() && !found; ++q)
                    {
                        f.showInspectorPage(q);
                        if (f.inspectorRows() == nullptr && f.inspector() != nullptr)
                        {
                            const std::vector<int> fs = tee::ui::fieldsOf(tee::ui::outlineOf(*f.inspector(), *rig.doc));
                            found = std::find(fs.begin(), fs.end(), t.fields.front()) != fs.end();
                        }
                    }
                    CHECK_MSG(found, f.slot(w).id + ": table " + t.title + " has no page");
                    f.showInspectorPage(p);
                }
            }
        }
        // ENGINE: the filter finds a field of another page; a value typed out of range is refused, the range said;
        // within it, kept as one undo step.
        show(rig, 1100, 700, "engine", "");
        tee::ui::RowsPage* rows = f.inspectorRows();
        CHECK(rows != nullptr);
        if (rows == nullptr)
            return 1;
        FieldInspector& col = rows->rows();
        funkgui::Rect value;
        const int field = shownNumber(col, *rig.doc, value);
        CHECK(field >= 0);
        const double was = rig.doc->get(field);
        rig.host->click(value.centreX(), value.centreY());
        CHECK(col.typing());
        rig.host->keys("9,9,9,9,9,9,return");
        rig.host->tick(2);
        CHECK_MSG(col.refusal().find("Out of range: ") == 0, col.refusal());
        CHECK(rig.doc->get(field) == was);
        rig.host->keys("escape");
        // A table's OPEN, through the filter: the frame shows its page.
        bool table = false;
        for (int p = 0; p < f.inspectorPageCount() && !table; ++p)
        {
            f.showInspectorPage(p);
            if (const tee::ui::RowsPage* r = f.inspectorRows(); r != nullptr && !r->tables().empty())
            {
                const tee::ui::RowsPage::Table t = r->tables().front();
                FieldInspector& c = f.inspectorRows()->rows();
                for (const FieldInspector::Row& row : c.rows())
                    if (row.kind == FieldInspector::Row::Kind::table)
                    {
                        const funkgui::Rect at{ row.value.x, c.bounds().y + 56.0f + row.value.y - c.scroll(), row.value.w, row.value.h };
                        if (at.bottom() > c.bounds().bottom())
                            break;
                        rig.host->click(at.centreX(), at.centreY());
                        rig.host->tick(3);
                        table = f.inspectorRows() == nullptr && f.inspector() != nullptr;
                        CHECK_MSG(table, t.title);
                        break;
                    }
            }
        }
        std::printf("  a table's OPEN tried: %s\n", table ? "yes" : "no table row in view on ENGINE's pages");
        // The card: a press on a part of ENGINE and of EXHAUST opens it beside the part, inside the stage; Escape
        // closes it.
        for (const char* ws : { "engine", "exhaust" })
        {
            show(rig, 1100, 700, ws, "");
            funkgui::Point at{};
            CHECK_MSG(pressPart(rig, at), std::string(ws) + ": no press opened a card");
            if (!f.partCard().isOpen())
                continue;
            const funkgui::Rect card = f.partCardItem().bounds(), stage = f.stageRect();
            CHECK(card.x >= stage.x && card.right() <= stage.right() && card.y >= stage.y && card.bottom() <= stage.bottom());
            CHECK(!card.contains(at));
            CHECK(f.partCardItem().shownFields() > 0 && f.partCardItem().shownFields() <= tee::ui::PartCard::kFields);
            rig.host->keys("escape");
            rig.host->tick(2);
            CHECK(!f.partCard().isOpen());
        }
        return 0;
    }

    BUILD_TEST(rowsTable, "build.rows.table")
    {
        tee::ui::BuildDoc& doc = *rig.doc;
        const int first = doc.find("exhaust.primD[0]");
        CHECK(first >= 0);
        if (first < 0)
            return 0;
        tee::ui::GuiNode n;
        n.kind = tee::ui::GuiKind::widget;
        n.widget = "array";
        n.file = "exhaust.primD";
        std::unique_ptr<tee::ui::Item> item = tee::ui::fieldWidget(n, 9002, doc, rig.units, rig.telemetry);
        auto* table = dynamic_cast<tee::ui::MapEdit*>(item.get());
        CHECK_MSG(table != nullptr, "exhaust.primD's widget is no table");
        if (table == nullptr)
            return 0;
        tee::ui::Board board{ 900, 400 };
        funkgui::HeadlessHost host{ board, 0, 1.0f };
        board.add(std::move(item));
        table->setPlace({ 20, 20, 860, 300 }, true);
        host.tick(2);
        doc.set(first, 0.040);
        host.tick(2);
        const funkgui::Rect cell = table->cellRect(0, 0);
        host.click(cell.centreX(), cell.centreY());
        host.keys("3,return");
        host.tick(2);
        CHECK(table->editing() && std::fabs(doc.get(first) - 0.040) < 1e-9);
        CHECK_MSG(table->refusal().rfind("Out of range: under the least stated bore", 0) == 0, table->refusal());
        std::printf("  exhaust.primD[0], 3 mm typed in its table: \"%s\"\n", table->refusal().c_str());
        host.keys("backspace");
        CHECK(table->refusal().empty());   // a key: a new try
        host.keys("x,return");
        host.tick(2);
        CHECK_MSG(table->refusal().rfind("Not a number of mm: ", 0) == 0, table->refusal());
        host.keys("escape");
        CHECK(!table->editing() && table->refusal().empty());
        return 0;
    }

    BUILD_TEST(rowsPictures, "build.rows.pictures")
    {
        struct Page
        {
            const char* ws;
            const char* page;
            const char* name;
        };
        const Page pages[] = { { "engine", "ENGINE 1", "engine" }, { "tune", "FUEL", "fuel" }, { "exhaust", "EXHAUST 1", "exhaust" },
                               { "tune", "ECU 1", "choices" } };
        const std::pair<int, int> sizes[] = { { 1100, 700 }, { 1600, 1000 } };
        for (const auto& [w, h] : sizes)
        {
            const std::string size = w == 1100 ? "small" : "large";
            for (const Page& p : pages)
            {
                rig.reset();
                show(rig, w, h, p.ws, p.page);
                if (rig.frame->inspectorTitle(rig.frame->inspectorPageIndex()) != p.page)
                    std::printf("  (%s has no page %s: its first page is pictured)\n", p.ws, p.page);
                changeSome(rig, 2);
                const std::string base = std::string("rows.") + p.name + "." + size;
                picture(rig, base);
                tee::ui::RowsPage* rows = rig.frame->inspectorRows();
                if (rows == nullptr)
                    continue;
                FieldInspector& col = rows->rows();
                if (std::string(p.name) == "exhaust")
                {
                    for (int k = 0; k < 8; ++k)
                        rig.host->wheel(col.bounds().x + 20.0f, col.bounds().centreY(), -1.0f);
                    rig.host->tick(30);
                    picture(rig, base + ".scrolled");
                }
                if (std::string(p.name) == "engine")
                {
                    funkgui::Rect value;
                    if (shownNumber(col, *rig.doc, value) >= 0)
                    {
                        rig.host->click(value.centreX(), value.centreY());
                        rig.host->keys("9,9,9,9,9,return");
                        rig.host->tick(5);
                        picture(rig, base + ".refused");
                        rig.host->keys("escape");
                    }
                    col.setFilter("valve");
                    rig.host->tick(20);
                    picture(rig, "rows.filter." + size);
                    col.setFilter({});
                    col.setChangedOnly(true);
                    rig.host->tick(20);
                    picture(rig, "rows.changed." + size);
                    col.setChangedOnly(false);
                }
            }
            if (w == 1100)
            {
                // An array's table, a bore under the least typed in a cell: the refusal said on the title's line.
                rig.reset();
                tee::ui::GuiNode n;
                n.kind = tee::ui::GuiKind::widget;
                n.widget = "array";
                n.file = "exhaust.primD";
                std::unique_ptr<tee::ui::Item> item = tee::ui::fieldWidget(n, 9003, *rig.doc, rig.units, rig.telemetry);
                if (auto* table = dynamic_cast<tee::ui::MapEdit*>(item.get()))
                {
                    tee::ui::Board board{ 700, 160 };
                    funkgui::HeadlessHost host{ board, 0, 2.0f };
                    board.add(std::move(item));
                    table->setPlace({ 16, 16, 668, 128 }, true);
                    host.tick(2);
                    const funkgui::Rect cell = table->cellRect(0, 0);
                    host.click(cell.centreX(), cell.centreY());
                    host.keys("3,return");
                    host.tick(5);
                    const std::filesystem::path dir = std::filesystem::path(TEE_UI_OUT_DIR) / "rows";
                    std::error_code ec;
                    std::filesystem::create_directories(dir, ec);
                    const std::string path = (dir / "rows.table.refused.png").string();
                    tee::test::writePng(funkgui::rasterise(host.draw(), funkgui::FontService::get().atlas(), 1), path);
                    std::printf("  %s\n", path.c_str());
                }
            }
            for (const char* ws : { "engine", "exhaust" })
            {
                rig.reset();
                show(rig, w, h, ws, "");
                funkgui::Point at{};
                if (!pressPart(rig, at))
                    std::printf("  (no press on %s's stage opened a card)\n", ws);
                rig.host->tick(30);
                picture(rig, std::string("rows.card.") + ws + "." + size);
            }
        }
        return 0;
    }
}
