// host/include/tee/test/Kept.h: an object made once a process and never destroyed, for a test's compiled programs.
//
// 01-rules.md, section 4 (found on macOS): nothing that owns a tee::Program (an EnginePlayer, a Sim, a rig) may live in
// a static with a destructor: at exit it is destroyed after FMinus's and LLVM's own statics, on a JIT already gone. A
// test that wants a program compiled once keeps it here instead:
//
//     static std::unique_ptr<tee::Program> p = [] { ... }();          // before: destroyed at exit
//     tee::Program& p = tee::test::kept([] { return ...; });           // after: made once, never destroyed
//
// `make` returns a std::unique_ptr<T> (or a T* it hands over); it runs once for each call site (each lambda is its own
// type), under the C++ guarantee that a function-local static is made once whatever the threads. A null result is kept
// as null: kept() then returns a null pointer through keptPointer(). For programs by a key (a map of them), keep the
// map itself: `static auto& programs = *new std::map<std::string, std::unique_ptr<Program>>();`.
// tee::test::compiled() (Scenario.h) is the same for the simulator's Sim, cached by directory and options.
#pragma once

#include <memory>
#include <type_traits>

namespace tee::test
{
    template <typename Make>
    auto* keptPointer(Make make)
    {
        using Made = decltype(make());
        if constexpr (std::is_pointer_v<Made>)
        {
            static auto* const object = make();
            return object;
        }
        else
        {
            static auto* const object = make().release();
            return object;
        }
    }

    template <typename Make>
    auto& kept(Make make)
    {
        return *keptPointer(make);
    }
} // namespace tee::test
