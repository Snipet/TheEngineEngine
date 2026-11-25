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
#include <chrono>
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
        commandsContext.coldStart = [&now] { return now.coldStart; };
        commandsContext.setColdStart = [&now](bool cold) { now.coldStart = cold; };
        commandsContext.audioChanged = [&] {
            audioLine = device.isOpen() ? device.info().line() : "no audio device";
            showStatus();
        };
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
        options.rawEvent = [&](const SDL_Event& e) {
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
        std::uint32_t readyTicks = 0, ticksSeen = 0;
        bool scriptStarted = !machine || engine.ready();
        options.beforeTick = [&] {
            const std::uint64_t now = SDL_GetTicksNS();
            const float dt = static_cast<float>(static_cast<double>(now - last) * 1e-9);
            last = now;
            ++ticksSeen;
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
            shown.window->coldStart = [&now] { return now.coldStart; };
            shown.window->setColdStart = [&now](bool cold) { now.coldStart = cold; };
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
