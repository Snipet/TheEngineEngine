// assistant/include/tee/assistant/Tools.h: what Claude may do to the app, as a table of tools, and the limits around it
// (docs/plan/w3-a-assistant.md, "The tools, first version"; docs/plan/w3-a-detailed.md for each tool's host call,
// thread, cancel and longest seconds). Both ways in (Claude Desktop over MCP, the in-app chat) call the same table.
//
//   tee::assistant::Inbox inbox;                                     // drained by the frame
//   tee::assistant::Host host{inbox, document, library, ...};        // the app's pieces (below)
//   tee::assistant::Tools tools(host);
//   tee::assistant::Drivers drivers;
//   tee::assistant::Turn turn(tools, drivers, tee::assistant::Driver::Desktop, clock);   // a worker's
//   tee::assistant::Result r = turn.call("set_fields", args, &cancel);
//
// What a tool may never do, each tested (tests/assistant): remove a build; take or return a path (builds by name only);
// change the quality, the audio device, the recorder or the settings (no tool exists for them); return a picture;
// read or write a hidden field, a section's `rev`, or anything under ctl, tel or scope (tel is read by get_status only,
// its fixed list). A value outside its range is refused and the stored value returned, never clamped. A batch of
// changes is one undo step, made only while no gesture is open. Loading over unsaved changes asks the owner first.
// Save-as refuses a name that is already a build's.
//
// What a tool returns is content, not instruction: Schema-known values, the build's name, the library's build names;
// never its notes, a file, or an error passed through from below.
//
// Threads: Tools::call and Turn::call on a worker (never the frame's thread: they wait for the frame). The pieces in
// Host are touched only inside the inbox's jobs, on the frame's thread, except `sim` (a shared program, read-only).
#pragma once

#include "tee/assistant/Inbox.h"
#include "tee/assistant/Json.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace tee
{
    class SimHost;
    class Sim;
    namespace files
    {
        class BuildFile;
        class BuildLibrary;
    }
}

namespace tee::assistant
{
    // The limits, each in one place.
    struct Limits
    {
        double jobSeconds = 2.0;          // a job's wait for the frame (real time)
        double gestureSeconds = 5.0;      // a batch's wait for an open gesture to end (real time: Host::now)
        double askSeconds = 40.0;         // the owner's answer to a question (real time)
        double predictSeconds = 20.0;     // a predicted curve (real time)
        double dynoSeconds = 40.0;        // a dyno run (real time)
        // (Every tool ends inside the MCP client's request timeout, 60 s by default in the MCP SDK (protocol.ts,
        // DEFAULT_REQUEST_TIMEOUT_MSEC), whatever it waits on: the bridge answers by wire::kCallSeconds, 55 s, and a
        // tool's longest path, two jobs and its own limit, stays under that: ToolsTest checks it.)
        int turnCalls = 24;               // tool calls in one turn
        double turnSeconds = 300.0;       // a turn (real time: the Turn's clock)
        std::size_t resultBytes = 32 * 1024;      // one tool's result
        std::size_t turnBytes = 256 * 1024;       // a turn's results in all
        std::size_t batchChanges = 64;    // changes in one set_fields
    };

    // The system's steady clock, seconds: what every limit of the tools runs on in the app.
    double realSeconds();

    // The app's pieces the tools work on. Everything but `sim` is the frame's thread's, reached only in a job.
    struct Host
    {
        Inbox& inbox;
        files::BuildFile& document;
        files::BuildLibrary& library;
        std::function<SimHost*()> simHost;                 // the playing host, null while the engine is not ready
        std::function<std::shared_ptr<Sim>()> sim;         // the compiled program for the private copies, or null
        // Real time, seconds, for the tools' own limits (never the frame's clock, which stands still while the window
        // is minimised: X-064). Empty: the system's steady clock (realSeconds). A test may hold it and step it.
        std::function<double()> now;
        // Puts a question to the owner in the app (called in a job, on the frame's thread) and returns the answer to
        // come: true yes. `withdrawn` is raised (release) when the asker stops waiting, for any reason (its time is
        // up, the call is cancelled, the session stops): the app then takes the question down (AppFrame::ask).
        // Empty function: nobody can be asked, and what needs asking is refused.
        std::function<std::future<bool>(const std::string& question, std::shared_ptr<const std::atomic<bool>> withdrawn)> ask;
        Limits limits{};
        // The app's own heavy work a person waits for is running: the ribbon's predicted curve
        // (PredictService::busyFlag) or the calibration bake (CalibrationBakeService::state() Baking or Checking).
        // Called from the worker, so it reads only what any thread may. The assistant's curves yield to it, as the bake
        // yields to the ribbon: they do not start while it is true, and they wait between their points while it is.
        // Nothing waits on the assistant's runs, so no order of them deadlocks. Empty: nothing to yield to.
        std::function<bool()> othersBusy;
    };

    // A tool's answer, as MCP and the chat both carry it: the JSON text of the value, and whether it is an error.
    struct Result
    {
        bool isError = false;
        nlohmann::json value;        // the structured answer ({"error": "..."} for an error)
        std::string text() const { return dumpText(value); }
    };

    struct ToolInfo
    {
        std::string name;
        std::string title;
        std::string description;
        nlohmann::json inputSchema;
        bool readOnly = true;        // MCP's readOnlyHint: true only when it changes nothing at all (chat-audit DOCS-2)
    };

    class Tools
    {
    public:
        explicit Tools(Host host);

        const std::vector<ToolInfo>& list() const noexcept { return infos_; }
        const ToolInfo* find(const std::string& name) const noexcept;
        const Limits& limits() const noexcept { return host_.limits; }
        // Real time, seconds: Host::now, or the steady clock.
        double now() const { return host_.now ? host_.now() : realSeconds(); }

        // Runs one tool on this (a worker's) thread. `cancel` (may be null) stops it as soon as it can.
        Result call(const std::string& name, const nlohmann::json& arguments = nlohmann::json::object(),
                    const std::atomic<bool>* cancel = nullptr);

        // Points the assistant's curves have run, and whether one runs now (any thread): what a test watches.
        std::uint64_t pointsRun() const noexcept { return points_.load(); }
        bool curveRunning() const noexcept { return running_.load(); }

    private:
        Result status(const nlohmann::json& a, const std::atomic<bool>* cancel);
        Result describe(const nlohmann::json& a, const std::atomic<bool>* cancel);
        Result getBuild(const nlohmann::json& a, const std::atomic<bool>* cancel);
        Result listBuilds(const nlohmann::json& a, const std::atomic<bool>* cancel);
        Result setFields(const nlohmann::json& a, const std::atomic<bool>* cancel);
        Result loadBuild(const nlohmann::json& a, const std::atomic<bool>* cancel);
        Result saveAs(const nlohmann::json& a, const std::atomic<bool>* cancel);
        Result predict(const nlohmann::json& a, const std::atomic<bool>* cancel);
        Result dyno(const nlohmann::json& a, const std::atomic<bool>* cancel);

        Result job(Inbox::Job j, const std::atomic<bool>* cancel, double seconds = -1.0);

        // Waits while othersBusy() (polling), until `stop` or `cancel` or `deadline`: false when it gave up.
        bool yieldToOthers(const std::atomic<bool>* stop, const std::atomic<bool>* cancel,
                           std::chrono::steady_clock::time_point deadline);

        Host host_;
        std::vector<ToolInfo> infos_;
        std::atomic<std::uint64_t> points_{0};
        std::atomic<bool> running_{false};
    };

    // Who drives: one at a time (TestingPlayground's chat-audit TURN-3: a chat turn and a Claude Desktop call side by
    // side on one document).
    enum class Driver
    {
        Chat,
        Desktop,
    };
    const char* driverName(Driver d) noexcept;

    class Drivers
    {
    public:
        // Takes the wheel for `who`; false, with `why`, while the other holds it. The same driver may take it again
        // (a second Claude Desktop call while one runs is the link's to order, not refused here).
        bool take(Driver who, std::string& why);
        void release(Driver who);
        bool held() const;

    private:
        mutable std::mutex mutex_;
        int count_ = 0;
        Driver holder_ = Driver::Chat;
    };

    // One turn of a driver: the wheel taken for its life, and its limits counted. A Claude Desktop call is a turn of
    // one call; a chat turn is the model's rounds until it stops.
    class Turn
    {
    public:
        // `clock`: real time, seconds, for turnSeconds (the app's session gives the steady clock; empty: Tools::now).
        Turn(Tools& tools, Drivers& drivers, Driver who, std::function<double()> clock);
        ~Turn();
        Turn(const Turn&) = delete;
        Turn& operator=(const Turn&) = delete;

        // The wheel was taken (else every call is refused with why()).
        bool active() const noexcept { return active_; }
        const std::string& why() const noexcept { return why_; }

        // A tool call within the turn's limits: refused, as an error result, when a limit is spent.
        Result call(const std::string& name, const nlohmann::json& arguments = nlohmann::json::object(),
                    const std::atomic<bool>* cancel = nullptr);

        int calls() const noexcept { return calls_; }
        std::size_t bytes() const noexcept { return bytes_; }

    private:
        Tools& tools_;
        Drivers& drivers_;
        Driver who_;
        std::function<double()> clock_;
        bool active_ = false;
        std::string why_;
        double started_ = 0.0;
        int calls_ = 0;
        std::size_t bytes_ = 0;
    };

    // Whether `name` may be a build's name here: what BuildLibrary::validName takes, and no separator of any system,
    // no "..", no leading dot, no drive, no control character, at most 64 bytes. The tools take nothing else.
    bool safeBuildName(const std::string& name, std::string& why);
    // Whether `name` may be asked for as a listed build (load_build): 1 to 200 bytes, "/" only between folders, no "..",
    // no leading dot or tilde, no backslash, drive or control character. Only a name the library's wholeBuilds() lists
    // is then handed to the library, in the listing's spelling.
    bool listedBuildName(const std::string& name, std::string& why);
}
