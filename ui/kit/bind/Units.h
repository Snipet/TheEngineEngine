// ui/kit/bind/Units.h: a physical quantity's value in SI, as text in the unit a person chose, and back (card J, item 7).
//
// A Spec field says what it is with ui::quantity("pressure") (docs/plan/02-contracts.md section 3); everything inside
// the simulator and the Build is SI (Pa, K, m, kg, s, rad/s; angles in crank degrees). What a person reads and types is
// in the unit chosen for that quantity: kPa or psi, C or F, rpm. Units holds that choice, one unit per quantity, and
// converts: user = si * scale + offset (an offset only for temperatures).
//
//   Units u;                                     // every quantity in its first unit (kPa, C, mm, ...)
//   u.choose(Quantity::pressure, "psi");
//   u.format(Quantity::pressure, 101325.0)       // "14.7 psi"
//   u.parse(Quantity::pressure, "1 bar", si)     // any of the quantity's units may be typed; none: the chosen one
//   std::string s = u.save();                    // "pressure=psi\n...": the settings' lines
//
// Text is ASCII (the panel's font, J1's asciiText): "C", not a degree sign. Plain C++: no FunkGui.
#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace tee::ui
{
    enum class Quantity : std::uint8_t
    {
        none, pressure, temperature, length, area, volume, mass, inertia, speed, rotation, torque, power, angle, time,
        frequency, flow, ratio, fraction,
    };
    inline constexpr int kQuantities = static_cast<int>(Quantity::fraction) + 1;

    // One unit of a quantity: user = si * scale + offset.
    struct Unit
    {
        const char* name;          // as shown and typed: "kPa"
        double scale = 1.0, offset = 0.0;
        int decimals = -1;         // its usual digits after the point; -1: by the value's size
    };

    // "pressure" -> Quantity::pressure; Quantity::none for a name that is none of section 3's.
    Quantity quantityOf(std::string_view name) noexcept;
    const char* nameOf(Quantity q) noexcept;
    // A quantity's units, the default first. Quantity::none has one, with no name.
    std::span<const Unit> unitsOf(Quantity q) noexcept;

    class Units
    {
    public:
        // The chosen unit's index among unitsOf(q).
        int chosen(Quantity q) const noexcept { return chosen_[static_cast<std::size_t>(q)]; }
        const Unit& unit(Quantity q) const noexcept;
        // False (and nothing changed) when the quantity has no unit of that name.
        bool choose(Quantity q, std::string_view unitName) noexcept;
        void choose(Quantity q, int index) noexcept;

        double toUser(Quantity q, double si) const noexcept;
        double toSi(Quantity q, double user) const noexcept;

        // The value in the chosen unit with its name: "14.7 psi". `decimals` -1: the unit's own, else by the size.
        std::string format(Quantity q, double si, int decimals = -1) const;
        // The number alone ("14.7").
        std::string number(Quantity q, double si, int decimals = -1) const;
        // Typed text to SI: a number, then optionally one of q's units ("1 bar", "30psi"), else the chosen unit is
        // meant. False for text that is no number or names a unit q does not have.
        bool parse(Quantity q, std::string_view text, double& si) const;

        // The choices as settings lines, "quantity=unit" each (only those not at the default), and back; a line that
        // names no quantity or unit is skipped and said in `notes`.
        std::string save() const;
        void load(std::string_view text, std::string& notes);

        // Bumps at every choice: what shows a value may cache its text by it.
        std::uint32_t revision() const noexcept { return revision_; }

    private:
        std::array<int, kQuantities> chosen_{};
        std::uint32_t revision_ = 0;
    };

    // A number as text with `decimals` digits (-1: 2 under 10, 1 under 100, then 0), ASCII, no exponent, "-0" as "0".
    std::string formatNumber(double v, int decimals);
}
