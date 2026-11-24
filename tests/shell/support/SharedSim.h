// tests/shell/support/SharedSim.h: one compiled simulator (sim/engine.fm) for a test executable's cases, so that a case
// that needs one costs a SimAdapter (a tenth of a second) and not a compile (seconds). Each case still gets an adapter,
// a host and a document of its own: only the machine code is shared.
//
// It lives in a static, freed by a Catch listener when the run ends, before main returns: a tee::Program must never be
// destroyed by a static's destructor, after FMinus's and LLVM's own statics (docs/plan/01-rules.md, "Found on macOS").
#pragma once

#include "player/SimAdapter.h"
#include "tee/Sim.h"

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace tee::test
{
    inline std::shared_ptr<Sim>& sharedSimSlot()
    {
        static std::shared_ptr<Sim> sim;   // emptied by SharedSimListener before main returns
        return sim;
    }

    // The simulator of TEE_SOURCE_SIM_DIR compiled once (null, with `errors`, when it does not compile).
    inline std::shared_ptr<Sim> sharedSim(std::string& errors)
    {
        std::shared_ptr<Sim>& slot = sharedSimSlot();
        if (!slot)
            slot = Sim::compile(std::filesystem::path(TEE_SOURCE_SIM_DIR), Sim::Options{}, errors);
        return slot;
    }

    // An adapter over the shared program (Options as a case wants them; `directory` and `sim` are set here).
    inline std::unique_ptr<player::SimAdapter> sharedAdapter(std::string& errors, player::SimAdapter::Options o = {})
    {
        o.directory = std::filesystem::path(TEE_SOURCE_SIM_DIR);
        o.sim = sharedSim(errors);
        if (!o.sim)
            return nullptr;
        return player::SimAdapter::create(o, errors);
    }

    class SharedSimListener final : public Catch::EventListenerBase
    {
    public:
        using Catch::EventListenerBase::EventListenerBase;
        void testRunEnded(const Catch::TestRunStats&) override { sharedSimSlot().reset(); }
    };
}

// (registered once a test executable: in the one .cpp that defines TEE_REGISTER_SHARED_SIM before including this)
#if defined(TEE_REGISTER_SHARED_SIM)
CATCH_REGISTER_LISTENER(tee::test::SharedSimListener)
#endif
