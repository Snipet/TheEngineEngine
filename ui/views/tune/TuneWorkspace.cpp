// ui/views/tune/TuneWorkspace.cpp: the TUNE workspace (docs/plan/ui-design.md section 3; docs/ui/tune.md). The ECU's
// maps, one at a time, as a heat map and a table of numbers at once (the kit's MapEdit), the cell the engine is in lit
// and trailed; a block of cells nudged, smoothed or filled between its corners, every edit one step of undo through the
// document (so the dyno ribbon answers); the map cut through the live cell along each axis; under it the traces that
// say whether the tune is right (lambda against its target, knock and the retard it pulled, the advance) and the
// limiter, launch control, the overrun cut, traction control and knock control as lamps with words.
//
// Fields are read by path through one table (kMaps, kTel below): a rename in the Spec is a change of one line, and a
// path that is not in the build yet draws as absent, never as a made-up value.
#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/widgets/Basic.h"
#include "kit/widgets/Plot.h"
#include "kit/widgets/Tables.h"
#include "views/dyno/Chart.h"
#include "views/tune/Tune.h"

#include <funkgui/params/ParamPort.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <string>
#include <vector>

namespace tee::ui
{
    namespace
    {
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Rect;

        // The maps, by path: the integrated Spec's first (sim/ecu/settings.fm's EcuMapsSpec), then older or demo paths.
        // `step`: a nudge, in the map's own units (SI). `live`: what its y axis is read from.
        enum class LiveY
        {
            load,      // the manifold over the air outside (a ratio), or the manifold itself on a pressure axis
            gear,
            pedal,
        };
        struct MapBind
        {
            const char* name;
            const char* paths[2];
            const char* autoPath;
            double step;
            LiveY y;
        };
        const MapBind kMaps[] = {
            { "Spark advance", { "ecu.maps.spark", "ecu.spark" }, "ecu.maps.sparkAuto", 0.5, LiveY::load },
            { "Lambda", { "ecu.maps.lambda", "fuel.lambda" }, "ecu.maps.lambdaAuto", 0.01, LiveY::load },
            { "Boost target", { "ecu.maps.boost", nullptr }, "ecu.maps.boostAuto", 5000.0, LiveY::gear },
            { "Intake cam", { "ecu.maps.camIn", nullptr }, "ecu.maps.camInAuto", 1.0, LiveY::load },
            { "Exhaust cam", { "ecu.maps.camEx", nullptr }, "ecu.maps.camExAuto", 1.0, LiveY::load },
            { "Pedal to plate", { "ecu.maps.pedal", nullptr }, "ecu.maps.pedalAuto", 0.02, LiveY::pedal },
            { "Lift command", { "ecu.maps.lift", nullptr }, "ecu.maps.liftAuto", 0.02, LiveY::load },
            { "Wastegate duty", { "boost.duty", nullptr }, nullptr, 0.02, LiveY::load },
        };
        constexpr int kMapCount = static_cast<int>(std::size(kMaps));

        // Telemetry, by name (ui/app/Workspace.h: NaN while absent).
        struct Tel
        {
            const char* speed = "engine.speed";
            const char* manifold = "engine.manifold";
            const char* throttle = "engine.throttle";
            const char* state = "engine.state";
            const char* gear = "car.gear";
            const char* load = "ecu.load";               // the maps' y axis (the manifold over the air outside)
            const char* pedal = "ctl.throttle";          // the pedal map's y axis (else the controls as sent)
            const char* lambdaAsked = "ecu.lambdaTarget";
            const char* lambdaMeasured = "ecu.lambda";   // the sensor
            const char* knock = "ecu.knock";             // a count: the knocks so far
            const char* knockByCylinder = "ecu.knockByCylinder";   // [16], each cylinder's retard
            const char* retard = "ecu.knockRetard";
            const char* advance = "ecu.advance";
            const char* ecuFlags = "ecu.flags";     // sim/ecu/ecu1.fm's ECU_FLAG_*
            const char* engineFlags = "engine.flags";
        };
        constexpr Tel kTel{};
        enum : int
        {
            kFlagLimiter = 1, kFlagFuelCut = 2, kFlagSparkCut = 4, kFlagOverrun = 8, kFlagLaunch = 16, kFlagTraction = 256,
            kFlagKnock = 1024,
        };
        constexpr double kAmbient = 101325.0;
        constexpr double kHistory = 6.0, kSampleEvery = 0.05;   // the traces' seconds, and their sampling

        // A map field of the document as the MapEdit's source; an edit is a gesture.
        struct DocMap final : MapSource
        {
            DocMap(FieldDoc& d, int f) : doc(d), field(f) {}
            Map16 map() const override { return doc.map(field); }
            void setMap(const Map16& m) override { doc.setMap(field, m); }
            void beginEdit() override { doc.beginGesture(); }
            void endEdit() override { doc.endGesture(); }
            FieldDoc& doc;
            int field;
        };

        // A switch field, chosen by a getter (the map shown), as a port.
        class SwitchPort final : public funkgui::ParamPort
        {
        public:
            SwitchPort(FieldDoc& d, std::function<int()> field) : doc_(d), field_(std::move(field)) {}
            float value01() const override
            {
                const int f = field_();
                return f >= 0 && doc_.get(f) >= 0.5 ? 1.0f : 0.0f;
            }
            float default01() const override { return 1.0f; }
            int numSteps() const override { return 1; }
            void beginGesture() override { doc_.beginGesture(); }
            void setValue01(float v) override
            {
                if (const int f = field_(); f >= 0)
                    doc_.set(f, v >= 0.5f ? 1.0 : 0.0);
            }
            void endGesture() override { doc_.endGesture(); }
            const char* id() const override { return "tune.auto"; }
            void* native() const override { return nullptr; }

        private:
            FieldDoc& doc_;
            std::function<int()> field_;
        };

        struct Sample
        {
            double lambda, target, knock, retard, advance;
            int flags;
        };

        std::string quantityUnit(const FieldInfo& f, const Units& u)
        {
            if (f.quantity != Quantity::none)
                return u.unit(f.quantity).name;
            return f.unit;
        }
        // A field's quantity, read from its fixed unit when it names one of ours ("deg", "Pa").
        Quantity zQuantity(const FieldInfo& f)
        {
            if (f.quantity != Quantity::none)
                return f.quantity;
            if (f.unit == "Pa")
                return Quantity::pressure;
            if (f.unit == "deg")
                return Quantity::angle;
            return Quantity::none;
        }
    }

    class TuneWorkspace final : public Workspace
    {
    public:
        explicit TuneWorkspace(WorkspaceContext& c) : Workspace(c)
        {
            FieldDoc& doc = ctx_.doc;
            for (int i = 0; i < kMapCount; ++i)
            {
                Slot& s = slots_[static_cast<std::size_t>(i)];
                for (const char* p : kMaps[i].paths)
                    if (p != nullptr && s.field < 0)
                    {
                        const int f = doc.find(p);
                        if (f >= 0 && doc.info(f).kind == FieldKind::map)
                            s.field = f;
                    }
                s.autoField = kMaps[i].autoPath != nullptr ? doc.find(kMaps[i].autoPath) : -1;
                if (s.field < 0)
                    continue;
                const FieldInfo& info = doc.info(s.field);
                const Units& u = ctx_.units;
                const auto text = [&u](Quantity q, int decimals) {
                    TableText t;
                    t.format = [&u, q, decimals](double si) { return u.number(q, si, decimals); };
                    t.parse = [&u, q](std::string_view v, double& si) { return u.parse(q, v, si); };
                    return t;
                };
                const auto axisName = [&u](Quantity q, const char* none) {
                    const std::string n = q == Quantity::none ? std::string() : std::string(u.unit(q).name);
                    return n.empty() ? std::string(none) : n;
                };
                const Quantity zq = zQuantity(info);
                s.source = std::make_unique<DocMap>(doc, s.field);
                MapEdit::Axis x{ axisName(info.axes[0], "x"), info.axisLo[0], info.axisHi[0], text(info.axes[0], 0) };
                MapEdit::Axis y{ axisName(info.axes[1], kMaps[i].y == LiveY::gear ? "gear" : "load"), info.axisLo[1],
                                 info.axisHi[1], text(info.axes[1], -1) };
                MapEdit::Axis z{ quantityUnit(info, u).empty() ? std::string("value") : quantityUnit(info, u), info.min,
                                 info.max, text(zq, info.decimals >= 0 ? info.decimals : (kMaps[i].step < 0.1 ? 2 : 1)) };
                s.edit = &make<MapEdit>(std::string(), *s.source, std::move(x), std::move(y), std::move(z));
                s.edit->setLive([this, i](double& lx, double& ly) { return livePoint(i, lx, ly); });
                s.edit->setPlace({}, false);
            }
            // Over the table: the trail of the live cell, and the note while the formula has the map.
            trail_ = &make<chart::Layer>([this](Canvas& cv, const funkgui::Theme& th) { drawOverMap(cv, th); },
                                         [this] { return overKey(); });
            autoPort_ = std::make_unique<SwitchPort>(doc, [this] { return slot().autoField; });
            auto_ = &make<Toggle>(*autoPort_, "Formula (auto)");
            bake_ = &make<Button>("Bake", [this] { bake(); });
            down_ = &make<Button>("-  step", [this] { nudge(-1.0); });
            up_ = &make<Button>("+  step", [this] { nudge(1.0); });
            smooth_ = &make<Button>("Smooth", [this] { smooth(); });
            fill_ = &make<Button>("Fill between corners", [this] { interpolate(); });
            sliceX_ = &make<Plot>("Along speed");
            sliceY_ = &make<Plot>("Along load");
            prev_ = &make<Button>("Prev", [this] { showMap((shown_ + kMapCount - 1) % kMapCount); });
            next_ = &make<Button>("Next", [this] { showMap((shown_ + 1) % kMapCount); });
            for (int i = 0; i < kMapCount; ++i)
                if (slots_[static_cast<std::size_t>(i)].field >= 0)
                {
                    shown_ = i;
                    break;
                }
            showMap(shown_);
        }

        const char* id() const override { return "tune"; }
        const char* title() const override { return "TUNE"; }
        std::vector<std::string> sections() const override { return { "ecu", "fuel" }; }
        bool moving() const override { return engineOn() || wantsFullRate(); }

        std::string inspectorPage() const override
        {
            // A map chosen: its formula switch and the map, named by their paths. Anything else: the generated pages
            // of the selected prefix (the limiter's, launch's, ...).
            const Slot& s = slot();
            if (s.field < 0 || !ctx_.selection.is(ctx_.doc.info(s.field).path))
                return {};
            std::string page = "section \"" + upper(kMaps[shown_].name) + "\" layout=column {\n";
            if (s.autoField >= 0)
                page += "  switch " + ctx_.doc.info(s.autoField).path + " \"TABLE\" \"FORMULA\"\n";
            page += "  map " + ctx_.doc.info(s.field).path + "\n}\n";
            return page;
        }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            k.mix(shown_).mix(ctx_.selection.revision()).mix(ctx_.units.revision()).mix(ctx_.doc.revision()).mix(message_);
            k.mix(historyRevision_);
            return k.h;
        }

        bool key(const funkgui::KeyEvent& e) override
        {
            if (Board::key(e))
                return true;
            using funkgui::Key;
            if (e.key == Key::pageUp || e.key == Key::pageDown)
            {
                nudge((e.key == Key::pageUp ? 1.0 : -1.0) * (e.mods.shift ? 5.0 : 1.0));
                return true;
            }
            if (e.key == Key::character && (e.ch == '+' || e.ch == '='))
            {
                nudge(1.0);
                return true;
            }
            if (e.key == Key::character && (e.ch == 's' || e.ch == 'S'))
            {
                smooth();
                return true;
            }
            if (e.key == Key::character && (e.ch == 'i' || e.ch == 'I'))
            {
                interpolate();
                return true;
            }
            return false;
        }

        void pointerDown(const funkgui::PointerEvent& e) override
        {
            for (int i = 0; i < kMapCount && !narrow_; ++i)
                if (listRow(i).contains({ e.x, e.y }))
                {
                    showMap(i);
                    if (slots_[static_cast<std::size_t>(i)].field >= 0)
                        ctx_.selection.select(ctx_.doc.info(slots_[static_cast<std::size_t>(i)].field).path);
                    else
                        ctx_.selection.select("ecu.maps");
                    return;
                }
            for (const Hit& h : lampHits())
                if (h.rect.contains({ e.x, e.y }))
                {
                    ctx_.selection.select(h.path);
                    return;
                }
            if (lanes_.contains({ e.x, e.y }))
            {
                const int lane = static_cast<int>((e.y - lanes_.y) / (lanes_.h / 3.0f));
                ctx_.selection.select(lane == 0 ? "fuel" : lane == 1 ? "ecu.knock" : "ecu.maps.spark");
                return;
            }
            Board::pointerDown(e);
        }

        // For tests.
        MapEdit* mapEdit() const { return slot().edit; }
        int shownMap() const noexcept { return shown_; }
        void choose(int i) { showMap(i); }
        int samples() const noexcept { return static_cast<int>(history_.size()); }
        funkgui::Rect bakeRect() const { return bake_->shown() ? bake_->bounds() : funkgui::Rect{}; }

    protected:
        void step(float dt) override
        {
            if (engineOn())
                since_ += dt;
            while (since_ >= kSampleEvery)
            {
                since_ -= kSampleEvery;
                const Live& l = ctx_.live;
                double flags = l.get(kTel.ecuFlags);
                if (!std::isfinite(flags))
                {
                    const double ef = l.get(kTel.engineFlags);   // the reference's: bit 0 the limiter, bit 1 the fuel cut
                    flags = std::isfinite(ef) ? ((static_cast<int>(ef) & 1) ? kFlagLimiter : 0) | ((static_cast<int>(ef) & 2) ? kFlagFuelCut : 0)
                                              : -1.0;
                }
                // Knocks: the count's rise since the last sample (a reset counts from 0).
                const double count = l.get(kTel.knock);
                const double knocks = !std::isfinite(count) ? count : !std::isfinite(lastCount_) || count < lastCount_ ? 0.0 : count - lastCount_;
                lastCount_ = count;
                // Each cylinder's retard, now.
                const int fc = ctx_.doc.find("engine.cylinders");
                cylinders_ = std::clamp(fc >= 0 ? static_cast<int>(std::lround(ctx_.doc.get(fc))) : 8, 1, 16);
                for (int k = 0; k < 16; ++k)
                    byCylinder_[static_cast<std::size_t>(k)] = l.get(std::string(kTel.knockByCylinder) + "[" + std::to_string(k) + "]");
                history_.push_back({ l.get(kTel.lambdaMeasured), l.get(kTel.lambdaAsked), knocks, l.get(kTel.retard),
                                     l.get(kTel.advance), std::isfinite(flags) ? static_cast<int>(flags) : -1 });
                while (static_cast<double>(history_.size()) > kHistory / kSampleEvery)
                    history_.pop_front();
                ++historyRevision_;
                // The trail: the live cell's place across the table, newest last.
                double lx = 0.0, ly = 0.0;
                const Slot& s = slot();
                if (s.edit != nullptr && livePoint(shown_, lx, ly))
                    trailPts_.push_back({ lx, ly });
                else
                    trailPts_.push_back({ std::nan(""), std::nan("") });
                while (trailPts_.size() > 40)
                    trailPts_.pop_front();
            }
            // A bake asked for: in when the Scope's count moves (the map it was asked for, even if another is shown).
            if (const float n = bakeCount(); bakeWanted_ >= 0 && n >= 1.0f && n > bakeSeen_)
            {
                const int shown = shown_;
                shown_ = bakeWanted_;
                copyBake();
                shown_ = shown;
                bakeWanted_ = -1;
            }
            // The slices follow the map and the live point.
            double lx = 0.0, ly = 0.0;
            const bool live = slot().field >= 0 && livePoint(shown_, lx, ly);
            const std::uint64_t key = DrawKey{}.mix(shown_).mix(ctx_.doc.revision()).mix(live).mix(static_cast<float>(lx))
                                          .mix(static_cast<float>(ly)).mix(ctx_.units.revision()).h;
            if (key != slicesKey_)
            {
                slicesKey_ = key;
                updateSlices();
            }
        }

        void layout(const Rect& s) override
        {
            // A narrow stage folds the list of maps into PREV and NEXT on the map's title row, and drops the slices: the
            // table keeps the room its numbers need.
            narrow_ = s.w < 900.0f;
            const float pad = 16.0f, listW = narrow_ ? 0.0f : std::clamp(s.w * 0.16f, 160.0f, 210.0f);
            const float sliceW = narrow_ ? 0.0f : std::clamp(s.w * 0.24f, 230.0f, 340.0f);
            const float traceH = std::clamp(s.h * 0.30f, 180.0f, 300.0f);
            const float topH = s.h - 3.0f * pad - traceH;
            list_ = narrow_ ? Rect{} : Rect{ s.x + pad, s.y + pad, listW, topH };
            const float mx = s.x + pad + (narrow_ ? 0.0f : listW + pad);
            mapBox_ = { mx, s.y + pad, s.x + s.w - pad - (narrow_ ? 0.0f : sliceW + pad) - mx, topH };
            slices_ = { mapBox_.right() + pad, s.y + pad, sliceW, topH };
            traces_ = { s.x + pad, s.y + pad + topH + pad, s.w - 2.0f * pad, traceH };
            prev_->setPlace({ mapBox_.right() - look::kPad - 116.0f, mapBox_.y + 4.0f, 56.0f, 20.0f }, narrow_);
            next_->setPlace({ mapBox_.right() - look::kPad - 56.0f, mapBox_.y + 4.0f, 56.0f, 20.0f }, narrow_);
            fill_->setTitle(narrow_ ? "Fill" : "Fill between corners");
            const float lampW = std::clamp(traces_.w * 0.2f, 130.0f, 190.0f);
            lampWords_ = lampW >= 170.0f;
            lanes_ = { traces_.x + look::kPad + 100.0f, traces_.y + 30.0f, traces_.w - 2.0f * look::kPad - 100.0f - lampW - 80.0f,
                       traces_.h - 30.0f - 26.0f };
            lamps_ = { traces_.right() - look::kPad - lampW, traces_.y + 30.0f, lampW, traces_.h - 30.0f - 30.0f };
            // The map's tools: the formula switch and bake at the top, the edits under the table.
            const float ix = mapBox_.x + look::kPad, iw = mapBox_.w - 2.0f * look::kPad;
            auto_->setPlace({ ix, mapBox_.y + 30.0f, std::min(230.0f, iw * 0.45f), 24.0f }, true);
            bake_->setPlace({ ix + std::min(230.0f, iw * 0.45f) + 12.0f, mapBox_.y + 30.0f, 70.0f, 24.0f }, true);
            const float ty = mapBox_.bottom() - look::kPad - 24.0f;
            float bx = ix;
            for (Button* b : { down_, up_, smooth_, fill_ })
            {
                const float w = b == fill_ ? (narrow_ ? 56.0f : 150.0f) : b == smooth_ ? 74.0f : 66.0f;
                b->setPlace({ bx, ty, w, 24.0f }, true);
                bx += w + 6.0f;
            }
            tableRect_ = { ix, mapBox_.y + 64.0f, iw, ty - 10.0f - (mapBox_.y + 64.0f) };
            for (int i = 0; i < kMapCount; ++i)
                if (MapEdit* m = slots_[static_cast<std::size_t>(i)].edit)
                    m->setPlace(tableRect_, i == shown_);
            trail_->setPlace(tableRect_, true);
            const float sh = (slices_.h - 10.0f) * 0.5f;
            sliceX_->setPlace({ slices_.x, slices_.y, slices_.w, sh }, true);
            sliceY_->setPlace({ slices_.x, slices_.y + sh + 10.0f, slices_.w, sh }, true);
            showMap(shown_);   // what the map shown has (a formula, a table) decides what is shown
        }

        void drawStage(Canvas& c, const funkgui::Theme& th) const override
        {
            const look::Palette p = look::Palette::of(th);
            drawList(c, p);
            drawMapFrame(c, p);
            drawTraces(c, p);
            drawLamps(c, p);
        }

    private:
        struct Slot
        {
            int field = -1, autoField = -1;
            std::unique_ptr<DocMap> source;
            MapEdit* edit = nullptr;
        };
        struct Hit
        {
            Rect rect;
            std::string path;
        };
        struct Pt
        {
            double x, y;
        };

        const Slot& slot() const { return slots_[static_cast<std::size_t>(shown_)]; }
        bool engineOn() const
        {
            const double st = ctx_.live.get(kTel.state);
            return !std::isfinite(st) || st >= 0.5;
        }
        bool isAuto(int i) const
        {
            const int f = slots_[static_cast<std::size_t>(i)].autoField;
            return f >= 0 && ctx_.doc.get(f) >= 0.5;
        }

        // The engine's operating point on map i's axes (false: none: stopped, or a value absent).
        bool livePoint(int i, double& x, double& y) const
        {
            const Live& l = ctx_.live;
            const double st = l.get(kTel.state);
            if (std::isfinite(st) && st < 0.5)
                return false;
            x = l.get(kTel.speed);
            const Slot& s = slots_[static_cast<std::size_t>(i)];
            const Quantity yq = s.field >= 0 ? ctx_.doc.info(s.field).axes[1] : Quantity::none;
            switch (kMaps[i].y)
            {
                case LiveY::gear: y = l.get(kTel.gear); break;
                case LiveY::pedal:
                    y = l.get(kTel.pedal);
                    if (!std::isfinite(y) && ctx_.app.controls)
                        y = ctx_.app.controls().throttle;
                    break;
                case LiveY::load:
                {
                    const double m = l.get(kTel.manifold), ld = l.get(kTel.load);
                    y = yq == Quantity::pressure ? m : std::isfinite(ld) ? ld : m / kAmbient;
                    break;
                }
            }
            return std::isfinite(x) && std::isfinite(y);
        }

        void showMap(int i)
        {
            shown_ = std::clamp(i, 0, kMapCount - 1);
            for (int k = 0; k < kMapCount; ++k)
                if (MapEdit* m = slots_[static_cast<std::size_t>(k)].edit)
                    m->setPlace(tableRect_, k == shown_ && !tableRect_.isEmpty());
            const Slot& s = slot();
            auto_->setPlace(auto_->bounds(), s.autoField >= 0);
            bake_->setPlace(bake_->bounds(), s.autoField >= 0);
            for (Button* b : { down_, up_, smooth_, fill_ })
                b->setPlace(b->bounds(), s.field >= 0);
            sliceX_->setPlace(sliceX_->bounds(), s.field >= 0 && !narrow_);
            sliceY_->setPlace(sliceY_->bounds(), s.field >= 0 && !narrow_);
            trailPts_.clear();
            message_.clear();
            updateSlices();
            bump();
        }

        // ---- edits: each one gesture of the document, one step of undo -----------------------------------------------
        bool blockOf(int& c0, int& r0, int& c1, int& r1) const
        {
            const Slot& s = slot();
            return s.edit != nullptr && s.edit->block(c0, r0, c1, r1);
        }
        void write(const Map16& m, std::string what)
        {
            const Slot& s = slot();
            ctx_.doc.beginGesture();
            ctx_.doc.setMap(s.field, m);
            ctx_.doc.endGesture();
            message_ = std::move(what);
        }
        void nudge(double times)
        {
            int c0 = 0, r0 = 0, c1 = 0, r1 = 0;
            if (!blockOf(c0, r0, c1, r1))
                return;
            const Slot& s = slot();
            const FieldInfo& info = ctx_.doc.info(s.field);
            Map16 m = ctx_.doc.map(s.field);
            const double d = kMaps[shown_].step * times;
            for (int j = r0; j <= r1; ++j)
                for (int i = c0; i <= c1; ++i)
                {
                    double& z = m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)];
                    z = std::clamp(z + d, info.min, info.max);
                }
            const Quantity q = zQuantity(info);
            write(m, std::string(d > 0 ? "Raised " : "Lowered ") + cellsWords(c0, r0, c1, r1) + " by " +
                         ctx_.units.format(q, std::fabs(d), kMaps[shown_].step < 0.1 ? 2 : 1));
        }
        void smooth()
        {
            int c0 = 0, r0 = 0, c1 = 0, r1 = 0;
            if (!blockOf(c0, r0, c1, r1))
                return;
            const Slot& s = slot();
            if (c0 == c1 && r0 == r1)   // one cell: smooth the whole map
            {
                c0 = r0 = 0;
                const Map16 now = ctx_.doc.map(s.field);
                c1 = now.nx - 1;
                r1 = now.ny - 1;
            }
            const Map16 a = ctx_.doc.map(s.field);
            Map16 m = a;
            // Each cell halfway to the mean of its neighbours (up, down, left, right, inside the map).
            for (int j = r0; j <= r1; ++j)
                for (int i = c0; i <= c1; ++i)
                {
                    double sum = 0.0;
                    int n = 0;
                    const int dx[] = { -1, 1, 0, 0 }, dy[] = { 0, 0, -1, 1 };
                    for (int k = 0; k < 4; ++k)
                    {
                        const int x = i + dx[k], y = j + dy[k];
                        if (x >= 0 && y >= 0 && x < a.nx && y < a.ny)
                        {
                            sum += a.z[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                            ++n;
                        }
                    }
                    if (n > 0)
                        m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] =
                            0.5 * a.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] + 0.5 * sum / n;
                }
            write(m, "Smoothed " + cellsWords(c0, r0, c1, r1));
        }
        void interpolate()
        {
            int c0 = 0, r0 = 0, c1 = 0, r1 = 0;
            if (!blockOf(c0, r0, c1, r1) || (c0 == c1 && r0 == r1))
            {
                message_ = "Choose a block (Shift and a click) to fill between its corners.";
                return;
            }
            Map16 m = ctx_.doc.map(slot().field);
            const auto z = [&m](int i, int j) { return m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)]; };
            const double a = z(c0, r0), b = z(c1, r0), cc = z(c0, r1), d = z(c1, r1);
            Map16 out = m;
            for (int j = r0; j <= r1; ++j)
                for (int i = c0; i <= c1; ++i)
                {
                    // Bilinear in the axes' own values, so uneven breakpoints are filled as the ECU reads between them.
                    const double tx = c1 > c0 ? (m.x[static_cast<std::size_t>(i)] - m.x[static_cast<std::size_t>(c0)]) /
                                                    (m.x[static_cast<std::size_t>(c1)] - m.x[static_cast<std::size_t>(c0)])
                                              : 0.0;
                    const double ty = r1 > r0 ? (m.y[static_cast<std::size_t>(j)] - m.y[static_cast<std::size_t>(r0)]) /
                                                    (m.y[static_cast<std::size_t>(r1)] - m.y[static_cast<std::size_t>(r0)])
                                              : 0.0;
                    out.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] =
                        (a * (1.0 - tx) + b * tx) * (1.0 - ty) + (cc * (1.0 - tx) + d * tx) * ty;
                }
            write(out, "Filled " + cellsWords(c0, r0, c1, r1) + " between its corners");
        }
        static std::string cellsWords(int c0, int r0, int c1, int r1)
        {
            const int n = (c1 - c0 + 1) * (r1 - r0 + 1);
            return n == 1 ? std::string("1 cell") : std::to_string(n) + " cells";
        }
        // Bake: the formula's values at the breakpoints, as the simulator baked them (scope.fm's baked arrays), into
        // the table, and the switch to the table. Without a bake in the Scope it says what is missing.
        // BAKE: ask the simulator for a bake (FrameServices::bakeMaps, ctl.bakeMaps) and copy it in when its count
        // moves; with no way to ask, copy the newest bake there is.
        float bakeCount() const
        {
            const int h = ctx_.scope.find("bakeCount");
            float n = -1.0f;
            if (h < 0 || ctx_.scope.read(h, &n, 1) < 1)
                return -1.0f;
            return n;
        }
        void bake()
        {
            if (ctx_.services != nullptr && ctx_.services->bakeMaps)
            {
                bakeSeen_ = bakeCount();
                bakeWanted_ = shown_;
                ctx_.services->bakeMaps();
                message_ = "Baking: the formula's values come into the table in a moment.";
                return;
            }
            copyBake();
        }
        void copyBake()
        {
            ScopeSource& sc = ctx_.scope;
            const int hCount = sc.find("bakeCount"), hNx = sc.find("bakedNx"), hNy = sc.find("bakedNy");
            const int hX = sc.find("bakedX"), hY = sc.find("bakedY"), hZ = sc.find("baked");
            // The ECU's order of maps (sim/ecu/maps.fm's MAP_*) is kMaps' first seven.
            if (shown_ >= 7 || hCount < 0 || hNx < 0 || hNy < 0 || hX < 0 || hY < 0 || hZ < 0)
            {
                message_ = "Bake needs the simulator's baked maps, which this program does not give yet.";
                return;
            }
            float count = 0.0f;
            std::vector<float> nx(7), ny(7), xs(7 * 16), ys(7 * 16), zs(7 * 256);
            if (sc.read(hCount, &count, 1) < 1 || count < 1.0f || sc.read(hNx, nx.data(), 7) < 7 || sc.read(hNy, ny.data(), 7) < 7 ||
                sc.read(hX, xs.data(), 7 * 16) < 7 * 16 || sc.read(hY, ys.data(), 7 * 16) < 7 * 16 ||
                sc.read(hZ, zs.data(), 7 * 256) < 7 * 256)
            {
                message_ = "No bake has come in yet.";
                return;
            }
            const std::size_t k = static_cast<std::size_t>(shown_);
            Map16 m;
            m.nx = std::clamp(static_cast<int>(nx[k]), 1, 16);
            m.ny = std::clamp(static_cast<int>(ny[k]), 1, 16);
            for (int i = 0; i < 16; ++i)
            {
                m.x[static_cast<std::size_t>(i)] = xs[k * 16 + static_cast<std::size_t>(i)];
                m.y[static_cast<std::size_t>(i)] = ys[k * 16 + static_cast<std::size_t>(i)];
            }
            for (int i = 0; i < m.nx; ++i)   // z[i * 16 + j] at x[i], y[j]: the table's rows are y
                for (int j = 0; j < m.ny; ++j)
                    m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)] = zs[k * 256 + static_cast<std::size_t>(i * 16 + j)];
            const Slot& s = slot();
            ctx_.doc.beginGesture();
            ctx_.doc.setMap(s.field, m);
            ctx_.doc.set(s.autoField, 0.0);
            ctx_.doc.endGesture();
            message_ = "Baked: the formula's values are now the table, and the table is in use.";
        }

        // ---- slices ------------------------------------------------------------------------------------------------
        void updateSlices()
        {
            const Slot& s = slot();
            if (s.field < 0)
                return;
            const FieldInfo& info = ctx_.doc.info(s.field);
            const Map16 m = ctx_.doc.map(s.field);
            if (m.nx < 1 || m.ny < 1)
                return;
            double lx = 0.0, ly = 0.0;
            const bool live = livePoint(shown_, lx, ly);
            if (!live)
            {
                lx = m.x[static_cast<std::size_t>(m.nx / 2)];
                ly = m.y[static_cast<std::size_t>(m.ny / 2)];
            }
            const Units& u = ctx_.units;
            const Quantity zq = zQuantity(info);
            const auto place = [](const std::array<double, kTableMax>& a, int n, double v, int& i0, double& t) {
                i0 = 0;
                t = 0.0;
                if (n <= 1 || v <= a[0])
                    return;
                for (int i = 1; i < n; ++i)
                    if (v <= a[static_cast<std::size_t>(i)])
                    {
                        i0 = i - 1;
                        t = (v - a[static_cast<std::size_t>(i - 1)]) / (a[static_cast<std::size_t>(i)] - a[static_cast<std::size_t>(i - 1)]);
                        return;
                    }
                i0 = n - 2;
                t = 1.0;
            };
            int jy = 0, ix = 0;
            double ty = 0.0, tx = 0.0;
            place(m.y, m.ny, ly, jy, ty);
            place(m.x, m.nx, lx, ix, tx);
            const auto zAt = [&m](int i, int j) { return m.z[static_cast<std::size_t>(j)][static_cast<std::size_t>(i)]; };
            const auto rowAt = [&](int i) { return m.ny > 1 ? zAt(i, jy) * (1.0 - ty) + zAt(i, jy + 1) * ty : zAt(i, 0); };
            const auto colAt = [&](int j) { return m.nx > 1 ? zAt(ix, j) * (1.0 - tx) + zAt(ix + 1, j) * tx : zAt(0, j); };
            const auto fill = [&](Plot& pl, Quantity aq, const std::array<double, kTableMax>& axis, int n, double at, bool alongX,
                                  const std::string& title) {
                double lo = 1e30, hi = -1e30;
                PlotSeries line;
                line.name = "map";
                line.points = true;
                line.width = 1.75f;
                for (int i = 0; i < n; ++i)
                {
                    const double z = u.toUser(zq, alongX ? rowAt(i) : colAt(i));
                    line.x.push_back(u.toUser(aq, axis[static_cast<std::size_t>(i)]));
                    line.y.push_back(z);
                    lo = std::min(lo, z);
                    hi = std::max(hi, z);
                }
                PlotSeries here;
                here.name = "now";
                here.points = true;
                here.width = 0.0f;
                if (live)
                {
                    here.x = { u.toUser(aq, at) };
                    here.y = { chart::at(line.x, line.y, u.toUser(aq, at)) };
                }
                const double span = std::max(hi - lo, std::fabs(hi) * 0.05 + 1e-6);
                pl.setTitle(title);
                PlotAxis& ax = pl.xAxis();
                ax.lo = line.x.front();
                ax.hi = n > 1 ? line.x.back() : line.x.front() + 1.0;
                ax.label = aq == Quantity::none ? std::string() : upper(u.unit(aq).name);
                ax.format = {};
                PlotAxis& ay = pl.yAxis();
                ay.lo = lo - span * 0.15;
                ay.hi = hi + span * 0.15;
                ay.label = upper(quantityUnit(info, u));
                ay.format = {};
                pl.series() = { line, here };
                pl.resetView();
                pl.touched();
            };
            const std::string atY = !live ? "THROUGH THE MIDDLE"
                                  : info.axes[1] == Quantity::ratio || info.axes[1] == Quantity::none
                                      ? "AT LOAD " + look::num(ly, "%.2f") : "AT " + u.format(info.axes[1], ly, -1);
            const std::string atX = live ? "AT " + u.format(info.axes[0], lx, 0) : "THROUGH THE MIDDLE";
            fill(*sliceX_, info.axes[0], m.x, m.nx, lx, true, "Along speed, " + atY);
            fill(*sliceY_, info.axes[1], m.y, m.ny, ly, false, "Along load, " + atX);
        }

        // ---- drawing -----------------------------------------------------------------------------------------------
        // The list: the maps the build has, a row each; then one line for those it has not (not rows of their own).
        int listSlot(int i) const
        {
            if (slots_[static_cast<std::size_t>(i)].field < 0)
                return -1;
            int k = 0;
            for (int j = 0; j < i; ++j)
                k += slots_[static_cast<std::size_t>(j)].field >= 0 ? 1 : 0;
            return k;
        }
        Rect listRow(int i) const
        {
            const int k = listSlot(i);
            return k < 0 ? Rect{} : Rect{ list_.x + 6.0f, list_.y + 30.0f + 40.0f * static_cast<float>(k), list_.w - 12.0f, 36.0f };
        }

        void drawList(Canvas& c, const look::Palette& p) const
        {
            if (narrow_)
                return;
            look::region(c, p, list_, "MAPS", ctx_.selection.is("ecu.maps"));
            int absent = 0, present = 0;
            for (int i = 0; i < kMapCount; ++i)
                (slots_[static_cast<std::size_t>(i)].field >= 0 ? present : absent) += 1;
            for (int i = 0; i < kMapCount; ++i)
            {
                const Rect r = listRow(i);
                if (r.isEmpty() || r.bottom() > list_.bottom() - 4.0f)
                    continue;
                const Slot& s = slots_[static_cast<std::size_t>(i)];
                const bool sel = i == shown_, there = s.field >= 0;
                if (sel)
                    c.rrect(r.x, r.y, r.w, r.h, 5.0f, p.over(p.selected, 0.12f), 1.5f, p.selected);
                c.text(kMaps[i].name, r.x + 8.0f, r.y + 5.0f, look::kBody, there ? p.value : p.dim);
                const char* state = !there ? "not in this build" : s.autoField < 0 ? "table" : isAuto(i) ? "formula (auto)" : "table";
                c.text(state, r.x + 8.0f, r.y + 21.0f, look::kUnit, there && isAuto(i) ? p.selected : p.label);
            }
            if (absent > 0)
            {
                const float y = list_.y + 30.0f + 40.0f * static_cast<float>(present) + 6.0f;
                const std::string w = std::to_string(absent) + (absent == 1 ? " more map" : " more maps") + " not in this build";
                chart::wrap(c, w, list_.x + 14.0f, y, list_.w - 28.0f, look::kUnit, p.dim, 13.0f, 2);
            }
        }

        void drawMapFrame(Canvas& c, const look::Palette& p) const
        {
            const Slot& s = slot();
            const bool sel = s.field >= 0 && ctx_.selection.is(ctx_.doc.info(s.field).path);
            look::region(c, p, mapBox_, upper(kMaps[shown_].name) + (narrow_ ? "   " + std::to_string(shown_ + 1) + " OF " +
                                                                             std::to_string(kMapCount) : std::string()), sel);
            if (s.field < 0)
            {
                look::placeholder(c, p, mapBox_, upper(kMaps[shown_].name),
                                  std::string("This build has no ") + kMaps[shown_].paths[0] +
                                      " yet. It appears here when the ECU's maps are in the Spec.");
                return;
            }
            // At the top: the formula's switch and bake (drawn as widgets), or the words that the map has no formula;
            // at the right, the words of the last edit, or how to edit.
            float left = mapBox_.x + look::kPad;
            if (s.autoField < 0)
            {
                const char* none = "No formula: this map is always its table.";
                c.text(none, left, mapBox_.y + 36.0f, look::kBody, p.label);
                left += c.textWidth(none, look::kBody) + 16.0f;
            }
            else
                left = bake_->bounds().right() + 16.0f;
            const std::string words = !message_.empty() ? message_
                                    : "Shift-click a block; Page Up, Page Down nudge it.";
            // Whole, at the top right; else under the table after the tools; else (a hint) not at all.
            const float right = mapBox_.right() - look::kPad, ww = c.textWidth(words.c_str(), look::kBody);
            const Col ink = message_.empty() ? p.dim : p.text;
            if (ww <= right - left)
                c.text(words.c_str(), right, mapBox_.y + 36.0f, look::kBody, ink, funkgui::Align::right);
            else if (ww <= right - fill_->bounds().right() - 12.0f)
                c.text(words.c_str(), right, fill_->bounds().y + 6.0f, look::kBody, ink, funkgui::Align::right);
            else if (!message_.empty())
            {
                const Canvas::ClipScope clip(c, { left, mapBox_.y, std::max(0.0f, right - left), mapBox_.h });
                c.text(words.c_str(), left, mapBox_.y + 36.0f, look::kBody, ink);
            }
        }

        // Over the table: the trail of the live point, and the formula's note while auto has the map.
        void drawOverMap(Canvas& c, const funkgui::Theme& th) const
        {
            const Slot& s = slot();
            if (s.edit == nullptr || !s.edit->shown())
                return;
            const look::Palette p = look::Palette::of(th);
            const Map16& m = s.edit->shownMap();
            if (m.nx > 0 && m.ny > 0)
            {
                const Rect a = s.edit->cellRect(0, 0);
                const auto place = [](const std::array<double, kTableMax>& ax, int n, double v) {
                    if (n <= 1 || v <= ax[0])
                        return 0.0;
                    for (int i = 1; i < n; ++i)
                        if (v <= ax[static_cast<std::size_t>(i)])
                            return (i - 1) + (v - ax[static_cast<std::size_t>(i - 1)]) / (ax[static_cast<std::size_t>(i)] - ax[static_cast<std::size_t>(i - 1)]);
                    return static_cast<double>(n - 1);
                };
                std::vector<float> xs, ys;
                for (const Pt& q : trailPts_)
                {
                    if (!std::isfinite(q.x))
                    {
                        xs.push_back(std::nanf(""));
                        ys.push_back(std::nanf(""));
                        continue;
                    }
                    xs.push_back(a.x + (static_cast<float>(place(m.x, m.nx, q.x)) + 0.5f) * a.w);
                    ys.push_back(a.y + (static_cast<float>(place(m.y, m.ny, q.y)) + 0.5f) * a.h);
                }
                const Canvas::ClipScope clip(c, s.edit->bounds());
                // Older is fainter: three stretches, each a shade.
                const std::size_t n = xs.size();
                for (int part = 0; part < 3; ++part)
                {
                    const std::size_t i0 = n * static_cast<std::size_t>(part) / 3, i1 = std::min(n, n * static_cast<std::size_t>(part + 1) / 3 + 1);
                    if (i1 <= i0 + 1)
                        continue;
                    const std::vector<float> px(xs.begin() + static_cast<long>(i0), xs.begin() + static_cast<long>(i1));
                    const std::vector<float> py(ys.begin() + static_cast<long>(i0), ys.begin() + static_cast<long>(i1));
                    chart::stroke(c, px, py, 1.5f, p.mix(p.ground, p.value, 0.25f + 0.25f * static_cast<float>(part)));
                }
            }
            if (isAuto(shown_))
            {
                const Rect b = s.edit->bounds();
                const std::string words = "FORMULA IN USE: the ECU makes this map itself; the table is kept, not read. "
                                          "Bake copies the formula in.";
                const float w = b.w - 54.0f, lines = c.textWidth(words.c_str(), look::kBody) > w - 20.0f ? 2.0f : 1.0f;
                const Rect note{ b.x + 54.0f, b.bottom() - 12.0f - 15.0f * lines, w, 12.0f + 15.0f * lines };
                c.rrect(note.x, note.y, note.w, note.h, 4.0f, p.raised, 1.5f, p.selected);
                chart::wrap(c, words, note.x + 10.0f, note.y + 6.0f, w - 20.0f, look::kBody, p.value, 15.0f, 2);
            }
        }
        std::uint64_t overKey() const
        {
            DrawKey k;
            k.mix(shown_).mix(static_cast<int>(trailPts_.size())).mix(historyRevision_).mix(ctx_.doc.revision());
            return k.h;
        }

        void drawTraces(Canvas& c, const look::Palette& p) const
        {
            look::region(c, p, traces_, "IS THE TUNE RIGHT?  THE LAST 6 SECONDS");
            const float laneH = lanes_.h / 3.0f;
            const int n = static_cast<int>(history_.size());
            const auto xOf = [&](int i) {
                return lanes_.x + lanes_.w * static_cast<float>(i + static_cast<int>(kHistory / kSampleEvery) - n) /
                                      static_cast<float>(kHistory / kSampleEvery - 1.0);
            };
            const Sample* last = n > 0 ? &history_.back() : nullptr;
            const auto lane = [&](int k, const char* label, const char* sel, double lo, double hi,
                                  const std::function<void(const chart::Scale&)>& body, const std::string& now,
                                  const std::string& unit) {
                const Rect r{ lanes_.x, lanes_.y + laneH * static_cast<float>(k) + 4.0f, lanes_.w, laneH - 8.0f };
                const bool isSel = ctx_.selection.is(sel);
                c.text(label, traces_.x + look::kPad, r.y + 2.0f, look::kCaps, isSel ? p.selected : p.label);
                c.rrect(r.x, r.y, r.w, r.h, 3.0f, p.ground);
                const chart::Scale ys{ lo, hi, r.bottom() - 2.0f, r.y + 2.0f };
                c.text(chart::fixed(hi, hi < 3.0 ? 2 : 0).c_str(), r.x - 6.0f, r.y, look::kUnit, p.dim, funkgui::Align::right);
                c.text(chart::fixed(lo, hi < 3.0 ? 2 : 0).c_str(), r.x - 6.0f, r.bottom() - 11.0f, look::kUnit, p.dim,
                       funkgui::Align::right);
                {
                    const Canvas::ClipScope clip(c, r);
                    body(ys);
                }
                c.text(now.c_str(), r.right() + 62.0f, r.y + 2.0f, look::kNumberS, now == "-" ? p.dim : p.value, funkgui::Align::right);
                c.text(unit.c_str(), r.right() + 62.0f, r.y + 18.0f, look::kUnit, p.label, funkgui::Align::right);
            };
            const auto series = [&](double Sample::*field, const chart::Scale& ys, float w, Col ink, chart::Dash d) {
                std::vector<float> xs, yv;
                for (int i = 0; i < n; ++i)
                {
                    const double v = history_[static_cast<std::size_t>(i)].*field;
                    xs.push_back(xOf(i));
                    yv.push_back(std::isfinite(v) ? ys.px(v) : std::nanf(""));
                }
                chart::stroke(c, xs, yv, w, ink, d);
            };
            // Lambda: the sensor's (solid) against what was asked (dashed); the gap is what is wrong.
            lane(0, "LAMBDA", "fuel", 0.7, 1.2, [&](const chart::Scale& ys) {
                c.hairlineH(lanes_.x, ys.px(1.0), lanes_.w, p.grid);
                series(&Sample::target, ys, 1.5f, p.label, { 5.0f, 4.0f });
                series(&Sample::lambda, ys, 1.75f, p.cold, {});
            }, last != nullptr ? look::num(last->lambda, "%.2f") : "-",
                 last != nullptr ? "asked " + look::num(last->target, "%.2f") : "asked -");
            // Knock: its intensity as bars, the retard pulled as a line (degrees).
            double most = 5.0;
            for (const Sample& s : history_)
                if (std::isfinite(s.retard))
                    most = std::max(most, s.retard * 1.2);
            lane(1, "KNOCK", "ecu.knock", 0.0, most, [&](const chart::Scale& ys) {
                for (int i = 0; i < n; ++i)
                {
                    const double k = history_[static_cast<std::size_t>(i)].knock;
                    if (std::isfinite(k) && k > 0.0)
                    {
                        const float x = xOf(i);
                        c.segment(x, ys.px(0.0), x, ys.px(0.0) - static_cast<float>(std::min(1.0, k / 3.0)) * (ys.p0 - ys.p1), 2.0f, p.warning);
                    }
                }
                series(&Sample::retard, ys, 1.75f, p.hot, {});
            }, last != nullptr ? look::num(last->retard, "%.1f") : "-", "deg pulled");
            // Advance.
            double lo = 1e9, hi = -1e9;
            for (const Sample& s : history_)
                if (std::isfinite(s.advance))
                {
                    lo = std::min(lo, s.advance);
                    hi = std::max(hi, s.advance);
                }
            if (hi < lo)
            {
                lo = 0.0;
                hi = 40.0;
            }
            lo = std::floor(lo / 5.0) * 5.0 - 5.0;
            hi = std::ceil(hi / 5.0) * 5.0 + 5.0;
            lane(2, "ADVANCE", "ecu.maps.spark", lo, hi, [&](const chart::Scale& ys) { series(&Sample::advance, ys, 1.75f, p.selected, {}); },
                 last != nullptr ? look::num(last->advance, "%.1f") : "-", "deg BTDC");
            // The key: what each stroke is.
            float kx = lanes_.x;
            const float ky = traces_.bottom() - 18.0f;
            const auto key = [&](const char* word, float w, Col ink, chart::Dash d, bool bar) {
                if (bar)
                    c.segment(kx + 6.0f, ky + 9.0f, kx + 6.0f, ky + 1.0f, 2.0f, ink);
                else
                    chart::sample(c, kx, kx + 22.0f, ky + 5.0f, w, ink, d);
                c.text(word, kx + 28.0f, ky, look::kCaps, p.label);
                kx += 34.0f + c.textWidth(word, look::kCaps) + 14.0f;
            };
            key("LAMBDA SENSED", 1.75f, p.cold, {}, false);
            key("ASKED", 1.5f, p.label, { 5.0f, 4.0f }, false);
            key("KNOCK", 2.0f, p.warning, {}, true);
            key("RETARD", 1.75f, p.hot, {}, false);
            key("ADVANCE", 1.75f, p.selected, {}, false);
            if (n == 0 || (last != nullptr && !std::isfinite(last->lambda) && !std::isfinite(last->knock) && !std::isfinite(last->advance)))
            {
                const char* w = "-  THE ECU'S LIVE VALUES ARE NOT IN THIS PROGRAM YET";
                c.text(w, traces_.right() - look::kPad, traces_.y + 9.0f, look::kCaps, p.dim, funkgui::Align::right);
            }
        }

        std::vector<Hit> lampHits() const
        {
            static const char* const paths[] = { "ecu.limiter", "ecu.launch", "ecu.overrun", "ecu.traction", "ecu.knock" };
            std::vector<Hit> out;
            const float top = lamps_.y + kCylinderH, h = std::min(30.0f, (lamps_.h - kCylinderH) / 5.0f);
            for (int i = 0; i < 5; ++i)
                out.push_back({ { lamps_.x, top + h * static_cast<float>(i), lamps_.w, h }, paths[i] });
            return out;
        }

        // Over the lamps: each cylinder's knock retard as a small bar, numbered (what a tuner reads first).
        static constexpr float kCylinderH = 46.0f;
        void drawCylinders(Canvas& c, const look::Palette& p) const
        {
            const bool sel = ctx_.selection.is("ecu.knock");
            c.text("KNOCK BY CYLINDER", lamps_.x + 6.0f, lamps_.y - 2.0f, look::kCaps, sel ? p.selected : p.label);
            bool any = false;
            double most = 3.0;
            for (int k = 0; k < cylinders_; ++k)
                if (std::isfinite(byCylinder_[static_cast<std::size_t>(k)]))
                {
                    any = true;
                    most = std::max(most, byCylinder_[static_cast<std::size_t>(k)]);
                }
            if (!any)
            {
                c.text("-  not in this program yet", lamps_.x + 6.0f, lamps_.y + 16.0f, look::kUnit, p.dim);
                return;
            }
            const float x0 = lamps_.x + 6.0f, w = lamps_.w - 12.0f, gap = 2.0f;
            const float bw = (w - gap * static_cast<float>(cylinders_ - 1)) / static_cast<float>(cylinders_);
            const float base = lamps_.y + 30.0f, hh = 16.0f;
            const bool numbers = bw >= 12.0f || cylinders_ <= 9;
            for (int k = 0; k < cylinders_; ++k)
            {
                const double v = byCylinder_[static_cast<std::size_t>(k)];
                const float x = x0 + static_cast<float>(k) * (bw + gap);
                c.rrect(x, base - hh, bw, hh, 1.5f, p.ground);
                if (std::isfinite(v) && v > 0.0)
                {
                    const float bh = std::max(1.5f, hh * static_cast<float>(std::min(1.0, v / most)));
                    c.rrect(x, base - bh, bw, bh, 1.5f, v > 0.5 ? p.warning : p.label);
                }
                if (numbers || k == 0 || k == cylinders_ - 1)
                    c.text(std::to_string(k + 1).c_str(), x + bw * 0.5f, base + 2.0f, look::kUnit, p.dim, funkgui::Align::centre);
            }
        }

        void drawLamps(Canvas& c, const look::Palette& p) const
        {
            drawCylinders(c, p);
            const int flags = history_.empty() ? -1 : history_.back().flags;
            const bool known = flags >= 0;
            const double limiter = [&] {
                const int f = ctx_.doc.find("ecu.limiter.speed");
                const int g = f >= 0 ? f : ctx_.doc.find("ecu.limiter");
                return g >= 0 && ctx_.doc.info(g).kind == FieldKind::number ? ctx_.doc.get(g) : std::nan("");
            }();
            struct L
            {
                const char* word;
                bool on;
                Col ink;
                std::string active, rest;
            };
            const bool cut = known && (flags & kFlagFuelCut) != 0, spark = known && (flags & kFlagSparkCut) != 0;
            const L lamps[] = {
                { "LIMITER", known && (flags & kFlagLimiter) != 0, p.warning,
                  std::string(spark ? "spark cut" : cut ? "fuel cut" : "holding") +
                      (std::isfinite(limiter) && limiter > 0.0 ? " at " + ctx_.units.format(Quantity::rotation, limiter, 0) : ""),
                  "" },
                { "LAUNCH", known && (flags & kFlagLaunch) != 0, p.selected, "holding the launch speed", "" },
                { "OVERRUN CUT", known && (flags & kFlagOverrun) != 0, p.hot, "fuel off, lift", "" },
                { "TRACTION", known && (flags & kFlagTraction) != 0, p.warning, "pulling power", "" },
                { "KNOCK", known && (flags & kFlagKnock) != 0, p.warning, "retarding", "" },
            };
            const std::vector<Hit> hits = lampHits();
            for (std::size_t i = 0; i < hits.size(); ++i)
            {
                const L& l = lamps[i];
                const Rect& r = hits[i].rect;
                const bool sel = ctx_.selection.is(hits[i].path);
                if (sel)
                    look::selectRing(c, p, { r.x + 2.0f, r.y + 3.0f, r.w - 4.0f, r.h - 6.0f }, 4.0f);
                look::lamp(c, p, r.x + 6.0f, r.centreY(), l.word, l.on, l.ink);
                const std::string w = !known ? "-" : l.on ? (lampWords_ ? l.active : std::string("on")) : "off";
                c.text(w.c_str(), r.right() - 6.0f, r.centreY() - 5.0f, look::kUnit, l.on ? p.value : p.dim, funkgui::Align::right);
            }
        }

        std::array<Slot, kMapCount> slots_{};
        int shown_ = 0;
        chart::Layer* trail_ = nullptr;
        std::unique_ptr<SwitchPort> autoPort_;
        Toggle* auto_ = nullptr;
        Button *bake_ = nullptr, *down_ = nullptr, *up_ = nullptr, *smooth_ = nullptr, *fill_ = nullptr;
        Plot *sliceX_ = nullptr, *sliceY_ = nullptr;
        Button *prev_ = nullptr, *next_ = nullptr;
        bool narrow_ = false, lampWords_ = true;
        std::deque<Sample> history_;
        std::deque<Pt> trailPts_;
        std::uint64_t historyRevision_ = 0;
        double since_ = 0.0, lastCount_ = std::nan("");
        int cylinders_ = 8;
        std::array<double, 16> byCylinder_{};
        int bakeWanted_ = -1;
        float bakeSeen_ = -1.0f;
        std::uint64_t slicesKey_ = 0;
        std::string message_;
        Rect list_{}, mapBox_{}, slices_{}, traces_{}, lanes_{}, lamps_{}, tableRect_{};
    };

    std::unique_ptr<Workspace> makeTuneWorkspace(WorkspaceContext& c) { return std::make_unique<TuneWorkspace>(c); }
}

namespace tee::ui
{
    MapEdit* tuneMapEdit(Workspace& w)
    {
        auto* t = dynamic_cast<TuneWorkspace*>(&w);
        return t != nullptr ? t->mapEdit() : nullptr;
    }
    void tuneChoose(Workspace& w, int map)
    {
        if (auto* t = dynamic_cast<TuneWorkspace*>(&w))
            t->choose(map);
    }
    funkgui::Rect tuneBakeButton(Workspace& w)
    {
        auto* t = dynamic_cast<TuneWorkspace*>(&w);
        return t != nullptr ? t->bakeRect() : funkgui::Rect{};
    }
}
