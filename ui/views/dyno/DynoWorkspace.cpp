// ui/views/dyno/DynoWorkspace.cpp: the DYNO workspace (docs/plan/ui-design.md section 3; docs/ui/dyno.md). The big
// version of the dyno ribbon: torque and power against engine speed on two axes with their units, the peaks marked
// with their numbers, a cursor that reads every trace at the pointer's speed, a third quantity (boost, lambda,
// advance) in a band under it; the runs listed, shown or hidden, renamed, copied out as CSV, overlaid in strokes that
// differ by dash and weight as well as colour; a run planned, started, watched and stopped.
//
// Everything comes through DynoSource (U1's, over card C's runner): predicted() (the build as it stands), before()
// (the curve before the newest edit) and the runs. What a run is planned with is this workspace's own (DynoRequest):
// it is not a Spec field, so it is set on the stage, not in the inspector.
#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/widgets/Basic.h"
#include "views/dyno/Chart.h"
#include "views/dyno/Dyno.h"

#include <funkgui/params/ParamPort.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace tee::ui
{
    namespace
    {
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Rect;
        using chart::Dash;

        // The quantities a run may carry besides torque and power, by their telemetry names (one line each: a rename
        // in the Spec is a change here). Shown in the band under the plot.
        struct Extra
        {
            const char* field;
            const char* label;
            Quantity quantity;
            double scale;   // a value from the trace times this is SI for `quantity` (1 for most)
        };
        const Extra kExtras[] = {
            { "boost.pressure", "Boost", Quantity::pressure, 1.0 },
            { "ecu.lambda", "Lambda", Quantity::none, 1.0 },
            { "ecu.spark", "Advance", Quantity::angle, 1.0 },
        };
        constexpr int kExtraCount = static_cast<int>(std::size(kExtras));

        // A value of the plan, as a ParamPort over a getter and a setter, in `steps` even steps from lo to hi.
        class PlanPort final : public funkgui::ParamPort
        {
        public:
            PlanPort(const char* id, std::function<double()> get, std::function<void(double)> set, double lo, double hi,
                     int steps, double def)
                : id_(id), get_(std::move(get)), set_(std::move(set)), lo_(lo), hi_(hi), steps_(steps), def_(def)
            {
            }
            float value01() const override { return to01(get_()); }
            float default01() const override { return to01(def_); }
            int numSteps() const override { return steps_; }
            void beginGesture() override {}
            void setValue01(float v) override
            {
                const double s = std::round(std::clamp(static_cast<double>(v), 0.0, 1.0) * steps_) / steps_;
                set_(lo_ + s * (hi_ - lo_));
            }
            void endGesture() override {}
            const char* id() const override { return id_; }
            void* native() const override { return nullptr; }
            double at01(float v) const { return lo_ + std::round(static_cast<double>(v) * steps_) / steps_ * (hi_ - lo_); }

        private:
            float to01(double v) const { return static_cast<float>(std::clamp((v - lo_) / (hi_ - lo_), 0.0, 1.0)); }
            const char* id_;
            std::function<double()> get_;
            std::function<void(double)> set_;
            double lo_, hi_;
            int steps_;
            double def_;
        };

        // A row's stroke: its colour and dash. Torque is drawn heavier than power whatever the row.
        struct Stroke
        {
            Col ink;
            Dash dash;
        };
        constexpr float kTorqueW = 2.25f, kPowerW = 1.4f;
    }

    class DynoWorkspace final : public Workspace
    {
    public:
        explicit DynoWorkspace(WorkspaceContext& c) : Workspace(c)
        {
            engineBtn_ = &make<Button>("Engine", [this] { plan_.chassis = false; });
            engineBtn_->setLit([this] { return !plan_.chassis; });
            chassisBtn_ = &make<Button>("Chassis", [this] { plan_.chassis = true; });
            chassisBtn_->setLit([this] { return plan_.chassis; });
            const auto rpmText = [this](PlanPort* port) {
                return [this, port](float v) { return ctx_.units.format(Quantity::rotation, port->at01(v), 0); };
            };
            fromPort_ = std::make_unique<PlanPort>("dyno.from", [this] { return plan_.from; },
                                                   [this](double v) { plan_.from = std::min(v, plan_.to - 500.0 * chart::kRpm); },
                                                   500.0 * chart::kRpm, 9000.0 * chart::kRpm, 34, 1500.0 * chart::kRpm);
            toPort_ = std::make_unique<PlanPort>("dyno.to", [this] { return plan_.to; },
                                                 [this](double v) { plan_.to = std::max(v, plan_.from + 500.0 * chart::kRpm); },
                                                 500.0 * chart::kRpm, 9000.0 * chart::kRpm, 34, 7000.0 * chart::kRpm);
            ratePort_ = std::make_unique<PlanPort>("dyno.rate", [this] { return plan_.rate; },
                                                   [this](double v) { plan_.rate = v; }, 100.0 * chart::kRpm,
                                                   3000.0 * chart::kRpm, 29, 600.0 * chart::kRpm);
            pedalPort_ = std::make_unique<PlanPort>("dyno.pedal", [this] { return plan_.pedal; },
                                                    [this](double v) { plan_.pedal = v; }, 0.0, 1.0, 20, 1.0);
            gearPort_ = std::make_unique<PlanPort>("dyno.gear", [this] { return static_cast<double>(gear_); },
                                                   [this](double v) { gear_ = static_cast<int>(std::lround(v)); }, 1.0,
                                                   6.0, 5, 4.0);
            from_ = &make<Stepper>(*fromPort_, "From", rpmText(fromPort_.get()));
            to_ = &make<Stepper>(*toPort_, "To", rpmText(toPort_.get()));
            rate_ = &make<Stepper>(*ratePort_, "Rate", [this](float v) {
                return ctx_.units.number(Quantity::rotation, ratePort_->at01(v), 0) + " " +
                       ctx_.units.unit(Quantity::rotation).name + "/s";
            });
            sweepBtn_ = &make<Button>("Sweep", [this] { stepped_ = false; });
            sweepBtn_->setLit([this] { return !stepped_; });
            stepsBtn_ = &make<Button>("Steps", [this] { stepped_ = true; });
            stepsBtn_->setLit([this] { return stepped_; });
            dwellPort_ = std::make_unique<PlanPort>("dyno.dwell", [this] { return plan_.dwell; }, [this](double v) { plan_.dwell = v; },
                                                    0.2, 3.0, 14, 0.6);
            dwell_ = &make<Stepper>(*dwellPort_, "Dwell", [this](float v) { return look::num(dwellPort_->at01(v), "%.1f") + " s"; });
            pedal_ = &make<Stepper>(*pedalPort_, "Pedal", [this](float v) { return look::num(pedalPort_->at01(v) * 100.0) + " %"; });
            gearStep_ = &make<Stepper>(*gearPort_, "Gear", [this](float v) { return look::num(gearPort_->at01(v)); });
            start_ = &make<Button>("Start run", [this] { startRun(); });
            stop_ = &make<Button>("Stop", [this] {
                if (ctx_.dyno.running())
                {
                    ctx_.dyno.stopRun();
                    message_ = "Stopped: the runner keeps what it measured, if it keeps anything.";
                }
            });
            name_ = &make<TextField>("Name", [this] { return rowName(selected_); },
                                     [this](const std::string& s) { return rename(selected_, s); }, 24);
            copy_ = &make<Button>(canSave() ? "Save CSV" : "Copy CSV", [this] { copyCsv(); });
            const char* extraNames[] = { "None", kExtras[0].label, kExtras[1].label, kExtras[2].label };
            for (int i = 0; i <= kExtraCount; ++i)
            {
                extraBtn_[static_cast<std::size_t>(i)] = &make<Button>(extraNames[i], [this, i] { extra_ = i - 1; });
                extraBtn_[static_cast<std::size_t>(i)]->setLit([this, i] { return extra_ == i - 1; });
            }
        }

        const char* id() const override { return "dyno"; }
        const char* title() const override { return "DYNO"; }
        std::vector<std::string> sections() const override { return {}; }
        bool ribbon() const override { return false; }
        bool moving() const override { return ctx_.dyno.running() || ctx_.dyno.busy() || wantsFullRate(); }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            k.mix(ctx_.dyno.revision()).mix(ctx_.dyno.running()).mix(ctx_.dyno.runCount()).mix(selected_).mix(extra_);
            k.mix(cursorPx_).mix(ctx_.units.revision()).mix(static_cast<int>(runTime_ * 10.0f)).mix(plan_.chassis);
            k.mix(static_cast<float>(plan_.from)).mix(static_cast<float>(plan_.to)).mix(static_cast<float>(plan_.pedal));
            for (const RowView& r : rows_)
                k.mix(r.shown).mix(r.name);
            k.mix(showPredicted_).mix(showBefore_).mix(message_).mix(stepped_);
            if (const DynoTrace* so = ctx_.dyno.runSoFar())
                k.mix(static_cast<int>(so->speed.size()));
            return k.h;
        }

        // ---- input -------------------------------------------------------------------------------------------------
        void pointerMove(const funkgui::PointerEvent& e) override
        {
            cursorPx_ = plotArea_.contains({ e.x, e.y }) || bandArea_.contains({ e.x, e.y })
                            ? static_cast<int>(std::lround(e.x))
                            : -1;
            Board::pointerMove(e);
        }
        void pointerExit() override
        {
            cursorPx_ = -1;
            Board::pointerExit();
        }
        void pointerDown(const funkgui::PointerEvent& e) override
        {
            for (const RowHit& h : rowLayout())
                if (h.rect.contains({ e.x, e.y }))
                {
                    if (h.box.contains({ e.x, e.y }))
                        setShown(h.row, !isShown(h.row));
                    else
                        selected_ = h.row;
                    return;
                }
            Board::pointerDown(e);
        }

        funkgui::Rect control(std::string_view which) const
        {
            const Widget* w = which == "start" ? static_cast<const Widget*>(start_) : which == "stop" ? stop_
                            : which == "name" ? static_cast<const Widget*>(name_) : which == "copy" ? copy_
                            : which == "engine" ? engineBtn_ : which == "chassis" ? chassisBtn_ : nullptr;
            const char* names[] = { "none", "boost", "lambda", "advance" };
            for (int i = 0; i <= kExtraCount && w == nullptr; ++i)
                if (which == names[i])
                    w = extraBtn_[static_cast<std::size_t>(i)];
            return w != nullptr ? w->bounds() : funkgui::Rect{};
        }

        // For tests: the rows (-2 predicted, -1 before, 0.. runs), and the cursor's speed (rad/s; NaN: none).
        int selectedRow() const noexcept { return selected_; }
        double cursorSpeed() const noexcept
        {
            computeScales();
            return cursorPx_ < 0 ? std::nan("") : xScale_.data(static_cast<float>(cursorPx_)) * xToSi_;
        }

    protected:
        void step(float dt) override
        {
            // Runs that came in: a row each, the newest chosen.
            const int n = ctx_.dyno.runCount();
            while (static_cast<int>(rows_.size()) < n)
            {
                const int i = static_cast<int>(rows_.size());
                rows_.push_back({ ctx_.dyno.run(i).name, true });
                selected_ = i;
                const DynoTrace& t = ctx_.dyno.run(i);
                message_ = rows_.back().name + " is in: " + peakWords(t);
            }
            if (gearStep_->shown() != plan_.chassis)
                gearStep_->setPlace(gearStep_->bounds(), plan_.chassis);
            if (rate_->shown() == stepped_)
            {
                rate_->setPlace(rate_->bounds(), !stepped_);
                dwell_->setPlace(rate_->bounds(), stepped_);
            }
            if (ctx_.dyno.running())
                runTime_ += dt;
            else if (runTime_ > 0.0f)
                runTime_ = 0.0f;
        }

        void layout(const Rect& s) override
        {
            const float pad = 16.0f, header = 52.0f;
            const float sideW = std::clamp(s.w * 0.3f, 220.0f, 330.0f);
            const Rect plotBox{ s.x + pad, s.y + pad + header, s.w - 3.0f * pad - sideW, s.h - 2.0f * pad - header };
            const float bandH = std::clamp(plotBox.h * 0.26f, 96.0f, 170.0f);
            plotBox_ = { plotBox.x, plotBox.y, plotBox.w, plotBox.h - bandH - 10.0f };
            bandBox_ = { plotBox.x, plotBox_.bottom() + 10.0f, plotBox.w, bandH };
            const float ml = 62.0f, mr = 62.0f;
            plotArea_ = { plotBox_.x + ml, plotBox_.y + 40.0f, plotBox_.w - ml - mr, plotBox_.h - 40.0f - 40.0f };
            bandArea_ = { bandBox_.x + ml, bandBox_.y + 34.0f, bandBox_.w - ml - mr, bandBox_.h - 34.0f - 26.0f };
            header_ = { s.x + pad, s.y + pad, s.w - 2.0f * pad, header - 8.0f };

            // The band's choice of quantity: small buttons in its title row, at the right.
            float bx = bandBox_.right() - look::kPad;
            const float bw = std::clamp((bandBox_.w - 2.0f * look::kPad - 120.0f) / static_cast<float>(kExtraCount + 1), 60.0f, 68.0f);
            for (int i = kExtraCount; i >= 0; --i)
            {
                const float w = bw;
                bx -= w;
                extraBtn_[static_cast<std::size_t>(i)]->setPlace({ bx, bandBox_.y + 6.0f, w - 4.0f, 20.0f }, true);
            }

            // The side: the plan at the bottom, the runs above it.
            const float sx = plotBox.right() + pad;
            const float planH = 306.0f;
            planBox_ = { sx, s.bottom() - pad - planH, sideW, planH };
            runsBox_ = { sx, plotBox.y, sideW, planBox_.y - 10.0f - plotBox.y };
            const float ix = sx + look::kPad, iw = sideW - 2.0f * look::kPad;
            float y = planBox_.y + 30.0f;
            engineBtn_->setPlace({ ix, y, iw * 0.5f - 3.0f, 26.0f }, true);
            chassisBtn_->setPlace({ ix + iw * 0.5f + 3.0f, y, iw * 0.5f - 3.0f, 26.0f }, true);
            y += 30.0f;
            sweepBtn_->setPlace({ ix, y, iw * 0.5f - 3.0f, 22.0f }, true);
            stepsBtn_->setPlace({ ix + iw * 0.5f + 3.0f, y, iw * 0.5f - 3.0f, 22.0f }, true);
            y += 30.0f;
            for (Stepper* st : { from_, to_, rate_, pedal_, gearStep_ })
            {
                st->setPlace({ ix, y, iw, 24.0f }, true);
                if (st == rate_)
                    dwell_->setPlace({ ix, y, iw, 24.0f }, false);   // the same row: a sweep's rate or a step's dwell
                y += 27.0f;
            }
            y += 4.0f;
            start_->setPlace({ ix, y, iw * 0.62f - 3.0f, 28.0f }, true);
            stop_->setPlace({ ix + iw * 0.62f + 3.0f, y, iw * 0.38f - 3.0f, 28.0f }, true);
            wordsY_ = y + 36.0f;

            // Under the runs' rows: the selected run's name, and its CSV.
            const float fy = runsBox_.bottom() - look::kPad - 28.0f;
            name_->setPlace({ ix, fy, iw * 0.6f - 3.0f, 28.0f }, true);
            copy_->setPlace({ ix + iw * 0.6f + 3.0f, fy, iw * 0.4f - 3.0f, 28.0f }, true);
            rowsArea_ = { ix, runsBox_.y + 30.0f, iw, fy - 8.0f - (runsBox_.y + 30.0f) };
        }

        void drawStage(Canvas& c, const funkgui::Theme& th) const override
        {
            const look::Palette p = look::Palette::of(th);
            computeScales();
            drawHeader(c, p);
            drawPlot(c, p);
            drawWalls(c, p);
            drawBand(c, p);
            drawRuns(c, p);
            drawPlan(c, p);
        }

    private:
        struct RowView
        {
            std::string name;
            bool shown = true;
        };
        struct RowHit
        {
            Rect rect, box;
            int row;
        };

        // ---- rows: -2 predicted, -1 before the last edit, 0.. the runs ----------------------------------------------
        const DynoTrace& trace(int row) const
        {
            return row == -2 ? ctx_.dyno.predicted() : row == -1 ? ctx_.dyno.before() : ctx_.dyno.run(row);
        }
        std::string rowName(int row) const
        {
            if (row == -2)
                return "Predicted";
            if (row == -1)
                return "Before last edit";
            return row >= 0 && row < static_cast<int>(rows_.size()) ? rows_[static_cast<std::size_t>(row)].name : "-";
        }
        bool isShown(int row) const
        {
            if (row == -2)
                return showPredicted_;
            if (row == -1)
                return showBefore_;
            return row >= 0 && row < static_cast<int>(rows_.size()) && rows_[static_cast<std::size_t>(row)].shown;
        }
        void setShown(int row, bool on)
        {
            if (row == -2)
                showPredicted_ = on;
            else if (row == -1)
                showBefore_ = on;
            else if (row >= 0 && row < static_cast<int>(rows_.size()))
                rows_[static_cast<std::size_t>(row)].shown = on;
        }
        bool rename(int row, const std::string& s)
        {
            if (row < 0 || row >= static_cast<int>(rows_.size()) || s.empty())
            {
                message_ = row < 0 ? "Only a run can be renamed." : message_;
                return row < 0;   // nothing to do: close the field
            }
            rows_[static_cast<std::size_t>(row)].name = s;
            return true;
        }
        Stroke strokeOf(const look::Palette& p, int row) const
        {
            if (row == -2)
                return { p.selected, {} };
            if (row == -1)
                return { p.label, { 3.0f, 3.5f } };
            static const Dash dashes[] = { { 11.0f, 5.0f }, { 2.0f, 4.0f, 10.0f, 4.0f }, { 6.0f, 3.0f }, { 1.5f, 3.5f } };
            const Col inks[] = { p.cold, p.ok, p.warning, p.hot, p.text };
            return { inks[static_cast<std::size_t>(row) % std::size(inks)], dashes[static_cast<std::size_t>(row) % std::size(dashes)] };
        }
        std::vector<int> allRows() const
        {
            std::vector<int> r{ -2, -1 };
            for (int i = 0; i < static_cast<int>(rows_.size()); ++i)
                r.push_back(i);
            return r;
        }
        // The row whose peaks the header reads: the selected one when it has a curve, else the predicted.
        int leadRow() const
        {
            return !trace(selected_).empty() ? selected_ : -2;
        }

        // ---- actions -----------------------------------------------------------------------------------------------
        void startRun()
        {
            if (ctx_.dyno.running())
                return;
            DynoRequest r = plan_;
            r.gear = plan_.chassis ? gear_ : 0;
            r.steps.clear();
            if (stepped_)   // every 500 rpm from `from` to `to`
                for (double w = plan_.from; w <= plan_.to + 1e-6; w += 500.0 * chart::kRpm)
                    r.steps.push_back(w);
            r.name = std::string(plan_.chassis ? "Chassis " : "Engine ") + std::to_string(rows_.size() + 1);
            runTime_ = 0.0f;
            if (ctx_.dyno.startRun(r))
            {
                runName_ = r.name;
                message_.clear();
            }
            else
                message_ = "This engine cannot run the dyno yet: no runner is connected.";
        }
        std::string csvOf(const DynoTrace& t) const
        {
            // A run says whether its exhaust walls were settled (docs/formats.md): a line before the header when known.
            std::string out = *t.wallsWord() != '\0' ? std::string("# exhaust ") + t.wallsWord() + "\n" : std::string();
            out += "speed_rad_s,rpm,torque_N_m,power_kW";
            for (const std::string& f : t.fields)
                out += "," + f;
            out += "\n";
            char b[64];
            for (std::size_t i = 0; i < t.speed.size(); ++i)
            {
                std::snprintf(b, sizeof b, "%.3f,%.1f,%.2f,%.3f", t.speed[i], t.speed[i] / chart::kRpm,
                              i < t.torque.size() ? t.torque[i] : std::nan(""), i < t.power.size() ? t.power[i] / 1000.0 : std::nan(""));
                out += b;
                for (std::size_t f = 0; f < t.fields.size(); ++f)
                {
                    const double v = f < t.values.size() && i < t.values[f].size() ? t.values[f][i] : std::nan("");
                    std::snprintf(b, sizeof b, ",%.6g", v);
                    out += b;
                }
                out += "\n";
            }
            return out;
        }
        void copyCsv()
        {
            const DynoTrace& t = trace(selected_);
            if (t.empty())
            {
                message_ = rowName(selected_) + " has no points to copy.";
                return;
            }
            if (canSave())
            {
                std::string error;
                const bool saved = ctx_.services->saveText(rowName(selected_) + ".csv", csvOf(t), error);
                message_ = saved ? rowName(selected_) + " saved as CSV, " + std::to_string(t.speed.size()) + " rows."
                                 : "Not saved: " + (error.empty() ? std::string("cancelled") : error);
                return;
            }
            const bool ok = host() != nullptr && host()->copyText(csvOf(t));
            message_ = ok ? rowName(selected_) + " copied as CSV, " + std::to_string(t.speed.size()) + " rows."
                          : "This window cannot reach the clipboard.";
        }
        // How far the run in progress has come (rad/s): the runner's newest point, else what the plan's rate says.
        double runSpeedNow() const
        {
            if (const DynoTrace* so = ctx_.dyno.runSoFar(); so != nullptr && !so->speed.empty())
                return so->speed.back();
            return stepped_ ? plan_.from : std::min(plan_.to, plan_.from + plan_.rate * runTime_);
        }
        bool canSave() const { return ctx_.services != nullptr && static_cast<bool>(ctx_.services->saveText); }
        std::string peakWords(const DynoTrace& t) const
        {
            const auto tq = t.peakTorque(), pw = t.peakPower();
            return ctx_.units.format(Quantity::torque, tq.value) + " at " + ctx_.units.format(Quantity::rotation, tq.speed, 0) +
                   ", " + ctx_.units.format(Quantity::power, pw.value) + " at " +
                   ctx_.units.format(Quantity::rotation, pw.speed, 0);
        }

        // ---- scales ------------------------------------------------------------------------------------------------
        void computeScales() const
        {
            const Units& u = ctx_.units;
            double x0 = 1e30, x1 = -1e30, tq = 0.0, pw = 0.0;
            for (int row : allRows())
            {
                const DynoTrace& t = trace(row);
                if (!isShown(row) || t.empty())
                    continue;
                for (std::size_t i = 0; i < t.speed.size(); ++i)
                {
                    x0 = std::min(x0, t.speed[i]);
                    x1 = std::max(x1, t.speed[i]);
                    if (i < t.torque.size() && std::isfinite(t.torque[i]))
                        tq = std::max(tq, t.torque[i]);
                    if (i < t.power.size() && std::isfinite(t.power[i]))
                        pw = std::max(pw, t.power[i]);
                }
            }
            if (x1 <= x0)
            {
                x0 = plan_.from;
                x1 = plan_.to;
            }
            if (plan_.chassis || ctx_.dyno.running())
            {
                x0 = std::min(x0, plan_.from);
                x1 = std::max(x1, plan_.to);
            }
            xToSi_ = 1.0 / u.toUser(Quantity::rotation, 1.0);
            const double ux0 = u.toUser(Quantity::rotation, x0), ux1 = u.toUser(Quantity::rotation, x1);
            xStep_ = chart::niceStep(ux1 - ux0, 8);
            xScale_ = { std::floor(ux0 / xStep_) * xStep_, std::ceil(ux1 / xStep_) * xStep_, plotArea_.x, plotArea_.right() };
            const double utq = u.toUser(Quantity::torque, tq > 0.0 ? tq : 500.0);
            const double upw = u.toUser(Quantity::power, pw > 0.0 ? pw : 300e3);
            // Five divisions shared by both axes, each axis's step the round one that holds its peak.
            divisions_ = 5;
            tqStep_ = chart::niceStep(utq * 1.06, divisions_);
            pwStep_ = chart::niceStep(upw * 1.06, divisions_);
            tqScale_ = { 0.0, tqStep_ * divisions_, plotArea_.bottom(), plotArea_.y };
            pwScale_ = { 0.0, pwStep_ * divisions_, plotArea_.bottom(), plotArea_.y };
        }

        // ---- drawing -----------------------------------------------------------------------------------------------
        void drawHeader(Canvas& c, const look::Palette& p) const
        {
            const int lead = leadRow();
            const DynoTrace& t = trace(lead);
            const auto tq = t.peakTorque(), pw = t.peakPower();
            const Units& u = ctx_.units;
            float x = header_.right();
            const auto block = [&](const char* label, double si, Quantity q, double at) {
                const std::string n = look::num(u.toUser(q, si), u.toUser(q, si) < 10.0 ? "%.2f" : "%.0f");
                const std::string unit = std::string(u.unit(q).name) + "  at " + u.format(Quantity::rotation, at, 0);
                const float w = c.textWidth(n.c_str(), look::kNumberL) + 6.0f + c.textWidth(unit.c_str(), look::kUnit);
                x -= std::max(w, c.textWidth(label, look::kCaps));
                look::readout(c, p, x, header_.y, label, n, unit, look::kNumberL);
                x -= 28.0f;
            };
            block("PEAK POWER", pw.value, Quantity::power, pw.speed);
            block("PEAK TORQUE", tq.value, Quantity::torque, tq.speed);
            const std::string whose = upper(rowName(lead));
            const Stroke s = strokeOf(p, lead);
            const float ww = c.textWidth(whose.c_str(), look::kCaps);
            c.text(whose.c_str(), x - ww, header_.y + 2.0f, look::kCaps, p.label);
            chart::sample(c, x - ww, x, header_.y + 22.0f, kTorqueW, s.ink, s.dash);
            // The title and a sentence in what is left at the left (the sentence only where it fits whole).
            const float room = x - ww - 20.0f - header_.x;
            const Canvas::ClipScope clip(c, { header_.x, header_.y - 4.0f, std::max(0.0f, room), header_.h + 8.0f });
            c.text("TORQUE AND POWER AT THE CRANK", header_.x, header_.y + 2.0f, look::kTitle, p.text);
            const std::string sub = ctx_.dyno.busy()
                                        ? "Predicting the build as it stands: " + look::num(ctx_.dyno.progress() * 100.0) + " %"
                                        : !ctx_.dyno.error().empty() ? "No prediction: " + ctx_.dyno.error()
                                        : "The predicted curve follows every edit; the runs you make are kept here.";
            if (c.textWidth(sub.c_str(), look::kBody) <= room || ctx_.dyno.busy())
                c.text(sub.c_str(), header_.x, header_.y + 24.0f, look::kBody, p.label);
        }

        // Runs of both kinds shown together: their difference is partly the walls', not the engine's. Said in words, over
        // the plot (drawn after it).
        void drawWalls(Canvas& c, const look::Palette& p) const
        {
            {
                bool cold = false, settled = false;
                for (int row : allRows())
                    if (isShown(row) && !trace(row).empty())
                    {
                        cold = cold || trace(row).walls == 0;
                        settled = settled || trace(row).walls > 0;
                    }
                if (cold && settled)
                {
                    // Inside the plot's top left (the curves start low), on a backing, as long as fits.
                    const char* longer = "MIXED: COLD-WALL AND SETTLED RUNS - NOT ALL THE DIFFERENCE IS THE ENGINE'S";
                    const char* w = c.textWidth(longer, look::kCaps) + 30.0f < plotArea_.w ? longer : "MIXED: COLD AND SETTLED WALLS";
                    const float tw = c.textWidth(w, look::kCaps), x = plotArea_.x + 8.0f, y = plotArea_.y + 6.0f;
                    c.rrect(x - 4.0f, y - 3.0f, tw + 24.0f, 16.0f, 3.0f, p.raised, 1.0f, p.warning);
                    c.disc(x + 4.0f, y + 5.0f, 3.5f, p.warning);
                    c.text(w, x + 14.0f, y, look::kCaps, p.warning);
                }
            }
        }

        void drawPlot(Canvas& c, const look::Palette& p) const
        {
            look::region(c, p, plotBox_);
            const Rect& a = plotArea_;
            const Units& u = ctx_.units;
            // The legend: what weight is which quantity (each row's colour and dash are in the list at the right).
            float lx = plotBox_.x + look::kPad;
            const float ly = plotBox_.y + 16.0f;
            chart::sample(c, lx, lx + 26.0f, ly, kTorqueW, p.text);
            lx += 32.0f;
            c.text("TORQUE (HEAVY)", lx, ly - 5.0f, look::kCaps, p.label);
            lx += c.textWidth("TORQUE (HEAVY)", look::kCaps) + 18.0f;
            chart::sample(c, lx, lx + 26.0f, ly, kPowerW, p.text);
            lx += 32.0f;
            c.text("POWER (LIGHT)", lx, ly - 5.0f, look::kCaps, p.label);
            // The grid, from the left axis; the right axis numbers the same lines.
            for (int i = 0; i <= divisions_; ++i)
            {
                const float y = tqScale_.px(tqStep_ * i);
                c.hairlineH(a.x, y, a.w, i == 0 ? p.edge : p.grid);
                c.text(chart::fixed(tqStep_ * i, chart::decimalsFor(tqStep_)).c_str(), a.x - 8.0f, y - 5.0f, look::kUnit,
                       p.label, funkgui::Align::right);
                c.text(chart::fixed(pwStep_ * i, chart::decimalsFor(pwStep_)).c_str(), a.right() + 8.0f, y - 5.0f, look::kUnit,
                       p.label);
            }
            for (double v = xScale_.d0; v <= xScale_.d1 + xStep_ * 0.01; v += xStep_)
            {
                const float x = xScale_.px(v);
                c.hairlineV(x, a.y, a.h, p.grid);
                c.text(chart::fixed(v, chart::decimalsFor(xStep_)).c_str(), x, a.bottom() + 6.0f, look::kUnit, p.label,
                       funkgui::Align::centre);
            }
            const std::string xl = "ENGINE SPEED  " + upper(u.unit(Quantity::rotation).name);
            c.text(xl.c_str(), a.centreX(), a.bottom() + 22.0f, look::kCaps, p.label, funkgui::Align::centre);
            const std::string tl = "TORQUE  " + std::string(u.unit(Quantity::torque).name);
            const std::string pl = "POWER  " + std::string(u.unit(Quantity::power).name);
            c.text(tl.c_str(), a.x, a.y - 16.0f, look::kCaps, p.text);
            c.text(pl.c_str(), a.right(), a.y - 16.0f, look::kCaps, p.text, funkgui::Align::right);

            // The run in progress: the span it sweeps, shaded up to where it should be by now.
            if (ctx_.dyno.running())
            {
                const double x0 = u.toUser(Quantity::rotation, plan_.from);
                const double now = runSpeedNow();
                const float px0 = xScale_.px(x0), px1 = xScale_.px(u.toUser(Quantity::rotation, now));
                c.rrect(px0, a.y, std::max(0.0f, px1 - px0), a.h, 0.0f, p.over(p.selected, 0.08f));
                c.segment(px1, a.y, px1, a.bottom(), 1.5f, p.selected);
            }

            // The traces: the selected row last, so it is on top.
            bool any = false;
            std::vector<int> order;
            for (int row : allRows())
                if (row != selected_)
                    order.push_back(row);
            order.push_back(selected_);
            for (int row : order)
            {
                const DynoTrace& t = trace(row);
                if (!isShown(row) || t.empty())
                    continue;
                any = true;
                const Stroke s = strokeOf(p, row);
                std::vector<float> xs, yt, yp;
                for (std::size_t i = 0; i < t.speed.size(); ++i)
                {
                    xs.push_back(xScale_.px(u.toUser(Quantity::rotation, t.speed[i])));
                    yt.push_back(i < t.torque.size() ? tqScale_.px(u.toUser(Quantity::torque, t.torque[i])) : std::nanf(""));
                    yp.push_back(i < t.power.size() ? pwScale_.px(u.toUser(Quantity::power, t.power[i])) : std::nanf(""));
                }
                const Canvas::ClipScope clip(c, { a.x - 4.0f, a.y - 4.0f, a.w + 8.0f, a.h + 8.0f });
                chart::stroke(c, xs, yt, kTorqueW, s.ink, s.dash);
                chart::stroke(c, xs, yp, kPowerW, s.ink, s.dash);
                // The peaks: a disc on each; the lead's with its number.
                const auto tq = t.peakTorque(), pw = t.peakPower();
                const bool lead = row == leadRow();
                const auto mark = [&](double sx, double sy, const Chart& ch, Quantity q) {
                    if (!std::isfinite(sx))
                        return;
                    const float x = xScale_.px(u.toUser(Quantity::rotation, sx));
                    const float y = (ch == Chart::torque ? tqScale_ : pwScale_).px(u.toUser(q, sy));
                    c.disc(x, y, lead ? 4.5f : 3.0f, lead ? s.ink : p.panel, 1.5f, s.ink);
                    if (lead)
                    {
                        const std::string n = u.format(q, sy);
                        c.text(n.c_str(), x, y - 20.0f, look::kNumberS, p.value, funkgui::Align::centre);
                    }
                };
                mark(tq.speed, tq.value, Chart::torque, Quantity::torque);
                mark(pw.speed, pw.value, Chart::power, Quantity::power);
            }
            // The run as it draws: its points so far in the stroke it will have, a disc at its newest.
            if (const DynoTrace* so = ctx_.dyno.runSoFar(); so != nullptr && !so->empty())
            {
                any = true;
                const Stroke st = strokeOf(p, static_cast<int>(rows_.size()));
                std::vector<float> xs, yt, yp;
                for (std::size_t i = 0; i < so->speed.size(); ++i)
                {
                    xs.push_back(xScale_.px(u.toUser(Quantity::rotation, so->speed[i])));
                    yt.push_back(i < so->torque.size() ? tqScale_.px(u.toUser(Quantity::torque, so->torque[i])) : std::nanf(""));
                    yp.push_back(i < so->power.size() ? pwScale_.px(u.toUser(Quantity::power, so->power[i])) : std::nanf(""));
                }
                const Canvas::ClipScope clip(c, { a.x - 4.0f, a.y - 4.0f, a.w + 8.0f, a.h + 8.0f });
                chart::stroke(c, xs, yt, kTorqueW, st.ink, st.dash);
                chart::stroke(c, xs, yp, kPowerW, st.ink, st.dash);
                if (!xs.empty())
                {
                    c.disc(xs.back(), yt.back(), 4.5f, st.ink, 1.5f, p.panel);
                    c.disc(xs.back(), yp.back(), 3.5f, st.ink, 1.5f, p.panel);
                }
            }
            if (!any)
            {
                const std::string why = ctx_.dyno.busy() ? "The predicted curve is being made."
                                      : !ctx_.dyno.error().empty() ? ctx_.dyno.error()
                                      : "No curve shown: the predicted curve comes a moment after the build loads, "
                                        "and runs appear here when they finish.";
                look::placeholder(c, p, a, "NO CURVE", why);
            }
            drawCursor(c, p);
        }

        enum class Chart
        {
            torque,
            power
        };

        void drawBand(Canvas& c, const look::Palette& p) const
        {
            const Units& u = ctx_.units;
            const Rect& a = bandArea_;
            if (extra_ < 0)
            {
                look::region(c, p, bandBox_, "THIRD TRACE");
                c.text("Choose a quantity recorded with the runs to draw it here against the same speed.", bandBox_.x + look::kPad,
                       bandBox_.y + 40.0f, look::kBody, p.dim);
                return;
            }
            const Extra& ex = kExtras[static_cast<std::size_t>(extra_)];
            const std::string unit = ex.quantity == Quantity::none ? std::string() : std::string(u.unit(ex.quantity).name);
            const std::string title = upper(ex.label) + (unit.empty() ? "" : "  " + unit);
            look::region(c, p, bandBox_, title);
            // The range over every shown row that recorded it.
            double lo = 1e30, hi = -1e30;
            bool any = false;
            for (int row : allRows())
            {
                const DynoTrace& t = trace(row);
                const int f = fieldIndex(t, ex.field);
                if (!isShown(row) || f < 0)
                    continue;
                for (double v : t.values[static_cast<std::size_t>(f)])
                    if (std::isfinite(v))
                    {
                        const double uv = toUser(ex, v);
                        lo = std::min(lo, uv);
                        hi = std::max(hi, uv);
                        any = true;
                    }
            }
            for (double v = xScale_.d0; v <= xScale_.d1 + xStep_ * 0.01; v += xStep_)
                c.hairlineV(xScale_.px(v), a.y, a.h, p.grid);
            if (!any)
            {
                c.hairlineH(a.x, a.bottom(), a.w, p.edge);
                const std::string why = "-   no run shown recorded " + std::string(ex.field);
                c.text(why.c_str(), a.centreX(), a.centreY() - 6.0f, look::kBody, p.dim, funkgui::Align::centre);
                return;
            }
            if (hi - lo < 1e-9)
            {
                lo -= 1.0;
                hi += 1.0;
            }
            const double step = chart::niceStep(hi - lo, 3);
            const double y0 = std::floor(lo / step) * step, y1 = std::ceil(hi / step) * step;
            const chart::Scale ys{ y0, y1, a.bottom(), a.y };
            for (double v = y0; v <= y1 + step * 0.01; v += step)
            {
                const float y = ys.px(v);
                c.hairlineH(a.x, y, a.w, std::fabs(v - y0) < step * 0.01 ? p.edge : p.grid);
                c.text(chart::fixed(v, chart::decimalsFor(step)).c_str(), a.x - 8.0f, y - 5.0f, look::kUnit, p.label,
                       funkgui::Align::right);
            }
            std::vector<int> order;
            for (int row : allRows())
                if (row != selected_)
                    order.push_back(row);
            order.push_back(selected_);
            for (int row : order)
            {
                const DynoTrace& t = trace(row);
                const int f = fieldIndex(t, ex.field);
                if (!isShown(row) || f < 0)
                    continue;
                const Stroke s = strokeOf(p, row);
                std::vector<float> xs, yv;
                const std::vector<double>& vals = t.values[static_cast<std::size_t>(f)];
                for (std::size_t i = 0; i < t.speed.size() && i < vals.size(); ++i)
                {
                    xs.push_back(xScale_.px(u.toUser(Quantity::rotation, t.speed[i])));
                    yv.push_back(std::isfinite(vals[i]) ? ys.px(toUser(ex, vals[i])) : std::nanf(""));
                }
                chart::stroke(c, xs, yv, 1.75f, s.ink, s.dash);
            }
        }

        static int fieldIndex(const DynoTrace& t, const char* field)
        {
            for (std::size_t i = 0; i < t.fields.size(); ++i)
                if (t.fields[i] == field && i < t.values.size())
                    return static_cast<int>(i);
            return -1;
        }
        double toUser(const Extra& ex, double v) const
        {
            return ex.quantity == Quantity::none ? v * ex.scale : ctx_.units.toUser(ex.quantity, v * ex.scale);
        }

        void drawCursor(Canvas& c, const look::Palette& p) const
        {
            if (cursorPx_ < 0)
                return;
            const Units& u = ctx_.units;
            const float x = static_cast<float>(cursorPx_);
            const double ux = xScale_.data(x);
            const double si = ux * xToSi_;
            c.segment(x, plotArea_.y, x, plotArea_.bottom(), 1.0f, p.text);
            c.segment(x, bandArea_.y, x, bandArea_.bottom(), 1.0f, p.text);
            struct Line
            {
                int row;
                double tq, pw, ex;
            };
            std::vector<Line> lines;
            const Extra* ex = extra_ >= 0 ? &kExtras[static_cast<std::size_t>(extra_)] : nullptr;
            for (int row : allRows())
            {
                const DynoTrace& t = trace(row);
                if (!isShown(row) || t.empty())
                    continue;
                Line l{ row, chart::at(t.speed, t.torque, si), chart::at(t.speed, t.power, si), std::nan("") };
                if (ex != nullptr)
                {
                    const int f = fieldIndex(t, ex->field);
                    if (f >= 0)
                        l.ex = chart::at(t.speed, t.values[static_cast<std::size_t>(f)], si);
                }
                if (std::isfinite(l.tq))
                {
                    const Stroke s = strokeOf(p, row);
                    c.disc(x, tqScale_.px(u.toUser(Quantity::torque, l.tq)), 3.5f, s.ink, 1.5f, p.panel);
                    c.disc(x, pwScale_.px(u.toUser(Quantity::power, l.pw)), 3.0f, s.ink, 1.5f, p.panel);
                }
                lines.push_back(l);
            }
            // The card: at the right of the cursor, or at its left near the right edge.
            // Its columns: the name, torque, power and the third quantity; on a narrow plot the third goes, then the
            // name narrows.
            if (ex != nullptr && plotArea_.w < 360.0f)
                ex = nullptr;
            const float nameW = std::clamp(plotArea_.w - 176.0f - (ex != nullptr ? 64.0f : 0.0f), 64.0f, 110.0f);
            const float w = nameW + 166.0f + (ex != nullptr ? 64.0f : 0.0f), rowH = 18.0f;
            const float h = 30.0f + rowH * static_cast<float>(lines.size());
            float cx = x + 14.0f;
            if (cx + w > plotArea_.right())
                cx = x - 14.0f - w;
            cx = std::clamp(cx, plotArea_.x + 2.0f, std::max(plotArea_.x + 2.0f, plotArea_.right() - w - 2.0f));
            const float cy = plotArea_.bottom() - 8.0f - h;   // low, where the curves leave room
            c.rrect(cx, cy, w, h, 5.0f, p.raised, 1.0f, p.edge);
            const std::string head = u.format(Quantity::rotation, si, 0);
            c.text(head.c_str(), cx + 10.0f, cy + 8.0f, look::kNumberS, p.value);
            c.text("TORQUE", cx + nameW + 58.0f, cy + 10.0f, look::kCaps, p.label, funkgui::Align::right);
            c.text("POWER", cx + nameW + 124.0f, cy + 10.0f, look::kCaps, p.label, funkgui::Align::right);
            if (ex != nullptr)
                c.text(upper(ex->label).c_str(), cx + w - 10.0f, cy + 10.0f, look::kCaps, p.label, funkgui::Align::right);
            float y = cy + 28.0f;
            for (const Line& l : lines)
            {
                const Stroke s = strokeOf(p, l.row);
                chart::sample(c, cx + 10.0f, cx + 26.0f, y + 6.0f, 2.0f, s.ink, s.dash);
                std::string nm = rowName(l.row);
                while (nm.size() > 3 && c.textWidth(nm.c_str(), look::kUnit) > nameW - 34.0f)
                {
                    nm = nm.substr(0, nm.size() - 2);
                    while (!nm.empty() && nm.back() == ' ')
                        nm.pop_back();
                    nm += ".";
                }
                c.text(nm.c_str(), cx + 32.0f, y, look::kUnit, p.text);
                c.text(look::num(u.toUser(Quantity::torque, l.tq)).c_str(), cx + nameW + 58.0f, y, look::kUnit, p.value,
                       funkgui::Align::right);
                c.text(look::num(u.toUser(Quantity::power, l.pw)).c_str(), cx + nameW + 124.0f, y, look::kUnit, p.value,
                       funkgui::Align::right);
                if (ex != nullptr)
                    c.text(look::num(std::isfinite(l.ex) ? toUser(*ex, l.ex) : l.ex, ex->quantity == Quantity::none ? "%.2f" : "%.1f").c_str(),
                           cx + w - 10.0f, y, look::kUnit, p.value, funkgui::Align::right);
                y += rowH;
            }
        }

        // The rows listed and where: predicted and before always, then as many of the newest runs as fit.
        std::vector<RowHit> rowLayout() const
        {
            std::vector<RowHit> out;
            const Rect& a = rowsArea_;
            const float rowH = 30.0f;
            const int fit = std::max(2, static_cast<int>(a.h / rowH));
            // Predicted always; before the last edit when it has a curve or there is room; then the newest runs.
            const int runs = static_cast<int>(rows_.size());
            const bool before = !ctx_.dyno.before().empty() || fit >= 2 + runs;
            std::vector<int> list{ -2 };
            if (before)
                list.push_back(-1);
            for (int i = std::max(0, runs - (fit - static_cast<int>(list.size()))); i < runs; ++i)
                list.push_back(i);
            float y = a.y;
            for (int row : list)
            {
                if (y + rowH > a.bottom() + 4.0f)
                    break;
                const Rect r{ a.x - 4.0f, y, a.w + 8.0f, rowH - 4.0f };
                out.push_back({ r, { r.x, r.y, 34.0f, r.h }, row });
                y += rowH;
            }
            return out;
        }

        void drawRuns(Canvas& c, const look::Palette& p) const
        {
            look::region(c, p, runsBox_, "CURVES AND RUNS");
            const std::vector<RowHit> hits = rowLayout();
            int listedRuns = 0;
            for (const RowHit& h : hits)
            {
                const int row = h.row;
                const Rect& r = h.rect;
                listedRuns += row >= 0 ? 1 : 0;
                const bool sel = row == selected_, shown = isShown(row);
                const DynoTrace& t = trace(row);
                if (sel)
                    c.rrect(r.x, r.y, r.w, r.h, 5.0f, p.over(p.selected, 0.12f), 1.5f, p.selected);
                const Rect box{ r.x + 8.0f, r.y + 6.0f, 14.0f, 14.0f };
                c.rrect(box.x, box.y, box.w, box.h, 3.0f, shown ? p.raised : p.panel, 1.25f, shown ? p.text : p.dim);
                if (shown)
                {
                    c.segment(box.x + 3.0f, box.y + 7.5f, box.x + 6.0f, box.y + 10.5f, 1.75f, p.value);
                    c.segment(box.x + 6.0f, box.y + 10.5f, box.x + 11.0f, box.y + 3.5f, 1.75f, p.value);
                }
                const Stroke s = strokeOf(p, row);
                chart::sample(c, r.x + 30.0f, r.x + 58.0f, r.y + 8.0f, kTorqueW, shown ? s.ink : p.dim, s.dash);
                chart::sample(c, r.x + 30.0f, r.x + 58.0f, r.y + 18.0f, kPowerW, shown ? s.ink : p.dim, s.dash);
                // The run's exhaust walls, in words at the right of its name (cold in amber too); the name cut to what
                // is left.
                float nameRoom = r.right() - 6.0f - (r.x + 66.0f);
                if (*t.wallsWord() != '\0')
                {
                    const float ww = c.textWidth(t.wallsWord(), look::kUnit);
                    c.text(t.wallsWord(), r.right() - 6.0f, r.y + 3.0f, look::kUnit,
                           !shown ? p.dim : t.walls == 0 ? p.warning : p.label, funkgui::Align::right);
                    nameRoom -= ww + 8.0f;
                }
                {
                    const Canvas::ClipScope clip(c, { r.x + 66.0f, r.y, std::max(0.0f, nameRoom), r.h });
                    c.text(rowName(row).c_str(), r.x + 66.0f, r.y + 1.0f, look::kBody, shown ? p.value : p.dim);
                }
                std::string sub;
                if (t.empty())
                    sub = row == -1 ? "none yet: edit the build" : row == -2 ? "being made" : "no points";
                else
                {
                    const auto tq = t.peakTorque(), pw = t.peakPower();
                    sub = ctx_.units.format(Quantity::torque, tq.value) + "   " + ctx_.units.format(Quantity::power, pw.value);
                }
                c.text(sub.c_str(), r.x + 66.0f, r.y + 14.0f, look::kUnit, shown ? p.label : p.dim);
            }
            const int older = static_cast<int>(rows_.size()) - listedRuns;
            if (older > 0)
                c.text((std::to_string(older) + " older runs not listed").c_str(), rowsArea_.right(), rowsArea_.bottom() - 10.0f,
                       look::kUnit, p.dim, funkgui::Align::right);
        }

        void drawPlan(Canvas& c, const look::Palette& p) const
        {
            look::region(c, p, planBox_, ctx_.dyno.running() ? "RUN IN PROGRESS" : "RUN PLAN", ctx_.dyno.running());
            const Units& u = ctx_.units;
            const float x = planBox_.x + look::kPad, w = planBox_.w - 2.0f * look::kPad;
            std::string words;
            if (ctx_.dyno.running())
            {
                const double now = runSpeedNow();
                const double done = std::clamp((now - plan_.from) / std::max(1e-6, plan_.to - plan_.from), 0.0, 1.0);
                words = runName_ + (stepped_ ? ": holding " : ": sweeping ") + u.format(Quantity::rotation, now, 0) + ", " +
                        look::num(done * 100.0) + " % of the way to " + u.format(Quantity::rotation, plan_.to, 0);
                look::level(c, p, { x, wordsY_ + 32.0f, w, 4.0f }, done, p.selected);
            }
            else if (!message_.empty())
                words = message_;
            else
                words = std::string(plan_.chassis ? "A chassis run: the car's own speed on rollers, in gear " + std::to_string(gear_)
                                                  : "An engine run: the crank held and swept") +
                        " at " + look::num(plan_.pedal * 100.0) + " % pedal.";
            // Two lines at most, cut at a space.
            std::string a = words, b;
            if (c.textWidth(a.c_str(), look::kBody) > w)
            {
                std::size_t cut = a.size();
                while (cut > 0 && (a[cut - 1] != ' ' || c.textWidth(a.substr(0, cut - 1).c_str(), look::kBody) > w))
                    --cut;
                if (cut > 0)
                {
                    b = a.substr(cut);
                    a = a.substr(0, cut - 1);
                }
            }
            c.text(a.c_str(), x, wordsY_, look::kBody, p.text);
            if (!b.empty())
                c.text(b.c_str(), x, wordsY_ + 15.0f, look::kBody, p.text);
        }

        // ---- state -------------------------------------------------------------------------------------------------
        DynoRequest plan_{ false, 1500.0 * chart::kRpm, 7000.0 * chart::kRpm, 600.0 * chart::kRpm, 1.0, {}, {}, 0.6, 0, true };
        int gear_ = 4;
        bool stepped_ = false;
        std::vector<RowView> rows_;
        bool showPredicted_ = true, showBefore_ = true;
        int selected_ = -2, extra_ = 0, cursorPx_ = -1;
        float runTime_ = 0.0f;
        std::string runName_, message_;

        std::unique_ptr<PlanPort> fromPort_, toPort_, ratePort_, pedalPort_, gearPort_, dwellPort_;
        Button *sweepBtn_ = nullptr, *stepsBtn_ = nullptr;
        Stepper* dwell_ = nullptr;
        Button *engineBtn_ = nullptr, *chassisBtn_ = nullptr, *start_ = nullptr, *stop_ = nullptr, *copy_ = nullptr;
        Stepper *from_ = nullptr, *to_ = nullptr, *rate_ = nullptr, *pedal_ = nullptr, *gearStep_ = nullptr;
        TextField* name_ = nullptr;
        std::array<Button*, kExtraCount + 1> extraBtn_{};

        Rect header_{}, plotBox_{}, bandBox_{}, plotArea_{}, bandArea_{}, runsBox_{}, planBox_{}, rowsArea_{};
        float wordsY_ = 0.0f;
        // What drawing works out (not state: remade every frame from the above).
        mutable chart::Scale xScale_, tqScale_, pwScale_;
        mutable double xStep_ = 1.0, tqStep_ = 1.0, pwStep_ = 1.0, xToSi_ = 1.0;
        mutable int divisions_ = 5;
    };

    std::unique_ptr<Workspace> makeDynoWorkspace(WorkspaceContext& c) { return std::make_unique<DynoWorkspace>(c); }
}

namespace tee::ui
{
    funkgui::Rect dynoControl(Workspace& w, const char* which)
    {
        auto* d = dynamic_cast<DynoWorkspace*>(&w);
        return d != nullptr ? d->control(which) : funkgui::Rect{};
    }
}
