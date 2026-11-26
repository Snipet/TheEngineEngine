// ui/kit/bind/Fields.h: what the interface needs of a Spec and its document (card J2, item 8), until card C's
// tee::Schema (FieldInfo) and tee::Build land; then an adapter of a few dozen lines makes a Build one of these
// (docs/ui-kit.md, "The binding").
//
//   FieldInfo   one field of the Spec as a person sets it: its path ("boost.level"), its section, what kind of control
//               it takes, its range and default (SI), and the attributes of docs/plan/02-contracts.md section 3 (label,
//               quantity or unit, choices, taper, decimals, group, help, hidden, axes).
//   FieldDoc    the document: every field's value by index, in SI; a Curve16's or a Map16's whole; gestures bracketing
//               a run of writes (one step of undo); a revision that moves at every change.
//   MemoryDoc   a FieldDoc in memory with undo and redo by gesture: the tests', the gallery's, and the application's
//               until the Build is there.
//
// UI thread only.
#pragma once

#include "kit/bind/Units.h"
#include "kit/widgets/Tables.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tee::ui
{
    enum class FieldKind : std::uint8_t
    {
        number,     // a float or a double: a knob
        integer,    // an int without choices: a knob in whole steps
        choice,     // an int with ui::choices: a menu
        toggle,     // a bool: a switch
        curve,      // a Curve16: CurveEdit
        map,        // a Map16: MapEdit
    };

    struct FieldInfo
    {
        std::string path;                    // "boost.level": the section, a dot, the field's name in it
        FieldKind kind = FieldKind::number;
        double min = 0.0, max = 1.0, def = 0.0;   // SI (a choice: its first index, its last, its default index)
        std::string label;                   // ui::label
        Quantity quantity = Quantity::none;  // ui::quantity: shown in the user's unit
        std::string unit;                    // ui::unit: a fixed unit, when no quantity fits
        std::vector<std::string> choices;    // ui::choices, from min up
        bool logTaper = false;               // ui::taper("log")
        int decimals = -1;                   // ui::decimals; -1: the unit's
        std::string group;                   // ui::group: its heading on a generated page ("" : the section's)
        std::string help;                    // ui::help
        bool hidden = false;                 // ui::hidden: not shown
        std::string preferUnit;              // ui::prefer (U1): the unit it reads in where the person's choice allows
                                             // ("m" for a listener's distance; Units::chosen(q, prefer))
        // A table's axes (ui::axes): the quantities of x and y, and the range each may take. Its values (a curve's y,
        // a map's z) are the field's own quantity and range (min, max).
        std::array<Quantity, 2> axes{ Quantity::none, Quantity::none };
        std::array<double, 2> axisLo{ 0.0, 0.0 }, axisHi{ 1.0, 1.0 };
        // An element of an array of numbers (the Spec's `float bankAngle[MAX_CYL]`): the array's path ("engine.bankAngle";
        // the element's own is "engine.bankAngle[2]"), its label ("Bank angle"; the element's is "Bank angle 3", counted
        // from 1), the element's index from 0 and how many the array holds. `array` empty: no array's element.
        std::string array, arrayLabel;
        int element = -1, extent = 0;
        // An array of arrays ("engine.gain[1][2]"): its outer and inner extents (element = outer * shape[1] + inner);
        // { 0, 0 } for an array of one dimension.
        std::array<int, 2> shape{ 0, 0 };

        std::string_view section() const noexcept { return std::string_view(path).substr(0, path.find('.')); }
        bool table() const noexcept { return kind == FieldKind::curve || kind == FieldKind::map; }
    };

    class FieldDoc
    {
    public:
        virtual ~FieldDoc() = default;

        virtual int count() const = 0;
        virtual const FieldInfo& info(int i) const = 0;
        virtual int find(std::string_view path) const;   // -1: none
        // A number's, an integer's, a choice's (its index) or a switch's (0 or 1) value, SI.
        virtual double get(int i) const = 0;
        // Kept in the field's range (an integer, a choice and a switch rounded).
        virtual void set(int i, double si) = 0;
        // Whether a person's input of `si` for field i is taken, before set (the document's own rule beyond the range: the
        // host's least stated bore); false with the reason, a phrase, in `why`. Every value by default.
        virtual bool accepts(int i, double si, std::string* why = nullptr) const
        {
            (void)i;
            (void)si;
            (void)why;
            return true;
        }
        virtual Curve16 curve(int i) const = 0;
        virtual void setCurve(int i, const Curve16& c) = 0;
        virtual Map16 map(int i) const = 0;
        virtual void setMap(int i, const Map16& m) = 0;
        // A run of writes that is one step of undo (a drag); they nest, and the outermost end closes the step.
        virtual void beginGesture() = 0;
        virtual void endGesture() = 0;
        // Moves at every change, from any writer.
        virtual std::uint64_t revision() const = 0;
    };

    // An array's elements (FieldInfo::array), in index order; none when `arrayPath` is no array of the document's.
    std::vector<int> arrayElements(const FieldDoc& doc, std::string_view arrayPath);

    // The sections the fields are in, in the order they first come.
    std::vector<std::string> sectionsOf(const FieldDoc& doc);
    // `v` kept in field `f`'s range (rounded for the kinds that step).
    double clampTo(const FieldInfo& f, double v) noexcept;

    class MemoryDoc final : public FieldDoc
    {
    public:
        explicit MemoryDoc(std::vector<FieldInfo> fields);   // every field at its default; a table at a plain ramp

        int count() const override { return static_cast<int>(fields_.size()); }
        const FieldInfo& info(int i) const override { return fields_[static_cast<std::size_t>(i)]; }
        double get(int i) const override { return now_.values[static_cast<std::size_t>(i)]; }
        void set(int i, double si) override;
        Curve16 curve(int i) const override { return now_.curves[static_cast<std::size_t>(i)]; }
        void setCurve(int i, const Curve16& c) override;
        Map16 map(int i) const override { return now_.maps[static_cast<std::size_t>(i)]; }
        void setMap(int i, const Map16& m) override;
        void beginGesture() override;
        void endGesture() override;
        std::uint64_t revision() const override { return revision_; }

        bool canUndo() const noexcept { return !undo_.empty(); }
        bool canRedo() const noexcept { return !redo_.empty(); }
        void undo();
        void redo();
        int steps() const noexcept { return static_cast<int>(undo_.size()); }
        int depth() const noexcept { return depth_; }   // gestures open
        void reset();                                    // every field to its default (one step)

    private:
        struct State
        {
            std::vector<double> values;
            std::vector<Curve16> curves;
            std::vector<Map16> maps;
            bool operator==(const State&) const = default;
        };
        void changed();

        std::vector<FieldInfo> fields_;
        State now_, before_;
        std::vector<State> undo_, redo_;
        int depth_ = 0;
        std::uint64_t revision_ = 1;
    };
}
