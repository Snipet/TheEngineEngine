// tests/ui/build/RowsLaunchTests.cpp: the inspector's Rows answer at launch (J2's finding, 25 Nov: the RowsPages made
// before the frame had a host were never attached to it, so the launch workspace's rows took no press and no key until a
// workspace switch made them again). A fresh frame, no switch: a click on the shown page's first number's value types it.
#include "build/BuildKit.h"

namespace
{
    using tee::test::build::Rig;

    BUILD_TEST(rowsLaunchTest, "build.rows.launch")
    {
        tee::ui::AppFrame& f = *rig.frame;   // as reset() leaves it: made, then shown in a host; nothing switched
        tee::ui::RowsPage* rows = f.inspectorRows();
        CHECK_MSG(rows != nullptr, "the launch workspace (" + f.slot(f.shown()).id + ") shows no Rows");
        if (rows == nullptr)
            return 0;
        int field = -1;
        funkgui::Rect value{};
        for (const auto& r : rows->rows().rows())
        {
            funkgui::Rect row, v;
            if (r.field >= 0 && rig.doc->info(r.field).kind == tee::ui::FieldKind::number && rows->rows().fieldRect(r.field, row, v) && v.w > 0.0f && v.bottom() <= f.inspectorRect().bottom())
            {
                field = r.field;
                value = v;
                break;
            }
        }
        CHECK_MSG(field >= 0, "no value in sight on the launch page");
        if (field < 0)
            return 0;
        rig.host->click(value.centreX(), value.centreY());
        rig.host->tick(5);
        CHECK_MSG(rows->rows().typing(), "a click on " + rig.doc->info(field).path + "'s value at launch did not start typing");
        std::printf("  %s: a click on %s's value at launch types it\n", f.slot(f.shown()).id.c_str(), rig.doc->info(field).path.c_str());
        return 0;
    }
}
