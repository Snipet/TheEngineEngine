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
        std::vector<tee::ValidationRow> rows, reference;
        if (!tee::Validation::loadFile(file, validation, errors))
        {
            std::fprintf(stderr, "tee-validate: %s\n", errors.c_str());
            return 1;
        }
        // the reference's column (unless --simulator): the build with every model switch at the reference's setting;
        // when that changes nothing, the simulator's column is the same run
        bool differs = simulatorOnly;
        if (!simulatorOnly
            && !tee::runValidation(compiled, validation, content, fast, reference, errors,
                                   [&](tee::Build& b) { differs = tee::referenceSettings(b) > 0; }))
        {
            std::fprintf(stderr, "tee-validate: %s\n", errors.c_str());
            return 1;
        }
        if (differs && !tee::runValidation(compiled, validation, content, fast, rows, errors))
        {
            std::fprintf(stderr, "tee-validate: %s\n", errors.c_str());
            return 1;
        }
        if (!differs)
            rows = reference;
        const std::string title = file.stem().generic_string() + " (" + validation.build + ")";
        if (simulatorOnly)
            std::printf("%s\n%s\n", title.c_str(), tee::validationTable(rows).c_str());
        else
            std::printf("%s\n%s\n", title.c_str(), tee::validationComparison(reference, rows, !differs).c_str());
        for (std::size_t i = 0; i < rows.size(); ++i)
        {
            const tee::ValidationRow& r = rows[i];
            const bool referenceMeets = !simulatorOnly && i < reference.size() && reference[i].measured && reference[i].pass;
            failed = failed || (r.measured && !r.pass && (r.target->fast || referenceMeets));
        }
    }
    return failed ? 1 : 0;
}
