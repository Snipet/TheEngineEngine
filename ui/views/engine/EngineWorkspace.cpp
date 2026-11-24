// ui/views/engine/EngineWorkspace.cpp: the ENGINE workspace (EngineWorkspace.h; docs/ui/engine.md).
//
// Everything on the stage is drawn from the build (EngineModel.h reads it by path) and moved by one angle: cylinder
// 1's place in its cycle, from the crank in telemetry (LIVE) or from the stage's own clock playing the Scope's last
// cycle slowly (SLOW). The block, the firing ring, the section and the loop's marker all follow that one angle, so
// they always agree. Time comes only from step(dt); drawing changes nothing.
#include "views/engine/EngineWorkspace.h"

#include "views/engine/EngineModel.h"
#include "views/engine/PageText.h"
#include "views/engine/StageDraw.h"
#include "views/engine/TwoStroke.h"

#include "app/Look.h"
#include "kit/widgets/Basic.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tee::ui
{
    namespace
    {
        using funkgui::Align;
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Point;
        using funkgui::Rect;
        using namespace engine;
        namespace lk = look;
        using stage::arcAngle;
        using stage::polar;
        using stage::put;
        using stage::putCentred;

        constexpr float kGap = 12.0f;
        constexpr double kPaces[] = { 2.0, 4.0, 8.0 };   // SLOW: seconds a cycle

        std::string fmt(double v, const char* f)
        {
            return lk::num(v, f);
        }

        // The stroke a cylinder is on at its own angle (0: the top where it fires).
        const char* strokeName(double deg, bool rotary)
        {
            if (!std::isfinite(deg))
                return "-";
            if (rotary)
                return deg < 270.0 ? "POWER" : deg < 540.0 ? "EXHAUST" : deg < 810.0 ? "INTAKE" : "COMPRESSION";
            return deg < 180.0 ? "POWER" : deg < 360.0 ? "EXHAUST" : deg < 540.0 ? "INTAKE" : "COMPRESSION";
        }

        std::string bankName(int b)
        {
            return std::string("BANK ") + static_cast<char>('A' + b);
        }
    }

    class EngineWorkspace final : public Workspace
    {
    public:
        explicit EngineWorkspace(WorkspaceContext& c) : Workspace(c)
        {
            live_ = &make<Button>("Live", [this] { slow_ = false; });
            slowBtn_ = &make<Button>("Slow", [this] { slow_ = true; });
            pace_ = &make<Button>(paceText(), [this] {
                paceIndex_ = (paceIndex_ + 1) % static_cast<int>(std::size(kPaces));
                pace_->setTitle(paceText());
            });
            linear_ = &make<Button>("Linear", [this] { log_ = false; });
            logLog_ = &make<Button>("Log-log", [this] { log_ = true; });
            live_->setLit([this] { return !slow_; });
            slowBtn_->setLit([this] { return slow_; });
            linear_->setLit([this] { return !log_; });
            logLog_->setLit([this] { return log_; });
            // on a small stage the firing ring, the loop and the numbers share one panel, chosen by these
            const char* names[3] = { "Firing", "P-V loop", "Derived" };
            for (int t = 0; t < 3; ++t)
            {
                tabs_[t] = &make<Button>(names[t], [this, t] {
                    tab_ = t;
                    arrange();
                });
                tabs_[t]->setLit([this, t] { return tab_ == t; });
            }
        }

        // The panels drawn now (for tests: no text may cross one's edge).
        std::vector<Rect> panels() const
        {
            std::vector<Rect> v{ block_, section_ };
            if (!compact_)
                v.insert(v.end(), { ring_, pv_, derived_ });
            else
                v.push_back(ring_);   // the shared panel
            return v;
        }
        bool compact() const noexcept { return compact_; }

        const char* id() const override { return "engine"; }
        const char* title() const override { return "ENGINE"; }
        std::vector<std::string> sections() const override { return { "engine" }; }
        bool moving() const override { return slow_ || Workspace::moving(); }

        std::uint64_t drawKey() const override
        {
            const double a = stageAngle();
            DrawKey k;
            k.mix(ctx_.doc.revision()).mix(ctx_.selection.revision()).mix(ctx_.units.revision());
            k.mix(std::isfinite(a) ? static_cast<std::int64_t>(std::lround(a * 4.0)) : -1);
            k.mix(slow_).mix(log_).mix(paceIndex_).mix(scopeVersion_).mix(tab_).mix(compact_);
            k.mix(static_cast<float>(ctx_.live.get(kTelSpeed)));
            k.mix(static_cast<float>(ctx_.live.get(kTelIvo))).mix(static_cast<float>(ctx_.live.get(kTelEvo)));
            for (const char* n : { kTelPowerValve, kTelCrankcase, kTelReed })
                k.mix(static_cast<float>(ctx_.live.get(n)));
            return k.h;
        }

        // Pressing a part selects it: a cylinder, the block (the layout), the ring (the layout), the section (the
        // cylinder's geometry). Elsewhere: the widgets'.
        void pointerDown(const funkgui::PointerEvent& e) override
        {
            const std::string hit = hitAt({ e.x, e.y });
            if (!hit.empty())
                ctx_.selection.select(hit);
            else
                Board::pointerDown(e);
        }
        void pointerMove(const funkgui::PointerEvent& e) override
        {
            ctx_.selection.hover(hitAt({ e.x, e.y }));
            Board::pointerMove(e);
        }

        // The inspector for what is selected: the layout (and its presets), one cylinder's place on the crank, or the
        // cylinder's geometry with its masses and friction; nothing selected: the generated pages of `engine`.
        std::string inspectorPage() const override
        {
            const std::string& s = ctx_.selection.path();
            // the field's path in this build: the branch's, else the standalone's or the demo's (never both)
            const auto both = [&](EF f, int i = -1) {
                std::vector<std::string> v;
                if (std::string x = fieldPath(ctx_.doc, f, i); !x.empty())
                    v.push_back(std::move(x));
                return v;
            };
            const auto cat = [](std::vector<std::vector<std::string>> parts) {
                std::vector<std::string> v;
                for (auto& p : parts)
                    v.insert(v.end(), p.begin(), p.end());
                return v;
            };
            const Layout& l = model().layout;
            if (s == "engine.layout")
            {
                std::vector<std::string> banks;
                for (int b = 0; b < kMaxBanks; ++b)
                    for (const std::string& x : both(EF::bankAngle, b))
                        banks.push_back(x);
                return pages::page(ctx_.doc,
                                   { { "LAYOUT", cat({ both(EF::layout), both(EF::custom), both(EF::cycle) }) },
                                     { "TWO-STROKE", l.twoStroke ? std::vector<std::string>{ "engine.tsCylinders", "engine.tsFiring" }
                                                                 : std::vector<std::string>{} },
                                     { "CUSTOM LAYOUT", cat({ both(EF::cylinders), both(EF::rotors), banks }) },
                                     { "LIMITS", { "engine.idleTrim", "engine.redlineTrim", "engine.redlineSpeed", "engine.rough" } } },
                                   "THE PRESETS ARE THE LAYOUT MENU; CUSTOM PLACES EACH CYLINDER");
            }
            if (const int k = selectedCylinder(); k >= 0)
            {
                const std::string title = "CYLINDER " + std::to_string(k + 1);
                std::string note = l.custom ? "" : "THIS CYLINDER STANDS WHERE THE LAYOUT PUTS IT; CUSTOM MOVES IT";
                return pages::page(ctx_.doc,
                                   { { title, cat({ both(EF::bank, k), both(EF::pin, k), both(EF::fireRev, k), both(EF::place, k) }) },
                                     { "LAYOUT", cat({ both(EF::layout), both(EF::custom) }) } },
                                   note);
            }
            if (s == "engine.bore")
            {
                pages::Section cylinder{ "CYLINDER", cat({ both(EF::displacement), both(EF::boreStroke), both(EF::bore), both(EF::stroke),
                                                           both(EF::rod), both(EF::compression) }) };
                pages::Section masses{ "MOVING MASSES", cat({ both(EF::pistonMass), both(EF::rodMass), both(EF::rodRecip), both(EF::flywheel),
                                                              { "engine.flywheel", "engine.balance1", "engine.balance2" } }) };
                // the build's oddities, in amber words under the fields they concern
                const Model& md = model();
                for (const Warning& w : geometryWarnings(ctx_.doc, md.layout, md.cyl, md.derived))
                    (w.fields.front() == EF::pistonMass ? masses : cylinder).warnings.push_back(w.text);
                return pages::page(ctx_.doc,
                                   { cylinder, masses,
                                     { "BURN AND SCAVENGING", md.layout.twoStroke
                                                                  ? std::vector<std::string>{ "engine.tsSpark", "engine.tsBurnDur", "engine.tsAfr",
                                                                                              "engine.tsMixing", "engine.tsShortCircuit" }
                                                                  : std::vector<std::string>{} },
                                     { "FRICTION", { "engine.friction", "engine.fricConst", "engine.fricPeak", "engine.fricSpeed",
                                                     "engine.fricSpeed2", "engine.fricBearing", "engine.frictionValvetrain", "engine.alternator",
                                                     "engine.airCon" } } });
            }
            return {};
        }

        // The selected cylinder (0-based), -1: none.
        int selectedCylinder() const
        {
            const std::string& s = ctx_.selection.path();
            static const std::string kPre = "engine.cylinder[";
            if (s.compare(0, kPre.size(), kPre) != 0)
                return -1;
            return std::atoi(s.c_str() + kPre.size());
        }

    protected:
        void layout(const Rect& s) override
        {
            const float pad = 16.0f;
            const Rect in{ s.x + pad, s.y + pad, s.w - 2 * pad, s.h - 2 * pad };
            // a small stage (the frame's at its smallest window, some 700 x 570): the block over a panel of tabs, the
            // section beside them, tall; a large one: every panel at once
            compact_ = in.w < 980.0f;
            if (compact_)
            {
                const float wL = std::floor(in.w * 0.56f), hB = std::floor(in.h * 0.5f);
                block_ = { in.x, in.y, wL, hB };
                ring_ = pv_ = derived_ = { in.x, in.y + hB + kGap, wL, in.h - hB - kGap };
                section_ = { in.x + wL + kGap, in.y, in.w - wL - kGap, in.h };
            }
            else
            {
                // BLOCK over FIRING at the left, the section in the middle, the loop over the numbers at the right
                const float wA = std::floor(in.w * 0.36f), wB = std::floor(in.w * 0.29f);
                const float wC = in.w - wA - wB - 2 * kGap;
                const float hBlock = std::floor(in.h * 0.48f);
                block_ = { in.x, in.y, wA, hBlock };
                ring_ = { in.x, in.y + hBlock + kGap, wA, in.h - hBlock - kGap };
                section_ = { in.x + wA + kGap, in.y, wB, in.h };
                const float hDer = std::clamp(std::floor(in.h * 0.26f), 170.0f, 210.0f);
                pv_ = { section_.right() + kGap, in.y, wC, in.h - hDer - kGap };
                derived_ = { pv_.x, pv_.bottom() + kGap, wC, hDer };
            }
            const float tw = std::min(84.0f, (ring_.w - 2 * lk::kPad - 8.0f) / 3.0f);
            for (int t = 0; t < 3; ++t)
                tabs_[t]->setPlace({ ring_.x + lk::kPad + (tw + 4.0f) * static_cast<float>(t), ring_.y + 6.0f, tw, 20.0f }, compact_);

            // LIVE, SLOW and the pace across the section's top; the pace's words shorten to fit
            const float bx = section_.x + lk::kPad, bwAll = section_.w - 2 * lk::kPad;
            const float by = section_.y + 30.0f, bw = std::min(64.0f, std::floor(bwAll / 4.0f));
            live_->setPlace({ bx, by, bw, 22.0f }, true);
            slowBtn_->setPlace({ bx + bw + 4.0f, by, bw, 22.0f }, true);
            const float px = bx + 2 * bw + 10.0f;
            paceShort_ = section_.right() - lk::kPad - px < 112.0f;
            pace_->setTitle(paceText());
            pace_->setPlace({ px, by, section_.right() - lk::kPad - px, 22.0f }, true);
            // LINEAR and LOG-LOG on the loop's axis line, at its right
            const Rect pc = inner(pv_);
            const float lw = 58.0f;
            const bool pvShown = !compact_ || tab_ == 1;
            logLog_->setPlace({ pc.right() - lw, pc.y - 4.0f, lw, 18.0f }, pvShown);
            linear_->setPlace({ pc.right() - 2 * lw - 4.0f, pc.y - 4.0f, lw, 18.0f }, pvShown);
        }

        void step(float dt) override
        {
            // the Scope, read while shown, again only when a newer copy is in
            const std::uint64_t g = ctx_.scope.generation();
            if (g != seenGeneration_ || !scopeRead_)
            {
                seenGeneration_ = g;
                scopeRead_ = true;
                readScope();
            }
            if (slow_)
            {
                const double cyc = model().layout.cyc;
                slowAngle_ = wrap(slowAngle_ + static_cast<double>(dt) * cyc / kPaces[paceIndex_], cyc);
            }
            else
            {
                // SLOW starts where LIVE was
                const double a = liveAngle();
                if (std::isfinite(a))
                    slowAngle_ = a;
            }
        }

        void drawStage(Canvas& c, const funkgui::Theme& th) const override
        {
            const lk::Palette p = lk::Palette::of(th);
            const Model& m = model();
            const double a1 = stageAngle();
            drawBlock(c, p, m, a1);
            drawSection(c, p, m, a1);
            if (!compact_ || tab_ == 0)
                drawRing(c, p, m, a1);
            if (!compact_ || tab_ == 1)
                drawPv(c, p, m, a1);
            if (!compact_ || tab_ == 2)
                drawDerived(c, p, m);
        }

    private:
        struct Model
        {
            Layout layout;
            Cylinder cyl;
            Derived derived;
            double liftIn = kNaN, liftEx = kNaN;   // m, the valves' full lifts when the build says
        };
        // The build's engine, made again when the document moves.
        const Model& model() const
        {
            if (modelRevision_ != ctx_.doc.revision() || !modelMade_)
            {
                modelMade_ = true;
                modelRevision_ = ctx_.doc.revision();
                model_.layout = layoutOf(ctx_.doc);
                model_.cyl = cylinderOf(ctx_.doc, model_.layout);
                double lim = fieldValue(ctx_.doc, EF::limiter);
                limiterFromTelemetry_ = false;
                if (!std::isfinite(lim) || lim <= 0.0)
                {
                    lim = ctx_.live.get(kTelRedline);
                    limiterFromTelemetry_ = std::isfinite(lim);
                }
                model_.derived = derive(model_.layout, model_.cyl, lim);
                model_.liftIn = fieldValue(ctx_.doc, FieldPath{ "valve.inLift", "valve.intakeLift" });
                model_.liftEx = fieldValue(ctx_.doc, FieldPath{ "valve.exLift", "valve.exhaustLift" });
            }
            return model_;
        }

        void readScope()
        {
            auto get = [&](const char* name, std::vector<float>& out) {
                out.assign(1080, 0.0f);
                const int h = ctx_.scope.find(name);
                const int n = h < 0 ? 0 : ctx_.scope.read(h, out.data(), 1080);
                out.resize(static_cast<std::size_t>(std::max(0, n)));
                return n;
            };
            const int np = get(kScopePressure, sp_), nv = get(kScopeVolume, sv_);
            get(kScopeLiftIn, sli_);
            get(kScopeLiftEx, sle_);
            get("portIntake", spi_);   // a two-stroke's crankcase pressure (card T)
            scopeN_ = std::min(np, nv);
            // where each lift peaks (the cams' noses point down there)
            const auto peak = [](const std::vector<float>& v) {
                if (v.empty())
                    return kNaN;
                const auto it = std::max_element(v.begin(), v.end());
                return *it > 0.0f ? static_cast<double>(it - v.begin()) : kNaN;
            };
            inPeak_ = peak(sli_);
            exPeak_ = peak(sle_);
            // where each lift leaves its seat (the first degree above 0 after one at 0)
            const auto opens = [](const std::vector<float>& v) {
                const int n = static_cast<int>(v.size());
                for (int i = 0; i < n; ++i)
                    if (v[static_cast<std::size_t>(i)] > 0.0f && v[static_cast<std::size_t>((i + n - 1) % n)] <= 0.0f)
                        return static_cast<double>(i);
                return kNaN;
            };
            inOpens_ = opens(sli_);
            exOpens_ = opens(sle_);
            ++scopeVersion_;
        }
        // Scope array at a cylinder's angle (NaN when it has none)
        static double at(const std::vector<float>& v, double deg)
        {
            if (v.empty() || !std::isfinite(deg))
                return kNaN;
            const int n = static_cast<int>(v.size());
            const int i = static_cast<int>(std::floor(wrap(deg, static_cast<double>(n))));
            return v[static_cast<std::size_t>(std::clamp(i, 0, n - 1))];
        }

        double liveAngle() const
        {
            const double crank = ctx_.live.get(kTelCrank);
            if (!std::isfinite(crank))
                return kNaN;
            return wrap(crank, model().layout.cyc);
        }
        // Cylinder 1's angle into its cycle (0: its firing top dead centre), as the stage shows it.
        double stageAngle() const { return slow_ ? slowAngle_ : liveAngle(); }

        std::string paceText() const
        {
            char b[32];
            std::snprintf(b, sizeof b, paceShort_ ? "%.0f s" : "%.0f s a cycle", kPaces[paceIndex_]);
            return b;
        }

        // ---- the block from above ----------------------------------------------------------------------------------
        struct Bore
        {
            Point at;
            float r;
        };
        struct Throw
        {
            float x;
            double pin;
            std::string label;
        };
        struct BlockGeo
        {
            std::vector<Bore> bores;
            std::vector<Rect> decks;          // a bank's casting
            std::vector<float> rowY;          // each bank's row
            Rect area{}, pins{};
            std::vector<Throw> throws;
            float throwR = 12.0f;
            float d = 0.0f;
        };
        BlockGeo blockGeo(const Layout& l) const
        {
            BlockGeo g;
            const Rect in = inner(block_);
            // the block takes the panel's width (or its height), the crank's pins right under it, the two together
            // in the middle of the room under the panel's header line
            const float head = 22.0f, avail = in.h - head;
            const float pinsH = l.rotors > 0 ? 0.0f : std::min(84.0f, avail * 0.32f);
            g.area = { in.x, in.y + head, in.w, avail - pinsH };
            g.pins = { in.x, g.area.bottom(), in.w, pinsH };
            if (l.empty())
                return g;
            const float left = 46.0f, right = l.banks > 1 ? 52.0f : 8.0f;
            double span = 0.0;
            for (int c = 0; c < l.n; ++c)
                span = std::max(span, l.along[static_cast<std::size_t>(c)]);
            // the rows' offsets across, in bores (a bank leaning the crank's way is below: the engine's right)
            std::vector<double> f(static_cast<std::size_t>(l.banks), 0.0);
            double lo = 0.0, hi = 0.0;
            for (int b = 0; b < l.banks; ++b)
            {
                f[static_cast<std::size_t>(b)] = std::sin(l.bankAngle[static_cast<std::size_t>(b)] * kPi / 180.0) * 1.15;
                lo = std::min(lo, f[static_cast<std::size_t>(b)]);
                hi = std::max(hi, f[static_cast<std::size_t>(b)]);
            }
            const float rows = static_cast<float>(hi - lo) + 1.3f;   // the block's height in bores
            const float pitchRoom = (g.area.w - left - right) / static_cast<float>(span + 1.0);
            const float sizeH = (avail - pinsH - 8.0f) / rows;
            const float pitch = std::min(pitchRoom, sizeH / 0.84f);
            g.d = std::min(pitch * 0.84f, sizeH);
            if (l.rotors > 0)
                g.d = std::min(pitch * 0.8f, avail * 0.6f);
            const float blockH = l.rotors > 0 ? g.d * 1.5f : rows * g.d;
            const float groupH = blockH + (pinsH > 0.0f ? 8.0f + pinsH : 0.0f);
            const float top0 = in.y + head + std::max(0.0f, (avail - groupH) * 0.5f);
            g.area = { in.x, top0, in.w, blockH };
            g.pins = { in.x, g.area.bottom() + 8.0f, in.w, pinsH };
            const float x0 = g.area.x + left + (g.area.w - left - right - pitch * static_cast<float>(span + 1.0)) * 0.5f
                + pitch * 0.5f;
            const float cy = l.rotors > 0 ? g.area.centreY() - g.d * 0.15f : top0 + 0.65f * g.d - static_cast<float>(lo) * g.d;
            g.rowY.resize(static_cast<std::size_t>(l.banks));
            for (int b = 0; b < l.banks; ++b)
                g.rowY[static_cast<std::size_t>(b)] = cy + static_cast<float>(f[static_cast<std::size_t>(b)]) * g.d;
            for (int c = 0; c < l.n; ++c)
            {
                const auto i = static_cast<std::size_t>(c);
                const float x = x0 + static_cast<float>(l.along[i]) * pitch;
                const int b = std::clamp(l.bank[i], 0, l.banks - 1);
                g.bores.push_back({ { x, g.rowY[static_cast<std::size_t>(b)] }, g.d * 0.5f });
            }
            for (int b = 0; b < l.banks && l.rotors == 0; ++b)
            {
                float a = 1e9f, z = -1e9f;
                for (int c = 0; c < l.n; ++c)
                    if (l.bank[static_cast<std::size_t>(c)] == b)
                    {
                        a = std::min(a, g.bores[static_cast<std::size_t>(c)].at.x);
                        z = std::max(z, g.bores[static_cast<std::size_t>(c)].at.x);
                    }
                if (a > z)
                    continue;
                const float m = g.d * 0.62f, y = g.rowY[static_cast<std::size_t>(b)];
                g.decks.push_back({ a - m, y - m, z - a + 2 * m, 2 * m });
            }
            // the crank's throws: cylinders side by side on one pin share a throw
            if (l.rotors == 0)
            {
                std::vector<int> throwOf(static_cast<std::size_t>(l.n), -1);
                for (int c = 0; c < l.n; ++c)
                {
                    const auto i = static_cast<std::size_t>(c);
                    for (std::size_t t = 0; t < g.throws.size(); ++t)
                    {
                        const double dp = std::fabs(wrap(g.throws[t].pin - l.pin[i] + 180.0, 360.0) - 180.0);
                        const int first = [&] { for (int k = 0; k < c; ++k) if (throwOf[static_cast<std::size_t>(k)] == static_cast<int>(t)) return k; return -1; }();
                        if (dp < 0.5 && first >= 0 && std::fabs(l.along[static_cast<std::size_t>(first)] - l.along[i]) < 0.6)
                        {
                            throwOf[i] = static_cast<int>(t);
                            g.throws[t].x = (g.throws[t].x + g.bores[i].at.x) * 0.5f;
                            g.throws[t].label += " " + std::to_string(c + 1);
                            break;
                        }
                    }
                    if (throwOf[i] < 0)
                    {
                        throwOf[i] = static_cast<int>(g.throws.size());
                        g.throws.push_back({ g.bores[i].at.x, l.pin[i], std::to_string(c + 1) });
                    }
                }
                std::sort(g.throws.begin(), g.throws.end(), [](const Throw& a, const Throw& b) { return a.x < b.x; });
                float gap = pitch;
                for (std::size_t t = 1; t < g.throws.size(); ++t)
                    gap = std::min(gap, g.throws[t].x - g.throws[t - 1].x);
                g.throwR = std::clamp(gap * 0.4f, 6.0f, std::min(24.0f, g.pins.h * 0.26f));
            }
            return g;
        }

        // How lit a cylinder is: 1 as it fires, falling to 0 a third of the way down its power stroke.
        static double glowOf(const Layout& l, int c, double a1)
        {
            if (!std::isfinite(a1))
                return 0.0;
            const double ca = l.cylinderAngle(c, a1);
            const double span = l.rotors > 0 ? 180.0 : 120.0;
            return ca < span ? 1.0 - ca / span : 0.0;
        }
        // The cylinder that fired last (-1: no angle).
        static int firingNow(const Layout& l, double a1)
        {
            if (!std::isfinite(a1) || l.empty())
                return -1;
            int best = 0;
            for (int c = 1; c < l.n; ++c)
                if (l.cylinderAngle(c, a1) < l.cylinderAngle(best, a1))
                    best = c;
            return best;
        }

        static Rect inner(const Rect& r) { return { r.x + lk::kPad, r.y + 26.0f, r.w - 2 * lk::kPad, r.h - 26.0f - lk::kPad }; }

        void drawBlock(Canvas& c, const lk::Palette& p, const Model& m, double a1) const
        {
            const Layout& l = m.layout;
            const bool sel = ctx_.selection.path() == "engine.layout";
            lk::region(c, p, block_, "BLOCK FROM ABOVE", sel);
            const Rect in = inner(block_);
            if (l.empty())
            {
                lk::placeholder(c, p, in, "NO LAYOUT", "This build has no engine.layout field yet, and no custom "
                                                       "cylinders: the block is drawn once it has.");
                return;
            }
            // the layout in words, and which cylinder fires now
            char head[96];
            std::snprintf(head, sizeof head, "%s, %d %s", l.name.c_str(), l.rotors > 0 ? l.rotors : l.n,
                          l.rotors > 0 ? (l.rotors == 1 ? "rotor" : "rotors") : (l.n == 1 ? "cylinder" : "cylinders"));
            put(c, head, in.x, in.y - 2.0f, lk::kNumberS, p.value);
            const int now = firingNow(l, a1);
            std::string fires = now < 0 ? "-" : std::to_string(now + 1);
            if (l.rotors > 0 && now >= 0)
                fires = "rotor " + std::to_string(now / 3 + 1) + ", face " + std::to_string(now % 3 + 1);
            const float fw = c.textWidth(fires.c_str(), lk::kNumberS);
            put(c, fires, in.right(), in.y - 2.0f, lk::kNumberS, now < 0 ? p.dim : p.hot, Align::right);
            put(c, "FIRING", in.right() - fw - 6.0f, in.y + 1.0f, lk::kCaps, p.label, Align::right);
            if (!l.ok)
                put(c, "TWO CYLINDERS FIRE TOGETHER", in.x, in.y + 14.0f, lk::kCaps, p.warning);

            const BlockGeo g = blockGeo(l);
            // the front, and the crank's line under the block
            put(c, "FRONT", g.area.x, c.capCentreTop(g.area.centreY(), lk::kCaps), lk::kCaps, p.label);
            if (!g.bores.empty())
            {
                const float y = g.rowY.empty() ? g.area.centreY() : (g.rowY.front() + g.rowY.back()) * 0.5f;
                c.segment(g.area.x + 40.0f, y, g.bores.back().at.x + g.d, y, 1.0f, p.faint);
            }
            for (const Rect& d : g.decks)
                c.rrect(d.x, d.y, d.w, d.h, std::min(10.0f, d.h * 0.3f), p.raised, 1.0f, p.edge);
            for (int b = 0; b < l.banks && l.banks > 1; ++b)
            {
                const float y = g.rowY[static_cast<std::size_t>(b)];
                const float x = g.area.right() - 48.0f;
                put(c, bankName(b), x, y - 11.0f, lk::kCaps, p.label);
                char ang[24];
                std::snprintf(ang, sizeof ang, "%+.1f deg", l.bankAngle[static_cast<std::size_t>(b)]);
                std::string s(ang);
                if (s.size() > 6 && s.compare(s.size() - 6, 2, ".0") == 0)
                    s.erase(s.size() - 6, 2);
                put(c, s, x, y + 2.0f, lk::kUnit, p.dim);
            }
            const int selCyl = selectedCylinder();
            const std::string& hov = ctx_.selection.hovered();
            if (l.rotors > 0)
            {
                // a rotary: each rotor a housing, its three faces round it, turning a third as fast as the shaft
                for (int r = 0; r < l.rotors; ++r)
                {
                    const Bore& b0 = g.bores[static_cast<std::size_t>(3 * r)];
                    const float R = g.d * 0.5f;
                    c.rrect(b0.at.x - R, b0.at.y - R * 0.86f, 2 * R, 1.72f * R, R * 0.7f, p.raised, 1.0f, p.edge);
                    put(c, "ROTOR " + std::to_string(r + 1), b0.at.x, b0.at.y + R + 6.0f, lk::kCaps, p.label, Align::centre);
                    for (int f = 0; f < 3; ++f)
                    {
                        const int cyl = 3 * r + f;
                        const double turn = std::isfinite(a1) ? (a1 + l.fire[static_cast<std::size_t>(3 * r)]) / 3.0 : 0.0;
                        const Point at = polar(b0.at, turn + 120.0 * f, R * 0.5);
                        const double gl = glowOf(l, cyl, a1);
                        const float fr = R * 0.3f;
                        c.disc(at.x, at.y, fr, Palette_mix(p, gl), 1.25f, gl > 0.0 ? p.hot : p.edge);
                        putCentred(c, std::to_string(f + 1), at.x, at.y, lk::kCaps, gl > 0.5 ? p.value : p.text);
                    }
                }
                return;
            }
            for (int cyl = 0; cyl < l.n; ++cyl)
            {
                const Bore& b = g.bores[static_cast<std::size_t>(cyl)];
                const double gl = glowOf(l, cyl, a1);
                const Col fill = Palette_mix(p, gl);
                c.disc(b.at.x, b.at.y, b.r, fill, gl > 0.0 ? 2.0f : 1.25f, gl > 0.0 ? p.hot : p.edge);
                if (b.r > 9.0f)
                    putCentred(c, std::to_string(cyl + 1), b.at.x, b.at.y, b.r > 15.0f ? lk::kNumberS : lk::kCaps,
                               gl > 0.4 ? p.value : p.text);
                const bool isSel = cyl == selCyl, isHov = hov == cylinderPath(cyl);
                if (isSel || isHov)
                    c.disc(b.at.x, b.at.y, b.r + 4.0f, kClear, isSel ? lk::kDiagram.selectRing : 1.0f,
                           isSel ? p.selected : p.dim);
            }
            // the crank's pins, from the front: each throw's circle, its pin where the crank has it now
            if (g.pins.h > 30.0f && !g.throws.empty())
            {
                put(c, std::isfinite(a1) ? "CRANK PINS, FROM THE FRONT" : "CRANK PINS AT CRANK 0: NO ANGLE", g.pins.x, g.pins.y + 2.0f,
                    lk::kCaps, p.label);
                const double theta = (std::isfinite(a1) ? a1 : 0.0) + l.bankAngle[static_cast<std::size_t>(l.bank[0])];
                const float cy = g.pins.y + 18.0f + g.throwR;
                for (const Throw& t : g.throws)
                {
                    const Point ctr{ t.x, cy };
                    c.disc(ctr.x, ctr.y, g.throwR, kClear, 1.0f, p.edge);
                    const Point pin = polar(ctr, t.pin + theta, g.throwR * 0.72);
                    c.segment(ctr.x, ctr.y, pin.x, pin.y, std::max(2.0f, g.throwR * 0.28f), p.dim);
                    c.disc(ctr.x, ctr.y, std::max(1.5f, g.throwR * 0.16f), p.label);
                    c.disc(pin.x, pin.y, std::max(2.0f, g.throwR * 0.26f), p.value);
                    put(c, t.label, ctr.x, cy + g.throwR + 4.0f, lk::kUnit, p.text, Align::centre);
                }
            }
        }

        static std::string pistonSpeedText(double ms, bool imperial)
        {
            return std::isfinite(ms) ? fmt(imperial ? ms * 196.850394 : ms, imperial ? "%.0f" : "%.1f") : std::string("-");
        }

        static Col Palette_mix(const lk::Palette& p, double glow)
        {
            return glow > 0.0 ? lk::Palette::mix(p.raised, p.hot, static_cast<float>(0.25 + 0.75 * glow)) : p.panel;
        }
        static std::string cylinderPath(int c) { return "engine.cylinder[" + std::to_string(c) + "]"; }

        // ---- derived numbers ---------------------------------------------------------------------------------------
        void drawDerived(Canvas& c, const lk::Palette& p, const Model& m) const
        {
            lk::region(c, p, derived_, compact_ ? "" : "DERIVED FROM THE BUILD");
            const Rect in = inner(derived_);
            const Derived& d = m.derived;
            const Units& u = ctx_.units;
            // a piston's speed is quoted in m/s, or ft/min where road speeds are in mph
            const bool imperial = std::string_view(u.unit(Quantity::speed).name) == "mph";
            const auto number = [&](Quantity q, double si) { return std::isfinite(si) ? u.number(q, si) : std::string("-"); };
            char lim[64] = "at the limiter";
            if (std::isfinite(d.limiter))
                std::snprintf(lim, sizeof lim, "2 S N, at %s%s", u.format(Quantity::rotation, d.limiter, 0).c_str(),
                              limiterFromTelemetry_ ? " (redline)" : "");
            struct Cell
            {
                const char* label;
                std::string value;
                std::string unit;
                std::string how;
            };
            const Cell cells[] = {
                { "DISPLACEMENT", number(Quantity::volume, d.displacement), u.unit(Quantity::volume).name,
                  m.layout.rotors > 0 ? "a rotary: see the dial" : std::to_string(m.layout.n) + " x pi/4 B^2 S" },
                { "BORE / STROKE", fmt(d.boreStroke, "%.2f"), "", d.boreStroke > 1.0 ? "oversquare" : d.boreStroke < 1.0 ? "undersquare" : "B / S" },
                { "ROD / STROKE", fmt(d.rodRatio, "%.2f"), "", m.cyl.rodDerived ? "rod 3.3 r, the reference's" : "L / S" },
                { "MEAN PISTON SPEED", pistonSpeedText(d.pistonSpeed, imperial), imperial ? "ft/min" : "m/s", lim },
                { "COMPRESSION", fmt(d.compression, "%.1f"), ": 1", "the build's ratio" },
                { "SWEPT A CYLINDER", number(Quantity::volume, m.cyl.known() ? m.cyl.swept() : kNaN),
                  u.unit(Quantity::volume).name, m.cyl.boreDerived ? "from size and B / S" : "pi/4 B^2 S" },
            };
            const int cols = in.w > 420.0f ? 3 : 2;
            const int rows = (static_cast<int>(std::size(cells)) + cols - 1) / cols;
            const float cw = in.w / static_cast<float>(cols), rh = std::min(64.0f, in.h / static_cast<float>(rows));
            for (int i = 0; i < static_cast<int>(std::size(cells)); ++i)
            {
                const float x = in.x + cw * static_cast<float>(i % cols), y = in.y + 4.0f + rh * static_cast<float>(i / cols);
                const Cell& k = cells[i];
                stage::readout(c, p, x, y, k.label, k.value, k.unit, lk::kNumberM);
                if (rh > 52.0f)
                    put(c, k.how, x, y + 38.0f, lk::kUnit, p.dim);
            }
        }

        // ---- one cylinder in section -------------------------------------------------------------------------------
        void drawSection(Canvas& c, const lk::Palette& p, const Model& m, double a1) const
        {
            const Layout& l = m.layout;
            const int cylSel = selectedCylinder();
            const int cyl = cylSel >= 0 && cylSel < l.n ? cylSel : 0;
            const bool sel = ctx_.selection.path() == "engine.bore";
            char title[48];
            std::snprintf(title, sizeof title, "CYLINDER %d IN SECTION", cyl + 1);
            lk::region(c, p, section_, title, sel);
            const Rect in = inner(section_);
            // what drives it, in words
            const char* mode = slow_ ? "SLOW: THE SCOPE'S LAST CYCLE" : "LIVE: THE CRANK AS IT TURNS";
            put(c, mode, in.x, in.y + 30.0f, lk::kCaps, slow_ ? p.selected : p.label);
            if (!slow_ && !std::isfinite(liveAngle()))
                put(c, "NO CRANK ANGLE: AT REST AT TDC", in.x, in.y + 43.0f, lk::kCaps, p.dim);
            else if (cyl != 0)
                put(c, "CYLINDER 1'S SCOPE, AT ITS ANGLE", in.x, in.y + 43.0f, lk::kCaps, p.dim);

            const Rect draw{ in.x, in.y + 56.0f, in.w, in.h - 56.0f - 98.0f };
            const Cylinder& cy = m.cyl;
            if (l.rotors > 0 || !cy.known())
            {
                lk::placeholder(c, p, draw, l.rotors > 0 ? "A ROTARY" : "NO CYLINDER YET",
                                l.rotors > 0 ? "A rotary has no piston to cut through: its faces are on the block."
                                             : "The bore and stroke are not in this build yet.");
                return;
            }
            if (l.twoStroke)
            {
                drawTwoStrokeSection(c, p, m, a1, cyl, in, draw);
                return;
            }
            const double ca = std::isfinite(a1) ? l.cylinderAngle(cyl, a1) : 0.0;   // at rest: top dead centre
            const double B = cy.bore, r = cy.radius(), L = std::isfinite(cy.rod) ? cy.rod : 3.3 * r;
            const double cr = std::isfinite(cy.compression) && cy.compression > 1.05 ? cy.compression : 10.0;
            const double clear = 2.0 * r / (cr - 1.0);               // the clearance volume as a height in the bore
            const double stems = 0.42 * B, head = 0.22 * B, pinDown = 0.38 * B, skirt = 0.62 * B, below = 0.2 * B;
            const double total = stems + head + clear + pinDown + L + 2.0 * r + below;
            const double wide = B * 1.40;
            // px a metre, the same both ways: as large as the panel allows, with room for TDC at the left, the gas's
            // number at the right and the valves' names over the cams
            const double k = std::min((draw.w - 30.0 - 78.0) / wide, (draw.h - 16.0) / total);
            const float cx = std::floor(draw.x + 30.0f + (draw.w - 30.0f - 78.0f) * 0.5f);
            const float top = draw.y + 16.0f + static_cast<float>((draw.h - 16.0 - total * k) * 0.5);
            const float headTop = top + static_cast<float>(stems * k), headBot = headTop + static_cast<float>(head * k);
            const float crownTdc = headBot + static_cast<float>(clear * k);
            const float crown = crownTdc + static_cast<float>(pistonDrop(ca, r, L) * k);
            const float crankY = crownTdc + static_cast<float>((pinDown + L + r) * k);
            const float hb = static_cast<float>(B * 0.5 * k), wall = std::max(4.0f, static_cast<float>(0.16 * B * k));
            const float linerBot = crownTdc + static_cast<float>((2.0 * r + skirt) * k);

            // the block's walls and the head
            c.rrect(cx - hb - wall, headBot, wall, linerBot - headBot, 2.0f, p.raised, 1.0f, p.edge);
            c.rrect(cx + hb, headBot, wall, linerBot - headBot, 2.0f, p.raised, 1.0f, p.edge);
            c.rrect(cx - hb - wall, headTop, 2 * (hb + wall), headBot - headTop, 3.0f, p.raised, 1.0f, p.edge);
            // the gas, tinted by its pressure (the number beside it)
            const bool turning = std::isfinite(a1);   // no angle: drawn at rest at TDC, nothing read
            const double pa = turning && scopeFits(m) ? at(sp_, ca) : kNaN;
            const Col gas = std::isfinite(pa) ? p.over(p.pressure(pa, 101325.0, 5.0e6), 0.55f) : p.panel;
            c.rrect(cx - hb + 1.0f, headBot, 2 * hb - 2.0f, std::max(1.0f, crown - headBot), 0.0f, gas);
            // the ports through the head, each in its gas's ink, faintly
            const Col inInk = p.cold, exInk = p.hot;
            const float vxIn = cx - static_cast<float>(0.24 * B * k), vxEx = cx + static_cast<float>(0.25 * B * k);
            const float portY = headTop + (headBot - headTop) * 0.3f, portH = headBot - portY - 1.0f;
            c.rrect(cx - hb - wall + 1.0f, portY, vxIn - (cx - hb - wall) + 4.0f, portH, 3.0f, p.over(inInk, 0.25f));
            c.rrect(vxEx - 4.0f, portY, cx + hb + wall - vxEx + 3.0f, portH, 3.0f, p.over(exInk, 0.25f));
            // the valves: intake at the left (cold), exhaust at the right (hot), each lifted by the Scope's curve, its
            // head opening into the chamber; over each stem its cam, turning at half the crank's speed
            const double inSlide = eventShift(kTelIvo, inOpens_), exSlide = eventShift(kTelEvo, exOpens_);
            const bool fits = scopeFits(m);
            const double li = turning && fits ? at(sli_, ca - inSlide) : kNaN, le = turning && fits ? at(sle_, ca - exSlide) : kNaN;
            const float rb = std::max(4.0f, static_cast<float>(0.09 * B * k));
            const auto valve = [&](float vx, double dia, double lift01, double fullLift, double peakDeg, Col ink) {
                const double full = std::isfinite(fullLift) ? fullLift : 0.26 * dia;   // to scale when the build says
                const float fullPx = static_cast<float>(full * k);
                const float lift = std::isfinite(lift01) ? static_cast<float>(lift01) * fullPx : 0.0f;
                const float hw = static_cast<float>(dia * 0.5 * k), face = std::max(3.0f, static_cast<float>(0.05 * B * k));
                const float faceY = headBot - face + lift, neckY = faceY - static_cast<float>(0.1 * B * k);
                const float camY = top + rb + 2.0f;
                c.segment(vx, camY + rb + lift, vx, faceY, std::max(2.0f, static_cast<float>(0.06 * B * k)), ink);
                c.segment(vx, neckY, vx - hw * 0.8f, faceY, 2.0f, ink);
                c.segment(vx, neckY, vx + hw * 0.8f, faceY, 2.0f, ink);
                c.rrect(vx - hw, faceY, 2 * hw, face, std::min(2.0f, face * 0.5f), ink);
                // the lobe: its base circle and its nose, pointing down at full lift
                if (std::isfinite(peakDeg))
                {
                    const double phi = 180.0 + (ca - peakDeg) * 0.5;
                    const Point nose = polar({ vx, camY }, phi, rb + fullPx - rb * 0.45f);
                    c.segment(vx, camY, nose.x, nose.y, rb * 0.9f, p.label);
                }
                c.disc(vx, camY, rb, p.label);
                c.disc(vx, camY, std::max(1.5f, rb * 0.25f), p.panel);
            };
            valve(vxIn, 0.40 * B, li, m.liftIn, inPeak_ + inSlide, inInk);
            valve(vxEx, 0.34 * B, le, m.liftEx, exPeak_ + exSlide, exInk);
            // each valve's name in its port, or over the cams when the port is too narrow for it
            const float inRoom = vxIn - static_cast<float>(0.2 * B * k) - (cx - hb - wall) - 6.0f;
            const float exRoom = (cx + hb + wall) - (vxEx + static_cast<float>(0.17 * B * k)) - 6.0f;
            const float nameY = portH >= 12.0f ? c.capCentreTop(portY + portH * 0.5f, lk::kCaps) : top - 14.0f;
            const bool inPort = portH >= 12.0f && inRoom >= c.textWidth("INTAKE", lk::kCaps);
            const bool exPort = portH >= 12.0f && exRoom >= c.textWidth("EXHAUST", lk::kCaps);
            put(c, "INTAKE", cx - hb - wall + 4.0f, inPort ? nameY : top - 14.0f, lk::kCaps, inInk);
            put(c, "EXHAUST", cx + hb + wall - 4.0f, exPort ? nameY : top - 14.0f, lk::kCaps, exInk, Align::right);
            // the spark plug, and the spark at the ECU's advance (a burst and a word)
            const float plugW = std::max(3.0f, static_cast<float>(0.05 * B * k));
            c.segment(cx, headTop - static_cast<float>(0.06 * B * k), cx, headBot + 1.0f, plugW, p.label);
            const double adv = ctx_.live.get(kTelAdvance);
            const double fromSpark = std::isfinite(adv) ? wrap(ca - (l.cyc - adv), l.cyc) : kNaN;
            if (turning && std::isfinite(fromSpark) && fromSpark < 14.0)
            {
                const float sp = std::max(5.0f, static_cast<float>(0.07 * B * k));
                for (int i = 0; i < 6; ++i)
                {
                    const Point a = polar({ cx, headBot + 3.0f }, 90.0 + i * 36.0, sp * 0.4);
                    const Point b = polar({ cx, headBot + 3.0f }, 90.0 + i * 36.0, sp * 1.3);
                    c.segment(a.x, a.y, b.x, b.y, 1.5f, p.warning);
                }
                put(c, "SPARK", cx + 6.0f, headBot + 4.0f + sp, lk::kCaps, p.warning);
            }
            // TDC and BDC beside the bore: the stroke
            const float crownBdc = crownTdc + static_cast<float>(2.0 * r * k);
            for (const auto& [y, word] : { std::pair{ crownTdc, "TDC" }, std::pair{ crownBdc, "BDC" } })
            {
                c.segment(cx - hb - wall - 8.0f, y, cx - hb - wall - 2.0f, y, 1.0f, p.label);
                put(c, word, cx - hb - wall - 10.0f, c.capCentreTop(y, lk::kCaps), lk::kCaps, p.label, Align::right);
            }
            // the piston, its rings and pin; the rod; the crank
            const float pinY = crown + static_cast<float>(pinDown * k);
            const double th = ca * kPi / 180.0;
            const Point crankPin{ cx + static_cast<float>(r * k * std::sin(th)), crankY - static_cast<float>(r * k * std::cos(th)) };
            const float web = static_cast<float>(r * k);
            c.arc({ cx, crankY }, web * 0.78f, arcAngle(ca + 180.0 - 60.0), arcAngle(ca + 180.0 + 60.0),
                  std::max(4.0f, web * 0.55f), p.raised);
            c.disc(cx, crankY, web, kClear, 1.0f, p.edge);
            c.segment(cx, crankY, crankPin.x, crankPin.y, std::max(4.0f, static_cast<float>(0.2 * B * k)), p.dim);
            c.disc(cx, crankY, std::max(3.0f, web * 0.3f), p.label);
            c.segment(cx, pinY, crankPin.x, crankPin.y, std::max(4.0f, static_cast<float>(0.13 * B * k)),
                      lk::Palette::mix(p.raised, p.text, 0.55f));
            c.disc(crankPin.x, crankPin.y, std::max(3.0f, static_cast<float>(0.11 * B * k)), p.text);
            c.rrect(cx - hb + 2.0f, crown, 2 * hb - 4.0f, static_cast<float>(skirt * k), 3.0f, p.raised, 1.0f, p.edge);
            for (int i = 1; i <= 2; ++i)
                c.hairlineH(cx - hb + 3.0f, crown + static_cast<float>(0.05 * B * k) * static_cast<float>(i) + 1.0f, 2 * hb - 6.0f, p.dim);
            c.disc(cx, pinY, std::max(2.5f, static_cast<float>(0.09 * B * k)), p.label);

            // beside the bore: the gas's pressure and the valves' lifts, each in its valve's ink
            const Units& u = ctx_.units;
            const float rx = cx + hb + wall + 10.0f;
            const float gy = std::max(headTop, (headBot + crown) * 0.5f - 16.0f);
            const std::string pv = std::isfinite(pa) ? u.number(Quantity::pressure, pa) : "-";
            stage::readout(c, p, rx, gy, "GAS", pv, std::isfinite(pa) ? u.unit(Quantity::pressure).name : "", lk::kNumberS);
            const auto liftText = [&](double f, double full) -> std::pair<std::string, std::string> {
                if (!std::isfinite(f))
                    return { "-", "" };
                if (std::isfinite(full))
                    return { u.number(Quantity::length, f * full), u.unit(Quantity::length).name };
                return { fmt(f * 100.0, "%.0f"), "%" };
            };
            const auto [lin, lunit] = liftText(li, m.liftIn);
            const auto [lex, eunit] = liftText(le, m.liftEx);

            // where in the cycle and which stroke; the geometry it is drawn from (to scale with each other)
            const float ry = draw.bottom() + 6.0f;
            const float colW = in.w / 3.0f;
            const std::string ang = std::isfinite(a1) ? fmt(ca, "%.0f") : "-";
            stage::readout(c, p, in.x, ry, "ANGLE", ang, "deg", lk::kNumberS);
            stage::readout(c, p, in.x + colW, ry, "STROKE", std::isfinite(a1) ? strokeName(ca, false) : "-", "", lk::kNumberS);
            stage::readout(c, p, in.x + 2 * colW, ry, "SPARK", std::isfinite(adv) ? fmt(adv, "%.0f") : "-", "BTDC", lk::kNumberS);
            stage::readout(c, p, in.x, ry + 36.0f, "IN LIFT", lin, lunit, lk::kNumberS, lin == "-" ? kClear : inInk);
            stage::readout(c, p, in.x + colW, ry + 36.0f, "EX LIFT", lex, eunit, lk::kNumberS, lex == "-" ? kClear : exInk);
            const std::string geo = u.number(Quantity::length, B) + " x " + u.number(Quantity::length, 2.0 * r) + ", rod "
                + u.number(Quantity::length, L) + " " + u.unit(Quantity::length).name;
            put(c, "BORE X STROKE", in.x, ry + 76.0f, lk::kCaps, p.label);
            put(c, geo, in.x + c.textWidth("BORE X STROKE", lk::kCaps) + 10.0f, ry + 75.0f, lk::kBody, p.value);
        }

        // ---- a two-stroke in section -------------------------------------------------------------------------------
        // The cylinder with its ports in the wall (the exhaust's at the right, the transfers' at the left, each window
        // open where the crown has passed its top edge), the crankcase under the piston as a volume tinted by its own
        // pressure, the inlet by its kind, no valves: the head is a plug and its spark at tsSpark.
        void drawTwoStrokeSection(Canvas& c, const lk::Palette& p, const Model& m, double a1, int cyl, const Rect& in,
                                  const Rect& draw) const
        {
            const Layout& l = m.layout;
            const Cylinder& cy = m.cyl;
            const TwoStroke ts = twoStrokeOf(ctx_.doc);
            const bool turning = std::isfinite(a1), fits = scopeFits(m);
            const double ca = turning ? l.cylinderAngle(cyl, a1) : 0.0;
            const double B = cy.bore, r = cy.radius(), L = std::isfinite(cy.rod) ? cy.rod : 3.3 * r;
            const double cr = std::isfinite(cy.compression) && cy.compression > 1.05 ? cy.compression : 10.0;
            const double clear = 2.0 * r / (cr - 1.0);
            const double plug = 0.14 * B, head = 0.22 * B, pinDown = 0.38 * B, skirt = 0.62 * B, below = 0.42 * B;
            const double total = plug + head + clear + pinDown + L + 2.0 * r + below;
            const double wide = std::max(B * 1.40, 2.0 * r + 0.9 * B);
            const double k = std::min((draw.w - 30.0 - 78.0) / wide, (draw.h - 4.0) / total);
            const float cx = std::floor(draw.x + 30.0f + (draw.w - 30.0f - 78.0f) * 0.5f);
            const float top = draw.y + 4.0f + static_cast<float>((draw.h - 4.0 - total * k) * 0.5);
            const float headTop = top + static_cast<float>(plug * k), headBot = headTop + static_cast<float>(head * k);
            const float crownTdc = headBot + static_cast<float>(clear * k);
            const float crownBdc = crownTdc + static_cast<float>(2.0 * r * k);
            const float crown = crownTdc + static_cast<float>(pistonDrop(ca, r, L) * k);
            const float crankY = crownTdc + static_cast<float>((pinDown + L + r) * k);
            const float hb = static_cast<float>(B * 0.5 * k), wall = std::max(5.0f, static_cast<float>(0.18 * B * k));
            const float linerBot = crownBdc + static_cast<float>(skirt * 0.6 * k);
            const float web = static_cast<float>(r * k);
            const Units& u = ctx_.units;

            // the crankcase: a volume round the crank, tinted by its pressure (the number under the drawing)
            // the crankcase's pressure: the Scope's at this angle, else (cylinder 1, live) the simulator's now
            const bool liveNow = turning && !slow_ && cyl == 0;
            double pcc = turning && fits ? at(spi_, ca) : kNaN;
            if (!std::isfinite(pcc) && liveNow)
                if (const double v = ctx_.live.get(kTelCrankcase); std::isfinite(v) && v > 0.0)
                    pcc = v;
            const Col caseInk = std::isfinite(pcc) ? p.over(p.pressure(pcc, 101325.0, 180000.0), 0.45f) : p.panel;
            const float caseW = std::max(2.0f * (hb + wall), 2.0f * web + static_cast<float>(0.5 * B * k));
            const float caseBot = crankY + web + static_cast<float>(0.3 * B * k);
            c.rrect(cx - caseW * 0.5f, linerBot - 2.0f, caseW, caseBot - linerBot + 2.0f, 8.0f, caseInk, 1.5f, p.edge);
            // the walls, the head and its plug
            c.rrect(cx - hb - wall, headBot, wall, linerBot - headBot, 2.0f, p.raised, 1.0f, p.edge);
            c.rrect(cx + hb, headBot, wall, linerBot - headBot, 2.0f, p.raised, 1.0f, p.edge);
            c.rrect(cx - hb - wall, headTop, 2 * (hb + wall), headBot - headTop, 6.0f, p.raised, 1.0f, p.edge);
            // the ports, as windows in the walls: open (bright) from the top edge down to the crown, covered below it
            const double exNow = exhaustDurationNow(ts, ctx_.live);
            const auto port = [&](bool right, double dur, Col ink) {
                const double h = portHeight(dur, r, L);
                if (!(h > 0.0))
                    return;
                const float y0 = crownBdc - static_cast<float>(h * k), y1 = crownBdc + 2.0f;
                const float x = right ? cx + hb : cx - hb - wall;
                c.rrect(x + 1.0f, y0, wall - 2.0f, y1 - y0, 2.0f, p.over(ink, 0.22f));
                if (crown > y0)
                    c.rrect(x + 1.0f, y0, wall - 2.0f, std::min(crown, y1) - y0, 2.0f, ink);
                // its passage: the exhaust's out to the right, the transfers' down beside the wall to the crankcase
                if (right)
                    c.rrect(cx + hb + wall, y0, static_cast<float>(0.22 * B * k), y1 - y0, 3.0f, p.over(ink, 0.3f));
                else
                    c.rrect(cx - hb - wall - static_cast<float>(0.1 * B * k), y0, static_cast<float>(0.1 * B * k), linerBot - y0, 2.0f,
                            p.over(ink, 0.3f));
            };
            port(true, exNow, p.hot);
            port(false, ts.trDur, p.cold);
            if (ts.hasValve() && exNow < ts.exDur - 0.5)
            {
                // the power valve's blade over the port's top: how much of it is shut now
                const float yFull = crownBdc - static_cast<float>(portHeight(ts.exDur, r, L) * k);
                const float yNow = crownBdc - static_cast<float>(portHeight(exNow, r, L) * k);
                c.rrect(cx + hb + 1.0f, yFull, wall - 2.0f, yNow - yFull, 1.0f, p.label);
            }
            // the gas over the crown, tinted by its pressure
            const double pa = turning && fits ? at(sp_, ca) : kNaN;
            const Col gas = std::isfinite(pa) ? p.over(p.pressure(pa, 101325.0, 5.0e6), 0.55f) : p.panel;
            c.rrect(cx - hb + 1.0f, headBot, 2 * hb - 2.0f, std::max(1.0f, crown - headBot), 0.0f, gas);
            // the inlet into the crankcase, at its left, by kind
            const float inY = caseBot - static_cast<float>(0.32 * B * k), inX = cx - caseW * 0.5f;
            const float duct = static_cast<float>(0.16 * B * k);
            bool inletOpen = false;
            double from = 0.0, dur = 0.0;
            if (ts.inletWindow(from, dur))
                inletOpen = turning && wrap(ca - from, 360.0) < dur;
            c.rrect(inX - static_cast<float>(0.3 * B * k), inY - duct * 0.5f, static_cast<float>(0.3 * B * k), duct, 2.0f,
                    inletOpen || ts.inlet == Inlet::reed ? p.over(p.cold, 0.45f) : p.over(p.cold, 0.18f));
            if (ts.inlet == Inlet::reed)
            {
                // the reed's petals, opened by the crankcase's pull below the air outside
                // the simulator's own opening when live, else from the crankcase's pull against tsReedFull
                const double reedNow = liveNow ? ctx_.live.get(kTelReed) : kNaN;
                const double open = std::isfinite(reedNow) ? std::clamp(reedNow, 0.0, 1.0)
                    : std::isfinite(pcc) && std::isfinite(ts.reedFull) && ts.reedFull > 0.0
                    ? std::clamp((101325.0 - pcc) / ts.reedFull, 0.0, 1.0) : 0.0;
                const float len = duct * 1.1f;
                for (const float side : { -1.0f, 1.0f })
                {
                    const Point hinge{ inX, inY + side * duct * 0.5f };
                    const Point tip = polar(hinge, 90.0 + side * (90.0 - 70.0 * open), len);
                    c.segment(hinge.x, hinge.y, tip.x, tip.y, 1.75f, p.cold);
                }
            }
            // the spark plug, and its spark at tsSpark before the top
            const float plugW = std::max(3.0f, static_cast<float>(0.05 * B * k));
            c.segment(cx, top, cx, headBot + 1.0f, plugW, p.label);
            const double fromSpark = std::isfinite(ts.spark) ? wrap(ca - (360.0 - ts.spark), 360.0) : kNaN;
            if (turning && std::isfinite(fromSpark) && fromSpark < 14.0)
            {
                const float sp = std::max(5.0f, static_cast<float>(0.07 * B * k));
                for (int i = 0; i < 6; ++i)
                {
                    const Point a = polar({ cx, headBot + 3.0f }, 90.0 + i * 36.0, sp * 0.4);
                    const Point b = polar({ cx, headBot + 3.0f }, 90.0 + i * 36.0, sp * 1.3);
                    c.segment(a.x, a.y, b.x, b.y, 1.5f, p.warning);
                }
                put(c, "SPARK", cx + 6.0f, headBot + 4.0f + sp, lk::kCaps, p.warning);
            }
            for (const auto& [y, word] : { std::pair{ crownTdc, "TDC" }, std::pair{ crownBdc, "BDC" } })
            {
                c.segment(cx - hb - wall - 8.0f - static_cast<float>(0.1 * B * k), y, cx - hb - wall - 2.0f - static_cast<float>(0.1 * B * k), y,
                          1.0f, p.label);
                put(c, word, cx - hb - wall - 10.0f - static_cast<float>(0.1 * B * k), c.capCentreTop(y, lk::kCaps), lk::kCaps, p.label,
                    Align::right);
            }
            // the piston, the rod, the crank
            const float pinY = crown + static_cast<float>(pinDown * k);
            const double th = ca * kPi / 180.0;
            const Point crankPin{ cx + static_cast<float>(r * k * std::sin(th)), crankY - static_cast<float>(r * k * std::cos(th)) };
            c.arc({ cx, crankY }, web * 0.78f, arcAngle(ca + 120.0), arcAngle(ca + 240.0), std::max(4.0f, web * 0.55f), p.raised);
            c.disc(cx, crankY, web, kClear, 1.0f, p.edge);
            c.segment(cx, crankY, crankPin.x, crankPin.y, std::max(4.0f, static_cast<float>(0.2 * B * k)), p.dim);
            c.disc(cx, crankY, std::max(3.0f, web * 0.3f), p.label);
            c.segment(cx, pinY, crankPin.x, crankPin.y, std::max(4.0f, static_cast<float>(0.13 * B * k)),
                      lk::Palette::mix(p.raised, p.text, 0.55f));
            c.disc(crankPin.x, crankPin.y, std::max(3.0f, static_cast<float>(0.11 * B * k)), p.text);
            c.rrect(cx - hb + 2.0f, crown, 2 * hb - 4.0f, static_cast<float>(skirt * k), 3.0f, p.raised, 1.0f, p.edge);
            for (int i = 1; i <= 2; ++i)
                c.hairlineH(cx - hb + 3.0f, crown + static_cast<float>(0.05 * B * k) * static_cast<float>(i) + 1.0f, 2 * hb - 6.0f, p.dim);
            c.disc(cx, pinY, std::max(2.5f, static_cast<float>(0.09 * B * k)), p.label);

            // beside the bore: the gas's pressure
            const float rx = cx + hb + wall + static_cast<float>(0.22 * B * k) + 8.0f;
            const float gy = std::max(headTop, (headBot + crown) * 0.5f - 16.0f);
            stage::readout(c, p, rx, gy, "GAS", std::isfinite(pa) ? u.number(Quantity::pressure, pa) : "-",
                           u.unit(Quantity::pressure).name, lk::kNumberS);
            // under it: where in the cycle, the ports' shares (the Scope's, else from the geometry), the crankcase
            const double exShare = turning ? (fits && !sle_.empty() ? at(sle_, ca) : portShare(ca, exNow, r, L)) : kNaN;
            const double trShare = turning ? (fits && !sli_.empty() ? at(sli_, ca) : portShare(ca, ts.trDur, r, L)) : kNaN;
            const float ry = draw.bottom() + 6.0f, colW = in.w / 3.0f;
            stage::readout(c, p, in.x, ry, "ANGLE", turning ? fmt(ca, "%.0f") : "-", "deg", lk::kNumberS);
            stage::readout(c, p, in.x + colW, ry, "PHASE", turning ? twoStrokePhase(ca, ts) : "-", "", lk::kNumberS);
            stage::readout(c, p, in.x + 2 * colW, ry, "SPARK", fmt(ts.spark, "%.0f"), "BTDC", lk::kNumberS);
            stage::readout(c, p, in.x, ry + 36.0f, "EXHAUST", fmt(exShare * 100.0, "%.0f"), "% open", lk::kNumberS,
                           std::isfinite(exShare) ? p.hot : kClear);
            stage::readout(c, p, in.x + colW, ry + 36.0f, "TRANSFERS", fmt(trShare * 100.0, "%.0f"), "% open", lk::kNumberS,
                           std::isfinite(trShare) ? p.cold : kClear);
            stage::readout(c, p, in.x + 2 * colW, ry + 36.0f, "CRANKCASE", std::isfinite(pcc) ? u.number(Quantity::pressure, pcc) : "-",
                           u.unit(Quantity::pressure).name, lk::kNumberS);
            const std::string geo = u.number(Quantity::length, B) + " x " + u.number(Quantity::length, 2.0 * r) + ", "
                + ts.inletName();
            put(c, "BORE X STROKE", in.x, ry + 76.0f, lk::kCaps, p.label);
            put(c, geo, in.x + c.textWidth("BORE X STROKE", lk::kCaps) + 10.0f, ry + 75.0f, lk::kBody, p.value);
        }

        // ---- the firing ring ---------------------------------------------------------------------------------------
        void drawRing(Canvas& c, const lk::Palette& p, const Model& m, double a1) const
        {
            const Layout& l = m.layout;
            lk::region(c, p, ring_, compact_ ? "" : "FIRING, " + fmt(l.cyc, "%.0f") + " DEGREES",
                       ctx_.selection.path() == "engine.layout");
            const Rect in = inner(ring_);
            if (l.empty())
            {
                lk::placeholder(c, p, in, "NO LAYOUT", "Firing angles come from the layout.");
                return;
            }
            const int banks = l.rotors > 0 ? 1 : l.banks;
            const float legend = 16.0f * static_cast<float>(banks > 1 ? banks + 1 : 1) + 4.0f;
            const float R = std::max(20.0f, std::min(in.w, in.h - legend) * 0.5f - 26.0f);
            const Point ctr{ in.centreX(), in.y + (in.h - legend) * 0.5f };
            c.arc(ctr, R, 0.0f, static_cast<float>(2.0 * kPi), 1.5f, p.edge);
            // the cycle's quarters, faintly: each 90 degrees of crank
            for (double d = 0.0; d < l.cyc; d += 90.0)
            {
                const Point a = polar(ctr, d / l.cyc * 360.0, R - 3.0f), b = polar(ctr, d / l.cyc * 360.0, R + 3.0f);
                c.segment(a.x, a.y, b.x, b.y, 1.0f, p.dim);
            }
            const int now = firingNow(l, a1);
            for (int cyl = 0; cyl < l.n; ++cyl)
            {
                const double ang = l.fire[static_cast<std::size_t>(cyl)] / l.cyc * 360.0;
                const bool lit = cyl == now && glowOf(l, cyl, a1) > 0.0;
                const Point a = polar(ctr, ang, R - 9.0f), b = polar(ctr, ang, R + 9.0f);
                c.segment(a.x, a.y, b.x, b.y, lit ? 3.0f : 2.0f, lit ? p.hot : p.value);
                const Point t = polar(ctr, ang, R + 18.0f);
                putCentred(c, std::to_string(cyl + 1), t.x, t.y, lk::kNumberS, lit ? p.hot : p.text);
            }
            // a ring for each bank inside: its own intervals (a crossplane V8's 180-90-180-270 a bank)
            for (int b = 0; b < banks && banks > 1; ++b)
            {
                const float rb = R * (0.72f - 0.17f * static_cast<float>(b));
                c.arc(ctr, rb, 0.0f, static_cast<float>(2.0 * kPi), 1.0f, p.faint);
                for (int cyl = 0; cyl < l.n; ++cyl)
                {
                    if (l.bank[static_cast<std::size_t>(cyl)] != b)
                        continue;
                    const double ang = l.fire[static_cast<std::size_t>(cyl)] / l.cyc * 360.0;
                    const Point a = polar(ctr, ang, rb - 5.0f), z = polar(ctr, ang, rb + 5.0f);
                    c.segment(a.x, a.y, z.x, z.y, 1.75f, cyl == now ? p.hot : p.label);
                }
            }
            // the intervals in words, in the middle
            const float inner = banks > 1 ? R * (0.72f - 0.17f * static_cast<float>(banks - 1)) - 6.0f : R - 14.0f;
            if (inner > 34.0f)
            {
                putCentred(c, "INTERVALS", ctr.x, ctr.y - 10.0f, lk::kCaps, p.label);
                std::string iv = l.intervalText();
                const funkgui::TextStyle& st = c.textWidth(iv.c_str(), lk::kNumberS) < inner * 2.0f - 8.0f ? lk::kNumberS : lk::kUnit;
                putCentred(c, iv, ctr.x, ctr.y + 6.0f, st, p.value);
            }
            // the crank, now
            if (std::isfinite(a1))
            {
                const double ang = a1 / l.cyc * 360.0;
                const Point b = polar(ctr, ang, R - 2.0f);
                const Point s = polar(ctr, ang, inner > 34.0f ? inner + 2.0f : 4.0f);
                c.segment(s.x, s.y, b.x, b.y, lk::kNeedle, p.value);
                c.disc(b.x, b.y, 4.0f, p.value);
            }
            // the legend: the order, the banks
            float ly = in.bottom() - legend + 2.0f;
            if (banks > 1)
                for (int b = 0; b < banks; ++b, ly += 16.0f)
                {
                    std::vector<double> f;
                    for (int cyl = 0; cyl < l.n; ++cyl)
                        if (l.bank[static_cast<std::size_t>(cyl)] == b)
                            f.push_back(l.fire[static_cast<std::size_t>(cyl)]);
                    std::sort(f.begin(), f.end());
                    std::string s;
                    for (std::size_t i = 0; i < f.size(); ++i)
                    {
                        const double next = i + 1 < f.size() ? f[i + 1] : f[0] + l.cyc;
                        s += (i ? "-" : "") + fmt(next - f[i], "%.0f");
                    }
                    put(c, std::string(b == 0 ? "OUTER " : "INNER ") + bankName(b), in.x, ly, lk::kCaps, p.label);
                    put(c, s, in.x + 96.0f, ly - 1.0f, lk::kBody, p.text);
                }
            put(c, "ORDER", in.x, ly, lk::kCaps, p.label);
            put(c, l.orderText(), in.x + 96.0f, ly - 1.0f, lk::kBody, p.text);
        }

        // ---- the pressure-volume loop ------------------------------------------------------------------------------
        void drawPv(Canvas& c, const lk::Palette& p, const Model& m, double a1) const
        {
            lk::region(c, p, pv_, compact_ ? "" : "PRESSURE AGAINST VOLUME");
            const Rect in = inner(pv_);
            const Rect plot{ in.x + 46.0f, in.y + 18.0f, in.w - 50.0f, in.h - 18.0f - 30.0f };
            const Units& u = ctx_.units;
            const int n = scopeN_;
            if (n < 8 || !scopeFits(m) || plot.w < 40.0f || plot.h < 30.0f)
            {
                c.rrect(plot.x, plot.y, plot.w, plot.h, 0.0f, kClear, 1.0f, p.grid);
                putCentred(c, n < 8 ? "NO SCOPE YET" : "NO SCOPE OF THIS CYCLE YET", plot.centreX(), plot.centreY(), lk::kCaps, p.dim);
                put(c, "VOLUME", plot.right(), plot.bottom() + 16.0f, lk::kCaps, p.label, Align::right);
                return;
            }
            double v0 = 1e30, v1 = 0.0, p0 = 1e30, p1 = 0.0;
            for (int i = 0; i < n; ++i)
            {
                const double v = sv_[static_cast<std::size_t>(i)], pr = sp_[static_cast<std::size_t>(i)];
                v0 = std::min(v0, v);
                v1 = std::max(v1, v);
                p0 = std::min(p0, pr);
                p1 = std::max(p1, pr);
            }
            const Quantity qv = Quantity::volume, qp = Quantity::pressure;
            // the view, in the user's units (both are absolute, above 0)
            double x0, x1, y0, y1;
            if (log_)
            {
                x0 = std::log10(std::max(1e-12, u.toUser(qv, v0 * 0.85)));
                x1 = std::log10(std::max(1e-12, u.toUser(qv, v1 * 1.15)));
                y0 = std::log10(std::max(1e-12, u.toUser(qp, p0 * 0.7)));
                y1 = std::log10(std::max(1e-12, u.toUser(qp, p1 * 1.4)));
            }
            else
            {
                x0 = 0.0;
                x1 = u.toUser(qv, v1 * 1.06);
                y0 = 0.0;
                y1 = u.toUser(qp, p1 * 1.08);
            }
            if (!(x1 > x0) || !(y1 > y0))
                return;
            const auto X = [&](double si) {
                const double uv = u.toUser(qv, si);
                const double v = log_ ? std::log10(std::max(1e-12, uv)) : uv;
                return plot.x + static_cast<float>((v - x0) / (x1 - x0)) * plot.w;
            };
            const auto Y = [&](double si) {
                const double up = u.toUser(qp, si);
                const double v = log_ ? std::log10(std::max(1e-12, up)) : up;
                return plot.bottom() - static_cast<float>((v - y0) / (y1 - y0)) * plot.h;
            };
            // grid and labels: 1-2-5 steps (linear), decades with 2 and 5 (log)
            const auto ticks = [&](double lo, double hi, bool lg, auto&& emit) {
                if (lg)
                {
                    for (int dcd = static_cast<int>(std::floor(lo)); dcd <= static_cast<int>(std::ceil(hi)); ++dcd)
                        for (double mlt : { 1.0, 2.0, 5.0 })
                        {
                            const double v = std::log10(mlt) + dcd;
                            if (v >= lo && v <= hi)
                                emit(v, std::pow(10.0, v), mlt == 1.0);
                        }
                    return;
                }
                const double raw = (hi - lo) / 5.0, mag = std::pow(10.0, std::floor(std::log10(raw)));
                const double st = raw / mag < 1.5 ? mag : raw / mag < 3.5 ? 2.0 * mag : raw / mag < 7.5 ? 5.0 * mag : 10.0 * mag;
                for (double v = std::ceil(lo / st) * st; v <= hi + 1e-9; v += st)
                    emit(v, v, true);
            };
            c.rrect(plot.x, plot.y, plot.w, plot.h, 0.0f, kClear, 1.0f, p.grid);
            const auto label = [](double v) {
                char b[24];
                const double a = std::fabs(v);
                std::snprintf(b, sizeof b, a >= 100.0 || a == 0.0 ? "%.0f" : a >= 10.0 ? "%.0f" : a >= 1.0 ? "%.1f" : a >= 0.1 ? "%.2f" : "%.3f", v);
                return std::string(b);
            };
            ticks(x0, x1, log_, [&](double pos, double val, bool major) {
                const float x = plot.x + static_cast<float>((pos - x0) / (x1 - x0)) * plot.w;
                c.hairlineV(x, plot.y, plot.h, major ? p.grid : p.over(p.grid, 0.5f));
                if (major || !log_)
                    put(c, label(val), x, plot.bottom() + 4.0f, lk::kUnit, p.label, Align::centre);
            });
            ticks(y0, y1, log_, [&](double pos, double val, bool major) {
                const float y = plot.bottom() - static_cast<float>((pos - y0) / (y1 - y0)) * plot.h;
                c.hairlineH(plot.x, y, plot.w, major ? p.grid : p.over(p.grid, 0.5f));
                if (major || !log_)
                    put(c, label(val), plot.x - 5.0f, c.capCentreTop(y, lk::kUnit), lk::kUnit, p.label, Align::right);
            });
            put(c, std::string("VOLUME, ") + u.unit(qv).name, plot.right(), plot.bottom() + 16.0f, lk::kCaps, p.label, Align::right);
            put(c, std::string("PRESSURE, ") + u.unit(qp).name, plot.x - 46.0f, in.y + 2.0f, lk::kCaps, p.label);

            // the loop, each stroke in its ink (and named in the key)
            // the loop by its phases, each in its ink and named in the key: a four-stroke's strokes; a two-stroke's power,
            // its ports open (blowdown, scavenge, trapping) and its compression
            std::vector<double> bounds;
            std::vector<const char*> words;
            std::vector<Col> inks;
            const TwoStroke ts = twoStrokeOf(ctx_.doc);
            if (m.layout.twoStroke && std::isfinite(ts.exDur))
            {
                bounds = { 0.0, ts.exOpens(), ts.exCloses(), 360.0 };
                words = { "POWER", "PORTS OPEN", "COMPRESSION" };
                inks = { p.hot, lk::Palette::mix(p.hot, p.text, 0.45f), p.value };
            }
            else
            {
                const double q = n / 4.0;
                bounds = { 0.0, q, 2 * q, 3 * q, 4 * q };
                words = { "POWER", "EXHAUST", "INTAKE", "COMPRESSION" };
                inks = { p.hot, lk::Palette::mix(p.hot, p.text, 0.45f), p.cold, p.value };
            }
            const funkgui::Canvas::ClipScope clip(c, plot);
            for (std::size_t s = 0; s + 1 < bounds.size(); ++s)
            {
                std::vector<float> xs, ys;
                const int i0 = static_cast<int>(std::floor(bounds[s])), i1 = static_cast<int>(std::ceil(bounds[s + 1]));
                for (int i = i0; i <= i1; ++i)
                {
                    const int j = i % n;
                    xs.push_back(X(sv_[static_cast<std::size_t>(j)]));
                    ys.push_back(Y(sp_[static_cast<std::size_t>(j)]));
                }
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), lk::kTrace, inks[s]);
            }
            // the key: a word a phase, stacked at the top right (where the loop never goes)
            for (std::size_t s = 0; s < words.size(); ++s)
                put(c, words[s], plot.right() - 8.0f, plot.y + 6.0f + 14.0f * static_cast<float>(s), lk::kCaps, inks[s], Align::right);
            if (std::isfinite(a1) && m.layout.rotors == 0)
            {
                const int cylSel = selectedCylinder();
                const int cyl = cylSel >= 0 && cylSel < m.layout.n ? cylSel : 0;
                const double ca = m.layout.cylinderAngle(cyl, a1);
                const float x = X(at(sv_, ca)), y = Y(at(sp_, ca));
                c.disc(x, y, 4.5f, p.selected, 1.5f, p.ground);
            }
        }

        // ---- presses -----------------------------------------------------------------------------------------------
        std::string hitAt(Point pt) const
        {
            const Model& m = model();
            if (block_.contains(pt))
            {
                const BlockGeo g = blockGeo(m.layout);
                for (std::size_t i = 0; i < g.bores.size() && m.layout.rotors == 0; ++i)
                {
                    const float dx = pt.x - g.bores[i].at.x, dy = pt.y - g.bores[i].at.y;
                    if (dx * dx + dy * dy <= (g.bores[i].r + 2.0f) * (g.bores[i].r + 2.0f))
                        return cylinderPath(static_cast<int>(i));
                }
                return "engine.layout";
            }
            if (ring_.contains(pt) && (!compact_ || tab_ == 0) && pt.y > ring_.y + 30.0f)
                return "engine.layout";
            if (section_.contains(pt) && pt.y > section_.y + 60.0f)
                return "engine.bore";
            return {};
        }

        Button *live_ = nullptr, *slowBtn_ = nullptr, *pace_ = nullptr, *linear_ = nullptr, *logLog_ = nullptr;
        bool slow_ = false, log_ = false, compact_ = false, paceShort_ = false;
        int tab_ = 0;
        Button* tabs_[3] = {};
        int paceIndex_ = 1;
        double slowAngle_ = 0.0;
        Rect block_{}, derived_{}, section_{}, ring_{}, pv_{};
        // the Scope as last read
        std::vector<float> sp_, sv_, sli_, sle_;
        int scopeN_ = 0;
        double inPeak_ = kNaN, exPeak_ = kNaN, inOpens_ = kNaN, exOpens_ = kNaN;
        std::vector<float> spi_;
        // The Scope is of this engine's cycle (720 degrees, a two-stroke's 360, a rotary's 1080): another's is not drawn.
        bool scopeFits(const Model& m) const { return scopeN_ > 0 && scopeN_ == static_cast<int>(std::lround(m.layout.cyc)); }
        // The Scope's lift slid to open where the simulator says it opens now (valve.ivo, valve.evo: the cycle's
        // degrees, the phasers' included), so the section and VALVES' ring agree; 0 when either is absent.
        double eventShift(const char* tel, double scopeOpens) const
        {
            const double now = ctx_.live.get(tel);
            if (!std::isfinite(now) || !std::isfinite(scopeOpens))
                return 0.0;
            return wrap(now - scopeOpens + 360.0, 720.0) - 360.0;
        }
        std::uint64_t seenGeneration_ = 0;
        bool scopeRead_ = false;
        std::uint32_t scopeVersion_ = 0;
        // the build's engine, by the document's revision
        mutable Model model_;
        mutable std::uint64_t modelRevision_ = 0;
        mutable bool modelMade_ = false, limiterFromTelemetry_ = false;
    };

    std::unique_ptr<Workspace> makeEngineWorkspace(WorkspaceContext& c) { return std::make_unique<EngineWorkspace>(c); }

    std::vector<funkgui::Rect> enginePanels(const Workspace& w)
    {
        const auto* e = dynamic_cast<const EngineWorkspace*>(&w);
        return e != nullptr ? e->panels() : std::vector<funkgui::Rect>{};
    }
    bool engineCompact(const Workspace& w)
    {
        const auto* e = dynamic_cast<const EngineWorkspace*>(&w);
        return e != nullptr && e->compact();
    }
}
