// tests/ui/InspectTests.inc: J2's inspector designs (25 Nov; ui/kit/inspect/) over the made-up Spec, on a Board: what
// each does with the pointer, the wheel and the keys. Their pictures on the real Spec, in the real frame, are
// tee-ui-build-tests build.inspect.pictures (out/ui/inspect/). Included by Tests.cpp.

    // A FieldInspector of `style` over the made-up document's sections, on a board.
    struct InspectRig
    {
        tee::ui::MemoryDoc doc{ tee::test::mockFields() };
        tee::ui::Units units;
        tee::ui::Board board{ 600, 900 };
        funkgui::HeadlessHost host{ board, 0, 2.0f };
        tee::ui::FieldInspector* col = nullptr;

        explicit InspectRig(tee::ui::FieldInspector::Style style, std::vector<std::string> sections = { "engine", "boost" })
        {
            col = &board.make<tee::ui::FieldInspector>(doc, units, style);
            std::vector<int> fields;
            for (const std::string& s : sections)
                for (const int f : tee::ui::sectionFields(doc, s))
                    fields.push_back(f);
            col->setOutline(tee::ui::outlineOf(doc, fields));
            std::vector<int> all;
            for (int f = 0; f < doc.count(); ++f)
                all.push_back(f);
            col->setSearchable(all);
            col->setPlace({ 20, 20, 340, 820 }, true);
            host.tick(10);
        }
        int f(const char* path) const { return doc.find(path); }
        double value(const char* path) const { return doc.get(doc.find(path)); }
        funkgui::Rect valueBox(const char* path) const
        {
            funkgui::Rect row, value;
            col->fieldRect(f(path), row, value);
            return value;
        }
    };

    // Design 1: a value typed within its range is kept (one undo step), in another unit too; out of its range it is
    // refused, the range said, and nothing moves; a choice is typed by its name.
    TEST(testInspectType, "inspect.type")
    {
        InspectRig rig(tee::ui::FieldInspector::Style::rows);
        const funkgui::Rect b = rig.valueBox("boost.level");
        CHECK(b.w > 0.0f);
        const int steps = rig.doc.steps();
        rig.host.click(b.centreX(), b.centreY());
        CHECK(rig.col->typing() && rig.col->typed() == "80.0");
        typeKeys(rig.host, "1 2 5 enter");
        CHECK_NEAR(rig.value("boost.level"), 125e3, 1e-6);
        CHECK(!rig.col->typing() && rig.doc.steps() == steps + 1);
        rig.host.click(b.centreX(), b.centreY());
        typeKeys(rig.host, "1 space b a r enter");      // another unit of the quantity, named
        CHECK_NEAR(rig.value("boost.level"), 100e3, 1e-6);
        rig.host.click(b.centreX(), b.centreY());
        typeKeys(rig.host, "9 0 0 enter");               // past 200 kPa: refused, not clamped
        CHECK(rig.col->typing());
        CHECK_MSG(rig.col->refusal() == "Out of range: 0.0 to 200.0 kPa", rig.col->refusal());
        CHECK_NEAR(rig.value("boost.level"), 100e3, 1e-6);
        typeKeys(rig.host, "escape");
        CHECK(!rig.col->typing() && rig.col->refusal().empty());
        // An integer refuses a fraction; a choice takes its name.
        const funkgui::Rect cyl = rig.valueBox("engine.cylinders");
        rig.host.click(cyl.centreX(), cyl.centreY());
        typeKeys(rig.host, "6 . 5 enter");
        CHECK(rig.col->refusal().find("A whole number") == 0 && rig.value("engine.cylinders") == 8.0);
        typeKeys(rig.host, "backspace backspace enter");
        CHECK(rig.value("engine.cylinders") == 6.0);
        CHECK(rig.col->beginTyping(rig.f("engine.layout")));
        typeKeys(rig.host, "v 1 2 enter");
        CHECK(rig.value("engine.layout") == 5.0);
        rig.doc.undo();
        CHECK(rig.value("engine.layout") == 3.0);
        return 0;
    }

    // A drag sideways scrubs (one step), the wheel nudges (a burst one step), a double-click puts the default back.
    TEST(testInspectScrub, "inspect.scrub")
    {
        InspectRig rig(tee::ui::FieldInspector::Style::rows);
        const funkgui::Rect b = rig.valueBox("boost.level");
        int steps = rig.doc.steps();
        rig.host.drag(b.centreX(), b.centreY(), b.centreX() + 60.0f, b.centreY(), 10);   // a quarter of 240 px
        CHECK_NEAR(rig.value("boost.level"), 80e3 + 0.25 * 200e3, 1.0);
        CHECK(rig.doc.steps() == steps + 1 && !rig.col->typing());
        steps = rig.doc.steps();
        for (int i = 0; i < 3; ++i)
            rig.host.wheel(b.centreX(), b.centreY(), 1.0f);
        CHECK_NEAR(rig.value("boost.level"), 130e3 + 3 * 2e3, 1.0);   // a hundredth of the travel a notch
        rig.host.tick(60);                                // the wheel rests: its burst closes
        CHECK(rig.doc.steps() == steps + 1);
        rig.host.doubleClick(b.centreX(), b.centreY());
        CHECK_NEAR(rig.value("boost.level"), 80e3, 1e-6);
        CHECK(!rig.col->typing());
        // A switch flips on a click.
        const funkgui::Rect sw = rig.valueBox("boost.blowOff");
        rig.host.click(sw.centreX(), sw.centreY());
        CHECK(rig.value("boost.blowOff") == 0.0);
        return 0;
    }

    // The filter finds across the workspace (every section); CHANGED shows what is off its default; a heading folds.
    TEST(testInspectFilter, "inspect.filter")
    {
        InspectRig rig(tee::ui::FieldInspector::Style::rows);
        const int all = rig.col->shownFields();
        CHECK(all == static_cast<int>(tee::ui::sectionFields(rig.doc, "engine").size() + tee::ui::sectionFields(rig.doc, "boost").size()));
        rig.host.click(rig.col->filterRect().centreX(), rig.col->filterRect().centreY());
        typeKeys(rig.host, "t e m p");
        CHECK(rig.col->filter() == "temp" && rig.col->shownFields() == 1);   // env.temperature: not on this page
        funkgui::Rect row, value;
        CHECK(rig.col->fieldRect(rig.f("env.temperature"), row, value));
        typeKeys(rig.host, "escape");
        CHECK(rig.col->filter().empty() && rig.col->shownFields() == all);
        rig.doc.set(rig.f("engine.compression"), 12.0);
        rig.doc.set(rig.f("boost.size"), 0.7);
        rig.host.click(rig.col->changedRect().centreX(), rig.col->changedRect().centreY());
        CHECK(rig.col->changedOnly() && rig.col->shownFields() == 2);
        rig.host.click(rig.col->changedRect().centreX(), rig.col->changedRect().centreY());
        CHECK(!rig.col->changedOnly());
        // Fold the first heading: its fields go, the others stay.
        const auto& rows = rig.col->rows();
        CHECK(!rows.empty() && rows.front().kind == tee::ui::FieldInspector::Row::Kind::heading);
        const std::size_t before = rows.size();
        funkgui::Rect h{ rows.front().rect.x, rig.col->bounds().y + 56.0f + rows.front().rect.y, rows.front().rect.w, rows.front().rect.h };
        rig.host.click(h.centreX(), h.centreY());
        CHECK(rig.col->rows().size() < before);
        CHECK(!rig.col->fieldRect(rig.f("engine.layout"), row, value));
        return 0;
    }

    // Keys: Down moves, a digit types, Left and Right nudge, Delete is the default; hover help is each row's.
    TEST(testInspectKeys, "inspect.keys")
    {
        InspectRig rig(tee::ui::FieldInspector::Style::rows);
        typeKeys(rig.host, "tab down down");             // engine.layout, engine.cylinders
        CHECK(rig.col->focusedField() == rig.f("engine.cylinders"));
        typeKeys(rig.host, "4 enter");
        CHECK(rig.value("engine.cylinders") == 4.0);
        typeKeys(rig.host, "right right");
        CHECK(rig.value("engine.cylinders") == 6.0);
        typeKeys(rig.host, "delete");
        CHECK(rig.value("engine.cylinders") == 8.0);
        // hover help: the row's field, its help and its range; another row, another tip
        funkgui::Rect row, value;
        rig.col->fieldRect(rig.f("boost.level"), row, value);
        rig.host.move(row.x + 30.0f, row.centreY());
        rig.host.tick(60);
        std::string title, body;
        funkgui::Rect anchor;
        CHECK(rig.col->help(title, body, anchor) && title == "Boost" && body.find("Range 0.0 to 200.0 kPa") != std::string::npos);
        const int part = rig.col->helpPart();
        rig.col->fieldRect(rig.f("boost.size"), row, value);
        rig.host.move(row.x + 30.0f, row.centreY());
        CHECK(rig.col->helpPart() != part);
        const auto items = rig.host.accessibility();
        bool slider = false;
        for (const auto& it : items)
            slider = slider || (it.title == "Boost" && it.role == funkgui::A11yRole::slider && it.value == "80.0 kPa");
        CHECK(slider);
        return 0;
    }

    // Design 2: a dial turned up and down (one step), its value typed on a click; design 3's card: a few fields, a
    // title, its height from what it holds.
    TEST(testInspectDials, "inspect.dials")
    {
        InspectRig rig(tee::ui::FieldInspector::Style::dials, { "boost" });
        funkgui::Rect row, value;
        const int steps = rig.doc.steps();
        CHECK(rig.col->fieldRect(rig.f("boost.size"), row, value));
        const float fy = value.y - 30.0f;                 // the face sits over the typed box
        rig.host.drag(value.centreX(), fy, value.centreX(), fy - 50.0f, 10);   // 200 px the whole travel
        CHECK_NEAR(rig.value("boost.size"), 0.75, 1e-3);
        CHECK(rig.doc.steps() == steps + 1);
        rig.host.click(value.centreX(), value.centreY());
        CHECK(rig.col->typing());
        typeKeys(rig.host, "4 0 enter");                 // a fraction typed in percent
        CHECK_NEAR(rig.value("boost.size"), 0.40, 1e-9);
        // The card.
        tee::ui::FieldInspector& card = rig.board.make<tee::ui::FieldInspector>(rig.doc, rig.units, tee::ui::FieldInspector::Style::list);
        card.setOutline(tee::ui::outlineOf(rig.doc, { rig.f("boost.level"), rig.f("boost.size"), rig.f("boost.blowOff") }));
        card.setCard("Turbo", "4 FIELDS HERE; ALL IN THE LIST");
        card.setPlace({ 380, 40, 300, 2000 }, true);
        CHECK(card.contentHeight() < 200.0f && card.shownFields() == 3);
        return 0;
    }
