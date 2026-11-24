// ui/views/garage/GarageWorkspace.cpp: see GarageWorkspace.h.
#include "views/garage/GarageWorkspace.h"

#include "views/garage/DemoLibrary.h"
#include "views/garage/ServicesLibrary.h"
#include "views/scope/Layout.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <limits>

namespace tee::ui
{
    namespace
    {
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Rect;
        using garage::BuildSummary;
        using garage::Part;

        constexpr double kPi = 3.14159265358979323846;
        constexpr double kRpmPerRad = 30.0 / kPi;
        constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
        constexpr float kGap = 10.0f, kBarH = 44.0f, kCardW = 232.0f, kCardH = 164.0f, kButtonH = 26.0f;
        constexpr uint32_t kCardA11y = 0x00F00000u;   // the cards' accessibility ids (below the Board's items')
        constexpr funkgui::TextStyle kName{ 14.0f, 0.2f, 0.0f, false };

        // A button's width for its words (10 px spaced capitals, about 8 px a letter).
        float wordsWidth(const std::string& s) { return 24.0f + 8.0f * static_cast<float>(s.size()); }

        std::string lower(std::string s)
        {
            for (char& c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }

        // Text cut to a width, with "..." when cut.
        std::string fit(Canvas& c, const std::string& s, const funkgui::TextStyle& st, float w)
        {
            if (c.textWidth(s.c_str(), st) <= w)
                return s;
            std::string t = s;
            while (!t.empty() && c.textWidth((t + "...").c_str(), st) > w)
                t.pop_back();
            while (!t.empty() && t.back() == ' ')
                t.pop_back();
            return t + "...";
        }

        // Words wrapped to a width, at most `maxLines` (the last cut with "...").
        std::vector<std::string> wrap(Canvas& c, const std::string& text, const funkgui::TextStyle& st, float w, int maxLines)
        {
            std::vector<std::string> lines;
            std::string line;
            std::size_t at = 0;
            while (at < text.size())
            {
                std::size_t sp = text.find(' ', at);
                if (sp == std::string::npos)
                    sp = text.size();
                const std::string word = text.substr(at, sp - at);
                const std::string tried = line.empty() ? word : line + " " + word;
                if (!line.empty() && c.textWidth(tried.c_str(), st) > w)
                {
                    lines.push_back(line);
                    line = word;
                }
                else
                    line = tried;
                at = sp + 1;
            }
            if (!line.empty())
                lines.push_back(line);
            if (static_cast<int>(lines.size()) > maxLines && maxLines > 0)
            {
                lines.resize(static_cast<std::size_t>(maxLines));
                lines.back() = fit(c, lines.back() + " ...", st, w);
                if (lines.back().size() < 3 || lines.back().compare(lines.back().size() - 3, 3, "...") != 0)
                    lines.back() += "...";
            }
            return lines;
        }

        double peakPower(const BuildSummary& b) { return b.curve.peakPower().value; }
    }

    GarageWorkspace::GarageWorkspace(WorkspaceContext& c, garage::Library* library) : Workspace(c), lib_(library)
    {
        if (lib_ == nullptr)
        {
            // The frame's own services when they list builds (the application today); else made-up builds.
            if (garage::ServicesLibrary::usable(ctx_))
                own_ = std::make_unique<garage::ServicesLibrary>(ctx_);
            else
            {
                own_ = std::make_unique<garage::DemoLibrary>(ctx_.doc);
                demo_ = true;
            }
            lib_ = own_.get();
        }
        search_ = &make<TextField>("Search", [this] { return query_; },
                                   [this](const std::string& s) { setSearch(s); return true; }, 40);
        const char* sorts[] = { "NAME", "POWER", "DISPLACEMENT", "LAYOUT" };
        for (int i = 0; i < 4; ++i)
        {
            sorts_[static_cast<std::size_t>(i)] = &make<Button>(sorts[i], [this, i] { setSort(static_cast<Sort>(i)); });
            sorts_[static_cast<std::size_t>(i)]->setLit([this, i] { return sort_ == static_cast<Sort>(i); });
        }
        const char* filters[] = { "ALL", "BUILDS", "ENGINES", "CARS", "TUNES" };
        for (int i = 0; i < 5; ++i)
        {
            filters_[static_cast<std::size_t>(i)] = &make<Button>(filters[i], [this, i] { setFilter(static_cast<Filter>(i)); });
            filters_[static_cast<std::size_t>(i)]->setLit([this, i] { return filter_ == static_cast<Filter>(i); });
        }
        back_ = &make<Button>("BACK TO BUILDS", [this] { compareWith({}); });

        std::string error;
        loadWhole_ = &make<Button>("LOAD BUILD", [this] {
            std::string e;
            const BuildSummary* b = find(chosen_);
            const Part part = b != nullptr ? b->part : Part::whole;
            act(part == Part::engine ? "Put the engine in" : part == Part::car ? "Put the engine in the car"
                : part == Part::tune ? "Tried the tune" : "Loaded",
                lib_->load(chosen_, part, e), e);
        });
        loadEngine_ = &make<Button>("ITS ENGINE", [this] {
            std::string e;
            act("Put its engine in this car:", lib_->load(chosen_, Part::engine, e), e);
        });
        loadCar_ = &make<Button>("ITS CAR", [this] {
            std::string e;
            act("Put this engine in its car:", lib_->load(chosen_, Part::car, e), e);
        });
        loadTune_ = &make<Button>("ITS TUNE", [this] {
            std::string e;
            act("Tried its tune on this engine:", lib_->load(chosen_, Part::tune, e), e);
        });
        compare_ = &make<Button>("COMPARE", [this] { compareWith(chosen_); });
        duplicate_ = &make<Button>("DUPLICATE", [this] {
            std::string e, id;
            const bool ok = lib_->duplicate(chosen_, id, e);
            act("Duplicated", ok, e);
            if (ok)
                choose(id);
        });
        delete_ = &make<Button>("DELETE", [this] {
            if (armed_ != chosen_)
            {
                armed_ = chosen_;   // the first press asks; the second deletes
                relayout();
                return;
            }
            std::string e;
            const std::string name = find(chosen_) != nullptr ? find(chosen_)->name : std::string();
            const bool ok = lib_->remove(chosen_, e);
            armed_.clear();
            act(("Deleted " + name).c_str(), ok, e);
            if (ok)
                chosen_.clear();
            relayout();
        });
        keep_ = &make<Button>("KEEP IT", [this] {
            armed_.clear();
            relayout();
        });
        save_ = &make<Button>("SAVE", [this] {
            std::string e;
            act("Saved", lib_->save(e), e);
        });
        revert_ = &make<Button>("REVERT", [this] {
            std::string e;
            act("Reverted to the saved", lib_->revert(e), e);
        });
        saveAs_ = &make<TextField>("Save as", [] { return std::string(); },
                                   [this](const std::string& name) {
                                       std::string e;
                                       const bool ok = lib_->saveAs(name, e);
                                       act("Saved as", ok, e);
                                       if (ok && currentBuild() != nullptr)
                                           choose(currentBuild()->id);
                                       return ok;
                                   },
                                   40);
        notes_ = &make<TextField>("Notes", [this] { return lib_->notes(); },
                                  [this](const std::string& text) {
                                      lib_->setNotes(text);
                                      return true;
                                  },
                                  400);
        (void) error;

        curves_ = &make<ScopePlot>("Torque and power");
        using Style = ScopePlot::Style;
        using Ink = ScopePlot::Ink;
        curves_->setTraces({ { "TQ", Style{ Ink::value, false, false, 2.5f } },
                             { "PWR", Style{ Ink::value, false, false, 1.25f } },
                             { "TQ", Style{ Ink::selected, true, false, 2.5f } },
                             { "PWR", Style{ Ink::selected, true, false, 1.25f } } });
        curves_->xAxis().label = "rpm";
        curves_->xAxis().format = [](double v) { return look::num(v); };
        curves_->setOverlay({}, [this](Canvas& cv, const look::Palette& p, const ScopePlot& pl) {
            if (pl.cursorOn())
                return;
            // The legend, under the title: which line is which, in words as well as ink and dash.
            const BuildSummary* a = currentBuild();
            const BuildSummary* b = find(compareId_);
            const Rect area = pl.plotArea();
            float x = area.x + 8.0f;
            const float y = area.y + 8.0f;
            const std::string an = a != nullptr ? a->name : std::string("this build");
            const std::string bn = b != nullptr ? b->name : std::string("-");
            cv.rrect(area.x + 2.0f, area.y + 2.0f, std::min(area.w - 4.0f, 520.0f), 40.0f, 4.0f, p.panel);
            x += legendEntry(cv, p, x, y, an + " TORQUE", p.value, 2.5f, false);
            legendEntry(cv, p, x, y, "POWER", p.value, 1.25f, false);
            x = area.x + 8.0f;
            x += legendEntry(cv, p, x, y + 16.0f, bn + " TORQUE", p.selected, 2.5f, true);
            legendEntry(cv, p, x, y + 16.0f, "POWER", p.selected, 1.25f, true);
        });
        chosen_ = currentBuild() != nullptr ? currentBuild()->id : std::string();
        refilter();
    }

    GarageWorkspace::~GarageWorkspace() = default;

    std::unique_ptr<Workspace> makeGarageWorkspace(WorkspaceContext& c) { return std::make_unique<GarageWorkspace>(c); }

    // ---- state ------------------------------------------------------------------------------------------------------
    const BuildSummary* GarageWorkspace::find(const std::string& id) const
    {
        for (const BuildSummary& b : lib_->builds())
            if (b.id == id)
                return &b;
        return nullptr;
    }

    const BuildSummary* GarageWorkspace::currentBuild() const
    {
        for (const BuildSummary& b : lib_->builds())
            if (b.current)
                return &b;
        return nullptr;
    }

    void GarageWorkspace::choose(const std::string& id)
    {
        if (id != chosen_)
        {
            chosen_ = id;
            armed_.clear();
            message_.clear();
            relayout();
        }
    }

    void GarageWorkspace::setSearch(const std::string& text)
    {
        query_ = text;
        scroll_ = 0.0f;
        refilter();
    }

    void GarageWorkspace::setSort(Sort s)
    {
        sort_ = s;
        refilter();
    }

    void GarageWorkspace::setFilter(Filter f)
    {
        filter_ = f;
        scroll_ = 0.0f;
        refilter();
    }

    void GarageWorkspace::compareWith(const std::string& id)
    {
        comparing_ = !id.empty() && find(id) != nullptr;
        compareId_ = comparing_ ? id : std::string();
        diffScroll_ = 0.0f;
        refreshCompare();
        relayout();
    }

    void GarageWorkspace::act(const char* what, bool ok, const std::string& error)
    {
        const BuildSummary* b = find(chosen_);
        if (ok)
            message_ = std::string(what) + (b != nullptr ? " " + b->name : std::string());
        else
            message_ = error.empty() ? std::string("That did not work.") : error;
        messageBad_ = !ok;
        refilter();
        relayout();
    }

    void GarageWorkspace::refilter()
    {
        libSeen_ = lib_->revision();
        const auto& all = lib_->builds();
        shown_.clear();
        const std::string q = lower(query_);
        for (int i = 0; i < static_cast<int>(all.size()); ++i)
        {
            const BuildSummary& b = all[static_cast<std::size_t>(i)];
            const bool part = filter_ == Filter::all || (filter_ == Filter::whole && b.part == Part::whole)
                           || (filter_ == Filter::engine && b.part == Part::engine) || (filter_ == Filter::car && b.part == Part::car)
                           || (filter_ == Filter::tune && b.part == Part::tune);
            if (!part)
                continue;
            if (!q.empty())
            {
                const std::string hay = lower(b.name + " " + garage::sentence(b, ctx_.doc) + " " + b.notes);
                if (hay.find(q) == std::string::npos)
                    continue;
            }
            shown_.push_back(i);
        }
        const auto key = [&](int i) -> double {
            const BuildSummary& b = all[static_cast<std::size_t>(i)];
            switch (sort_)
            {
                case Sort::power: return -peakPower(b);
                case Sort::displacement: return -garage::displacement(b, ctx_.doc);
                case Sort::layout:
                {
                    const double n = layout::cylindersOf(garage::choiceOf(b, ctx_.doc, "engine.layout"));
                    return n > 0 ? n : kNaN;
                }
                case Sort::name: break;
            }
            return 0.0;
        };
        std::stable_sort(shown_.begin(), shown_.end(), [&](int a, int b) {
            if (sort_ == Sort::name)
                return lower(all[static_cast<std::size_t>(a)].name) < lower(all[static_cast<std::size_t>(b)].name);
            const double ka = key(a), kb = key(b);
            if (std::isfinite(ka) != std::isfinite(kb))
                return std::isfinite(ka);   // what has no number goes last
            if (ka != kb && std::isfinite(ka))
                return ka < kb;
            return lower(all[static_cast<std::size_t>(a)].name) < lower(all[static_cast<std::size_t>(b)].name);
        });
        if (find(chosen_) == nullptr)
            chosen_ = currentBuild() != nullptr ? currentBuild()->id : std::string();
        if (comparing_)
            refreshCompare();
        scroll_ = std::clamp(scroll_, 0.0f, std::max(0.0f, contentHeight() - grid_.h));
        ++revision_;
    }

    void GarageWorkspace::refreshCompare()
    {
        diffs_.clear();
        const BuildSummary* a = currentBuild();
        const BuildSummary* b = find(compareId_);
        for (auto& s : curves_->series())
        {
            s.x.clear();
            s.y.clear();
        }
        if (a == nullptr || b == nullptr)
            return;
        diffs_ = garage::differences(*a, *b, ctx_.doc, ctx_.units);
        // The curves in the user's units: torque and power on one axis, each its own number (N m and kW read alike).
        const Units& u = ctx_.units;
        double top = 1.0, rpm1 = 1000.0;
        const BuildSummary* both[2] = { a, b };
        for (int k = 0; k < 2; ++k)
        {
            const DynoTrace& t = both[k]->curve;
            PlotSeries& tq = curves_->series()[static_cast<std::size_t>(k * 2)];
            PlotSeries& pw = curves_->series()[static_cast<std::size_t>(k * 2 + 1)];
            for (std::size_t i = 0; i < t.speed.size(); ++i)
            {
                const double rpm = t.speed[i] * kRpmPerRad;
                const double q = u.toUser(Quantity::torque, t.torque[i]), w = u.toUser(Quantity::power, t.power[i]);
                tq.x.push_back(rpm);
                tq.y.push_back(q);
                pw.x.push_back(rpm);
                pw.y.push_back(w);
                top = std::max({ top, q, w });
                rpm1 = std::max(rpm1, rpm);
            }
        }
        curves_->yAxis().label = std::string(u.unit(Quantity::torque).name) + " / " + u.unit(Quantity::power).name;
        const double step = top > 500.0 ? 100.0 : 50.0;
        curves_->setAxes(0.0, std::ceil(rpm1 / 1000.0) * 1000.0, false, 0.0, std::ceil(top * 1.2 / step) * step, false);
        curves_->setNote(curves_->hasData() ? "" : "no curves for these yet");
        curves_->touched();
    }

    // ---- time -------------------------------------------------------------------------------------------------------
    void GarageWorkspace::step(float dt)
    {
        if (own_ != nullptr)
            own_->tick(dt);
        if (lib_->revision() != libSeen_)
        {
            refilter();
            relayout();
        }
        // Live search: the field's text as it is typed.
        if (search_->editing() && search_->buffer() != query_)
        {
            query_ = search_->buffer();
            scroll_ = 0.0f;
            refilter();
        }
        // The scroll eases to where it was asked.
        if (scrollShown_ != scroll_)
        {
            scrollShown_ += (scroll_ - scrollShown_) * std::min(1.0f, dt * 16.0f);
            if (std::fabs(scroll_ - scrollShown_) < 0.5f)
                scrollShown_ = scroll_;
            ++revision_;
        }
    }

    bool GarageWorkspace::moving() const { return scrollShown_ != scroll_ || wantsFullRate(); }

    std::uint64_t GarageWorkspace::drawKey() const
    {
        return DrawKey{}
            .mix(revision_)
            .mix(lib_->revision())
            .mix(ctx_.doc.revision())
            .mix(ctx_.units.revision())
            .mix(scrollShown_)
            .mix(chosen_)
            .mix(message_)
            .h;
    }

    // ---- layout -----------------------------------------------------------------------------------------------------
    int GarageWorkspace::columns() const noexcept
    {
        return std::max(1, static_cast<int>((grid_.w + kGap) / (kCardW + kGap)));
    }

    float GarageWorkspace::contentHeight() const noexcept
    {
        const int cols = columns();
        const int rows = (static_cast<int>(shown_.size()) + cols - 1) / cols;
        return rows * (kCardH + kGap) - kGap;
    }

    Rect GarageWorkspace::cardRect(int k) const noexcept
    {
        if (comparing_ || k < 0 || k >= static_cast<int>(shown_.size()))
            return {};
        const int cols = columns();
        const float w = (grid_.w - (cols - 1) * kGap) / cols;
        const int row = k / cols, col = k % cols;
        return { grid_.x + col * (w + kGap), grid_.y + row * (kCardH + kGap) - scrollShown_, w, kCardH };
    }

    void GarageWorkspace::relayout()
    {
        if (!stage_.isEmpty())
            layout(stage_);
        bump();
        ++revision_;
    }

    void GarageWorkspace::layout(const Rect& s)
    {
        bar_ = { s.x + kGap, s.y + kGap, s.w - 2 * kGap, kBarH };
        const float by = bar_.y + (kBarH - kButtonH) * 0.5f;
        // The bar: search, sort, show (the cards); or the way back (compare).
        float x = bar_.x + look::kPad;
        search_->setPlace({ x, by, 230.0f, kButtonH }, !comparing_);
        x += 230.0f + 22.0f + 40.0f;   // "SORT" is drawn in the gap
        for (Button* b : sorts_)
        {
            const float w = wordsWidth(b->title());
            b->setPlace({ x, by, w, kButtonH }, !comparing_);
            x += w + 4.0f;
        }
        x += 18.0f + 42.0f;            // "SHOW"
        for (Button* b : filters_)
        {
            const float w = wordsWidth(b->title());
            b->setPlace({ x, by, w, kButtonH }, !comparing_);
            x += w + 4.0f;
        }
        back_->setPlace({ bar_.x + look::kPad, by, wordsWidth(back_->title()), kButtonH }, comparing_);

        const float top = bar_.bottom() + kGap;
        const float dw = comparing_ ? 0.0f : std::clamp(s.w * 0.28f, 280.0f, 360.0f);
        detail_ = comparing_ ? Rect{} : Rect{ s.right() - kGap - dw, top, dw, s.bottom() - kGap - top };
        grid_ = { s.x + kGap, top, s.w - 2 * kGap - (comparing_ ? 0.0f : dw + kGap), s.bottom() - kGap - top };
        if (comparing_)
        {
            const float pw = std::floor(grid_.w * 0.56f);
            curves_->setPlace({ grid_.x + 10.0f, grid_.y + 10.0f, pw - 20.0f, grid_.h - 18.0f }, true);
            diffRect_ = { grid_.x + pw + kGap, grid_.y, grid_.w - pw - kGap, grid_.h };
        }
        else
        {
            curves_->setPlace({}, false);
            diffRect_ = {};
        }

        // The card's column: its actions from the bottom up.
        const BuildSummary* b = find(chosen_);
        const bool current = b != nullptr && b->current;
        const bool whole = b != nullptr && b->part == Part::whole;
        const float ix = detail_.x + look::kPad, iw = detail_.w - 2 * look::kPad;
        const float half = (iw - 6.0f) * 0.5f, third = (iw - 12.0f) / 3.0f;
        float y = detail_.bottom() - look::kPad - kButtonH;
        const auto row = [&](float h = kButtonH) {
            const float at = y;
            y -= h + 6.0f;
            return at;
        };
        const bool shown = !comparing_ && b != nullptr;
        for (Button* w : { loadWhole_, loadEngine_, loadCar_, loadTune_, compare_, duplicate_, delete_, keep_, save_, revert_ })
            w->setPlace({}, false);
        saveAs_->setPlace({}, false);
        notes_->setPlace({}, false);
        if (shown)
        {
            // Delete (yours, not the current), with its confirmation.
            if (b->user && !current)
            {
                const float ry = row();
                if (armed_ == chosen_)
                {
                    delete_->setTitle("CONFIRM DELETE");
                    delete_->setPlace({ ix, ry, half, kButtonH }, true);
                    keep_->setPlace({ ix + half + 6.0f, ry, half, kButtonH }, true);
                }
                else
                {
                    delete_->setTitle("DELETE");
                    delete_->setPlace({ ix, ry, iw, kButtonH }, true);
                }
            }
            if (current)
            {
                const float ry = row();
                duplicate_->setPlace({ ix, ry, iw, kButtonH }, true);
                notes_->setPlace({ ix, row(), iw, kButtonH }, true);
                saveAs_->setPlace({ ix, row(), iw, kButtonH }, true);
                const float sy = row();
                save_->setPlace({ ix, sy, half, kButtonH }, b->user);
                revert_->setPlace({ ix + (b->user ? half + 6.0f : 0.0f), sy, b->user ? half : iw, kButtonH }, true);
            }
            else
            {
                const float ry = row();
                compare_->setPlace({ ix, ry, half, kButtonH }, true);
                duplicate_->setPlace({ ix + half + 6.0f, ry, half, kButtonH }, true);
                if (whole)
                {
                    const float py = row();
                    loadEngine_->setPlace({ ix, py, third, kButtonH }, true);
                    loadCar_->setPlace({ ix + third + 6.0f, py, third, kButtonH }, true);
                    loadTune_->setPlace({ ix + 2 * (third + 6.0f), py, third, kButtonH }, true);
                }
                loadWhole_->setTitle(b->part == Part::engine ? "PUT THIS ENGINE IN"
                                     : b->part == Part::car  ? "USE THIS CAR"
                                     : b->part == Part::tune ? "TRY THIS TUNE"
                                                             : "LOAD THIS BUILD");
                loadWhole_->setPlace({ ix, row(), iw, kButtonH }, true);
            }
        }
        actions_ = { detail_.x, y + kButtonH + 6.0f, detail_.w, detail_.bottom() - (y + kButtonH + 6.0f) };
    }

    // ---- input ------------------------------------------------------------------------------------------------------
    void GarageWorkspace::pointerDown(const funkgui::PointerEvent& e)
    {
        if (!comparing_ && grid_.contains({ e.x, e.y }))
        {
            for (int k = 0; k < static_cast<int>(shown_.size()); ++k)
            {
                const Rect r = cardRect(k);
                if (r.contains({ e.x, e.y }) && e.y >= grid_.y && e.y <= grid_.bottom())
                {
                    choose(lib_->builds()[static_cast<std::size_t>(shown_[static_cast<std::size_t>(k)])].id);
                    return;
                }
            }
            return;
        }
        Board::pointerDown(e);
    }

    bool GarageWorkspace::wheel(const funkgui::WheelEvent& e)
    {
        if (!comparing_ && grid_.contains({ e.x, e.y }))
        {
            const float most = std::max(0.0f, contentHeight() - grid_.h);
            scroll_ = std::clamp(scroll_ - e.dy * (e.smooth ? 1.0f : 48.0f), 0.0f, most);
            ++revision_;
            return true;
        }
        if (comparing_ && diffRect_.contains({ e.x, e.y }))
        {
            diffScroll_ = std::max(0.0f, diffScroll_ - e.dy * (e.smooth ? 1.0f : 40.0f));
            ++revision_;
            return true;
        }
        return Board::wheel(e);
    }

    void GarageWorkspace::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        Board::accessibility(out);
        // Each card: a button whose words are the build's name, part and sentence.
        for (int k = 0; k < static_cast<int>(shown_.size()); ++k)
        {
            const BuildSummary& b = lib_->builds()[static_cast<std::size_t>(shown_[static_cast<std::size_t>(k)])];
            funkgui::A11yItem it;
            it.id = kCardA11y + static_cast<uint32_t>(k);
            it.role = funkgui::A11yRole::listItem;
            it.bounds = cardRect(k);
            it.title = b.name;
            it.description = std::string(garage::partWord(b.part)) + ". " + garage::sentence(b, ctx_.doc)
                           + (b.current ? ". The current build" : "");
            it.checkable = true;
            it.checked = b.id == chosen_;
            it.visible = !it.bounds.isEmpty() && it.bounds.bottom() > grid_.y && it.bounds.y < grid_.bottom();
            out.push_back(std::move(it));
        }
    }

    // ---- drawing ----------------------------------------------------------------------------------------------------
    void GarageWorkspace::drawCard(Canvas& c, const look::Palette& p, const BuildSummary& b, const Rect& r) const
    {
        const bool chosen = b.id == chosen_;
        c.rrect(r.x, r.y, r.w, r.h, look::kRadius, b.current ? p.raised : p.panel, 1.0f, p.edge);
        if (chosen)
            look::selectRing(c, p, r);
        const float x = r.x + look::kPad, w = r.w - 2 * look::kPad;
        float y = r.y + 10.0f;
        // The current build: a bar down its left edge and the word.
        if (b.current)
            c.rrect(r.x + 1.0f, r.y + 8.0f, 3.0f, r.h - 16.0f, 1.5f, p.selected);
        // What part it is: a word in a frame at the top right.
        const std::string part = garage::partWord(b.part);
        const float pw = c.textWidth(part.c_str(), look::kCaps) + 12.0f;
        c.rrect(r.right() - look::kPad - pw, y - 2.0f, pw, 16.0f, 3.0f, kClear, 1.0f, b.part == Part::whole ? p.dim : p.label);
        c.text(part.c_str(), r.right() - look::kPad - pw * 0.5f, y + 1.0f, look::kCaps, p.label, funkgui::Align::centre);
        c.text(fit(c, b.name, kName, w - pw - 8.0f).c_str(), x, y, kName, p.value);
        y += 22.0f;
        const std::string tag = b.current ? (lib_->unsaved() ? "CURRENT  * UNSAVED" : "CURRENT") : b.user ? "YOURS" : "SHIPPED";
        c.text(tag.c_str(), x, y, look::kCaps, b.current ? p.selected : p.dim);
        y += 16.0f;
        // The engine in a sentence, or what a car or a tune sets.
        std::string line = garage::sentence(b, ctx_.doc);
        if (line.empty())
        {
            int n = 0;
            for (const garage::FieldValue& v : b.values)
            {
                const int f = ctx_.doc.find(v.path);
                if (f < 0 || ctx_.doc.info(f).quantity == Quantity::none)
                    continue;
                line += (n++ ? ", " : "") + lower(ctx_.doc.info(f).label) + " "
                      + garage::valueText(&v, ctx_.doc, v.path, ctx_.units);
                if (n == 2)
                    break;
            }
        }
        c.text(fit(c, line.empty() ? std::string("-") : line, look::kBody, w).c_str(), x, y, look::kBody, p.text);
        y += 22.0f;
        // Peak torque and power, with their speeds.
        const DynoTrace::Peak tq = b.curve.peakTorque(), pw2 = b.curve.peakPower();
        const Units& u = ctx_.units;
        const auto peak = [&](float at, const char* label, Quantity q, const DynoTrace::Peak& pk) {
            look::readout(c, p, at, y, label, std::isfinite(pk.value) ? u.number(q, pk.value, 0) : "-", u.unit(q).name,
                          look::kNumberS);
            const std::string sp = std::isfinite(pk.speed) ? "at " + look::num(pk.speed * kRpmPerRad) + " rpm" : "";
            c.text(sp.c_str(), at, y + 32.0f, look::kUnit, p.label);
        };
        if (b.part == Part::whole || b.part == Part::engine)
        {
            peak(x, "TORQUE", Quantity::torque, tq);
            peak(x + w * 0.5f, "POWER", Quantity::power, pw2);
        }
        // A small power curve, on a scale common to every card (so cards compare): 0 to 8000 rpm, 0 to the most power.
        const Rect sl{ x, r.bottom() - 34.0f, w, 24.0f };
        double most = 1.0;
        for (const BuildSummary& o : lib_->builds())
            most = std::max(most, std::isfinite(peakPower(o)) ? peakPower(o) : 0.0);
        c.hairlineH(sl.x, c.snapY(sl.bottom()), sl.w, p.grid);
        if (!b.curve.empty())
        {
            std::vector<float> xs, ys;
            for (std::size_t i = 0; i < b.curve.speed.size(); ++i)
            {
                const double rpm = b.curve.speed[i] * kRpmPerRad;
                xs.push_back(sl.x + static_cast<float>(std::clamp(rpm / 8000.0, 0.0, 1.0)) * sl.w);
                ys.push_back(sl.bottom() - static_cast<float>(std::clamp(b.curve.power[i] / most, 0.0, 1.0)) * sl.h);
            }
            c.areaStrip(xs.data(), static_cast<int>(xs.size()), ys.data(), nullptr, sl.bottom(), p.over(p.value, 0.1f));
            c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), 1.5f, b.current ? p.selected : p.text);
        }
        else if (b.part == Part::whole || b.part == Part::engine)
            c.text("curve not made yet", sl.x, sl.y + 8.0f, look::kUnit, p.dim);
    }

    void GarageWorkspace::drawDetail(Canvas& c, const look::Palette& p) const
    {
        const BuildSummary* b = find(chosen_);
        const Rect in = look::region(c, p, detail_, b != nullptr && b->current ? "THE CURRENT BUILD" : "THE CARD CHOSEN");
        if (b == nullptr)
        {
            c.text("Choose a card.", in.x, in.y, look::kBody, p.label);
            return;
        }
        float y = in.y + 2.0f;
        for (const std::string& l : wrap(c, b->name, look::kNumberM, in.w, 2))
        {
            c.text(l.c_str(), in.x, y, look::kNumberM, p.value);
            y += 24.0f;
        }
        c.text((std::string(garage::partWord(b->part)) + (b->user ? "   YOURS" : "   SHIPPED")).c_str(), in.x, y, look::kCaps,
               p.label);
        y += 18.0f;
        const std::string sent = garage::sentence(*b, ctx_.doc);
        if (!sent.empty())
        {
            for (const std::string& l : wrap(c, sent, look::kBody, in.w, 2))
            {
                c.text(l.c_str(), in.x, y, look::kBody, p.text);
                y += 16.0f;
            }
        }
        y += 8.0f;
        // The notes, as many lines as there is room for above the actions.
        c.text("NOTES", in.x, y, look::kCaps, p.label);
        y += 16.0f;
        const float room = actions_.y - 34.0f - y;
        const int lines = std::max(1, static_cast<int>(room / 16.0f));
        const std::string notes = b->current ? lib_->notes() : b->notes;
        for (const std::string& l : wrap(c, notes.empty() ? std::string("None.") : notes, look::kBody, in.w, lines))
        {
            c.text(l.c_str(), in.x, y, look::kBody, notes.empty() ? p.dim : p.text);
            y += 16.0f;
        }
        // What the last action did, over the actions.
        if (!message_.empty())
        {
            const float my = actions_.y - 22.0f;
            const Col ink = messageBad_ ? p.warning : p.ok;
            c.rrect(in.x, my + 2.0f, 3.0f, 12.0f, 1.5f, ink);
            c.text(fit(c, message_, look::kBody, in.w - 10.0f).c_str(), in.x + 9.0f, my, look::kBody, messageBad_ ? p.warning : p.text);
        }
    }

    void GarageWorkspace::drawDiffs(Canvas& c, const look::Palette& p) const
    {
        const BuildSummary* a = currentBuild();
        const BuildSummary* b = find(compareId_);
        const Rect in = look::region(c, p, diffRect_, "WHAT DIFFERS");
        const float colA = in.x + in.w * 0.46f, colB = in.x + in.w * 0.73f;
        const Canvas::ClipScope clip(c, { in.x - 4.0f, in.y, in.w + 8.0f, in.h });
        float y = in.y - diffScroll_;
        // The heads: each build's name over its column, in its ink and line.
        c.text(fit(c, a != nullptr ? a->name : std::string("-"), look::kCaps, colB - colA - 8.0f).c_str(), colA, y, look::kCaps,
               p.value);
        c.text(fit(c, b != nullptr ? b->name : std::string("-"), look::kCaps, in.right() - colB).c_str(), colB, y, look::kCaps,
               p.selected);
        c.segment(colA, y + 15.0f, colA + 30.0f, y + 15.0f, 2.0f, p.value);
        const float xs[2] = { colB, colB + 30.0f }, ys[2] = { y + 15.0f, y + 15.0f };
        dashedPolyline(c, xs, ys, 2, 2.0f, p.selected, 6.0f, 3.0f);
        y += 26.0f;
        if (diffs_.empty())
        {
            c.text("They are the same build.", in.x, y, look::kBody, p.label);
            return;
        }
        std::string section;
        for (const garage::Difference& d : diffs_)
        {
            if (d.section != section)
            {
                section = d.section;
                y += 6.0f;
                c.text(garage::sectionWord(section).c_str(), in.x, y, look::kCaps, p.label);
                c.hairlineH(in.x, c.snapY(y + 15.0f), in.w, p.grid);
                y += 20.0f;
            }
            if (y > in.y - 20.0f && y < in.bottom())
            {
                c.text(fit(c, d.label, look::kBody, colA - in.x - 8.0f).c_str(), in.x, y, look::kBody, p.text);
                c.text(fit(c, d.a, look::kBody, colB - colA - 8.0f).c_str(), colA, y, look::kBody, p.value);
                c.text(fit(c, d.b, look::kBody, in.right() - colB).c_str(), colB, y, look::kBody, p.value);
            }
            y += 17.0f;
        }
    }

    void GarageWorkspace::drawStage(Canvas& c, const funkgui::Theme& th) const
    {
        const look::Palette p = look::Palette::of(th);
        c.rrect(bar_.x, bar_.y, bar_.w, bar_.h, look::kRadius, p.panel, 1.0f, p.edge);
        const float cy = c.capCentreTop(bar_.centreY(), look::kCaps);
        if (comparing_)
        {
            const BuildSummary* a = currentBuild();
            const BuildSummary* b = find(compareId_);
            const float x = back_->bounds().right() + 20.0f;
            const std::string t = "COMPARING   " + (a != nullptr ? a->name : std::string("-")) + "   WITH   "
                                + (b != nullptr ? b->name : std::string("-"));
            c.text(t.c_str(), x, cy, look::kCaps, p.text);
            c.rrect(grid_.x, grid_.y, std::floor(grid_.w * 0.56f), grid_.h, look::kRadius, p.panel, 1.0f, p.edge);
            drawDiffs(c, p);
            return;
        }
        c.text("SORT", sorts_[0]->bounds().x - 40.0f, cy, look::kCaps, p.label);
        c.text("SHOW", filters_[0]->bounds().x - 42.0f, cy, look::kCaps, p.label);
        // The cards.
        {
            const Canvas::ClipScope clip(c, { grid_.x - 6.0f, grid_.y - 6.0f, grid_.w + 12.0f, grid_.h + 12.0f });
            for (int k = 0; k < static_cast<int>(shown_.size()); ++k)
            {
                const Rect r = cardRect(k);
                if (r.bottom() < grid_.y - 6.0f || r.y > grid_.bottom() + 6.0f)
                    continue;
                drawCard(c, p, lib_->builds()[static_cast<std::size_t>(shown_[static_cast<std::size_t>(k)])], r);
            }
        }
        if (shown_.empty())
            c.text(query_.empty() ? "No builds of that part." : ("Nothing matches \"" + query_ + "\".").c_str(), grid_.x + 4.0f,
                   grid_.y + 8.0f, look::kBody, p.label);
        // A scroll bar when there are more cards than room.
        const float total = contentHeight();
        if (total > grid_.h + 1.0f)
        {
            const float h = std::max(30.0f, grid_.h * grid_.h / total);
            const float t = scrollShown_ / (total - grid_.h);
            c.rrect(grid_.right() + 3.0f, grid_.y + t * (grid_.h - h), 3.0f, h, 1.5f, p.dim);
        }
        drawDetail(c, p);
        if (demo_)
        {
            // Said plainly: these are not the user's files.
            const float x = bar_.right() - look::kPad;
            if (x - c.textWidth("DEMO LIBRARY", look::kCaps) > filters_[4]->bounds().right() + 12.0f)
                c.text("DEMO LIBRARY", x, cy, look::kCaps, p.warning, funkgui::Align::right);
            else
                c.text("DEMO LIBRARY", detail_.right() - look::kPad, detail_.y + 9.0f, look::kCaps, p.warning,
                       funkgui::Align::right);
        }
    }
}
