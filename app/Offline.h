// app/Offline.h: the application with no window and no audio device: the same Player, Controls and Panel, run on a
// simulated clock of 60 frames a second. Each frame it does what the script says is due, advances the controls,
// renders that frame's share of the audio (to a WAV file when asked), ticks the Panel, and takes the pictures that are
// due with FunkGui's software rasteriser. Nothing in it depends on how fast the machine is, so the same script gives
// the same audio and the same pictures every time. tee-headless is this; the window's --seconds is too.
#pragma once

#include "Options.h"

#include "tee/app/Sources.h"

#include <funkgui/panel/Panel.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tee::input
{
    class Controls;
}

namespace tee::files
{
    class BuildFile;
    class BuildLibrary;
}

namespace tee::assistant
{
    class Session;
}

namespace tee::app
{
    // Runs it. 0: done, and every check of the script held. 1: a check failed, or a file could not be written. 2: the
    // options or the script could not be read, or the program did not compile (without --keep-going).
    int runOffline(const Options&);

    // The status line while the engine's machine code is being made: "compiling the engine: 3 s".
    std::string compilingLine(double seconds);

    // The versions, a line each: the application, FMinus and FunkGui.
    void printVersions();

    // The Panel for this run, and what it needs kept alive beside it.
    //   the simulator (--engine sim), in a build with the interface's frame: tee::ui::AppFrame over the engine's
    //     tee::Build (a BuildDoc, the user's Units, the garage's builds), its Scope and its dyno (U1's
    //     SimScope and ServiceDyno, ui/app/host/HostSources.h), opened on --workspace and at --size;
    //   the reference: the interface's tee::ui::makeMainPanel (the reference's own panel);
    //   --panel plain, a program that did not start, or a build with no interface package: the shell's plain Panel.
    struct Shown
    {
        std::shared_ptr<void> keep;                 // the frame's document and units; destroyed after the Panel
        std::unique_ptr<funkgui::Panel> panel;
        // The units as the frame has them now, as the settings file keeps them; empty function: no frame.
        std::function<std::string()> unitChoices;
        // Once a frame, before the Panel ticks (the Scope's copy made stale); empty: nothing to do.
        std::function<void()> frame;
        // Waits up to `seconds` for the dyno's first predicted curve (a picture that should show it); false: none
        // came (`error`: the dyno's own words), or there is no dyno. Empty function: no frame.
        std::function<bool(double, std::string& error)> waitForDyno;
        // The frame's workspaces by id, and one shown (--workspace all: each pictured in turn). Empty: no frame.
        std::function<std::vector<std::string>()> workspaces;
        std::function<void(int)> showWorkspace;
        // Not empty: what the options asked for could not be shown (a --workspace that is none, or with no frame);
        // the run stops with it.
        std::string problem;
        // The simulator's document on disk and the library of builds (null: not the simulator's frame). Owned by keep.
        // What only the window serves (a save dialog, the audio device), filled once the window exists. The frame's
        // FrameServices call these; tee-headless gives the frame none of them (a workspace then copies its text, the
        // settings page shows the status line).
        struct WindowHooks
        {
            std::function<bool(const std::string&, const std::string&, std::string&)> saveText;
            std::function<std::string()> audioInfo;
            std::function<void()> audioMenu;
            std::function<void(int)> qualityChosen;     // the QUALITY row's choice, for the settings file
            std::function<std::string()> tryFirst;      // the first build's notes while its TRY FIRST card is wanted
            std::function<void()> tryFirstDone;         // the card was closed: not again
            std::function<bool()> coldStart;            // the settings' coldStart (ENGINE START: from the next launch)
            std::function<void(bool)> setColdStart;
            std::function<void(bool)> desktopChosen;     // the CLAUDE DESKTOP switch took: the settings' allowClaudeDesktop
        };
        std::shared_ptr<WindowHooks> window;
        files::BuildFile* document = nullptr;
        files::BuildLibrary* library = nullptr;
        // The assistant over that document (assistant/include/tee/assistant/Session.h): its frame drained by `frame`
        // above; Claude Desktop's link off until the window turns it on (the settings' allowClaudeDesktop). Owned by
        // keep; null: not the simulator's frame.
        assistant::Session* assistant = nullptr;

        Shown() = default;
        Shown(Shown&&) = default;
        Shown& operator=(Shown&&) = default;
        ~Shown() { panel.reset(); }                 // the Panel first: it holds references into `keep`
    };
    struct Engine;
    // `controls` gives the Panel's services and takes the frame's mode and clutch; it outlives the Panel.
    // `window`: the window's run (it can serve a save dialog), not tee-headless's.
    Shown makePanel(const Options&, Engine&, input::Controls& controls, bool window = false);
}
