// host/src/SimHost.cpp: tee::SimHost (SimHost.h).
#include "tee/SimHost.h"

#include "tee/Denormals.h"
#include "tee/Reload.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstring>

namespace tee
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        // The loop budget, as the reference's player sets it: 2^32 loop iterations for a second of samples, 16
        // samples' worth at least a call, and 2^32 for an init or an end.
        constexpr double kLoopsPerSecond = 4294967296.0;
        constexpr std::int64_t kInitBudget = std::int64_t{1} << 32;

        double seconds(Clock::time_point a, Clock::time_point b)
        {
            return std::chrono::duration<double>(b - a).count();
        }

        double wallSeconds()
        {
            return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
        }
    } // namespace

    // ---- the controls by name -------------------------------------------------------------------------------------

    const std::vector<ControlField>& controlFields()
    {
#define TEE_CONTROL(name, isFloat) ControlField{#name, offsetof(SimControls, name), isFloat}
        static const std::vector<ControlField> list = {
            TEE_CONTROL(ignition, false),  TEE_CONTROL(throttle, true),  TEE_CONTROL(brake, true),
            TEE_CONTROL(clutch, true),     TEE_CONTROL(handbrake, true), TEE_CONTROL(steer, true),
            TEE_CONTROL(lever, false),     TEE_CONTROL(shiftUp, false),  TEE_CONTROL(shiftDown, false),
            TEE_CONTROL(buttons, false),   TEE_CONTROL(mode, false),     TEE_CONTROL(dynoSpeed, true),
            TEE_CONTROL(dynoRate, true),   TEE_CONTROL(holdSpeed, true), TEE_CONTROL(strip, false),
            TEE_CONTROL(scopeOn, false),   TEE_CONTROL(bakeMaps, false),
        };
#undef TEE_CONTROL
        return list;
    }

    bool setControl(SimControls& controls, std::string_view name, double value) noexcept
    {
        for (const ControlField& f : controlFields())
        {
            if (name != f.name)
                continue;
            unsigned char* at = reinterpret_cast<unsigned char*>(&controls) + f.offset;
            writeScalar(at, f.isFloat ? Scalar::Float : Scalar::Int, value);
            return true;
        }
        return false;
    }

    bool getControl(const SimControls& controls, std::string_view name, double& value) noexcept
    {
        for (const ControlField& f : controlFields())
        {
            if (name != f.name)
                continue;
            const unsigned char* at = reinterpret_cast<const unsigned char*>(&controls) + f.offset;
            value = readScalar(at, f.isFloat ? Scalar::Float : Scalar::Int);
            return true;
        }
        return false;
    }

    // ---- an instance, as the audio thread plays it ----------------------------------------------------------------

    struct SimHost::Live
    {
        std::shared_ptr<Sim> sim;
        std::unique_ptr<Instance> instance;
        std::uint32_t generation = 0;
        struct Slot
        {
            std::size_t offset;    // in the instance
            Scalar scalar;         // the program's type
            std::size_t member;    // in SimControls
            bool isFloat;
        };
        std::vector<Slot> controls;
        std::size_t stateOffset = 0;   // tel.engine.state, for the crank after a fault
        Scalar stateScalar = Scalar::None;
        std::size_t flagsOffset = 0;   // tel.engine.flags: FLAG_FAULT (4), the engine put at rest after it diverged
        Scalar flagsScalar = Scalar::None;
        struct Held
        {
            Schema::Handle handle;
            double value;
        };
        std::vector<Held> held;        // LoadOptions::hold, resolved
        std::size_t telOffset = 0, telSize = 0, scopeOffset = 0, scopeSize = 0;
        std::size_t moduleSize = 0;
        std::int64_t budgetPerSample = 0;
        bool begun = false;
        bool dead = false;             // tripped or stopped: never called again
        bool stoppedByCheck = false;   // its state may be anything: not even its end is called
    };

    namespace
    {
        constexpr long kFlagFault = 4;   // sim/spec/telemetry.fm's FLAG_FAULT
        constexpr const char* kEconomyDefine = "TEE_ENGINE_RATE_48000";   // sim/engine.fm's setupAll (D, e9a9748)

        // tel.engine.state and tel.engine.flags, where the program has them
        template <typename LiveT>
        void resolveEngineSlots(LiveT& live, const Fields& fields)
        {
            if (const FieldInfo* state = fields.find("tel.engine.state"); state != nullptr && state->count == 1)
            {
                live.stateOffset = state->offset;
                live.stateScalar = state->scalar;
            }
            if (const FieldInfo* flags = fields.find("tel.engine.flags"); flags != nullptr && flags->count == 1)
            {
                live.flagsOffset = flags->offset;
                live.flagsScalar = flags->scalar;
            }
        }

        // LoadOptions::hold's paths, found in the program (a path it lacks is skipped)
        template <typename LiveT>
        void resolveHeld(LiveT& live, const Sim& sim, const std::vector<HeldValue>& hold)
        {
            for (const HeldValue& h : hold)
            {
                Schema::Handle handle;
                std::string unused;
                if (sim.schema().find(h.path, handle, unused) && handle.scalar() != Scalar::None
                    && handle.scalar() != Scalar::Pointer)
                    live.held.push_back({handle, h.value});
            }
        }
    } // namespace

    // The session log's ring: records written by the audio thread, read by service().
    struct SimHost::LogRing
    {
        struct Header
        {
            std::int64_t sample;
            std::uint32_t offset;
            std::uint32_t size;
            std::int16_t section;
            std::uint8_t kind;
            std::uint8_t pad[5];
        };

        explicit LogRing(std::size_t bytes) : capacity(bytes), data(std::make_unique<unsigned char[]>(bytes)) {}

        bool push(const Header& h, const void* bytes) noexcept
        {
            const std::size_t need = (sizeof(Header) + h.size + 7) / 8 * 8;
            const std::uint64_t w = head.load(std::memory_order_relaxed);
            if (w + need - tail.load(std::memory_order_acquire) > capacity)
                return false;
            put(w, &h, sizeof h);
            put(w + sizeof h, bytes, h.size);
            head.store(w + need, std::memory_order_release);
            return true;
        }

        template <typename F>
        void drain(F&& take)
        {
            std::uint64_t r = tail.load(std::memory_order_relaxed);
            const std::uint64_t end = head.load(std::memory_order_acquire);
            std::vector<unsigned char> bytes;
            while (r < end)
            {
                Header h;
                get(r, &h, sizeof h);
                bytes.resize(h.size);
                get(r + sizeof h, bytes.data(), h.size);
                take(h, bytes);
                r += (sizeof(Header) + h.size + 7) / 8 * 8;
            }
            tail.store(r, std::memory_order_release);
        }

        void put(std::uint64_t at, const void* source, std::size_t n) noexcept
        {
            const unsigned char* in = static_cast<const unsigned char*>(source);
            for (std::size_t i = 0; i < n; ++i)
                data[(at + i) % capacity] = in[i];
        }

        void get(std::uint64_t at, void* destination, std::size_t n) const noexcept
        {
            unsigned char* out = static_cast<unsigned char*>(destination);
            for (std::size_t i = 0; i < n; ++i)
                out[i] = data[(at + i) % capacity];
        }

        std::size_t capacity;
        std::unique_ptr<unsigned char[]> data;
        std::atomic<std::uint64_t> head{0};
        std::atomic<std::uint64_t> tail{0};
        std::atomic<std::uint32_t> lost{0};
    };

    SimHost::SimHost() = default;

    SimHost::~SimHost()
    {
        stopWorker();
        if (Live* p = pending_.exchange(nullptr))
            destroyLive(p);
        for (auto& slot : retired_)
            if (Live* l = slot.exchange(nullptr))
                destroyLive(l);
        if (retireLater_ != nullptr)
            destroyLive(retireLater_);
        if (fading_ != nullptr)
            destroyLive(fading_);
        if (live_ != nullptr)
            destroyLive(live_);
        if (sim_ && sim_->options().checks)
            sim_->release();
    }

    bool SimHost::load(const std::filesystem::path& simDir, const LoadOptions& options, std::string& errors)
    {
        std::shared_ptr<Sim> sim = Sim::compile(simDir, options.sim, errors);
        if (!sim)
            return false;
        return attach(std::move(sim), options, errors);
    }

    bool SimHost::attach(std::shared_ptr<Sim> sim, const LoadOptions& options, std::string& errors)
    {
        if (sim_ != nullptr)
        {
            errors = "SimHost: a host is loaded once (make another for another program, or reload)\n";
            return false;
        }
        if (sim->options().checks && !sim->claim())
        {
            errors = "SimHost: this Sim was compiled with the checks and another host plays it: FMinus keeps one loop "
                     "budget and one bounds record a program, so compile another for this host\n";
            return false;
        }
        options_ = options;
        rate_ = options.sampleRate > 0.0 ? options.sampleRate : 48000.0;
        maxBlock_ = std::max(options.maxBlock, 16);
        const Fields& fields = sim->fields();
        commands_ = std::make_unique<CommandQueue>(options.commandSlots, options.poolBytes);
        shadow_.assign(sim->module().size, 0);
        sim->writeDefaults(shadow_.data());
        generation_ = fields.generation();
        controlsBuffer_.resize(sizeof(SimControls));
        controlsBuffer_.write(&controlsSent_, sizeof controlsSent_);
        telemetryRoom_ = std::max<std::size_t>(4096, 2 * fields.tel().size);
        telemetryBuffer_.resize(sizeof(HostStats) + 8 + telemetryRoom_);
        scopeRoom_ = std::max<std::size_t>(4096, 2 * fields.scope().size);
        scopeBuffer_.resize(16 + scopeRoom_);
        publishBuffer_.assign(std::max(sizeof(HostStats) + 8 + telemetryRoom_, 16 + scopeRoom_), 0);
        scratchLeft_.assign(static_cast<std::size_t>(maxBlock_), 0.0f);
        scratchRight_.assign(static_cast<std::size_t>(maxBlock_), 0.0f);
        sim_ = std::move(sim);
        Live* live = makeLive(sim_, errors);
        if (live == nullptr)
        {
            if (sim_->options().checks)
                sim_->release();
            sim_.reset();
            return false;
        }
        live_ = live;
        logStart_ = shadow_;
        if (options.log)
            logRing_ = std::make_unique<LogRing>(std::size_t{4} << 20);
        if (options.worker)
            worker_ = std::thread([this] { workerLoop(); });
        return true;
    }

    SimHost::Live* SimHost::makeLive(const std::shared_ptr<Sim>& sim, std::string& errors)
    {
        std::unique_ptr<Live> live(new Live());
        live->sim = sim;
        live->instance = std::make_unique<Instance>(sim->module());
        const Fields& fields = sim->fields();
        live->generation = fields.generation();
        live->moduleSize = sim->module().size;
        live->telOffset = fields.tel().offset;
        live->telSize = fields.tel().size;
        live->scopeOffset = fields.scope().offset;
        live->scopeSize = fields.scope().size;
        live->budgetPerSample = static_cast<std::int64_t>(kLoopsPerSecond / rate_);
        for (const ControlField& c : controlFields())
        {
            const FieldInfo* f = fields.find(std::string("ctl.") + c.name);
            if (f != nullptr && f->count == 1 && (f->scalar == Scalar::Float || f->scalar == Scalar::Int
                                                  || f->scalar == Scalar::Double || f->scalar == Scalar::Bool))
                live->controls.push_back({f->offset, f->scalar, c.offset, c.isFloat});
        }
        resolveHeld(*live, *sim, options_.hold);
        resolveEngineSlots(*live, fields);

        // the Spec: what the host has sent (its defaults and every write since), when it is of this program
        unsigned char* data = static_cast<unsigned char*>(live->instance->data());
        {
            const std::lock_guard<std::mutex> lock(producers_);
            if (generation_ == live->generation && shadow_.size() == live->moduleSize)
                std::memcpy(data + fields.spec().offset, shadow_.data() + fields.spec().offset, fields.spec().size);
            else
                sim->writeDefaults(data);
        }

        fminus::Program& fm = sim->program().fminus();
        const ScopedFlushDenormals flush;
        if (sim->options().checks)
        {
            fm.clearBoundsViolation();
            fm.setLoopBudget(kInitBudget);
        }
        sim->init()(data, static_cast<float>(rate_));
        live->begun = true;
        if (sim->options().checks)
        {
            if (const auto failed = fm.boundsViolation())
                errors = sim->options().module + "::init failed a bounds check: " + failed->message() + "\n";
            else if (fm.loopBudgetExceeded())
                errors = sim->options().module + "::init ran past its loop budget\n";
            if (!errors.empty())
            {
                live->dead = true;
                live->stoppedByCheck = true;
                destroyLive(live.release());
                return nullptr;
            }
        }
        return live.release();
    }

    void SimHost::destroyLive(Live* live)
    {
        if (live == nullptr)
            return;
        if (live->begun && !live->stoppedByCheck)
        {
            fminus::Program& fm = live->sim->program().fminus();
            const ScopedFlushDenormals flush;
            if (live->sim->options().checks)
                fm.setLoopBudget(kInitBudget);
            live->sim->end()(live->instance->data());
        }
        delete live;
    }

    void SimHost::retire(Live* live) noexcept
    {
        if (live == nullptr)
            return;
        for (auto& slot : retired_)
        {
            Live* expected = nullptr;
            if (slot.compare_exchange_strong(expected, live, std::memory_order_acq_rel))
                return;
        }
        // every slot taken (nobody has serviced the host for a while): try again next block. retireLater_ holds one;
        // a second while it is full is kept playing nowhere, and freed when the host goes.
        if (retireLater_ == nullptr)
            retireLater_ = live;
    }

    void SimHost::stopWorker()
    {
        workerStop_.store(true);
        if (worker_.joinable())
            worker_.join();
    }

    void SimHost::workerLoop()
    {
        while (!workerStop_.load())
        {
            service();
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
        }
    }

    // ---- the producers --------------------------------------------------------------------------------------------

    bool SimHost::set(FieldRef ref, double value, std::size_t index)
    {
        if (!sim_ || !ref.valid() || ref.field < 0)
            return false;
        const std::lock_guard<std::mutex> lock(producers_);
        if (ref.generation != generation_)
            return false;
        const FieldInfo& info = sim_->fields().all()[static_cast<std::size_t>(ref.field)];
        if (info.block != "spec" || info.kind == FieldKind::Rev || index >= ref.count)
            return false;
        const std::size_t step = scalarBytes(ref.scalar);
        if (step == 0)
            return false;
        // a table's own field (its n, say) has no range of its own: it is written as given, made whole
        const double v = info.table() ? (std::isfinite(value) ? value : 0.0) : info.clamp(value);
        unsigned char bytes[8] = {};
        writeScalar(bytes, ref.scalar, v);
        const std::uint32_t offset = ref.offset + static_cast<std::uint32_t>(index * step);
        if (!commands_->push(offset, bytes, static_cast<std::uint32_t>(step), ref.section, ref.generation))
        {
            refused_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        std::memcpy(shadow_.data() + offset, bytes, step);
        return true;
    }

    bool SimHost::setBytes(FieldRef ref, const void* data, std::size_t n)
    {
        if (!sim_ || !ref.valid() || ref.field < 0 || n != ref.size)
            return false;
        const std::lock_guard<std::mutex> lock(producers_);
        if (ref.generation != generation_)
            return false;
        const FieldInfo& info = sim_->fields().all()[static_cast<std::size_t>(ref.field)];
        if (info.block != "spec" || info.kind == FieldKind::Rev || ref.offset + n > shadow_.size())
            return false;
        if (!commands_->push(ref.offset, data, static_cast<std::uint32_t>(n), ref.section, ref.generation))
        {
            refused_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        std::memcpy(shadow_.data() + ref.offset, data, n);
        return true;
    }

    bool SimHost::readSpec(FieldRef ref, std::vector<unsigned char>& bytes) const
    {
        const std::lock_guard<std::mutex> lock(producers_);
        if (!ref.valid() || ref.generation != generation_ || ref.offset + ref.size > shadow_.size())
            return false;
        bytes.assign(shadow_.begin() + ref.offset, shadow_.begin() + ref.offset + ref.size);
        return true;
    }

    double SimHost::readSpec(FieldRef ref, std::size_t index) const
    {
        const std::lock_guard<std::mutex> lock(producers_);
        const std::size_t step = scalarBytes(ref.scalar);
        if (!ref.valid() || ref.generation != generation_ || index >= ref.count
            || ref.offset + (index + 1) * step > shadow_.size())
            return 0.0;
        return readScalar(shadow_.data() + ref.offset + index * step, ref.scalar);
    }

    void SimHost::setControls(const SimControls& controls)
    {
        const std::lock_guard<std::mutex> lock(controlsWriters_);
        controlsSent_ = controls;
        controlsBuffer_.write(&controlsSent_, sizeof controlsSent_);
    }

    SimControls SimHost::controls() const
    {
        const std::lock_guard<std::mutex> lock(controlsWriters_);
        return controlsSent_;
    }

    bool SimHost::telemetry(TelemetryFrame& out) const
    {
        if (!sim_)
            return false;
        std::vector<unsigned char>& raw = out.tel;
        raw.resize(telemetryBuffer_.size());
        if (!telemetryBuffer_.read(raw.data()))
            return false;
        std::memcpy(&out.host, raw.data(), sizeof(HostStats));
        std::uint32_t generation = 0, size = 0;
        std::memcpy(&generation, raw.data() + sizeof(HostStats), 4);
        std::memcpy(&size, raw.data() + sizeof(HostStats) + 4, 4);
        size = std::min<std::uint32_t>(size, static_cast<std::uint32_t>(telemetryRoom_));
        std::memmove(raw.data(), raw.data() + sizeof(HostStats) + 8, size);
        raw.resize(size);
        out.generation = generation;
        out.telOffset = generation == sim_->fields().generation() ? sim_->fields().tel().offset : 0;
        out.host.commandsRefused = refused_.load(std::memory_order_relaxed);
        return true;
    }

    bool SimHost::scope(ScopeFrame& out)
    {
        if (!sim_)
            return false;
        scopeAskedAt_.store(samplesNow_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        std::vector<unsigned char>& raw = out.bytes;
        raw.resize(scopeBuffer_.size());
        if (!scopeBuffer_.read(raw.data()))
            return false;
        std::uint32_t generation = 0, size = 0;
        std::memcpy(&out.block, raw.data(), 8);
        std::memcpy(&generation, raw.data() + 8, 4);
        std::memcpy(&size, raw.data() + 12, 4);
        size = std::min<std::uint32_t>(size, static_cast<std::uint32_t>(scopeRoom_));
        std::memmove(raw.data(), raw.data() + 16, size);
        raw.resize(size);
        out.generation = generation;
        out.scopeOffset = generation == sim_->fields().generation() ? sim_->fields().scope().offset : 0;
        return true;
    }

    std::string SimHost::stopReason() const
    {
        const std::lock_guard<std::mutex> lock(reasonMutex_);
        return stopReason_;
    }

    std::vector<LogEvent> SimHost::takeLog()
    {
        service();
        const std::lock_guard<std::mutex> lock(logMutex_);
        std::vector<LogEvent> out;
        out.swap(log_);
        return out;
    }

    // ---- off the audio thread: what it handed back, a fresh instance, the log -------------------------------------

    void SimHost::service()
    {
        const std::lock_guard<std::mutex> guard(serviceMutex_);
        for (auto& slot : retired_)
            if (Live* l = slot.exchange(nullptr, std::memory_order_acq_rel))
                destroyLive(l);

        // the load verdict, on the audio's clock (samples played since the last update)
        if (rate_ > 0.0)
        {
            const std::int64_t samples = samplesNow_.load(std::memory_order_relaxed);
            if (samples < fitSamples_)
                fitMeter_.reset();
            const double seconds = static_cast<double>(std::max<std::int64_t>(samples - fitSamples_, 0)) / rate_;
            fitSamples_ = samples;
            if (seconds > 0.0)
                fitsNow_.store(fitMeter_.update(loadPeakNow_.load(std::memory_order_relaxed),
                                                lateNow_.load(std::memory_order_relaxed), seconds),
                               std::memory_order_relaxed);
        }

        if (const std::uint32_t n = engineFaultsNow_.load(std::memory_order_relaxed); n != engineFaultsTold_)
        {
            engineFaultsTold_ = n;
            const std::lock_guard<std::mutex> lock(reasonMutex_);
            stopReason_ = "the engine diverged and was restarted (" + std::to_string(n) + (n == 1 ? " time" : " times")
                          + " since load): the simulator put it at rest afresh (FLAG_FAULT)";
        }

        if (restartWanted_.load(std::memory_order_acquire) && sim_)
        {
            const double now = wallSeconds();
            if (now - lastRestart_ >= 0.25 || restartsInARow_ == 0)
            {
                lastRestart_ = now;
                ++restartsInARow_;
                std::string words;
                const int kind = tripKind_.exchange(0);
                fminus::Program& fm = sim_->program().fminus();
                if (kind == 3)
                {
                    const auto failed = fm.boundsViolation();
                    words = sim_->options().module + "::process failed a bounds check"
                            + (failed ? ": " + failed->message() : std::string());
                }
                else if (kind == 2)
                    words = sim_->options().module + "::process ran past its loop budget";
                else
                    words = "the output guard refused a block: a sample not finite, or beyond "
                            + std::to_string(options_.absurdLimit) + " (" + std::to_string(options_.absurdLimit)
                            + " times full scale): the simulator was restarted"
                            + (options_.restartCrankSeconds > 0.0 ? " and cranked back to running" : ", stopped: press start");
                {
                    const std::lock_guard<std::mutex> lock(reasonMutex_);
                    stopReason_ = words;
                }
                restartWanted_.store(false, std::memory_order_release);
                std::string errors;
                if (Live* fresh = makeLive(sim_, errors))
                {
                    pendingIsReload_.store(0, std::memory_order_relaxed);
                    if (Live* old = pending_.exchange(fresh, std::memory_order_acq_rel))
                        destroyLive(old);
                }
                else
                {
                    const std::lock_guard<std::mutex> lock(reasonMutex_);
                    stopReason_ += "; a fresh instance failed too: " + errors;
                }
            }
        }

        if (logRing_)
        {
            const std::lock_guard<std::mutex> lock(logMutex_);
            logRing_->drain([this](const LogRing::Header& h, const std::vector<unsigned char>& bytes) {
                LogEvent e;
                e.sample = h.sample;
                e.kind = static_cast<LogEvent::Kind>(h.kind);
                e.offset = h.offset;
                e.section = h.section;
                if (e.kind == LogEvent::Kind::Controls)
                    std::memcpy(&e.controls, bytes.data(), std::min(bytes.size(), sizeof(SimControls)));
                else
                    e.bytes = bytes;
                log_.push_back(std::move(e));
            });
        }
    }

    bool SimHost::setQuality(Quality quality, std::string& errors)
    {
        if (!sim_)
        {
            errors = "setQuality: nothing loaded";
            return false;
        }
        if (quality == quality_)
            return true;
        if (sim_->directory().empty())
        {
            errors = "setQuality: the program was compiled from memory, not from a directory";
            return false;
        }
        Sim::Options o = sim_->options();
        o.defines.erase(kEconomyDefine);
        if (quality == Quality::Economy)
            o.defines[kEconomyDefine] = "1";
        std::shared_ptr<Sim> next = Sim::compile(sim_->directory(), o, errors);
        if (!next)
            return false;
        // does it run at the rate asked? (a program without the define's hook runs at its own)
        const double want = quality == Quality::Economy ? 48000.0 : 96000.0;
        Schema::Handle rate;
        std::string unused;
        if (next->schema().find("rate", rate, unused))
        {
            Instance probe(next->module());
            next->writeDefaults(probe.data());
            next->init()(probe.data(), static_cast<float>(rate_));
            const double seen = probe.read(rate);
            next->end()(probe.data());
            if (std::fabs(seen - want) > 0.5)
            {
                errors = "setQuality: the program does not take " + std::string(kEconomyDefine) + " (it runs at "
                         + std::to_string(seen) + " Hz inside)";
                return false;
            }
            innerRate_ = seen;
        }
        if (!reloadWith(std::move(next), false, errors))
            return false;
        quality_ = quality;
        return true;
    }

    bool SimHost::reload(std::shared_ptr<Sim> next, std::string& errors)
    {
        return reloadWith(std::move(next), true, errors);
    }

    bool SimHost::reloadWith(std::shared_ptr<Sim> next, bool carry, std::string& errors)
    {
        if (!sim_ || !next)
            return false;
        const Fields& nf = next->fields();
        if (nf.tel().size > telemetryRoom_ || nf.scope().size > scopeRoom_)
        {
            errors = "reload: the new program's tel or scope block is larger than the host made room for at load; "
                     "start the application again\n";
            return false;
        }
        if (next->options().checks && !next->claim())
        {
            errors = "reload: the new Sim, compiled with the checks, is another host's\n";
            return false;
        }
        const std::shared_ptr<Sim> old = sim_;

        // the playing instance's bytes, taken by the audio thread at a block's end (or here, when it is not running)
        snapshot_.assign(old->module().size, 0);
        snapshotState_.store(1, std::memory_order_release);
        const Clock::time_point asked = Clock::now();
        while (snapshotState_.load(std::memory_order_acquire) != 2 && seconds(asked, Clock::now()) < 0.3)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        int expected = 1;
        if (snapshotState_.compare_exchange_strong(expected, 0))
        {
            // nobody plays: the audio thread is stopped, and its instance is ours to read
            if (live_ != nullptr && live_->instance)
                std::memcpy(snapshot_.data(), live_->instance->data(), std::min(snapshot_.size(), live_->moduleSize));
        }
        snapshotState_.store(0, std::memory_order_release);

        // the new Spec: the defaults, then every field of the host's own Spec carried by name
        std::vector<unsigned char> shadow(next->module().size, 0);
        next->writeDefaults(shadow.data());
        {
            const std::lock_guard<std::mutex> lock(producers_);
            carryField(old->schema(), shadow_.data(), next->schema(), shadow.data(), "spec");
        }

        // a fresh instance of the new program at the new Spec, init run, then the state carried over
        std::unique_ptr<Live> live(new Live());
        live->sim = next;
        live->instance = std::make_unique<Instance>(next->module());
        live->generation = nf.generation();
        live->moduleSize = next->module().size;
        live->telOffset = nf.tel().offset;
        live->telSize = nf.tel().size;
        live->scopeOffset = nf.scope().offset;
        live->scopeSize = nf.scope().size;
        live->budgetPerSample = static_cast<std::int64_t>(kLoopsPerSecond / rate_);
        for (const ControlField& c : controlFields())
        {
            const FieldInfo* f = nf.find(std::string("ctl.") + c.name);
            if (f != nullptr && f->count == 1 && f->scalar != Scalar::None && f->scalar != Scalar::Pointer)
                live->controls.push_back({f->offset, f->scalar, c.offset, c.isFloat});
        }
        resolveHeld(*live, *next, options_.hold);
        resolveEngineSlots(*live, nf);
        unsigned char* data = static_cast<unsigned char*>(live->instance->data());
        std::memcpy(data + nf.spec().offset, shadow.data() + nf.spec().offset, nf.spec().size);
        {
            const ScopedFlushDenormals flush;
            fminus::Program& fm = next->program().fminus();
            if (next->options().checks)
            {
                fm.clearBoundsViolation();
                fm.setLoopBudget(kInitBudget);
            }
            next->init()(data, static_cast<float>(rate_));
            live->begun = true;
        }
        if (carry)
            carryState(old->schema(), snapshot_.data(), next->schema(), data);
        std::memcpy(data + nf.spec().offset, shadow.data() + nf.spec().offset, nf.spec().size);
        for (const Section& s : nf.sections())
        {
            std::int32_t rev = 0;
            std::memcpy(&rev, data + s.revOffset, 4);
            ++rev;
            std::memcpy(data + s.revOffset, &rev, 4);
            std::memcpy(shadow.data() + s.revOffset, &rev, 4);
        }

        {
            const std::lock_guard<std::mutex> lock(producers_);
            shadow_.swap(shadow);
            generation_ = nf.generation();
            sim_ = next;
        }
        if (old->options().checks)
            old->release();
        pendingIsReload_.store(carry ? 1 : 2, std::memory_order_relaxed);
        if (Live* previous = pending_.exchange(live.release(), std::memory_order_acq_rel))
            destroyLive(previous);
        return true;
    }

    // ---- the audio thread -----------------------------------------------------------------------------------------

    void SimHost::prepare(double sampleRate, int maxBlock)
    {
        if (!sim_)
            return;
        // the audio is stopped: everything handed over or back is ours
        service();
        if (Live* p = pending_.exchange(nullptr))
            destroyLive(p);
        if (fading_ != nullptr)
            destroyLive(fading_);
        fading_ = nullptr;
        if (retireLater_ != nullptr)
            destroyLive(retireLater_);
        retireLater_ = nullptr;
        if (live_ != nullptr)
            destroyLive(live_);
        live_ = nullptr;
        rate_ = sampleRate > 0.0 ? sampleRate : rate_;
        maxBlock_ = std::max(maxBlock, 16);
        scratchLeft_.assign(static_cast<std::size_t>(maxBlock_), 0.0f);
        scratchRight_.assign(static_cast<std::size_t>(maxBlock_), 0.0f);
        std::string errors;
        live_ = makeLive(sim_, errors);
        if (live_ == nullptr)
        {
            const std::lock_guard<std::mutex> lock(reasonMutex_);
            stopReason_ = errors;
        }
        fadeLeft_ = fadeInLeft_ = 0;
        restartWanted_.store(false);
    }

    void SimHost::logEvent(LogEvent::Kind kind, std::uint32_t offset, std::int16_t section, const void* bytes,
                           std::uint32_t size) noexcept
    {
        if (!logRing_)
            return;
        LogRing::Header h{};
        h.sample = stats_.samples;
        h.offset = offset;
        h.size = size;
        h.section = section;
        h.kind = static_cast<std::uint8_t>(kind);
        if (!logRing_->push(h, bytes))
            logRing_->lost.fetch_add(1, std::memory_order_relaxed);
    }

    void SimHost::applyCommands(Live& live) noexcept
    {
        sectionsTouched_ = 0;
        unsigned char* data = static_cast<unsigned char*>(live.instance->data());
        const std::size_t taken = commands_->drain([&](const CommandQueue::Command& c, const unsigned char* bytes) {
            if (c.generation > live.generation)
                return false;   // a newer program's: it waits for the reload the audio thread has not taken yet
            if (c.generation == live.generation && c.offset + c.size <= live.moduleSize)
            {
                std::memcpy(data + c.offset, bytes, c.size);
                if (c.section >= 0 && c.section < 32)
                    sectionsTouched_ |= 1u << c.section;
                logEvent(LogEvent::Kind::Spec, c.offset, c.section, bytes, c.size);
            }
            return true;
        });
        stats_.commandsApplied += static_cast<std::uint32_t>(taken);
        if (sectionsTouched_ == 0)
            return;
        const std::vector<Section>& sections = live.sim->fields().sections();
        for (std::size_t s = 0; s < sections.size() && s < 32; ++s)
        {
            if ((sectionsTouched_ & (1u << s)) == 0)
                continue;
            std::int32_t rev = 0;
            std::memcpy(&rev, data + sections[s].revOffset, 4);
            ++rev;
            std::memcpy(data + sections[s].revOffset, &rev, 4);
        }
    }

    void SimHost::writeControls(Live& live) noexcept
    {
        SimControls c;
        if (controlsBuffer_.read(&c, 4) && !(c == controlsApplied_))
        {
            controlsApplied_ = c;
            logEvent(LogEvent::Kind::Controls, 0, -1, &c, sizeof c);
        }
        c = controlsApplied_;
        if (crankLeft_ > 0 && &live == live_)   // (the instance fading out runs on as it was)
        {
            // a fresh instance after a fault: an ignition left on cranks it until it runs (LoadOptions)
            const bool running = live.stateScalar != Scalar::None
                                 && readScalar(static_cast<const unsigned char*>(live.instance->data()) + live.stateOffset,
                                               live.stateScalar) == 2.0;
            if (running || c.ignition < 1)
                crankLeft_ = 0;
            else
                c.ignition = 2;
        }
        const std::int64_t asked = scopeAskedAt_.load(std::memory_order_relaxed);
        if (asked >= 0 && static_cast<double>(stats_.samples - asked) < 0.5 * rate_)
            c.scopeOn = 1;
        unsigned char* data = static_cast<unsigned char*>(live.instance->data());
        const unsigned char* from = reinterpret_cast<const unsigned char*>(&c);
        for (const Live::Slot& slot : live.controls)
            writeScalar(data + slot.offset, slot.scalar,
                        readScalar(from + slot.member, slot.isFloat ? Scalar::Float : Scalar::Int));
    }

    bool SimHost::runChunk(Live& live, float* left, float* right, int n) noexcept
    {
        Sim& sim = *live.sim;
        const bool checks = sim.options().checks;
        fminus::Program& fm = sim.program().fminus();
        if (checks)
            fm.setLoopBudget(live.budgetPerSample * std::max(n, 16));
        for (const Live::Held& h : live.held)
            live.instance->write(h.handle, h.value);
        sim.process()(live.instance->data(), left, right, n);
        if (checks)
        {
            int kind = 0;
            if (fm.boundsViolation())
                kind = 3;
            else if (fm.loopBudgetExceeded())
                kind = 2;
            if (kind != 0)
            {
                live.stoppedByCheck = true;
                tripKind_.store(kind, std::memory_order_relaxed);
                return false;
            }
        }
        // the safety stage (LoadOptions): beyond absurdLimit or not finite is a fault; beyond guardLimit is clamped
        const float limit = options_.guardLimit;
        const float absurd = std::max(options_.absurdLimit, limit);
        float largest = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float a = std::fabs(left[i]), b = std::fabs(right[i]);
            if (!(a <= absurd) || !(b <= absurd))
            {
                int none = 0;
                tripKind_.compare_exchange_strong(none, 1, std::memory_order_relaxed);
                return false;
            }
            largest = std::max(largest, std::max(a, b));
        }
        if (largest > limit)
        {
            for (int i = 0; i < n; ++i)
            {
                left[i] = std::clamp(left[i], -limit, limit);
                right[i] = std::clamp(right[i], -limit, limit);
            }
            clippedNow_ = true;
        }
        stats_.largestSample = std::max(stats_.largestSample, largest);
        return true;
    }

    void SimHost::publish(Live& live) noexcept
    {
        HostStats s = stats_;
        unsigned char* out = publishBuffer_.data();
        const std::uint32_t generation = live.generation;
        const std::uint32_t telSize = static_cast<std::uint32_t>(std::min(live.telSize, telemetryRoom_));
        std::memcpy(out, &s, sizeof s);
        std::memcpy(out + sizeof s, &generation, 4);
        std::memcpy(out + sizeof s + 4, &telSize, 4);
        std::memcpy(out + sizeof s + 8, static_cast<const unsigned char*>(live.instance->data()) + live.telOffset, telSize);
        telemetryBuffer_.write(out, sizeof s + 8 + telSize);

        const std::int64_t asked = scopeAskedAt_.load(std::memory_order_relaxed);
        const bool wanted = asked >= 0 && static_cast<double>(stats_.samples - asked) < 0.5 * rate_;
        if (wanted && static_cast<double>(stats_.samples - lastScopeSample_) >= rate_ / 60.0)
        {
            lastScopeSample_ = stats_.samples;
            const std::uint32_t scopeSize = static_cast<std::uint32_t>(std::min(live.scopeSize, scopeRoom_));
            std::memcpy(out, &stats_.block, 8);
            std::memcpy(out + 8, &generation, 4);
            std::memcpy(out + 12, &scopeSize, 4);
            std::memcpy(out + 16, static_cast<const unsigned char*>(live.instance->data()) + live.scopeOffset, scopeSize);
            scopeBuffer_.write(out, 16 + scopeSize);
        }
    }

    void SimHost::process(float* left, float* right, int frames) noexcept
    {
        if (frames <= 0)
            return;
        const ScopedFlushDenormals flush;
        const Clock::time_point t0 = Clock::now();

        if (Live* p = pending_.exchange(nullptr, std::memory_order_acq_rel))
        {
            const int reloadKind = pendingIsReload_.exchange(0, std::memory_order_relaxed);
            const bool isReload = reloadKind != 0;
            if (reloadKind == 2)   // a fresh engine (another quality): an ignition left on cranks it back
                crankLeft_ = static_cast<std::int64_t>(options_.restartCrankSeconds * rate_);
            if (isReload && live_ != nullptr && !live_->dead)
            {
                if (fading_ != nullptr)
                    retire(fading_);
                fading_ = live_;
                fadeTotal_ = fadeLeft_ = std::max(1, static_cast<int>(options_.crossfadeSeconds * rate_));
                ++stats_.reloads;
            }
            else
            {
                if (live_ != nullptr)
                    retire(live_);
                fadeInTotal_ = fadeInLeft_ = std::max(1, static_cast<int>(options_.fadeInSeconds * rate_));
                if (!isReload)
                {
                    ++stats_.restarts;
                    crankLeft_ = static_cast<std::int64_t>(options_.restartCrankSeconds * rate_);
                }
                else
                    ++stats_.reloads;
            }
            live_ = p;
            stats_.faulted = 0;
        }
        if (retireLater_ != nullptr)
        {
            Live* again = retireLater_;
            retireLater_ = nullptr;
            retire(again);
        }

        Live* live = live_;
        if (live == nullptr)
        {
            std::memset(left, 0, static_cast<std::size_t>(frames) * sizeof(float));
            std::memset(right, 0, static_cast<std::size_t>(frames) * sizeof(float));
            stats_.samples += frames;
            samplesNow_.store(stats_.samples, std::memory_order_relaxed);
            return;
        }
        applyCommands(*live);
        writeControls(*live);
        if (fading_ != nullptr)
            writeControls(*fading_);
        const Clock::time_point t1 = Clock::now();

        float peak = 0.0f;
        for (int at = 0; at < frames; at += maxBlock_)
        {
            const int n = std::min(maxBlock_, frames - at);
            float* l = left + at;
            float* r = right + at;
            bool ok = !live->dead && runChunk(*live, l, r, n);
            if (!ok)
            {
                if (!live->dead)
                {
                    live->dead = true;
                    ++stats_.guardTrips;
                    if (live->stoppedByCheck)
                        ++stats_.stops;
                    stats_.faulted = 1;
                    restartWanted_.store(true, std::memory_order_release);
                    if (fading_ != nullptr)
                    {
                        retire(fading_);
                        fading_ = nullptr;
                    }
                }
                // what it wrote is not played: a short fade from the last sample played
                const int ramp = std::min(n, 64);
                for (int i = 0; i < n; ++i)
                {
                    const float g = i < ramp ? 1.0f - static_cast<float>(i + 1) / static_cast<float>(ramp) : 0.0f;
                    l[i] = lastLeft_ * g;
                    r[i] = lastRight_ * g;
                }
                fadeInLeft_ = 0;
            }
            else
            {
                // the program put a diverged engine at rest (FLAG_FAULT): counted, and cranked back as after a restart
                if (live->flagsScalar != Scalar::None)
                {
                    const double flags = readScalar(static_cast<const unsigned char*>(live->instance->data())
                                                        + live->flagsOffset,
                                                    live->flagsScalar);
                    if ((static_cast<long>(flags) & kFlagFault) != 0)
                    {
                        ++stats_.engineFaults;
                        engineFaultsNow_.store(stats_.engineFaults, std::memory_order_relaxed);
                        crankLeft_ = static_cast<std::int64_t>(options_.restartCrankSeconds * rate_);
                    }
                }
            }
            if (ok && fading_ != nullptr)
            {
                float* ol = scratchLeft_.data();
                float* orr = scratchRight_.data();
                const bool oldOk = runChunk(*fading_, ol, orr, n);
                for (int i = 0; i < n; ++i)
                {
                    const float g = fadeLeft_ > 0 ? static_cast<float>(fadeLeft_) / static_cast<float>(fadeTotal_) : 0.0f;
                    if (fadeLeft_ > 0)
                        --fadeLeft_;
                    const float a = oldOk ? g : 0.0f;
                    l[i] = l[i] * (1.0f - g) + ol[i] * a;
                    r[i] = r[i] * (1.0f - g) + orr[i] * a;
                }
                if (fadeLeft_ <= 0 || !oldOk)
                {
                    if (!oldOk)
                        fading_->dead = true;
                    retire(fading_);
                    fading_ = nullptr;
                }
            }
            if (ok && fadeInLeft_ > 0)
            {
                for (int i = 0; i < n && fadeInLeft_ > 0; ++i)
                {
                    const float g = 1.0f - static_cast<float>(fadeInLeft_) / static_cast<float>(fadeInTotal_);
                    l[i] *= g;
                    r[i] *= g;
                    --fadeInLeft_;
                }
            }
            for (int i = 0; i < n; ++i)
                peak = std::max(peak, std::max(std::fabs(l[i]), std::fabs(r[i])));
            lastLeft_ = l[n - 1];
            lastRight_ = r[n - 1];
        }
        const Clock::time_point t2 = Clock::now();

        stats_.samples += frames;
        ++stats_.block;
        stats_.outputPeak = peak;
        if (clippedNow_)
        {
            ++stats_.clippedBlocks;
            clippedNow_ = false;
        }
        crankLeft_ = std::max<std::int64_t>(crankLeft_ - frames, 0);
        stats_.restarting = crankLeft_ > 0 ? 1 : 0;
        samplesNow_.store(stats_.samples, std::memory_order_relaxed);
        if (snapshotState_.load(std::memory_order_acquire) == 1)
        {
            std::memcpy(snapshot_.data(), live->instance->data(), std::min(snapshot_.size(), live->moduleSize));
            snapshotState_.store(2, std::memory_order_release);
        }
        publish(*live);
        const Clock::time_point t3 = Clock::now();

        // the meters, published with the next block's telemetry
        const double duration = static_cast<double>(frames) / rate_;
        const double load = seconds(t0, t3) / duration;
        const double host = (seconds(t0, t1) + seconds(t2, t3)) * 1e6;
        const double a = std::min(1.0, duration);
        stats_.load = load;
        stats_.loadMean = stats_.block == 1 ? load : stats_.loadMean + a * (load - stats_.loadMean);
        windowPeak_ = std::max(windowPeak_, load);
        peakWindow_ += duration;
        stats_.loadPeak = std::max(stats_.loadPeak, windowPeak_);
        if (peakWindow_ >= 1.0)
        {
            stats_.loadPeak = windowPeak_;
            windowPeak_ = 0.0;
            peakWindow_ = 0.0;
        }
        stats_.hostMicros = host;
        stats_.hostMicrosMean = stats_.block == 1 ? host : stats_.hostMicrosMean + 0.01 * (host - stats_.hostMicrosMean);
        stats_.hostMicrosPeak = std::max(stats_.hostMicrosPeak, host);
        if (load >= 1.0)
            ++stats_.lateCallbacks;
        loadPeakNow_.store(stats_.loadPeak, std::memory_order_relaxed);
        lateNow_.store(stats_.lateCallbacks, std::memory_order_relaxed);
        stats_.fits = fitsNow_.load(std::memory_order_relaxed);
    }
} // namespace tee
