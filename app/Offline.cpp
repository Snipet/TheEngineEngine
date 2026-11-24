// app/Offline.cpp: Offline.h.
#include "Offline.h"

#include "Engine.h"
#include "Script.h"

#include "audio/Ftz.h"
#include "audio/Wav.h"
#include "files/Png.h"
#include "input/Controls.h"
#include "input/Keys.h"
#include "player/RefAdapter.h"
#include "player/SimAdapter.h"
#include "panel/PlaceholderPanel.h"
#include "tee/app/MainPanel.h"

#if defined(TEE_HAS_UI_FRAME)
 #include "app/AppFrame.h"                  // the interface package's (ui/): the frame, its document over a tee::Build
 #include "kit/bind/Units.h"
 #include "kit/bind/host/BuildDoc.h"
 #include "tee/Build.h"
#endif

#include <fminus/FMinus.h>
#include <funkgui/canvas/SoftRaster.h>
#include <funkgui/core/Version.h>
#include <funkgui/panel/HeadlessHost.h>
#include <funkgui/text/FontService.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#ifndef TEE_VERSION
 #define TEE_VERSION "0.0.0"
#endif

namespace tee::app
{
    namespace
    {
        constexpr double kFrameRate = 60.0;
        constexpr double kRadToRpm = 9.549296585513721;

        bool compare(double x, const std::string& op, double y)
        {
            if (op == ">")
                return x > y;
            if (op == "<")
                return x < y;
            if (op == ">=")
                return x >= y;
            if (op == "<=")
                return x <= y;
            if (op == "==")
                return x == y;
            return x != y;
        }

        // The folder a file goes into, made if it is not there.
        void makeFolderOf(const std::string& file)
        {
            std::error_code ec;
            const std::filesystem::path parent = std::filesystem::path(file).parent_path();
            if (!parent.empty())
                std::filesystem::create_directories(parent, ec);
        }

        struct Level                    // the output since it was last asked
        {
            double peak = 0.0, squares = 0.0;
            long long samples = 0;
            bool finite = true;

            void add(const float* l, const float* r, int n)
            {
                for (int i = 0; i < n; ++i)
                {
                    const double a = l[i], b = r[i];
                    if (!(a == a) || !(b == b) || std::fabs(a) > 1e30 || std::fabs(b) > 1e30)
                    {
                        finite = false;
                        continue;
                    }
                    peak = std::max({ peak, std::fabs(a), std::fabs(b) });
                    squares += a * a + b * b;
                }
                samples += n;
            }

            double rms() const { return samples > 0 ? std::sqrt(squares / (2.0 * static_cast<double>(samples))) : 0.0; }
        };
    }

    namespace
    {
       #if defined(TEE_HAS_UI_FRAME)
        std::string lower(std::string s)
        {
            for (char& c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }
       #endif
    }

    Shown makePanel(const Options& o, Engine& engine, const AppServices& services)
    {
        Shown shown;
        Player& player = *engine.player;
        // a program that did not start has nothing an interface could bind to: the plain Panel says why
        if (o.panel == "plain" || player.count() == 0)
        {
            shown.panel = ui::makePlaceholderPanel(player, services);
            return shown;
        }
       #if defined(TEE_HAS_UI_FRAME)
        if (engine.sim != nullptr)
        {
            // the application's frame over the simulator's document (docs/handoffs/j2-ui-kit.md)
            struct Kept
            {
                explicit Kept(tee::Build& build) : doc(build) {}
                ui::BuildDoc doc;
                ui::Units units;
            };
            const auto kept = std::make_shared<Kept>(engine.sim->build());
            if (!o.unitChoices.empty())
            {
                std::string notes;
                kept->units.load(o.unitChoices, notes);
                if (!notes.empty())
                    std::fprintf(stderr, "settings: units: %s\n", notes.c_str());
            }
            shown.unitChoices = [kept] { return kept->units.save(); };
            const std::filesystem::path content = findContent();
            std::unique_ptr<ui::AppFrame> frame =
                ui::makeAppFrame(kept->doc, kept->units, player,
                                 ui::buildServices(engine.sim->build(), content.empty() ? content : content / "builds",
                                                   Presets::userDirectory(), services));
            // --workspace ID[:PAGE]: the frame opens there (a name that is none is said, and the first one shown)
            if (!o.workspace.empty())
            {
                const std::size_t colon = o.workspace.find(':');
                const std::string id = lower(o.workspace.substr(0, colon));
                const int page = colon == std::string::npos ? 1 : std::atoi(o.workspace.c_str() + colon + 1);
                const int found = frame->find(id);
                if (found < 0)
                {
                    std::string names;
                    for (int s = 0; s < frame->workspaceCount(); ++s)
                        names += (s == 0 ? "" : ", ") + std::string(frame->workspace(s).id());
                    std::fprintf(stderr, "--workspace: no workspace '%s' (there are: %s)\n", o.workspace.c_str(), names.c_str());
                }
                else
                {
                    // the workspace, then the page of its inspector (the frame makes the pages when it shows one)
                    frame->show(found);
                    if (frame->inspectorPageCount() > 0)
                        frame->showInspectorPage(std::clamp(page, 1, frame->inspectorPageCount()) - 1);
                }
            }
            shown.panel = std::move(frame);
            shown.keep = kept;
            return shown;
        }
       #endif
        // the reference's own panel reads the reference's dials and its raw fields: for the reference alone
        if (engine.ref != nullptr)
            shown.panel = ui::makeMainPanel(player, services);
        else
            shown.panel = ui::makePlaceholderPanel(player, services);
        return shown;
    }

    void printVersions()
    {
        std::printf("TheEngineEngine %s\n", TEE_VERSION);
        std::printf("FMinus %s\n", FMINUS_VERSION_STRING);
        std::printf("FunkGui %s\n", funkgui::version());
    }

    int runOffline(const Options& o)
    {
        // ---- the script and the length ------------------------------------------------------------------------------
        Script script;
        std::string why;
        if (!o.script.empty() && !loadScript(o.script, script, why))
        {
            std::fprintf(stderr, "error: %s\n", why.c_str());
            return 2;
        }
        double seconds = o.seconds;
        if (seconds < 0.0 && o.frames >= 0)
            seconds = static_cast<double>(o.frames) / kFrameRate;
        if (seconds < 0.0)
            seconds = script.end >= 0.0 ? script.end : 10.0;
        if (o.script.empty())
            script = defaultScript(seconds);
        const int frames = o.frames >= 0 && o.seconds < 0.0 ? o.frames : static_cast<int>(std::ceil(seconds * kFrameRate));

        // ---- the engine ---------------------------------------------------------------------------------------------
        Engine engine = loadEngine(o, static_cast<double>(o.rate));
        if (!engine.notes.empty())
            std::fprintf(stderr, "%s", engine.notes.c_str());
        if (!engine.error.empty())
        {
            std::fprintf(stderr, "error: the program did not start\n%s\n", engine.error.c_str());
            if (!o.keepGoing)
                return 2;
        }
        if (!engine.warnings.empty())
            std::fprintf(stderr, "%s\n", engine.warnings.c_str());
        Player& player = *engine.player;
        ParamSource& params = player;
        TelemetrySource& telemetry = player;
        player.prepare(static_cast<double>(o.rate), o.block);

        input::Controls controls(player, player);
        controls.setStatus(engine.error.empty() ? engine.name + "   no audio device (rendering offline)"
                                                : "error: the program did not start\n" + engine.error);
        const Shown shown = makePanel(o, engine, controls.services());
        funkgui::HeadlessHost host(*shown.panel, o.theme, static_cast<float>(o.dpi));

        audio::WavWriter wav;
        if (!o.wav.empty())
        {
            makeFolderOf(o.wav);
            if (!wav.open(o.wav, o.rate, why))
            {
                std::fprintf(stderr, "error: %s\n", why.c_str());
                return 1;
            }
        }

        const int hSpeed = telemetry.findValue(telemetry::kSpeed);
        const int hState = telemetry.findValue(telemetry::kState);
        const int hGear = telemetry.findValue(telemetry::kGear);
        const int hThrottle = telemetry.findValue(telemetry::kThrottle);
        const auto rpm = [&] { return hSpeed >= 0 ? telemetry.value(hSpeed) * kRadToRpm : 0.0; };

        std::vector<float> left(static_cast<std::size_t>(o.block)), right(static_cast<std::size_t>(o.block));
        Level whole, sincePeak, sinceRms;
        int failures = 0, shots = 0, checks = 0;
        bool wroteAll = true;
        const float dt = static_cast<float>(1.0 / kFrameRate);

        const auto shoot = [&](const std::string& file) {
            const funkgui::PrimList& list = host.draw();
            const funkgui::Image image = funkgui::rasterise(list, funkgui::FontService::get().atlas(), 2);
            makeFolderOf(file);
            std::string reason;
            if (!files::writePng(file, image.rgba.data(), image.w, image.h, reason))
            {
                std::fprintf(stderr, "error: %s\n", reason.c_str());
                wroteAll = false;
                return;
            }
            ++shots;
            if (list.missingGlyphs != 0)
                std::fprintf(stderr, "note: %s: %u characters the font does not have\n", file.c_str(), list.missingGlyphs);
        };

        // ---- the frames ---------------------------------------------------------------------------------------------
        const audio::ScopedFtz ftz;
        std::size_t next = 0;
        long long rendered = 0;
        bool ended = false;
        int frame = 0;
        for (; frame < frames && !ended; ++frame)
        {
            const double now = static_cast<double>(frame) / kFrameRate;
            std::vector<const ScriptEvent*> after;      // what waits for this frame's audio and tick
            while (next < script.events.size() && script.events[next].time <= now + 1e-9)
            {
                const ScriptEvent& e = script.events[next++];
                switch (e.kind)
                {
                    case ScriptEvent::Kind::key:
                    {
                        const int key = input::keyFromName(e.name);
                        if (key == 0)
                            std::fprintf(stderr, "note: script line %d: no key '%s'\n", e.line, e.name.c_str());
                        else
                            controls.key(key, e.a != 0.0);
                        break;
                    }
                    case ScriptEvent::Kind::set:
                    {
                        const int i = params.findParam(e.name);
                        if (i < 0)
                            std::fprintf(stderr, "note: script line %d: no parameter '%s'\n", e.line, e.name.c_str());
                        else
                            params.set(i, e.a);
                        break;
                    }
                    case ScriptEvent::Kind::pointer:
                    {
                        const auto x = static_cast<float>(e.a), y = static_cast<float>(e.b);
                        if (e.name == "move")
                            host.moveTo(x, y);
                        else if (e.name == "down")
                            host.press(x, y);
                        else if (e.name == "up")
                        {
                            host.moveTo(x, y);
                            host.release();
                        }
                        else
                            host.click(x, y);
                        break;
                    }
                    case ScriptEvent::Kind::wheel:
                        host.wheel(static_cast<float>(e.a), static_cast<float>(e.b), static_cast<float>(e.c));
                        break;
                    case ScriptEvent::Kind::end:
                        ended = true;
                        break;
                    default:
                        after.push_back(&e);
                        break;
                }
            }
            if (ended)
                break;

            engine.frame();
            controls.tick(dt);

            // this frame's share of the audio, in blocks
            const long long until = std::llround(static_cast<double>(frame + 1) * static_cast<double>(o.rate) / kFrameRate);
            while (rendered < until)
            {
                const int n = static_cast<int>(std::min<long long>(until - rendered, o.block));
                player.process(left.data(), right.data(), n);
                whole.add(left.data(), right.data(), n);
                sincePeak.add(left.data(), right.data(), n);
                sinceRms.add(left.data(), right.data(), n);
                if (!o.wav.empty())
                    wroteAll = wav.write(left.data(), right.data(), n) && wroteAll;
                rendered += n;
            }

            engine.frame();             // the telemetry the Panel and the checks read is this frame's
            host.tick(1, dt);

            for (const ScriptEvent* e : after)
            {
                if (e->kind == ScriptEvent::Kind::shot)
                    shoot(e->name);
                else if (e->kind == ScriptEvent::Kind::print)
                    std::printf("t=%.2f state=%.0f rpm=%.0f gear=%.0f throttle=%.2f peak=%.4f\n", now,
                                hState >= 0 ? telemetry.value(hState) : 0.0, rpm(), hGear >= 0 ? telemetry.value(hGear) : 0.0,
                                hThrottle >= 0 ? telemetry.value(hThrottle) : 0.0, static_cast<double>(telemetry.outputPeak()));
                else if (e->kind == ScriptEvent::Kind::expect)
                {
                    double x = 0.0;
                    bool known = true;
                    if (e->name == "peak")
                    {
                        x = sincePeak.peak;
                        sincePeak = {};
                    }
                    else if (e->name == "rms")
                    {
                        x = sinceRms.rms();
                        sinceRms = {};
                    }
                    else if (e->name == "rpm")
                        x = rpm();
                    else if (e->name.rfind("param ", 0) == 0)
                    {
                        const int i = params.findParam(e->name.substr(6));
                        known = i >= 0;
                        x = known ? params.get(i) : 0.0;
                    }
                    else
                    {
                        const int h = telemetry.findValue(e->name);
                        known = h >= 0;
                        x = known ? telemetry.value(h) : 0.0;
                    }
                    ++checks;
                    if (!known || !compare(x, e->op, e->a))
                    {
                        ++failures;
                        std::fprintf(stderr, "FAILED: script line %d at %.2f s: %s %s %g, and it is %s%g\n", e->line, now,
                                     e->name.c_str(), e->op.c_str(), e->a, known ? "" : "unknown: ", x);
                    }
                }
            }
        }

        if (!o.shot.empty())
            shoot(o.shot);
        if (!o.wav.empty())
            wroteAll = wav.close() && wroteAll;
        if (const std::string stopped = engine.stopped(); !stopped.empty())
        {
            std::fprintf(stderr, "error: the program was stopped: %s\n", stopped.c_str());
            ++failures;
        }
        if (!whole.finite)
        {
            std::fprintf(stderr, "FAILED: the output held a sample that is not finite\n");
            ++failures;
        }

        std::printf("tee-offline: %s frames=%d seconds=%.3f rate=%d block=%d peak=%.5f rms=%.5f checks=%d failed=%d shots=%d"
                    " compile=%.2fs%s%s\n",
                    failures == 0 && wroteAll ? "ok" : "FAILED", frame, static_cast<double>(rendered) / o.rate, o.rate, o.block,
                    whole.peak, whole.rms(), checks, failures, shots, engine.compileSeconds, o.wav.empty() ? "" : " wav=",
                    o.wav.c_str());
        return failures == 0 && wroteAll ? 0 : 1;
    }
}
