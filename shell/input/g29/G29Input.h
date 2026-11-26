// shell/input/g29/G29Input.h: a G29's state (g29/G29.h) into the Driver, and its editing buttons into the frame, a
// frame at a time, as the keys and a gamepad go in (input/Driver.h). docs/controls.md, "A wheel and pedals", says it to
// a person.
//
//   the pedals             Driver::setWheel: throttle, brake and clutch, each the larger of the wheel's and the
//                          others' (a gamepad's, the keys', the Panel's strips)
//   the steering           not the car's (it runs in a straight line): RacingWheel::state() publishes it, nothing
//                          drives with it
//   the shifter            the gate it is in, asked for as a gear key asks (Driver::requestGear): with the automatic
//                          clutch off, the H-pattern lever itself (the clutch pedal is yours); with it on, the gear
//                          sought through the gearbox's own shifts (a sequential or automatic box too); out of every
//                          gate: neutral
//   the buttons            what the bindings say (Buttons below), of two kinds:
//     a driving action     (Keys.h's Action) held while the button is held (Driver::set(Source::wheel, ...)): a shift
//                          counts its press, the start key is held, a toggle turns at each press. By default the right
//                          paddle shifts up, the left down, PS is the start key.
//     an edit command      (EditCommand) given to the frame once a press (the inspector's hands: rows, pages,
//                          workspaces, values, undo): Input::apply collects them, the app hands them to the frame's
//                          edit(). The D-pad's up and down repeat while held (kRepeatAfter, then kRepeatRate a second).
//                          SHARE held makes a value's command fine (a thousandth of its travel, as Shift). The
//                          defaults: editDefaults() below; the engine keeps running while it is edited (no mode).
//   a learn                (ButtonLearn: "press the button for ...") binds the next button pressed to one of either
//                          kind; kept in the settings file (wheelButtons, saveButtons' format)
//
// Driving wins: an edit default never takes a button the owner bound (loadButtons' migration), and SHARE is the
// modifier only while no binding has it.
//
// Reading stops (the wheel off or gone): everything the wheel held is let go, once (Driver::release(Source::wheel)).
#pragma once

#include "input/Keys.h"
#include "input/g29/G29.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tee::input
{
    class Driver;
}

namespace tee::input::g29
{
    // The frame's editing commands (the seam U1's frame and J2's inspector serve: edit(EditCommand, fine)). Held here
    // until the frame's header exists, then this is that header's.
    enum class EditCommand : std::uint8_t
    {
        rowUp, rowDown, pagePrev, pageNext, workspacePrev, workspaceNext, valueDown, valueUp, valueDownCoarse,
        valueUpCoarse, activate, back, undo, redo, toDefault, changedOnly,
        count
    };
    std::string_view editName(EditCommand);                  // "rowup", ..., as the settings file writes it
    std::optional<EditCommand> editFromName(std::string_view);
    std::string_view editLabel(EditCommand);                 // "the inspector's row up", for a person

    inline constexpr int kBitShare = 8;                      // SHARE: the fine modifier (while unbound)
    static_assert(kBits[kBitShare].button == 9);

    // A button of the mask (G29.h's kBits) bound to a driving action or to an edit command.
    struct Binding
    {
        int bit = -1;
        Action action = Action::none;
        std::optional<EditCommand> edit;
        bool isEdit() const noexcept { return edit.has_value(); }
        friend bool operator==(const Binding&, const Binding&) = default;
    };
    using Buttons = std::vector<Binding>;
    inline Binding drive(int bit, Action a) { return { bit, a, std::nullopt }; }
    inline Binding editOn(int bit, EditCommand c) { return { bit, Action::none, c }; }

    // The driving defaults: the right paddle shift up, the left paddle shift down, the PS button the start key.
    Buttons drivingDefaults();
    // The editing defaults (docs/controls.md's table): the D-pad rows and pages, L2/R2 the workspaces, the dial a value
    // a nudge, minus/plus ten, the dial's return and cross activate, circle back, square undo, triangle redo, L3 the
    // default, OPTIONS the "changed only" filter. R3 is free.
    Buttons editDefaults();
    Buttons defaultButtons();                                // both

    // As the settings file keeps them: "2:4=shiftup,5=shiftdown,17=start,18=e.rowup,..." (Keys.h's action names; an
    // edit command's with "e."). Empty: the defaults.
    std::string saveButtons(const Buttons&);
    // Read back. "2:..." is taken exactly. A string without "2:" is the format before editing from the wheel: every
    // pair is kept as the owner made it, and each edit default is added on a button no pair uses (driving wins). A pair
    // it cannot read is left out and said in `notes`. Empty text: the defaults.
    Buttons loadButtons(const std::string& text, std::string& notes);

    // What a button may be bound to, in the order the learn menu lists them: the driving actions, then the edits.
    struct Target
    {
        Action action = Action::none;
        std::optional<EditCommand> edit;
    };
    const std::vector<Target>& bindables();
    std::string targetName(const Target&);                   // "shiftup" or "e.rowup"
    std::string targetLabel(const Target&);                  // "drive: shiftup" or "edit: the inspector's row up"

    // Learning a button for a target: begin(target, state now), then watch() each frame; the first button that goes
    // down (one that was not down when learning began; SHARE never, while it is the modifier) is bound to it. The
    // button loses whatever it had. A driving action is taken from any button that had it (it lives on one); an edit
    // command may stay on other buttons too (activate on both the dial's return and cross). A learn not answered
    // within kSeconds ends with nothing.
    class ButtonLearn
    {
    public:
        static constexpr double kSeconds = 10.0;
        void begin(Target target, const State& now);
        void cancel() noexcept { active_ = false; }
        bool active() const noexcept { return active_; }
        const Target& target() const noexcept { return target_; }
        // Each frame while active (`dt` seconds on): true when a button was bound (learning ends); timed out: false,
        // and active() false.
        bool watch(const State& now, double dt, Buttons& buttons);

    private:
        bool active_ = false;
        Target target_;
        std::uint32_t was_ = 0;                             // the buttons down when it began
        double left_ = 0.0;
    };

    // One edit command from the wheel: `fine` while SHARE is held.
    struct Edit
    {
        EditCommand command;
        bool fine = false;
        friend bool operator==(const Edit&, const Edit&) = default;
    };

    inline constexpr double kRepeatAfter = 0.35;             // a held repeating button's first repeat (s)
    inline constexpr double kRepeatRate = 12.0;              // then this many a second
    bool repeats(EditCommand) noexcept;                      // the D-pad's row up and down

    class Input
    {
    public:
        // The bindings from now on (a button held under the old ones is let go).
        void setButtons(Buttons buttons);
        const Buttons& buttons() const noexcept { return buttons_; }

        // Once a frame, `dt` seconds on. `reading`: the wheel is read (RacingWheel::reading()); false: let go, the
        // first time. `buttonsLive` false (a learn is listening): the buttons do nothing this frame (their presses are
        // the learn's). The edit commands of this frame: edits() until the next apply (at most one a button a frame:
        // a dial's detents faster than the frames count once each frame).
        // (tee::input::Driver, the keys' and the gamepad's: not g29::Driver, the wheel's own decisions)
        void apply(const State& now, bool reading, tee::input::Driver& driver, bool buttonsLive = true, double dt = 0.0);
        const std::vector<Edit>& edits() const noexcept { return edits_; }

    private:
        bool modifier(const State& now) const noexcept;

        Buttons buttons_ = defaultButtons();
        State was_;
        bool active_ = false;
        bool releaseButtons_ = false;
        std::vector<Edit> edits_;
        std::array<double, 32> held_{};                      // seconds a repeating button has been held
        std::array<double, 32> next_{};                      // its next repeat at
    };
}
