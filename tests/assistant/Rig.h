// tests/assistant/Rig.h: the application as tee-headless runs it, for the assistant's tests: the simulator playing (one
// compiled program for the whole executable: tests/shell/support/SharedSim.h), its document, the library (shipped:
// content/builds; the owner's: a folder of the case's own under out/assistant/), the inbox drained by the frame, at 60
// frames a second on a simulated clock; the owner scripted.
#pragma once

#include "files/BuildFiles.h"
#include "player/SimAdapter.h"
#include "support/SharedSim.h"
#include "tee/Build.h"
#include "tee/Fields.h"
#include "tee/SimHost.h"
#include "tee/assistant/Tools.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace tee::test
{
    using namespace tee::assistant;
    using nlohmann::json;
    namespace fs = std::filesystem;

    inline std::string readAll(const fs::path& p)
    {
        std::ifstream in(p, std::ios::binary);
        std::stringstream s;
        s << in.rdbuf();
        return s.str();
    }

    // The application as tee-headless has it, without the window: the simulator playing, its document, the library
    // (shipped: content/builds; the owner's: a folder of this case's own), the inbox drained by the frame.
    struct Rig
    {
        static constexpr int kRate = 48000;
        static constexpr int kFrame = kRate / 60;

        std::unique_ptr<player::SimAdapter> sim;
        std::unique_ptr<files::BuildFile> file;
        std::unique_ptr<files::BuildLibrary> library;
        Inbox inbox;
        double clock = 0.0;                     // the simulated clock: frames / 60
        long frames = 0;
        std::vector<float> left = std::vector<float>(kFrame), right = std::vector<float>(kFrame);
        fs::path user;
        // the owner, as a script: each question's answer (std::nullopt: never answered), and what to do before
        // answering, on the frame's thread
        std::vector<std::string> questions;
        std::vector<std::shared_ptr<const std::atomic<bool>>> withdrawals;   // each question's flag, as asked
        std::optional<bool> answer;
        std::function<void()> whileAsked;
        std::vector<std::shared_ptr<std::promise<bool>>> pending;
        std::unique_ptr<Tools> tools;
        std::vector<std::string> replies;       // every reply's text, for the "never a path" property
        std::atomic<bool> othersBusy{false};    // the app's own heavy work (the ribbon's curve, the bake), as a test sets it
        std::function<bool()> busyToo;          // and a real service's busy flag, when a case has one

        explicit Rig(const std::string& name, Limits limits = {})
        {
            std::string errors;
            sim = test::sharedAdapter(errors);
            INFO(errors);
            REQUIRE(sim != nullptr);
            REQUIRE(sim->waitReady(120.0));
            sim->prepare(kRate, kFrame);
            file = std::make_unique<files::BuildFile>(sim->build());
            user = fs::path(TEE_SOURCE_DIR) / "out" / "assistant" / name;
            std::error_code ec;
            fs::remove_all(user, ec);
            fs::create_directories(user);
            library = std::make_unique<files::BuildLibrary>(fs::path(TEE_SOURCE_DIR) / "content" / "builds", user);
            Host host{inbox,
                      *file,
                      *library,
                      [this]() -> SimHost* { return sim->ready() ? &sim->host() : nullptr; },
                      [this]() { return sim->ready() ? sim->host().sim() : std::shared_ptr<Sim>(); },
                      [this]() { return clock; },
                      [this](const std::string& q, std::shared_ptr<const std::atomic<bool>> withdrawn) {
                          questions.push_back(q);
                          withdrawals.push_back(std::move(withdrawn));
                          auto p = std::make_shared<std::promise<bool>>();
                          if (whileAsked)
                              whileAsked();
                          if (answer)
                              p->set_value(*answer);
                          pending.push_back(p);   // never answered: kept, so its future waits
                          return p->get_future();
                      },
                      limits,
                      [this]() { return othersBusy.load() || (busyToo && busyToo()); }};
            tools = std::make_unique<Tools>(host);
            for (int i = 0; i < 6; ++i)
                frame();
        }

        ~Rig()
        {
            inbox.close();
        }

        // One frame: the inbox drained in beforeTick, the adapter's frame, the frame's audio.
        void frame()
        {
            inbox.drain();
            sim->frame();
            sim->process(left.data(), right.data(), kFrame);
            ++frames;
            clock = static_cast<double>(frames) / 60.0;
        }

        // Runs `work` on a worker while the frame runs here, until it is done.
        template <class F>
        auto whileFrames(F work)
        {
            auto f = std::async(std::launch::async, std::move(work));
            while (f.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
            {
                frame();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return f.get();
        }

        Result call(const std::string& name, const json& args = json::object())
        {
            Result r = whileFrames([&] { return tools->call(name, args); });
            replies.push_back(r.text());
            return r;
        }

        Build& build() { return sim->build(); }

        // The first few Spec number fields a tool may set, with a range, at a value inside it that differs from the
        // stored one.
        std::vector<std::pair<const FieldInfo*, double>> settable(std::size_t n)
        {
            std::vector<std::pair<const FieldInfo*, double>> out;
            for (const FieldInfo& f : build().fields().all())
                if (f.block == "spec" && !f.hidden && f.kind == FieldKind::Number && f.hasRange() && *f.maximum > *f.minimum
                    && f.section >= 0 && f.path.rfind("spec.engine.", 0) == 0)
                {
                    const double now = build().get(f.path);
                    const double want = now < (*f.minimum + *f.maximum) / 2.0 ? *f.maximum : *f.minimum;
                    out.push_back({&f, want});
                    if (out.size() == n)
                        break;
                }
            return out;
        }
    };

    inline std::string shortPath(const FieldInfo& f)
    {
        return f.path.substr(5);
    }
}
