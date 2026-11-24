// tools/tee-validate: builds held to what they should measure as (content/validation/; docs/formats.md, "A
// validation"): runs the dyno and the scenarios each file needs, and prints a table: target, range, measured, pass.
//
//   tee-validate                       every file of content/validation/
//   tee-validate v8 large-diesel       those files (a name, or a path)
//   tee-validate --fast                only the targets marked "fast" (what ctest runs)
//   --sim DIR (sim/ by default), --content DIR (content/ by default)
// Each file is run twice: on the build with every model switch at the reference's setting (tee::referenceSettings), and
// on the build as it is (the simulator at its defaults); the table shows both, then which targets each misses. A
// target the reference misses is reported, not failed: the reference is the baseline, and the packages bring the
// simulator inside. Exit 0 unless the simulator misses a target the reference meets, or a fast target fails (1);
// a target the program cannot be measured for yet is "n/a", no failure. --simulator: the simulator's column only.
#include "common/Paths.h"

#include "tee/Validation.h"

#include <cstdio>
#include <functional>

int main(int argc, char** argv)
{
    tee::tools::Arguments args(argc, argv);
    std::string simFlag, contentFlag;
    bool fast = false, simulatorOnly = false;
    std::vector<std::string> names;
    while (!args.done())
    {
        if (args.is("--sim"))
        {
            if (!args.value(simFlag))
                return 2;
        }
        else if (args.is("--content"))
        {
            if (!args.value(contentFlag))
                return 2;
        }
        else if (args.is("--fast"))
            fast = true;
        else if (args.is("--simulator"))
            simulatorOnly = true;
        else if (args.is("--help") || args.is("-h"))
        {
            std::printf("usage: tee-validate [--sim DIR] [--content DIR] [--fast] [--simulator] [name-or-file ...]\n");
            return 0;
        }
        else
            names.push_back(args.take());
    }
    const std::filesystem::path sim =
        tee::tools::findDirectory(simFlag, "TEE_SIM_DIR", argv[0], "sim", TEE_SOURCE_SIM_DIR "/..");
    const std::filesystem::path content =
        tee::tools::findDirectory(contentFlag, "TEE_CONTENT_DIR", argv[0], "content", TEE_SOURCE_CONTENT_DIR "/..");
    if (names.empty())
        names = tee::tools::namesIn(content / "validation");
    std::string errors;
    auto compiled = tee::Sim::compile(sim, {}, errors);
    if (!compiled)
    {
        std::fprintf(stderr, "%s", errors.c_str());
        return 1;
    }
    bool failed = false;
    for (const std::string& name : names)
    {
        const std::filesystem::path file = tee::tools::namedFile(content / "validation", name);
        tee::Validation validation;
        if (!tee::Validation::loadFile(file, validation, errors))
        {
            std::fprintf(stderr, "tee-validate: %s\n", errors.c_str());
            return 1;
        }
        // Three columns: every model off (the reference: tee::referenceSettings), the build as it is, every new model
        // on (tee::modelsOnSettings); a column whose build is one already run is not run again (its title says so).
        // --simulator: the build as it is only.
        struct Column
        {
            std::string title;
            std::function<int(tee::Build&)> adjust;
        };
        std::vector<Column> wanted;
        if (simulatorOnly)
            wanted.push_back({"as built", {}});
        else
            wanted = {{"models off", [](tee::Build& b) { return tee::referenceSettings(b); }},
                      {"as built", {}},
                      {"models on", [](tee::Build& b) { return tee::modelsOnSettings(b); }}};
        // the build each column runs: loaded here as runValidation loads it, to tell which are the same
        std::vector<std::uint64_t> hashes;
        std::vector<std::string> titles;
        std::vector<std::vector<tee::ValidationRow>> columns;
        for (const Column& c : wanted)
        {
            tee::Build probe(compiled->fields());
            std::vector<std::string> warnings;
            const std::filesystem::path buildFile =
                validation.build.find('/') != std::string::npos || validation.build.find(".json") != std::string::npos
                    ? (std::filesystem::exists(validation.build) ? std::filesystem::path(validation.build)
                                                                 : content / validation.build)
                    : content / "builds" / (validation.build + ".json");
            if (!validation.build.empty() && !probe.loadFile(buildFile, tee::BuildPart::Whole, warnings, errors))
            {
                std::fprintf(stderr, "tee-validate: %s\n", errors.c_str());
                return 1;
            }
            if (c.adjust)
                c.adjust(probe);
            const std::uint64_t h = probe.hash();
            bool seen = false;
            for (std::size_t k = 0; k < hashes.size(); ++k)
                if (hashes[k] == h)
                {
                    titles[k] += " = " + c.title;
                    seen = true;
                }
            if (seen)
                continue;
            std::vector<tee::ValidationRow> columnRows;
            if (!tee::runValidation(compiled, validation, content, fast, columnRows, errors,
                                    c.adjust ? std::function<void(tee::Build&)>([&](tee::Build& b) { c.adjust(b); })
                                             : std::function<void(tee::Build&)>()))
            {
                std::fprintf(stderr, "tee-validate: %s\n", errors.c_str());
                return 1;
            }
            hashes.push_back(h);
            titles.push_back(c.title);
            columns.push_back(std::move(columnRows));
        }
        const std::string title = file.stem().generic_string() + " (" + validation.build + ")";
        std::printf("%s\n%s\n", title.c_str(), tee::validationColumns(titles, columns).c_str());
        // a fast target that fails in any column fails the run; a column's miss of a target the reference (the first
        // column) meets is a regression and fails it too; the rest is reported
        for (std::size_t c = 0; c < columns.size(); ++c)
            for (std::size_t i = 0; i < columns[c].size(); ++i)
            {
                const tee::ValidationRow& r = columns[c][i];
                const bool referenceMeets =
                    c > 0 && i < columns[0].size() && columns[0][i].measured && columns[0][i].pass && !simulatorOnly;
                failed = failed || (r.measured && !r.pass && (r.target->fast || referenceMeets));
            }
    }
    return failed ? 1 : 0;
}
