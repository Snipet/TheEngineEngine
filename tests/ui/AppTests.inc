// tests/ui/AppTests.inc: J2's application frame (card J, items 9 and 10) over the made-up Spec: every field that is not
// hidden reachable, set through the frame with undo, the start button, the garage and the units, sizes, a golden of
// each screen and the contact sheet (out/ui/sheet.png). Included by Tests.cpp.

    // The frame over a made-up document and player, with services that do what the shell's would.
    struct AppRig
    {
        tee::ui::MemoryDoc doc{ tee::test::mockFields() };
        tee::ui::Units units;
        MockPlayer player;
        std::string name = "Stock V8";
        std::vector<std::string> saved = { "Hot cam", "Stock V8", "Twin turbo" };
        std::string loaded;
        tee::PanelControls lastControls{};
        int controlCalls = 0;
        tee::ControlsFrame pedals{};
        std::unique_ptr<tee::ui::AppFrame> frame;
        std::unique_ptr<funkgui::HeadlessHost> host;

        explicit AppRig(int w = 1280, int h = 800)
        {
            const double kRpm = 3.14159265358979 / 30.0;
            player.setTelemetry("engine.state", 2.0);
            player.setTelemetry("engine.speed", 3450.0 * kRpm);
            player.setTelemetry("engine.redline", 6500.0 * kRpm);
            player.setTelemetry("engine.throttle", 0.42);
            player.setTelemetry("engine.manifold", 78e3);
            player.setTelemetry("engine.coolant", 361.0);
            player.setTelemetry("engine.oilPressure", 310e3);
            player.setTelemetry("engine.flags", 0.0);
            player.setTelemetry("boost.pressure", 45e3);
            player.setTelemetry("car.speed", 22.5);
            player.setTelemetry("car.gear", 3.0);
            pedals.throttle = 0.42f;
            pedals.clutch = 0.1f;
            tee::ui::FrameServices s;
            s.app.panelControls = [this](const tee::PanelControls& c) {
                lastControls = c;
                ++controlCalls;
            };
            s.app.controls = [this] { return pedals; };
            s.app.status = [] { return std::string("made-up player   null audio"); };
            s.buildName = [this] { return name; };
            s.rename = [this](const std::string& n) {
                name = n;
                return true;
            };
            s.undo = [this] { doc.undo(); };
            s.redo = [this] { doc.redo(); };
            s.builds = [this] { return saved; };
            s.load = [this](const std::string& n, std::string&) {
                loaded = n;
                name = n;
                return true;
            };
            s.save = [this](const std::string& n, std::string& error) {
                if (n.empty())
                    return error = "no name", false;
                if (std::find(saved.begin(), saved.end(), n) == saved.end())
                    saved.push_back(n);
                return true;
            };
            frame = std::make_unique<tee::ui::AppFrame>(doc, units, player, std::move(s), tee::ui::defaultScreens(), w, h);
            host = std::make_unique<funkgui::HeadlessHost>(*frame, 0, 2.0f);
            host->tick(40);
        }
        void showScreen(const std::string& screenName)
        {
            for (int s = 0; s < frame->screenCount(); ++s)
                if (frame->screenDef(s).name == screenName)
                {
                    const funkgui::Rect r = frame->railRect(s);
                    host->click(r.centreX(), r.centreY());
                    host->tick(40);
                    return;
                }
            failAt(__FILE__, __LINE__, "no screen " + screenName);
        }
        double value(const char* path) const { return doc.get(doc.find(path)); }
    };

    // The nine screens, the rail chooses them, and every field that is not hidden has its control or widget on a page.
    TEST(testAppFrame, "app.frame")
    {
        AppRig rig;
        tee::ui::AppFrame& f = *rig.frame;
        CHECK(f.screenCount() == 9);
        const char* names[9] = { "Build", "Valvetrain", "Boost", "Exhaust", "Tune", "Dyno", "Drive", "Scope", "Garage" };
        for (int s = 0; s < 9; ++s)
            CHECK(f.screenDef(s).name == names[s]);
        for (int s = 0; s < 9; ++s)
        {
            rig.host->click(f.railRect(s).centreX(), f.railRect(s).centreY());
            CHECK(f.screen() == s);
            if (!f.screenDef(s).garage)
            {
                CHECK(f.shownPage() != nullptr);
                for (int p = 0; p < f.pageCount(s); ++p)
                    CHECK_MSG(f.pageOf(s, p)->notes().empty(), f.screenDef(s).name + ": " + f.pageOf(s, p)->notes());
            }
            else
                CHECK(f.shownPage() == nullptr);
        }
        // Every field a person sets is somewhere.
        const std::vector<int> shown = f.fieldsShown();
        for (int i = 0; i < rig.doc.count(); ++i)
        {
            const bool there = std::find(shown.begin(), shown.end(), i) != shown.end();
            const tee::ui::FieldInfo& info = rig.doc.info(i);
            CHECK_MSG(there != info.hidden, info.path + (info.hidden ? " is hidden and shown" : " is on no page"));
        }
        // Build has its sections as pages, with a strip of tabs: ENGINE, FUEL.
        rig.showScreen("Build");
        CHECK(f.pageCount(0) == 2 && f.pageTitle(0, 0) == "ENGINE" && f.pageTitle(0, 1) == "FUEL");
        rig.host->click(f.pageTabRect(1).centreX(), f.pageTabRect(1).centreY());
        CHECK(f.page() == 1 && f.shownPage()->controlRect("fuel.kind").w > 0.0f);
        // The content sits inside the frame's bars.
        const funkgui::Rect r = f.shownPage()->controlRect("fuel.kind");
        CHECK(r.x >= tee::ui::AppFrame::kRailW && r.y >= tee::ui::AppFrame::kTopH + tee::ui::AppFrame::kPagesH);
        return 0;
    }

    // A knob on a screen turns its field; Cmd-Z through the frame takes the drag back as one step.
    TEST(testAppSet, "app.set")
    {
        AppRig rig;
        rig.showScreen("Boost");
        tee::ui::FieldPage* page = rig.frame->shownPage();
        CHECK(page != nullptr);
        const funkgui::Rect r = page->controlRect("boost.level");
        CHECK(r.w > 0.0f);
        rig.host->drag(r.centreX(), r.centreY(), r.centreX(), r.centreY() - 80.0f, 12);
        CHECK(rig.value("boost.level") > 80e3);
        typeKeys(*rig.host, "cmd+z");
        CHECK_NEAR(rig.value("boost.level"), 80e3, 1e-6);
        typeKeys(*rig.host, "cmd+shift+z");
        CHECK(rig.value("boost.level") > 80e3);
        // A menu on a generated page: the host's menu, its choice written as one step.
        const funkgui::Rect m = page->controlRect("boost.kind");
        rig.host->click(m.centreX(), m.centreY());
        const funkgui::MenuRequest* menu = rig.host->pendingMenu();
        CHECK(menu != nullptr);
        std::string labels;
        for (const funkgui::MenuItem& it : menu != nullptr ? menu->items : std::vector<funkgui::MenuItem>{})
            labels += it.label + ";";
        CHECK_MSG(rig.host->chooseMenuItem("TWIN TURBO"), labels);
        rig.host->tick(2);
        CHECK(rig.value("boost.kind") == 2.0);
        // A map on the Tune screen.
        rig.showScreen("Tune");
        page = rig.frame->shownPage();
        bool found = false;
        for (const GuiPanel::WidgetAt& w : page->widgets())
            if (page->spec().gui.nodes[static_cast<std::size_t>(w.node)].file == "ecu.spark")
            {
                auto& map = static_cast<tee::ui::MapEdit&>(*page->items()[static_cast<std::size_t>(w.item)]);
                found = map.shown();
                const funkgui::Rect c = map.cellRect(0, 0);
                rig.host->click(c.centreX(), c.centreY());
                typeKeys(*rig.host, "2 5 enter");
                CHECK_NEAR(rig.doc.map(rig.doc.find("ecu.spark")).z[0][0], 25.0, 1e-9);
            }
        CHECK(found);
        return 0;
    }

    // START held is a PanelControls with startHeld, let go it is not; the pedals and the gear show the frame's.
    TEST(testAppStart, "app.start")
    {
        AppRig rig;
        const funkgui::Rect s = rig.frame->startRect();
        rig.host->press(s.centreX(), s.centreY());
        CHECK(rig.lastControls.startHeld);
        rig.host->release();
        CHECK(!rig.lastControls.startHeld && rig.controlCalls == 2);
        const auto items = rig.host->accessibility();
        bool gear = false, throttle = false;
        for (const funkgui::A11yItem& it : items)
        {
            gear = gear || (it.title == "Gear" && it.value == "3");
            throttle = throttle || (it.title == "Throttle" && std::fabs(it.v - 0.42) < 1e-6);
        }
        CHECK(gear && throttle);
        return 0;
    }

    // The garage: the builds listed, one loaded, the name changed and saved; a unit chosen makes the pages again in it.
    TEST(testAppGarage, "app.garage")
    {
        AppRig rig;
        rig.showScreen("Garage");
        tee::ui::AppFrame& f = *rig.frame;
        CHECK(f.garageList().rows().size() == 3 && f.garageList().shown());
        const funkgui::Rect l = f.garageList().bounds();
        rig.host->click(l.centreX(), l.y + 18.0f + tee::ui::List::kRowH * 2.5f);
        rig.host->click(f.garageLoad().bounds().centreX(), f.garageLoad().bounds().centreY());
        CHECK(rig.loaded == "Twin turbo" && f.garageMessage() == "Loaded Twin turbo.");
        const funkgui::Rect n = f.garageName().bounds();
        rig.host->click(n.right() - 20.0f, n.centreY());
        typeKeys(*rig.host, "M y space b u i l d enter");
        CHECK(rig.name == "My build");
        rig.host->click(f.garageSave().bounds().centreX(), f.garageSave().bounds().centreY());
        CHECK(rig.saved.size() == 4 && f.garageList().rows().size() == 4);
        // Pressure in psi: the Boost screen's knob says so.
        rig.units.choose(tee::ui::Quantity::pressure, "psi");
        rig.host->tick(2);
        rig.showScreen("Boost");
        tee::ui::FieldPage* page = f.shownPage();
        const int k = page->params().findParam("boost.level");
        CHECK(k >= 0 && page->params().info(k).unit == "psi");
        CHECK_NEAR(page->params().get(k), 80e3 / 6894.757293168, 1e-6);
        return 0;
    }

    // Small and large, the frame lays out again; nothing of it leaves the window.
    TEST(testAppResize, "app.resize")
    {
        AppRig rig;
        for (const auto& [w, h] : { std::pair{ 900, 560 }, std::pair{ 1920, 1200 } })
        {
            CHECK(rig.host->resize(w, h));
            rig.host->tick(5);
            CHECK(rig.frame->width() == w && rig.frame->height() == h);
            CHECK(rig.frame->startRect().right() <= static_cast<float>(w));
            CHECK(rig.frame->railRect(8).bottom() <= static_cast<float>(h) - tee::ui::AppFrame::kBottomH);
            CHECK(rig.frame->shownPage()->width() == w);
        }
        return 0;
    }

    // A golden of every screen at 1280 x 800, and the contact sheet: every screen at 1024 x 640 and at 1600 x 1000,
    // scaled to tiles, three screens a row (each screen's two sizes side by side), for a person to look at in one glance.
    TEST(testAppSheet, "app.sheet")
    {
        const std::vector<std::pair<int, int>> sizes = { { 1024, 640 }, { 1600, 1000 } };
        constexpr int kTileW = 480, kTileH = 300, kGap = 12, kPerRow = 3;
        const int screens = 9;
        const int sheetW = kPerRow * (2 * kTileW + kGap) + (kPerRow + 1) * kGap * 2;
        const int rows = (screens + kPerRow - 1) / kPerRow;
        const int sheetH = rows * (kTileH + kGap * 2) + kGap * 2;
        funkgui::Image sheet;
        sheet.w = sheetW;
        sheet.h = sheetH;
        sheet.rgba.assign(static_cast<std::size_t>(sheetW) * static_cast<std::size_t>(sheetH) * 4, 0);
        for (std::size_t i = 0; i < sheet.rgba.size(); i += 4)
            sheet.rgba[i] = sheet.rgba[i + 1] = sheet.rgba[i + 2] = 8, sheet.rgba[i + 3] = 255;
        {
            AppRig rig(1280, 800);
            for (int s = 0; s < screens; ++s)
            {
                rig.showScreen(rig.frame->screenDef(s).name);
                std::string name = "app." + rig.frame->screenDef(s).name;
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                golden(name, *rig.host);
            }
        }
        for (std::size_t z = 0; z < sizes.size(); ++z)
        {
            AppRig rig(sizes[z].first, sizes[z].second);
            for (int s = 0; s < screens; ++s)
            {
                rig.showScreen(rig.frame->screenDef(s).name);
                const funkgui::Image img = funkgui::rasterise(rig.host->draw(), funkgui::FontService::get().atlas(), 1);
                // Box-filtered into its tile.
                const int col = s % kPerRow, row = s / kPerRow;
                const int ox = kGap * 2 + col * (2 * kTileW + kGap + kGap * 2) + static_cast<int>(z) * (kTileW + kGap);
                const int oy = kGap * 2 + row * (kTileH + kGap * 2);
                const double sx = static_cast<double>(img.w) / kTileW, sy = static_cast<double>(img.h) / kTileH;
                for (int y = 0; y < kTileH; ++y)
                    for (int x = 0; x < kTileW; ++x)
                    {
                        const int x0 = static_cast<int>(x * sx), x1 = std::max(x0 + 1, static_cast<int>((x + 1) * sx));
                        const int y0 = static_cast<int>(y * sy), y1 = std::max(y0 + 1, static_cast<int>((y + 1) * sy));
                        unsigned sum[3] = { 0, 0, 0 }, n = 0;
                        for (int yy = y0; yy < y1 && yy < img.h; ++yy)
                            for (int xx = x0; xx < x1 && xx < img.w; ++xx)
                            {
                                const std::size_t k = (static_cast<std::size_t>(yy) * static_cast<std::size_t>(img.w) + static_cast<std::size_t>(xx)) * 4;
                                for (int c = 0; c < 3; ++c)
                                    sum[c] += img.rgba[k + static_cast<std::size_t>(c)];
                                ++n;
                            }
                        const std::size_t d = (static_cast<std::size_t>(oy + y) * static_cast<std::size_t>(sheetW) + static_cast<std::size_t>(ox + x)) * 4;
                        for (int c = 0; c < 3; ++c)
                            sheet.rgba[d + static_cast<std::size_t>(c)] = static_cast<uint8_t>(n > 0 ? sum[c] / n : 0);
                        sheet.rgba[d + 3] = 255;
                    }
            }
        }
        std::error_code ec;
        std::filesystem::create_directories(kOutDir, ec);
        const std::string path = (kOutDir / "sheet.png").string();
        CHECK_MSG(tee::test::writePng(sheet, path), path);
        std::printf("  contact sheet: %s (software rasteriser; each row: three screens, each at 1024 x 640 then 1600 x 1000)\n",
                    path.c_str());
        return 0;
    }

    // `scope out` and `spectrum out` read the source's newest samples (TelemetrySource::outputTail) when the panel's
    // owner gives none: silent, the frame stays; a tone in the tail, the two displays move.
    TEST(testDisplayTail, "display.tail")
    {
        // Two panels made and ticked alike, one with a tone in its source's tail: only the tail differs between them.
        const auto frameWith = [](double hz) {
            Rig rig;
            rig.player.tone = hz;
            rig.page("DISPLAYS");
            rig.host.tick(60);
            return funkgui::fingerprint(rig.host.draw()).geometry;
        };
        const uint64_t silent = frameWith(0.0), low = frameWith(440.0), high = frameWith(3000.0);
        CHECK(frameWith(0.0) == silent);                 // deterministic
        CHECK(low != silent && high != silent && high != low);
        // The reference's own panel, as makeMainPanel makes it, reads it too.
        const auto mainWith = [](double hz) {
            MockPlayer player;
            player.tone = hz;
            std::unique_ptr<funkgui::Panel> main = tee::ui::makeMainPanel(player, tee::AppServices{});
            funkgui::HeadlessHost host(*main, 0, 2.0f);
            host.tick(60);
            return funkgui::fingerprint(host.draw()).geometry;
        };
        CHECK(mainWith(440.0) != mainWith(0.0));
        return 0;
    }
