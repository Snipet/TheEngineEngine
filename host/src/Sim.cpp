// host/src/Sim.cpp: tee::Sim (Sim.h).
#include "tee/Sim.h"

#include "tee/Telemetry.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string_view>

namespace tee
{
    namespace
    {
        std::uint64_t fnv(std::uint64_t h, std::string_view text) noexcept
        {
            for (const char c : text)
            {
                h ^= static_cast<unsigned char>(c);
                h *= 1099511628211ull;
            }
            return h;
        }

        std::uint64_t hashOf(const Program::Files& files, const Sim::Options& options) noexcept
        {
            std::uint64_t h = 1469598103934665603ull;
            for (const auto& [name, text] : files)   // (a map: in the name's order)
                h = fnv(fnv(fnv(h, name), "\n"), text);
            h = fnv(fnv(h, options.entry), options.module);
            for (const auto& [k, v] : options.defines)
                h = fnv(fnv(fnv(h, k), "="), v);
            return h;
        }

        Program::Files sourcesOf(const std::filesystem::path& directory)
        {
            Program::Files files;
            std::error_code ec;
            for (std::filesystem::recursive_directory_iterator it(directory, ec), end; !ec && it != end; it.increment(ec))
                if (it->is_regular_file(ec) && it->path().extension() == ".fm")
                {
                    std::ifstream in(it->path(), std::ios::binary);
                    std::ostringstream text;
                    text << in.rdbuf();
                    files[std::filesystem::relative(it->path(), directory, ec).generic_string()] = text.str();
                }
            return files;
        }
    } // namespace

    Sim::~Sim() = default;

    std::shared_ptr<Sim> Sim::compile(const std::filesystem::path& directory, const Options& options, std::string& errors)
    {
        Program::Options compile;
        compile.optimisation = options.optimisation;
        compile.checks = options.checks;
        compile.defines = options.defines;
        std::shared_ptr<Sim> sim(new Sim());
        sim->options_ = options;
        sim->directory_ = directory;
        sim->program_ = Program::fromDirectory(directory, options.entry, compile, errors);
        if (!sim->program_ || !sim->bind(errors))
            return nullptr;
        sim->sourceHash_ = hashOf(sourcesOf(directory), options);
        return sim;
    }

    std::shared_ptr<Sim> Sim::compileFiles(Program::Files files, const Options& options, std::string& errors)
    {
        const std::uint64_t sourceHash = hashOf(files, options);
        Program::Options compile;
        compile.optimisation = options.optimisation;
        compile.checks = options.checks;
        compile.defines = options.defines;
        std::shared_ptr<Sim> sim(new Sim());
        sim->options_ = options;
        sim->program_ = Program::fromFiles(std::move(files), options.entry, compile, errors);
        if (!sim->program_ || !sim->bind(errors))
            return nullptr;
        sim->sourceHash_ = sourceHash;
        return sim;
    }

    bool Sim::bind(std::string& errors)
    {
        fminus::Program& fm = program_->fminus();
        const std::string& m = options_.module;
        const std::string where = options_.entry + ": error: ";
        module_ = fm.module(m);
        if (module_ == nullptr)
        {
            errors = where + "the program has no module " + m + " (02-contracts.md, section 1)\n";
            return false;
        }
        std::string error;
        schema_ = Schema::of(fm, m, error);
        if (schema_)
            fields_ = Fields::of(*schema_, error);
        if (!fields_)
        {
            errors = where + error + "\n";
            return false;
        }
        if (!fm.nonBlocking(m + "::process"))
        {
            errors = where + m + "::process is not [[clang::nonblocking]]: the host plays no process that might "
                     "allocate (02-contracts.md, section 1)\n";
            return false;
        }
        init_ = program_->function<InitFn>(m + "::init", error);
        end_ = init_ != nullptr ? program_->function<EndFn>(m + "::end", error) : nullptr;
        process_ = end_ != nullptr ? program_->function<ProcessFn>(m + "::process", error) : nullptr;
        if (process_ == nullptr)
        {
            errors = where + m + " wants init(float), process(float*, float*, int) and end(): " + error + "\n";
            return false;
        }
        return true;
    }

    void Sim::writeDefaults(void* instance) const noexcept
    {
        unsigned char* base = static_cast<unsigned char*>(instance);
        for (const FieldInfo& f : fields_->all())
        {
            if (f.block != "spec" || f.table() || f.kind == FieldKind::Rev || !f.defaultValue)
                continue;
            const std::size_t step = scalarBytes(f.scalar);
            for (std::size_t i = 0; i < f.count; ++i)
                writeScalar(base + f.offset + i * step, f.scalar, *f.defaultValue);
        }
    }

    bool Sim::claim() const noexcept
    {
        bool expected = false;
        return claimed_.compare_exchange_strong(expected, true);
    }

    void Sim::release() const noexcept
    {
        claimed_.store(false);
    }
} // namespace tee
