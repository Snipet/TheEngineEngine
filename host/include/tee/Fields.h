// host/include/tee/Fields.h: the simulator's main module as the host, the document and the interface see it: every
// field of its four blocks (spec, ctl, tel, scope) with what its attributes say (02-contracts.md, section 3) parsed into
// a FieldInfo, the Spec's sections and their `rev` fields, and FieldRef, a resolved field any thread may use.
//
//   std::string error;
//   auto fields = tee::Fields::of(*schema, error);                 // the main module's Schema (Schema.h)
//   const tee::FieldInfo* bore = fields->find("spec.engine.bore");  // label, range, default, quantity, ...
//   tee::FieldRef ref = fields->ref("spec.engine.bore", error);     // SimHost::set takes it
//
// What a field is. A field of numbers (float, double, int, bool) is one FieldInfo, an array of them too (its extents
// say how many: a FieldRef may name one element, "spec.sound.harmonics[2]"). A field whose module is Curve16 or Map16
// is a table: one FieldInfo, its leaves the table's own fields (x, y, n; x, y, z, nx, ny), written whole. A field of
// any other module is followed: its fields are fields of their own ("spec.engine.bore"), and an array of modules is
// unrolled ("spec.boost.turbo[1].size"). A pointer, a vector, a string or a handle is no field here.
//
// A section is a field of `spec` held by value whose first field is `int rev;` (02-contracts.md, section 2, rule 4):
// the host adds one to it after it has written any field of the section, and that is how the simulator learns of it.
//
// Threads: made once a program, off the audio thread; read-only after. A FieldRef is plain data.
#pragma once

#include "tee/Schema.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    // What a field is to a person (and to the interface that shows it).
    enum class FieldKind : std::uint8_t
    {
        Number,    // a float or a double
        Integer,   // an int without ui::choices
        Switch,    // a bool
        Choice,    // an int with ui::choices: its options' names, from the range's minimum up
        Array,     // an array of numbers, ints or bools
        Curve,     // a Curve16
        Map,       // a Map16
        Rev,       // a section's `int rev`: the host's, never a person's
    };

    const char* kindName(FieldKind kind) noexcept;

    // The quantities ui::quantity may name (02-contracts.md, section 3), and whether a name is one.
    const std::vector<std::string>& quantities();
    bool isQuantity(std::string_view name) noexcept;

    struct FieldInfo
    {
        // One run of numbers inside a field: a field of numbers has one; a table one for each of its own fields.
        struct Leaf
        {
            std::string name;          // the table's field ("x", "z", "n"); empty for a field that is no table
            std::size_t offset = 0;    // bytes from the field's start
            Scalar scalar = Scalar::None;
            std::size_t count = 1;     // elements
            std::size_t stride = 0;    // one element's bytes
            std::vector<std::size_t> extents;   // its shape, outermost first ({16, 16} for a map's z)
        };

        std::string path;              // from the main module: "spec.engine.bore", "tel.engine.speed"
        std::string block;             // the first part: "spec", "ctl", "tel" or "scope"
        std::string type;              // as FMinus writes it: "float", "float[4]", "Curve16"
        FieldKind kind = FieldKind::Number;
        Scalar scalar = Scalar::None;  // one element's (Scalar::None for a table)
        std::size_t offset = 0;        // bytes from the main module's start
        std::size_t size = 0;          // the whole field's bytes
        std::size_t count = 1;         // elements (an array's), 1 otherwise
        std::vector<std::size_t> extents;
        std::vector<Leaf> leaves;
        int section = -1;              // its section in Fields::sections(), or -1 (not in the Spec, or in none)

        // The attributes (02-contracts.md, section 3). An optional that is empty was not written.
        std::optional<double> minimum, maximum;   // fminus::range
        std::optional<double> defaultValue;       // fminus::default
        std::string label;             // ui::label
        std::string quantity;          // ui::quantity
        std::string unit;              // ui::unit
        std::vector<std::string> choices;         // ui::choices
        std::string taper;             // ui::taper: "log", or empty (linear)
        int decimals = -1;             // ui::decimals; -1 when not written
        std::string group;             // ui::group, as it holds from the field that wrote it
        std::string help;              // ui::help
        bool hidden = false;           // ui::hidden (on it, or on a module that holds it)
        std::string axisX, axisY;      // ui::axes, a table's

        bool table() const noexcept { return kind == FieldKind::Curve || kind == FieldKind::Map; }
        bool hasRange() const noexcept { return minimum.has_value() && maximum.has_value(); }
        // The value a new build has: fminus::default, else 0 (02-contracts.md, section 2, rule 1).
        double fallback() const noexcept { return defaultValue.value_or(0.0); }
        // `value` within the range (when there is one), and an int's or a bool's made whole.
        double clamp(double value) const noexcept;
        // A choice's index from its name, or -1.
        int choiceIndex(std::string_view name) const noexcept;
    };

    // A Spec section: spec.engine, spec.car, ...
    struct Section
    {
        std::string path;              // "spec.engine"
        std::string name;              // "engine"
        std::size_t offset = 0;        // its first byte, from the main module's start
        std::size_t size = 0;
        std::size_t revOffset = 0;     // its `int rev`
    };

    // A field resolved: where it is, how big, what it holds, its section. Plain data for any thread. `generation` is
    // the Fields' it came from: a SimHost refuses a ref of another program (after a reload, resolve it again).
    struct FieldRef
    {
        static constexpr std::uint32_t kNone = 0xffffffffu;

        std::uint32_t offset = kNone;  // from the main module's start
        std::uint32_t size = 0;        // bytes: an element's, a whole array's or a whole table's, as the path named it
        std::uint32_t count = 0;       // elements
        Scalar scalar = Scalar::None;  // an element's; None for a table
        std::int16_t section = -1;     // the Spec section it is in, or -1
        std::int32_t field = -1;       // its FieldInfo's index in Fields::all()
        std::uint32_t generation = 0;

        bool valid() const noexcept { return offset != kNone; }
        explicit operator bool() const noexcept { return valid(); }
    };

    class Fields
    {
    public:
        // The fields of `schema`, a main module's (Engine). nullptr, with `error` set, when it has no `spec`, `ctl`,
        // `tel` or `scope` block. Each Fields made gets a generation of its own.
        static std::unique_ptr<Fields> of(const Schema& schema, std::string& error);

        const Schema& schema() const noexcept { return *schema_; }
        std::uint32_t generation() const noexcept { return generation_; }

        // Every field, the blocks in the order spec, ctl, tel, scope, each in declaration order.
        const std::vector<FieldInfo>& all() const noexcept { return fields_; }
        const std::vector<Section>& sections() const noexcept { return sections_; }

        // The field of that path (no index), or nullptr; its index, or -1.
        const FieldInfo* find(std::string_view path) const noexcept;
        int indexOf(std::string_view path) const noexcept;
        // The section of that name ("engine") or path ("spec.engine"), or -1.
        int sectionOf(std::string_view nameOrPath) const noexcept;

        // Resolves a path: a field, an element of an array ("spec.sound.harmonics[2]"), a table whole, or one of a
        // table's own fields ("spec.engine.torqueShape.n"). An invalid ref, with `error` set, when it names nothing.
        FieldRef ref(std::string_view path, std::string& error) const;
        FieldRef ref(int index) const noexcept;

        struct Block
        {
            std::size_t offset = 0;
            std::size_t size = 0;
        };
        Block spec() const noexcept { return spec_; }
        Block ctl() const noexcept { return ctl_; }
        Block tel() const noexcept { return tel_; }
        Block scope() const noexcept { return scope_; }

        // The fields as JSON, for tee-schema and for whoever builds an interface from them.
        std::string json() const;
        // One line a field, "path<TAB>type<TAB>kind": what tests/host/schema.snapshot holds.
        std::string snapshot() const;
        // docs/parameters.md: the Spec as tables a person reads, one a section.
        std::string markdown() const;

    private:
        Fields() = default;

        const Schema* schema_ = nullptr;
        std::uint32_t generation_ = 0;
        std::vector<FieldInfo> fields_;
        std::vector<Section> sections_;
        Block spec_, ctl_, tel_, scope_;
    };

    // The contract's lint (card C, section 1): what is wrong with the fields' attributes, a line each ("spec.engine.bore:
    // no fminus::default"); empty when nothing is. Every Spec field has a label, a range and a default (a switch a
    // default, a table a label and its axes); every quantity is one of the list; a choice's names are as many as its
    // range holds; a log taper's range is above 0; a default is within its range; every field directly in `spec` is a
    // section starting with `int rev;`; every telemetry field has a label.
    std::vector<std::string> lint(const Fields& fields);

    // What a snapshot (Fields::snapshot's text, as committed) says that `fields` broke, a line each: a path that
    // disappeared or changed its type. A field added is no problem: the Spec is append-only.
    std::vector<std::string> compareSnapshot(const Fields& fields, std::string_view snapshot);
} // namespace tee
