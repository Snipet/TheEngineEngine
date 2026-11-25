// app/GarageLibrary.cpp: GarageLibrary.h.
#include "GarageLibrary.h"

#if defined(TEE_HAS_UI_FRAME)

#include "app/host/HostSources.h"

#include "tee/Build.h"
#include "tee/Dyno.h"
#include "tee/Fields.h"

#include <algorithm>
#include <system_error>

namespace tee::app
{
    namespace fs = std::filesystem;
    using ui::garage::Part;

    namespace
    {
        Part toPart(BuildPart p)
        {
            return p == BuildPart::Engine ? Part::engine : p == BuildPart::Car ? Part::car : p == BuildPart::Tune ? Part::tune : Part::whole;
        }

        BuildPart fromPart(Part p)
        {
            return p == Part::engine ? BuildPart::Engine : p == Part::car ? BuildPart::Car : p == Part::tune ? BuildPart::Tune : BuildPart::Whole;
        }

        bool samePath(const fs::path& a, const fs::path& b)
        {
            if (a.empty() || b.empty())
                return false;
            std::error_code ec;
            return fs::equivalent(a, b, ec) || a.lexically_normal() == b.lexically_normal();
        }

        // A name as a file's: the characters a file system refuses become '-'.
        std::string fileName(std::string name)
        {
            for (char& c : name)
                if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|'
                    || static_cast<unsigned char>(c) < 32)
                    c = '-';
            while (!name.empty() && (name.back() == ' ' || name.back() == '.'))
                name.pop_back();
            while (!name.empty() && (name.front() == ' ' || name.front() == '.'))
                name.erase(name.begin());
            return name.empty() ? std::string("Untitled") : name;
        }

        constexpr double kRescanSeconds = 3.0;
    }

    GarageLibrary::GarageLibrary(files::BuildFile& document, std::shared_ptr<Sim> sim, fs::path shipped, fs::path user)
        : document_(document), sim_(std::move(sim)), shipped_(std::move(shipped)), user_(std::move(user))
    {
        rescan();
    }

    GarageLibrary::GarageLibrary(files::BuildFile& document, std::function<std::shared_ptr<Sim>()> sim, fs::path shipped,
                                 fs::path user)
        : GarageLibrary(document, std::shared_ptr<Sim>(), std::move(shipped), std::move(user))
    {
        simLater_ = std::move(sim);
    }

    GarageLibrary::~GarageLibrary()
    {
        dyno_.reset();      // its worker joined here, before the program it holds a share of is let go
    }

    void GarageLibrary::rescan()
    {
        sinceScan_ = 0.0;
        std::vector<Entry> found;
        const Fields& fields = document_.build().fields();
        const auto scan = [&](const fs::path& dir, bool user) {
            std::error_code ec;
            if (dir.empty() || !fs::is_directory(dir, ec))
                return;
            std::vector<fs::path> files;
            for (auto it = fs::recursive_directory_iterator(dir, ec); !ec && it != fs::recursive_directory_iterator();
                 it.increment(ec))
            {
                if (!user && it.depth() == 0 && it->is_directory(ec)
                    && files::BuildLibrary::isFixture(it->path().lexically_relative(dir)))
                {
                    it.disable_recursion_pending();   // the test suite's fixtures: not shipped builds
                    continue;
                }
                if (it->is_regular_file(ec) && it->path().extension() == ".json")
                    files.push_back(it->path());
            }
            std::sort(files.begin(), files.end());
            for (const fs::path& f : files)
            {
                Entry e;
                std::string error;
                if (!files::readContents(f, fields, e.contents, error))
                    continue;
                e.file = f;
                e.user = user;
                // its values, read once a scan (a summary made again on every edit of the document reuses them)
                tee::Build scratch(fields);
                std::vector<std::string> warnings;
                if (scratch.loadFile(f, BuildPart::Whole, warnings, error))
                {
                    for (const int i : e.contents.fields)
                        e.values.push_back({ fields.all()[static_cast<std::size_t>(i)].path.substr(5), scratch.values(i) });
                    if (e.contents.part == BuildPart::Whole)
                        e.hash = scratch.hash();
                }
                found.push_back(std::move(e));
            }
        };
        scan(shipped_, false);
        scan(user_, true);
        entries_ = std::move(found);
        summarise();
    }

    void GarageLibrary::summarise()
    {
        const tee::Build& doc = document_.build();
        const Fields& fields = doc.fields();
        docHash_ = doc.hash();
        docFile_ = document_.file();
        docDirty_ = document_.dirty();
        builds_.clear();
        for (const Entry& e : entries_)
        {
            ui::garage::BuildSummary b;
            b.id = e.file.string();
            b.name = e.contents.name.empty() ? e.file.stem().string() : e.contents.name;
            b.notes = e.contents.notes;
            b.part = toPart(e.contents.part);
            b.user = e.user;
            b.current = e.contents.part == BuildPart::Whole && samePath(e.file, docFile_);
            if (b.current)
            {
                // the document's values as they stand: every Spec field a person sets (the document holds them all,
                // whatever the file named)
                b.name = doc.name.empty() ? b.name : doc.name;
                b.notes = doc.notes;
                for (std::size_t i = 0; i < fields.all().size(); ++i)
                {
                    const FieldInfo& f = fields.all()[i];
                    if (f.block == "spec" && f.section >= 0 && !f.hidden && f.kind != FieldKind::Rev)
                        b.values.push_back({ f.path.substr(5), doc.values(static_cast<int>(i)) });
                }
                if (const auto it = curves_.find(docHash_); it != curves_.end())
                    b.curve = it->second;
            }
            else
            {
                b.values = e.values;
                if (const auto it = curves_.find(e.hash); e.hash != 0 && it != curves_.end())
                    b.curve = it->second;
            }
            builds_.push_back(std::move(b));
        }
        ++revision_;
    }

    void GarageLibrary::askNextCurve()
    {
        if (!sim_ && simLater_)
            sim_ = simLater_();
        if (!sim_ || asking_ || !curveError_.empty())
            return;
        for (const Entry& e : entries_)
        {
            if (e.hash == 0 || curves_.count(e.hash) != 0)
                continue;
            if (!dyno_)
                dyno_ = std::make_unique<DynoService>(sim_);
            tee::Build scratch(document_.build().fields());
            std::vector<std::string> warnings;
            std::string error;
            if (!scratch.loadFile(e.file, BuildPart::Whole, warnings, error))
                continue;
            DynoCurve cached;
            askedFor_ = e.hash;
            if (dyno_->request(scratch, DynoPlan{}, cached))
            {
                curves_[e.hash] = ui::ServiceDyno::traceOf(cached, "predicted");
                continue;
            }
            asking_ = true;
            return;
        }
    }

    void GarageLibrary::tick(float dt)
    {
        sinceScan_ += std::max(0.0f, dt);
        if (sinceScan_ >= kRescanSeconds)
            rescan();
        else if (document_.build().hash() != docHash_ || document_.file() != docFile_ || document_.dirty() != docDirty_)
            summarise();
        if (dyno_ && asking_)
        {
            DynoCurve curve;
            if (dyno_->take(curve))
            {
                asking_ = false;
                curves_[askedFor_] = ui::ServiceDyno::traceOf(curve, "predicted");
                summarise();
            }
            else if (!dyno_->busy() && !dyno_->error().empty())
            {
                asking_ = false;
                curveError_ = dyno_->error();     // the program cannot be put on a dyno: no more asking
                ++revision_;
            }
        }
        askNextCurve();
    }

    const GarageLibrary::Entry* GarageLibrary::find(const std::string& id) const
    {
        for (const Entry& e : entries_)
            if (e.file.string() == id)
                return &e;
        return nullptr;
    }

    bool GarageLibrary::load(const std::string& id, Part part, std::string& error)
    {
        const Entry* e = find(id);
        if (e == nullptr)
        {
            error = "no build " + id;
            return false;
        }
        std::vector<std::string> warnings;
        bool ok = false;
        if (part == Part::whole)
        {
            if (e->contents.part != BuildPart::Whole)
            {
                error = "the file holds the " + std::string(partName(e->contents.part)) + " alone: load it as that part";
                return false;
            }
            ok = document_.open(e->file, BuildPart::Whole, warnings, error);
        }
        else if (e->contents.declared)
            ok = document_.open(e->file, fromPart(part), warnings, error);
        else
        {
            ok = files::applyFields(document_.build(), e->file, e->contents, fromPart(part), warnings, error);
            if (ok)
                document_.recent().add(e->file);
        }
        summarise();
        return ok;
    }

    bool GarageLibrary::save(std::string& error)
    {
        const fs::path& file = document_.file();
        if (file.empty())
        {
            error = "the build has no file yet: save it as a name";
            return false;
        }
        const Entry* e = find(file.string());
        if (e != nullptr && !e->user)
        {
            error = "a shipped build is not written over: save it as a name of your own";
            return false;
        }
        const bool ok = document_.save(error);
        rescan();
        return ok;
    }

    bool GarageLibrary::saveAs(const std::string& name, std::string& error)
    {
        if (user_.empty())
        {
            error = "there is no folder for your builds";
            return false;
        }
        const std::string was = document_.build().name;
        document_.build().name = name;
        const bool ok = document_.saveAs(user_ / (fileName(name) + ".json"), BuildPart::Whole, error);
        if (!ok)
            document_.build().name = was;
        rescan();
        return ok;
    }

    bool GarageLibrary::duplicate(const std::string& id, std::string& newId, std::string& error)
    {
        const Entry* e = find(id);
        if (e == nullptr)
        {
            error = "no build " + id;
            return false;
        }
        const std::string base = (e->contents.name.empty() ? e->file.stem().string() : e->contents.name) + " copy";
        std::string name = base;
        std::error_code ec;
        for (int n = 2; fs::exists(user_ / (fileName(name) + ".json"), ec); ++n)
            name = base + " " + std::to_string(n);
        const fs::path to = user_ / (fileName(name) + ".json");
        if (!files::copyBuild(e->file, to, name, error))
            return false;
        newId = to.string();
        rescan();
        return true;
    }

    bool GarageLibrary::remove(const std::string& id, std::string& error)
    {
        const Entry* e = find(id);
        if (e == nullptr)
        {
            error = "no build " + id;
            return false;
        }
        if (!e->user)
        {
            error = "a shipped build cannot be deleted";
            return false;
        }
        std::error_code ec;
        if (!fs::remove(e->file, ec))
        {
            error = "cannot delete " + e->file.string() + (ec ? ": " + ec.message() : std::string());
            return false;
        }
        document_.recent().remove(e->file);
        rescan();
        return true;
    }

    bool GarageLibrary::revert(std::string& error)
    {
        if (document_.file().empty())
        {
            error = "the build has no file to go back to";
            return false;
        }
        std::vector<std::string> warnings;
        const fs::path file = document_.file();
        const bool ok = document_.open(file, BuildPart::Whole, warnings, error);
        summarise();
        return ok;
    }

    bool GarageLibrary::unsaved() const
    {
        return document_.dirty();
    }

    std::string GarageLibrary::notes() const
    {
        return document_.build().notes;
    }

    void GarageLibrary::setNotes(const std::string& text)
    {
        document_.build().notes = text;
        summarise();
    }
}

#endif
