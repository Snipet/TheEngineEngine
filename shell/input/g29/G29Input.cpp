// shell/input/g29/G29Input.cpp: G29Input.h.
#include "input/g29/G29Input.h"

#include "input/Driver.h"

#include <algorithm>
#include <iterator>

namespace tee::input::g29
{
    namespace
    {
        struct EditRow
        {
            EditCommand command;
            const char* name;
            const char* label;
        };
        constexpr EditRow kEdits[] = {
            { EditCommand::rowUp, "rowup", "the inspector's row up" },
            { EditCommand::rowDown, "rowdown", "the inspector's row down" },
            { EditCommand::pagePrev, "pageprev", "the inspector's page before" },
            { EditCommand::pageNext, "pagenext", "the inspector's page after" },
            { EditCommand::workspacePrev, "workspaceprev", "the workspace before" },
            { EditCommand::workspaceNext, "workspacenext", "the workspace after" },
            { EditCommand::valueDown, "valuedown", "the value down a nudge" },
            { EditCommand::valueUp, "valueup", "the value up a nudge" },
            { EditCommand::valueDownCoarse, "valuedowncoarse", "the value down ten nudges" },
            { EditCommand::valueUpCoarse, "valueupcoarse", "the value up ten nudges" },
            { EditCommand::activate, "activate", "activate (flip, fold, step, open)" },
            { EditCommand::back, "back", "back (close a table; let the focus go)" },
            { EditCommand::undo, "undo", "undo" },
            { EditCommand::redo, "redo", "redo" },
            { EditCommand::toDefault, "todefault", "the field to its default" },
            { EditCommand::changedOnly, "changedonly", "the \"changed only\" filter" },
        };
        static_assert(std::size(kEdits) == static_cast<std::size_t>(EditCommand::count));

        constexpr std::string_view kVersion = "2:";
        constexpr std::string_view kEditPrefix = "e.";

        bool bindableAction(Action a)
        {
            for (const Target& t : bindables())
                if (!t.edit && t.action == a)
                    return true;
            return false;
        }

        // Each pair of "bit=name,...": false (and a note) for one it cannot read.
        Buttons readPairs(std::string_view text, std::string& notes)
        {
            Buttons out;
            std::size_t at = 0;
            while (at <= text.size())
            {
                const std::size_t comma = std::min(text.find(',', at), text.size());
                const std::string pair(text.substr(at, comma - at));
                at = comma + 1;
                if (pair.empty())
                    continue;
                const std::size_t eq = pair.find('=');
                int bit = -1;
                if (eq != std::string::npos && eq > 0)
                    try
                    {
                        bit = std::stoi(pair.substr(0, eq));
                    }
                    catch (...)
                    {
                        bit = -1;
                    }
                const std::string name = eq == std::string::npos ? std::string() : pair.substr(eq + 1);
                Binding b;
                b.bit = bit;
                bool known = false;
                if (name.rfind(kEditPrefix, 0) == 0)
                {
                    b.edit = editFromName(std::string_view(name).substr(kEditPrefix.size()));
                    known = b.edit.has_value();
                }
                else
                {
                    b.action = actionFromName(name);
                    known = bindableAction(b.action);
                }
                if (bit < 0 || bit >= static_cast<int>(std::size(kBits)) || !known)
                {
                    notes += (notes.empty() ? "" : "; ") + std::string("the wheel's binding \"") + pair + "\" was not read";
                    continue;
                }
                out.push_back(b);
            }
            return out;
        }
    }

    std::string_view editName(EditCommand c)
    {
        for (const EditRow& r : kEdits)
            if (r.command == c)
                return r.name;
        return {};
    }

    std::optional<EditCommand> editFromName(std::string_view name)
    {
        for (const EditRow& r : kEdits)
            if (name == r.name)
                return r.command;
        return std::nullopt;
    }

    std::string_view editLabel(EditCommand c)
    {
        for (const EditRow& r : kEdits)
            if (r.command == c)
                return r.label;
        return {};
    }

    Buttons drivingDefaults()
    {
        return { drive(kBitRightPaddle, Action::shiftUp), drive(kBitLeftPaddle, Action::shiftDown),
                 drive(kBitPs, Action::start) };
    }

    Buttons editDefaults()
    {
        // the bits by name (G29.h's kBits): a name not there is a table changed under this list
        const auto bit = [](std::string_view name) {
            for (int b = 0; b < static_cast<int>(std::size(kBits)); ++b)
                if (name == kBits[b].name)
                    return b;
            return -1;
        };
        using E = EditCommand;
        return { editOn(bit("d-pad up"), E::rowUp),         editOn(bit("d-pad down"), E::rowDown),
                 editOn(bit("d-pad left"), E::pagePrev),    editOn(bit("d-pad right"), E::pageNext),
                 editOn(bit("L2"), E::workspacePrev),       editOn(bit("R2"), E::workspaceNext),
                 editOn(bit("dial left"), E::valueDown),    editOn(bit("dial right"), E::valueUp),
                 editOn(bit("-"), E::valueDownCoarse),      editOn(bit("+"), E::valueUpCoarse),
                 editOn(bit("return"), E::activate),        editOn(bit("cross"), E::activate),
                 editOn(bit("circle"), E::back),            editOn(bit("square"), E::undo),
                 editOn(bit("triangle"), E::redo),          editOn(bit("L3"), E::toDefault),
                 editOn(bit("options"), E::changedOnly) };
    }

    Buttons defaultButtons()
    {
        Buttons b = drivingDefaults();
        for (const Binding& e : editDefaults())
            b.push_back(e);
        return b;
    }

    const std::vector<Target>& bindables()
    {
        static const std::vector<Target> all = [] {
            std::vector<Target> t;
            for (const Action a : { Action::start, Action::shiftUp, Action::shiftDown, Action::neutral, Action::handbrake,
                                    Action::splitter, Action::range, Action::engineBrake })
                t.push_back({ a, std::nullopt });
            for (const EditRow& r : kEdits)
                t.push_back({ Action::none, r.command });
            return t;
        }();
        return all;
    }

    std::string targetName(const Target& t)
    {
        return t.edit ? std::string(kEditPrefix) + std::string(editName(*t.edit)) : std::string(actionName(t.action));
    }

    std::string targetLabel(const Target& t)
    {
        return t.edit ? "edit: " + std::string(editLabel(*t.edit)) : "drive: " + std::string(actionName(t.action));
    }

    std::string saveButtons(const Buttons& buttons)
    {
        std::string s(kVersion);
        bool first = true;
        for (const Binding& b : buttons)
        {
            if (b.bit < 0 || (!b.edit && b.action == Action::none))
                continue;
            s += (first ? "" : ",") + std::to_string(b.bit) + "=" + targetName({ b.action, b.edit });
            first = false;
        }
        return s;
    }

    Buttons loadButtons(const std::string& text, std::string& notes)
    {
        if (text.empty())
            return defaultButtons();
        if (text.rfind(kVersion, 0) == 0)
            return readPairs(std::string_view(text).substr(kVersion.size()), notes);
        // before editing from the wheel: the owner's pairs kept, the edit defaults added where no pair is
        Buttons out = readPairs(text, notes);
        const Buttons owned = out;
        for (const Binding& e : editDefaults())
            if (std::none_of(owned.begin(), owned.end(), [&](const Binding& b) { return b.bit == e.bit; })
                && e.bit != kBitShare)
                out.push_back(e);
        return out;
    }

    void ButtonLearn::begin(Target target, const State& now)
    {
        active_ = true;
        target_ = target;
        was_ = now.buttons;
        left_ = kSeconds;
    }

    bool ButtonLearn::watch(const State& now, double dt, Buttons& buttons)
    {
        if (!active_)
            return false;
        left_ -= dt;
        // a button that was down when learning began counts once it has been let go
        was_ &= now.buttons;
        std::uint32_t pressed = now.buttons & ~was_;
        // SHARE is the modifier while nothing has it: never the answer then
        const bool shareBound = std::any_of(buttons.begin(), buttons.end(), [](const Binding& b) { return b.bit == kBitShare; });
        if (!shareBound && (pressed & (1u << kBitShare)) != 0)
        {
            pressed &= ~(1u << kBitShare);
            was_ |= 1u << kBitShare;                        // said once a press
            shareRefused_ = true;
        }
        if (pressed == 0)
        {
            if (left_ <= 0.0)
                active_ = false;
            return false;
        }
        int bit = 0;
        while (((pressed >> bit) & 1u) == 0)
            ++bit;
        const Target t = target_;
        buttons.erase(std::remove_if(buttons.begin(), buttons.end(),
                                     [&](const Binding& b) {
                                         // the button loses what it had; a driving action lives on one button
                                         return b.bit == bit || (!t.edit && !b.edit && b.action == t.action);
                                     }),
                      buttons.end());
        buttons.push_back({ bit, t.action, t.edit });
        active_ = false;
        return true;
    }

    bool repeats(EditCommand c) noexcept
    {
        return c == EditCommand::rowUp || c == EditCommand::rowDown || c == EditCommand::valueDownCoarse
               || c == EditCommand::valueUpCoarse;
    }

    void Input::setButtons(Buttons buttons)
    {
        buttons_ = std::move(buttons);
        releaseButtons_ = true;
    }

    bool Input::modifier(const State& now) const noexcept
    {
        if (!now.button(kBitShare))
            return false;
        return std::none_of(buttons_.begin(), buttons_.end(), [](const Binding& b) { return b.bit == kBitShare; });
    }

    void Input::apply(const State& now, bool reading, tee::input::Driver& driver, bool buttonsLive, double dt,
                      const Presses* presses)
    {
        edits_.clear();
        if (!reading)
        {
            if (active_)
            {
                driver.release(Source::wheel);
                was_ = State{};
                active_ = false;
            }
            held_.fill(0.0);
            return;
        }
        const bool first = !active_;
        active_ = true;
        driver.setWheel(now.pedal(Axis::throttle), now.pedal(Axis::brake), now.pedal(Axis::clutch));
        // the bindings changed, or a learn is listening: what the wheel's buttons held is let go, and a button held now
        // counts once it is pressed again
        if (releaseButtons_ || !buttonsLive)
        {
            for (const Target& t : bindables())
                if (!t.edit)
                    driver.set(Source::wheel, t.action, false);
            releaseButtons_ = false;
            held_.fill(0.0);
        }
        else
        {
            const bool fine = modifier(now);
            for (const Binding& b : buttons_)
            {
                const bool down = now.button(b.bit), was = was_.button(b.bit);
                if (!b.edit)
                {
                    if (down != was)
                        driver.set(Source::wheel, b.action, down);
                    continue;
                }
                // every press since the last frame (the transport's count), or the sample's edge where none is counted
                const int counted = presses != nullptr ? (*presses)[static_cast<std::size_t>(b.bit)] : 0;
                const int n = std::min(std::max(counted, down && !was ? 1 : 0), kMostPerFrame);
                if (n > 0)
                {
                    for (int k = 0; k < n; ++k)
                        edits_.push_back({ *b.edit, fine });
                    held_[static_cast<std::size_t>(b.bit)] = 0.0;
                    next_[static_cast<std::size_t>(b.bit)] = kRepeatAfter;
                }
                else if (down && repeats(*b.edit))
                {
                    // held: the first repeat after kRepeatAfter, then kRepeatRate a second, at most one a frame
                    double& h = held_[static_cast<std::size_t>(b.bit)];
                    double& at = next_[static_cast<std::size_t>(b.bit)];
                    h += dt;
                    if (h >= at)
                    {
                        edits_.push_back({ *b.edit, fine });
                        at = std::max(at + 1.0 / kRepeatRate, h);
                    }
                }
            }
        }
        // the shifter: a change of gate (and the first frame read, a gate already in: what the lever says from now on)
        if (now.gear != was_.gear || (first && now.gear != 0))
            driver.requestGear(now.gear);
        was_ = now;
    }
}
