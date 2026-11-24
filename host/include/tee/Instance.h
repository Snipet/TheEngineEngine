// host/include/tee/Instance.h: the memory of one instance of an FMinus module, and typed access to its fields.
//
// FMinus runs no constructor: an instance is the module's size in bytes, aligned as the module says and zeroed, with
// each parameter set to its fminus::default. This class makes that, frees it, and reads and writes fields in it:
//
//   by a resolved handle (Schema::field<T>): a load or a store, for the audio thread;
//   by a path: resolved each time, with the value converted to and from a double, for tools, tests and files.
//
// It runs none of the module's methods: a module with an init wants it called after this, and its end before this
// goes (RefPlayer does both for the reference's modules).
//
// A `param` is written through FMinus's setValue (an atomic store a method reads when it next starts), so any thread
// may write one while the audio thread runs. Any other field is plain memory: write it from the thread that calls the
// module's methods, between two calls.
#pragma once

#include "tee/Schema.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>

namespace tee
{
    class Instance
    {
    public:
        // An instance of `module` (fminus::Program::module()'s): zeroed, its parameters at their defaults. The module
        // is the Program's own and must outlive the instance.
        explicit Instance(const fminus::Program::Module& module);
        ~Instance();
        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        void* data() noexcept { return bytes_; }
        const void* data() const noexcept { return bytes_; }
        std::size_t size() const noexcept { return size_; }
        const fminus::Program::Module& module() const noexcept { return *module_; }

        // As it was made: every byte 0, then the parameters' defaults. What the module's init had taken is not given
        // back by this: call its end first.
        void reset() noexcept;

        // ---- by handle: no string work, no allocation -----------------------------------------------------------
        template <typename T>
        T get(Field<T> field, std::size_t index = 0) const noexcept
        {
            T value;
            std::memcpy(&value, bytes_ + field.offset + index * sizeof(T), sizeof(T));
            return value;
        }

        template <typename T>
        void set(Field<T> field, T value, std::size_t index = 0) noexcept
        {
            std::memcpy(bytes_ + field.offset + index * sizeof(T), &value, sizeof(T));
        }

        // A handle's element as a double, whatever number it holds (a bool 0 or 1; a pointer or a module 0).
        double read(const Schema::Handle& handle, std::size_t index = 0) const noexcept;
        // Writes one: a float rounded, an int truncated toward 0, a bool true unless 0; a parameter through FMinus's
        // setValue. A pointer or a module is not written.
        void write(const Schema::Handle& handle, double value, std::size_t index = 0) noexcept;

        // ---- by path: resolved each time ------------------------------------------------------------------------
        // False, with `error` set, when the path names nothing in `schema` or nothing the host reads as a number.
        bool read(const Schema& schema, std::string_view path, double& value, std::string& error) const;
        bool write(const Schema& schema, std::string_view path, double value, std::string& error);

    private:
        const fminus::Program::Module* module_;
        unsigned char* bytes_ = nullptr;
        std::size_t size_ = 0;
        std::size_t alignment_ = 0;
    };
} // namespace tee
