// ui/app/Demo.cpp: see Demo.h. The numbers are made up but plausible (a 5.7 L V8 with a turbo, in third gear), so a
// picture of a workspace looks like what it will show.
#include "app/Demo.h"

#include <algorithm>
#include <limits>
#include <cmath>

namespace tee::ui
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kRpm = kPi / 30.0;   // rad/s in one rpm
        constexpr double kRedline = 6800.0, kIdle = 850.0, kLimiter = 6600.0;
        constexpr int kDegrees = 720, kPipePoints = 64;

        // The names, in the order of sim/spec/telemetry.fm.
        const char* const kNames[] = {
            "engine.state", "engine.speed", "engine.redline", "engine.idle", "engine.throttle", "engine.manifold",
            "engine.coolant", "engine.oilPressure", "engine.flags", "engine.cycle", "engine.crank",
            "boost.pressure", "exhaust.portTemperature", "ecu.knockRetard",
            "car.speed", "car.gear", "car.brake", "car.clutch", "sound.peak",
            "ecu.advance", "sound.rate",
            "engine.torque", "engine.power", "engine.firing", "engine.imep", "engine.bmep", "engine.veAir",
        };
        const char* const kScope[] = { "degrees", "cycles", "pressure", "volume", "liftExhaust", "liftIntake",
                                       "pipePoints", "pipePressure", "portIntake", "portExhaust" };

        double smooth(double x) { x = std::clamp(x, 0.0, 1.0); return x * x * (3.0 - 2.0 * x); }

        // The script's pedal at t (0..8 s): idle, a stab, a pull, the limiter, a lift.
        double pedalAt(double t)
        {
            if (t < 1.0) return 0.0;
            if (t < 1.4) return smooth((t - 1.0) / 0.15) * (1.0 - smooth((t - 1.25) / 0.15)) * 0.6;
            if (t < 2.0) return 0.0;
            if (t < 6.2) return smooth((t - 2.0) / 0.3);
            if (t < 6.6) return 1.0 - smooth((t - 6.2) / 0.3);
            return 0.0;
        }
        // The engine's speed at t, rpm: what the pedal would make of it, eased.
        double rpmAt(double t)
        {
            if (t < 1.0) return kIdle;
            if (t < 2.0) return kIdle + 1800.0 * std::sin(kPi * std::clamp((t - 1.05) / 0.95, 0.0, 1.0)) * std::exp(-0.6 * (t - 1.0));
            if (t < 6.0) return kIdle + (kLimiter - kIdle) * smooth((t - 2.05) / 3.9);
            if (t < 6.4) return kLimiter - 60.0 * std::fabs(std::sin(40.0 * t));   // on the limiter
            return kIdle + (kLimiter - 60.0 - kIdle) * std::exp(-(t - 6.4) * 2.2);
        }
    }

    bool DemoHooks::wants(const char* id) const
    {
        return all || std::find(only.begin(), only.end(), std::string_view(id)) != only.end();
    }

    std::string demoFieldConflicts()
    {
        // Each hook is given the fields so far (as demoFields gives them) and may add; what it adds that is already
        // there is the conflict.
        std::vector<FieldInfo> acc = demoFields(DemoHooks::none());
        std::map<std::string, std::string> owner;
        for (const FieldInfo& f : acc)
            owner[f.path] = "the base demo";
        std::string out;
        for (std::size_t k = 0; k < registeredDemos().size(); ++k)
        {
            const std::size_t before = acc.size();
            DemoExtras x;
            x.fields = &acc;
            registeredDemos()[k](x);
            const std::string who = std::string(registeredDemoIds()[k]) + "'s hook";
            for (std::size_t i = before; i < acc.size(); ++i)
            {
                const auto [it, fresh] = owner.emplace(acc[i].path, who);
                if (!fresh)
                    out += "field " + acc[i].path + ": " + it->second + " and " + who + "\n";
            }
        }
        return out;
    }

    std::vector<FieldInfo> demoFields(const DemoHooks& hooks)
    {
        std::vector<FieldInfo> out;
        std::string group;
        const auto add = [&](std::string path, FieldKind kind, double lo, double hi, double def, std::string label,
                             Quantity q = Quantity::none) -> FieldInfo& {
            FieldInfo f;
            f.path = std::move(path);
            f.kind = kind;
            f.min = lo;
            f.max = hi;
            f.def = def;
            f.label = std::move(label);
            f.quantity = q;
            f.group = group;
            out.push_back(std::move(f));
            return out.back();
        };
        const auto choice = [&](std::string path, std::string label, std::vector<std::string> names, int def = 0) -> FieldInfo& {
            FieldInfo& f = add(std::move(path), FieldKind::choice, 0, static_cast<double>(names.size() - 1), def, std::move(label));
            f.choices = std::move(names);
            return f;
        };
        const auto axes = [](FieldInfo& f, Quantity x, double x0, double x1, Quantity y = Quantity::none, double y0 = 0,
                             double y1 = 0) {
            f.axes = { x, y };
            f.axisLo = { x0, y0 };
            f.axisHi = { x1, y1 };
        };

        group = "Block";
        add("engine.rev", FieldKind::integer, 0, 1e9, 0, "Revision").hidden = true;
        choice("engine.layout", "Layout", { "I4", "I6", "V6", "V8", "V10", "V12", "Flat 4", "Flat 6" }, 3);
        add("engine.cylinders", FieldKind::integer, 1, 16, 8, "Cylinders");
        add("engine.bore", FieldKind::number, 0.06, 0.12, 0.1016, "Bore", Quantity::length);
        add("engine.stroke", FieldKind::number, 0.05, 0.12, 0.0884, "Stroke", Quantity::length);
        add("engine.rod", FieldKind::number, 0.10, 0.20, 0.1554, "Rod", Quantity::length);
        add("engine.compression", FieldKind::number, 7.0, 15.0, 10.0, "Compression", Quantity::ratio).help =
            "The cylinder's volume at the bottom of the stroke over its volume at the top";   // hover help's test (frame.help)
        group = "Rotating";
        add("engine.flywheel", FieldKind::number, 0.05, 1.0, 0.25, "Flywheel", Quantity::inertia).logTaper = true;
        add("engine.friction", FieldKind::number, 0, 1, 0.3, "Friction", Quantity::fraction);

        group = "Throttle";
        add("intake.throttle", FieldKind::number, 0.04, 0.12, 0.08, "Throttle bore", Quantity::length);
        add("intake.plenum", FieldKind::number, 0.001, 0.02, 0.006, "Plenum", Quantity::volume);
        add("intake.runner", FieldKind::number, 0.1, 0.6, 0.3, "Runner length", Quantity::length);
        add("intake.filter", FieldKind::number, 0, 1, 0.2, "Filter loss", Quantity::fraction);

        choice("fuel.kind", "Fuel", { "Petrol", "E85", "Methanol", "Diesel" });
        add("fuel.octane", FieldKind::number, 85, 110, 95, "Octane").decimals = 0;

        group = "Intake cam";
        add("valve.intakeDuration", FieldKind::number, 180, 320, 248, "Duration").unit = "deg";
        add("valve.intakeLift", FieldKind::number, 0.006, 0.016, 0.0115, "Lift", Quantity::length);
        add("valve.intakeCentre", FieldKind::number, 90, 130, 108, "Centreline").unit = "deg";
        group = "Exhaust cam";
        add("valve.exhaustDuration", FieldKind::number, 180, 320, 254, "Duration").unit = "deg";
        add("valve.exhaustLift", FieldKind::number, 0.006, 0.016, 0.011, "Lift", Quantity::length);
        add("valve.exhaustCentre", FieldKind::number, 90, 130, 114, "Centreline").unit = "deg";
        add("valve.phaser", FieldKind::toggle, 0, 1, 1, "Intake phaser");
        {
            FieldInfo& lift = add("valve.profile", FieldKind::curve, 0, 1, 0.5, "Lift profile", Quantity::fraction);
            axes(lift, Quantity::angle, 0, 180);
        }

        group = "Turbo";
        choice("boost.kind", "Aspiration", { "Natural", "Turbo", "Twin turbo", "Supercharger" }, 1);
        add("boost.level", FieldKind::number, 0, 200e3, 80e3, "Boost", Quantity::pressure);
        add("boost.size", FieldKind::number, 0, 1, 0.5, "Turbo size", Quantity::fraction);
        add("boost.blowOff", FieldKind::toggle, 0, 1, 1, "Blow-off");
        group = "Wastegate";
        add("boost.wastegateSpring", FieldKind::number, 20e3, 150e3, 50e3, "Spring", Quantity::pressure);
        {
            FieldInfo& duty = add("boost.duty", FieldKind::map, 0, 1, 0.4, "Duty", Quantity::fraction);
            axes(duty, Quantity::rotation, 1000 * kRpm, 8000 * kRpm, Quantity::pressure, 0, 200e3);
        }

        group = "Headers";
        choice("exhaust.headers", "Headers", { "Log", "Shorty", "Long tube" }, 2);
        add("exhaust.primary", FieldKind::number, 0.2, 1.2, 0.75, "Primary length", Quantity::length);
        add("exhaust.pipe", FieldKind::number, 0.035, 0.09, 0.0635, "Pipe", Quantity::length);
        group = "System";
        add("exhaust.crossover", FieldKind::toggle, 0, 1, 1, "X-pipe");
        add("exhaust.muffler", FieldKind::number, 0, 1, 0.5, "Muffler", Quantity::fraction);
        add("exhaust.cat", FieldKind::toggle, 0, 1, 1, "Converter");

        group = "Ignition";
        add("ecu.timing", FieldKind::number, -10, 10, 0, "Timing", Quantity::angle);
        {
            FieldInfo& spark = add("ecu.spark", FieldKind::map, -10, 50, 20, "Spark advance", Quantity::angle);
            axes(spark, Quantity::rotation, 800 * kRpm, 7500 * kRpm, Quantity::pressure, 20e3, 250e3);
        }
        group = "Limiter";
        add("ecu.limiter", FieldKind::number, 3000 * kRpm, 9500 * kRpm, kLimiter * kRpm, "Limiter", Quantity::rotation);
        add("ecu.launch", FieldKind::number, 2000 * kRpm, 6000 * kRpm, 4000 * kRpm, "Launch", Quantity::rotation);
        add("ecu.pops", FieldKind::toggle, 0, 1, 0, "Overrun pops");
        {
            FieldInfo& curve = add("ecu.idleCurve", FieldKind::curve, 600 * kRpm, 1400 * kRpm, 850 * kRpm, "Idle by coolant",
                                   Quantity::rotation);
            axes(curve, Quantity::temperature, 233.15, 383.15);
        }

        group = "Body";
        choice("car.kind", "Vehicle", { "Car", "Truck" });
        add("car.mass", FieldKind::number, 600, 4000, 1550, "Mass", Quantity::mass);
        add("car.wheelbase", FieldKind::number, 2.0, 3.5, 2.7, "Wheelbase", Quantity::length);
        group = "Driveline";
        choice("car.drive", "Driven wheels", { "Rear", "Front", "All" });
        add("car.finalDrive", FieldKind::number, 2.0, 5.5, 3.55, "Final drive", Quantity::ratio);
        add("car.tyre", FieldKind::number, 0.25, 0.45, 0.33, "Tyre radius", Quantity::length);
        add("car.tractionControl", FieldKind::toggle, 0, 1, 0, "Traction control");
        add("car.autoClutch", FieldKind::toggle, 0, 1, 1, "Automatic clutch");

        group = "";
        add("env.temperature", FieldKind::number, 233.15, 323.15, 293.15, "Air temperature", Quantity::temperature);
        add("env.pressure", FieldKind::number, 60e3, 105e3, 101.325e3, "Air pressure", Quantity::pressure);
        add("env.grade", FieldKind::number, -0.2, 0.2, 0.0, "Grade", Quantity::fraction);
        choice("env.surface", "Surface", { "Dry tarmac", "Wet tarmac", "Gravel", "Snow" });

        choice("sound.view", "Listener", { "Exhaust", "Cabin", "Engine bay", "Trackside" });
        add("sound.distance", FieldKind::number, 0.5, 100, 5, "Distance", Quantity::length).logTaper = true;
        add("sound.room", FieldKind::number, 0, 1, 0.2, "Room", Quantity::fraction);
        add("sound.level", FieldKind::number, 0, 1, 0.7, "Level", Quantity::fraction);
        for (std::size_t k = 0; k < registeredDemos().size(); ++k)
            if (hooks.wants(registeredDemoIds()[k]))
            {
                DemoExtras x;
                x.fields = &out;
                registeredDemos()[k](x);
            }
        // A field defined twice (the base and a hook, or two hooks): the first is kept (ui.demo.hooks says which).
        std::vector<FieldInfo> unique;
        for (FieldInfo& f : out)
            if (std::none_of(unique.begin(), unique.end(), [&](const FieldInfo& u) { return u.path == f.path; }))
                unique.push_back(std::move(f));
        return unique;
    }

    // ---- the engine ------------------------------------------------------------------------------------------------
    DemoEngine::DemoEngine(const FieldDoc* doc, const DemoHooks& hooks) : doc_(doc)
    {
        for (const char* n : kNames)
            owner_[n] = "the base demo";
        for (const char* n : kScope)
            owner_[std::string("scope.") + n] = "the base demo";
        for (std::size_t k = 0; k < registeredDemos().size(); ++k)
            if (hooks.wants(registeredDemoIds()[k]))
            {
                hookId_ = std::string(registeredDemoIds()[k]) + "'s hook";
                DemoExtras x;
                x.engine = this;
                registeredDemos()[k](x);
            }
        hookId_ = "a test";
        for (const char* n : kNames)
            names_.emplace_back(n);
        values_.assign(names_.size(), 0.0);
        predicted_ = curveFor(fieldOr("engine.compression", 10.0), fieldOr("boost.level", 80e3), "predicted");
        update();
        seenRevision_ = doc_ != nullptr ? doc_->revision() : 0;
    }

    double DemoEngine::fieldOr(std::string_view path, double fallback) const
    {
        if (doc_ == nullptr)
            return fallback;
        const int f = doc_->find(path);
        return f < 0 ? fallback : doc_->get(f);
    }

    void DemoEngine::setTime(double t)
    {
        t_ = std::fmod(std::max(0.0, t), kPeriod);
        // The crank's phase follows the speed's integral; a fixed function of t keeps a frame repeatable.
        phase_ = std::fmod(t_ * 37.0, 1.0);
        update();
    }

    void DemoEngine::advance(double dt)
    {
        setTime(t_ + dt);
        ++generation_;
    }

    void DemoEngine::tick(float dt)
    {
        // The script moves on, with a new Scope copy, while the engine turns (engine.state held at 0: it stands still,
        // as a stopped engine's telemetry does).
        if (const int st = findValue("engine.state"); !frozen_ && (st < 0 || value(st) > 0.5))
            advance(static_cast<double>(dt));
        // The dyno: an edit is noticed, a quiet of 0.3 s waited, then 0.4 s "running" before the new curve is in.
        if (doc_ != nullptr && doc_->revision() != seenRevision_)
        {
            seenRevision_ = doc_->revision();
            quiet_ = 0.3;
            waiting_ = -1.0;
            ++dynoRevision_;
        }
        if (quiet_ > 0.0 && (quiet_ -= dt) <= 0.0)
        {
            quiet_ = 0.0;
            waiting_ = 0.4;
            ++requests_;
            ++dynoRevision_;
        }
        if (waiting_ >= 0.0)
        {
            waiting_ -= dt;
            ++dynoRevision_;
            if (waiting_ < 0.0)
            {
                before_ = predicted_;
                before_.name = "before";
                predicted_ = curveFor(fieldOr("engine.compression", 10.0), fieldOr("boost.level", 80e3), "predicted");
            }
        }
        if (runLeft_ > 0.0)
        {
            // The run so far: the points of the whole curve up to where 2 s of running have reached.
            DynoTrace whole = curveFor(fieldOr("engine.compression", 10.0), fieldOr("boost.level", 80e3) * runPlan_.pedal, "so far");
            const auto n = static_cast<std::size_t>(static_cast<double>(whole.speed.size()) * std::clamp(1.0 - runLeft_ / 2.0, 0.0, 1.0));
            whole.speed.resize(n);
            whole.torque.resize(n);
            whole.power.resize(n);
            for (auto& v : whole.values)
                v.resize(n);
            partial_ = std::move(whole);
            ++dynoRevision_;
        }
        if (runLeft_ > 0.0 && (runLeft_ -= dt) <= 0.0)
        {
            runLeft_ = 0.0;
            DynoTrace r = curveFor(fieldOr("engine.compression", 10.0), fieldOr("boost.level", 80e3) * runPlan_.pedal,
                                   runPlan_.name.empty() ? "Run " + std::to_string(runs_.size() + 1) : runPlan_.name);
            r.settled = runPlan_.steps.empty() ? -1 : runPlan_.settleExhaust ? 1 : 0;   // as card C's runner says it
            runs_.push_back(std::move(r));
            ++dynoRevision_;
        }
    }

    double DemoEngine::progress() const { return waiting_ >= 0.0 ? std::clamp(1.0 - waiting_ / 0.4, 0.0, 1.0) : 0.0; }

    bool DemoEngine::startRun(const DynoRequest& r)
    {
        runPlan_ = r;
        runLeft_ = 2.0;
        ++dynoRevision_;
        return true;
    }

    void DemoEngine::update()
    {
        const double t = t_;
        rpm_ = rpmAt(t);
        throttle_ = std::max(0.04, pedalAt(t));
        const double w = rpm_ * kRpm;
        const double boost = std::max(0.0, (rpm_ - 2600.0) / 2400.0) * throttle_;
        const double boostPa = std::min(1.0, boost) * fieldOr("boost.level", 80e3);
        const double manifold = 28e3 + throttle_ * (101e3 - 28e3) + boostPa;
        const double gear = 3.0;
        // The brake torque: the predicted curve's at this speed, by the throttle (a part-throttle engine makes less);
        // the brake mean effective pressure from it (4 pi T / displacement, 5.7 L).
        const double torque = std::max(0.0, interpolate(predicted_, w) * (0.15 + 0.85 * throttle_) - 25.0);
        const double bmep = 4.0 * 3.14159265358979 * torque / 0.0057;
        const double carSpeed = w / (1.36 * fieldOr("car.finalDrive", 3.55)) * fieldOr("car.tyre", 0.33);
        const double v[] = {
            2.0, w, kRedline * kRpm, kIdle * kRpm, throttle_, manifold,
            361.0 + 4.0 * throttle_, 180e3 + 50.0 * rpm_, rpm_ > kLimiter - 80.0 ? 1.0 : 0.0, phase_, phase_ * 720.0,
            manifold - 101325.0,   // boost: over the air outside, negative (vacuum) under a closed throttle
            720.0 + 420.0 * throttle_ + 0.03 * rpm_, rpm_ > 5200.0 && throttle_ > 0.9 ? 1.5 : 0.0,
            carSpeed, gear, 0.0, 0.0, 0.25 + 0.5 * throttle_,
            12.0 + 22.0 * std::min(1.0, rpm_ / 5000.0) - 8.0 * throttle_, 48000.0,
            torque, torque * w, rpm_ / 60.0 * 4.0, bmep * 1.18 + 1.2e5, bmep, 0.55 + 0.4 * throttle_ + 0.3 * std::min(1.0, boostPa / 1e5),
        };
        for (std::size_t i = 0; i < values_.size(); ++i)
            values_[i] = v[i];
    }

    void DemoEngine::addTelemetry(std::string name, std::function<double(const DemoEngine&)> value)
    {
        if (const auto it = owner_.find(name); it != owner_.end())
            conflicts_ += "telemetry " + name + ": " + it->second + " and " + hookId_ + "\n";
        else
            owner_[name] = hookId_;
        extraTel_.emplace_back(std::move(name), std::move(value));
    }

    void DemoEngine::addScope(std::string name, std::function<int(const DemoEngine&, float*, int)> read)
    {
        const std::string key = "scope." + name;
        if (const auto it = owner_.find(key); it != owner_.end())
            conflicts_ += "Scope " + name + ": " + it->second + " and " + hookId_ + "\n";
        else
            owner_[key] = hookId_;
        extraScope_.emplace_back(std::move(name), std::move(read));
    }

    std::vector<std::string> DemoEngine::baseTelemetry() { return { std::begin(kNames), std::end(kNames) }; }
    std::vector<std::string> DemoEngine::baseScope() { return { std::begin(kScope), std::end(kScope) }; }

    double DemoEngine::interpolate(const DynoTrace& c, double speed)
    {
        if (c.speed.empty())
            return 0.0;
        if (speed <= c.speed.front())
            return c.torque.front() * speed / std::max(c.speed.front(), 1.0);
        for (std::size_t i = 1; i < c.speed.size(); ++i)
            if (speed <= c.speed[i])
            {
                const double t = (speed - c.speed[i - 1]) / (c.speed[i] - c.speed[i - 1]);
                return c.torque[i - 1] + (c.torque[i] - c.torque[i - 1]) * t;
            }
        return c.torque.back();
    }

    void DemoEngine::set(std::string_view name, double v)
    {
        if (auto it = heldIndex_.find(name); it != heldIndex_.end())
            heldValues_[it->second] = v;
        else
        {
            heldIndex_.emplace(std::string(name), heldNames_.size());
            heldNames_.emplace_back(name);
            heldValues_.push_back(v);
        }
        forgotten_.erase(std::string(name));
    }

    void DemoEngine::forget(std::string_view name)
    {
        // Its held value goes (its handle stays its own and reads NaN); a new reader finds no such name.
        if (auto it = heldIndex_.find(name); it != heldIndex_.end())
            heldValues_[it->second] = std::numeric_limits<double>::quiet_NaN();
        forgotten_.insert(std::string(name));
    }

    const double* DemoEngine::held(std::string_view name) const
    {
        if (auto it = heldIndex_.find(name); it != heldIndex_.end() && forgotten_.count(name) == 0)
            return &heldValues_[it->second];
        return nullptr;
    }

    int DemoEngine::findValue(std::string_view name) const
    {
        if (forgotten_.count(name) != 0)
            return -1;
        if (auto it = heldIndex_.find(name); it != heldIndex_.end())
            return 1000 + static_cast<int>(it->second);
        for (std::size_t i = 0; i < names_.size(); ++i)
            if (names_[i] == name)
                return static_cast<int>(i);
        for (std::size_t i = 0; i < extraTel_.size(); ++i)
            if (extraTel_[i].first == name)
                return 2000 + static_cast<int>(i);
        return -1;
    }

    double DemoEngine::value(int handle) const
    {
        // A handle read after its name was forgotten reads NaN; a held value overrides the script's of the same name.
        constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
        if (handle >= 2000)
        {
            const auto k = static_cast<std::size_t>(handle - 2000);
            if (k >= extraTel_.size())
                return 0.0;
            if (forgotten_.count(extraTel_[k].first) != 0)
                return kNaN;
            if (const double* h = held(extraTel_[k].first))
                return *h;
            return extraTel_[k].second(*this);
        }
        if (handle >= 1000)
        {
            const auto k = static_cast<std::size_t>(handle - 1000);
            if (k >= heldNames_.size())
                return 0.0;
            return forgotten_.count(heldNames_[k]) != 0 ? kNaN : heldValues_[k];
        }
        if (handle < 0 || handle >= static_cast<int>(values_.size()))
            return 0.0;
        const std::string& name = names_[static_cast<std::size_t>(handle)];
        if (forgotten_.count(name) != 0)
            return kNaN;
        if (const double* h = held(name))
            return *h;
        return values_[static_cast<std::size_t>(handle)];
    }

    float DemoEngine::outputPeak() const { return static_cast<float>(0.25 + 0.5 * throttle_); }

    int DemoEngine::outputTail(float* mono, int max) const
    {
        // The firing frequency's sawtooth with its second harmonic, at 48 kHz.
        const double f = rpm_ / 60.0 * 4.0;
        for (int i = 0; i < max; ++i)
        {
            const double ph = std::fmod(f * static_cast<double>(i) / 48000.0, 1.0);
            mono[i] = static_cast<float>((0.2 + 0.3 * throttle_) * ((ph * 2.0 - 1.0) * 0.7 + 0.3 * std::sin(4.0 * kPi * ph)));
        }
        return max;
    }

    // ---- the Scope --------------------------------------------------------------------------------------------------
    int DemoEngine::find(std::string_view name) const
    {
        for (int i = 0; i < static_cast<int>(std::size(kScope)); ++i)
            if (name == kScope[i])
                return i;
        for (std::size_t i = 0; i < extraScope_.size(); ++i)
            if (extraScope_[i].first == name)
                return 100 + static_cast<int>(i);
        return -1;
    }

    int DemoEngine::read(int handle, float* out, int max)
    {
        ++reads_;
        if (handle < 0 || max <= 0)
            return 0;
        if (handle >= 100)
        {
            const auto k = static_cast<std::size_t>(handle - 100);
            return k < extraScope_.size() ? extraScope_[k].second(*this, out, max) : 0;
        }
        const double bore = fieldOr("engine.bore", 0.1016), stroke = fieldOr("engine.stroke", 0.0884);
        const double rod = fieldOr("engine.rod", 0.1554), cr = fieldOr("engine.compression", 10.0);
        const double swept = kPi / 4.0 * bore * bore * stroke, clearance = swept / (cr - 1.0);
        const auto volumeAt = [&](double deg) {
            const double a = deg * kPi / 180.0, r = stroke / 2.0;
            const double x = r * (1.0 - std::cos(a)) + rod - std::sqrt(rod * rod - r * r * std::sin(a) * std::sin(a));
            return clearance + kPi / 4.0 * bore * bore * x;   // 0 deg: top dead centre of the power stroke
        };
        const double manifold = values_[5];
        const auto lift = [](double deg, double open, double dur) {
            const double u = std::fmod(deg - open + 720.0, 720.0) / dur;
            return u < 1.0 ? std::pow(std::sin(kPi * u), 1.6) : 0.0;
        };
        switch (handle)
        {
            case 0: out[0] = kDegrees; return 1;
            case 1: out[0] = static_cast<float>(generation_ / 4); return 1;
            case 6: out[0] = kPipePoints; return 1;
            case 7:
            {
                const int n = std::min(max, kPipePoints);
                for (int i = 0; i < n; ++i)
                {
                    const double x = static_cast<double>(i) / (kPipePoints - 1);
                    out[i] = static_cast<float>(101325.0 + 30e3 * throttle_ * std::exp(-2.0 * x)
                                                * std::sin(2.0 * kPi * (3.0 * x - phase_ * 4.0)));
                }
                return n;
            }
            default: break;
        }
        const int n = std::min(max, kDegrees);
        const double v0 = volumeAt(180.0);
        const double peak = std::max(manifold, 30e3) * std::pow(cr, 1.3) * (2.2 + 1.8 * throttle_);
        for (int i = 0; i < n; ++i)
        {
            const double deg = static_cast<double>(i);
            const double vol = volumeAt(deg);
            double p;
            if (deg < 180.0)        // power: from the burn's peak, expanding
                p = peak * std::pow(volumeAt(12.0) / std::max(vol, volumeAt(12.0)), 1.3) * smooth(deg / 14.0 + 0.4);
            else if (deg < 360.0)   // exhaust
                p = 108e3 + 40e3 * std::exp(-(deg - 180.0) / 25.0);
            else if (deg < 540.0)   // intake
                p = manifold;
            else                    // compression, then the spark's start
                p = manifold * std::pow(v0 / vol, 1.3) + (deg > 705.0 ? (deg - 705.0) / 15.0 * 0.3 * peak : 0.0);
            switch (handle)
            {
                case 2: out[i] = static_cast<float>(p); break;
                case 3: out[i] = static_cast<float>(vol); break;
                case 4: out[i] = static_cast<float>(lift(deg, 130.0, 254.0)); break;
                case 5: out[i] = static_cast<float>(lift(deg, 345.0, 248.0)); break;
                case 8:   // the intake port: the manifold, pulled down while the valve is open, a ringing after it shuts
                    out[i] = static_cast<float>(manifold * (1.0 - 0.18 * lift(deg, 345.0, 248.0))
                                                + 6e3 * std::exp(-std::fmod(deg - 593.0 + 720.0, 720.0) / 90.0)
                                                      * std::sin(std::fmod(deg - 593.0 + 720.0, 720.0) / 18.0));
                    break;
                case 9:   // the exhaust port: the blowdown's pulse as the valve opens, then the stroke's push
                    out[i] = static_cast<float>(104e3 + (60e3 + 120e3 * throttle_) * lift(deg, 130.0, 254.0)
                                                            * std::exp(-std::max(0.0, deg - 140.0) / 60.0));
                    break;
                default: return 0;
            }
        }
        return n;
    }

    // ---- the dyno ---------------------------------------------------------------------------------------------------
    DynoTrace DemoEngine::curveFor(double compression, double boost, const std::string& name) const
    {
        DynoTrace c;
        c.name = name;
        c.fields = { "boost.pressure" };
        c.values.resize(1);
        const double gain = (1.0 + 0.035 * (compression - 10.0)) * (1.0 + boost / 101325.0 * 0.45);
        for (double rpm = 1500.0; rpm <= 7000.0 + 1e-9; rpm += 250.0)
        {
            const double x = (rpm - 1500.0) / 5500.0;
            const double spool = smooth((rpm - 2200.0) / 2000.0);
            const double base = 380.0 + 140.0 * std::sin(kPi * std::min(1.0, x * 1.15)) - 90.0 * x * x;
            const double tq = base * (1.0 + (gain - 1.0) * spool);
            c.speed.push_back(rpm * kRpm);
            c.torque.push_back(tq);
            c.power.push_back(tq * rpm * kRpm);
            c.values[0].push_back(boost * spool);
            // Knock (card C's knock-limited points): past 160 kPa of boost, from 4500 rpm up, the retard growing with
            // both (the default 80 kPa, and the tests' edits to 150, knock nowhere).
            const double excess = (boost - 160e3) / 40e3;
            const bool knocks = excess > 0.0 && rpm >= 4500.0;
            const double lo = knocks ? 1.0 + 6.0 * std::min(1.0, excess) * (rpm - 4500.0) / 2500.0 : 0.0;
            c.limited.push_back(rpm > kLimiter ? 1 : 0);   // the limiter's points (card C's PredictPoint::limited)
            c.knock.push_back(knocks ? 1 : 0);
            c.retardMin.push_back(lo);
            c.retardMax.push_back(knocks ? lo + 4.0 : 0.0);
        }
        return c;
    }

    // ---- the rig ----------------------------------------------------------------------------------------------------
    DemoRig::DemoRig(double t, const DemoHooks& hooks) : doc(demoFields(hooks)), engine(&doc, hooks) { at(t); }

    void DemoRig::at(double t)
    {
        engine.setTime(t);
        const int th = engine.findValue("engine.throttle");
        sent.throttle = static_cast<float>(engine.value(th));
        sent.clutch = 0.0f;
        sent.brake = 0.0f;
        sent.ignition = 1;
    }
}
