// app/FrameSources.cpp: FrameSources.h.
#include "FrameSources.h"

#if defined(TEE_HAS_UI_FRAME)

#include "files/BuildFiles.h"
#include "input/Controls.h"
#include "player/SimAdapter.h"

#include "kit/bind/host/BuildDoc.h"
#include "tee/Build.h"
#include "tee/Fields.h"
#include "tee/SimHost.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace tee::app
{
    // ---- the Scope ------------------------------------------------------------------------------------------------
    namespace
    {
        // The arrays filled for each degree of the cycle (as many entries as scope.degrees says), and the pipe's (as
        // many as scope.pipePoints); any other field: all of it.
        bool perDegree(std::string_view name)
        {
            return name == "pressure" || name == "volume" || name == "liftIntake" || name == "liftExhaust";
        }
    }

    SimScope::SimScope(SimHost& host) : host_(host), copy_(std::make_unique<ScopeFrame>()) {}
    SimScope::~SimScope() = default;

    int SimScope::find(std::string_view name) const
    {
        const std::shared_ptr<Sim> sim = host_.sim();
        if (!sim || sim->fields().find("scope." + std::string(name)) == nullptr)
            return -1;
        for (std::size_t i = 0; i < names_.size(); ++i)
            if (names_[i] == name)
                return static_cast<int>(i);
        names_.emplace_back(name);
        return static_cast<int>(names_.size()) - 1;
    }

    void SimScope::fetch()
    {
        stale_ = false;
        ++fetches_;
        if (host_.scope(*copy_))
        {
            have_ = true;
            if (copy_->block != lastBlock_)
            {
                lastBlock_ = copy_->block;
                ++generation_;
            }
        }
    }

    int SimScope::read(int handle, float* out, int max)
    {
        if (handle < 0 || handle >= static_cast<int>(names_.size()) || out == nullptr || max <= 0)
            return 0;
        if (stale_)
            fetch();
        const std::shared_ptr<Sim> sim = host_.sim();
        if (!have_ || !sim)
            return 0;
        const Fields& fields = sim->fields();
        const std::string& name = names_[static_cast<std::size_t>(handle)];
        const FieldInfo* f = fields.find("scope." + name);
        if (f == nullptr || f->table())
            return 0;
        std::size_t n = f->count;
        const auto scalarOf = [&](const char* other) {
            const FieldInfo* g = fields.find(std::string("scope.") + other);
            return g != nullptr ? static_cast<std::size_t>(std::max(0.0, copy_->get(*g))) : n;
        };
        if (perDegree(name))
            n = std::min(n, scalarOf("degrees"));
        else if (name == "pipePressure")
            n = std::min(n, scalarOf("pipePoints"));
        n = std::min(n, static_cast<std::size_t>(max));
        for (std::size_t i = 0; i < n; ++i)
            out[i] = static_cast<float>(copy_->get(*f, i));
        return static_cast<int>(n);
    }

    // ---- the dyno -------------------------------------------------------------------------------------------------
    SimDyno::SimDyno(player::SimAdapter& player, Settings settings) : player_(player), settings_(settings) {}

    SimDyno::~SimDyno()
    {
        // the workers are joined here, on the UI thread, before the program they hold a share of is let go
        runner_.reset();
        service_.reset();
    }

    ui::DynoTrace SimDyno::trace(const DynoCurve& c, std::string name)
    {
        ui::DynoTrace t;
        t.name = std::move(name);
        t.fields = c.fields;
        t.values.assign(c.fields.size(), {});
        for (const DynoPoint& p : c.points)
        {
            t.speed.push_back(p.speed);
            t.torque.push_back(p.torque);
            t.power.push_back(p.power);
            for (std::size_t f = 0; f < t.values.size(); ++f)
                t.values[f].push_back(f < p.fields.size() ? p.fields[f] : std::nan(""));
        }
        return t;
    }

    DynoPlan SimDyno::predictedPlan() const
    {
        DynoPlan plan;
        const auto telemetry = [this](const char* name) {
            const int h = player_.findValue(name);
            return h >= 0 ? player_.value(h) : 0.0;
        };
        const double idle = telemetry("engine.idle"), red = telemetry("engine.redline");
        plan.from = settings_.from > 0.0 ? settings_.from : (idle > 50.0 ? idle : 105.0);
        plan.to = settings_.to > 0.0 ? settings_.to : (red > plan.from + 50.0 ? red : 680.0);
        // round to whole steps so a small wobble of the idle the telemetry reports keeps the same cache key
        plan.from = std::floor(plan.from / 10.0) * 10.0;
        plan.to = std::ceil(plan.to / 10.0) * 10.0;
        plan.rate = settings_.rate;
        plan.spacing = settings_.spacing;
        return plan;
    }

    void SimDyno::ask()
    {
        const std::shared_ptr<Sim> sim = player_.host().sim();
        if (!sim)
            return;
        if (!service_)
            service_ = std::make_unique<DynoService>(sim);
        DynoCurve cached;
        askedHash_ = seenHash_;
        if (service_->request(player_.build(), predictedPlan(), cached))
        {
            // an edit undone: the curve is known already
            if (!predicted_.empty())
                before_ = std::move(predicted_);
            predicted_ = trace(cached, "predicted");
            waiting_ = false;
            ++revision_;
            return;
        }
        waiting_ = true;
        sawBusy_ = false;
        sinceAsk_ = 0.0;
        ++revision_;
    }

    void SimDyno::tick(float dt)
    {
        const double step = std::max(0.0, static_cast<double>(dt));
        sinceEdit_ += step;
        sinceHash_ += step;
        sinceAsk_ += step;
        // the build's hash, a few times a second (it reads every field)
        if (sinceHash_ >= 0.1)
        {
            sinceHash_ = 0.0;
            const std::uint64_t h = player_.build().hash();
            if (h != seenHash_)
            {
                seenHash_ = h;
                sinceEdit_ = 0.0;
            }
        }
        if (seenHash_ != 0 && seenHash_ != askedHash_ && sinceEdit_ >= settings_.quiet)
            ask();

        if (service_)
        {
            DynoCurve curve;
            if (service_->take(curve))
            {
                if (!predicted_.empty())
                    before_ = std::move(predicted_);
                predicted_ = trace(curve, "predicted");
                lastRealTime_ = curve.realTime();
                lastWall_ = curve.wallSeconds;
                error_.clear();
                waiting_ = false;
                ++revision_;
            }
            const bool nowBusy = service_->busy();
            sawBusy_ = sawBusy_ || nowBusy;
            if (waiting_ && !nowBusy && (sawBusy_ || sinceAsk_ > 2.0))
            {
                const std::string e = service_->error();
                if (!e.empty())
                {
                    error_ = e;
                    waiting_ = false;
                    ++revision_;
                }
            }
            const int pct = static_cast<int>(service_->progress() * 100.0);
            if (nowBusy != lastBusy_ || (nowBusy && pct != lastProgress_))
            {
                lastBusy_ = nowBusy;
                lastProgress_ = pct;
                ++revision_;
            }
        }
        if (runner_)
        {
            DynoCurve curve;
            if (runner_->take(curve))
            {
                ui::DynoTrace t = trace(curve, runName_);
                runs_.push_back(std::move(t));
                runRunning_ = false;
                ++revision_;
            }
            else if (runRunning_ && !runner_->busy() && !runner_->error().empty() && sinceAsk_ > 0.5)
            {
                error_ = runner_->error();
                runRunning_ = false;
                ++revision_;
            }
        }
    }

    bool SimDyno::busy() const
    {
        return waiting_ || (service_ && service_->busy());
    }

    double SimDyno::progress() const
    {
        return service_ ? service_->progress() : 0.0;
    }

    const ui::DynoTrace& SimDyno::run(int i) const
    {
        return i >= 0 && i < runCount() ? runs_[static_cast<std::size_t>(i)] : DynoSource::run(i);
    }

    bool SimDyno::startRun(const ui::DynoRequest& r)
    {
        const std::shared_ptr<Sim> sim = player_.host().sim();
        if (!sim)
            return false;
        if (!runner_)
            runner_ = std::make_unique<DynoService>(sim);
        DynoPlan plan;
        plan.chassis = r.chassis;
        plan.from = r.from;
        plan.to = r.to;
        plan.rate = r.rate;
        plan.pedal = r.pedal;
        runName_ = r.name.empty() ? "run " + std::to_string(++runsMade_) : r.name;
        DynoCurve cached;
        if (runner_->request(player_.build(), plan, cached))
        {
            runs_.push_back(trace(cached, runName_));
            runRunning_ = false;
        }
        else
            runRunning_ = true;
        sinceAsk_ = 0.0;
        ++revision_;
        return true;
    }

    void SimDyno::stopRun()
    {
        // the service has no cancel of its own: a new one replaces it, and the old one's worker is cancelled and joined
        if (runRunning_)
        {
            runner_.reset();
            runRunning_ = false;
            ++revision_;
        }
    }

    bool SimDyno::waitForCurve(double seconds)
    {
        const auto start = std::chrono::steady_clock::now();
        if (!waiting_ && predicted_.empty())
        {
            seenHash_ = player_.build().hash();
            ask();
        }
        while (waiting_)
        {
            tick(0.0f);
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() > seconds)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return !predicted_.empty();
    }

    // ---- the services ---------------------------------------------------------------------------------------------
    namespace
    {
        // The frame's modes (0 bench, 1 dyno, 2 drive) and the simulator's (MODE_BENCH 0, MODE_DRIVE 1,
        // MODE_DYNO_ENGINE 2).
        int toSim(int frameMode) { return frameMode == 1 ? 2 : frameMode == 2 ? 1 : 0; }
        int toFrame(int simMode) { return simMode == 2 ? 1 : simMode == 1 ? 2 : 0; }
    }

    void addShellServices(ui::FrameServices& s, player::SimAdapter& player, input::Controls& controls,
                          const std::function<float()>& level)
    {
        s.simLoad = [&player] { return static_cast<float>(player.load()); };
        s.mode = [&controls] { return toFrame(controls.driver().settings().mode); };
        s.setMode = [&controls](int m) { controls.driver().setMode(toSim(m)); };
        s.clutch = [&controls](float v) { controls.driver().setClutchStrip(v, v >= 0.0f); };
        if (level)
            s.outputLevel = level;
    }

    void addFileServices(ui::FrameServices& s, files::BuildFile& file, const files::BuildLibrary& library)
    {
        s.buildName = [&file] {
            const std::string t = file.title();
            return t.size() > 2 && t.compare(t.size() - 2, 2, " *") == 0 ? t.substr(0, t.size() - 2) : t;
        };
        s.unsaved = [&file] { return file.dirty(); };
        s.builds = [&library] {
            std::vector<std::string> names;
            for (const files::BuildSummary& b : library.list())
                names.push_back(b.name);
            return names;
        };
        s.load = [&file, &library](const std::string& name, std::string& error) {
            std::vector<std::string> warnings;
            return library.load(file, name, warnings, error);
        };
        s.save = [&file, &library](const std::string& name, std::string& error) { return library.save(file, name, error); };
    }
}

#endif
