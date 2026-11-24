// host/src/Program.cpp: tee::Program (Program.h).
#include "tee/Program.h"

#include <chrono>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <utility>

namespace tee
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        double secondsSince(Clock::time_point start)
        {
            return std::chrono::duration<double>(Clock::now() - start).count();
        }

        // A file's name as the program knows it: relative to the program's root, with forward slashes, no "." and
        // no ".." left in it. Empty when the name leaves the root or is absolute: such a file is never read.
        std::string normalised(const std::filesystem::path& name)
        {
            if (name.empty() || name.is_absolute() || name.has_root_name())
                return {};
            const std::filesystem::path clean = name.lexically_normal();
            if (clean.empty() || *clean.begin() == "..")
                return {};
            return clean.generic_string();
        }

        std::optional<std::string> readWhole(const std::filesystem::path& path)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in)
                return std::nullopt;
            std::ostringstream text;
            text << in.rdbuf();
            if (in.bad())
                return std::nullopt;
            return text.str();
        }
    } // namespace

    Program::~Program() = default;

    namespace
    {
        // The compile itself, over whatever holds the files: `read` gives a file's text by its normalised name.
        using Reader = std::function<std::optional<std::string>(const std::string& name)>;
    } // namespace

    static std::unique_ptr<fminus::Program> compileWith(const Reader& read, const std::string& entry,
                                                        const Program::Options& options, std::string& errors,
                                                        std::vector<std::string>& filesRead, double& seconds)
    {
        errors.clear();
        filesRead.clear();
        std::optional<std::string> source = read(entry);
        if (!source)
        {
            errors = entry + ": error: the program's entry file cannot be read\n";
            return nullptr;
        }
        filesRead.push_back(entry);
        if (!options.appended.empty())
        {
            if (!source->empty() && source->back() != '\n')
                *source += '\n';
            *source += options.appended;
        }

        fminus::Program::Options o;
        o.optimisation = options.optimisation;
        o.fileName = entry;
        o.defines = options.defines;
        o.denormals = fminus::Program::Denormals::Flush;
        o.loopBudget = options.checks;
        o.boundsCheck = options.checks;
        // #include "name": beside the file that includes it first, then from the program's root, as a C compiler
        // looks beside the including file and then along its include path.
        o.include = [&read, &filesRead](std::string_view name,
                                        std::string_view from) -> std::optional<fminus::Program::IncludedFile> {
            const std::filesystem::path wanted{std::string(name)};
            const std::filesystem::path beside = std::filesystem::path{std::string(from)}.parent_path() / wanted;
            for (const std::filesystem::path& candidate : {beside, wanted})
            {
                const std::string file = normalised(candidate);
                if (file.empty())
                    continue;
                if (std::optional<std::string> text = read(file))
                {
                    bool seen = false;
                    for (const std::string& had : filesRead)
                        seen = seen || had == file;
                    if (!seen)
                        filesRead.push_back(file);
                    return fminus::Program::IncludedFile{file, std::move(*text)};
                }
            }
            return std::nullopt;
        };

        const Clock::time_point start = Clock::now();
        std::unique_ptr<fminus::Program> program = fminus::Program::compile(*source, errors, o);
        seconds = secondsSince(start);
        return program;
    }

    std::unique_ptr<Program> Program::fromDirectory(const std::filesystem::path& directory, std::string_view entry,
                                                    const Options& options, std::string& errors)
    {
        const std::string entryName = normalised(std::filesystem::path{std::string(entry)});
        if (entryName.empty())
        {
            errors = std::string(entry) + ": error: the entry file is named inside the program's directory\n";
            return nullptr;
        }
        std::error_code ec;
        if (!std::filesystem::is_directory(directory, ec))
        {
            errors = directory.generic_string() + ": error: no such directory\n";
            return nullptr;
        }
        const Reader read = [&directory](const std::string& name) { return readWhole(directory / name); };

        std::unique_ptr<Program> out(new Program());
        out->options_ = options;
        out->entry_ = entryName;
        out->program_ = compileWith(read, entryName, options, errors, out->filesRead_, out->compileSeconds_);
        if (!out->program_)
            return nullptr;
        return out;
    }

    std::unique_ptr<Program> Program::fromFiles(Files files, std::string_view entry, const Options& options,
                                                std::string& errors)
    {
        const std::string entryName = normalised(std::filesystem::path{std::string(entry)});
        if (entryName.empty())
        {
            errors = std::string(entry) + ": error: the entry file is named among the program's files\n";
            return nullptr;
        }
        const Reader read = [&files](const std::string& name) -> std::optional<std::string> {
            const auto found = files.find(name);
            if (found == files.end())
                return std::nullopt;
            return found->second;
        };

        std::unique_ptr<Program> out(new Program());
        out->options_ = options;
        out->entry_ = entryName;
        out->program_ = compileWith(read, entryName, options, errors, out->filesRead_, out->compileSeconds_);
        if (!out->program_)
            return nullptr;
        return out;
    }

    bool Program::link(std::string& errors)
    {
        if (linked_)
            return true;
        const Clock::time_point start = Clock::now();
        const std::vector<fminus::Program::CodeRange> ranges = program_->codeRanges(errors);
        linkSeconds_ = secondsSince(start);
        if (ranges.empty())
        {
            if (errors.empty())
                errors = entry_ + ": error: the program has no machine code\n";
            return false;
        }
        codeBytes_ = 0;
        for (const fminus::Program::CodeRange& range : ranges)
            codeBytes_ += range.size;
        linked_ = true;
        return true;
    }
} // namespace tee
