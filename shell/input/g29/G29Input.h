// shell/input/g29/G29Input.h: a G29's state (g29/G29.h) into the Driver, a frame at a time, as the keys and a gamepad
// go in (input/Driver.h). docs/controls.md, "A wheel and pedals", says it to a person.
//
//   the pedals             Driver::setWheel: throttle, brake and clutch, each the larger of the wheel's and the
//                          others' (a gamepad's, the keys', the Panel's strips)
//   the steering           not the car's (it runs in a straight line): RacingWheel::state() publishes it, nothing
//                          drives with it
//   the right paddle       by default: shift up (a press each); the left paddle: shift down
//   the PS button          by default: the start key, held as long as it is held (a short press starts a stopped engine; held
//                          on a running one, off: as the key)
//   the shifter            the gate it is in, asked for as a gear key asks (Driver::requestGear): with the automatic
//                          clutch off, the H-pattern lever itself (the clutch pedal is yours); with it on, the gear
//                          sought through the gearbox's own shifts (a sequential or automatic box too); out of every
//                          gate: neutral
//   other buttons          what the bindings say (Buttons below): by default nothing; a person binds one by its
//                          learn (ButtonLearn: "press the button for ..."), kept in the settings file (wheelButtons)
//
// The paddles and the PS button are bindings too (the defaults), so a person may move them. A button is an action
// held while it is held (Driver::set(Source::wheel, ...)): a shift counts its press, the start key is held, a toggle
// (the splitter, the range, the engine brake) turns at each press.
//
// Reading stops (the wheel off or gone): everything the wheel held is let go, once (Driver::release(Source::wheel)).
#pragma once

#include "input/Keys.h"
#include "input/g29/G29.h"

#include <string>
#include <vector>

namespace tee::input
{
    class Driver;
}

namespace tee::input::g29
{
    // A button of the mask (G29.h's kBits) bound to an action.
    struct Binding
    {
        int bit = -1;
        Action action = Action::none;
        friend bool operator==(const Binding&, const Binding&) = default;
    };
    using Buttons = std::vector<Binding>;

    // The defaults: the right paddle shift up, the left paddle shift down, the PS button the start key.
    Buttons defaultButtons();
    // As the settings file keeps them: "4=shiftup,5=shiftdown,17=start" (Keys.h's action names); empty: the defaults.
    std::string saveButtons(const Buttons&);
    // Read back; a pair it cannot read (a bit outside the mask, an action it does not know) is left out and said in
    // `notes`. Empty text: the defaults.
    Buttons loadButtons(const std::string& text, std::string& notes);
    // The actions a wheel's button may be bound to, in the order a menu lists them.
    const std::vector<Action>& bindableActions();

    // Learning a button for an action: begin(action, state now), then watch() each frame; the first button that goes
    // down (one that was not down when learning began) is bound to the action: it takes the action from any button
    // that had it, and the button loses whatever action it had. A learn not answered within kSeconds ends with nothing.
    class ButtonLearn
    {
    public:
        static constexpr double kSeconds = 10.0;
        void begin(Action action, const State& now);
        void cancel() noexcept { active_ = false; }
        bool active() const noexcept { return active_; }
        Action action() const noexcept { return action_; }
        // Each frame while active (`dt` seconds on): true when a button was bound (learning ends); timed out: false,
        // and active() false.
        bool watch(const State& now, double dt, Buttons& buttons);

    private:
        bool active_ = false;
        Action action_ = Action::none;
        std::uint32_t was_ = 0;                             // the buttons down when it began
        double left_ = 0.0;
    };

    class Input
    {
    public:
        // The bindings from now on (a button held under the old ones is let go).
        void setButtons(Buttons buttons);
        const Buttons& buttons() const noexcept { return buttons_; }

        // Once a frame. `reading`: the wheel is read (RacingWheel::reading()); false: let go, the first time.
        // `buttonsLive` false (a learn is listening): the buttons do nothing this frame (their presses are the learn's).
        // (tee::input::Driver, the keys' and the gamepad's: not g29::Driver, the wheel's own decisions)
        void apply(const State& now, bool reading, tee::input::Driver& driver, bool buttonsLive = true);

    private:
        Buttons buttons_ = defaultButtons();
        State was_;
        bool active_ = false;
        bool releaseButtons_ = false;
    };
}
