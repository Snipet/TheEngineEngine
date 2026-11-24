// shell/player/RefAdapter.cpp: RefAdapter.h.
#include "player/RefAdapter.h"

#include "tee/RefPlayer.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace tee::player
{
    namespace
    {
        // ---- what panel.gui says of each dial (sim/ref/panel.gui), which the program's declarations do not ----------
        struct Look
        {
            std::string_view id, label, unit, group;
            std::vector<std::string> choices;       // an int that is a choice, from its range's minimum up
            bool toggle = false;
            bool logTaper = false;
            int decimals = 2;
            double scale = 1.0;                     // 100 with unit "%": a 0 to 1 dial shown as a share
        };

        const std::vector<Look>& looks()
        {
            static const std::vector<Look> table{
                { "engine", "LAYOUT", "", "BLOCK",
                  { "SINGLE", "TWIN 360", "TWIN 270", "VTWIN 45", "INLINE 3", "INLINE 4", "FLAT 4", "XPLANE I4", "INLINE 5",
                    "V6", "INLINE 6", "FLAT 6", "V8 CROSS", "V8 FLAT", "V10", "V12", "ROTARY" },
                  false, false, 0, 1.0 },
                { "fuel", "FUEL", "", "BLOCK", { "GASOLINE", "DIESEL" }, false, false, 0, 1.0 },
                { "size", "SIZE", "L", "BLOCK", {}, false, true, 2, 1.0 },
                { "borestroke", "BORE/STROKE", "", "BLOCK", {}, false, true, 2, 1.0 },
                { "compression", "COMPRESSION", ":1", "BLOCK", {}, false, false, 1, 1.0 },
                { "timing", "TIMING", "deg", "TUNE", {}, false, false, 1, 1.0 },
                { "cam", "CAM", "%", "TUNE", {}, false, false, 0, 100.0 },
                { "inertia", "FLYWHEEL", "%", "TUNE", {}, false, false, 0, 100.0 },
                { "idle", "IDLE +", "rpm", "REVS", {}, false, false, 0, 1.0 },
                { "redline", "REDLINE +", "rpm", "REVS", {}, false, false, 0, 1.0 },
                { "load", "LOAD", "%", "REVS", {}, false, false, 0, 100.0 },
                { "aspiration", "ASPIRATION", "", "INDUCTION", { "NA", "TURBO", "TWIN TURBO", "QUAD TURBO", "S/CHARGER" },
                  false, false, 0, 1.0 },
                { "turbo", "BOOST", "%", "INDUCTION", {}, false, false, 0, 100.0 },
                { "turbosize", "TURBO SIZE", "x", "INDUCTION", {}, false, true, 2, 1.0 },
                { "bov", "BLOW-OFF", "%", "INDUCTION", {}, false, false, 0, 100.0 },
                { "headers", "HEADERS", "%", "EXHAUST", {}, false, false, 0, 100.0 },
                { "pipe", "PIPE", "%", "EXHAUST", {}, false, false, 0, 100.0 },
                { "xpipe", "X-PIPE", "%", "EXHAUST", {}, false, false, 0, 100.0 },
                { "muffler", "MUFFLER", "%", "EXHAUST", {}, false, false, 0, 100.0 },
                { "pops", "POPS", "%", "CHARACTER", {}, false, false, 0, 100.0 },
                { "rough", "ROUGH", "%", "CHARACTER", {}, false, false, 0, 100.0 },
                { "view", "VIEW", "", "LISTEN", { "EXHAUST", "REAR", "SIDE", "ENGINE", "CABIN" }, false, false, 0, 1.0 },
                { "distance", "DISTANCE", "%", "LISTEN", {}, false, false, 0, 100.0 },
                { "room", "ROOM", "%", "LISTEN", {}, false, false, 0, 100.0 },
                { "intake", "INTAKE", "%", "MIX", {}, false, false, 0, 100.0 },
                { "mech", "MECHANICAL", "%", "MIX", {}, false, false, 0, 100.0 },
                { "level", "LEVEL", "dB", "MIX", {}, false, false, 1, 1.0 },
                { "starter", "STARTER", "", "START", { "OFF", "ON" }, true, false, 0, 1.0 },
                { "hold", "IDLE FOR", "s", "START", {}, false, true, 0, 1.0 },
                { "ambient", "AMBIENT", "F", "START", {}, false, false, 0, 1.0 },
                { "drive", "DRIVE", "", "DRIVE", { "KEYS", "CAR", "TRUCK", "PITCH" }, false, false, 0, 1.0 },
                { "mass", "WEIGHT", "kg", "DRIVE", {}, false, true, 0, 1.0 },
            };
            return table;
        }

        ParamInfo infoOf(const RefPlayer::Dial& dial)
        {
            ParamInfo p;
            p.id = dial.name;
            p.label = dial.name;
            p.min = dial.minimum;
            p.max = dial.maximum;
            p.def = dial.defaultValue;
            const bool whole = dial.type != "float";
            p.kind = whole ? ParamInfo::Kind::integer : ParamInfo::Kind::real;
            p.decimals = whole ? 0 : 2;
            for (const Look& look : looks())
            {
                if (look.id != dial.name)
                    continue;
                p.label = std::string(look.label);
                p.unit = std::string(look.unit);
                p.group = std::string(look.group);
                p.decimals = look.decimals;
                p.scale = look.scale;
                p.logTaper = look.logTaper && p.min > 0.0;
                // the names only when they are the range's: a reference whose range grew shows numbers, never a
                // wrong name
                if (whole && !look.choices.empty()
                    && static_cast<double>(look.choices.size()) == std::round(p.max - p.min) + 1.0)
                {
                    p.choices = look.choices;
                    p.kind = look.toggle ? ParamInfo::Kind::toggle : ParamInfo::Kind::choice;
                }
                break;
            }
            return p;
        }

        // ---- the telemetry, in Sources.h's names ---------------------------------------------------------------------
        enum Tel
        {
            tState, tSpeed, tRedline, tThrottle, tManifold, tCoolant, tOil, tFlags, tCycle,
            tCarSpeed, tGear, tDriven, tCarBrake, tCarClutch, tBoost, tCount
        };

        constexpr std::array<std::string_view, tCount> kTelNames{
            telemetry::kState,    telemetry::kSpeed,   telemetry::kRedline, telemetry::kThrottle, telemetry::kManifold,
            telemetry::kCoolant,  telemetry::kOilPressure, telemetry::kFlags, telemetry::kCycle,  telemetry::kCarSpeed,
            telemetry::kGear,     telemetry::kDriven,  telemetry::kCarBrake, telemetry::kCarClutch, telemetry::kBoost,
        };

        constexpr double kRpmToRad = 6.283185307179586 / 60.0;
    }

    RefAdapter::RefAdapter() = default;
    RefAdapter::~RefAdapter() = default;

    std::unique_ptr<RefAdapter> RefAdapter::create(const Options& options, std::string& errors)
    {
        RefPlayer::Options ro;
        ro.directory = options.directory;
        ro.sampleRate = options.sampleRate;
        ro.optimisation = options.optimisation;
        ro.checks = options.checks;
        std::unique_ptr<RefPlayer> ref = RefPlayer::create(ro, errors);
        if (!ref)
            return nullptr;

        std::unique_ptr<RefAdapter> a(new RefAdapter());
        a->rate_ = ref->sampleRate();
        a->compileSeconds_ = ref->program().compileSeconds() + ref->program().linkSeconds();
        a->warnings_ = ref->program().warnings();
        const auto& dials = ref->dials();
        a->infos_.reserve(dials.size());
        a->shown_ = std::make_unique<std::atomic<double>[]>(dials.size());
        for (std::size_t i = 0; i < dials.size(); ++i)
        {
            a->infos_.push_back(infoOf(dials[i]));
            a->shown_[i].store(ref->dial(i), std::memory_order_relaxed);
        }
        a->driveDial_ = ref->dialIndex("drive");
        a->sRpm_ = ref->showIndex("showRpm");
        a->sRed_ = ref->showIndex("showRed");
        a->sGear_ = ref->showIndex("showGear");
        a->sKmh_ = ref->showIndex("showKmh");
        a->sThr_ = ref->showIndex("showThr");
        a->sMap_ = ref->showIndex("showMap");
        a->sLamps_ = ref->showIndex("showLamps");
        a->sPedals_ = ref->showIndex("showPedals");
        a->sCycle_ = ref->showIndex("showCycle");
        a->sBoost_ = ref->showIndex("showBoost");
        a->showNames_ = ref->showNames();
        const std::size_t values = static_cast<std::size_t>(tCount) + a->showNames_.size();
        a->tel_ = std::make_unique<std::atomic<double>[]>(values);
        for (std::size_t i = 0; i < values; ++i)
            a->tel_[i].store(0.0, std::memory_order_relaxed);
        a->ref_ = std::move(ref);
        a->prepare(a->rate_, 0);
        a->publish();
        return a;
    }

    std::string RefAdapter::why() const
    {
        return stopped() ? ref_->stopReason() : std::string();
    }

    const ParamInfo& RefAdapter::info(int i) const
    {
        static const ParamInfo none;
        return i >= 0 && i < count() ? infos_[static_cast<std::size_t>(i)] : none;
    }

    double RefAdapter::get(int i) const
    {
        return i >= 0 && i < count() ? shown_[static_cast<std::size_t>(i)].load(std::memory_order_relaxed) : 0.0;
    }

    void RefAdapter::set(int i, double value)
    {
        if (i < 0 || i >= count() || !(value == value))
            return;
        const auto u = static_cast<std::size_t>(i);
        const ParamInfo& p = infos_[u];
        double v = std::clamp(value, p.min, p.max);         // FMinus neither clamps nor checks: the host does
        if (p.kind != ParamInfo::Kind::real)
            v = std::round(v);
        shown_[u].store(v, std::memory_order_relaxed);
        ref_->setDial(u, v);                                // an atomic store the next call reads
    }

    int RefAdapter::findParam(std::string_view id) const
    {
        for (std::size_t i = 0; i < infos_.size(); ++i)
            if (infos_[i].id == id)
                return static_cast<int>(i);
        return -1;
    }

    int RefAdapter::findValue(std::string_view name) const
    {
        for (std::size_t i = 0; i < kTelNames.size(); ++i)
            if (kTelNames[i] == name)
                return static_cast<int>(i);
        for (std::size_t i = 0; i < showNames_.size(); ++i)
            if (showNames_[i] == name)
                return tCount + static_cast<int>(i);
        return -1;
    }

    double RefAdapter::value(int handle) const
    {
        const auto n = static_cast<int>(showNames_.size());
        return handle >= 0 && handle < tCount + n ? tel_[static_cast<std::size_t>(handle)].load(std::memory_order_relaxed)
                                                 : 0.0;
    }

    void RefAdapter::prepare(double rate, int)
    {
        if (rate > 1000.0 && rate != ref_->sampleRate())
        {
            // another rate: the reference starts again at it (its init sizes everything by the rate), with the dials
            // as they were set. The engine is off afterwards, as after a start of the application.
            ref_->reset(rate);
            for (std::size_t i = 0; i < infos_.size(); ++i)
                ref_->setDial(i, shown_[i].load(std::memory_order_relaxed));
            noteHeld_ = false;
            up_ = {};
            down_ = {};
            const ControlsFrame f = controls_.get();
            up_.seen = f.shiftUp;
            down_.seen = f.shiftDown;
        }
        rate_ = ref_->sampleRate();
        pulseSamples_ = std::max(64, static_cast<int>(0.01 * rate_));
    }

    // One paddle. The reference looks at the buttons every 16 of its own samples and takes a bit that was clear and is
    // set as a press, so each counted press holds the bit for a hundredth of a second and then leaves it clear for as
    // long. A press that arrives while the reference is mid-shift is the reference's to drop, as a real paddle's is.
    bool RefAdapter::pulse(Pulse& p, int count, int n) noexcept
    {
        if (p.hold > 0)
        {
            p.hold -= n;
            if (p.hold <= 0)
                p.gap = pulseSamples_;
            return true;
        }
        if (p.gap > 0)
        {
            p.gap -= n;
            return false;
        }
        if (p.seen == count)
            return false;
        // one press at a time; a count that jumped far (a new engine, a wrapped counter) is taken as one press
        const int behind = count - p.seen;
        p.seen = behind > 0 && behind < 8 ? p.seen + 1 : count;
        p.hold = pulseSamples_ - n;
        if (p.hold <= 0)
            p.gap = pulseSamples_;
        return true;
    }

    void RefAdapter::process(float* l, float* r, int n) noexcept
    {
        if (n <= 0)
            return;
        const ControlsFrame f = controls_.get();
        const int drive = driveDial_ >= 0
            ? static_cast<int>(shown_[static_cast<std::size_t>(driveDial_)].load(std::memory_order_relaxed)) : 0;
        const bool driven = drive == 1 || drive == 2;

        RefPlayer::Input& in = ref_->input();
        in.throttle = f.throttle;
        in.brake = f.brake;
        in.clutch = f.clutch;
        in.steering = f.steer;
        in.gear = f.lever;
        in.modwheel = driven ? 0.0f : f.throttle;           // played from the keys, the throttle is "by hand"
        int buttons = f.buttons & ~(RefPlayer::Buttons::paddleUp | RefPlayer::Buttons::paddleDown);
        if (pulse(up_, f.shiftUp, n))
            buttons |= RefPlayer::Buttons::paddleUp;
        if (pulse(down_, f.shiftDown, n))
            buttons |= RefPlayer::Buttons::paddleDown;
        in.buttons = buttons;

        // the start key is the reference's note 36 (RefAdapter.h)
        const bool want = driven ? f.ignition == 2 : f.ignition >= 1;
        if (want && !noteHeld_)
            ref_->start(36.0f, 1.0f);
        else if (!want && noteHeld_)
            ref_->stop();
        noteHeld_ = want;

        ref_->process(l, r, n);
        if (ref_->stopped() && !stopped_.load(std::memory_order_relaxed))
        {
            // The program was stopped in this call (its loop budget, a bounds check: a build with checks): whatever
            // the call left in the buffers is not sound. In its place the output runs down from where it was to
            // nothing over this block, and it is silence from then on; the UI thread shows why (stopped(), why()).
            for (int i = 0; i < n; ++i)
            {
                const float g = 1.0f - static_cast<float>(i + 1) / static_cast<float>(n);
                l[i] = lastL_ * g;
                r[i] = lastR_ * g;
            }
            stopped_.store(true, std::memory_order_release);
        }
        lastL_ = l[n - 1];
        lastR_ = r[n - 1];
        peak_.store(blockPeak(l, r, n), std::memory_order_relaxed);
        tail_.write(l, r, n);
        publish();
    }

    // Shared's show* fields into Sources.h's names and SI (sim/ref/engine.fm says how each is packed).
    void RefAdapter::publish() noexcept
    {
        const auto show = [this](int index) {
            return index >= 0 ? static_cast<double>(ref_->show(static_cast<std::size_t>(index))) : 0.0;
        };
        const auto put = [this](Tel t, double v) { tel_[static_cast<std::size_t>(t)].store(v, std::memory_order_relaxed); };

        // showLamps: the state (0 to 3) + 4 limiter + 8 fuel cut + 16 driven + 32 (coolant F + 60) + 16384 oil psi
        // + 2097152 a truck
        const long lamps = std::lround(show(sLamps_));
        const bool driven = ((lamps >> 4) & 1) != 0;
        put(tState, static_cast<double>(lamps & 3));
        put(tFlags, static_cast<double>((lamps >> 2) & 3));
        put(tDriven, driven ? 1.0 : 0.0);
        put(tCoolant, (static_cast<double>((lamps >> 5) & 511) - 92.0) / 1.8 + 273.15);
        put(tOil, static_cast<double>((lamps >> 14) & 127) * 6894.76);
        put(tSpeed, show(sRpm_) * kRpmToRad);
        put(tRedline, show(sRed_) * kRpmToRad);
        put(tThrottle, show(sThr_));
        put(tManifold, show(sMap_) * 1e5 + 101325.0);
        put(tCycle, show(sCycle_));
        put(tBoost, show(sBoost_) * 1e5);
        put(tGear, driven ? show(sGear_) : 0.0);
        put(tCarSpeed, driven ? show(sKmh_) / 3.6 : 0.0);
        // showPedals: the brake (0 to 100) + 1000 times the clutch (0 to 100)
        const long pedals = std::lround(show(sPedals_));
        put(tCarBrake, static_cast<double>(pedals % 1000) / 100.0);
        put(tCarClutch, static_cast<double>(pedals / 1000) / 100.0);
        // and each of them as it is, for the reference's own panel
        for (std::size_t i = 0; i < showNames_.size(); ++i)
            tel_[static_cast<std::size_t>(tCount) + i].store(static_cast<double>(ref_->show(i)), std::memory_order_relaxed);
    }
}
