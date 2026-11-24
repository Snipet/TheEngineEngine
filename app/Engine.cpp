// app/Engine.cpp: Engine.h.
#include "Engine.h"

#include "player/RefAdapter.h"
#include "player/SilentPlayer.h"
#include "player/TonePlayer.h"

#include "tee/Scenario.h"

#include <cstdlib>
#include <system_error>

#if defined(_WIN32)
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
#elif defined(__APPLE__)
 #include <mach-o/dyld.h>
#else
 #include <unistd.h>
#endif

namespace tee::app
{
    namespace fs = std::filesystem;

    fs::path executableDir()
    {
        std::error_code ec;
       #if defined(_WIN32)
        std::wstring buffer(32768, L'\0');
        const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (n == 0 || n >= buffer.size())
            return fs::current_path(ec);
        buffer.resize(n);
        return fs::path(buffer).parent_path();
       #elif defined(__APPLE__)
        std::uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::string buffer(size, '\0');
        if (_NSGetExecutablePath(buffer.data(), &size) != 0)
            return fs::current_path(ec);
        const fs::path real = fs::canonical(fs::path(buffer.c_str()), ec);
        return ec ? fs::path(buffer.c_str()).parent_path() : real.parent_path();
       #else
        const fs::path real = fs::read_symlink("/proc/self/exe", ec);
        return ec ? fs::current_path(ec) : real.parent_path();
       #endif
    }

    namespace
    {
        // The first of the candidates that holds `probe`.
        fs::path firstWith(const std::vector<fs::path>& candidates, const fs::path& probe)
        {
            std::error_code ec;
            for (const fs::path& c : candidates)
                if (!c.empty() && fs::exists(c / probe, ec))
                    return c;
            return {};
        }

        fs::path fromEnvironment(const char* name)
        {
            const char* value = std::getenv(name);
            return value != nullptr && *value != '\0' ? fs::path(value) : fs::path();
        }

        // Beside the executable, or in a macOS bundle's Resources.
        std::vector<fs::path> beside(const char* folder)
        {
            const fs::path exe = executableDir();
            return { exe / folder, exe.parent_path() / "Resources" / folder };
        }
    }

    fs::path findSim(const Options& o)
    {
        const fs::path probe = fs::path("ref") / "program.fm";
        if (!o.sim.empty())
            return firstWith({ fs::path(o.sim) }, probe);
        std::vector<fs::path> candidates{ fromEnvironment("TEE_SIM_DIR") };
        for (const fs::path& p : beside("sim"))
            candidates.push_back(p);
        return firstWith(candidates, probe);
    }

    fs::path findContent()
    {
        std::vector<fs::path> candidates{ fromEnvironment("TEE_CONTENT_DIR") };
        for (const fs::path& p : beside("content"))
            candidates.push_back(p);
        return firstWith(candidates, fs::path("ref") / "presets");
    }

    namespace
    {
        void setByName(Engine& e, const std::string& id, double value, const std::string& from)
        {
            ParamSource& params = *e.player;
            const int i = params.findParam(id);
            if (i < 0)
                e.notes += from + ": no parameter '" + id + "'\n";
            else
                params.set(i, value);
        }

        void applyPreset(Engine& e, const Options& o)
        {
            if (o.preset.empty())
                return;
            std::error_code ec;
            fs::path file = o.preset;
            if (!fs::is_regular_file(file, ec))
            {
                const fs::path content = findContent();
                file = content / "ref" / "presets" / (o.preset + ".json");
                if (content.empty() || !fs::is_regular_file(file, ec))
                {
                    e.notes += "--preset: no preset '" + o.preset + "' (a file, or a name in content/ref/presets)\n";
                    return;
                }
            }
            Preset preset;
            std::string error;
            if (!loadPreset(file, preset, error))
            {
                e.notes += "--preset: " + error + "\n";
                return;
            }
            for (const auto& [id, value] : preset.dials)
                setByName(e, id, value, "--preset " + o.preset);
        }
    }

    Engine loadEngine(const Options& o, double rate)
    {
        Engine e;
        if (o.engine == "tone")
        {
            e.player = std::make_unique<player::TonePlayer>();
            e.name = "a plain tone (--engine tone: no simulator)";
        }
        else
        {
            e.simDir = findSim(o);
            if (e.simDir.empty())
                e.error = o.sim.empty()
                    ? "no sim/ref/program.fm beside the executable (" + executableDir().string()
                          + "): the simulator's source is missing. --sim DIR names another place."
                    : "no ref/program.fm in " + o.sim;
            else
            {
                player::RefAdapter::Options ro;
                ro.directory = e.simDir / "ref";
                ro.checks = o.checks;
                ro.sampleRate = rate;
                std::unique_ptr<player::RefAdapter> ref = player::RefAdapter::create(ro, e.error);
                if (ref)
                {
                    e.compileSeconds = ref->compileSeconds();
                    e.warnings = ref->warnings();
                    e.ref = ref.get();
                    e.player = std::move(ref);
                    e.name = "the reference engine (sim/ref)";
                    // the application's own choice: in a car, in neutral (Engine.h)
                    ParamSource& params = *e.player;
                    const int drive = params.findParam("drive");
                    if (drive >= 0)
                        params.set(drive, 1.0);
                }
                else if (e.error.empty())
                    e.error = "the reference did not compile, and FMinus gave no message";
            }
            if (!e.player)
            {
                e.player = std::make_unique<player::SilentPlayer>();
                e.name = "nothing (the simulator did not start)";
                return e;
            }
        }
        applyPreset(e, o);
        for (const auto& [id, value] : o.sets)
            setByName(e, id, value, "--set");
        return e;
    }
}
