// tests/ui/build/BoreFloorTests.cpp: a stated bore under its least is refused at a person's input (C's rule, 26 Nov:
// Build::accepts; exhaust pipes 10 mm, the intake's runner 8 mm, its snorkel 10 mm; 0 still means derived). Rows over
// the real Spec: 3 mm typed into an exhaust pipe's bore (the crossover's, a main pipe's) and into the intake's runner is refused as Rows refuses a value
// out of range ("Out of range: " and C's reason, the typed text kept, nothing set); 0 typed is taken; BuildDoc::set
// refuses such a bore from any of the interface's writers; an array's table refuses it in its cell.
#include "build/BuildKit.h"

#include "kit/inspect/FieldInspector.h"
#include "kit/inspect/Outline.h"
#include "kit/widgets/Tables.h"

#include <cmath>

namespace
{
    using tee::test::build::Rig;
    using tee::ui::FieldInspector;

    // Types `keys` into the field's value in Rows over the build and presses Return; the inspector after it.
    void typeInto(Rig& rig, FieldInspector& col, funkgui::HeadlessHost& host, int field, const char* keys)
    {
        funkgui::Rect row, value;
        CHECK_MSG(col.fieldRect(field, row, value), rig.doc->info(field).path + " is not in the rows");
        host.click(value.centreX(), value.centreY());
        host.tick(1);
        CHECK_MSG(col.typing(), "a click on " + rig.doc->info(field).path + "'s value did not start typing");
        host.keys("backspace,backspace,backspace,backspace,backspace,backspace,backspace,backspace,backspace,backspace");
        host.keys(keys);
        host.keys("return");
        host.tick(2);
    }

    BUILD_TEST(boreFloorTest, "build.bore.floor")
    {
        tee::ui::BuildDoc& doc = *rig.doc;
        // An exhaust pipe's bore that Rows shows as a row (a field, and an element of a short array), and the runner's.
        std::vector<int> fields;
        for (const char* path : { "exhaust.crossD", "exhaust.mainD[0]", "intake.runnerBore" })
        {
            const int f = doc.find(path);
            CHECK_MSG(f >= 0, std::string("no ") + path + " in the document");
            if (f >= 0)
                fields.push_back(f);
        }
        tee::ui::Board board{ 600, 900 };
        funkgui::HeadlessHost host{ board, 0, 1.0f };
        FieldInspector& col = board.make<FieldInspector>(doc, rig.units, FieldInspector::Style::rows);
        col.setOutline(tee::ui::outlineOf(doc, fields));
        col.setPlace({ 20, 20, 340, 820 }, true);
        host.tick(2);

        for (const int field : fields)
        {
            const std::string path = doc.info(field).path;
            doc.set(field, 0.050);   // a stated bore, 50 mm
            host.tick(2);
            CHECK_MSG(std::fabs(doc.get(field) - 0.050) < 1e-9, path + " did not take 50 mm");
            // 3 mm: refused, C's reason said, the typing kept, the value as it was.
            typeInto(rig, col, host, field, "3");
            CHECK_MSG(col.refusal().rfind("Out of range: under the least stated bore", 0) == 0, path + ": " + col.refusal());
            CHECK_MSG(col.typing() && col.typed() == "3", path + ": the typed text was not kept (" + col.typed() + ")");
            CHECK_MSG(std::fabs(doc.get(field) - 0.050) < 1e-9, path + " took 3 mm");
            std::printf("  %s, 3 mm typed: \"%s\"\n", path.c_str(), col.refusal().c_str());
            host.keys("escape");
            // 0: taken (derived).
            typeInto(rig, col, host, field, "0");
            CHECK_MSG(col.refusal().empty() && !col.typing(), path + ", 0 typed: " + col.refusal());
            CHECK_MSG(doc.get(field) == 0.0, path + " did not take 0 (derived)");
            // Any writer of the interface: BuildDoc::set refuses 3 mm, takes 20 mm.
            doc.set(field, 0.003);
            CHECK_MSG(doc.get(field) == 0.0, path + ": BuildDoc::set took 3 mm");
            doc.set(field, 0.020);
            CHECK_MSG(std::fabs(doc.get(field) - 0.020) < 1e-9, path + ": BuildDoc::set refused 20 mm");
        }
        // An array's table (the primaries' bores: too many for rows): 3 mm typed into a cell is refused as text that is
        // no number is (the typing kept, marked), its sentence said (J2's TableText::refusal); 0 and 20 mm taken, the
        // sentence gone.
        {
            const int first = doc.find("exhaust.primD[0]");
            CHECK(first >= 0);
            tee::ui::GuiNode n;
            n.kind = tee::ui::GuiKind::widget;
            n.widget = "array";
            n.file = "exhaust.primD";
            std::unique_ptr<tee::ui::Item> item = tee::ui::fieldWidget(n, 9001, doc, rig.units, rig.telemetry);
            auto* table = dynamic_cast<tee::ui::MapEdit*>(item.get());
            CHECK_MSG(table != nullptr, "exhaust.primD's widget is no table");
            if (table == nullptr || first < 0)
                return 0;
            tee::ui::Board tb{ 900, 400 };
            funkgui::HeadlessHost th{ tb, 0, 1.0f };
            tb.add(std::move(item));
            table->setPlace({ 20, 20, 860, 300 }, true);
            th.tick(2);
            doc.set(first, 0.040);
            th.tick(2);
            const funkgui::Rect cell = table->cellRect(0, 0);
            th.click(cell.centreX(), cell.centreY());
            th.keys("3,return");
            th.tick(2);
            CHECK_MSG(table->editing(), "3 mm typed into a primary's cell was taken or dropped");
            CHECK_MSG(table->refusal().rfind("Out of range: under the least stated bore", 0) == 0, table->refusal());
            CHECK_MSG(std::fabs(doc.get(first) - 0.040) < 1e-9, "the primary's bore moved");
            th.keys("escape,0,return");
            th.tick(2);
            CHECK_MSG(!table->editing() && doc.get(first) == 0.0, "0 typed into a primary's cell was not taken");
            CHECK_MSG(table->refusal().empty(), "the refusal still said once 0 was taken: " + table->refusal());
            th.click(cell.centreX(), cell.centreY());
            th.keys("2,0,return");
            th.tick(2);
            CHECK_MSG(!table->editing() && std::fabs(doc.get(first) - 0.020) < 1e-9, "20 mm typed into a primary's cell was not taken");
            std::printf("  exhaust.primD[0] in its table: 3 mm refused, 0 and 20 mm taken\n");
        }
        return 0;
    }
}
