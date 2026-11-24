// host/include/tee/Reload.h: reloading the simulator while it plays, for development (card C, section 4).
//
// carryState copies a running instance's state into an instance of an edited program, by field name and type, as
// FMinus's desktop experiment does (docs/research/desktop/experiments/host.cpp): a field of the same name and the same
// type takes the old value; a nested module is carried field by field (each element of an array of modules alike); a
// field the edit added keeps what the new instance has (0, or its default); an array whose size changed is a field of
// another type, so it starts afresh; a pointer is never carried (the new instance's init took its own memory).
//
// Reloader watches a directory (the simulator's files' times and sizes, looked at every quarter second), compiles on a
// thread of its own when they change, and hands a program that compiled to SimHost::reload on the caller's thread
// (poll()); one that did not keeps the old one playing and keeps clang's words for whoever shows them. It runs only in
// development: when the environment says TEE_DEV=1 (devMode()), and never in a release unless asked for.
#pragma once

#include "tee/Schema.h"
#include "tee/Sim.h"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace tee
{
    class SimHost;

    // How many fields were carried.
    std::size_t carryState(const Schema& from, const void* source, const Schema& to, void* destination);
    // The same for one field of each (a block: "spec"), found by name in both.
    std::size_t carryField(const Schema& from, const void* source, const Schema& to, void* destination,
                           std::string_view name);

    // TEE_DEV is set to something other than 0 or empty.
    bool devMode() noexcept;

    class Reloader
    {
    public:
        // Watches `directory`, compiling with `options` (the host's own, usually). Starts its thread at once.
        Reloader(std::filesystem::path directory, Sim::Options options);
        ~Reloader();
        Reloader(const Reloader&) = delete;
        Reloader& operator=(const Reloader&) = delete;

        // On the thread that owns `host` (the UI thread): when a compile has finished since the last call, reloads
        // the host with it (true), or keeps the errors (false, errors() says them). False when nothing happened.
        bool poll(SimHost& host);
        // Clang's words for the newest compile that failed (or a reload the host refused); empty after one succeeds.
        std::string errors() const;
        // Compiles that finished, either way.
        int compiles() const noexcept { return compiles_.load(); }

    private:
        void loop();

        std::filesystem::path directory_;
        Sim::Options options_;
        std::thread thread_;
        std::atomic<bool> stop_{false};
        mutable std::mutex mutex_;
        std::shared_ptr<Sim> ready_;
        std::string errors_;
        std::atomic<int> compiles_{0};
    };
} // namespace tee
