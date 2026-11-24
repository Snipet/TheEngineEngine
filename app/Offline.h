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

namespace tee::app
{
    // Runs it. 0: done, and every check of the script held. 1: a check failed, or a file could not be written. 2: the
    // options or the script could not be read, or the program did not compile (without --keep-going).
    int runOffline(const Options&);

    // The versions, a line each: the application, FMinus and FunkGui.
    void printVersions();

    // The Panel for this run, and what it needs kept alive beside it.
    //   the simulator (--engine sim), in a build with the interface's frame: tee::ui::AppFrame over the engine's
    //     tee::Build (a BuildDoc, the user's Units, the garage's builds);
    //   the reference: the interface's tee::ui::makeMainPanel (the reference's own panel);
    //   --panel plain, a program that did not start, or a build with no interface package: the shell's plain Panel.
    struct Shown
    {
        std::shared_ptr<void> keep;                 // the frame's document and units; destroyed after the Panel
        std::unique_ptr<funkgui::Panel> panel;
        // The units as the frame has them now, as the settings file keeps them; empty function: no frame.
        std::function<std::string()> unitChoices;

        Shown() = default;
        Shown(Shown&&) = default;
        Shown& operator=(Shown&&) = default;
        ~Shown() { panel.reset(); }                 // the Panel first: it holds references into `keep`
    };
    struct Engine;
    Shown makePanel(const Options&, Engine&, const AppServices&);
}
