// host/include/tee/Sim.h: the simulator compiled, ready to make instances of: the program, its main module's Schema
// and Fields, and the three methods of 02-contracts.md, section 1, by their C types. Several SimHosts may share one
// (the one that plays and a worker's), each with an instance of its own.
//
//   std::string errors;
//   auto sim = tee::Sim::compile("sim", {}, errors);       // sim/engine.fm, module Engine
//   if (!sim) report(errors);                              // clang's words
//
// One thing a shared Sim cannot share: FMinus keeps one loop budget and one bounds record a Program, so a Sim compiled
// with the checks (Options::checks) belongs to one SimHost. SimHost::attach refuses a second.
//
// Threads: compile() allocates and takes seconds: off the audio thread. The rest is read-only once made.
#pragma once

#include "tee/Fields.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace tee
{
    class Sim
    {
    public:
        struct Options
        {
            std::string entry = "engine.fm";   // the program's entry file, in its directory
            std::string module = "Engine";     // its main module
            int optimisation = 2;
            // The loop budget and the bounds checks (Program::Options::checks): a development build's. A call they
            // stop is handled as the output guard handles a blast, with FMinus's message kept (SimHost::stopReason).
            bool checks = false;
            std::map<std::string, std::string> defines;
        };

        using InitFn = void(void*, float);
        using ProcessFn = void(void*, float*, float*, int);
        using EndFn = void(void*);

        // Compiles `options.entry` in `directory`. nullptr, with `errors` holding FMinus's messages or what the program
        // lacks of the contract: the main module, its four blocks, init(float), end() and a process that is
        // [[clang::nonblocking]].
        static std::shared_ptr<Sim> compile(const std::filesystem::path& directory, const Options& options,
                                            std::string& errors);
        static std::shared_ptr<Sim> compileFiles(Program::Files files, const Options& options, std::string& errors);

        ~Sim();
        Sim(const Sim&) = delete;
        Sim& operator=(const Sim&) = delete;

        Program& program() const noexcept { return *program_; }
        const Schema& schema() const noexcept { return *schema_; }
        const Fields& fields() const noexcept { return *fields_; }
        const fminus::Program::Module& module() const noexcept { return *module_; }
        const Options& options() const noexcept { return options_; }
        const std::filesystem::path& directory() const noexcept { return directory_; }
        // A hash of the program as compiled: every source file's name and text (the directory's .fm files, or the
        // files given), the entry, the module and the defines (not the optimisation or the checks: the same program).
        // What a calibration baked on it (Bake.h) is keyed by, so a bake made on another program is known as stale.
        std::uint64_t sourceHash() const noexcept { return sourceHash_; }

        InitFn* init() const noexcept { return init_; }
        ProcessFn* process() const noexcept { return process_; }
        EndFn* end() const noexcept { return end_; }

        // Writes every Spec field's fminus::default into `instance` (an instance of the main module), every element
        // of an array alike; tables and revs are left at 0. FMinus initialises nothing (02-contracts.md, section 3).
        void writeDefaults(void* instance) const noexcept;

        // Claimed by a SimHost that runs the checks (above): false when another holds it.
        bool claim() const noexcept;
        void release() const noexcept;

    private:
        Sim() = default;
        bool bind(std::string& errors);

        std::unique_ptr<Program> program_;
        std::unique_ptr<Schema> schema_;
        std::unique_ptr<Fields> fields_;
        const fminus::Program::Module* module_ = nullptr;
        Options options_;
        std::filesystem::path directory_;
        InitFn* init_ = nullptr;
        ProcessFn* process_ = nullptr;
        EndFn* end_ = nullptr;
        std::uint64_t sourceHash_ = 0;
        mutable std::atomic<bool> claimed_{false};
    };
} // namespace tee
