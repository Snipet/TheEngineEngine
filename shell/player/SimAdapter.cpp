// shell/player/SimAdapter.cpp: SimAdapter.h.
#include "player/SimAdapter.h"

#include "tee/Timeline.h"

#include "tee/Build.h"
#include "tee/Fields.h"
#include "tee/Program.h"
#include "tee/Schema.h"
#include "tee/Sim.h"
#include "tee/SimHost.h"
#include "tee/Telemetry.h"
#include "tee/Warm.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

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

    SimAdapter::~SimAdapter()
    {
        // the workers joined here, before anything they write goes (never left to a static's destructor)
        if (worker_.joinable())
            worker_.join();
        if (qualityWorker_.joinable())
            qualityWorker_.join();
    }

    std::unique_ptr<SimAdapter> SimAdapter::create(const Options& options, std::string& errors)
    {
        std::unique_ptr<SimAdapter> a(new SimAdapter());
        a->started_ = std::chrono::steady_clock::now();
        a->load_ = std::make_unique<LoadOptions>();
        LoadOptions& lo = *a->load_;
        lo.sim.optimisation = options.optimisation;
        lo.sim.checks = options.checks;
        lo.sampleRate = options.sampleRate;
        for (const auto& [path, value] : options.hold)
            lo.hold.push_back({ path, value });
        if (options.warm)
            lo.initial = warmEngine();

        // The front end alone, here: the program's errors, its Schema and its Fields (a tenth of a second). The
        // machine code is not made: no function is looked up.
        Program::Options po;
        po.optimisation = lo.sim.optimisation;
        po.checks = lo.sim.checks;
        po.defines = lo.sim.defines;
        a->preview_ = Program::fromDirectory(options.directory, lo.sim.entry, po, errors);
        if (!a->preview_)
        {
            if (errors.empty())
                errors = "the simulator did not compile, and FMinus gave no message";
            return nullptr;
        }
        std::string error;
        a->previewSchema_ = Schema::of(a->preview_->fminus(), lo.sim.module, error);
        if (a->previewSchema_)
            a->previewFields_ = Fields::of(*a->previewSchema_, error);
        if (!a->previewFields_)
        {
            errors = lo.sim.entry + ": error: " + (error.empty() ? "the program has no module " + lo.sim.module : error);
            return nullptr;
        }
        a->warnings_ = a->preview_->warnings();

        const Fields& fields = *a->previewFields_;
        a->host_ = std::make_unique<SimHost>();
        a->build_ = std::make_unique<tee::Build>(fields);
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
                a->telIndex_.push_back(0);
                a->telNames_.push_back(f.path.substr(4));   // without "tel."
            }
        }
        a->frame_ = std::make_unique<TelemetryFrame>();

        // A program given compiled: taken at once.
        if (options.sim)
        {
            a->sim_ = options.sim;
            a->workerState_.store(1, std::memory_order_release);
            a->adopt();
            if (!a->ready())
            {
                errors = a->compileError_.empty() ? std::string("the given program was not taken") : a->compileError_;
                return nullptr;
            }
            return a;
        }
        // The machine code: the whole compile again on a worker (Sim::compile; the front end is a small share of it).
        SimAdapter* self = a.get();
        const std::filesystem::path dir = options.directory;
        const Sim::Options so = lo.sim;
        a->worker_ = std::thread([self, dir, so] {
            std::string e;
            std::shared_ptr<Sim> sim = Sim::compile(dir, so, e);
            {
                const std::lock_guard<std::mutex> lock(self->compiled_);
                self->sim_ = std::move(sim);
                self->workerErrors_ = e;
            }
            self->workerState_.store(self->sim_ ? 1 : 2, std::memory_order_release);
        });
        if (!options.background && !a->waitReady(600.0))
        {
            errors = a->compileError_.empty() ? std::string("the simulator's machine code was not made") : a->compileError_;
            return nullptr;
        }
        return a;
    }

    void SimAdapter::requestQuality(int quality)
    {
        wantedQuality_ = quality == 1 ? 1 : 0;
        {
            const std::lock_guard<std::mutex> lock(qualityMutex_);
            qualityError_.clear();              // asked again: tried again
        }
        if (ready())
            startQuality();
    }

    void SimAdapter::startQuality()
    {
        if (qualityBusy_.load(std::memory_order_acquire) || wantedQuality_ == madeQuality_)
            return;
        if (qualityWorker_.joinable())
            qualityWorker_.join();
        const int q = wantedQuality_;
        qualityBusy_.store(true, std::memory_order_release);
        qualityWorker_ = std::thread([this, q] {
            std::string errors;
            const bool ok = host_->setQuality(q == 1 ? SimHost::Quality::Economy : SimHost::Quality::Full, errors);
            {
                const std::lock_guard<std::mutex> lock(qualityMutex_);
                qualityError_ = ok ? std::string() : (errors.empty() ? std::string("the quality was not changed") : errors);
            }
            if (ok)
                madeQuality_ = q;
            qualityBusy_.store(false, std::memory_order_release);
        });
    }

    std::string SimAdapter::qualityError() const
    {
        const std::lock_guard<std::mutex> lock(qualityMutex_);
        return qualityError_;
    }

    double SimAdapter::innerRate() const
    {
        return ready() ? host_->innerRate() : 0.0;
    }

    bool SimAdapter::waitQuality(double seconds)
    {
        const auto start = std::chrono::steady_clock::now();
        while (qualitySwitching() || (ready() && wantedQuality_ != madeQuality_ && qualityError().empty()))
        {
            if (!qualitySwitching())
                startQuality();
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() > seconds)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    double SimAdapter::compilingSeconds() const
    {
        return ready() ? 0.0 : std::chrono::duration<double>(std::chrono::steady_clock::now() - started_).count();
    }

    void SimAdapter::adopt()
    {
        if (ready() || held_ || !compileError_.empty())
            return;
        const int state = workerState_.load(std::memory_order_acquire);
        if (state == 0)
            return;
        if (worker_.joinable())
            worker_.join();
        std::shared_ptr<Sim> sim;
        {
            const std::lock_guard<std::mutex> lock(compiled_);
            sim = sim_;
            if (state == 2)
                compileError_ = workerErrors_.empty() ? std::string("the simulator's machine code was not made") : workerErrors_;
        }
        if (!sim)
            return;
        std::string errors;
        if (!host_->attach(sim, *load_, errors))
        {
            compileError_ = errors.empty() ? std::string("the host did not take the simulator") : errors;
            return;
        }
        if (rate_ > 1000.0)
            host_->prepare(rate_, maxBlock_ > 0 ? maxBlock_ : host_->maxBlock());
        // the document: the compiled program's Fields from now on (the same layout), and everything sent to the host
        build_->rebind(host_->fields());
        bound_ = sim;
        build_->attach(host_.get());
        build_->flush();
        compileSeconds_ = sim->program().compileSeconds() + sim->program().linkSeconds();
        codeBytes_ = sim->program().codeBytes();
        if (warnings_.empty())
            warnings_ = sim->program().warnings();
        ready_.store(true, std::memory_order_release);   // the audio thread plays it from its next call
        if (wantedQuality_ != madeQuality_)
            startQuality();
    }

    bool SimAdapter::waitReady(double seconds)
    {
        const auto start = std::chrono::steady_clock::now();
        while (!ready() && compileError_.empty())
        {
            if (!held_ && workerState_.load(std::memory_order_acquire) != 0)
            {
                adopt();
                continue;
            }
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() > seconds)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return ready();
    }

    void SimAdapter::frame()
    {
        if (!ready())
            adopt();
        // A quality change made: the host plays another program and refuses writes resolved against the one the
        // document is bound to, so the document follows (its values and undo kept, everything sent again). Only
        // between changes: the worker swaps the host's program (it starts on this thread, so none starts meanwhile).
        if (ready() && !qualitySwitching())
            if (std::shared_ptr<Sim> now = host_->sim(); now != nullptr && now != bound_)
            {
                build_->rebind(now->fields());
                bound_ = std::move(now);
            }
        if (ready())
            build_->flush();
        // a quality asked for while another was being made: its turn
        if (ready() && wantedQuality_ != madeQuality_ && !qualitySwitching() && qualityError().empty())
            startQuality();
        stale_ = true;              // the next value asked is of the newest block, however soon it is asked
    }

    std::string SimAdapter::why() const
    {
        if (!compileError_.empty())
            return compileError_;
        if (!ready())
            return std::string();
        // (an engine that diverged is put at rest and restarted by the host: no stop, said apart from the stops:
        // HostStats::engineFaults)
        const std::string reason = host_->stopReason();
        return reason.rfind("the engine diverged", 0) == 0 ? std::string() : reason;
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
        if (ready())
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
        // an element of an array ("car.wheelSlip[2]", "a.b[1][2]"), resolved as the host resolves it (Timeline.h), and
        // kept as a handle of its own: read from the same block as the scalars
        if (name.find('[') == std::string_view::npos)
            return -1;
        const FieldInfo* field = nullptr;
        std::size_t index = 0;
        std::string error;
        if (!telemetryElement(*previewFields_, name, field, index, error) || field == nullptr || field->table())
            return -1;
        tel_.push_back(field);
        telIndex_.push_back(index);
        telNames_.emplace_back(name);
        return static_cast<int>(tel_.size()) - 1;
    }

    HostStats SimAdapter::hostStats() const
    {
        refresh();
        return have_ ? frame_->host : HostStats{};
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
        if (!ready())
        {
            have_ = false;
            return;
        }
        have_ = host_->telemetry(*frame_) || have_;
    }

    double SimAdapter::value(int handle) const
    {
        if (handle < 0 || handle >= static_cast<int>(tel_.size()))
            return 0.0;
        if (!ready())
            return std::numeric_limits<double>::quiet_NaN();     // nothing plays yet: absent, drawn as a dash
        refresh();
        const auto h = static_cast<std::size_t>(handle);
        return have_ ? frame_->get(*tel_[h], telIndex_[h]) : 0.0;
    }

    double SimAdapter::load() const
    {
        refresh();
        return have_ ? frame_->host.loadMean : 0.0;
    }

    float SimAdapter::outputPeak() const
    {
        refresh();
        return have_ ? std::min(frame_->host.outputPeak, 1.0f) : 0.0f;
    }

    bool SimAdapter::hasControl(std::string_view name) const
    {
        double v = 0.0;
        return getControl(host_->controls(), name, v);
    }

    bool SimAdapter::pressCounter(std::string_view name)
    {
        if (!ready())
            return false;
        SimControls c = host_->controls();
        double v = 0.0;
        if (!getControl(c, name, v) || !setControl(c, name, v + 1.0))
            return false;
        host_->setControls(c);
        return true;
    }

    void SimAdapter::setControls(const ControlsFrame& f)
    {
        if (!ready())
            return;                             // nothing plays yet: the driver's frame waits for the next one
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
        // (the audio is stopped: kept for the host when it is not ready yet)
        if (rate > 1000.0)
        {
            rate_ = rate;
            maxBlock_ = maxBlock;
            load_->sampleRate = rate;
            if (maxBlock > 0)
                load_->maxBlock = std::max(load_->maxBlock, maxBlock);
            if (ready())
                host_->prepare(rate, maxBlock > 0 ? maxBlock : host_->maxBlock());
        }
    }

    void SimAdapter::process(float* l, float* r, int n) noexcept
    {
        if (n <= 0)
            return;
        if (!ready_.load(std::memory_order_acquire))
        {
            std::fill(l, l + n, 0.0f);
            std::fill(r, r + n, 0.0f);
            tail_.write(l, r, n);
            return;
        }
        host_->process(l, r, n);
        tail_.write(l, r, n);
    }
}
