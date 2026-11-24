// tools/tee-schema: the simulator's Spec, Controls, Telemetry and Scope as the host reads them (tee::Fields), printed
// as JSON, as the snapshot the schema test holds the Spec to, or as docs/parameters.md; and the contract's lint.
//
//   tee-schema                          JSON of every field, from sim/engine.fm
//   tee-schema --lint                   the lint's problems, one a line; exit 1 when there are any
//   tee-schema --snapshot [--out F]     tests/host/schema.snapshot's text
//   tee-schema --markdown [--out F]     docs/parameters.md
//   --sim DIR, --entry FILE, --module NAME: another program (a test's stand-in)
#include "common/Paths.h"

#include "tee/Fields.h"
#include "tee/Sim.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
    int usage()
    {
        std::fprintf(stderr,
                     "usage: tee-schema [--sim DIR] [--entry FILE] [--module NAME] [--json | --snapshot | --markdown | "
                     "--lint] [--out FILE]\n");
        return 2;
    }
} // namespace

int main(int argc, char** argv)
{
    tee::tools::Arguments args(argc, argv);
    std::string simFlag, out, mode = "json";
    tee::Sim::Options options;
    while (!args.done())
    {
        if (args.is("--sim"))
        {
            if (!args.value(simFlag))
                return usage();
        }
        else if (args.is("--entry"))
        {
            if (!args.value(options.entry))
                return usage();
        }
        else if (args.is("--module"))
        {
            if (!args.value(options.module))
                return usage();
        }
        else if (args.is("--out"))
        {
            if (!args.value(out))
                return usage();
        }
        else if (args.is("--json"))
            mode = "json";
        else if (args.is("--snapshot"))
            mode = "snapshot";
        else if (args.is("--markdown"))
            mode = "markdown";
        else if (args.is("--lint"))
            mode = "lint";
        else if (args.is("--help") || args.is("-h"))
        {
            usage();
            return 0;
        }
        else
            return usage();
    }
    const std::filesystem::path sim =
        tee::tools::findDirectory(simFlag, "TEE_SIM_DIR", argv[0], "sim", TEE_SOURCE_SIM_DIR "/..");
    std::string errors;
    auto compiled = tee::Sim::compile(sim, options, errors);
    if (!compiled)
    {
        std::fprintf(stderr, "%s", errors.c_str());
        return 1;
    }
    const tee::Fields& fields = compiled->fields();
    std::string text;
    int status = 0;
    if (mode == "json")
        text = fields.json();
    else if (mode == "snapshot")
        text = "# tests/host/schema.snapshot: every field's path, type and kind (tee-schema --snapshot). Append-only:\n"
               "# the schema test fails when a path here disappears or changes its type.\n"
               + fields.snapshot();
    else if (mode == "markdown")
        text = fields.markdown();
    else
    {
        for (const std::string& problem : tee::lint(fields))
            text += problem + "\n";
        status = text.empty() ? 0 : 1;
    }
    if (out.empty())
        std::fwrite(text.data(), 1, text.size(), stdout);
    else
    {
        std::ofstream file(out, std::ios::binary);
        file << text;
        if (!file)
        {
            std::fprintf(stderr, "tee-schema: cannot write %s\n", out.c_str());
            return 1;
        }
    }
    return status;
}
