// host/include/tee/Program.h: an FMinus program, compiled from a directory of files (or from files held in memory).
//
// FMinus compiles a program from one string, with a callback that finds the file of each #include. Here the files
// are a directory's: the entry file is named, and an #include "name" reads the file of that name beside it. What comes
// back is FMinus's own fminus::Program (fminus/FMinus.h, the only FMinus header the host includes), with what a host
// wants to know about the compile kept beside it: the messages as FMinus words them, and how long it took.
//
//   std::string errors;
//   auto program = tee::Program::fromDirectory("sim/ref", "program.fm", {}, errors);
//   if (!program)
//       return report(errors);                       // "engine.fm:412:9: error: ...", as clang words it
//   auto* init = program->function<void(void*, float)>("Shared::init", errors);
//
// Every program is compiled with subnormals flushed to zero (fminus::Program::Denormals::Flush), the mode an audio
// thread runs in: call what is not an entry point of the program's (its init, its end) inside a ScopedFlushDenormals
// (Denormals.h), as RefPlayer does.
//
// Threads: compiling, link() and the first function() of a name allocate and take milliseconds to seconds: off the
// audio thread. A pointer function() gave is valid while the Program lives.
#pragma once

#include "fminus/FMinus.h"

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    class Program
    {
    public:
        struct Options
        {
            int optimisation = 2;    // -O0 to -O3: LLVM's pipeline at that level
            // A development build's two checks, together: the loop budget (a call that loops past the budget the host
            // set returns early) and the bounds checks (an index outside its array stops the call before the access).
            // Off, the code is as fast as it gets; on, the host sets a budget before each call and reads both flags
            // after it (RefPlayer does).
            bool checks = false;
            // Text compiled after the entry file's last line, as if it ended with it: functions of the host's own, such
            // as the [[clang::nonblocking]] functions RefPlayer calls the reference through, which make FMinus prove
            // that what they call cannot allocate.
            std::string appended;
            // Macros defined before the program's first line, as C's -D (fminus::Program::Options::defines).
            std::map<std::string, std::string> defines;
        };

        // A program's files in memory: each name, as an #include writes it, and its text.
        using Files = std::map<std::string, std::string>;

        // Compiles `entry`, a file of `directory`; an #include reads the file of its name in the same directory (a
        // name with a directory in it, "core/crank.fm", the file there). nullptr when it does not compile, or a file
        // cannot be read, with `errors` holding FMinus's messages, one a line, as clang words them.
        static std::unique_ptr<Program> fromDirectory(const std::filesystem::path& directory, std::string_view entry,
                                                      const Options& options, std::string& errors);

        // The same from files in memory (a test's, or an edited copy).
        static std::unique_ptr<Program> fromFiles(Files files, std::string_view entry, const Options& options,
                                                  std::string& errors);

        ~Program();
        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;

        // FMinus's program: its modules and their fields, its parameters, its functions.
        fminus::Program& fminus() noexcept { return *program_; }
        const fminus::Program& fminus() const noexcept { return *program_; }

        const Options& options() const noexcept { return options_; }
        const std::string& entry() const noexcept { return entry_; }

        // The compile's warnings, as FMinus words them, one a line with clang's notes; empty when there are none.
        const std::string& warnings() const noexcept { return program_->warnings(); }

        // The names of the files the compile read, the entry first, each once, in the order they were first read.
        const std::vector<std::string>& filesRead() const noexcept { return filesRead_; }

        // Seconds fminus::Program::compile took: the front end, and with the checks on the optimiser and the code
        // generator too (FMinus compiles those programs whole).
        double compileSeconds() const noexcept { return compileSeconds_; }

        // Generates the program's machine code now, if no function has been looked up yet (FMinus runs the optimiser
        // and the code generator at the first look-up otherwise, wherever that happens). False, with `errors` set,
        // when the code does not link. linkSeconds() is how long that took, and codeBytes() the machine code's size.
        bool link(std::string& errors);
        double linkSeconds() const noexcept { return linkSeconds_; }
        std::size_t codeBytes() const noexcept { return codeBytes_; }

        // The function `name` ("Shared::init", "tee_ref_process"), as fminus::Program::function gives it: the
        // Signature must be the declaration's C type, and nothing checks that it is. nullptr, with `errors` set, when
        // the program has none of that name.
        template <typename Signature>
        Signature* function(std::string_view name, std::string& errors)
        {
            if (!linked_ && !link(errors))
                return nullptr;
            return program_->function<Signature>(name, errors);
        }

    private:
        Program() = default;

        std::unique_ptr<fminus::Program> program_;
        Options options_;
        std::string entry_;
        std::vector<std::string> filesRead_;
        double compileSeconds_ = 0.0;
        double linkSeconds_ = 0.0;
        std::size_t codeBytes_ = 0;
        bool linked_ = false;
    };
} // namespace tee
