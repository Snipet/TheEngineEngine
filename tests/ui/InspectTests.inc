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
        typeKeys(rig.host, "tab down down down");        // the heading ENGINE, engine.layout, engine.cylinders
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

    // The card (Style::list, setCard): a few fields under a title, no filter, its height from what it holds; a value
    // typed in it is kept or refused as in the rows.
    TEST(testInspectCard, "inspect.card")
    {
        InspectRig rig(tee::ui::FieldInspector::Style::rows, { "boost" });
        tee::ui::FieldInspector& card = rig.board.make<tee::ui::FieldInspector>(rig.doc, rig.units, tee::ui::FieldInspector::Style::list);
        card.setOutline(tee::ui::outlineOf(rig.doc, { rig.f("boost.level"), rig.f("boost.size"), rig.f("boost.blowOff") }));
        card.setCard("Turbo", "4 FIELDS HERE; ALL IN THE LIST");
        card.setPlace({ 380, 40, 300, 2000 }, true);
        CHECK(card.contentHeight() < 200.0f && card.shownFields() == 3);
        CHECK(card.filterRect().isEmpty() && card.changedRect().isEmpty());
        card.setPlace({ 380, 40, 300, 34.0f + 26.0f + card.contentHeight() }, true);
        rig.host.tick(2);
        funkgui::Rect row, value;
        CHECK(card.fieldRect(rig.f("boost.size"), row, value));
        rig.host.click(value.centreX(), value.centreY());
        CHECK(card.typing());
        typeKeys(rig.host, "4 0 enter");                 // a fraction typed in percent
        CHECK_NEAR(rig.value("boost.size"), 0.40, 1e-9);
        rig.host.click(value.centreX(), value.centreY());
        typeKeys(rig.host, "1 4 0 enter");
        CHECK(card.typing() && card.refusal().find("Out of range") == 0);
        CHECK_NEAR(rig.value("boost.size"), 0.40, 1e-9);
        typeKeys(rig.host, "escape /");                  // the card has no filter
        CHECK(card.filter().empty());
        return 0;
    }

    // RowsPage, the frame's pane: over a page the frame built (a page of a table alone stays as it is); its fields and
    // tables; the keyboard through headings, fields and tables (Enter on a table asks the frame to open its page); the
    // filter across the workspace; Tab past it leaves (the frame's turn).
    TEST(testInspectPage, "inspect.page")
    {
        const tee::ui::MemoryDoc spec{ tee::test::mockFields() };
        PageRig rig(tee::ui::pageGui(spec, tee::ui::sectionFields(spec, "boost"), "boost"), 1100, 700);
        PageRig table(tee::ui::pageGui(spec, { spec.find("boost.duty") }, {}), 1100, 700);
        CHECK(tee::ui::RowsPage::over(*table.page, table.doc, table.units) == nullptr);
        std::unique_ptr<tee::ui::RowsPage> rows = tee::ui::RowsPage::over(*rig.page, rig.doc, rig.units);
        CHECK(rows != nullptr);
        CHECK(rows->width() == 1100 && rows->height() == 700);
        CHECK(rows->fields().size() == tee::ui::sectionFields(rig.doc, "boost").size());
        CHECK(rows->tables().size() == 1 && rows->tables()[0].fields == std::vector<int>{ rig.doc.find("boost.duty") });
        std::vector<int> all;
        for (int f = 0; f < rig.doc.count(); ++f)
            all.push_back(f);
        rows->setSearchable(all);
        std::vector<std::string> opened;
        rows->setOnTable([&](const tee::ui::RowsPage::Table& t) { opened.push_back(t.title); });
        funkgui::HeadlessHost host(*rows, 0, 1.0f);
        rows->place(1100, 700, { 760, 100, 340, 560 });
        host.tick(2);
        tee::ui::FieldInspector& col = rows->rows();
        CHECK(col.bounds().x == 760.0f && col.bounds().w == 340.0f && col.shown());
        // The keyboard: Tab to the rows, Down through a heading, its fields and the table; Enter on the table opens it.
        typeKeys(host, "tab");
        CHECK(rows->focus() == 0);
        using Kind = tee::ui::FieldInspector::Row::Kind;
        const auto kindAt = [&] { return col.rows()[static_cast<std::size_t>(col.focusedRow())].kind; };
        int fieldsSeen = 0;
        for (int i = 0; i < 40 && (col.focusedRow() < 0 || kindAt() != Kind::table); ++i)
        {
            typeKeys(host, "down");
            fieldsSeen += col.focusedField() >= 0 ? 1 : 0;
        }
        CHECK(col.focusedRow() >= 0 && kindAt() == Kind::table && fieldsSeen >= 3);
        typeKeys(host, "return");
        CHECK_MSG(opened.size() == 1 && opened[0] == "Duty", std::to_string(opened.size()));
        // A heading folds on Enter, and opens again.
        const std::size_t before = col.rows().size();
        for (int i = 0; i < 40 && kindAt() != Kind::heading; ++i)
            typeKeys(host, "up");
        CHECK(kindAt() == Kind::heading);
        typeKeys(host, "return");
        CHECK(col.rows().size() < before);
        typeKeys(host, "return");
        CHECK(col.rows().size() == before);
        // The filter: '/' and a word finds a field of another page (another section); Down goes to it.
        typeKeys(host, "/ s p a r k");
        CHECK(col.filter() == "spark");
        bool spark = false;
        for (const auto& r : col.rows())
            spark = spark || r.kind == tee::ui::FieldInspector::Row::Kind::table;
        CHECK(spark);                                    // ecu.spark: a table, on no page of this one's
        typeKeys(host, "return return");
        CHECK(opened.size() == 2 && opened[1] == "Spark advance");
        typeKeys(host, "escape");
        CHECK(col.filter().empty() && col.rows().size() == before);
        // A value typed is refused out of range, the range said; the draw key moves with it.
        const std::uint64_t key = rows->drawKey();
        funkgui::Rect row, value;
        CHECK(col.fieldRect(rig.doc.find("boost.level"), row, value));
        host.click(value.centreX(), value.centreY());
        typeKeys(host, "9 0 0 return");
        CHECK(col.refusal() == "Out of range: 0.0 to 200.0 kPa" && rows->drawKey() != key);
        typeKeys(host, "escape");
        // Tab past the rows leaves the board (a pane: the frame's turn).
        funkgui::KeyEvent tab;
        tab.key = funkgui::Key::tab;
        CHECK(!rows->key(tab));
        return 0;
    }
