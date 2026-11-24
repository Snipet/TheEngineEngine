// tools/tee-validate: builds held to what they should measure as (content/validation/; docs/formats.md, "A
// validation"): runs the dyno and the scenarios each file needs, and prints a table: target, range, measured, pass.
//
//   tee-validate                       every file of content/validation/
//   tee-validate v8 large-diesel       those files (a name, or a path)
//   tee-validate --fast                only the targets marked "fast" (what ctest runs)
//   --sim DIR (sim/ by default), --content DIR (content/ by default)
// Exit 0 when nothing failed (a target the program cannot be measured for yet is "n/a", no failure), 1 otherwise.
#include "common/Paths.h"

#include "tee/Validation.h"

#include <cstdio>

int main(int argc, char** argv)
{
    tee::tools::Arguments args(argc, argv);
    std::string simFlag, contentFlag;
    bool fast = false;
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
        else if (args.is("--help") || args.is("-h"))
        {
            std::printf("usage: tee-validate [--sim DIR] [--content DIR] [--fast] [name-or-file ...]\n");
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
        std::vector<tee::ValidationRow> rows;
        if (!tee::Validation::loadFile(file, validation, errors)
            || !tee::runValidation(compiled, validation, content, fast, rows, errors))
        {
            std::fprintf(stderr, "tee-validate: %s\n", errors.c_str());
            return 1;
        }
        std::printf("%s (%s)\n%s\n", file.stem().generic_string().c_str(), validation.build.c_str(),
                    tee::validationTable(rows).c_str());
        for (const tee::ValidationRow& r : rows)
            failed = failed || (r.measured && !r.pass);
    }
    return failed ? 1 : 0;
}
