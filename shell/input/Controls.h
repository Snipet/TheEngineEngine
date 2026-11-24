// shell/input/Controls.h: the application's one set of controls. It owns the Driver and the key bindings, takes what
// a Panel's own controls ask for (PanelControls, through AppServices), and once a frame reads what the engine says,
// advances the Driver and sends the merged frame to the engine. No SDL: shell/input/SdlInput.h turns SDL's events
// into the calls below, and the headless script makes the same calls. UI thread only.
#pragma once

#include "input/Driver.h"
#include "input/Keys.h"
#include "tee/app/Sources.h"

#include <string>
#include <vector>

namespace tee::input
{
    class Controls
    {
    public:
        // The sink takes the frames; the telemetry answers "is it running" and "which gear". Both outlive this.
        Controls(ControlSink&, const TelemetrySource&);

        // Another engine plays from now on (the handles are looked up again).
        void retarget(ControlSink&, const TelemetrySource&);

        Driver& driver() noexcept { return driver_; }
        std::vector<Binding>& bindings() noexcept { return bindings_; }

        // A key went down or came up. True: it is bound and the driver took it. False: not bound, or a text field has
        // the keyboard (then nothing is taken).
        bool key(int key, bool down);

        // A text field has the keyboard: until it says false again no key drives the car (what the keyboard held is
        // let go at once).
        void setTyping(bool typing);
        bool typing() const noexcept { return typing_; }

        // The window lost the keyboard: the keys are let go.
        void focusLost();

        // What a Panel's own controls ask for (AppServices::panelControls).
        void panel(const PanelControls&);

        // Once a frame: dt seconds on, the frame sent.
        void tick(float dt);

        // The frame last sent.
        const ControlsFrame& frame() const noexcept { return driver_.frame(); }

        // The line AppServices::status answers; the application sets it when it changes.
        void setStatus(std::string status) { status_ = std::move(status); }
        const std::string& status() const noexcept { return status_; }

        // The services a Panel is made with. They hold `this`: the Controls outlive the Panel.
        AppServices services();

    private:
        ControlSink* sink_;
        const TelemetrySource* telemetry_;
        int stateHandle_ = -1, gearHandle_ = -1, drivenHandle_ = -1;
        Driver driver_;
        std::vector<Binding> bindings_;
        std::string status_;
        bool typing_ = false;
        // the Panel's controls as last seen
        int panelUp_ = 0, panelDown_ = 0, panelLever_ = 0;
        bool panelLeverSet_ = false;
    };
}
