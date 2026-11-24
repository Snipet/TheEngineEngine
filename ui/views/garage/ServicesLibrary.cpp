// ui/views/garage/ServicesLibrary.cpp: see ServicesLibrary.h.
#include "views/garage/ServicesLibrary.h"

namespace tee::ui::garage
{
    namespace
    {
        const char* const kNotGiven = "the application does not do that here yet";
    }

    bool ServicesLibrary::usable(const WorkspaceContext& ctx) noexcept
    {
        return ctx.services != nullptr && ctx.services->builds && ctx.services->load;
    }

    ServicesLibrary::ServicesLibrary(WorkspaceContext& ctx) : ctx_(ctx) { refresh(); }

    void ServicesLibrary::refresh()
    {
        const FrameServices& s = *ctx_.services;
        names_ = s.builds();
        if (s.buildName)
            current_ = s.buildName();
        builds_.clear();
        for (const std::string& n : names_)
        {
            BuildSummary b;
            b.id = n;
            b.name = n;
            b.current = n == current_;
            if (b.current)
            {
                for (int f = 0; f < ctx_.doc.count(); ++f)
                    if (!ctx_.doc.info(f).hidden && !ctx_.doc.info(f).table())
                        b.values.push_back({ ctx_.doc.info(f).path, { ctx_.doc.get(f) } });
                b.curve = ctx_.dyno.predicted();
            }
            builds_.push_back(std::move(b));
        }
        docSeen_ = ctx_.doc.revision();
        dynoSeen_ = ctx_.dyno.revision();
        ++revision_;
    }

    void ServicesLibrary::tick(float)
    {
        const std::string name = ctx_.services->buildName ? ctx_.services->buildName() : current_;
        if (ctx_.doc.revision() != docSeen_ || ctx_.dyno.revision() != dynoSeen_ || name != current_)
            refresh();
    }

    bool ServicesLibrary::load(const std::string& id, Part part, std::string& error)
    {
        if (part != Part::whole)
        {
            error = "loading a part alone: " + std::string(kNotGiven);
            return false;
        }
        const bool ok = ctx_.services->load(id, error);
        refresh();
        return ok;
    }

    bool ServicesLibrary::save(std::string& error)
    {
        return saveAs(current_, error);
    }

    bool ServicesLibrary::saveAs(const std::string& name, std::string& error)
    {
        if (!ctx_.services->save)
        {
            error = kNotGiven;
            return false;
        }
        const bool ok = ctx_.services->save(name, error);
        refresh();
        return ok;
    }

    bool ServicesLibrary::duplicate(const std::string&, std::string&, std::string& error)
    {
        error = std::string("duplicate: ") + kNotGiven;
        return false;
    }

    bool ServicesLibrary::remove(const std::string&, std::string& error)
    {
        error = std::string("delete: ") + kNotGiven;
        return false;
    }

    bool ServicesLibrary::revert(std::string& error)
    {
        return load(current_, Part::whole, error);
    }

    bool ServicesLibrary::unsaved() const { return ctx_.services->unsaved ? ctx_.services->unsaved() : false; }
}
