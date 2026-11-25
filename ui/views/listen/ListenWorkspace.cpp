// ui/views/listen/ListenWorkspace.cpp: see ListenWorkspace.h.
#include "views/listen/ListenWorkspace.h"

#include "views/engine/PageText.h"
#include "views/scope/ScopePlot.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdio>
#include <limits>

namespace tee::ui
{
    namespace
    {
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Point;
        using funkgui::Rect;

        constexpr double kPi = 3.14159265358979323846;
        constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
        constexpr float kGap = 10.0f, kButtonH = 26.0f;

        const char* const kSourceNames[ListenWorkspace::kSources] = { "EXHAUST L", "EXHAUST R", "INTAKE", "ENGINE", "TURBO",
                                                                    "TYRES", "WIND", "GEARBOX", "AXLE", "TANK" };
        const char* const kCameraWords[ListenWorkspace::kCameras] = { "TAILPIPE", "REAR", "SIDE", "ENGINE BAY", "DRIVER",
                                                                    "TRACKSIDE", "CHASE", "FREE" };
        const char* const kPlaceWords[5] = { "HALL", "FIELD", "STREET", "TUNNEL", "GARAGE" };
        const char* const kViewWords[5] = { "EXHAUST", "REAR", "SIDE", "ENGINE", "CABIN" };

        // The model's documented places (docs/models/listen.md), front, mid and rear engine: x, y in metres.
        struct Place2
        {
            double x, y;
        };
        Place2 documented(int source, int enginePlace)
        {
            const int e = std::clamp(enginePlace, 0, 2);
            switch (source)
            {
                case 0: return { -0.55, 0.0 };
                case 1: return { 0.4, 0.0 };
                case 2: return e == 0 ? Place2{ 0.35, 3.9 } : e == 1 ? Place2{ 0.0, 1.4 } : Place2{ 0.0, 0.65 };
                case 3:
                case 4: return e == 0 ? Place2{ 0.0, 3.4 } : e == 1 ? Place2{ 0.0, 1.1 } : Place2{ 0.0, 0.55 };
                case 5: return { 0.0, 2.2 };
                case 6: return { 0.0, 2.4 };
                case 7: return e == 0 ? Place2{ 0.0, 2.4 } : Place2{ 0.0, 0.9 };
                case 8: return e == 0 ? Place2{ 0.0, 0.9 } : Place2{ 0.0, 0.6 };
                case 9: return e == 0 ? Place2{ 0.3, 0.8 } : Place2{ 0.3, 3.6 };
                default: return { 0.0, 0.0 };
            }
        }

        std::string fmt(const char* f, double v)
        {
            if (!std::isfinite(v))
                return "-";
            char b[48];
            std::snprintf(b, sizeof b, f, v);
            return b;
        }
        float wordsWidth(const std::string& s) { return 18.0f + 8.4f * static_cast<float>(s.size()); }
    }

    ListenWorkspace::ListenWorkspace(WorkspaceContext& c) : Workspace(c)
    {
        for (int i = 0; i < kCameras; ++i)
        {
            cameras_[static_cast<std::size_t>(i)] = &make<Button>(kCameraWords[i], [this, i] { set("sound.camera", i); });
            cameras_[static_cast<std::size_t>(i)]->setLit([this, i] { return scene().camera == i; });
        }
        for (int i = 0; i < 5; ++i)
        {
            places_[static_cast<std::size_t>(i)] = &make<Button>(kPlaceWords[i], [this, i] { set("sound.place", i); });
            places_[static_cast<std::size_t>(i)]->setLit([this, i] { return scene().place == i; });
            views_[static_cast<std::size_t>(i)] = &make<Button>(kViewWords[i], [this, i] { set("sound.view", i); });
            views_[static_cast<std::size_t>(i)]->setLit([this, i] { return std::lround(field("sound.view", 0.0)) == i; });
        }
        // Hover help (the owner's ask, 25 Nov): where each button puts the listener.
        const char* cameraSays[kCameras] = {
            "Listen at the tailpipe's mouth, behind the car",
            "Listen from behind the car, a few metres back",
            "Listen from beside the car",
            "Listen in the engine bay, beside the engine",
            "Listen from the driver's seat: the cabin's ways in, and the windows to open or shut",
            "Listen from the side of the road as the car passes",
            "Listen from a camera following the car",
            "Listen where you put the listener: drag the point on the plan, turn it by its handle",
        };
        for (int i = 0; i < kCameras; ++i)
            cameras_[static_cast<std::size_t>(i)]->setHelp(cameraSays[i]);
        const char* placeSays[5] = {
            "The reference's room around the car",
            "Open ground: the ground's reflection and little else",
            "A street between building fronts, which echo",
            "Inside a tunnel: its walls close on both sides, a long reverberation",
            "Inside a garage: a small hard room",
        };
        const char* viewSays[5] = {
            "The reference's view at the exhaust",
            "The reference's view from behind",
            "The reference's view from the side",
            "The reference's view at the engine",
            "The reference's view from the cabin",
        };
        for (int i = 0; i < 5; ++i)
        {
            places_[static_cast<std::size_t>(i)]->setHelp(placeSays[i]);
            views_[static_cast<std::size_t>(i)]->setHelp(viewSays[i]);
        }
    }

    std::unique_ptr<Workspace> makeListenWorkspace(WorkspaceContext& c) { return std::make_unique<ListenWorkspace>(c); }

    // ---- the fields and the scene -----------------------------------------------------------------------------------
    double ListenWorkspace::field(const char* path, double fallback) const
    {
        const int f = ctx_.doc.find(path);
        return f < 0 ? fallback : ctx_.doc.get(f);
    }

    std::string ListenWorkspace::choice(const char* path) const
    {
        const int f = ctx_.doc.find(path);
        if (f < 0 || ctx_.doc.info(f).kind != FieldKind::choice)
            return {};
        const FieldInfo& fi = ctx_.doc.info(f);
        const long i = std::lround(ctx_.doc.get(f) - fi.min);
        return i >= 0 && i < static_cast<long>(fi.choices.size()) ? fi.choices[static_cast<std::size_t>(i)] : std::string();
    }

    void ListenWorkspace::set(const char* path, double v)
    {
        const int f = ctx_.doc.find(path);
        if (f < 0)
            return;
        ctx_.doc.beginGesture();
        ctx_.doc.set(f, v);
        ctx_.doc.endGesture();
        ++revision_;
    }

    ListenWorkspace::Scene ListenWorkspace::scene() const
    {
        Scene s;
        const Live& live = ctx_.live;
        s.model = static_cast<int>(std::lround(field("sound.model", 0.0)));
        s.camera = s.model == 0 ? std::clamp(static_cast<int>(std::lround(field("sound.view", 0.0))), 0, 4)
                                : static_cast<int>(std::lround(field("sound.camera", 0.0)));
        s.place = s.model == 0 ? 0 : static_cast<int>(std::lround(field("sound.place", 0.0)));
        s.enginePlace = static_cast<int>(std::lround(field("sound.enginePlace", 0.0)));
        s.bike = choice("car.kind") == "Motorcycle";
        const double lift = live.get("car.liftAngle");   // a bike's pitch about its rear tyre, degrees
        s.lift = s.bike && std::isfinite(lift) ? lift * kPi / 180.0 : 0.0;
        s.wheelbase = field("car.wheelbase", 2.6);
        s.track = field("car.track", 1.55);
        const double ra = live.get("sound.rearAxleY");
        s.rearAxle = std::isfinite(ra) ? ra : (s.bike ? 0.3 : 0.85);
        const double fa = live.get("sound.frontAxleY");
        if (std::isfinite(fa) && std::isfinite(ra) && fa > ra)
            s.wheelbase = fa - ra;   // L's, the same as the build's
        s.length = s.rearAxle + s.wheelbase + (s.bike ? 0.3 : 0.9);
        s.width = s.bike ? 0.8 : s.track + 0.25;
        // The sources: the program's places and levels, else the model's documented places (no level).
        for (int i = 0; i < kSources; ++i)
        {
            Source& o = s.src[static_cast<std::size_t>(i)];
            o.name = kSourceNames[i];
            const std::string k = "[" + std::to_string(i) + "]";
            const double x = live.get("sound.srcX" + k), y = live.get("sound.srcY" + k);
            o.published = std::isfinite(x) && std::isfinite(y);
            const Place2 d = documented(i, s.enginePlace);
            o.x = o.published ? x : d.x;
            o.y = o.published ? y : d.y;
            o.level = live.get("sound.srcLevel" + k);
        }
        // The listener: as heard, else where its camera puts it (docs/models/listen.md, "Cameras").
        const int fd = ctx_.doc.find("sound.distance");
        const double d = field("sound.distance", 0.3);
        const double r = fd >= 0 && ctx_.doc.info(fd).max > 1.0 ? d : 0.5 * std::exp(4.094 * d);   // a fraction, or metres
        const double bay = s.src[3].y, mid = s.length * 0.5;
        const double lx = live.get("sound.listenerX"), ly = live.get("sound.listenerY"), lf = live.get("sound.facing");
        if (s.camera == 7)
        {
            s.lx = field("sound.listenerX", 0.0);
            s.ly = field("sound.listenerY", -3.0);
            s.facing = field("sound.facing", 0.0) * kPi / 180.0;   // degrees from forward toward the right
            s.listenerPublished = has("sound.listenerX");
        }
        else if (std::isfinite(lx) && std::isfinite(ly))
        {
            s.lx = lx;
            s.ly = ly;
            s.facing = std::isfinite(lf) ? lf * kPi / 180.0 : 0.0;
            s.listenerPublished = true;
        }
        else
        {
            const double road = live.get("sound.roadPosition");
            switch (s.camera)
            {
                case 0: s.lx = 0.0; s.ly = -r; s.facing = 0.0; break;
                case 1: s.lx = 0.0; s.ly = -2.0 - r; s.facing = 0.0; break;
                case 2: s.lx = 0.9 + r; s.ly = 1.3; s.facing = -kPi / 2; break;
                case 3: s.lx = 0.0; s.ly = bay + r / 2; s.facing = kPi; break;
                case 4: s.lx = -0.35; s.ly = s.enginePlace == 0 ? 1.7 : s.enginePlace == 1 ? 2.1 : 2.0; s.facing = 0.0; break;
                case 5: s.lx = 0.9 + 2 * r; s.ly = mid - (std::isfinite(road) ? road : -40.0); s.facing = -kPi / 2; break;
                case 6: s.lx = 0.0; s.ly = -3.0 - r; s.facing = 0.0; break;
                default: s.lx = 0.0; s.ly = -r; s.facing = 0.0; break;
            }
        }
        const double rg = live.get("sound.range");
        s.range = rg;   // the program's, or a dash: not worked out here
        return s;
    }

    std::string ListenWorkspace::inspectorPage() const
    {
        const bool reference = std::lround(field("sound.model", 0.0)) == 0;
        std::vector<pages::Section> secs;
        if (reference)
            secs.push_back({ "HEARD FROM", { "sound.model", "sound.view", "sound.distance" }, { "MODE", "REFERENCE VIEW", "DISTANCE" } });
        else
        {
            secs.push_back({ "HEARD FROM", { "sound.model", "sound.camera", "sound.distance" }, { "MODE", "CAMERA", "DISTANCE" } });
            secs.push_back({ "PLACE", { "sound.place", "sound.ground", "sound.enginePlace" }, { "PLACE", "GROUND", "ENGINE AT" } });
            secs.push_back({ "THE FREE LISTENER",
                             { "sound.listenerX", "sound.listenerY", "sound.listenerZ", "sound.facing" },
                             { "ACROSS", "ALONG", "HEIGHT", "FACING" } });
        }
        secs.push_back({ "ROOM", { "sound.room", "sound.windows" }, { "ROOM", "WINDOWS" } });
        secs.push_back({ "LEVELS", { "sound.level", "sound.intake", "sound.mechanical" }, { "LEVEL", "INTAKE", "MECHANICAL" } });
        // Every field is on a page (the frame holds it): what the mode now does not read goes last, named so.
        if (!reference)
            secs.push_back({ "REFERENCE MODE ONLY", { "sound.view" }, { "REFERENCE VIEW" } });
        else
            secs.push_back({ "PLACES MODE ONLY",
                             { "sound.camera", "sound.place", "sound.ground", "sound.enginePlace", "sound.windows",
                               "sound.listenerX", "sound.listenerY", "sound.listenerZ", "sound.facing" },
                             { "CAMERA", "PLACE", "GROUND", "ENGINE AT", "WINDOWS", "ACROSS", "ALONG", "HEIGHT", "FACING" } });
        // And any field of the section this page does not name (one added since), under its own label.
        pages::Section rest{ "MORE", {}, {} };
        for (int f = 0; f < ctx_.doc.count(); ++f)
        {
            const FieldInfo& fi = ctx_.doc.info(f);
            if (fi.hidden || fi.section() != "sound")
                continue;
            bool named = false;
            for (const pages::Section& sc : secs)
                named = named || std::find(sc.paths.begin(), sc.paths.end(), fi.path) != sc.paths.end();
            if (!named)
                rest.paths.push_back(fi.path);
        }
        if (!rest.paths.empty())
            secs.push_back(rest);
        return pages::page(ctx_.doc, secs);
    }

    // ---- the plan's mapping -----------------------------------------------------------------------------------------
    Point ListenWorkspace::toPx(double x, double y) const noexcept
    {
        return { plan_.centreX() + static_cast<float>((x - cx_) * scale_), plan_.centreY() - static_cast<float>((y - cy_) * scale_) };
    }

    void ListenWorkspace::toCar(Point p, double& x, double& y) const noexcept
    {
        x = cx_ + (p.x - plan_.centreX()) / scale_;
        y = cy_ - (p.y - plan_.centreY()) / scale_;
    }

    Point ListenWorkspace::listenerPx() const
    {
        const Scene s = scene();
        return toPx(s.lx, s.ly);
    }

    Point ListenWorkspace::facingHandlePx() const
    {
        const Scene s = scene();
        const Point l = toPx(s.lx, s.ly);
        return { l.x + 30.0f * static_cast<float>(std::sin(s.facing)), l.y - 30.0f * static_cast<float>(std::cos(s.facing)) };
    }

    std::vector<Rect> ListenWorkspace::wheelRects() const
    {
        const Scene s = scene();
        const float m = static_cast<float>(scale_);
        std::vector<Rect> out;
        if (s.bike)
        {
            for (const double y : { s.rearAxle, s.rearAxle + s.wheelbase * std::cos(s.lift) })
            {
                const Point w = toPx(0.0, y);
                out.push_back({ w.x - 0.08f * m, w.y - 0.33f * m, 0.16f * m, 0.66f * m });
            }
            return out;
        }
        for (const double y : { s.rearAxle, s.rearAxle + s.wheelbase })
            for (const double x : { -s.track / 2, s.track / 2 })
            {
                const Point w = toPx(x, y);
                out.push_back({ w.x - 0.12f * m, w.y - 0.32f * m, 0.24f * m, 0.64f * m });
            }
        return out;
    }

    void ListenWorkspace::fit()
    {
        if (plan_.isEmpty())
            return;
        const Scene s = scene();
        double x0 = -s.width / 2 - 0.5, x1 = s.width / 2 + 0.5, y0 = -0.8, y1 = s.length + 0.8;
        const auto take = [&](double x, double y) {
            x0 = std::min(x0, x);
            x1 = std::max(x1, x);
            y0 = std::min(y0, y);
            y1 = std::max(y1, y);
        };
        take(s.lx - 1.2, s.ly - 1.2);
        take(s.lx + 1.2, s.ly + 1.2);
        if (s.place == 3)   // the tunnel's walls, 9 m apart
        {
            take(-5.0, y0);
            take(5.0, y1);
        }
        if (s.place == 4)   // the garage, 6 by 7 m
        {
            take(-3.5, -2.0);
            take(3.5, 6.0);
        }
        if (s.place == 2)   // the street: a facade 4 m to the left, the other 14 m across
            take(-5.0, y0);
        // At least 9 m across, so a car is never drawn huge; the scale a round step, so it does not creep.
        const double w = std::max(9.0, x1 - x0), h = std::max(9.0, y1 - y0);
        const double fitScale = std::min((plan_.w - 40.0) / w, (plan_.h - 60.0) / h);
        const double steps[] = { 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80 };
        double sc = steps[0];
        for (const double k : steps)
            if (k <= fitScale)
                sc = k;
        // The middle on a half-metre grid, for the same reason.
        const double mx = std::round((x0 + x1) * 0.5 * 2.0) / 2.0, my = std::round((y0 + y1) * 0.5 * 2.0) / 2.0;
        if (sc != scale_ || mx != cx_ || my != cy_)
        {
            scale_ = sc;
            cx_ = mx;
            cy_ = my;
            ++revision_;
        }
    }

    // ---- time and input ---------------------------------------------------------------------------------------------
    void ListenWorkspace::step(float)
    {
        // What the stage reads, mixed: when it moves, the plan is fitted again and drawn again.
        const Scene s = scene();
        DrawKey k;
        k.mix(static_cast<float>(s.lx)).mix(static_cast<float>(s.ly)).mix(static_cast<float>(s.facing)).mix(s.camera).mix(s.place);
        for (const Source& o : s.src)
            k.mix(static_cast<float>(o.x)).mix(static_cast<float>(o.y)).mix(static_cast<float>(o.level));
        for (const char* n : { "sound.loudness", "sound.mixLevel", "sound.limiter", "sound.ceilingParts", "sound.doppler",
                               "sound.range", "sound.roadPosition", "sound.pathFloor", "sound.pathFirewall",
                               "sound.pathGlass", "sound.pathStructure" })
            k.mix(static_cast<float>(ctx_.live.get(n)));
        k.word(ctx_.doc.revision());
        if (k.h != seen_)
        {
            seen_ = k.h;
            // Laid out again (the bar's buttons follow the model; the window's handle the camera) and fitted again,
            // except while a drag holds the plan's scale still under the pointer.
            if (drag_ == Drag::none && !stage_.isEmpty())
            {
                layout(stage_);
                bump();
            }
            ++revision_;
        }
    }

    bool ListenWorkspace::moving() const { return drag_ != Drag::none || wantsFullRate(); }

    std::uint64_t ListenWorkspace::drawKey() const { return DrawKey{}.mix(revision_).mix(seen_).mix(ctx_.units.revision()).h; }

    void ListenWorkspace::pointerDown(const funkgui::PointerEvent& e)
    {
        const Point at{ e.x, e.y };
        const auto near = [&](Point q, float r) { return std::hypot(at.x - q.x, at.y - q.y) <= r; };
        const Scene s = scene();
        if (s.model != 0 && plan_.contains(at) && has("sound.listenerX") && has("sound.camera"))
        {
            if (near(facingHandlePx(), 9.0f))
                drag_ = Drag::facing;
            else if (near(listenerPx(), 14.0f))
                drag_ = Drag::listener;
        }
        if (drag_ == Drag::none && s.model != 0 && s.camera == 4 && !windows_.isEmpty() && windows_.contains(at)
            && has("sound.windows"))
            drag_ = Drag::windows;
        if (drag_ == Drag::none)
        {
            Board::pointerDown(e);
            return;
        }
        ctx_.doc.beginGesture();
        if (drag_ != Drag::windows)
        {
            // From where it is heard now: the Free camera starts at the listener's place and facing.
            ctx_.doc.set(ctx_.doc.find("sound.listenerX"), s.lx);
            ctx_.doc.set(ctx_.doc.find("sound.listenerY"), s.ly);
            if (has("sound.facing"))
                ctx_.doc.set(ctx_.doc.find("sound.facing"), s.facing * 180.0 / kPi);
            ctx_.doc.set(ctx_.doc.find("sound.camera"), 7.0);
        }
        pointerDrag(e);
    }

    void ListenWorkspace::pointerDrag(const funkgui::PointerEvent& e)
    {
        if (drag_ == Drag::none)
        {
            Board::pointerDrag(e);
            return;
        }
        if (drag_ == Drag::listener)
        {
            double x = 0, y = 0;
            toCar({ e.x, e.y }, x, y);
            ctx_.doc.set(ctx_.doc.find("sound.listenerX"), std::round(x * 20.0) / 20.0);   // 5 cm steps
            ctx_.doc.set(ctx_.doc.find("sound.listenerY"), std::round(y * 20.0) / 20.0);
        }
        else if (drag_ == Drag::facing && has("sound.facing"))
        {
            const Point l = listenerPx();
            const double a = std::atan2(static_cast<double>(e.x - l.x), static_cast<double>(l.y - e.y));
            ctx_.doc.set(ctx_.doc.find("sound.facing"), std::round(a * 180.0 / kPi / 5.0) * 5.0);   // degrees, 5 at a time
        }
        else if (drag_ == Drag::windows)
        {
            // Down the window opens it: the top is shut.
            const double v = std::clamp((e.y - windows_.y) / std::max(1.0f, windows_.h), 0.0f, 1.0f);
            ctx_.doc.set(ctx_.doc.find("sound.windows"), std::round(v * 20.0) / 20.0);
        }
        ++revision_;
    }

    void ListenWorkspace::pointerUp(const funkgui::PointerEvent& e)
    {
        if (drag_ == Drag::none)
        {
            Board::pointerUp(e);
            return;
        }
        drag_ = Drag::none;
        ctx_.doc.endGesture();
        seen_ = 0;   // fitted again at the next step
        ++revision_;
    }

    funkgui::Cursor ListenWorkspace::cursor() const
    {
        if (drag_ != Drag::none)
            return funkgui::Cursor::grabbing;
        return Board::cursor();
    }

    // ---- layout -----------------------------------------------------------------------------------------------------
    void ListenWorkspace::layout(const Rect& s)
    {
        const Scene sc = scene();
        const bool reference = sc.model == 0;
        // The bar: the cameras (or the reference's views), then the places, each group whole, wrapping.
        const float x0 = s.x + kGap + look::kPad, right = s.right() - kGap - look::kPad;
        float x = x0, line = 0.0f;
        const auto groupWidth = [](auto& buttons, float lead) {
            float w = lead;
            for (Button* b : buttons)
                w += wordsWidth(b->title()) + 4.0f;
            return w;
        };
        const auto place = [&](float w) {
            if (x > x0 && x + w > right)
            {
                x = x0;
                line += 1.0f;
            }
            const float at = x;
            x += w + 16.0f;
            return at;
        };
        const float camLead = 64.0f, placeLead = 52.0f;
        const float camX = place(reference ? groupWidth(views_, camLead) : groupWidth(cameras_, camLead));
        const float camLine = line;
        const float plX = reference ? 0.0f : place(groupWidth(places_, placeLead));
        const float plLine = line;
        const float rows = line + 1.0f;
        bar_ = { s.x + kGap, s.y + kGap, s.w - 2 * kGap, 12.0f + rows * (kButtonH + 8.0f) };
        const float by = bar_.y + 10.0f, step = kButtonH + 8.0f;
        x = camX + camLead;
        for (Button* b : cameras_)
        {
            const float w = wordsWidth(b->title());
            b->setPlace(reference ? Rect{} : Rect{ x, by + camLine * step, w, kButtonH }, !reference);
            if (!reference)
                x += w + 4.0f;
        }
        x = camX + camLead;
        for (Button* b : views_)
        {
            const float w = wordsWidth(b->title());
            b->setPlace(reference ? Rect{ x, by + camLine * step, w, kButtonH } : Rect{}, reference);
            if (reference)
                x += w + 4.0f;
        }
        x = plX + placeLead;
        for (Button* b : places_)
        {
            const float w = wordsWidth(b->title());
            b->setPlace(reference ? Rect{} : Rect{ x, by + plLine * step, w, kButtonH }, !reference);
            if (!reference)
                x += w + 4.0f;
        }
        // The plan, and the meters' column at its right.
        const float top = bar_.bottom() + kGap;
        const float cw = std::clamp(s.w * 0.26f, 200.0f, 280.0f);
        column_ = { s.right() - kGap - cw, top, cw, s.bottom() - kGap - top };
        plan_ = { s.x + kGap, top, column_.x - kGap - (s.x + kGap), s.bottom() - kGap - top };
        fit();
        // The driver's window, on the picture (the left side, beside the driver's seat).
        if (sc.camera == 4 && !reference && !sc.bike)
        {
            const Point a = toPx(-sc.width / 2, sc.ly + 0.7), b = toPx(-sc.width / 2, sc.ly - 0.5);
            windows_ = { a.x - 7.0f, a.y, 14.0f, b.y - a.y };
        }
        else
            windows_ = {};
    }

    // ---- drawing ----------------------------------------------------------------------------------------------------
    void ListenWorkspace::drawPlace(Canvas& c, const look::Palette& p, const Scene& s) const
    {
        const Rect a = plan_;
        const Canvas::ClipScope clip(c, a);
        const auto wallX = [&](double x, const char* word) {
            const float px = toPx(x, 0.0).x;
            if (px < a.x || px > a.right())
                return;
            c.segment(px, a.y + 30.0f, px, a.bottom() - 6.0f, 4.0f, p.dim);
            c.text(word, px + (x < 0 ? 8.0f : -8.0f), a.y + 32.0f, look::kCaps, p.label,
                   x < 0 ? funkgui::Align::left : funkgui::Align::right);
        };
        switch (s.place)
        {
            case 1:   // field: a tree line 34 m off, said where it is
                c.text("FIELD: A TREE LINE 34 M AWAY", a.x + look::kPad, a.bottom() - 20.0f, look::kCaps, p.label);
                break;
            case 2:   // street: a facade 4 m to the left, the other 14 m across
                wallX(-s.width / 2 - 4.0, "FACADE");
                wallX(-s.width / 2 - 4.0 + 14.0, "FACADE");
                break;
            case 3:
                wallX(-4.5, "TUNNEL WALL");
                wallX(4.5, "TUNNEL WALL");
                break;
            case 4:
            {
                const Point t = toPx(-3.0, 5.5), b = toPx(3.0, -1.5);
                c.rrect(t.x, t.y, b.x - t.x, b.y - t.y, 3.0f, kClear, 3.0f, p.dim);
                c.text("GARAGE 6 X 7 M", t.x + 8.0f, t.y + 6.0f, look::kCaps, p.label);
                break;
            }
            default:
                c.text(s.model == 0 ? "THE REFERENCE'S ROOM" : "HALL: THE REFERENCE'S ROOM", a.x + look::kPad,
                       a.bottom() - 20.0f, look::kCaps, p.label);
                break;
        }
        // Trackside: the road along the car's line, the car's place along it.
        if (s.camera == 5 && s.model != 0)
        {
            const float l = toPx(-2.0, 0.0).x, r = toPx(2.0, 0.0).x;
            c.hairlineV(c.snapX(l), a.y + 26.0f, a.h - 26.0f, p.label);
            c.hairlineV(c.snapX(r), a.y + 26.0f, a.h - 26.0f, p.label);
            const float xs[2] = { (l + r) * 0.5f, (l + r) * 0.5f }, ys[2] = { a.y + 26.0f, a.bottom() };
            dashedPolyline(c, xs, ys, 2, 1.25f, p.dim, 10.0f, 10.0f);
            c.text("ROAD", r + 6.0f, a.bottom() - 20.0f, look::kCaps, p.label);
        }
    }

    void ListenWorkspace::drawCar(Canvas& c, const look::Palette& p, const Scene& s) const
    {
        const float m = static_cast<float>(scale_);
        const Point tl = toPx(-s.width / 2, s.length), br = toPx(s.width / 2, 0.0);
        const Col body = p.raised;
        if (s.bike)
        {
            // A bike: its frame a capsule, its two wheels in line; lifted about its rear tyre, it is shorter from above.
            const Point f = toPx(0.0, s.rearAxle + s.wheelbase * std::cos(s.lift)), r = toPx(0.0, s.rearAxle);
            c.segment(f.x, f.y, r.x, r.y, 0.45f * m, body);
            c.rrect(f.x - 0.06f * m, f.y - 0.33f * m, 0.12f * m, 0.66f * m, 0.06f * m, p.dim);
            c.rrect(r.x - 0.08f * m, r.y - 0.33f * m, 0.16f * m, 0.66f * m, 0.08f * m, p.dim);
        }
        else
        {
            c.rrect(tl.x, tl.y, br.x - tl.x, br.y - tl.y, 0.45f * m, body, 1.5f, p.edge);
            // the wheels
            for (const double y : { s.rearAxle, s.rearAxle + s.wheelbase })
                for (const double x : { -s.track / 2, s.track / 2 })
                {
                    const Point w = toPx(x, y);
                    c.rrect(w.x - 0.12f * m, w.y - 0.32f * m, 0.24f * m, 0.64f * m, 0.06f * m, p.dim);
                }
            // the windscreen and the rear window, where a driver sits a little behind the middle
            const double screen = s.rearAxle + s.wheelbase * 0.62, back = s.rearAxle + s.wheelbase * 0.12;
            for (const double y : { screen, back })
            {
                const Point a = toPx(-s.width / 2 + 0.15, y), b = toPx(s.width / 2 - 0.15, y);
                c.segment(a.x, a.y, b.x, b.y, 2.0f, p.edge);
            }
        }
        // its front, in words
        const Point front = toPx(0.0, s.length);
        c.text("FRONT", front.x, front.y - 16.0f, look::kCaps, p.label, funkgui::Align::centre);
    }

    void ListenWorkspace::drawPlan(Canvas& c, const look::Palette& p, const Scene& s) const
    {
        look::region(c, p, plan_, "FROM ABOVE");
        drawPlace(c, p, s);
        const Canvas::ClipScope clip(c, { plan_.x + 1.0f, plan_.y + 24.0f, plan_.w - 2.0f, plan_.h - 25.0f });
        drawCar(c, p, s);
        const Point l = toPx(s.lx, s.ly);
        // The sources: a dot each, its size its level at the listener (dim and hollow when the level is not known),
        // its name and level beside it, placed where it does not cover another's.
        std::vector<Rect> taken;
        const auto free = [&](const Rect& r) {
            for (const Rect& t : taken)
                if (r.x < t.right() && t.x < r.right() && r.y < t.bottom() && t.y < r.bottom())
                    return false;
            return true;
        };
        for (int i = 0; i < kSources; ++i)
        {
            const Source& o = s.src[static_cast<std::size_t>(i)];
            const Point q = toPx(o.x, o.y);
            taken.push_back({ q.x - 6.0f, q.y - 6.0f, 12.0f, 12.0f });
        }
        taken.push_back({ l.x - 10.0f, l.y - 10.0f, 20.0f, 20.0f });
        for (const Rect& w : wheelRects())   // no label on a wheel
            taken.push_back(w);
        if (!windows_.isEmpty())
            taken.push_back({ windows_.x - 130.0f, windows_.bottom() + 2.0f, 136.0f, 16.0f });   // the window's words
        {
            // the car's word, and the listener's (drawn below) are kept clear too
            const Point front = toPx(0.0, s.length);
            taken.push_back({ front.x - 24.0f, front.y - 17.0f, 48.0f, 14.0f });
        }
        // Rings of distance round the listener, numbered at their tops where the number has room.
        for (const double d : { 1.0, 2.0, 5.0, 10.0, 20.0, 30.0 })
        {
            const float r = static_cast<float>(d * scale_);
            if (r < 18.0f || r > plan_.w + plan_.h)
                continue;
            c.arcDotted(l, r, 0.0f, 6.2831853f, 6.0f, 1.5f, p.dim);
            const std::string t = fmt("%.0f", d) + " m";
            const Rect tr{ l.x + 2.0f, l.y - r - 14.0f, c.textWidth(t.c_str(), look::kUnit) + 4.0f, 13.0f };
            if (free(tr))
            {
                taken.push_back(tr);
                c.text(t.c_str(), tr.x + 1.0f, tr.y, look::kUnit, p.label);
            }
        }
        // Sources at one place (within 15 cm) share a label: their names joined, the loudest's level.
        std::array<bool, kSources> done{};
        for (int i = 0; i < kSources; ++i)
        {
            if (done[static_cast<std::size_t>(i)])
                continue;
            const Source& o = s.src[static_cast<std::size_t>(i)];
            const Point q = toPx(o.x, o.y);
            std::string name = o.name;
            double loudest = o.level;
            for (int j = i + 1; j < kSources; ++j)
            {
                const Source& u = s.src[static_cast<std::size_t>(j)];
                if (!done[static_cast<std::size_t>(j)] && std::hypot(u.x - o.x, u.y - o.y) < 0.15)
                {
                    done[static_cast<std::size_t>(j)] = true;
                    name += std::string(", ") + u.name;
                    if (std::isfinite(u.level) && !(u.level <= loudest))
                        loudest = u.level;
                }
            }
            const Col ink = i <= 1 ? p.hot : i == 2 ? p.cold : p.text;
            const bool known = std::isfinite(loudest) && loudest > -120.0;
            const float r = known ? 3.0f + 8.0f * static_cast<float>(std::clamp((loudest + 60.0) / 60.0, 0.0, 1.0)) : 4.0f;
            if (known)
                c.disc(q.x, q.y, r, ink, 1.25f, p.ground);
            else
                c.disc(q.x, q.y, r, kClear, 1.25f, p.dim);
            // A silent source keeps its hollow dot and no label until it sounds (its place is the model's, said once).
            if (std::isfinite(loudest) && !known)
                continue;
            const std::string lv = known ? fmt("%.0f dB", loudest) : std::string("-");
            const float w = std::max(c.textWidth(name.c_str(), look::kCaps), c.textWidth(lv.c_str(), look::kUnit)) + 4.0f;
            const float h = 26.0f;
            const Rect tries[] = { { q.x + r + 4.0f, q.y - 12.0f, w, h }, { q.x - r - 4.0f - w, q.y - 12.0f, w, h },
                                   { q.x - w * 0.5f, q.y - r - 4.0f - h, w, h }, { q.x - w * 0.5f, q.y + r + 4.0f, w, h },
                                   { q.x + r + 4.0f, q.y + 8.0f, w, h }, { q.x - r - 4.0f - w, q.y + 8.0f, w, h },
                                   { q.x + r + 4.0f, q.y - 34.0f, w, h }, { q.x - r - 4.0f - w, q.y - 34.0f, w, h },
                                   { q.x + r + 30.0f, q.y - 12.0f, w, h }, { q.x - r - 30.0f - w, q.y - 12.0f, w, h } };
            for (const Rect& t : tries)
                if (free(t))
                {
                    taken.push_back(t);
                    c.segment(q.x, q.y, std::clamp(q.x, t.x, t.right()), std::clamp(q.y, t.y + 6.0f, t.bottom() - 6.0f), 1.0f,
                              p.dim);
                    c.text(name.c_str(), t.x + 2.0f, t.y, look::kCaps, known ? p.text : p.label);
                    c.text(lv.c_str(), t.x + 2.0f, t.y + 13.0f, look::kUnit, known ? p.value : p.dim);
                    break;
                }
        }
        // The listener: its point, its facing, the handle that turns it; in the accent (and its word).
        const Point h = facingHandlePx();
        c.segment(l.x, l.y, h.x, h.y, 2.0f, p.selected);
        c.disc(h.x, h.y, 4.0f, p.ground, 1.5f, p.selected);
        c.disc(l.x, l.y, 7.0f, p.selected, 2.0f, p.ground);
        const std::string who = std::string("LISTENER, ") + (s.model == 0 ? kViewWords[std::clamp(s.camera, 0, 4)]
                                                                          : kCameraWords[std::clamp(s.camera, 0, 7)]);
        const Rect lt{ l.x + 12.0f, l.y + 6.0f, c.textWidth(who.c_str(), look::kCaps) + 4.0f, 14.0f };
        const Rect alt{ l.x - 12.0f - lt.w, l.y + 6.0f, lt.w, 14.0f };
        const Rect& use = free(lt) ? lt : alt;
        c.text(who.c_str(), use.x + 2.0f, use.y, look::kCaps, p.selected);
        // The driver's window: a handle on the car's side, down is open.
        if (!windows_.isEmpty())
        {
            const double w = field("sound.windows", kNaN);
            c.rrect(windows_.x, windows_.y, windows_.w, windows_.h, 3.0f, p.panel, 1.25f, p.edge);
            if (std::isfinite(w) && w > 0.0)
                c.rrect(windows_.x + 2.0f, windows_.y + 2.0f, windows_.w - 4.0f, (windows_.h - 4.0f) * static_cast<float>(w), 2.0f,
                        p.cold);
            c.text(("WINDOW " + fmt("%.0f%% OPEN", w * 100.0)).c_str(), windows_.x - 6.0f, windows_.bottom() + 4.0f,
                   look::kCaps, p.text, funkgui::Align::right);
        }
        // Said in words when the places are the model's, not the program's.
        bool anyPublished = false;
        for (const Source& o : s.src)
            anyPublished = anyPublished || o.published;
        if (!anyPublished)
            c.text("SOURCES AT THE MODEL'S PLACES", plan_.right() - look::kPad, plan_.y + 28.0f, look::kCaps, p.label,
                   funkgui::Align::right);
        // A scale: 1, 2 or 5 m, whichever is near 80 px.
        double len = 1.0;
        for (const double k : { 1.0, 2.0, 5.0, 10.0 })
            if (k * scale_ <= 110.0)
                len = k;
        const float sx = plan_.right() - look::kPad - static_cast<float>(len * scale_), sy = plan_.bottom() - 14.0f;
        c.segment(sx, sy, sx + static_cast<float>(len * scale_), sy, 2.0f, p.label);
        c.text((fmt("%.0f", len) + " M").c_str(), sx, sy - 15.0f, look::kCaps, p.label);
    }

    void ListenWorkspace::drawColumn(Canvas& c, const look::Palette& p, const Scene& s) const
    {
        const Rect in = look::region(c, p, column_, "WHAT IS HEARD");
        const Live& live = ctx_.live;
        float y = in.y;
        const auto row = [&](const char* label, const std::string& number, const char* unit, Col ink = kClear) {
            look::readout(c, p, in.x, y, label, number, unit, look::kNumberS, ink);
            y += 36.0f;
        };
        row("LOUDNESS TRIM", fmt("%.1f", live.get("sound.loudness")), "dB");
        const double mix = live.get("sound.mixLevel");
        row("MIX LEVEL", fmt("%.1f", mix), "dB");
        look::level(c, p, { in.x, y - 8.0f, in.w, 5.0f }, std::isfinite(mix) ? (mix + 60.0) / 60.0 : kNaN, p.text);
        y += 6.0f;
        const double lim = live.get("sound.limiter");
        const bool limiting = std::isfinite(lim) && std::fabs(lim) > 0.1;
        row("LIMITER", fmt("%.1f", -std::fabs(lim)), "dB", limiting ? p.warning : kClear);
        look::level(c, p, { in.x, y - 8.0f, in.w, 5.0f }, std::isfinite(lim) ? std::fabs(lim) / 12.0 : kNaN, p.warning);
        y += 4.0f;
        look::lamp(c, p, in.x, y + 5.0f, limiting ? "LIMITING" : "NOT LIMITING", limiting, p.warning);
        y += 22.0f;
        row("CEILING", fmt("%.0f", live.get("sound.ceilingParts")), "parts bent");
        row("RANGE", fmt("%.1f", s.range), "m");
        row("DOPPLER", fmt("%.3f", live.get("sound.doppler")), "x pitch");
        if (s.camera == 5 && s.model != 0)
            row("CAR PAST THE LISTENER", fmt("%.1f", live.get("sound.roadPosition")), "m");
        // In the driver's seat: the windows, and the four ways in with their levels, in words.
        if (s.camera == 4 && s.model != 0 && y + 100.0f < in.bottom())
        {
            y += 4.0f;
            c.hairlineH(in.x, c.snapY(y), in.w, p.grid);
            y += 8.0f;
            c.text("THE CABIN", in.x, y, look::kCaps, p.label);
            y += 16.0f;
            c.text(("Windows " + fmt("%.0f%% open", field("sound.windows", kNaN) * 100.0)).c_str(), in.x, y, look::kBody, p.text);
            y += 18.0f;
            const char* const ways[4][2] = { { "Through the floor", "sound.pathFloor" },
                                             { "Through the firewall", "sound.pathFirewall" },
                                             { "Through the glass", "sound.pathGlass" },
                                             { "Through the structure", "sound.pathStructure" } };
            for (const auto& w : ways)
            {
                if (y + 14.0f > in.bottom())
                    break;
                c.text(w[0], in.x, y, look::kBody, p.text);
                const double lv = live.get(w[1]);   // not published yet: the way in words alone
                if (std::isfinite(lv) && lv > -120.0)   // -200: not heard this way (L's)
                    c.text(fmt("%.0f dB", lv).c_str(), in.right(), y, look::kBody, p.value, funkgui::Align::right);
                y += 16.0f;
            }
        }
    }

    void ListenWorkspace::drawStage(Canvas& c, const funkgui::Theme& th) const
    {
        const look::Palette p = look::Palette::of(th);
        const Scene s = scene();
        c.rrect(bar_.x, bar_.y, bar_.w, bar_.h, look::kRadius, p.panel, 1.0f, p.edge);
        const Button* firstCam = s.model == 0 ? views_[0] : cameras_[0];
        c.text(s.model == 0 ? "VIEW" : "CAMERA", firstCam->bounds().x - 60.0f,
               c.capCentreTop(firstCam->bounds().centreY(), look::kCaps), look::kCaps, p.label);
        if (s.model != 0)
            c.text("PLACE", places_[0]->bounds().x - 48.0f, c.capCentreTop(places_[0]->bounds().centreY(), look::kCaps),
                   look::kCaps, p.label);
        drawPlan(c, p, s);
        if (s.model == 0)
            c.text("THE REFERENCE'S LISTENER: ITS FIVE VIEWS", plan_.x + look::kPad, plan_.y + 28.0f, look::kCaps, p.text);
        drawColumn(c, p, s);
    }
}
