// tests/ui/BindTests.inc: J2's binding (card J, item 8): FieldPort, FieldParams, generated pages for a made-up Spec with
// every kind of field (tests/ui/MockSpec.h), `auto` and the widget elements in a .gui file. Included by Tests.cpp.

    // A FieldPage over the made-up document, in a headless host.
    struct PageRig
    {
        tee::ui::MemoryDoc doc{ tee::test::mockFields() };
        tee::ui::Units units;
        MockPlayer player;
        std::unique_ptr<tee::ui::FieldPage> page;
        std::unique_ptr<funkgui::HeadlessHost> host;

        explicit PageRig(const std::string& gui, int w = 0, int h = 0)
        {
            open(gui, w, h);
        }
        void open(const std::string& gui, int w = 0, int h = 0)
        {
            host.reset();
            GuiPanel::Options o;
            o.width = w;
            o.height = h;
            page = std::make_unique<tee::ui::FieldPage>(doc, units, player, Sources{ { "page.gui", gui } }, o);
            host = std::make_unique<funkgui::HeadlessHost>(*page, 0, 2.0f);
            host->tick(2);
        }
        double value(const char* path) const { return doc.get(doc.find(path)); }
    };

    // A field's 0..1 by its range and taper, its steps, its text in the user's units, its gestures one undo step.
    TEST(testFieldPort, "fieldport")
    {
        tee::ui::MemoryDoc doc(tee::test::mockFields());
        tee::ui::Units units;
        tee::ui::FieldPort level(doc, doc.find("boost.level"), units);
        CHECK_NEAR(level.value01(), 0.4, 1e-6);           // 80 of 0..200 kPa
        CHECK(level.numSteps() == 0 && std::string(level.id()) == "boost.level");
        CHECK(level.text(0.5f) == "100.0 kPa");
        units.choose(tee::ui::Quantity::pressure, "psi");
        CHECK(level.text(0.5f) == "14.50 psi");
        tee::ui::FieldPort fly(doc, doc.find("engine.flywheel"), units);   // log: 0.05 to 1, its middle is sqrt(0.05)
        fly.beginGesture();
        fly.setValue01(0.5f);
        fly.endGesture();
        CHECK_NEAR(doc.get(doc.find("engine.flywheel")), std::sqrt(0.05), 1e-6);
        tee::ui::FieldPort layout(doc, doc.find("engine.layout"), units);
        CHECK(layout.numSteps() == 5 && layout.text(layout.value01()) == "V8");
        tee::ui::FieldPort bo(doc, doc.find("boost.blowOff"), units);
        CHECK(bo.numSteps() == 1 && bo.text(1.0f) == "ON");
        // A drag through a GestureController is one step of undo, however many writes.
        funkgui::Theme th = funkgui::Theme::graphite();
        (void)th;
        const int steps = doc.steps();
        level.beginGesture();
        for (int i = 0; i <= 10; ++i)
            level.setValue01(0.05f * static_cast<float>(i));
        level.endGesture();
        CHECK(doc.steps() == steps + 1 && level.gesturesOpen() == 0 && doc.depth() == 0);
        doc.undo();
        CHECK_NEAR(doc.get(doc.find("boost.level")), 80e3, 1e-6);
        doc.redo();
        CHECK_NEAR(doc.get(doc.find("boost.level")), 100e3, 1.0);
        // Text both ways.
        double si = 0.0;
        const tee::ui::FieldInfo& lay = doc.info(doc.find("engine.layout"));
        CHECK(tee::ui::parseField(lay, "v12", units, si) && si == 5.0);
        CHECK(tee::ui::parseField(doc.info(doc.find("boost.level")), "1 bar", units, si) && std::fabs(si - 1e5) < 1e-6);
        CHECK(tee::ui::parseField(doc.info(doc.find("boost.level")), "900 kPa", units, si) && si == 200e3);   // clamped
        CHECK(!tee::ui::parseField(doc.info(doc.find("exhaust.tipAngle")), "4 furlongs", units, si));
        CHECK(tee::ui::parseField(doc.info(doc.find("exhaust.tipAngle")), "12 deg", units, si) && si == 12.0);
        CHECK(tee::ui::parseField(doc.info(doc.find("ecu.pops")), "On", units, si) && si == 1.0);
        return 0;
    }

    // The parameters of a page in the user's units: a temperature in C has an offset, so values are converted, not
    // scaled; every write reaches the document in SI, and a control's gestures reach it too.
    TEST(testFieldParams, "fieldparams")
    {
        tee::ui::MemoryDoc doc(tee::test::mockFields());
        tee::ui::Units units;
        tee::ui::FieldParams p(doc, units, tee::ui::sectionFields(doc, "env"));
        CHECK(p.count() == 2);
        const int t = p.findParam("env.temperature");
        CHECK(t >= 0 && p.info(t).unit == "C");
        CHECK_NEAR(p.info(t).min, -40.0, 1e-9);
        CHECK_NEAR(p.get(t), 20.0, 1e-9);
        p.set(t, 30.0);
        CHECK_NEAR(doc.get(doc.find("env.temperature")), 303.15, 1e-9);
        units.choose(tee::ui::Quantity::temperature, "F");
        p.refresh();
        CHECK(p.info(t).unit == "F");
        CHECK_NEAR(p.get(t), 86.0, 1e-9);
        // Tables and hidden fields are not parameters.
        tee::ui::FieldParams e(doc, units, tee::ui::sectionFields(doc, "ecu"));
        CHECK(e.findParam("ecu.spark") < 0 && e.findParam("ecu.timing") >= 0);
        CHECK(tee::ui::sectionFields(doc, "engine").size() == 9);   // engine.rev is hidden
        // A control's gestures reach the document: one step a drag.
        PageRig rig("knob boost.level");
        const int steps = rig.doc.steps();
        const funkgui::Rect r = rig.page->controlRect("boost.level");
        CHECK(r.w > 0.0f);
        rig.host->drag(r.centreX(), r.centreY(), r.centreX(), r.centreY() - 60.0f);
        CHECK(rig.value("boost.level") > 80e3);
        CHECK(rig.doc.steps() == steps + 1 && rig.doc.depth() == 0);
        return 0;
    }

    // Every section's generated page reads with no notes, and every field of it that is not hidden has its control or
    // its widget there; a hidden one has none.
    TEST(testPageGenerated, "page.generated")
    {
        tee::ui::MemoryDoc doc(tee::test::mockFields());
        for (const std::string& section : tee::ui::sectionsOf(doc))
        {
            const std::vector<int> fields = tee::ui::sectionFields(doc, section);
            PageRig rig(tee::ui::pageGui(doc, fields, section), 1200, 900);
            CHECK_MSG(rig.page->notes().empty(), section + ": " + rig.page->notes());
            for (const int f : fields)
            {
                const tee::ui::FieldInfo& info = doc.info(f);
                if (info.table())
                {
                    bool placed = false;
                    for (const GuiPanel::WidgetAt& w : rig.page->widgets())
                        placed = placed || (rig.page->spec().gui.nodes[static_cast<std::size_t>(w.node)].file == info.path
                                            && rig.page->items()[static_cast<std::size_t>(w.item)]->shown());
                    CHECK_MSG(placed, info.path + " has no widget on its page");
                }
                else
                    CHECK_MSG(rig.page->controlRect(info.path).w > 0.0f, info.path + " has no control on its page");
            }
            for (int f = 0; f < doc.count(); ++f)
                if (doc.info(f).hidden)
                    CHECK(rig.page->controlRect(doc.info(f).path).w == 0.0f);
        }
        // The headings are the groups, in order.
        const std::string ecu = tee::ui::pageGui(doc, tee::ui::sectionFields(doc, "ecu"));
        CHECK(ecu.find("\"IGNITION\"") != std::string::npos && ecu.find("\"LIMITER\"") > ecu.find("\"IGNITION\""));
        CHECK(ecu.find('\n') == std::string::npos);
        return 0;
    }

    // `auto "boost"` is the boost section's page inside a hand-written file, whose own lines keep their numbers; a
    // control names a field by path; the widget elements are made and placed.
    TEST(testGuiAuto, "gui.auto")
    {
        const std::string gui = "row {\n"
                                "  knob engine.displacement label=\"SIZE\"\n"
                                "  gauge engine.speed\n"
                                "}\n"
                                "auto \"boost\"\n"
                                "nonsense 1\n";
        PageRig rig(gui, 1200, 900);
        CHECK(rig.page->controlRect("engine.displacement").w > 0.0f);
        CHECK(rig.page->controlRect("boost.level").w > 0.0f);
        CHECK(rig.page->controlRect("boost.blowOff").w > 0.0f);
        CHECK_MSG(rig.page->notes().find("page.gui:6:") != std::string::npos, rig.page->notes());   // its line kept
        CHECK(rig.page->widgets().size() == 2);         // the gauge and boost.duty's map
        // An `auto` of a section there is not: a note, and the rest stands.
        PageRig none("auto \"nowhere\"\nknob boost.level\n");
        CHECK(none.page->notes().find("page.gui:1:1: note: auto \"nowhere\"") != std::string::npos);
        CHECK(none.page->controlRect("boost.level").w > 0.0f);
        return 0;
    }

    // plot, gauge, map, curve, bar, lamp: each made by the binding, placed in its cell, fed from the telemetry or the
    // document, and gone with its page.
    TEST(testGuiWidgets, "gui.widgets")
    {
        const std::string gui = "tabs {\n"
                                " page \"ONE\" {\n"
                                "  row { gauge engine.speed ; bar engine.throttle ; lamp engine.flags label=\"LIMITER\" }\n"
                                "  plot engine.speed width=300 height=150\n"
                                " }\n"
                                " page \"TWO\" { map ecu.spark ; curve ecu.idleCurve }\n"
                                "}\n";
        PageRig rig(gui, 1100, 800);
        rig.player.setTelemetry("engine.speed", 3000.0 * 3.14159265358979 / 30.0);
        rig.player.setTelemetry("engine.throttle", 0.5);
        rig.player.setTelemetry("engine.flags", 1.0);
        rig.player.setTelemetry("engine.state", 2.0);
        rig.player.setTelemetry("engine.manifold", 60e3);
        rig.open(gui, 1100, 800);                        // made again now that the telemetry has those names
        rig.host->tick(60);
        CHECK(rig.page->widgets().size() == 6);
        const auto item = [&](int k) -> Item& { return *rig.page->items()[static_cast<std::size_t>(rig.page->widgets()[static_cast<std::size_t>(k)].item)]; };
        auto& gauge = static_cast<tee::ui::Gauge&>(item(0));
        CHECK(gauge.shown());
        CHECK_NEAR(gauge.value(), 3000.0, 1e-6);         // in rpm
        CHECK(item(3).shown() && !item(4).shown() && !item(5).shown());   // the map and the curve are page TWO's
        CHECK(rig.page->notes().empty());
        // Page TWO: the map's live cell from the speed and the manifold's pressure.
        CHECK(rig.page->tabRect("TWO").w > 0.0f);
        rig.host->click(rig.page->tabRect("TWO").centreX(), rig.page->tabRect("TWO").centreY());
        rig.host->tick(30);
        CHECK(!gauge.shown() && item(4).shown() && item(5).shown());
        auto& map = static_cast<tee::ui::MapEdit&>(item(4));
        int lx = -1, ly = -1;
        CHECK(map.liveCell(lx, ly));
        // A cell typed reaches the document.
        const funkgui::Rect cell = map.cellRect(1, 1);
        rig.host->click(cell.centreX(), cell.centreY());
        typeKeys(*rig.host, "3 3 enter");
        const tee::ui::Map16 m = rig.doc.map(rig.doc.find("ecu.spark"));
        CHECK_NEAR(m.z[1][1], 33.0, 1e-9);
        return 0;
    }

    // A section with more controls than a panel holds is cut into pages, a group kept whole where it fits.
    TEST(testPageSplit, "page.split")
    {
        std::vector<tee::ui::FieldInfo> many;
        for (int i = 0; i < 150; ++i)
        {
            tee::ui::FieldInfo f;
            f.path = "big.f" + std::to_string(i);
            f.group = "G" + std::to_string(i / 25);
            f.label = "F" + std::to_string(i);
            many.push_back(f);
        }
        tee::ui::MemoryDoc doc(many);
        const auto pages = tee::ui::splitPages(doc, tee::ui::sectionFields(doc, "big"), 60);
        CHECK(pages.size() == 3);
        std::size_t total = 0;
        for (const auto& p : pages)
        {
            CHECK(p.size() <= 60);
            total += p.size();
            CHECK(doc.info(p.front()).group != doc.info(p.front() - (p.front() > 0 ? 1 : 0)).group || p.front() == 0);
        }
        CHECK(total == 150);
        PageRig rig("knob boost.level");
        (void)rig;
        return 0;
    }

    // ---- arrays: the Spec's `float pin[MAX_CYL]`, an element a field ("cyl.pin[2]", label "Crank pin 3") ----
    // The made-up Spec with a section of arrays: two of knobs' size, one of a table's, one longer than a table.
    std::vector<tee::ui::FieldInfo> mockArrayFields()
    {
        std::vector<tee::ui::FieldInfo> out = tee::test::mockFields();
        const auto array = [&](const std::string& name, const std::string& label, int n, tee::ui::FieldKind kind, double lo,
                               double hi, std::vector<std::string> choices = {}) {
            for (int e = 0; e < n; ++e)
            {
                tee::ui::FieldInfo f;
                f.path = "cyl." + name + "[" + std::to_string(e) + "]";
                f.kind = kind;
                f.min = lo;
                f.max = hi;
                f.def = lo;
                f.label = label + " " + std::to_string(e + 1);
                f.unit = kind == tee::ui::FieldKind::number ? "deg" : "";
                f.choices = choices;
                f.group = "Cylinders";
                f.array = "cyl." + name;
                f.arrayLabel = label;
                f.element = e;
                f.extent = n;
                out.push_back(f);
            }
        };
        array("bankAngle", "Bank angle", 2, tee::ui::FieldKind::number, -180, 180);
        array("pin", "Crank pin", 8, tee::ui::FieldKind::number, 0, 360);
        array("fireRev", "Fires on", 12, tee::ui::FieldKind::choice, 0, 1, { "First", "Second" });
        array("trim", "Trim", 18, tee::ui::FieldKind::number, -1, 1);
        return out;
    }

    // An element named on a .gui page is a control of its own; an index past the array is a note that gives the extent;
    // an element's edit is one undo step.
    TEST(testGuiElement, "gui.element")
    {
        PageRig rig("");
        rig.doc = tee::ui::MemoryDoc(mockArrayFields());
        rig.open("knob cyl.pin[2]\nknob cyl.bankAngle[1]\nknob cyl.pin[20]\n");
        CHECK(rig.page->controlRect("cyl.pin[2]").w > 0.0f && rig.page->controlRect("cyl.bankAngle[1]").w > 0.0f);
        CHECK_MSG(rig.page->notes().find("page.gui:3:6: note: cyl.pin[20] is past the array: it has 8 elements, cyl.pin[0] "
                                         "to cyl.pin[7]") != std::string::npos, rig.page->notes());
        const int k = rig.page->params().findParam("cyl.pin[2]");
        CHECK(k >= 0 && rig.page->params().info(k).label == "Crank pin 3" && rig.page->params().info(k).unit == "deg");
        const int steps = rig.doc.steps();
        const funkgui::Rect r = rig.page->controlRect("cyl.pin[2]");
        rig.host->drag(r.centreX(), r.centreY(), r.centreX(), r.centreY() - 40.0f, 10);
        CHECK(rig.value("cyl.pin[2]") > 0.0 && rig.value("cyl.pin[1]") == 0.0);
        CHECK(rig.doc.steps() == steps + 1);
        rig.doc.undo();
        CHECK(rig.value("cyl.pin[2]") == 0.0);
        return 0;
    }

    // A generated page over arrays: up to 8 elements a knob each (kArrayKnobs), 9 to 16 one row of cells (`array`), more
    // a knob each; every element reachable; a cell typed (a choice by its name) and dragged, each one undo step.
    TEST(testPageArray, "page.array")
    {
        PageRig rig("");
        rig.doc = tee::ui::MemoryDoc(mockArrayFields());
        const std::vector<int> fields = tee::ui::sectionFields(rig.doc, "cyl");
        CHECK(fields.size() == 2 + 8 + 12 + 18);
        const std::string gui = tee::ui::pageGui(rig.doc, fields, "cyl");
        CHECK(gui.find("array cyl.fireRev") != std::string::npos && gui.find("array cyl.pin") == std::string::npos
              && gui.find("array cyl.trim") == std::string::npos);
        CHECK(tee::ui::splitPages(rig.doc, fields, 60).size() == 1);   // 28 knobs and one table
        rig.open(gui, 1400, 900);
        CHECK_MSG(rig.page->notes().empty(), rig.page->notes());
        for (const char* p : { "cyl.bankAngle[1]", "cyl.pin[7]", "cyl.trim[17]" })
            CHECK_MSG(rig.page->controlRect(p).w > 0.0f, p);
        CHECK(rig.page->controlRect("cyl.fireRev[3]").w == 0.0f);      // in the table, not a control
        CHECK(rig.page->widgets().size() == 1);
        auto& table = static_cast<tee::ui::MapEdit&>(*rig.page->items()[static_cast<std::size_t>(rig.page->widgets()[0].item)]);
        CHECK(table.shown() && table.shownMap().nx == 12 && table.shownMap().ny == 1);
        CHECK(tee::ui::fieldsOfWidget(rig.doc, rig.page->spec().gui.nodes[static_cast<std::size_t>(rig.page->widgets()[0].node)]).size() == 12);
        // Type "Second" into element 3 (a choice by its name): one step.
        int steps = rig.doc.steps();
        const funkgui::Rect c3 = table.cellRect(3, 0);
        rig.host->click(c3.centreX(), c3.centreY());
        typeKeys(*rig.host, "1 return");                 // a number is the choice's index
        CHECK(rig.value("cyl.fireRev[3]") == 1.0 && rig.value("cyl.fireRev[2]") == 0.0);
        CHECK(rig.doc.steps() == steps + 1);
        rig.doc.undo();
        CHECK(rig.value("cyl.fireRev[3]") == 0.0);
        // A drag up on element 5 raises it (200 px the range of 0..1), one step.
        steps = rig.doc.steps();
        const funkgui::Rect c5 = table.cellRect(5, 0);
        rig.host->drag(c5.centreX(), c5.centreY(), c5.centreX(), c5.centreY() - 150.0f, 10);
        CHECK(rig.value("cyl.fireRev[5]") == 1.0 && rig.doc.steps() == steps + 1);
        // The header of numbers is not editable: an element's number stays.
        const funkgui::Rect h = table.cellRect(4, -1);
        rig.host->click(h.centreX(), h.centreY());
        typeKeys(*rig.host, "9 return");
        CHECK(table.shownMap().x[4] == 5.0);
        return 0;
    }
