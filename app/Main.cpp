// app/Main.cpp: TheEngineEngine, the application in a window. It finds sim/ beside the executable, compiles it, opens
// the audio device, opens the window (FunkGui's DesktopHost: SDL3's window, bgfx under it) and runs: the keyboard, a
// gamepad and the Panel's own controls into one ControlsFrame a frame, the simulator on the audio thread, the Panel
// drawing what it says. A program that does not compile is shown in the window, in the compiler's words, and the
// application stays up.
//
//   TheEngineEngine                         run
//   TheEngineEngine --info                  the versions, the renderer, the audio device; exit
//   TheEngineEngine --frames N --shot f.png N frames (and a --script to its end), the last picture read back from the GPU
//   TheEngineEngine --seconds S --wav f.wav S seconds rendered with no window and no device (app/Offline.h); exit 0
//   TheEngineEngine --help                  the rest
//
// It is a console program on Windows for now: what it prints (the compile's time, the audio device, an error) is
// what a person testing it is asked for (docs/checkpoints/M1.md).
#include "AssistantWake.h"
#include "Engine.h"
#include "InputProbe.h"
#include "Launch.h"
#include "Offline.h"
#include "Options.h"
#include "Script.h"
#include "WindowCommands.h"

#include "audio/AudioDevice.h"
#include "audio/Recorder.h"
#include "files/BuildFiles.h"
#include "files/Png.h"
#include "input/Controls.h"
#include "input/Keys.h"
#include "input/SdlInput.h"
#include "input/g29/G29Input.h"
#include "input/g29/RacingWheel.h"
#include "player/RefAdapter.h"
#include "player/SimAdapter.h"
#include "settings/Settings.h"
#include "tee/Build.h"
#include "tee/Telemetry.h"
#if defined(TEE_HAS_UI_FRAME)
 #include "tee/assistant/Session.h"   // card A: the assistant the frame holds (Shown::assistant)
#endif

#include <funkgui/core/Version.h>
#include <funkgui/desktop/DesktopHost.h>

#if defined(TEE_HAS_UI)
 #include "app/Look.h"                   // look::shiftLightBefore: the DRIVE view's shift lights, the wheel's too
 #include "kit/shaders/Registry.h"       // the interface package's panel shaders (ui/)
 // U5's atmosphere shaders (shaders/fx: glow, flame, smoke, shimmer), where the build made their table
 #if __has_include("tee/ui/shaders/fx.h") && __has_include("views/fx/Fx.h")
  #include "tee/ui/shaders/fx.h"
  #include "views/fx/Fx.h"
  #define TEE_HAS_FX 1
 #endif
#endif

#define SDL_MAIN_HANDLED         // main() below is the program's own: SDL_main.h is taken only for SDL_SetMainReady
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace
{
    using namespace tee;

    // The status line a Panel shows: what plays and the audio device; or the error.
    std::string statusOf(const app::Engine& engine, const std::string& audio, const std::string& stopped)
    {
        if (!engine.error.empty())
            return "error: the program did not start\n" + engine.error;
        if (!stopped.empty())
            return "error: the program was stopped\n" + stopped;
        return engine.name + "   " + audio;
    }

    int run(const app::Options& given)
    {
        app::Options o = given;
        SDL_SetMainReady();
        SDL_SetAppMetadata("TheEngineEngine", TEE_VERSION, "com.snipet.theengineengine");
        // --probe-input: the game controllers SDL sees and their changes (app/InputProbe.h); no window, no engine
        if (o.probeInput >= 0.0)
            return app::probeInput(o.probeInput);

        // ---- what was remembered: the bindings, the pedals' times, the audio buffer (shell/settings/Settings.h) ------
        const settings::Loaded remembered = settings::load();
        if (!remembered.note.empty())
            std::fprintf(stderr, "settings: %s\n", remembered.note.c_str());
        // a cold start: --cold, or the settings' (a machine's run as a person's: the settings it was given)
        if (remembered.settings.coldStart)
            o.cold = true;

        // ---- the engine: its program read here (the front end, a tenth of a second: the document and the parameters),
        // its machine code made on a worker while the window opens (the simulator's; the reference's is made here) ----
        o.background = true;
        const auto launched = std::chrono::steady_clock::now();
        app::Engine engine = app::loadEngine(o, 48000.0);
        if (!engine.notes.empty())
            std::fprintf(stderr, "%s", engine.notes.c_str());
        if (!engine.error.empty())
            std::fprintf(stderr, "error: the program did not start\n%s\n", engine.error.c_str());
        else if (engine.ref != nullptr)
            std::printf("engine: %s, compiled in %.2f s\n", engine.name.c_str(), engine.compileSeconds);
        else if (engine.sim != nullptr)
            std::printf("engine: %s, being compiled while the window opens\n", engine.name.c_str());
        if (!engine.warnings.empty())
            std::fprintf(stderr, "%s\n", engine.warnings.c_str());
        Player& player = *engine.player;

        o.unitChoices = remembered.settings.unitChoices;
        // the engine's quality: --quality, else the settings' (made once the engine is ready: never changed by itself)
        const std::string quality = !o.quality.empty() ? o.quality : remembered.settings.audioQuality;
        if (engine.sim != nullptr)
            engine.sim->requestQuality(quality == "economy" ? 1 : 0);

        input::Controls controls(player, player);
        controls.bindings() = remembered.settings.bindings;
        controls.driver().setSettings(remembered.settings.driver);
        input::SdlInput sdlInput(controls);

        // ---- the audio device ---------------------------------------------------------------------------------------
        // (what the device plays is the player, heard on the way by the recorder: --record)
        audio::Recorder recorder;
        audio::RecordingSource heard(player, recorder);
        audio::Device device(heard);
        std::string audioLine = "no audio device (--no-audio)";
        audio::Request request;
        if (!o.noAudio)
        {
            request.bufferFrames = o.bufferGiven ? o.buffer : remembered.settings.audioBuffer;
            request.rate = remembered.settings.audioRate;
            request.deviceName = remembered.settings.audioDevice;
            std::string why;
            if (device.open(request, why))
            {
                audioLine = device.info().line();
                if (device.info().silent && !why.empty())
                    std::fprintf(stderr, "audio: nothing will be heard: %s\n", why.c_str());
            }
            else
            {
                audioLine = "no audio device: " + why;
                std::fprintf(stderr, "audio: %s\n", audioLine.c_str());
            }
            std::printf("audio: %s\n", audioLine.c_str());
            if (device.isOpen())
                std::printf("audio: SDL reports the device's buffer, not the system's whole output latency: %.1f ms is the "
                            "least it can be\n", device.info().bufferMs);
        }
        if (!o.record.empty())
        {
            std::string why;
            std::error_code ec;
            const std::filesystem::path parent = std::filesystem::path(o.record).parent_path();
            if (!parent.empty())
                std::filesystem::create_directories(parent, ec);
            if (!device.isOpen())
                std::fprintf(stderr, "record: there is no audio device, so nothing plays to record\n");
            else if (!recorder.start(o.record, device.info().rate, why))
                std::fprintf(stderr, "record: %s\n", why.c_str());
            else
                std::printf("record: writing %s at %d Hz\n", o.record.c_str(), device.info().rate);
        }
        std::string stopped;
        controls.setStatus(statusOf(engine, audioLine, stopped));

        // ---- the script, when one was given: its keys and its parameters, by the clock ------------------------------
        app::Script script;
        if (!o.script.empty())
        {
            std::string why;
            if (!app::loadScript(o.script, script, why))
            {
                std::fprintf(stderr, "error: %s\n", why.c_str());
                return 2;
            }
        }
        std::size_t nextEvent = 0;
        std::vector<const app::ScriptEvent*> scriptChecks;
        int scriptFailures = 0, scriptCheckCount = 0;
        // A check of the script's, in the window: a telemetry value by name, `rpm`, `ready`, `param ID` (the document),
        // `sent PATH` (the host's Spec as it was sent); a print: a line of the engine's state.
        const auto checkScript = [&](const app::ScriptEvent& e, double at) {
            TelemetrySource& values = player;
            const auto telemetry = [&values](const char* name) {
                const int h = values.findValue(name);
                return h >= 0 ? values.value(h) : 0.0;
            };
            if (e.kind == app::ScriptEvent::Kind::print)
            {
                std::printf("t=%.2f state=%.0f rpm=%.0f stage=%.0f inPhase=%.2f oil=%.1fK\n", at, telemetry("engine.state"),
                            telemetry("engine.speed") * 60.0 / (2.0 * 3.14159265358979), telemetry("valve.stage"),
                            telemetry("valve.inPhase"), telemetry("engine.oilTemperature"));
                return;
            }
            double x = 0.0;
            bool known = true;
            if (e.name == "rpm")
                x = telemetry("engine.speed") * 60.0 / (2.0 * 3.14159265358979);
            else if (e.name == "ready")
                x = engine.ready() ? 1.0 : 0.0;
            else if (e.name.rfind("sent ", 0) == 0)
                known = engine.sim != nullptr && app::readSent(*engine.sim, e.name.substr(5), x);
            else if (e.name.rfind("param ", 0) == 0)
            {
                ParamSource& params = player;
                const int i = params.findParam(e.name.substr(6));
                known = i >= 0;
                x = known ? params.get(i) : 0.0;
            }
            else
            {
                const int h = values.findValue(e.name);
                known = h >= 0;
                x = known ? values.value(h) : 0.0;
            }
            ++scriptCheckCount;
            if (!known || !app::compareScript(x, e.op, e.a))
            {
                ++scriptFailures;
                std::fprintf(stderr, "FAILED: script line %d at %.2f s: %s %s %g, and it is %s%g\n", e.line, at,
                             e.name.c_str(), e.op.c_str(), e.a, known ? "" : "unknown: ", x);
            }
        };

        // ---- the window ---------------------------------------------------------------------------------------------
        const app::Shown shown = app::makePanel(o, engine, controls, /*window*/ true);
        if (!shown.problem.empty())
        {
            std::fprintf(stderr, "tee-app: %s\n", shown.problem.c_str());
            return 2;
        }

        const bool machine = o.frames >= 0 || !o.shot.empty();

        // ---- Claude Desktop's link (card A): only when the owner turned it on (the settings' allowClaudeDesktop; the
        // frame's switch is U1's to draw), never in a machine's run ----------------------------------------------------
#if defined(TEE_HAS_UI_FRAME)
        if (shown.assistant != nullptr && remembered.settings.allowClaudeDesktop && !machine)
        {
            std::string why;
            if (shown.assistant->allowDesktop(true, why))
                std::printf("Claude Desktop: allowed (the switch is on in the settings)\n");
            else
                std::fprintf(stderr, "Claude Desktop: %s\n", why.c_str());
        }
#endif

        // ---- the File and Audio menus and their keys (app/WindowCommands.h) ---------------------------------------
        settings::Settings now = remembered.settings;
        std::string note;               // the newest thing a command said, after the status line
        // the simulator's health as the host reports it (card C's HostStats): restarting after a fault; clamped (a block
        // with a sample past full scale, held to it) for a moment after it happens
        std::string health;
        std::string wheelLine;           // the racing wheel's status ("wheel: switching to its full mode"), when it says one
        bool wheelChanged = true;
        std::string claudeSaid;          // the status line's Claude Desktop part, as last said
        std::uint32_t clippedSeen = 0, faultsSeen = 0;
        double clampedLeft = 0.0, faultLeft = 0.0, loadHigh = 0.0, suggestLeft = 0.0;
        bool suggested = false;
        std::string compiling;          // "compiling the engine: 3 s" while the machine code is being made
        bool saidReady = false;
        const auto showStatus = [&] {
            std::string line = statusOf(engine, audioLine, stopped);
            if (stopped.empty() && !compiling.empty())
                line += "   " + compiling;
            if (stopped.empty() && !health.empty())
                line += "   " + health;
            if (stopped.empty() && !note.empty())
                line += "   " + note;
            if (!wheelLine.empty())
                line += "   " + wheelLine;
#if defined(TEE_HAS_UI_FRAME)
            if (shown.assistant != nullptr)
                if (const std::string claude = shown.assistant->statusNote(); !claude.empty())
                    line += "   " + claude;
#endif
            controls.setStatus(line);
        };
        app::WindowCommands::Context commandsContext;
        commandsContext.document = shown.document;
        commandsContext.library = shown.library;
        commandsContext.device = o.noAudio ? nullptr : &device;
        commandsContext.request = request;
        commandsContext.recorder = &recorder;
        commandsContext.settings = &now;
        commandsContext.recordFolder = (Presets::userDirectory().parent_path() / "recordings").string();
        commandsContext.say = [&](const std::string& line) {
            note = line;
            showStatus();
        };
        if (engine.sim != nullptr)
        {
            commandsContext.quality = [&engine] { return engine.sim->quality(); };
            commandsContext.setQuality = [&engine, &now](int q) {
                engine.sim->requestQuality(q);
                now.audioQuality = q == 1 ? "economy" : "full";
            };
        }
        // the clutch: the Driver's setting, at once, and the settings file's when the window closes
        const auto setClutchMode = [&controls, &now](int m) {
            input::DriverSettings s = controls.driver().settings();
            input::setClutchMode(s, static_cast<input::ClutchMode>(std::clamp(m, 0, 2)));
            controls.driver().setSettings(s);
            now.driver.autoClutch = s.autoClutch;
            now.driver.shifterLever = s.shifterLever;
        };
        const auto clutchMode = [&controls] { return static_cast<int>(input::clutchModeOf(controls.driver().settings())); };
        commandsContext.clutchMode = clutchMode;
        commandsContext.setClutchMode = setClutchMode;
        commandsContext.autoClutchSays = [&controls] { return controls.clutchLine(); };
        commandsContext.coldStart = [&now] { return now.coldStart; };
        commandsContext.setColdStart = [&now](bool cold) { now.coldStart = cold; };
        commandsContext.audioChanged = [&] {
            audioLine = device.isOpen() ? device.info().line() : "no audio device";
            showStatus();
        };
        // ---- the racing wheel: a Logitech G29 (input/g29/RacingWheel.h; macOS only so far) ------------------------
        // Its five shift lights count as the DRIVE view's do (ui/views/drive/Dash.cpp: light k lit from the redline less
        // look::shiftLightBefore(redline, k)), all five flashing about eight times a second on the limiter; none while
        // there are no revs to count (-1: the wheel's are turned off).
        input::g29::RacingWheel::Hooks wheelHooks;
        wheelHooks.changed = [&wheelChanged] { wheelChanged = true; };
        wheelHooks.lights = [&player, handles = std::array<int, 3>{ -2, -2, -2 }]() mutable {
#if defined(TEE_HAS_UI)
            TelemetrySource& values = player;
            if (handles[0] == -2)
                handles = { values.findValue(telemetry::kSpeed), values.findValue(telemetry::kRedline),
                            values.findValue(telemetry::kFlags) };
            if (handles[0] < 0 || handles[1] < 0)
                return -1;
            constexpr double kToRpm = 30.0 / 3.14159265358979323846;
            const double rpm = values.value(handles[0]) * kToRpm, redline = values.value(handles[1]) * kToRpm;
            if (!std::isfinite(rpm) || !std::isfinite(redline) || redline <= 0.0)
                return 0;
            const double flags = handles[2] >= 0 ? values.value(handles[2]) : 0.0;
            if (std::isfinite(flags) && (std::lround(flags) & 1) != 0)
                return (SDL_GetTicks() / 62) % 2 == 0 ? input::g29::kAllLights : 0;   // the limiter: flashing
            int lit = 0;
            for (int k = 0; k < input::g29::kLights; ++k)
                if (rpm >= redline - ui::look::shiftLightBefore(redline, k))
                    lit = k + 1;
            return input::g29::litMask(lit);
#else
            (void)handles;
            return -1;
#endif
        };
        input::g29::RacingWheel wheel(std::move(wheelHooks), now.wheelOn);
        input::g29::Input wheelInput;
        input::g29::ButtonLearn wheelLearn;
        bool wheelEditSaid = false;          // "editing from the wheel waits for the frame", said once
        {
            std::string notes;
            wheelInput.setButtons(input::g29::loadButtons(now.wheelButtons, notes));
            if (!notes.empty())
                std::fprintf(stderr, "settings: %s (left out)\n", notes.c_str());
        }
        // a bindable target's line in the menu: "drive: shiftup: right paddle", "edit: undo: square"
        const auto wheelButtonLine = [&wheelInput](int i) {
            const input::g29::Target& t = input::g29::bindables()[static_cast<std::size_t>(i)];
            std::string held;
            for (const input::g29::Binding& b : wheelInput.buttons())
                if (b.edit == t.edit && (t.edit || b.action == t.action))
                    held += (held.empty() ? "" : ", ") + std::string(input::g29::kBits[b.bit].name);
            return input::g29::targetLabel(t) + ": " + (held.empty() ? "no button" : held);
        };
        // (the loop's `now` is its clock: the settings are kept through this)
        const auto learnedButtons = [&now](std::string text) { now.wheelButtons = std::move(text); };
        commandsContext.wheelButtonCount = [] { return static_cast<int>(input::g29::bindables().size()); };
        commandsContext.wheelButton = wheelButtonLine;
        commandsContext.learnWheelButton = [&](int i) {
            if (i < 0)
            {
                wheelLearn.cancel();
                wheelInput.setButtons(input::g29::defaultButtons());
                now.wheelButtons.clear();
                note = "wheel: the buttons' defaults (the paddles shift, PS starts, the rest edit)";
                showStatus();
                return;
            }
            const input::g29::Target& t = input::g29::bindables()[static_cast<std::size_t>(i)];
            wheelLearn.begin(t, wheel.state());
            note = "wheel: press the button for " + input::g29::targetLabel(t) + " (10 s)";
            showStatus();
        };
        commandsContext.wheel = [&wheel] {
            const std::string s = wheel.status();
            return input::g29::RacingWheel::supported()
                       ? std::string(wheel.on() ? "on" : "off") + (s.empty() ? std::string() : ": " + s)
                       : std::string();
        };
        commandsContext.setWheelOn = [&wheel, &now](bool on) {
            wheel.setOn(on);
            now.wheelOn = on;
        };
        commandsContext.wheelOn = [&wheel] { return wheel.on(); };
        app::WindowCommands commands(std::move(commandsContext));
        std::filesystem::path tryFirstFile;     // the first build a launch opened (its card is wanted while it is open)
        if (shown.document != nullptr)
        {
            shown.document->recent().load(remembered.settings.recentBuilds);
            shown.document->recent().prune();
            // The build the window opens with (app/Launch.h): the one last open, else the first build
            // (content/builds/first.json), opened as a person opens a file: clean, its name in the top bar, its values
            // sent to the engine as a loaded build's are. None over a --preset, and none with --reference.
            const app::LaunchBuild launch = app::chooseLaunchBuild(o, machine, remembered.settings.lastBuild);
            if (!launch.file.empty())
            {
                std::vector<std::string> warnings;
                std::string error;
                if (shown.document->open(launch.file, BuildPart::Whole, warnings, error))
                {
                    std::printf("file: %s, %s (%s)\n", launch.what, launch.file.string().c_str(),
                                shown.document->dirty() ? "with unsaved changes" : "clean");
                    // a first launch's hand-hold: the first build's notes on the frame's TRY FIRST card (U1's), until
                    // it is closed once (the settings remember that); the console gets their first line
                    if (launch.first)
                    {
                        const std::string& notes = shown.document->build().notes;
                        std::printf("note: %s\n", notes.substr(0, notes.find('\n')).c_str());
                        tryFirstFile = launch.file;
                    }
                }
                else
                    std::fprintf(stderr, "file: %s (%s) did not open: %s\n", launch.what, launch.file.string().c_str(),
                                 error.c_str());
            }
            else if (launch.what[0] != '\0')
                std::printf("file: %s\n", launch.what);
        }
        funkgui::Panel* const panel = shown.panel.get();
        const std::uint32_t frames = static_cast<std::uint32_t>(o.frames >= 0 ? o.frames : 120);

        funkgui::desktop::Options options;
        options.title = "TheEngineEngine";
        options.themeIndex = o.themeGiven ? o.theme : -1;
        options.hidden = o.info;
        // the assistant's wake (app/AssistantWake.h): made once SDL runs (the host's), served first of every event
        std::unique_ptr<app::AssistantWake> assistantWake;
        // --minimise-after N (scripts/run-wake.sh): after N frames the window is minimised (hidden where that is not
        // honoured a second on), and twice a second, on the main thread, what SDL says of it and the frames ticked so
        // far are printed, ticking or not; 20 s on, the window is closed. A window that does not tick, for the wake.
        funkgui::desktop::DesktopHost* hostSeen = nullptr;
        std::uint32_t ticksSeen = 0;
        struct Minimise
        {
            Uint32 type = 0;
            SDL_TimerID timer = 0;
            std::uint64_t since = 0;    // SDL_GetTicks when asked
            bool hidden = false;
        } minimise;
        const auto minimiseReport = [&] {
            SDL_Window* w = hostSeen != nullptr ? hostSeen->window() : nullptr;
            const SDL_WindowFlags f = w != nullptr ? SDL_GetWindowFlags(w) : 0;
            const double t = static_cast<double>(SDL_GetTicks() - minimise.since) / 1000.0;
            const char* sim = engine.sim == nullptr ? "none" : engine.sim->ready() ? "ready" : "compiling";
            std::printf("minimise: %.1f s minimised=%d hidden=%d occluded=%d simulator=%s ticks=%u\n", t,
                        (f & SDL_WINDOW_MINIMIZED) != 0, (f & SDL_WINDOW_HIDDEN) != 0, (f & SDL_WINDOW_OCCLUDED) != 0,
                        sim, ticksSeen);
            std::fflush(stdout);
            if (w != nullptr && t >= 1.0 && !minimise.hidden && (f & SDL_WINDOW_MINIMIZED) == 0)
            {
                minimise.hidden = true;
                std::printf("minimise: not honoured here: the window hidden instead\n");
                SDL_HideWindow(w);
            }
            if (t >= 20.0)
            {
                SDL_RemoveTimer(minimise.timer);
                minimise.timer = 0;
                SDL_Event quit{};
                quit.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quit);
            }
        };
        options.rawEvent = [&](const SDL_Event& e) {
            if (minimise.type != 0 && e.type == minimise.type)
            {
                minimiseReport();
                return true;
            }
            if (assistantWake && assistantWake->take(e))
            {
                // a tool posted: its job drained here, the window ticking or not; jobs left waiting: again a little later.
                // The drain runs the engine's frame around the jobs (Shown::drainAssistant, X-066). Of beforeTick, this
                // still does NOT run while the window does not tick, deliberately: the keys', the gamepad's and the wheel's
                // input (nobody drives a window they cannot see; the wheel's pump is the frame's), the bake's tick and the
                // frames clock (shown.frame: the tools' clock stands still, so a job waiting on it waits for its
                // condition, not its limit), the status line and the script's checks.
                if (shown.drainAssistant)
                    shown.drainAssistant();
                if (shown.assistant != nullptr && shown.assistant->waiting() > 0)
                    assistantWake->again();
                return true;
            }
            if (commands.event(e))
                return true;
            if (device.handleEvent(e))
                return true;
            return sdlInput.handle(e);
        };
        std::uint64_t last = SDL_GetTicksNS();
        // A machine's script (--script, --frames) runs on the engine's clock: it starts when the engine is ready to play,
        // so its keys find an engine whatever the compile took; the frames it asks for are counted from then too.
        std::uint64_t began = last;
        std::uint32_t readyTicks = 0;
        bool scriptStarted = !machine || engine.ready();
        options.beforeTick = [&] {
            const std::uint64_t now = SDL_GetTicksNS();
            const float dt = static_cast<float>(static_cast<double>(now - last) * 1e-9);
            last = now;
            ++ticksSeen;
            if (o.minimiseAfter > 0 && ticksSeen == static_cast<std::uint32_t>(o.minimiseAfter) && hostSeen != nullptr
                && minimise.type == 0)
            {
                minimise.type = SDL_RegisterEvents(1);
                minimise.since = SDL_GetTicks();
                std::printf("minimise: asked after %u frames\n", ticksSeen);
                SDL_MinimizeWindow(hostSeen->window());
                minimise.timer = SDL_AddTimer(500, [](void* type, SDL_TimerID, Uint32 interval) -> Uint32 {
                    SDL_Event e{};
                    e.type = static_cast<Uint32>(reinterpret_cast<std::uintptr_t>(type));
                    SDL_PushEvent(&e);
                    return interval;
                }, reinterpret_cast<void*>(static_cast<std::uintptr_t>(minimise.type)));
            }
            if (ticksSeen == 1)
                std::printf("window: the first frame %.2f s after launch\n",
                            std::chrono::duration<double>(std::chrono::steady_clock::now() - launched).count());
            if (!scriptStarted && engine.ready())
            {
                scriptStarted = true;
                began = now;
                readyTicks = ticksSeen;
            }
            const double seconds = scriptStarted ? static_cast<double>(now - began) * 1e-9 : -1.0;
            while (nextEvent < script.events.size() && script.events[nextEvent].time <= seconds)
            {
                const app::ScriptEvent& e = script.events[nextEvent++];
                if (e.kind == app::ScriptEvent::Kind::key)
                {
                    if (const int key = input::keyFromName(e.name); key != 0)
                        controls.key(key, e.a != 0.0);
                }
                else if (e.kind == app::ScriptEvent::Kind::set)
                {
                    ParamSource& params = player;
                    if (const int i = params.findParam(e.name); i >= 0)
                        params.set(i, e.a);
                    else
                        std::fprintf(stderr, "note: script line %d: no parameter '%s'\n", e.line, e.name.c_str());
                }
                else if (e.kind == app::ScriptEvent::Kind::open)
                    app::openInScript(shown.document, e);
                else if (e.kind == app::ScriptEvent::Kind::quality)
                {
                    if (engine.sim != nullptr)
                        engine.sim->requestQuality(static_cast<int>(e.a));
                }
                else if (e.kind == app::ScriptEvent::Kind::expect || e.kind == app::ScriptEvent::Kind::print)
                    scriptChecks.push_back(&e);         // read after this frame's engine.frame(), below
            }
            // the racing wheel: its sources serviced, its state into the driver (before the frame reads the driver)
            wheel.pump();
            if (wheelLearn.active())
            {
                input::g29::Buttons buttons = wheelInput.buttons();
                const input::g29::Target learning = wheelLearn.target();
                if (wheelLearn.watch(wheel.state(), dt, buttons))
                {
                    wheelInput.setButtons(buttons);
                    learnedButtons(input::g29::saveButtons(buttons));
                    const auto& all = input::g29::bindables();
                    const auto at = std::find_if(all.begin(), all.end(), [&](const input::g29::Target& t) {
                        return t.edit == learning.edit && t.action == learning.action;
                    });
                    note = "wheel: " + wheelButtonLine(static_cast<int>(at - all.begin()));
                    showStatus();
                }
                else if (!wheelLearn.active())
                {
                    note = "wheel: no button pressed (nothing changed)";
                    showStatus();
                }
                else if (wheelLearn.takeShareRefused())
                {
                    note = "wheel: SHARE is the fine modifier (held, a value's edit is fine): press another button";
                    showStatus();
                }
            }
            const input::g29::Presses presses = wheel.takePresses();
            wheelInput.setClutchAxis(wheel.clutchAxis());
            wheelInput.apply(wheel.state(), wheel.reading(), controls.driver(), !wheelLearn.active(), dt, &presses);
            // its edit commands to the frame (the inspector's hands), on this thread, the frame's; none to take them
            // yet (the frame's edit() not served): said once, dropped
            if (!wheelInput.edits().empty())
            {
                if (shown.wheelEdit)
                    for (const input::g29::Edit& e : wheelInput.edits())
                        shown.wheelEdit(e.command, e.fine);
                else if (!wheelEditSaid)
                {
                    wheelEditSaid = true;
                    std::printf("wheel: editing from the wheel waits for the frame (its buttons are read; nothing "
                                "takes their edits yet)\n");
                }
            }
            if (wheelChanged)
            {
                wheelChanged = false;
                const std::string said = wheel.status();
                const std::string line = !wheel.present() ? std::string()
                                       : !said.empty()     ? "wheel: " + said
                                       : wheel.reading()   ? std::string("wheel: G29 read")
                                                           : std::string();
                if (line != wheelLine)
                {
                    wheelLine = line;
                    std::printf("%s\n", line.empty() ? "wheel: none read" : line.c_str());
                    showStatus();
                }
            }
            engine.frame();
            if (shown.frame)
                shown.frame();
            // the script's checks and prints (as tee-headless's: app/Script.h), of this frame's telemetry and document
            for (const app::ScriptEvent* e : scriptChecks)
                checkScript(*e, seconds);
            scriptChecks.clear();
            // the engine's compile: START does nothing and the status line counts the seconds until it is ready
            bool compileChanged = false;
            if (engine.sim != nullptr)
            {
                controls.driver().setStartEnabled(engine.ready());
                const std::string line = engine.ready() || !engine.sim->compileError().empty()
                                             ? std::string()
                                             : app::compilingLine(engine.sim->compilingSeconds());
                if (line != compiling)
                {
                    compiling = line;
                    compileChanged = true;
                }
                if (engine.ready() && !saidReady)
                {
                    saidReady = true;
                    std::printf("engine: %s, compiled in %.2f s in the background, ready %.2f s after launch, %zu kB of "
                                "machine code\n", engine.name.c_str(), engine.compileSeconds,
                                std::chrono::duration<double>(std::chrono::steady_clock::now() - launched).count(),
                                engine.sim->codeBytes() / 1024);
                }
            }
            controls.tick(dt);
            bool changed = device.pump() || compileChanged;
#if defined(TEE_HAS_UI_FRAME)
            // Claude Desktop's line (card A: "connected, 3 changes"), said again whenever it changes
            if (shown.assistant != nullptr)
                if (std::string claude = shown.assistant->statusNote(); claude != claudeSaid)
                {
                    claudeSaid = std::move(claude);
                    changed = true;
                }
#endif
            if (changed)
                audioLine = device.isOpen() ? device.info().line() : "the audio device went away: waiting for one";
            if (stopped.empty())
            {
                stopped = engine.stopped();
                changed = changed || !stopped.empty();
            }
            if (engine.sim != nullptr)
            {
                const HostStats stats = engine.sim->hostStats();
                if (stats.clippedBlocks != clippedSeen)
                {
                    clippedSeen = stats.clippedBlocks;
                    clampedLeft = 2.0;
                }
                if (stats.engineFaults != faultsSeen)
                {
                    faultsSeen = stats.engineFaults;
                    if (faultLeft <= 0.0)       // (said once an episode: again after five quiet seconds)
                        std::fprintf(stderr, "engine: it diverged and was restarted (%u times since load)\n", faultsSeen);
                    faultLeft = 5.0;
                }
                clampedLeft = std::max(0.0, clampedLeft - static_cast<double>(dt));
                faultLeft = std::max(0.0, faultLeft - static_cast<double>(dt));
                // a load over 80 % for five seconds, at Full: ECONOMY suggested, once (never switched by itself)
                if (engine.sim->quality() == 0 && stats.loadMean > 0.8)
                    loadHigh += static_cast<double>(dt);
                else
                    loadHigh = 0.0;
                if (loadHigh >= 5.0 && !suggested)
                {
                    suggested = true;
                    suggestLeft = 10.0;
                }
                suggestLeft = std::max(0.0, suggestLeft - static_cast<double>(dt));
                const std::string qualityError = engine.sim->qualityError();
                const std::string said = faultLeft > 0.0      ? std::string("the engine diverged and was restarted")
                                        : engine.sim->qualitySwitching()
                                            ? std::string(engine.sim->quality() == 1 ? "switching to economy: compiling"
                                                                                     : "switching to full: compiling")
                                        : !qualityError.empty() ? "quality not changed: " + qualityError
                                        : suggestLeft > 0.0 ? std::string("the load is high: QUALITY economy (settings, or "
                                                                          "F9) would about halve it")
                                        : stats.restarting != 0 ? std::string("restarting (after a fault)")
                                        : clampedLeft > 0.0  ? std::string("clamped (the output went past full scale)")
                                                             : std::string();
                if (said != health)
                {
                    health = said;
                    changed = true;
                }
            }
            if (changed)
                showStatus();
        };

        funkgui::desktop::DesktopHost host(*panel, std::move(options));
        commands.attach(&host);
        hostSeen = &host;
        if (shown.assistant != nullptr && shown.drainAssistant)
        {
            assistantWake = std::make_unique<app::AssistantWake>();
            if (assistantWake->usable())
                shown.assistant->setWake(assistantWake->wake());
        }
        if (shown.window)
        {
            shown.window->saveText = [&commands](const std::string& name, const std::string& text, std::string& error) {
                return commands.saveText(name, text, error);
            };
            shown.window->audioInfo = [&audioLine] { return audioLine; };
            shown.window->audioMenu = [&commands] { commands.audioMenu(); };
            shown.window->qualityChosen = [&now](int q) { now.audioQuality = q == 1 ? "economy" : "full"; };
            shown.window->tryFirst = [&now, &tryFirstFile, &shown] {
                if (now.tryFirstSeen || tryFirstFile.empty() || shown.document == nullptr
                    || shown.document->file() != tryFirstFile)
                    return std::string();
                return shown.document->build().notes;
            };
            shown.window->tryFirstDone = [&now] { now.tryFirstSeen = true; };
            shown.window->wheelOn = [&wheel] { return wheel.on(); };
            shown.window->setWheelOn = [&wheel, &now](bool on) {
                wheel.setOn(on);
                now.wheelOn = on;
            };
            shown.window->wheelStatus = [&wheel] {
                const std::string said = wheel.status();
                return !said.empty() ? said : wheel.reading() ? std::string("G29 read") : std::string();
            };
            shown.window->coldStart = [&now] { return now.coldStart; };
            shown.window->setColdStart = [&now](bool cold) { now.coldStart = cold; };
            shown.window->clutchMode = clutchMode;
            shown.window->setClutchMode = setClutchMode;
            shown.window->autoClutchSays = [&controls] { return controls.clutchLine(); };
            shown.window->desktopChosen = [&now](bool on) { now.allowClaudeDesktop = on; };
        }
        std::string why;
        if (!host.open(why))
        {
            std::fprintf(stderr, "error: the window did not open: %s\n", why.c_str());
            if (o.info)
            {
                app::printVersions();
                std::printf("renderer: none (%s)\n", why.c_str());
            }
            return o.info ? 0 : 1;
        }
       #if defined(TEE_HAS_UI)
        // the panels' own shaders to the GPU, now that there is one, so that the first frame compiles nothing; with them
        // the atmosphere's (an effect the GPU refuses draws nothing at all: no placeholder, no error, the launch goes on)
       #if defined(TEE_HAS_FX)
        ui::shaders::add(ui::shaders::fxShaders());
       #endif
        if (std::string refused; !ui::shaders::registerAll(refused))
            std::fprintf(stderr, "shaders: the GPU refused some (a panel's draw as placeholders; an effect draws "
                                 "nothing):\n%s\n", refused.c_str());
       #endif
        // The effects present, a word each: "glow flame smoke shimmer", or what is missing and why.
        const auto effectsLine = [] {
           #if defined(TEE_HAS_FX)
            std::string on, off;
            for (const ui::fx::Effect e : { ui::fx::Effect::glow, ui::fx::Effect::flame, ui::fx::Effect::smoke,
                                            ui::fx::Effect::shimmer })
            {
                std::string name = ui::fx::fileOf(e);
                name = name.substr(0, name.find('.'));
                (ui::fx::available(e) ? on : off) += (name + " ");
            }
            std::string line = on.empty() ? std::string("none") : on.substr(0, on.size() - 1);
            if (!off.empty())
                line += " (not drawn: " + off.substr(0, off.size() - 1) + ")";
            return line;
           #else
            return std::string("none (this build has no atmosphere shaders)");
           #endif
        };
        std::string padWhy;
        if (!sdlInput.openGamepads(padWhy))
            std::fprintf(stderr, "gamepads: none will be read: %s\n", padWhy.c_str());

        if (o.info)
        {
            for (int i = 0; i < 3 && host.pump(); ++i)      // a frame or two, so the sizes are the drawn ones
                host.nudgeFullRate();
            const funkgui::desktop::DesktopHost::Diagnostics d = host.diagnostics();
            app::printVersions();
            std::printf("SDL %d.%d.%d\n", SDL_VERSIONNUM_MAJOR(SDL_GetVersion()), SDL_VERSIONNUM_MINOR(SDL_GetVersion()),
                        SDL_VERSIONNUM_MICRO(SDL_GetVersion()));
            std::printf("renderer: %s (video driver %s)\n", d.renderer.c_str(), d.videoDriver.c_str());
            std::printf("window: %d x %d logical px, %d x %d pixels, scale %.3f (display scale %.3f)\n", d.logicalW,
                        d.logicalH, d.physW, d.physH, d.scale, d.displayScale);
            std::printf("engine: %s%s\n", engine.name.c_str(), engine.error.empty() ? "" : " (it did not start)");
            if (engine.sim != nullptr)
            {
                // the compile waited for, so its time and size can be said
                const bool done = engine.sim->waitReady(120.0);
                engine.frame();
                if (done)
                    std::printf("compile: %.2f s (in the background; %.2f s from launch to ready), machine code %zu kB\n",
                                engine.compileSeconds,
                                std::chrono::duration<double>(std::chrono::steady_clock::now() - launched).count(),
                                engine.sim->codeBytes() / 1024);
                else
                    std::printf("compile: not finished (%s)\n", engine.sim->compileError().c_str());
            }
            else if (engine.ref != nullptr)
                std::printf("compile: %.2f s\n", engine.compileSeconds);
            std::printf("audio: %s\n", audioLine.c_str());
            std::printf("effects: %s\n", effectsLine().c_str());
            if (engine.sim != nullptr)
            {
                engine.sim->waitQuality(120.0);
                std::printf("quality: %s (the engine at %.0f Hz inside)%s\n", engine.sim->quality() == 1 ? "economy" : "full",
                            engine.sim->innerRate(),
                            engine.sim->qualityError().empty() ? "" : (" not made: " + engine.sim->qualityError()).c_str());
            }
            const std::string pad = sdlInput.gamepadName();
            std::printf("gamepad: %s\n", pad.empty() ? "none" : pad.c_str());
            std::printf("settings: %s (%s)\n", settings::file().string().c_str(),
                        remembered.found ? "read" : "not there: the defaults");
            host.close();
            return 0;
        }

        int code = 0;
        if (machine)
        {
            // Full rate, so a machine does not wait for the idle cadence; real time, so the audio keeps up. At least
            // the frames asked for, and until the script's end when there is one: a machine that draws slowly (a
            // software GPU) and one that draws fast both get the script's whole story into the last picture.
            const auto scriptRuns = [&] {
                return !scriptStarted
                       || (script.end >= 0.0 && static_cast<double>(SDL_GetTicksNS() - began) * 1e-9 < script.end);
            };
            const auto waiting = [&] {
                // the engine's compile, for at most two minutes (one that fails or hangs ends the run all the same)
                const bool stuck = std::chrono::steady_clock::now() - launched > std::chrono::minutes(2);
                return engine.sim != nullptr && !engine.ready() && engine.sim->compileError().empty() && !stuck;
            };
            while ((waiting() || ticksSeen - readyTicks < frames || scriptRuns()) && host.pump())
            {
                if (!scriptStarted && !waiting() && !engine.ready())
                {
                    // a compile that failed (or took past two minutes): the script runs on what there is, from now
                    scriptStarted = true;
                    began = SDL_GetTicksNS();
                    readyTicks = ticksSeen;
                }
                host.nudgeFullRate();
            }
            if (!o.shot.empty() && host.isOpen())
            {
                std::vector<std::uint8_t> rgba;
                int w = 0, h = 0;
                std::error_code ec;
                const std::filesystem::path parent = std::filesystem::path(o.shot).parent_path();
                if (!parent.empty())
                    std::filesystem::create_directories(parent, ec);
                if (!host.capture(rgba, w, h, why) || !files::writePng(o.shot, rgba.data(), w, h, why))
                {
                    std::fprintf(stderr, "error: no picture: %s\n", why.c_str());
                    code = 1;
                }
                else
                    std::printf("picture: %s (%d x %d, read back from the GPU)\n", o.shot.c_str(), w, h);
            }
        }
        else
            code = host.run();
        if (minimise.timer != 0)
            SDL_RemoveTimer(minimise.timer);
        if (shown.assistant != nullptr)
            shown.assistant->setWake({});   // the window's loop is over: a post now has nothing to wake
        if (scriptCheckCount > 0)
            std::printf("checks: %d, %d failed\n", scriptCheckCount, scriptFailures);
        if (scriptFailures > 0)
            code = 1;

        const funkgui::desktop::DesktopHost::Diagnostics d = host.diagnostics();
        // what the engine said last, for a machine's log (a person sees it in the window)
        const TelemetrySource& telemetry = player;
        const int hState = telemetry.findValue(telemetry::kState), hSpeed = telemetry.findValue(telemetry::kSpeed);
        const double state = hState >= 0 ? telemetry.value(hState) : -1.0;
        const double rpm = hSpeed >= 0 ? telemetry.value(hSpeed) * 9.549296585513721 : 0.0;
        const std::uint64_t audioFrames = device.frames();
        const std::uint64_t callbacks = device.callbacks();
        const int largest = device.largestRequest();
        host.close();
        if (shown.window)
            *shown.window = {};             // they hold the commands and this function's locals, which go first
        device.close();
        if (recorder.recording())
        {
            const bool whole = recorder.stop();
            std::printf("record: %s: %llu frames%s%s\n", o.record.c_str(), static_cast<unsigned long long>(recorder.written()),
                        recorder.dropped() != 0 ? " (some frames were dropped: the disk kept the writer waiting)" : "",
                        whole ? "" : " (the file is not whole: a write failed)");
        }
        // A build with unsaved changes when the window closed is kept: written to the user's folder as "Unsaved
        // changes", which opens at the next start (and is listed with the builds). Nothing is asked: the window is gone.
        if (!machine && shown.document != nullptr && shown.document->dirty() && shown.library != nullptr)
        {
            std::string notKept;
            const std::string name = shown.document->build().name;
            if (shown.library->save(*shown.document, "Unsaved changes", notKept))
            {
                std::printf("file: the build's unsaved changes are kept in %s\n", shown.document->file().string().c_str());
                now.lastBuild = shown.document->file().string();
            }
            else
                std::fprintf(stderr, "file: the build's unsaved changes were lost: %s\n", notKept.c_str());
            shown.document->build().name = name;
        }
        else if (shown.document != nullptr && !shown.document->file().empty())
            now.lastBuild = shown.document->file().string();
        if (shown.document != nullptr)
            now.recentBuilds = shown.document->recent().save();

        // A first run leaves the settings file behind, with the defaults, for a person to find and edit. After that
        // the file is written only when something the window can change did: the units chosen in the frame, the audio
        // device, the build open and the recent files.
        if (shown.unitChoices)
            now.unitChoices = shown.unitChoices();
        if (!machine && (!remembered.found || settings::toJson(now) != settings::toJson(remembered.settings)))
        {
            std::string notSaved;
            if (!settings::save(now, notSaved))
                std::fprintf(stderr, "settings: %s\n", notSaved.c_str());
        }
        // what the window ended with: the build's name (the top bar's), its file, and each model switch as the document
        // holds it and as the engine was sent it (a test's read-back: tests/shell/CMakeLists.txt, shell.window.first)
        if (machine && shown.document != nullptr)
        {
            std::printf("%s\n", app::launchReadBack(*shown.document, engine.sim).c_str());
            const bool card = !now.tryFirstSeen && !tryFirstFile.empty() && shown.document->file() == tryFirstFile;
            std::printf("try-first: %s\n", card ? "the card is wanted" : "no card");
        }
        std::printf("tee-app: %s renderer=%s frames=%u ticks=%u fps=%.1f audio-callbacks=%llu audio-frames=%llu "
                    "largest-request=%d engine-state=%.0f rpm=%.0f faults=%u\n",
                    code == 0 ? "ok" : "FAILED", d.renderer.c_str(), static_cast<unsigned>(d.frames),
                    static_cast<unsigned>(d.ticks), static_cast<double>(d.fps), static_cast<unsigned long long>(callbacks),
                    static_cast<unsigned long long>(audioFrames), largest, state, rpm,
                    engine.sim != nullptr ? engine.sim->hostStats().engineFaults : 0u);
        return code;
    }
}

int main(int argc, char** argv)
{
#if !defined(_WIN32)
    std::signal(SIGPIPE, SIG_IGN);   // a peer gone (Claude Desktop's bridge) is an error, never the app's end
#endif
    tee::app::Options options;
    std::string why;
    if (!tee::app::parseOptions(argc, argv, options, why))
    {
        std::fprintf(stderr, "TheEngineEngine: %s\n", why.c_str());
        return 2;
    }
    if (options.help)
    {
        std::printf("%s", tee::app::usage("TheEngineEngine").c_str());
        return 0;
    }
    if (options.version)
    {
        tee::app::printVersions();
        return 0;
    }
    if (options.seconds >= 0.0)
        return tee::app::runOffline(options);       // no window, no device
    return run(options);
}
