// shell/player/SimAdapter.cpp: SimAdapter.h.
#include "player/SimAdapter.h"

#include "tee/Build.h"
#include "tee/Fields.h"
#include "tee/SimHost.h"
#include "tee/Telemetry.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace tee::player
{
    namespace
    {
        // A quantity as the plain Panel shows it: a usual unit and what the SI value is multiplied by. (The interface
        // package has the user's own choice of units; this is for a Panel that knows only ParamInfo.)
        struct Shown
        {
            std::string_view quantity, unit;
            double scale;
        };

        constexpr std::array<Shown, 15> kShown{ {
            { "volume", "L", 1000.0 },       { "rotation", "rpm", 9.549296585513721 }, { "pressure", "bar", 1e-5 },
            { "fraction", "%", 100.0 },      { "length", "mm", 1000.0 },               { "area", "cm2", 1e4 },
            { "mass", "kg", 1.0 },           { "inertia", "kg m2", 1.0 },              { "speed", "km/h", 3.6 },
            { "torque", "N m", 1.0 },        { "power", "kW", 1e-3 },                  { "angle", "deg", 1.0 },
            { "time", "s", 1.0 },            { "frequency", "Hz", 1.0 },               { "temperature", "K", 1.0 },
        } };

        bool oneNumber(const FieldInfo& f) noexcept
        {
            return (f.kind == FieldKind::Number || f.kind == FieldKind::Integer || f.kind == FieldKind::Switch
                    || f.kind == FieldKind::Choice)
                   && f.count == 1 && f.extents.empty();
        }

        ParamInfo infoOf(const FieldInfo& f)
        {
            ParamInfo p;
            p.id = f.path.substr(5);                        // without "spec."
            p.label = f.label.empty() ? p.id : f.label;
            p.group = f.group;
            if (p.group.empty())
            {
                // its section's name: "engine" of "engine.displacement"
                const std::size_t dot = p.id.find('.');
                p.group = dot == std::string::npos ? std::string() : p.id.substr(0, dot);
            }
            p.min = f.minimum.value_or(0.0);
            p.max = f.maximum.value_or(f.kind == FieldKind::Switch ? 1.0 : std::max(1.0, p.min + 1.0));
            p.def = f.clamp(f.fallback());
            p.unit = f.unit;
            switch (f.kind)
            {
                case FieldKind::Switch: p.kind = ParamInfo::Kind::toggle; break;
                case FieldKind::Choice: p.kind = ParamInfo::Kind::choice; break;
                case FieldKind::Integer: p.kind = ParamInfo::Kind::integer; break;
                default: p.kind = ParamInfo::Kind::real; break;
            }
            p.choices = f.choices;
            p.logTaper = f.taper == "log" && p.min > 0.0;
            if (p.kind == ParamInfo::Kind::real)
                for (const Shown& s : kShown)
                    if (s.quantity == f.quantity)
                    {
                        p.unit = std::string(s.unit);
                        p.scale = s.scale;
                    }
            if (f.decimals >= 0)
                p.decimals = f.decimals;
            else if (p.kind != ParamInfo::Kind::real)
                p.decimals = 0;
            else
            {
                const double span = std::fabs(p.max - p.min) * p.scale;
                p.decimals = span >= 200.0 ? 0 : (span >= 20.0 ? 1 : 2);
            }
            return p;
        }
    }

    SimAdapter::SimAdapter() = default;
    SimAdapter::~SimAdapter() = default;

    std::unique_ptr<SimAdapter> SimAdapter::create(const Options& options, std::string& errors)
    {
        std::unique_ptr<SimAdapter> a(new SimAdapter());
        a->host_ = std::make_unique<SimHost>();
        LoadOptions lo;
        lo.sim.optimisation = options.optimisation;
        lo.sim.checks = options.checks;
        lo.sampleRate = options.sampleRate;
        if (!a->host_->load(options.directory, lo, errors))
        {
            if (errors.empty())
                errors = "the simulator did not load, and the host gave no reason";
            return nullptr;
        }
        const std::shared_ptr<Sim> sim = a->host_->sim();
        a->compileSeconds_ = sim->program().compileSeconds() + sim->program().linkSeconds();
        a->warnings_ = sim->program().warnings();

        const Fields& fields = a->host_->fields();
        a->build_ = std::make_unique<tee::Build>(fields);
        a->build_->attach(a->host_.get());
        for (const FieldInfo& f : fields.all())
        {
            if (f.block == "spec" && oneNumber(f) && !f.hidden && f.path.size() > 5)
            {
                a->infos_.push_back(infoOf(f));
                a->paths_.push_back(f.path);
            }
            else if (f.block == "tel" && f.count == 1 && f.extents.empty() && f.scalar != Scalar::None && f.path.size() > 4)
            {
                a->tel_.push_back(&f);
                a->telNames_.push_back(f.path.substr(4));   // without "tel."
            }
        }
        a->frame_ = std::make_unique<TelemetryFrame>();
        return a;
    }

    void SimAdapter::frame()
    {
        build_->flush();
        stale_ = true;              // the next value asked is of the newest block, however soon it is asked
    }

    std::string SimAdapter::why() const
    {
        return host_->stopReason();
    }

    const ParamInfo& SimAdapter::info(int i) const
    {
        static const ParamInfo none;
        return i >= 0 && i < count() ? infos_[static_cast<std::size_t>(i)] : none;
    }

    double SimAdapter::get(int i) const
    {
        return i >= 0 && i < count() ? build_->get(paths_[static_cast<std::size_t>(i)]) : 0.0;
    }

    void SimAdapter::set(int i, double value)
    {
        if (i < 0 || i >= count() || !(value == value))
            return;
        const auto u = static_cast<std::size_t>(i);
        const ParamInfo& p = infos_[u];
        double v = std::clamp(value, p.min, p.max);
        if (p.kind != ParamInfo::Kind::real)
            v = std::round(v);
        build_->set(paths_[u], v);
        build_->flush();            // to the host's queue at once; what finds it full goes with a later frame()
    }

    int SimAdapter::findParam(std::string_view id) const
    {
        for (std::size_t i = 0; i < infos_.size(); ++i)
            if (infos_[i].id == id)
                return static_cast<int>(i);
        return -1;
    }

    int SimAdapter::findValue(std::string_view name) const
    {
        for (std::size_t i = 0; i < telNames_.size(); ++i)
            if (telNames_[i] == name)
                return static_cast<int>(i);
        return -1;
    }

    // The newest block's telemetry, copied out of the host at most every few milliseconds: a frame of the interface
    // asks for dozens of values, and they should be of one block.
    void SimAdapter::refresh() const
    {
        const auto now = std::chrono::steady_clock::now();
        if (have_ && !stale_ && now - fetched_ < std::chrono::milliseconds(4))
            return;
        fetched_ = now;
        stale_ = false;
        have_ = host_->telemetry(*frame_) || have_;
    }

    double SimAdapter::value(int handle) const
    {
        if (handle < 0 || handle >= static_cast<int>(tel_.size()))
            return 0.0;
        refresh();
        return have_ ? frame_->get(*tel_[static_cast<std::size_t>(handle)]) : 0.0;
    }

    float SimAdapter::outputPeak() const
    {
        refresh();
        return have_ ? std::min(frame_->host.outputPeak, 1.0f) : 0.0f;
    }

    void SimAdapter::setControls(const ControlsFrame& f)
    {
        SimControls c = host_->controls();      // what the frame does not carry (the dyno's, the scope) stays
        c.ignition = f.ignition;
        c.throttle = f.throttle;
        c.brake = f.brake;
        c.clutch = f.clutch;
        c.handbrake = f.handbrake;
        c.steer = f.steer;
        c.lever = f.lever;
        c.shiftUp = f.shiftUp;
        c.shiftDown = f.shiftDown;
        c.buttons = f.buttons;
        c.mode = f.mode;
        host_->setControls(c);
    }

    void SimAdapter::prepare(double rate, int maxBlock)
    {
        if (rate > 1000.0)
            host_->prepare(rate, maxBlock > 0 ? maxBlock : host_->maxBlock());
    }

    void SimAdapter::process(float* l, float* r, int n) noexcept
    {
        if (n <= 0)
            return;
        host_->process(l, r, n);
        tail_.write(l, r, n);
    }
}
