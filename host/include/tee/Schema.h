// host/include/tee/Schema.h: a module of an FMinus program as a tree the host can walk: every field of the module and
// of the modules it holds, each with its path, its type, where it sits in an instance, its array extents and its
// attributes. It is what fminus::Program::module() says, with the nested modules followed and the offsets added up.
//
//   std::string error;
//   auto schema = tee::Schema::of(program->fminus(), "Shared", error);
//   auto rpm = schema->field<float>("showRpm", error);          // resolve once, off the audio thread
//   auto p3 = schema->field<float>("cyl[3].p", error);          // an element of an array of modules, then its field
//   ...
//   float now = instance.get(rpm);                               // on the audio thread: a load, no string work
//
// Paths. A field is named as the program names it from inside the module: `set.size` (a field of a module held by
// value), `cyl[3].p` (an element of an array, then a field of it), `fire[2]` (an element of an array of numbers),
// `grid[1][4]` (an array of arrays: outermost first). An array named without an index, or with fewer indexes than it
// has extents, is the whole of what is left: its first element's place, and how many elements follow.
//
// What a path cannot follow is a pointer (`shared->set`): the pointer is a leaf (Scalar::Pointer), and what it points
// at is an instance of its own, with a Schema of its own.
#pragma once

#include "fminus/FMinus.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace tee
{
    // What one element of a field holds, as the host reads it.
    enum class Scalar : std::uint8_t
    {
        None,      // nothing the host reads as a number: a module (its fields are the node's children), a vector, a
                   // string, a handle
        Float,     // FMinus's float: 4 bytes
        Double,    // double: 8 bytes
        Int,       // int: 4 bytes
        Bool,      // bool: 1 byte
        Char,      // char: 1 byte, read as unsigned
        Pointer,   // any pointer ("float *", "Shared *"): the host's void*
    };

    // The C++ type's Scalar: float, double, std::int32_t, bool, unsigned char, or a pointer.
    template <typename T>
    constexpr Scalar scalarOf() noexcept
    {
        if constexpr (std::is_same_v<T, float>)
            return Scalar::Float;
        else if constexpr (std::is_same_v<T, double>)
            return Scalar::Double;
        else if constexpr (std::is_same_v<T, std::int32_t>)
            return Scalar::Int;
        else if constexpr (std::is_same_v<T, bool>)
            return Scalar::Bool;
        else if constexpr (std::is_same_v<T, unsigned char>)
            return Scalar::Char;
        else if constexpr (std::is_pointer_v<T>)
            return Scalar::Pointer;
        else
            return Scalar::None;
    }

    // A resolved field of type T: where it is in an instance, and how many elements lie there (1 unless the path named
    // an array). Plain data: the audio thread reads and writes through it (Instance::get, set) with no string work.
    template <typename T>
    struct Field
    {
        static constexpr std::size_t kNone = static_cast<std::size_t>(-1);

        std::size_t offset = kNone;   // bytes from the instance's address
        std::size_t count = 0;        // elements from there

        bool valid() const noexcept { return offset != kNone; }
        explicit operator bool() const noexcept { return valid(); }
    };

    class Schema
    {
    public:
        using Attribute = fminus::Program::Attribute;

        // One field of a module.
        struct Node
        {
            std::string name;             // as declared: "size", "cyl"
            std::string type;             // as FMinus writes it: "float", "Cylinder[12]", "Set", "float *", "param int"
            Scalar scalar = Scalar::None; // what one element holds (an array's: its elements')
            bool parameter = false;       // a `param`: `offset` is its value's, and writes go through FMinus's setValue
            std::size_t offset = 0;       // bytes from the start of the module that declares it (a parameter: its value)
            std::size_t size = 0;         // the whole field's bytes (a parameter: its value's)
            std::size_t stride = 0;       // one element's bytes: `size` over the extents' product
            std::vector<std::size_t> extents;     // an array's shape, outermost first; empty when it is no array
            std::vector<Attribute> attributes;    // as written, in order: fminus::range, ui::unit, ...
            std::string module;           // the module its elements are, when they are one ("Cylinder"); else empty
            std::vector<Node> children;   // that module's fields, once (the same for every element of an array)
            // FMinus's description of a parameter (its range, default and slot), the Program's own: valid while the
            // Program lives. nullptr when the field is no parameter.
            const fminus::Program::Parameter* param = nullptr;

            // How many elements the field holds: the extents' product, 1 when it is no array.
            std::size_t count() const noexcept;
            // The last attribute of a name ("ui::unit"), or nullptr.
            const Attribute* attribute(std::string_view attributeName) const noexcept;
        };

        // A path, resolved: the node it names, where that is in an instance of the schema's module, and how many
        // elements lie there (the node's count, less what the path's indexes took).
        struct Handle
        {
            const Node* node = nullptr;
            std::size_t offset = 0;
            std::size_t count = 0;

            Scalar scalar() const noexcept { return node != nullptr ? node->scalar : Scalar::None; }
        };

        // The schema of module `name`. nullptr, with `error` set, when the program has no such module.
        static std::unique_ptr<Schema> of(const fminus::Program& program, std::string_view name, std::string& error);

        const std::string& module() const noexcept { return module_; }
        std::size_t size() const noexcept { return size_; }             // an instance's bytes
        std::size_t alignment() const noexcept { return alignment_; }   // and its alignment
        const std::vector<Node>& fields() const noexcept { return fields_; }   // in declaration order

        // Resolves `path` (above). False, with `error` saying what is wrong and where, when the path names nothing.
        bool find(std::string_view path, Handle& handle, std::string& error) const;

        // The same, as a typed handle: the field's elements must be T's Scalar (float for "float" and "param float",
        // std::int32_t for "int", ...). An invalid Field, with `error` set, when the path names nothing or another type.
        template <typename T>
        Field<T> field(std::string_view path, std::string& error) const
        {
            Handle handle;
            if (!find(path, handle, error))
                return {};
            if (handle.scalar() != scalarOf<T>() || handle.scalar() == Scalar::None)
            {
                error = "'" + std::string(path) + "' is '" + handle.node->type + "' in module " + module_
                        + ", not the type it was asked as";
                return {};
            }
            return {handle.offset, handle.count};
        }

        // Calls `visit` for every field the host can read as numbers, at any depth, in declaration order: its path, its
        // node, and its offset in an instance. An array of modules is unrolled (`cyl[0].p`, `cyl[1].p`, ...); an array
        // of numbers is visited once, as a whole (`fire`, its node's extents saying how many).
        using Visitor = std::function<void(const std::string& path, const Node& node, std::size_t offset)>;
        void forEach(const Visitor& visit) const;

        // Every field as a line of text, nested ones indented: offset, size, type, name, attributes. For a tool's
        // --schema, and for reading a layout by eye.
        std::string describe() const;

    private:
        Schema() = default;

        std::string module_;
        std::size_t size_ = 0;
        std::size_t alignment_ = 1;
        std::vector<Node> fields_;
    };
} // namespace tee
