// app/Launch.cpp: Launch.h.
#include "Launch.h"

#include "Engine.h"
#include "files/BuildFiles.h"
#include "player/SimAdapter.h"

#include "tee/Build.h"
#include "tee/Fields.h"
#include "tee/SimHost.h"

#include <cstdio>
#include <cstdlib>
#include <system_error>

namespace tee::app
{
    namespace fs = std::filesystem;

    LaunchBuild chooseLaunchBuild(const Options& o, bool machine, const std::string& lastBuild, const fs::path& content)
    {
        LaunchBuild l;
        if (!o.preset.empty())
            return l;                   // the preset's, applied already
        if (o.reference)
        {
            l.what = "none (--reference): the simulator's defaults, every model at Reference";
            return l;
        }
        std::error_code ec;
        const char* settingsDir = std::getenv("TEE_SETTINGS_DIR");
        const bool settingsChosen = settingsDir != nullptr && settingsDir[0] != '\0';
        if ((!machine || settingsChosen) && !lastBuild.empty() && fs::is_regular_file(lastBuild, ec))
        {
            l.file = lastBuild;
            l.what = "the last build";
            return l;
        }
        // a machine's run opens the first build only by name (--first-build): what CI's window runs draw and time does
        // not change with a content file, nor carry every model on
        if (machine && o.firstBuild.empty())
        {
            l.what = "none (a scripted run: --first-build FILE or --preset NAME opens one): the simulator's defaults, "
                     "every model at Reference";
            return l;
        }
        const fs::path first = !o.firstBuild.empty() ? fs::path(o.firstBuild)
                               : content.empty()     ? fs::path()
                                                     : content / "builds" / "first.json";
        if (!first.empty() && fs::is_regular_file(first, ec))
        {
            l.file = first;
            l.what = "the first build";
            l.first = true;
            return l;
        }
        if (!o.firstBuild.empty())
        {
            l.what = "none (--first-build: no such file): the simulator's defaults, every model at Reference";
            return l;
        }
        l.what = "none (no content/builds/first.json): the simulator's defaults, every model at Reference";
        return l;
    }

    LaunchBuild chooseLaunchBuild(const Options& o, bool machine, const std::string& lastBuild)
    {
        return chooseLaunchBuild(o, machine, lastBuild, findContent());
    }

    std::string launchReadBack(files::BuildFile& document, player::SimAdapter* sim)
    {
        const tee::Build& build = document.build();
        std::string title = document.title();
        if (title.size() > 2 && title.compare(title.size() - 2, 2, " *") == 0)
            title.resize(title.size() - 2);
        std::string line = "build: \"" + title + "\" file=" + (document.file().empty() ? std::string("none") : document.file().string())
                           + " unsaved=" + (document.dirty() ? "yes" : "no") + " models:";
        const bool sent = sim != nullptr && sim->ready();
        const std::string suffix = ".model";
        for (const FieldInfo& f : build.fields().all())
        {
            if (f.block != "spec" || f.path.size() <= 5 + suffix.size()
                || f.path.compare(f.path.size() - suffix.size(), suffix.size(), suffix) != 0)
                continue;
            const std::string path = f.path.substr(5);
            char text[96];
            std::snprintf(text, sizeof text, " %s=%g", path.substr(0, path.size() - suffix.size()).c_str(), build.get(path));
            line += text;
            if (sent)
            {
                std::string error;
                const FieldRef ref = sim->host().fields().ref(f.path, error);
                if (error.empty())
                {
                    std::snprintf(text, sizeof text, "/%g", sim->host().readSpec(ref));
                    line += text;
                }
            }
        }
        return line;
    }

    bool readSent(player::SimAdapter& sim, const std::string& path, double& out)
    {
        if (!sim.ready())
            return false;
        std::string error;
        const FieldRef ref = sim.host().fields().ref(path.rfind("spec.", 0) == 0 ? path : "spec." + path, error);
        if (!error.empty())
            return false;
        out = sim.host().readSpec(ref);
        return true;
    }
}
