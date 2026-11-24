// host/test/Scenario.cpp: the test library (tee/test/Scenario.h).
#include "tee/test/Scenario.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <mutex>
#include <tuple>

namespace tee::test
{
    namespace
    {
        // The directories and the compiled programs, kept for the process and never destroyed: a Sim held by a
        // static would be destroyed after FMinus's and LLVM's own statics at exit, and its Program's destructor then
        // runs on a JIT already torn down (a crash at exit on macOS, where the order differs from Linux's).
        struct Shared
        {
            std::mutex mutex;
            std::filesystem::path sim, content;
            std::map<std::tuple<std::string, std::string, std::string, int>, std::shared_ptr<Sim>> cache;
        };

        Shared& shared()
        {
            static Shared* const kept = new Shared();
            return *kept;
        }
    } // namespace

    void setSimDirectory(std::filesystem::path directory)
    {
        const std::lock_guard<std::mutex> lock(shared().mutex);
        shared().sim = std::move(directory);
    }

    void setContentDirectory(std::filesystem::path directory)
    {
        const std::lock_guard<std::mutex> lock(shared().mutex);
        shared().content = std::move(directory);
    }

    std::filesystem::path simDirectory()
    {
        const std::lock_guard<std::mutex> lock(shared().mutex);
        if (!shared().sim.empty())
            return shared().sim;
        if (const char* d = std::getenv("TEE_SIM_DIR"); d != nullptr && *d != '\0')
            return d;
        return std::filesystem::path(TEE_TEST_SOURCE_DIR) / "sim";
    }

    std::filesystem::path contentDirectory()
    {
        const std::lock_guard<std::mutex> lock(shared().mutex);
        if (!shared().content.empty())
            return shared().content;
        if (const char* d = std::getenv("TEE_CONTENT_DIR"); d != nullptr && *d != '\0')
            return d;
        return std::filesystem::path(TEE_TEST_SOURCE_DIR) / "content";
    }

    std::shared_ptr<Sim> compiled(const std::filesystem::path& directory, const Sim::Options& options, std::string& errors)
    {
        if (options.checks)
            return Sim::compile(directory, options, errors);
        std::string defines;
        for (const auto& [k, v] : options.defines)
            defines += k + "=" + v + ";";
        const auto key = std::make_tuple(std::filesystem::weakly_canonical(directory).generic_string(),
                                         options.entry + ":" + options.module, defines, options.optimisation);
        {
            const std::lock_guard<std::mutex> lock(shared().mutex);
            if (auto it = shared().cache.find(key); it != shared().cache.end())
                return it->second;
        }
        std::shared_ptr<Sim> sim = Sim::compile(directory, options, errors);
        if (sim)
        {
            const std::lock_guard<std::mutex> lock(shared().mutex);
            shared().cache[key] = sim;
        }
        return sim;
    }

    // ---- Signal ---------------------------------------------------------------------------------------------------

    Signal::Signal(std::shared_ptr<const std::vector<float>> samples, double rate, std::size_t from, std::size_t to)
        : samples_(std::move(samples)), rate_(rate), from_(from), to_(to)
    {
        const std::size_t n = samples_ ? samples_->size() : 0;
        to_ = std::min(to_, n);
        from_ = std::min(from_, to_);
    }

    Signal Signal::during(double from, double to) const
    {
        const auto at = [&](double t) {
            return from_ + static_cast<std::size_t>(std::max(0.0, std::llround(t * rate_) * 1.0));
        };
        return Signal(samples_, rate_, std::min(at(from), to_), std::min(at(to), to_));
    }

    std::span<const float> Signal::samples() const noexcept
    {
        if (!samples_)
            return {};
        return std::span<const float>(samples_->data() + from_, to_ - from_);
    }

    Level Signal::floor() const
    {
        return {spectrum().floorDb(20.0, 0.45 * rate_)};
    }

    // ---- Series ---------------------------------------------------------------------------------------------------

    Series::Series(std::vector<double> times, std::vector<double> values) : times_(std::move(times)), values_(std::move(values))
    {
    }

    Series Series::during(double from, double to) const
    {
        std::vector<double> t, v;
        for (std::size_t i = 0; i < times_.size(); ++i)
            if (times_[i] >= from && times_[i] <= to)
            {
                t.push_back(times_[i]);
                v.push_back(values_[i]);
            }
        return Series(std::move(t), std::move(v));
    }

    double Series::max() const noexcept
    {
        return values_.empty() ? std::numeric_limits<double>::quiet_NaN() : *std::max_element(values_.begin(), values_.end());
    }

    double Series::min() const noexcept
    {
        return values_.empty() ? std::numeric_limits<double>::quiet_NaN() : *std::min_element(values_.begin(), values_.end());
    }

    double Series::mean() const noexcept
    {
        if (values_.empty())
            return std::numeric_limits<double>::quiet_NaN();
        double s = 0.0;
        for (const double v : values_)
            s += v;
        return s / static_cast<double>(values_.size());
    }

    double Series::last() const noexcept
    {
        return values_.empty() ? std::numeric_limits<double>::quiet_NaN() : values_.back();
    }

    double Series::first() const noexcept
    {
        return values_.empty() ? std::numeric_limits<double>::quiet_NaN() : values_.front();
    }

    double Series::at(double t) const noexcept
    {
        double v = values_.empty() ? std::numeric_limits<double>::quiet_NaN() : values_.front();
        for (std::size_t i = 0; i < times_.size() && times_[i] <= t; ++i)
            v = values_[i];
        return v;
    }

    double Series::crossing(double threshold, bool rising) const noexcept
    {
        for (std::size_t i = 1; i < values_.size(); ++i)
        {
            const bool crossed = rising ? (values_[i - 1] < threshold && values_[i] >= threshold)
                                        : (values_[i - 1] > threshold && values_[i] <= threshold);
            if (crossed)
                return times_[i];
        }
        return std::numeric_limits<double>::quiet_NaN();
    }

    double Series::settledWithin(double tolerance) const noexcept
    {
        if (values_.empty())
            return std::numeric_limits<double>::quiet_NaN();
        const double end = values_.back();
        std::size_t from = values_.size() - 1;
        while (from > 0 && std::fabs(values_[from - 1] - end) <= tolerance)
            --from;
        return times_[from];
    }

    // ---- Run ------------------------------------------------------------------------------------------------------

    Series Run::tel(std::string_view path) const
    {
        const int c = result_.column(path);
        if (c < 0)
            return {};
        std::vector<double> v;
        v.reserve(result_.rowCount());
        for (std::size_t r = 0; r < result_.rowCount(); ++r)
            v.push_back(result_.at(r, static_cast<std::size_t>(c)));
        return Series(result_.rowTime, std::move(v));
    }

    double Run::firingHz(double from, double to) const
    {
        const double end = result_.simulatedSeconds;
        if (to < 0.0)
            to = end;
        if (from < 0.0)
            from = std::max(0.0, to - 2.0);
        const Series firing = tel("engine.firing");
        if (!firing.empty())
            return firing.during(from, to).mean();
        return tel("engine.speed").during(from, to).mean() / (4.0 * 3.14159265358979323846) * cylinders_;
    }

    double Run::speed() const noexcept
    {
        return result_.wallSeconds > 0.0 ? result_.simulatedSeconds / result_.wallSeconds : 0.0;
    }

    // ---- Scenario -------------------------------------------------------------------------------------------------

    Scenario::Scenario(std::string build)
    {
        timeline_.build = std::move(build);
    }

    Scenario& Scenario::sim(std::filesystem::path directory)
    {
        sim_ = std::move(directory);
        return *this;
    }

    Scenario& Scenario::entry(std::string file)
    {
        options_.entry = std::move(file);
        return *this;
    }

    Scenario& Scenario::checks(bool on)
    {
        options_.checks = on;
        return *this;
    }

    Scenario& Scenario::rate(double hz)
    {
        timeline_.rate = hz;
        return *this;
    }

    Scenario& Scenario::block(int samples)
    {
        timeline_.block = samples;
        return *this;
    }

    Scenario& Scenario::log(bool on)
    {
        log_ = on;
        return *this;
    }

    Scenario& Scenario::at(double seconds)
    {
        time_ = seconds;
        return *this;
    }

    Scenario& Scenario::control(std::string name, double value)
    {
        timeline_.events.push_back({time_, TimelineEvent::Kind::Control, std::move(name), value, {}, 0.0});
        return *this;
    }

    Scenario& Scenario::dyno(double radPerSecond, double sweepRate)
    {
        control("dynoSpeed", radPerSecond);
        return control("dynoRate", sweepRate);
    }

    Scenario& Scenario::press(const char* counter)
    {
        timeline_.events.push_back({time_, TimelineEvent::Kind::Press, counter, 1.0, {}, 0.0});
        return *this;
    }

    Scenario& Scenario::shiftUp() { return press("shiftUp"); }
    Scenario& Scenario::shiftDown() { return press("shiftDown"); }
    Scenario& Scenario::strip() { return press("strip"); }

    Scenario& Scenario::spec(std::string path, double value)
    {
        timeline_.events.push_back({time_, TimelineEvent::Kind::Spec, std::move(path), value, {}, 0.0});
        return *this;
    }

    Scenario& Scenario::spec(std::string path, std::string choice)
    {
        timeline_.events.push_back({time_, TimelineEvent::Kind::Choice, std::move(path), 0.0, std::move(choice), 0.0});
        return *this;
    }

    Scenario& Scenario::ramp(double seconds)
    {
        if (!timeline_.events.empty())
            timeline_.events.back().ramp = seconds;
        return *this;
    }

    Run Scenario::run(double seconds) const
    {
        Run run;
        const std::filesystem::path dir = sim_.empty() ? simDirectory() : sim_;
        std::string errors;
        run.sim_ = compiled(dir, options_, errors);
        if (!run.sim_)
        {
            run.error_ = errors;
            return run;
        }
        SimHost host;
        LoadOptions load;
        load.sim = options_;
        load.sampleRate = timeline_.rate;
        load.maxBlock = std::max(timeline_.block, 16);
        load.worker = false;
        load.log = log_;
        if (!host.attach(run.sim_, load, errors))
        {
            run.error_ = errors;
            return run;
        }
        run.build_ = std::make_shared<Build>(host.fields());
        Build& build = *run.build_;
        if (!timeline_.build.empty())
        {
            std::filesystem::path file = timeline_.build;
            const std::filesystem::path content = contentDirectory();
            if (!std::filesystem::exists(file))
            {
                if (file.extension() == ".json")
                    file = content / file;
                else
                    file = content / "builds" / (timeline_.build + ".json");
            }
            std::vector<std::string> warnings;
            if (!build.loadFile(file, BuildPart::Whole, warnings, errors))
            {
                run.error_ = errors;
                return run;
            }
        }
        if (const int c = build.fieldOf("engine.cylinders"); c >= 0)
            run.cylinders_ = build.values(c).front();
        build.attach(&host);
        Timeline t = timeline_;
        t.sort();
        t.seconds = seconds;
        RunOptions options;
        if (!runTimeline(host, build, t, options, run.result_, errors))
        {
            run.error_ = errors;
            return run;
        }
        run.left_ = std::make_shared<std::vector<float>>(std::move(run.result_.left));
        run.right_ = std::make_shared<std::vector<float>>(std::move(run.result_.right));
        run.mid_ = std::make_shared<std::vector<float>>(run.left_->size());
        for (std::size_t i = 0; i < run.mid_->size(); ++i)
            (*run.mid_)[i] = 0.5f * ((*run.left_)[i] + (*run.right_)[i]);
        if (log_)
            run.log_ = host.takeLog();
        build.attach(nullptr);
        return run;
    }
} // namespace tee::test
