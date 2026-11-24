// app/Offline.h: the application with no window and no audio device: the same Player, Controls and Panel, run on a
// simulated clock of 60 frames a second. Each frame it does what the script says is due, advances the controls,
// renders that frame's share of the audio (to a WAV file when asked), ticks the Panel, and takes the pictures that are
// due with FunkGui's software rasteriser. Nothing in it depends on how fast the machine is, so the same script gives
// the same audio and the same pictures every time. tee-headless is this; the window's --seconds is too.
#pragma once

#include "Options.h"

#include "tee/app/Sources.h"

#include <funkgui/panel/Panel.h>

#include <memory>

namespace tee::app
{
    // Runs it. 0: done, and every check of the script held. 1: a check failed, or a file could not be written. 2: the
    // options or the script could not be read, or the program did not compile (without --keep-going).
    int runOffline(const Options&);

    // The versions, a line each: the application, FMinus and FunkGui.
    void printVersions();

    // The Panel for this run: the interface's (tee::ui::makeMainPanel), or the shell's plain one with --panel plain
    // and when the program did not start.
    std::unique_ptr<funkgui::Panel> makePanel(const Options&, Player&, const AppServices&);
}
