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
#include "Offline.h"
#include "Options.h"
#include "Script.h"

#include "audio/AudioDevice.h"
#include "audio/Recorder.h"
#include "files/Png.h"
#include "input/Controls.h"
#include "input/Keys.h"
#include "input/SdlInput.h"
#include "player/RefAdapter.h"
#include "settings/Settings.h"

#include <funkgui/core/Version.h>
#include <funkgui/desktop/DesktopHost.h>

#if defined(TEE_HAS_UI)
 #include "kit/shaders/Registry.h"       // the interface package's panel shaders (ui/)
#endif

#define SDL_MAIN_HANDLED         // main() below is the program's own: SDL_main.h is taken only for SDL_SetMainReady
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
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

    int run(const app::Options& o)
    {
        SDL_SetMainReady();
        SDL_SetAppMetadata("TheEngineEngine", TEE_VERSION, "com.snipet.theengineengine");

        // ---- the engine: compiled here, before the window (the compile's time is printed) ---------------------------
        app::Engine engine = app::loadEngine(o, 48000.0);
        if (!engine.notes.empty())
            std::fprintf(stderr, "%s", engine.notes.c_str());
        if (!engine.error.empty())
            std::fprintf(stderr, "error: the program did not start\n%s\n", engine.error.c_str());
        else if (engine.ref != nullptr)
            std::printf("engine: %s, compiled in %.2f s\n", engine.name.c_str(), engine.compileSeconds);
        if (!engine.warnings.empty())
            std::fprintf(stderr, "%s\n", engine.warnings.c_str());
        Player& player = *engine.player;

        // ---- what was remembered: the bindings, the pedals' times, the audio buffer (shell/settings/Settings.h) ------
        const settings::Loaded remembered = settings::load();
        if (!remembered.note.empty())
            std::fprintf(stderr, "settings: %s\n", remembered.note.c_str());

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
        if (!o.noAudio)
        {
            audio::Request request;
            request.bufferFrames = o.bufferGiven ? o.buffer : remembered.settings.audioBuffer;
            request.rate = remembered.settings.audioRate;
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

        // ---- the window ---------------------------------------------------------------------------------------------
        const std::unique_ptr<funkgui::Panel> panel = app::makePanel(o, player, controls.services());
        const bool machine = o.frames >= 0 || !o.shot.empty();
        const std::uint32_t frames = static_cast<std::uint32_t>(o.frames >= 0 ? o.frames : 120);

        funkgui::desktop::Options options;
        options.title = "TheEngineEngine";
        options.themeIndex = o.themeGiven ? o.theme : -1;
        options.hidden = o.info;
        options.prefsOrganisation = "TheEngineEngine";
        options.rawEvent = [&](const SDL_Event& e) {
            if (device.handleEvent(e))
                return true;
            return sdlInput.handle(e);
        };
        std::uint64_t last = SDL_GetTicksNS();
        const std::uint64_t began = last;
        options.beforeTick = [&] {
            const std::uint64_t now = SDL_GetTicksNS();
            const float dt = static_cast<float>(static_cast<double>(now - last) * 1e-9);
            last = now;
            const double seconds = static_cast<double>(now - began) * 1e-9;
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
                }
            }
            controls.tick(dt);
            bool changed = device.pump();
            if (changed)
                audioLine = device.isOpen() ? device.info().line() : "the audio device went away: waiting for one";
            if (stopped.empty() && engine.ref != nullptr && engine.ref->stopped())
            {
                stopped = engine.ref->why();
                changed = true;
            }
            if (changed)
                controls.setStatus(statusOf(engine, audioLine, stopped));
        };

        funkgui::desktop::DesktopHost host(*panel, std::move(options));
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
        // the panels' own shaders to the GPU, now that there is one, so that the first frame compiles nothing
        if (std::string refused; !ui::shaders::registerAll(refused))
            std::fprintf(stderr, "shaders: the GPU refused some (they draw as placeholders):\n%s\n", refused.c_str());
       #endif
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
            if (engine.ref != nullptr)
                std::printf("compile: %.2f s\n", engine.compileSeconds);
            std::printf("audio: %s\n", audioLine.c_str());
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
                return script.end >= 0.0 && static_cast<double>(SDL_GetTicksNS() - began) * 1e-9 < script.end;
            };
            while ((host.diagnostics().ticks < frames || scriptRuns()) && host.pump())
                host.nudgeFullRate();
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
        device.close();
        if (recorder.recording())
        {
            const bool whole = recorder.stop();
            std::printf("record: %s: %llu frames%s%s\n", o.record.c_str(), static_cast<unsigned long long>(recorder.written()),
                        recorder.dropped() != 0 ? " (some frames were dropped: the disk kept the writer waiting)" : "",
                        whole ? "" : " (the file is not whole: a write failed)");
        }
        // a first run leaves the settings file behind, with the defaults, for a person to find and edit; a file that
        // is there is a person's and is left as it is (nothing in the window changes a setting yet)
        if (!remembered.found && !machine)
        {
            std::string notSaved;
            if (!settings::save(remembered.settings, notSaved))
                std::fprintf(stderr, "settings: %s\n", notSaved.c_str());
        }
        std::printf("tee-app: %s renderer=%s frames=%u ticks=%u fps=%.1f audio-callbacks=%llu audio-frames=%llu "
                    "largest-request=%d engine-state=%.0f rpm=%.0f\n",
                    code == 0 ? "ok" : "FAILED", d.renderer.c_str(), static_cast<unsigned>(d.frames),
                    static_cast<unsigned>(d.ticks), static_cast<double>(d.fps), static_cast<unsigned long long>(callbacks),
                    static_cast<unsigned long long>(audioFrames), largest, state, rpm);
        return code;
    }
}

int main(int argc, char** argv)
{
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
