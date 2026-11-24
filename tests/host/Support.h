// tests/host/Support.h: what the host's tests share: the stand-in simulator, a host on it, the allocation counter.
#pragma once

#include "tee/Build.h"
#include "tee/SimHost.h"
#include "tee/test/Scenario.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace tee::test
{
    // Allocations (operator new) made on a thread while it counts them (Alloc.cpp).
    struct AllocationCounter
    {
        static void countThisThread(bool on) noexcept;
        static long count() noexcept;
    };

    inline std::filesystem::path standin()
    {
        return TEE_STANDIN_DIR;
    }

    inline std::shared_ptr<Sim> standinSim(bool checks = false)
    {
        Sim::Options options;
        options.checks = checks;
        std::string errors;
        auto sim = compiled(standin(), options, errors);
        INFO(errors);
        REQUIRE(sim != nullptr);
        return sim;
    }

    // The stand-in's files in memory, for a test that edits one.
    inline Program::Files standinFiles()
    {
        Program::Files files;
        for (const auto& entry : std::filesystem::directory_iterator(standin()))
        {
            std::ifstream in(entry.path(), std::ios::binary);
            std::ostringstream text;
            text << in.rdbuf();
            files[entry.path().filename().generic_string()] = text.str();
        }
        return files;
    }

    inline std::string replaced(std::string text, const std::string& from, const std::string& to)
    {
        const std::size_t at = text.find(from);
        REQUIRE(at != std::string::npos);
        return text.replace(at, from.size(), to);
    }

    inline LoadOptions offline(double rate = 48000.0, int block = 512)
    {
        LoadOptions o;
        o.sampleRate = rate;
        o.maxBlock = block;
        o.worker = false;
        return o;
    }

    inline FieldRef ref(const SimHost& host, const std::string& path)
    {
        std::string error;
        FieldRef r = host.fields().ref(path, error);
        INFO(error);
        REQUIRE(r.valid());
        return r;
    }

    inline double tel(const SimHost& host, const TelemetryFrame& frame, const std::string& path)
    {
        const FieldInfo* f = host.fields().find(path);
        REQUIRE(f != nullptr);
        return frame.get(*f);
    }
} // namespace tee::test
