// tests/ui/RefTests.inc: the reference's own panel (sim/ref/panel.gui and its shaders, as the build embedded them)
// over a made-up Player that has the reference's 32 dials (read from sim/ref/program.fm's `param` lines: their names,
// types, ranges and defaults) and its show* fields as telemetry. Included by Tests.cpp. Skipped (77) in a build made
// before sim/ref/ was in the tree.
class RefMock final : public tee::Player
{
public:
    RefMock()
    {
        // param int engine [[fminus::range(0, 16)]] [[fminus::default(12)]];
        std::stringstream in(readText(kRepoDir / "sim" / "ref" / "program.fm"));
        for (std::string line; std::getline(in, line);)
        {
            char type[16] = {}, name[32] = {};
            double lo = 0, hi = 1, def = 0;
            if (std::sscanf(line.c_str(), " param %15s %31s [[fminus::range(%lf, %lf)]] [[fminus::default(%lf)]]", type, name, &lo, &hi,
                            &def) != 5)
                continue;
            tee::ParamInfo p;
            p.id = p.label = name;
            p.kind = std::string(type) == "int" ? tee::ParamInfo::Kind::integer : tee::ParamInfo::Kind::real;
            p.min = lo;
            p.max = hi;
            p.def = def;
            params_.push_back(p);
            values_.push_back(def);
        }
        telemetry_ = { { "showRpm", 2400.0 },   { "showRed", 7000.0 }, { "showGear", 2.0 },     { "showKmh", 62.0 },
                       { "showThr", 0.35 },     { "showMap", -0.4 },   { "showLamps", 18.0 },   { "showPedals", 0.0 },
                       { "showCycle", 0.25 },   { "showAir", 0.0 },    { "showPipes", 505000.0 }, { "showBoost", 0.0 },
                       { "car.gear", 2.0 },     { "engine.state", 2.0 } };
    }

    int count() const override { return static_cast<int>(params_.size()); }
    const tee::ParamInfo& info(int i) const override { return params_[static_cast<std::size_t>(i)]; }
    double get(int i) const override { return values_[static_cast<std::size_t>(i)]; }
    void set(int i, double value) override { values_[static_cast<std::size_t>(i)] = value; }
    int findParam(std::string_view id) const override
    {
        for (std::size_t i = 0; i < params_.size(); ++i)
            if (params_[i].id == id)
                return static_cast<int>(i);
        return -1;
    }
    double plain(std::string_view id) const { return get(findParam(id)); }
    int findValue(std::string_view name) const override
    {
        for (std::size_t i = 0; i < telemetry_.size(); ++i)
            if (telemetry_[i].first == name)
                return static_cast<int>(i);
        return -1;
    }
    double value(int h) const override
    {
        return h >= 0 && h < static_cast<int>(telemetry_.size()) ? telemetry_[static_cast<std::size_t>(h)].second : 0.0;
    }
    void setTelemetry(std::string_view name, double v) { telemetry_[static_cast<std::size_t>(findValue(name))].second = v; }
    float outputPeak() const override { return 0.3f; }
    void setControls(const tee::ControlsFrame&) override { ++frames; }
    void prepare(double, int) override {}
    void process(float*, float*, int) noexcept override {}

    int frames = 0;

private:
    std::vector<tee::ParamInfo> params_;
    std::vector<double> values_;
    std::vector<std::pair<std::string, double>> telemetry_;
};

int refPanel()
{
    const Sources files = refPanelFiles();
    if (files.empty())
    {
        std::printf("  sim/ref/ was not in the tree when this was built: the reference's panel is not tested\n");
        return kSkip;
    }
    CHECK(files.size() >= 4 && files[0].name == "panel.gui");
    for (const SourceFile& f : files)                    // embedded byte for byte
        CHECK_MSG(f.text == readText(kRepoDir / "sim" / "ref" / f.name), f.name + " differs from sim/ref/");

    RefMock player;
    CHECK(player.count() == 32);
    Services app;
    RefPanel panel(files, player, app.make(), {});
    funkgui::HeadlessHost host(panel, 0, 2.0f);
    host.tick(3);
    // The file is taken whole: every dial has its control, every field its telemetry, every shader its element.
    CHECK_MSG(panel.notes().empty(), panel.notes());
    CHECK(panel.spec().shaders.size() == 3);
    const GuiLayout& natural = panel.view().places();
    CHECK(natural.maxScrollX == 0.0f && natural.maxScrollY == 0.0f);
    std::printf("  natural size %d x %d\n", panel.naturalWidth(), panel.naturalHeight());

    const auto click = [&](const funkgui::Rect& r) {
        host.click(r.centreX(), r.centreY());
        host.tick(2);
    };
    const auto control = [&](const char* id) -> GuiControlItem& {
        return static_cast<GuiControlItem&>(*panel.items()[static_cast<std::size_t>(panel.view().controlItem(panel.slotOf(id)))]);
    };
    // Each page's controls are there and work: a drag turns a knob and the parameter moves.
    const std::map<std::string, std::vector<const char*>> pages = {
        { "ENGINE", { "size", "borestroke", "compression", "timing", "cam", "inertia", "idle", "redline", "load" } },
        { "AIR & EXHAUST", { "turbo", "turbosize", "bov", "headers", "pipe", "xpipe", "muffler", "pops", "rough" } },
        { "SOUND & DRIVE", { "distance", "room", "intake", "mech", "level", "hold", "ambient", "mass" } },
    };
    int page = 0;
    for (const auto& [title, knobs] : pages)
    {
        const funkgui::Rect tab = panel.tabRect(title);
        CHECK_MSG(tab.w > 0.0f, "no tab " + title);
        click(tab);
        for (const char* id : knobs)
        {
            const funkgui::Rect r = panel.controlRect(id);
            CHECK_MSG(r.w > 0.0f, std::string(id) + " has no place on " + title);
            CHECK_MSG(r.x >= 0.0f && r.right() <= static_cast<float>(panel.width()) && r.bottom() <= static_cast<float>(panel.height()),
                      std::string(id) + " is outside the panel");
            const tee::ParamInfo& info = player.info(player.findParam(id));
            const double before = player.plain(id);
            const bool up = before < 0.5 * (info.min + info.max);
            host.drag(r.centreX(), r.centreY(), r.centreX(), r.centreY() + (up ? -30.0f : 30.0f));
            CHECK_MSG(up ? player.plain(id) > before : player.plain(id) < before, std::string(id) + " did not move");
            host.doubleClick(r.centreX(), r.centreY());  // back to its default
            CHECK_NEAR(player.plain(id), info.def, 1e-3 * (info.max - info.min));
        }
        host.tick(20);
        golden("ref.page" + std::to_string(++page), host);
    }
    // A menu picks, buttons choose, a switch flips.
    click(panel.tabRect("ENGINE"));
    click(control("engine").menuBox());
    CHECK(host.pendingMenu() != nullptr && host.pendingMenu()->items.size() == 17);
    CHECK(host.chooseMenuItem("V10"));
    CHECK_NEAR(player.plain("engine"), 14.0, 0.0);
    click(control("fuel").menuBox());
    CHECK(host.chooseMenuItem("DIESEL"));
    CHECK_NEAR(player.plain("fuel"), 1.0, 0.0);
    click(panel.tabRect("AIR & EXHAUST"));
    click(control("aspiration").menuBox());
    CHECK(host.chooseMenuItem("TWIN TURBO"));
    CHECK_NEAR(player.plain("aspiration"), 2.0, 0.0);
    click(panel.tabRect("SOUND & DRIVE"));
    const auto button = [&](const char* id, int k) {
        GuiControlItem& item = control(id);
        const funkgui::Rect c = item.cell();
        for (float y = c.y + 1.0f; y < c.bottom(); y += 2.0f)
            for (float x = c.x + 1.0f; x < c.right(); x += 2.0f)
                if (item.buttonAt({ x, y }) == k)
                {
                    host.click(x + 1.0f, y + 1.0f);
                    return true;
                }
        return false;
    };
    CHECK(button("view", 4));
    CHECK_NEAR(player.plain("view"), 4.0, 0.0);
    CHECK(button("drive", 1));
    CHECK_NEAR(player.plain("drive"), 1.0, 0.0);
    CHECK_NEAR(player.plain("starter"), 1.0, 0.0);
    click(panel.controlRect("starter"));
    CHECK_NEAR(player.plain("starter"), 0.0, 0.0);

    // The three shaders are laid out and fed: a dial's turn, a field's value, the output's level.
    int shown = 0;
    for (const int i : panel.view().shaders())
    {
        const auto& item = static_cast<const GuiShaderItem&>(*panel.items()[static_cast<std::size_t>(i)]);
        shown += item.shown() ? 1 : 0;
    }
    CHECK(shown == 3);
    const auto shaderOf = [&](const char* file) -> const GuiShaderItem* {
        const auto& nodes = panel.spec().gui.nodes;
        for (const int i : panel.view().shaders())
        {
            const auto& item = static_cast<const GuiShaderItem&>(*panel.items()[static_cast<std::size_t>(i)]);
            for (std::size_t n = 0; n < nodes.size(); ++n)
                if (nodes[n].kind == GuiKind::shader && nodes[n].file == file && item.shown()
                    && std::fabs(item.cell().x - panel.view().places().rects[n].x) < 0.5f
                    && std::fabs(item.cell().y - panel.view().places().rects[n].y) < 0.5f)
                    return &item;
        }
        return nullptr;
    };
    const GuiShaderItem* header = shaderOf("header.sc");
    const GuiShaderItem* cluster = shaderOf("cluster.sc");
    const GuiShaderItem* layout = shaderOf("layout.sc");
    CHECK(header != nullptr && cluster != nullptr && layout != nullptr);
    if (header != nullptr && cluster != nullptr && layout != nullptr)
    {
        player.setTelemetry("showRpm", 5100.0);
        host.tick(2);
        CHECK_NEAR(cluster->inputs()[0], 5100.0, 0.5);   // shared->showRpm
        CHECK_NEAR(cluster->inputs()[1], 7000.0, 0.5);   // shared->showRed
        CHECK_NEAR(cluster->inputs()[6], 18.0, 0.01);    // shared->showLamps
        CHECK_NEAR(header->inputs()[0], 14.0 / 16.0, 1e-4);   // engine, its dial's turn: V10 of 0..16
        CHECK_NEAR(header->inputs()[5], 0.3, 1e-4);      // out
        CHECK_NEAR(layout->inputs()[1], 0.25, 1e-4);     // shared->showCycle
        CHECK_NEAR(header->cell().h, 46.0, 0.5);
        CHECK(cluster->cell().w > 300.0f && cluster->cell().h > 300.0f);
    }
#if TEE_UI_GPU
    CHECK(header != nullptr && header->program() != 0 && cluster->program() != 0 && layout->program() != 0);
#endif
    // The strip stands in the keyboard's cell, on every page.
    const std::vector<funkgui::Rect> keys = panel.keyboardRects();
    CHECK(keys.size() == 1);
    CHECK(!keys.empty() && panel.stripRect().y == keys[0].y && panel.stripRect().h == keys[0].h);
    CHECK(panel.gearText() == "2");
    host.press(panel.startRect().centreX(), panel.startRect().centreY());
    CHECK(app.asked.startHeld);
    host.release();
    CHECK(player.frames == 0);

    // A small window scrolls; a large one is filled.
    host.resize(640, 400);
    host.tick(3);
    CHECK(panel.view().places().maxScrollY > 0.0f);
    golden("ref.small", host);
    host.resize(1920, 1200);
    host.tick(3);
    CHECK(panel.view().places().maxScrollX == 0.0f && panel.view().places().maxScrollY == 0.0f);
    CHECK_NEAR(panel.view().places().contentW, 1920.0, 1.0);
    golden("ref.large", host);
    return 0;
}
