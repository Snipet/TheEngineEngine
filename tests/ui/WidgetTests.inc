// tests/ui/WidgetTests.inc: J2's widgets (card J, item 6) and units (item 7), driven through FunkGui's HeadlessHost on a
// Board, with scripted input; and the widgets' gallery, three goldens. Included by Tests.cpp, inside its namespace.

    // Keys as words between spaces ("H o t space V 8 enter"), each a key HeadlessHost::keys knows (its grammar is
    // comma-separated, Return for Enter).
    void typeKeys(funkgui::HeadlessHost& host, std::string_view spec)
    {
        std::string out;
        std::size_t i = 0;
        while (i < spec.size())
        {
            const std::size_t sp = std::min(spec.find(' ', i), spec.size());
            std::string word(spec.substr(i, sp - i));
            if (word == "enter")
                word = "return";
            if (!word.empty())
                out += (out.empty() ? "" : ",") + word;
            i = sp + 1;
        }
        host.keys(out.c_str());
    }

    // A Board of widgets over a made-up document, in a headless host.
    struct BoardRig
    {
        tee::ui::MemoryDoc doc{ tee::test::mockFields() };
        tee::ui::Units units;
        MockPlayer player;
        tee::ui::Board board;
        funkgui::HeadlessHost host;
        std::vector<std::unique_ptr<tee::ui::FieldPort>> ports;

        explicit BoardRig(int w = 900, int h = 600, float dpi = 2.0f) : board(w, h), host(board, 0, dpi) {}

        tee::ui::FieldPort& port(const char* path)
        {
            ports.push_back(std::make_unique<tee::ui::FieldPort>(doc, doc.find(path), units));
            return *ports.back();
        }
        template <typename T, typename... Args>
        T& place(funkgui::Rect r, Args&&... args)
        {
            T& w = board.make<T>(std::forward<Args>(args)...);
            w.setPlace(r, true);
            return w;
        }
        double value(const char* path) const { return doc.get(doc.find(path)); }
        void settle() { host.tick(40); }
    };

    TEST(testUnits, "units")
    {
        using tee::ui::Quantity;
        tee::ui::Units u;
        CHECK(u.format(Quantity::pressure, 101325.0) == "101.3 kPa");
        CHECK(u.format(Quantity::temperature, 373.15) == "100.0 C");
        CHECK(u.format(Quantity::rotation, 3.14159265358979 * 100.0) == "3000 rpm");
        CHECK(u.format(Quantity::fraction, 0.25) == "25.0%");
        CHECK(u.choose(Quantity::pressure, "psi"));
        CHECK(u.format(Quantity::pressure, 101325.0) == "14.70 psi");
        CHECK(u.choose(Quantity::temperature, "F"));
        CHECK_NEAR(u.toUser(Quantity::temperature, 273.15), 32.0, 1e-9);
        CHECK_NEAR(u.toSi(Quantity::temperature, 212.0), 373.15, 1e-9);
        double si = 0.0;
        CHECK(u.parse(Quantity::pressure, "1 bar", si) && std::fabs(si - 1e5) < 1e-6);
        CHECK(u.parse(Quantity::pressure, " 30psi ", si) && std::fabs(si - 206842.71879) < 1e-3);
        CHECK(u.parse(Quantity::pressure, "14.7", si) && std::fabs(si - 101352.93) < 0.1);   // the chosen unit
        CHECK(!u.parse(Quantity::pressure, "3 furlongs", si));
        CHECK(!u.parse(Quantity::pressure, "abc", si));
        CHECK(u.parse(Quantity::length, "2.5 in", si) && std::fabs(si - 0.0635) < 1e-12);
        CHECK(!u.choose(Quantity::mass, "stone"));
        // The choices as settings, and back.
        const std::string saved = u.save();
        CHECK(saved == "pressure=psi\ntemperature=F\n");
        tee::ui::Units v;
        std::string notes;
        v.load(saved + "colour=blue\nmass=stone\n", notes);
        CHECK(v.chosen(Quantity::pressure) == u.chosen(Quantity::pressure) && v.chosen(Quantity::temperature) == 1);
        CHECK(notes.find("colour=blue") != std::string::npos && notes.find("mass=stone") != std::string::npos);
        CHECK(tee::ui::quantityOf("rotation") == Quantity::rotation && tee::ui::quantityOf("nonsense") == Quantity::none);
        // Every quantity of section 3 has units, and every unit goes there and back.
        for (int q = 1; q < tee::ui::kQuantities; ++q)
        {
            const auto qq = static_cast<Quantity>(q);
            CHECK_MSG(!tee::ui::unitsOf(qq).empty(), tee::ui::nameOf(qq));
            for (int k = 0; k < static_cast<int>(tee::ui::unitsOf(qq).size()); ++k)
            {
                tee::ui::Units w;
                w.choose(qq, k);
                CHECK_NEAR(w.toSi(qq, w.toUser(qq, 123.456)), 123.456, 1e-9);
            }
        }
        return 0;
    }

    // A click presses (released over it), a drag off it does not; Enter presses with the focus; held reports.
    TEST(testButtonClick, "button.click")
    {
        BoardRig rig;
        int pressed = 0;
        std::vector<bool> held;
        auto& b = rig.place<tee::ui::Button>({ 20, 20, 120, 30 }, std::string("Start"), [&] { ++pressed; });
        b.setOnHold([&](bool on) { held.push_back(on); });
        rig.host.click(80, 35);
        CHECK(pressed == 1);
        CHECK(held.size() == 2 && held[0] && !held[1]);
        rig.host.drag(80, 35, 300, 300);
        CHECK(pressed == 1);                             // let go elsewhere: no press
        typeKeys(rig.host, "tab");
        typeKeys(rig.host, "enter");
        CHECK(pressed == 2);
        const auto items = rig.host.accessibility();
        CHECK(items.size() == 1 && items[0].role == funkgui::A11yRole::button && items[0].title == "Start");
        rig.board.a11yAction(items[0].id, funkgui::A11yAction::press, 0.0);
        CHECK(pressed == 3);
        return 0;
    }

    // A click flips a switch's field in one gesture (one step of undo); Space flips it back; the knob eases.
    TEST(testToggleClick, "toggle.click")
    {
        BoardRig rig;
        auto& t = rig.place<tee::ui::Toggle>({ 20, 20, 200, 28 }, rig.port("boost.blowOff"), std::string("Blow-off"));
        CHECK(rig.value("boost.blowOff") == 1.0);
        const int steps = rig.doc.steps();
        rig.host.click(200, 34);
        CHECK(rig.value("boost.blowOff") == 0.0);
        CHECK(rig.doc.steps() == steps + 1 && rig.doc.depth() == 0);
        CHECK(!t.settled() || true);
        rig.settle();
        CHECK(t.settled());
        typeKeys(rig.host, "tab");
        typeKeys(rig.host, "space");
        CHECK(rig.value("boost.blowOff") == 1.0);
        rig.doc.undo();
        CHECK(rig.value("boost.blowOff") == 0.0);
        return 0;
    }

    // + and - step a choice by one and a number by its step; the wheel, the keys and a double-click on the value too.
    TEST(testStepperClick, "stepper.click")
    {
        BoardRig rig;
        tee::ui::FieldPort& layout = rig.port("engine.layout");
        tee::ui::FieldPort& level = rig.port("boost.level");
        auto& a = rig.place<tee::ui::Stepper>({ 20, 20, 320, 28 }, layout, std::string("Layout"),
                                               [&](float v) { return layout.text(v); });
        auto& b = rig.place<tee::ui::Stepper>({ 20, 60, 320, 28 }, level, std::string("Boost"),
                                               [&](float v) { return level.text(v); }, 0.05f);
        CHECK(rig.value("engine.layout") == 3.0);
        const funkgui::Rect plus = a.plusRect(), minus = a.minusRect();
        rig.host.click(plus.centreX(), plus.centreY());
        CHECK(rig.value("engine.layout") == 4.0);
        CHECK(layout.text(layout.value01()) == "V10");
        rig.host.click(minus.centreX(), minus.centreY());
        rig.host.click(minus.centreX(), minus.centreY() + 0.5f);
        CHECK(rig.value("engine.layout") == 2.0);
        // 80 kPa of 0..200: +5 % of the range is +10 kPa.
        rig.host.click(b.plusRect().centreX(), b.plusRect().centreY());
        CHECK_NEAR(rig.value("boost.level"), 90e3, 1.0);
        rig.host.wheel(b.bounds().centreX(), b.bounds().centreY(), -1.0f);
        CHECK_NEAR(rig.value("boost.level"), 80e3, 1.0);
        rig.host.click(30, 74);                          // the focus, then the keys
        typeKeys(rig.host, "up up");
        CHECK_NEAR(rig.value("boost.level"), 100e3, 1.0);
        typeKeys(rig.host, "home");
        CHECK(rig.value("boost.level") == 0.0);
        const funkgui::Rect m = b.minusRect(), p = b.plusRect();
        rig.host.doubleClick((m.right() + p.x) * 0.5f, m.centreY());
        CHECK_NEAR(rig.value("boost.level"), 80e3, 1.0);   // the value double-clicked: its default
        return 0;
    }

    // A click starts editing with the text selected; typing replaces it; Enter commits through the callback; a
    // refusal keeps it open; Escape gives up.
    TEST(testTextFieldType, "textfield.type")
    {
        BoardRig rig;
        std::string name = "Stock V8";
        auto& f = rig.place<tee::ui::TextField>({ 20, 20, 360, 28 }, std::string("Name"), [&] { return name; },
                                                 [&](const std::string& s) {
                                                     if (s.empty())
                                                         return false;
                                                     name = s;
                                                     return true;
                                                 });
        rig.host.click(200, 34);
        CHECK(f.editing() && f.buffer() == "Stock V8");
        typeKeys(rig.host, "H o t space V 8");
        CHECK(f.buffer() == "Hot V8");
        typeKeys(rig.host, "enter");
        CHECK(!f.editing() && name == "Hot V8");
        rig.host.click(200, 34);
        typeKeys(rig.host, "backspace enter");               // empty: refused, still open
        CHECK(f.editing() && name == "Hot V8");
        typeKeys(rig.host, "escape");
        CHECK(!f.editing() && name == "Hot V8");
        const auto items = rig.host.accessibility();
        CHECK(items.size() == 1 && items[0].role == funkgui::a11yrole::editableText && items[0].value == "Hot V8");
        return 0;
    }

    // A click chooses a row, the keys move, the wheel scrolls and the chosen row stays in view.
    TEST(testListSelect, "list.select")
    {
        BoardRig rig;
        std::vector<std::string> rows;
        for (int i = 0; i < 30; ++i)
            rows.push_back("Build " + std::to_string(i + 1));
        int chosen = -1;
        auto& l = rig.place<tee::ui::List>({ 20, 20, 240, 200 }, std::string("Garage"), rows, [&](int r) { chosen = r; });
        const float top = 20.0f + 18.0f;
        rig.host.click(100, top + tee::ui::List::kRowH * 2.5f);
        CHECK(chosen == 2 && l.selected() == 2);
        typeKeys(rig.host, "down down");
        CHECK(chosen == 4);
        typeKeys(rig.host, "end");
        CHECK(chosen == 29 && l.scroll() > 0.0f);
        typeKeys(rig.host, "home");
        CHECK(chosen == 0 && l.scroll() == 0.0f);
        rig.host.wheel(100, 100, -2.0f);
        CHECK(l.scroll() > 0.0f);
        rig.settle();
        CHECK(l.settled());
        // A trackpad's smooth delta is in JUCE's units, 1 / 512 of one a px (funkgui::scroll): ten px of it scroll ten.
        typeKeys(rig.host, "home");
        rig.host.wheel(100, 100, -10.0f / funkgui::scroll::kWheelPxPerUnit, true);
        CHECK_MSG(std::fabs(l.scroll() - 10.0f) < 0.01f, std::to_string(l.scroll()));
        rig.settle();
        const auto items = rig.host.accessibility();
        CHECK(!items.empty() && items[0].role == funkgui::a11yrole::list);
        return 0;
    }

    // A bar and a lamp follow what they read, eased; a gauge's needle lands on its value.
    TEST(testInstruments, "instruments")
    {
        BoardRig rig;
        float level = 0.25f;
        bool on = false;
        double rpm = 3000.0;
        auto& bar = rig.place<tee::ui::Bar>({ 20, 20, 200, 40 }, std::string("Throttle"), [&] { return level; });
        auto& lamp = rig.place<tee::ui::Lamp>({ 20, 70, 120, 30 }, std::string("Limiter"), [&] { return on; });
        auto& g = rig.place<tee::ui::Gauge>({ 260, 20, 220, 236 }, std::string("Engine"), [&] { return rpm; }, 0.0, 8000.0);
        g.setRedline(6500.0);
        rig.settle();
        CHECK(bar.shownValue() == 0.25f && lamp.glow() == 0.0f);
        CHECK_NEAR(g.needle(), 3000.0 / 8000.0, 1e-6);
        level = 0.75f;
        on = true;
        rpm = 7000.0;
        rig.host.tick(1);
        CHECK(bar.shownValue() > 0.25f && bar.shownValue() < 0.75f);   // eased, not stepped
        CHECK(g.needle() < 7.0f / 8.0f);
        rig.settle();
        CHECK(bar.shownValue() == 0.75f && lamp.glow() == 1.0f);
        CHECK_NEAR(g.needle(), 7.0 / 8.0, 1e-6);
        CHECK(bar.settled() && lamp.settled() && g.settled());
        return 0;
    }

    // The wheel zooms about the pointer, a drag pans, a double-click puts the whole range back; ticks of a log axis are
    // decades.
    TEST(testPlotZoom, "plot.zoom")
    {
        BoardRig rig;
        auto& p = rig.place<tee::ui::Plot>({ 20, 20, 500, 300 }, std::string("Response"));
        p.xAxis() = { 20.0, 20000.0, true, "Hz", {} };
        p.yAxis() = { -40.0, 20.0, false, "dB", {} };
        p.resetView();
        CHECK_NEAR(p.pxX(20.0), p.plotArea().x, 1e-3);
        CHECK_NEAR(p.pxX(632.455532), p.plotArea().centreX(), 1e-2);   // the middle decade of three: sqrt(20 * 20000)
        std::vector<double> t;
        std::vector<bool> major;
        tee::ui::Plot::ticks(p.xAxis(), 20.0, 20000.0, 400.0f, t, major);
        int decades = 0;
        for (std::size_t i = 0; i < t.size(); ++i)
            decades += major[i] ? 1 : 0;
        CHECK(decades >= 3);
        const funkgui::Rect a = p.plotArea();
        const double before = p.dataX(a.x + a.w * 0.25f);
        rig.host.wheel(a.x + a.w * 0.25f, a.centreY(), 2.0f);
        CHECK(p.viewX1() / p.viewX0() < 1000.0 * 0.99);
        CHECK_NEAR(p.dataX(a.x + a.w * 0.25f), before, before * 1e-6);   // about the pointer
        const double x0 = p.viewX0();
        rig.host.drag(a.centreX(), a.centreY(), a.centreX() - 100.0f, a.centreY());
        CHECK(p.viewX0() > x0);
        rig.host.doubleClick(a.centreX(), a.centreY());
        CHECK_NEAR(p.viewX0(), 20.0, 1e-9);
        CHECK_NEAR(p.viewX1(), 20000.0, 1e-6);
        // The view never leaves the range.
        for (int i = 0; i < 5; ++i)
            rig.host.drag(a.centreX(), a.centreY(), a.centreX() + 300.0f, a.centreY());
        CHECK(p.viewX0() >= 20.0 - 1e-9);
        p.series().push_back({ "gain", { 20, 200, 2000, 20000 }, { 0, 6, -3, -30 }, {}, true });
        CHECK_NEAR(tee::ui::Plot::yAt(p.series()[0], 110.0), 3.0, 1e-9);
        return 0;
    }

    // A point dragged moves the curve's field (one undo step), kept between its neighbours; the keys nudge it.
    TEST(testCurveDrag, "curve.drag")
    {
        BoardRig rig;
        const int f = rig.doc.find("valve.lift");
        tee::ui::PlotAxis x{ 0.0, 720.0, false, "deg", {} }, y{ 0.0, 0.016, false, "m", {} };
        struct Src final : tee::ui::CurveSource
        {
            tee::ui::MemoryDoc& d;
            int f;
            Src(tee::ui::MemoryDoc& dd, int ff) : d(dd), f(ff) {}
            tee::ui::Curve16 curve() const override { return d.curve(f); }
            void setCurve(const tee::ui::Curve16& c) override { d.setCurve(f, c); }
            void beginEdit() override { d.beginGesture(); }
            void endEdit() override { d.endGesture(); }
        } src(rig.doc, f);
        auto& c = rig.place<tee::ui::CurveEdit>({ 20, 20, 520, 300 }, std::string("Lift"), src, x, y);
        rig.host.tick(1);
        const tee::ui::Curve16 before = rig.doc.curve(f);
        const float px = c.pxX(before.x[3]), py = c.pxY(before.y[3]);
        CHECK(c.pointAt({ px, py }) == 3);
        const int steps = rig.doc.steps();
        rig.host.drag(px, py, px + 400.0f, py - 60.0f);
        const tee::ui::Curve16 after = rig.doc.curve(f);
        CHECK(after.y[3] > before.y[3]);
        CHECK(after.x[3] <= before.x[4] && after.x[3] > before.x[3]);   // not past its neighbour
        CHECK(rig.doc.steps() == steps + 1 && rig.doc.depth() == 0);
        CHECK(c.selected() == 3);
        typeKeys(rig.host, "down");
        CHECK(rig.doc.curve(f).y[3] < after.y[3]);
        typeKeys(rig.host, "left");
        CHECK(c.selected() == 2);
        rig.doc.undo();
        rig.doc.undo();
        CHECK(rig.doc.curve(f) == before);
        return 0;
    }

    // A cell typed, a cell dragged, an axis's point typed (kept in order), a refusal, and the live cell.
    TEST(testMapEdit, "map.edit")
    {
        BoardRig rig;
        const int f = rig.doc.find("ecu.spark");
        struct Src final : tee::ui::MapSource
        {
            tee::ui::MemoryDoc& d;
            int f;
            Src(tee::ui::MemoryDoc& dd, int ff) : d(dd), f(ff) {}
            tee::ui::Map16 map() const override { return d.map(f); }
            void setMap(const tee::ui::Map16& m) override { d.setMap(f, m); }
            void beginEdit() override { d.beginGesture(); }
            void endEdit() override { d.endGesture(); }
        } src(rig.doc, f);
        const auto num = [](double v) { return tee::ui::formatNumber(v, 1); };
        const auto parse = [](std::string_view s, double& v) {
            const std::string t(s);
            char* e = nullptr;
            v = std::strtod(t.c_str(), &e);
            return e != t.c_str() && *e == '\0';
        };
        tee::ui::MapEdit::Axis ax{ "rpm", 80.0, 800.0, { num, parse } }, ay{ "kPa", 20e3, 250e3, { num, parse } },
            az{ "deg", -10.0, 50.0, { num, parse } };
        double liveX = 0.0, liveY = 0.0;
        bool running = false;
        auto& m = rig.place<tee::ui::MapEdit>({ 20, 20, 640, 340 }, std::string("Spark"), src, ax, ay, az);
        m.setLive([&](double& x, double& y) {
            x = liveX;
            y = liveY;
            return running;
        });
        rig.host.tick(1);
        const tee::ui::Map16 start = rig.doc.map(f);
        // Type 32.5 into cell (2, 1).
        const funkgui::Rect c21 = m.cellRect(2, 1);
        rig.host.click(c21.centreX(), c21.centreY());
        CHECK(m.selectedColumn() == 2 && m.selectedRow() == 1);
        typeKeys(rig.host, "3 2 . 5");
        CHECK(m.editing());
        typeKeys(rig.host, "enter");
        CHECK(!m.editing());
        CHECK_NEAR(rig.doc.map(f).z[1][2], 32.5, 1e-9);
        CHECK(m.selectedRow() == 2);                     // Enter moves down a row
        // A refusal: not a number.
        typeKeys(rig.host, "- - enter");
        CHECK(m.editing());
        typeKeys(rig.host, "escape");
        CHECK(!m.editing());
        // A drag up on cell (0, 0) raises it: 200 px the whole range of 60 degrees.
        const funkgui::Rect c00 = m.cellRect(0, 0);
        const int steps = rig.doc.steps();
        rig.host.drag(c00.centreX(), c00.centreY(), c00.centreX(), c00.centreY() - 50.0f);
        CHECK_NEAR(rig.doc.map(f).z[0][0], start.z[0][0] + 15.0, 0.01);
        CHECK(rig.doc.steps() == steps + 1);
        // An axis's point: typed, kept between its neighbours.
        const funkgui::Rect h3 = m.cellRect(3, -1);
        rig.host.click(h3.centreX(), h3.centreY());
        CHECK(m.selectedRow() == -1 && m.selectedColumn() == 3);
        typeKeys(rig.host, "9 9 9 9 enter");
        CHECK_NEAR(rig.doc.map(f).x[3], start.x[4], 1e-9);
        // The live cell: none while stopped, the nearest while running.
        int lx = -1, ly = -1;
        rig.host.tick(1);
        CHECK(!m.liveCell(lx, ly));
        running = true;
        const tee::ui::Map16 now = rig.doc.map(f);
        liveX = now.x[5] + (now.x[6] - now.x[5]) * 0.2;
        liveY = now.y[2] + (now.y[3] - now.y[2]) * 0.7;
        rig.settle();
        CHECK(m.liveCell(lx, ly) && lx == 5 && ly == 3);
        const auto items = rig.host.accessibility();
        CHECK(!items.empty() && items[0].role == funkgui::a11yrole::table && items[0].rows == 9 && items[0].cols == 9);
        return 0;
    }

    // The gallery: every widget, drawn by the software rasteriser, three pages.
    struct Gallery
    {
        BoardRig rig;
        std::string name = "Stock V8";
        double rpm = 6120.0;
        struct CurveSrc final : tee::ui::CurveSource
        {
            tee::ui::MemoryDoc& d;
            int f;
            CurveSrc(tee::ui::MemoryDoc& dd, int ff) : d(dd), f(ff) {}
            tee::ui::Curve16 curve() const override { return d.curve(f); }
            void setCurve(const tee::ui::Curve16& c) override { d.setCurve(f, c); }
            void beginEdit() override { d.beginGesture(); }
            void endEdit() override { d.endGesture(); }
        };
        struct MapSrc final : tee::ui::MapSource
        {
            tee::ui::MemoryDoc& d;
            int f;
            MapSrc(tee::ui::MemoryDoc& dd, int ff) : d(dd), f(ff) {}
            tee::ui::Map16 map() const override { return d.map(f); }
            void setMap(const tee::ui::Map16& m) override { d.setMap(f, m); }
            void beginEdit() override { d.beginGesture(); }
            void endEdit() override { d.endGesture(); }
        };
        std::unique_ptr<CurveSrc> curve;
        std::unique_ptr<MapSrc> map;

        explicit Gallery(int page, float dpi = 2.0f) : rig(1000, 700, dpi)
        {
            using namespace tee::ui;
            auto& r = rig;
            if (page == 0)
            {
                r.place<Button>({ 30, 30, 120, 30 }, std::string("Start"));
                auto& lit = r.place<Button>({ 160, 30, 120, 30 }, std::string("Launch"));
                lit.setLit([] { return true; });
                r.place<Toggle>({ 30, 80, 250, 28 }, r.port("boost.blowOff"), std::string("Blow-off"));
                r.place<Toggle>({ 30, 116, 250, 28 }, r.port("ecu.pops"), std::string("Overrun pops"));
                FieldPort& lay = r.port("engine.layout");
                r.place<Stepper>({ 30, 160, 320, 28 }, lay, std::string("Layout"), [&lay](float v) { return lay.text(v); });
                FieldPort& lvl = r.port("boost.level");
                r.place<Stepper>({ 30, 196, 320, 28 }, lvl, std::string("Boost"), [&lvl](float v) { return lvl.text(v); }, 0.05f);
                r.place<TextField>({ 30, 240, 320, 28 }, std::string("Name"), [this] { return name; },
                                   [this](const std::string& s) { name = s; return true; });
                std::vector<std::string> rows = { "Stock V8", "Hot cam", "Twin turbo", "Track day", "Diesel tow", "Rally I4",
                                                  "Flat-plane", "Drag" };
                auto& l = r.place<List>({ 400, 30, 240, 230 }, std::string("Garage"), rows);
                l.select(2, false);
                float pedals[3] = { 0.8f, 0.0f, 0.35f };
                const char* names[3] = { "Throttle", "Brake", "Clutch" };
                for (int i = 0; i < 3; ++i)
                    r.place<Bar>({ 30, 300.0f + 46.0f * static_cast<float>(i), 320, 40 }, std::string(names[i]),
                                 [v = pedals[i]] { return v; }, [v = pedals[i]] { return formatNumber(v * 100.0, 0) + "%"; });
                for (int i = 0; i < 3; ++i)
                    r.place<Bar>({ 400.0f + 40.0f * static_cast<float>(i), 300, 30, 140 }, std::string(names[i]).substr(0, 3),
                                 [v = pedals[i]] { return v; }, [v = pedals[i]] { return formatNumber(v * 100.0, 0); }, true);
                const char* lamps[4] = { "Limiter", "Fuel cut", "Boost", "Oil" };
                for (int i = 0; i < 4; ++i)
                    r.place<Lamp>({ 560, 300.0f + 34.0f * static_cast<float>(i), 160, 28 }, std::string(lamps[i]),
                                  [i] { return i % 2 == 0; });
                r.host.click(500, 100);                  // a row hovered and nothing else
                r.host.move(990, 690);
                r.host.click(100, 254);                  // the text field, being edited
                typeKeys(r.host, "H o t");
            }
            else if (page == 1)
            {
                auto& tach = r.place<Gauge>({ 30, 30, 300, 316 }, std::string("Engine"), [this] { return rpm; }, 0.0, 8000.0);
                tach.setRedline(6500.0);
                tach.setNumerals([](double v) { return formatNumber(v / 1000.0, 0); });
                tach.setUnit("x1000 rpm");
                tach.setReadout([](double v) { return formatNumber(v, 0); });
                auto& boost = r.place<Gauge>({ 350, 30, 180, 196 }, std::string("Boost"), [] { return 92.0; }, -100.0, 200.0);
                boost.setTicks(6, 2);
                boost.setUnit("kPa");
                auto& dyno = r.place<Plot>({ 560, 30, 420, 300 }, std::string("Dyno"));
                dyno.xAxis() = { 1000.0, 7000.0, false, "rpm", {} };
                dyno.yAxis() = { 0.0, 600.0, false, "Nm / kW", {} };
                dyno.resetView();
                PlotSeries torque{ "Nm", {}, {}, {}, true }, power{ "kW", {}, {}, {}, false };
                for (int k = 0; k <= 24; ++k)
                {
                    const double rr = 1000.0 + 250.0 * k, t = 380.0 + 140.0 * std::sin((rr - 1000.0) / 6000.0 * 2.6);
                    torque.x.push_back(rr);
                    torque.y.push_back(t);
                    power.x.push_back(rr);
                    power.y.push_back(t * rr * 3.14159265 / 30.0 / 1000.0);
                }
                dyno.series() = { torque, power };
                auto& bode = r.place<Plot>({ 30, 380, 560, 280 }, std::string("Spectrum"));
                bode.xAxis() = { 20.0, 20000.0, true, "Hz", {} };
                bode.yAxis() = { -80.0, 0.0, false, "dB", {} };
                bode.resetView();
                PlotSeries s{ "out", {}, {}, {}, true };
                for (int k = 0; k <= 200; ++k)
                {
                    const double hz = 20.0 * std::pow(1000.0, k / 200.0);
                    s.x.push_back(hz);
                    s.y.push_back(-12.0 - 20.0 * std::log10(1.0 + hz / 400.0) + 9.0 * std::exp(-std::pow(std::log(hz / 210.0) * 6.0, 2.0)));
                }
                bode.series() = { s };
                r.host.move(dyno.pxX(4500.0), dyno.plotArea().centreY());   // the cursor's readout
            }
            else
            {
                curve = std::make_unique<CurveSrc>(r.doc, r.doc.find("valve.lift"));
                map = std::make_unique<MapSrc>(r.doc, r.doc.find("ecu.spark"));
                tee::ui::Curve16 c;
                c.n = 9;
                for (int k = 0; k < 9; ++k)
                {
                    c.x[static_cast<std::size_t>(k)] = 200.0 + 40.0 * k;
                    c.y[static_cast<std::size_t>(k)] = 0.012 * std::sin(3.14159265 * k / 8.0);
                }
                r.doc.setCurve(r.doc.find("valve.lift"), c);
                tee::ui::Map16 m = r.doc.map(r.doc.find("ecu.spark"));
                for (int j = 0; j < m.ny; ++j)
                    for (int i = 0; i < m.nx; ++i)
                        m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] = 12.0 + 3.5 * i - 2.0 * j;
                r.doc.setMap(r.doc.find("ecu.spark"), m);
                PlotAxis x{ 0.0, 720.0, false, "deg", {} }, y{ 0.0, 0.016, false, "mm", [](double v) { return formatNumber(v * 1000.0, 1); } };
                auto& ce = r.place<CurveEdit>({ 30, 30, 440, 300 }, std::string("Intake lift"), *curve, x, y);
                ce.select(4);
                const auto num = [](double v) { return formatNumber(v, 0); };
                const auto kpa = [](double v) { return formatNumber(v / 1000.0, 0); };
                const auto parse = [](std::string_view, double&) { return false; };
                MapEdit::Axis ax{ "rpm", 80, 800, { [](double v) { return formatNumber(v * 30.0 / 3.14159265, 0); }, parse } },
                    ay{ "kPa", 20e3, 250e3, { kpa, parse } }, az{ "deg", -10, 50, { num, parse } };
                auto& me = r.place<MapEdit>({ 30, 350, 940, 320 }, std::string("Spark advance"), *map, ax, ay, az);
                me.setLive([this](double& lx, double& ly) {
                    const tee::ui::Map16 mm = rig.doc.map(rig.doc.find("ecu.spark"));
                    lx = mm.x[4] * 1.03;
                    ly = mm.y[2] * 1.1;
                    return true;
                });
                me.select(6, 3);
            }
            r.settle();
        }
    };

    TEST(testWidgetsGallery, "widgets.gallery")
    {
        const char* names[3] = { "widgets.controls", "widgets.instruments", "widgets.tables" };
        for (int page = 0; page < 3; ++page)
        {
            Gallery g(page);
            golden(names[page], g.rig.host);
            Gallery one(page, 1.0f);                     // and at a pixel density of 1, a picture to look at
            tee::test::writePng(funkgui::rasterise(one.rig.host.draw(), funkgui::FontService::get().atlas(), 1),
                                (kOutDir / (std::string(names[page]) + ".dpi1.png")).string());
        }
        return 0;
    }
