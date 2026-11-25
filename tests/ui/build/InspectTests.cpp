// tests/ui/build/InspectTests.cpp: J2's inspector designs (25 Nov; docs/ui-kit.md "Inspector designs") over the real
// Spec, in the real frame. Each design is laid over the frame's inspector column by a test-only Panel (Overlay): the
// frame draws itself as it ships, then the design covers its column and takes the input there. Nothing under ui/app
// changes, so the shipped inspector is untouched until the owner picks one.
//
//   build.inspect.list.pictures   the workspaces' inspector pages and what each holds (a probe; prints only)
//   build.inspect.pictures        the pictures for the owner, under out/ui/inspect/ (named below)
//   build.inspect.rows            design 1 over the real Spec: typing refuses out of range and keeps in range (one
//                                 undo step), the filter finds across the workspace, CHANGED, folding
//   build.inspect.dials           design 2: a dial turned and a value typed, each one undo step; headings stay put
//   build.inspect.card            design 3: a part chosen on the stage brings its card beside it; the column lists all
#include "build/BuildKit.h"

#include "Png.h"
#include "kit/inspect/FieldInspector.h"
#include "kit/inspect/Outline.h"
#include "kit/widgets/Board.h"

#include <funkgui/canvas/SoftRaster.h>
#include <funkgui/text/FontService.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>

namespace
{
    using tee::test::build::Rig;
    using tee::ui::FieldInspector;
    using tee::ui::OutlineEntry;

    enum class Design { rows, dials, picture };
    const char* nameOf(Design d) { return d == Design::rows ? "rows" : d == Design::dials ? "dials" : "picture"; }

    // The frame as it ships, with a design over its inspector column.
    class Overlay final : public tee::ui::Board
    {
    public:
        Overlay(tee::ui::AppFrame& frame, tee::ui::FieldDoc& doc, const tee::ui::Units& units, Design design, int w, int h)
            : Board(w, h, 1100, 700), frame_(frame), doc_(doc), design_(design)
        {
            column_ = &make<FieldInspector>(doc, units, design == Design::rows    ? FieldInspector::Style::rows
                                                        : design == Design::dials ? FieldInspector::Style::dials
                                                                                  : FieldInspector::Style::list);
            if (design == Design::picture)
            {
                card_ = &make<FieldInspector>(doc, units, FieldInspector::Style::list);
                card_->setShown(false);
            }
        }

        FieldInspector& column() const { return *column_; }
        FieldInspector* card() const { return card_; }
        // Where a part was chosen on the stage (the card's anchor), as the frame would know it from the press.
        void chose(funkgui::Point at) { anchor_ = at; seen_ = {}; sync(); }
        void sync()
        {
            const std::string key = std::to_string(frame_.shown()) + "|" + std::to_string(frame_.inspectorPageIndex()) + "|"
                                  + frame_.selection().path();
            if (key == seen_)
                return;
            seen_ = key;
            column_->setPlace(columnRect(), true);           // the tabs of this workspace may wrap to more rows
            if (design_ == Design::picture)
            {
                // The column: everything the workspace holds, with nothing selected; the card: the selection's page.
                if (wsSeen_ != frame_.shown())
                {
                    wsSeen_ = frame_.shown();
                    const std::string sel = frame_.selection().path();
                    frame_.select({});
                    frame_.tick(0.0f);
                    tee::ui::Outline all;
                    for (int p = 0; p < frame_.inspectorPageCount(); ++p)
                    {
                        frame_.showInspectorPage(p);
                        if (frame_.inspector() != nullptr)
                        {
                            const tee::ui::Outline o = tee::ui::outlineOf(*frame_.inspector(), doc_);
                            all.insert(all.end(), o.begin(), o.end());
                        }
                    }
                    frame_.select(sel);
                    frame_.tick(0.0f);
                    column_->setOutline(all);
                }
                if (!frame_.selection().empty() && frame_.inspector() != nullptr)
                {
                    tee::ui::Outline o = tee::ui::outlineOf(*frame_.inspector(), doc_), few;
                    int n = 0;
                    for (const OutlineEntry& e : o)
                    {
                        if (e.kind == OutlineEntry::Kind::field && n++ >= 6)
                            break;
                        if (e.kind != OutlineEntry::Kind::heading)
                            few.push_back(e);
                    }
                    // "engine.cylinder[2]": CYLINDER 3 (the last part, its index counted from 1)
                    std::string title = frame_.selection().path();
                    title = title.substr(title.rfind('.') == std::string::npos ? 0 : title.rfind('.') + 1);
                    if (const std::size_t b = title.find('['); b != std::string::npos)
                        title = title.substr(0, b) + " " + std::to_string(std::atoi(title.c_str() + b + 1) + 1);
                    card_->setCard(title, std::to_string(tee::ui::fieldsOf(o).size()) + " FIELDS HERE; ALL IN THE LIST");
                    card_->setOutline(few);
                    placeCard();
                }
                else
                    card_->setShown(false);
                return;
            }
            if (frame_.inspector() != nullptr)
            {
                column_->setOutline(tee::ui::outlineOf(*frame_.inspector(), doc_));
                // The filter searches the whole workspace: every page's fields.
                std::vector<int> all;
                const int page = frame_.inspectorPageIndex();
                for (int p = 0; p < frame_.inspectorPageCount(); ++p)
                {
                    frame_.showInspectorPage(p);
                    if (frame_.inspector() != nullptr)
                    {
                        const std::vector<int> f = tee::ui::fieldsOf(tee::ui::outlineOf(*frame_.inspector(), doc_));
                        all.insert(all.end(), f.begin(), f.end());
                    }
                }
                frame_.showInspectorPage(page);
                column_->setSearchable(all);
            }
        }
        funkgui::Rect columnRect() const
        {
            funkgui::Rect r = frame_.inspectorRect();
            // Below the frame's page tabs, however many rows they wrap to.
            float tabs = r.y;
            for (int p = 0; p < frame_.inspectorPageCount(); ++p)
                tabs = std::max(tabs, frame_.inspectorTabRect(p).bottom() + 6.0f);
            if (design_ != Design::picture && tabs > r.y)
            {
                r.h -= tabs - r.y;
                r.y = tabs;
            }
            if (design_ == Design::picture && frame_.inspectorPageCount() > 0)
            {
                const float top = frame_.inspectorTabRect(0).y;   // the list replaces the page tabs
                r.h += r.y - top;
                r.y = top;
            }
            return r;
        }

        void tick(float dt) override
        {
            Board::tick(dt);
            sync();
        }

    protected:
        funkgui::Panel* inner() const override { return &frame_; }
        void attached() override { frame_.attach(*host()); }
        void arrange() override
        {
            frame_.resize(width(), height());
            column_->setPlace(columnRect(), true);
            if (card_ != nullptr && card_->shown())
                placeCard();
        }
        void drawOver(funkgui::Canvas& c, const funkgui::Theme& th) const override
        {
            funkgui::Rect r = columnRect();                   // the shipped page under the design: covered, from
            const funkgui::Rect page = frame_.inspectorRect(); // its own top (wrapped tabs end inside it)
            if (page.y < r.y)
            {
                r.h += r.y - page.y;
                r.y = page.y;
            }
            float tabs = 0.0f;
            for (int p = 0; p < frame_.inspectorPageCount(); ++p)
                tabs = std::max(tabs, frame_.inspectorTabRect(p).bottom());
            if (design_ != Design::picture && tabs > r.y)
            {
                r.h -= tabs + 1.0f - r.y;
                r.y = tabs + 1.0f;
            }
            c.rrect(r.x, r.y, r.w, r.h, 0.0f, th.ground);
        }

    private:
        void placeCard()
        {
            const funkgui::Rect stage = frame_.stageRect();
            const float w = 300.0f;
            card_->setPlace({ 0.0f, 0.0f, w, 2000.0f }, true);   // laid out at its width, then as tall as it holds
            const float h = std::min(34.0f + 26.0f + card_->contentHeight(), stage.h - 20.0f);
            float x = anchor_.x + 24.0f;
            if (x + w > stage.right() - 8.0f)
                x = anchor_.x - 24.0f - w;
            x = std::clamp(x, stage.x + 8.0f, stage.right() - 8.0f - w);
            const float y = std::clamp(anchor_.y - 40.0f, stage.y + 8.0f, stage.bottom() - 8.0f - h);
            card_->setPlace({ x, y, w, h }, true);
            bump();
        }

        tee::ui::AppFrame& frame_;
        tee::ui::FieldDoc& doc_;
        Design design_;
        FieldInspector* column_ = nullptr;
        FieldInspector* card_ = nullptr;
        std::string seen_;
        int wsSeen_ = -1;
        funkgui::Point anchor_{};
    };

    // An overlay of `design` over the rig's frame at a window of w x h, in a host of its own.
    struct Shot
    {
        std::unique_ptr<Overlay> overlay;
        std::unique_ptr<funkgui::HeadlessHost> host;
        Shot(Rig& rig, Design d, int w, int h)
        {
            overlay = std::make_unique<Overlay>(*rig.frame, *rig.doc, rig.units, d, w, h);
            host = std::make_unique<funkgui::HeadlessHost>(*overlay, 0, 2.0f);
            host->resize(w, h);
            host->tick(10);
        }
        void show(Rig& rig, const char* workspace, const char* page)
        {
            tee::ui::AppFrame& f = *rig.frame;
            f.select({});
            f.show(f.find(workspace));
            host->tick(2);
            for (int p = 0; p < f.inspectorPageCount(); ++p)
                if (f.inspectorTitle(p) == page)
                    f.showInspectorPage(p);
            host->tick(30);
        }
        void picture(const std::string& name)
        {
            const std::filesystem::path dir = std::filesystem::path(TEE_UI_OUT_DIR) / "inspect";
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            const std::string path = (dir / (name + ".png")).string();
            tee::test::writePng(funkgui::rasterise(host->draw(), funkgui::FontService::get().atlas(), 1), path);
            std::printf("  %s\n", path.c_str());
        }
    };

    // The first number fields of what the column shows, moved off their defaults (the CHANGED marks in a picture).
    void changeSome(Rig& rig, FieldInspector& col, int n)
    {
        int done = 0;
        for (const FieldInspector::Row& r : col.rows())
            if (r.kind == FieldInspector::Row::Kind::field && done < n)
            {
                const tee::ui::FieldInfo& f = rig.doc->info(r.field);
                if (f.kind != tee::ui::FieldKind::number || !(f.max > f.min))
                    continue;
                const double d = rig.doc->get(r.field);   // about a tenth off its default, as a person would
                rig.doc->set(r.field, std::clamp(d == 0.0 ? f.min + (f.max - f.min) * 0.1 : d * (done == 0 ? 1.1 : 0.92), f.min, f.max));
                ++done;
            }
    }

    // A part on the stage that selects something with a page: found by pressing a grid of points over the stage.
    bool choosePart(Rig& rig, Shot& s, funkgui::Point& at)
    {
        tee::ui::AppFrame& f = *rig.frame;
        const funkgui::Rect st = f.stageRect();
        for (float fy = 0.30f; fy < 0.95f; fy += 0.07f)
            for (float fx = 0.25f; fx < 0.95f; fx += 0.06f)
            {
                f.select({});
                s.host->tick(1);
                at = { st.x + st.w * fx, st.y + st.h * fy };
                s.host->click(at.x, at.y);
                s.host->tick(3);
                if (!f.selection().empty() && f.inspector() != nullptr
                    && !tee::ui::fieldsOf(tee::ui::outlineOf(*f.inspector(), *rig.doc)).empty())
                    return true;
            }
        return false;
    }

    BUILD_TEST(inspectPictures, "build.inspect.pictures")
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
        for (const Design d : { Design::rows, Design::dials })
            for (const auto& [w, h] : sizes)
                for (const Page& p : pages)
                {
                    rig.reset();
                    Shot s(rig, d, w, h);
                    s.show(rig, p.ws, p.page);
                    changeSome(rig, s.overlay->column(), 2);
                    s.host->tick(5);
                    const std::string base = std::string(nameOf(d)) + "." + p.name + "." + (w == 1100 ? "small" : "large");
                    s.picture(base);
                    if (std::string(p.name) == "exhaust")
                    {
                        // scrolled half way: a heading stays at the top while its group scrolls under it
                        FieldInspector& col = s.overlay->column();
                        const funkgui::Rect r = col.bounds();
                        for (int k = 0; k < 8; ++k)
                            s.host->wheel(r.x + 20.0f, r.centreY(), -1.0f);
                        s.host->tick(30);
                        s.picture(base + ".scrolled");
                    }
                    if (std::string(p.name) == "engine")
                    {
                        // a value typed out of range: refused, the range said
                        FieldInspector& col = s.overlay->column();
                        for (const FieldInspector::Row& r : col.rows())
                            if (r.kind == FieldInspector::Row::Kind::field && rig.doc->info(r.field).kind == tee::ui::FieldKind::number
                                && r.field != col.rows().front().field)
                            {
                                funkgui::Rect row, value;
                                if (col.fieldRect(r.field, row, value))
                                {
                                    s.host->click(value.centreX(), value.centreY());   // a click types it
                                    s.host->keys("9,9,9,9,9,return");
                                    s.host->tick(5);
                                    s.picture(base + ".refused");
                                    break;
                                }
                            }
                    }
                }
        // Design 1's filter and CHANGED: across the workspace.
        for (const auto& [w, h] : sizes)
        {
            rig.reset();
            Shot s(rig, Design::rows, w, h);
            s.show(rig, "engine", "ENGINE 1");
            FieldInspector& col = s.overlay->column();
            changeSome(rig, col, 2);
            col.setFilter("valve");
            s.host->tick(20);
            s.picture(std::string("rows.filter.") + (w == 1100 ? "small" : "large"));
            col.setFilter({});
            col.setChangedOnly(true);
            s.host->tick(20);
            s.picture(std::string("rows.changed.") + (w == 1100 ? "small" : "large"));
        }
        // Design 3: a part chosen on the stage, its card beside it; the column lists everything.
        for (const char* ws : { "engine", "exhaust", "tune" })
            for (const auto& [w, h] : sizes)
            {
                rig.reset();
                Shot s(rig, Design::picture, w, h);
                s.show(rig, ws, "");
                funkgui::Point at{};
                const bool found = choosePart(rig, s, at);
                if (found)
                    s.overlay->chose(at);
                s.host->tick(30);
                s.picture(std::string("picture.") + ws + "." + (w == 1100 ? "small" : "large"));
                if (!found)
                    std::printf("  (no part of %s's stage selects a page by a press: the picture has no card)\n", ws);
            }
        return 0;
    }
}

