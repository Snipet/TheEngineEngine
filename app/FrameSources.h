// app/FrameSources.h: the shell's share of what the interface's frame (ui/app/AppFrame.h) is given over the simulator.
// The Scope and the dyno are U1's (ui/app/host/HostSources.h: SimScope, ServiceDyno); the build's own services are
// J2's (ui/kit/bind/host/BuildDoc.h: buildServices). Here:
//
//   addShellServices  the simulator's load, the cockpit's mode and clutch, the output's level
//   addFileServices   the build's name, its unsaved flag and the garage's list, load and save over the document on
//                     disk and the library (shell/files/BuildFiles.h)
//   waitForCurve      tee-headless's --dyno-wait: the dyno ticked until its first curve is in (or an error, or time)
//
// UI thread only. Compiled into the executables that have the frame (TEE_HAS_UI_FRAME).
#pragma once

#if defined(TEE_HAS_UI_FRAME)

#include "app/Workspace.h"

#include <functional>

namespace tee::input
{
    class Controls;
}

namespace tee::player
{
    class SimAdapter;
}

namespace tee::files
{
    class BuildFile;
    class BuildLibrary;
}

namespace tee::app
{
    // `level` is the output's level (0..1) for the top bar; empty: the telemetry's peak.
    void addShellServices(ui::FrameServices& services, player::SimAdapter& player, input::Controls& controls,
                          const std::function<float()>& level = {});

    // In place of the plain ones of ui::buildServices: a build loaded or saved from the frame is the document's file,
    // and its star is the dirty flag's.
    void addFileServices(ui::FrameServices& services, files::BuildFile& file, const files::BuildLibrary& library);

    // Ticks `dyno` (this machine's time) until a predicted curve is in, it reports an error, or `seconds` pass. True:
    // a curve is in.
    bool waitForCurve(ui::DynoSource& dyno, double seconds);
}

#endif
