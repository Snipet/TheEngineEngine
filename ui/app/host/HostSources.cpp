// ui/app/host/HostSources.cpp: see HostSources.h.
#include "app/host/HostSources.h"

#include "tee/Fields.h"

#include <algorithm>

namespace tee::ui
{
    // ---- the dyno ---------------------------------------------------------------------------------------------------
    ServiceDyno::ServiceDyno(std::shared_ptr<tee::Sim> sim, const tee::Build& build, tee::DynoPlan plan)
        : build_(build), plan_(std::move(plan)), service_(sim), sim_(std::move(sim))
    {
        if (plan_.fields.empty())
            plan_.fields = { "boost.pressure", "engine.manifold" };
    }

    DynoTrace ServiceDyno::traceOf(const tee::DynoCurve& c, std::string name)
    {
        DynoTrace t;
        t.name = std::move(name);
        t.fields = c.fields;
        t.values.assign(c.fields.size(), {});
        for (const tee::DynoPoint& p : c.points)
        {
            t.speed.push_back(p.speed);
            t.torque.push_back(p.torque);
            t.power.push_back(p.power);
            for (std::size_t f = 0; f < t.values.size(); ++f)
                t.values[f].push_back(f < p.fields.size() ? p.fields[f] : 0.0);
        }
        return t;
    }

    void ServiceDyno::take(const DynoTrace& t)
    {
        if (!predicted_.empty())
        {
            before_ = predicted_;
            before_.name = "before";
        }
        predicted_ = t;
        ++revision_;
    }

    void ServiceDyno::tick(float dt)
    {
        // An edit (any change of the build's hash, an undo and a load too): wait for a quiet, then ask.
        const std::uint64_t h = build_.hash();
        if (h != seen_)
        {
            seen_ = h;
            quiet_ = first_ ? 1e-6 : kQuiet;
            first_ = false;
            ++revision_;
        }
        if (quiet_ > 0.0 && (quiet_ -= dt) <= 0.0)
        {
            quiet_ = 0.0;
            tee::DynoCurve cached;
            if (service_.request(build_, plan_, cached))
                take(traceOf(cached, "predicted"));
            else
                waiting_ = true;
            ++revision_;
        }
        tee::DynoCurve done;
        if (service_.take(done))
        {
            waiting_ = false;
            if (!done.cancelled)
                take(traceOf(done, "predicted"));
            ++revision_;
        }
        if (waiting_)
            ++revision_;   // its progress moves
        if (runner_ != nullptr)
        {
            tee::DynoCurve run;
            if (runner_->take(run))
            {
                running_ = false;
                if (!run.cancelled)
                    runs_.push_back(traceOf(run, runName_));
                ++revision_;
            }
        }
    }

    bool ServiceDyno::startRun(const DynoRequest& r)
    {
        if (runner_ == nullptr)
            runner_ = std::make_unique<tee::DynoService>(sim_);
        tee::DynoPlan p = plan_;
        p.chassis = r.chassis;
        p.from = r.from;
        p.to = r.to;
        p.rate = r.rate;
        p.pedal = r.pedal;
        runName_ = r.name.empty() ? "Run " + std::to_string(runs_.size() + 1) : r.name;
        tee::DynoCurve cached;
        if (runner_->request(build_, p, cached))
        {
            runs_.push_back(traceOf(cached, runName_));
            running_ = false;
        }
        else
            running_ = true;
        ++revision_;
        return true;
    }

    void ServiceDyno::stopRun()
    {
        // A request for nothing new cancels the one running; its curve, cancelled, is not kept.
        runner_.reset();
        running_ = false;
        ++revision_;
    }

    // ---- the Scope --------------------------------------------------------------------------------------------------
    SimScope::SimScope(tee::SimHost& host) : host_(host) {}

    int SimScope::find(std::string_view name) const
    {
        if (!host_.loaded())
            return -1;
        const tee::FieldInfo* f = host_.fields().find("scope." + std::string(name));
        if (f == nullptr)
            return -1;
        auto& fields = const_cast<std::vector<const tee::FieldInfo*>&>(fields_);
        const auto it = std::find(fields.begin(), fields.end(), f);
        if (it != fields.end())
            return static_cast<int>(it - fields.begin());
        fields.push_back(f);
        return static_cast<int>(fields.size()) - 1;
    }

    double SimScope::scalar(std::string_view name) const
    {
        const tee::FieldInfo* f = host_.fields().find("scope." + std::string(name));
        return f != nullptr && have_ ? frame_.get(*f) : 0.0;
    }

    int SimScope::read(int handle, float* out, int max)
    {
        if (handle < 0 || handle >= static_cast<int>(fields_.size()) || max <= 0)
            return 0;
        // Asking keeps the host copying for about half a second; the newest copy is kept.
        tee::ScopeFrame f;
        if (host_.scope(f))
        {
            if (!have_ || f.block != lastBlock_)
                ++generation_;
            frame_ = std::move(f);
            lastBlock_ = frame_.block;
            have_ = true;
        }
        if (!have_)
            return 0;
        const tee::FieldInfo& field = *fields_[static_cast<std::size_t>(handle)];
        std::size_t n = field.count;
        // The per-degree arrays hold a cycle of `degrees`; the pipe's `pipePoints`.
        if (field.path == "scope.pipePressure")
            n = std::min(n, static_cast<std::size_t>(std::max(0.0, scalar("pipePoints"))));
        else if (n > 1 && field.extents.size() == 1)
        {
            const double d = scalar("degrees");
            if (d > 0.0 && static_cast<std::size_t>(d) <= n)
                n = static_cast<std::size_t>(d);
        }
        n = std::min(n, static_cast<std::size_t>(max));
        for (std::size_t i = 0; i < n; ++i)
            out[i] = static_cast<float>(frame_.get(field, i));
        return static_cast<int>(n);
    }
}
