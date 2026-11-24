// tools/common/Paths.h: where a tool finds the simulator's files and the content, and how it reads its command line.
#pragma once

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace tee::tools
{
    // A directory by, in order: the command line's flag; the environment variable; the checkout this was built from,
    // while it is there (so a developer's edit is read without a build copying it anywhere); `beside` in the
    // executable's own directory (where an unpacked CI artefact has sim/ and content/, as the application looks for
    // them: app/Engine.h) or one above it (bin/ beside sim/ and content/).
    inline std::filesystem::path findDirectory(const std::string& flag, const char* variable, const char* argv0,
                                               const char* beside, const char* source)
    {
        if (!flag.empty())
            return flag;
        if (const char* env = std::getenv(variable); env != nullptr && *env != '\0')
            return env;
        std::error_code ec;
        if (std::filesystem::is_directory(source, ec))
            return source;
        const std::filesystem::path exe = std::filesystem::weakly_canonical(std::filesystem::path(argv0), ec);
        if (!ec && exe.has_parent_path())
        {
            for (const std::filesystem::path& holder : {exe.parent_path(), exe.parent_path().parent_path()})
            {
                const std::filesystem::path sibling = holder / beside;
                if (std::filesystem::is_directory(sibling, ec))
                    return sibling;
            }
        }
        return source;
    }

    // "v8" is <directory>/v8.json; anything with a slash or ending in .json is the file itself.
    inline std::filesystem::path namedFile(const std::filesystem::path& directory, const std::string& name)
    {
        const bool isFile = name.find('/') != std::string::npos || name.find('\\') != std::string::npos
                            || (name.size() > 5 && name.compare(name.size() - 5, 5, ".json") == 0);
        return isFile ? std::filesystem::path(name) : directory / (name + ".json");
    }

    // The .json files of a directory, by name without the extension, sorted.
    inline std::vector<std::string> namesIn(const std::filesystem::path& directory)
    {
        std::vector<std::string> names;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
            if (entry.path().extension() == ".json")
                names.push_back(entry.path().stem().generic_string());
        std::sort(names.begin(), names.end());
        return names;
    }

    // A command line read left to right: `--name value` and `--name`.
    class Arguments
    {
    public:
        Arguments(int argc, char** argv) : args_(argv + 1, argv + argc) {}

        bool done() const { return at_ >= args_.size(); }
        const std::string& peek() const { return args_[at_]; }
        std::string take() { return args_[at_++]; }
        // Whether the next argument is `name`; then it is taken.
        bool is(const char* name)
        {
            if (done() || args_[at_] != name)
                return false;
            ++at_;
            return true;
        }
        // The value after an option just taken; false when the line ends there.
        bool value(std::string& out)
        {
            if (done())
                return false;
            out = args_[at_++];
            return true;
        }

    private:
        std::vector<std::string> args_;
        std::size_t at_ = 0;
    };
} // namespace tee::tools
