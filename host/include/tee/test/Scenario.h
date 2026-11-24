// host/include/tee/test/Scenario.h: the test library every simulator package writes its tests with (card C, section 7),
// so that a test reads as a sentence:
//
//   tee::test::Run r = tee::test::Scenario("builds/default.json")
//                          .at(0.0).ignition(tee::Start).until(1.5).ignition(tee::On)
//                          .at(3.0).throttle(1.0).ramp(0.3)
//                          .run(8.0);
//   CHECK(r.tel("engine.speed").during(6.0, 8.0).max() > 0.95 * r.tel("engine.redline").last());
//   CHECK(r.audio().finite());  CHECK(r.audio().peak() <= 1.0f);
//   CHECK(r.audio().during(6.0, 8.0).line(r.firingHz()).above(r.audio().during(6.0, 8.0).floor(), 12 /*dB*/));
//   CHECK(r.audio().largestStep() < tee::test::clickLimit);
//
// A Scenario is a build (a file, a name under content/builds/, or nothing: the defaults) and a timeline of the driver's
// controls and Spec changes (Timeline.h). at(t) moves the pen to a time; until(t) is the same word read the other way
// ("held until 1.5 s, then"); each control or spec() after it is set at that time; ramp(s) makes the change just
// written a straight line over s seconds from the value before. run(seconds) plays it offline through a SimHost of its
// own (the same every time: no thread of the host's) and keeps the audio and every telemetry field after each block.
//
// The simulator is sim/ of this checkout (its entry engine.fm), or TEE_SIM_DIR, or sim(dir); the content is content/
// of this checkout, or TEE_CONTENT_DIR. Each directory and set of options is compiled once a process and shared.
#pragma once

#include "tee/Build.h"
#include "tee/Measures.h"
#include "tee/SimHost.h"
#include "tee/Timeline.h"

#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace tee
{
    // The key, as Controls::ignition reads it.
    enum Ignition : int
    {
        Off = 0,
        On = 1,
        Start = 2,
    };
} // namespace tee

namespace tee::test
{
    // (declared here so that Run's `friend class Scenario` names this namespace's Scenario, not tee::Scenario, the
    // scenario file's struct, when a translation unit includes both headers: clang-cl refused it on Windows)
    class Scenario;

    // A click: a step between two samples this many times the RMS of the steps around it (Measures.h, largestStep).
    // A smooth sound stays under about 4; the firing pulses of a loud engine may reach 6 to 8.
    inline constexpr double clickLimit = 12.0;

    // A level in dB, for comparing one with another.
    struct Level
    {
        double db = -300.0;
        // At least `margin` dB above `other`.
        bool above(Level other, double margin = 0.0) const noexcept { return db >= other.db + margin; }
        bool below(Level other, double margin = 0.0) const noexcept { return db <= other.db - margin; }
    };

    // A stretch of one channel (or of the two's mean), with the measures of Measures.h.
    class Signal
    {
    public:
        Signal() = default;
        Signal(std::shared_ptr<const std::vector<float>> samples, double rate, std::size_t from, std::size_t to);

        // The part between two times in seconds (clipped to what there is).
        Signal during(double from, double to) const;
        std::span<const float> samples() const noexcept;
        double rate() const noexcept { return rate_; }
        double seconds() const noexcept { return static_cast<double>(to_ - from_) / rate_; }

        bool finite() const noexcept { return measure::finite(samples()); }
        float peak() const noexcept { return static_cast<float>(measure::peak(samples())); }
        double rms() const noexcept { return measure::rms(samples()); }
        Level level() const noexcept { return {measure::dB(rms() * std::sqrt(2.0))}; }   // a sine of amplitude A: A
        Level line(double hz) const { return {measure::lineDb(samples(), rate_, hz)}; }
        // The spectrum's median level between 20 Hz and 90 % of Nyquist: what a line stands above.
        Level floor() const;
        measure::Spectrum spectrum() const { return measure::spectrum(samples(), rate_); }
        double largestStep() const { return measure::largestStep(samples(), rate_); }
        double nullDepth(const Signal& other) const { return measure::nullDepth(samples(), other.samples()); }

    private:
        std::shared_ptr<const std::vector<float>> samples_;
        double rate_ = 48000.0;
        std::size_t from_ = 0, to_ = 0;
    };

    // A telemetry field's values after each block, with the times they were read at.
    class Series
    {
    public:
        Series() = default;
        Series(std::vector<double> times, std::vector<double> values);

        Series during(double from, double to) const;
        bool empty() const noexcept { return values_.empty(); }
        std::size_t size() const noexcept { return values_.size(); }
        const std::vector<double>& times() const noexcept { return times_; }
        const std::vector<double>& values() const noexcept { return values_; }

        double max() const noexcept;
        double min() const noexcept;
        double mean() const noexcept;
        double last() const noexcept;
        double first() const noexcept;
        // The value at time t (the newest read at or before it).
        double at(double t) const noexcept;
        // The first time it crosses `threshold` going up (rising) or down; NaN when it never does.
        double crossing(double threshold, bool rising = true) const noexcept;
        // The time from which it stays within `tolerance` of its last value to the end; NaN when it has no values.
        double settledWithin(double tolerance) const noexcept;

    private:
        std::vector<double> times_, values_;
    };

    class Run
    {
    public:
        Run() = default;

        // A telemetry field ("engine.speed" or "tel.engine.speed"); empty when there is none (a test then fails on it).
        Series tel(std::string_view path) const;
        // The two channels' mean, and each channel.
        Signal audio() const { return Signal(mid_, result_.rate, 0, mid_ ? mid_->size() : 0); }
        Signal left() const { return Signal(left_, result_.rate, 0, left_ ? left_->size() : 0); }
        Signal right() const { return Signal(right_, result_.rate, 0, right_ ? right_->size() : 0); }
        // The firing frequency in Hz, from tel.engine.firing when the program has it (else engine.speed and the
        // build's cylinder count, engine.cylinders, or 8), averaged over [from, to] (the last two seconds by default).
        double firingHz(double from = -1.0, double to = -1.0) const;

        // The host's numbers after the last block (load, the guard's trips, ...), and its words for a trip.
        const HostStats& host() const noexcept { return result_.host; }
        const std::string& stopReason() const noexcept { return result_.stopReason; }
        // How many times real time the run went (simulated seconds over wall seconds).
        double speed() const noexcept;
        const RunResult& result() const noexcept { return result_; }
        // An error the run met (a build that could not be read, an event that names nothing); empty when none.
        const std::string& error() const noexcept { return error_; }
        bool ok() const noexcept { return error_.empty(); }

        // The build as the run ended (a test reads the Spec back), and the session log when it was kept (log()).
        const Build* build() const noexcept { return build_.get(); }
        const std::vector<LogEvent>& log() const noexcept { return log_; }

    private:
        friend class Scenario;
        RunResult result_;
        std::shared_ptr<std::vector<float>> left_, right_, mid_;
        std::string error_;
        double cylinders_ = 8.0;
        std::shared_ptr<Sim> sim_;
        std::shared_ptr<Build> build_;
        std::vector<LogEvent> log_;
    };

    class Scenario
    {
    public:
        // `build`: a build file (a path, or relative to the content directory: "builds/v8.json"), a name under
        // content/builds/ ("v8"), or empty for every field at its default.
        explicit Scenario(std::string build = {});

        Scenario& sim(std::filesystem::path directory);   // another simulator (a test's stand-in)
        Scenario& entry(std::string file);                // its entry file (engine.fm)
        Scenario& checks(bool on = true);                 // the loop budget and the bounds checks
        Scenario& rate(double hz);
        Scenario& block(int samples);
        Scenario& log(bool on = true);                    // keep the session log (Run::log)

        Scenario& at(double seconds);
        Scenario& until(double seconds) { return at(seconds); }

        Scenario& control(std::string name, double value);
        Scenario& ignition(int key) { return control("ignition", key); }
        Scenario& throttle(double v) { return control("throttle", v); }
        Scenario& brake(double v) { return control("brake", v); }
        Scenario& clutch(double v) { return control("clutch", v); }
        Scenario& handbrake(double v) { return control("handbrake", v); }
        Scenario& steer(double v) { return control("steer", v); }
        Scenario& lever(int position) { return control("lever", position); }
        Scenario& buttons(int bits) { return control("buttons", bits); }
        Scenario& mode(int m) { return control("mode", m); }
        Scenario& holdSpeed(double radPerSecond) { return control("holdSpeed", radPerSecond); }
        Scenario& dyno(double radPerSecond, double rate = 0.0);
        Scenario& shiftUp();
        Scenario& shiftDown();
        Scenario& strip();

        Scenario& spec(std::string path, double value);
        Scenario& spec(std::string path, std::string choice);
        // The change written last moves to its value over `seconds`, from the value before it.
        Scenario& ramp(double seconds);

        // Plays it for `seconds`.
        Run run(double seconds) const;

        const Timeline& timeline() const noexcept { return timeline_; }

    private:
        Scenario& press(const char* counter);

        Timeline timeline_;
        std::filesystem::path sim_;
        Sim::Options options_;
        double time_ = 0.0;
        bool log_ = false;
    };

    // The directories a Scenario finds the simulator and the content in (above): for a test runner of another layout.
    void setSimDirectory(std::filesystem::path directory);
    void setContentDirectory(std::filesystem::path directory);
    std::filesystem::path simDirectory();
    std::filesystem::path contentDirectory();
    // The Sim of a directory and options, compiled once a process (a Sim with the checks is compiled for each caller:
    // it belongs to one host). nullptr, with `errors`, when it does not compile.
    std::shared_ptr<Sim> compiled(const std::filesystem::path& directory, const Sim::Options& options, std::string& errors);
} // namespace tee::test
