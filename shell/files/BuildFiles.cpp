// shell/files/BuildFiles.cpp: BuildFiles.h.
#include "files/BuildFiles.h"

#include "tee/Fields.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <system_error>

namespace tee::files
{
    // ---- RecentFiles --------------------------------------------------------------------------------------------
    namespace
    {
        bool samePath(const fs::path& a, const fs::path& b)
        {
            std::error_code ec;
            if (fs::exists(a, ec) && fs::exists(b, ec))
                return fs::equivalent(a, b, ec);
            return a.lexically_normal() == b.lexically_normal();
        }
    }

    void RecentFiles::add(const fs::path& file)
    {
        if (file.empty())
            return;
        remove(file);
        files_.insert(files_.begin(), file);
        if (files_.size() > kMost)
            files_.resize(kMost);
    }

    void RecentFiles::remove(const fs::path& file)
    {
        files_.erase(std::remove_if(files_.begin(), files_.end(), [&](const fs::path& f) { return samePath(f, file); }),
                     files_.end());
    }

    void RecentFiles::prune()
    {
        std::error_code ec;
        files_.erase(std::remove_if(files_.begin(), files_.end(),
                                    [&](const fs::path& f) { return !fs::is_regular_file(f, ec); }),
                     files_.end());
    }

    std::vector<std::string> RecentFiles::save() const
    {
        std::vector<std::string> out;
        for (const fs::path& f : files_)
            out.push_back(f.string());
        return out;
    }

    void RecentFiles::load(const std::vector<std::string>& files)
    {
        files_.clear();
        for (auto it = files.rbegin(); it != files.rend(); ++it)
            add(fs::path(*it));
    }

    // ---- BuildFile ----------------------------------------------------------------------------------------------
    BuildFile::BuildFile(tee::Build& build) : build_(build), clean_(now()) {}

    BuildFile::Mark BuildFile::now() const
    {
        return { build_.hash(), build_.name, build_.notes };
    }

    bool BuildFile::dirty() const
    {
        const Mark m = now();
        return m.hash != clean_.hash || m.name != clean_.name || m.notes != clean_.notes;
    }

    void BuildFile::markClean()
    {
        clean_ = now();
    }

    void BuildFile::makeNew()
    {
        build_.reset(BuildPart::Whole);
        build_.name.clear();
        build_.notes.clear();
        file_.clear();
        markClean();
    }

    bool BuildFile::open(const fs::path& file, BuildPart part, std::vector<std::string>& warnings, std::string& error)
    {
        if (!build_.loadFile(file, part, warnings, error))
            return false;
        if (part == BuildPart::Whole)
        {
            file_ = file;
            if (build_.name.empty())
                build_.name = file.stem().string();
            markClean();
        }
        recent_.add(file);
        return true;
    }

    bool BuildFile::save(std::string& error)
    {
        if (file_.empty())
        {
            error = "no file yet: save it as a file first";
            return false;
        }
        return saveAs(file_, BuildPart::Whole, error);
    }

    bool BuildFile::saveAs(const fs::path& given, BuildPart part, std::string& error)
    {
        fs::path file = given;
        if (!file.has_extension())
            file += ".json";
        std::error_code ec;
        if (file.has_parent_path())
            fs::create_directories(file.parent_path(), ec);
        if (!build_.saveFile(file, part, error))
            return false;
        if (part == BuildPart::Whole)
        {
            file_ = file;
            markClean();
        }
        recent_.add(file);
        return true;
    }

    std::string BuildFile::title() const
    {
        std::string t = build_.name.empty() ? (file_.empty() ? std::string("Untitled") : file_.stem().string()) : build_.name;
        return dirty() ? t + " *" : t;
    }

    // ---- the library --------------------------------------------------------------------------------------------
    bool readSummary(const fs::path& file, BuildSummary& out)
    {
        std::ifstream in(file, std::ios::binary);
        if (!in)
            return false;
        std::stringstream text;
        text << in.rdbuf();
        const nlohmann::json j = nlohmann::json::parse(text.str(), nullptr, /*allow_exceptions*/ false);
        if (!j.is_object() || j.value("format", std::string()) != "tee-build")
            return false;
        out.file = file;
        out.name = file.stem().string();
        out.title = j.value("name", std::string());
        out.notes = j.value("notes", std::string());
        out.part = j.value("part", std::string());
        if (const auto spec = j.find("spec"); spec != j.end() && spec->is_object())
        {
            if (const auto engine = spec->find("engine"); engine != spec->end() && engine->is_object())
            {
                if (const auto l = engine->find("layout"); l != engine->end() && l->is_string())
                    out.layout = l->get<std::string>();
                if (const auto d = engine->find("displacement"); d != engine->end() && d->is_number())
                    out.displacement = d->get<double>();
            }
            if (const auto boost = spec->find("boost"); boost != spec->end() && boost->is_object())
                if (const auto k = boost->find("kind"); k != boost->end() && k->is_string())
                    out.boost = k->get<std::string>();
        }
        std::error_code ec;
        out.modified = fs::last_write_time(file, ec);
        return true;
    }

    BuildLibrary::BuildLibrary(fs::path shipped, fs::path user) : shipped_(std::move(shipped)), user_(std::move(user)) {}

    bool BuildLibrary::validName(const std::string& name, std::string& why)
    {
        if (name.empty() || name.size() > 120)
        {
            why = "a build's name is 1 to 120 characters";
            return false;
        }
        for (const char c : name)
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|'
                || static_cast<unsigned char>(c) < 32)
            {
                why = "a build's name may not hold / \\ : * ? \" < > | or a control character";
                return false;
            }
        if (name.front() == ' ' || name.back() == ' ' || name.front() == '.' || name.back() == '.')
        {
            why = "a build's name may not start or end with a space or a period";
            return false;
        }
        return true;
    }

    std::vector<BuildSummary> BuildLibrary::list() const
    {
        std::map<std::string, BuildSummary> shipped, user;
        const auto scan = [](const fs::path& dir, bool isUser, std::map<std::string, BuildSummary>& into) {
            std::error_code ec;
            if (dir.empty() || !fs::is_directory(dir, ec))
                return;
            for (const fs::directory_entry& e : fs::directory_iterator(dir, ec))
            {
                if (!e.is_regular_file(ec) || e.path().extension() != ".json")
                    continue;
                BuildSummary s;
                if (!readSummary(e.path(), s) || !s.part.empty())
                    continue;
                s.user = isUser;
                into[s.name] = std::move(s);
            }
        };
        scan(shipped_, false, shipped);
        scan(user_, true, user);
        std::vector<BuildSummary> out;
        for (auto& [name, s] : shipped)
            if (user.find(name) == user.end())
                out.push_back(std::move(s));
        for (auto& [name, s] : user)
            out.push_back(std::move(s));
        return out;
    }

    fs::path BuildLibrary::find(const std::string& name, bool& user) const
    {
        std::error_code ec;
        std::string why;
        if (!validName(name, why))
            return {};
        if (!user_.empty() && fs::is_regular_file(user_ / (name + ".json"), ec))
        {
            user = true;
            return user_ / (name + ".json");
        }
        if (!shipped_.empty() && fs::is_regular_file(shipped_ / (name + ".json"), ec))
        {
            user = false;
            return shipped_ / (name + ".json");
        }
        return {};
    }

    bool BuildLibrary::summary(const std::string& name, BuildSummary& out) const
    {
        bool user = false;
        const fs::path f = find(name, user);
        if (f.empty() || !readSummary(f, out))
            return false;
        out.user = user;
        return true;
    }

    bool BuildLibrary::load(BuildFile& file, const std::string& name, std::vector<std::string>& warnings,
                            std::string& error) const
    {
        bool user = false;
        const fs::path f = find(name, user);
        if (f.empty())
        {
            error = "no build named '" + name + "'";
            return false;
        }
        return file.open(f, BuildPart::Whole, warnings, error);
    }

    bool BuildLibrary::save(BuildFile& file, const std::string& name, std::string& error) const
    {
        if (!validName(name, error))
            return false;
        if (user_.empty())
        {
            error = "there is no folder for the user's builds";
            return false;
        }
        const std::string was = file.build().name;
        file.build().name = name;
        if (!file.saveAs(user_ / (name + ".json"), BuildPart::Whole, error))
        {
            file.build().name = was;
            return false;
        }
        return true;
    }

    bool BuildLibrary::duplicate(const std::string& name, const std::string& as, std::string& error) const
    {
        if (!validName(as, error))
            return false;
        bool user = false;
        const fs::path from = find(name, user);
        if (from.empty())
        {
            error = "no build named '" + name + "'";
            return false;
        }
        const fs::path to = user_ / (as + ".json");
        std::error_code ec;
        if (fs::exists(to, ec))
        {
            error = "there is a build named '" + as + "' already";
            return false;
        }
        std::ifstream in(from, std::ios::binary);
        std::stringstream text;
        text << in.rdbuf();
        nlohmann::json j = nlohmann::json::parse(text.str(), nullptr, false);
        if (!j.is_object())
        {
            error = from.string() + " is not a build";
            return false;
        }
        j["name"] = as;
        fs::create_directories(user_, ec);
        std::ofstream out(to, std::ios::binary);
        out << j.dump(2) << "\n";
        if (!out)
        {
            error = "cannot write " + to.string();
            return false;
        }
        return true;
    }

    bool BuildLibrary::remove(const std::string& name, std::string& error) const
    {
        bool user = false;
        const fs::path f = find(name, user);
        if (f.empty())
        {
            error = "no build named '" + name + "'";
            return false;
        }
        if (!user)
        {
            error = "'" + name + "' is a shipped build: it cannot be deleted";
            return false;
        }
        std::error_code ec;
        if (!fs::remove(f, ec))
        {
            error = "cannot delete " + f.string() + (ec ? ": " + ec.message() : std::string());
            return false;
        }
        return true;
    }

    namespace
    {
        bool readJson(const fs::path& file, nlohmann::json& j, std::string& error)
        {
            std::ifstream in(file, std::ios::binary);
            if (!in)
            {
                error = "cannot read " + file.string();
                return false;
            }
            std::stringstream text;
            text << in.rdbuf();
            j = nlohmann::json::parse(text.str(), nullptr, false);
            if (!j.is_object() || j.value("format", std::string()) != "tee-build")
            {
                error = file.string() + " is not a build";
                return false;
            }
            return true;
        }

        std::string sectionOf(const Fields& fields, const FieldInfo& f)
        {
            return f.section >= 0 ? fields.sections()[static_cast<std::size_t>(f.section)].name : std::string();
        }
    }

    bool readContents(const fs::path& file, const Fields& fields, FileContents& out, std::string& error)
    {
        nlohmann::json j;
        if (!readJson(file, j, error))
            return false;
        out = {};
        out.name = j.value("name", std::string());
        out.notes = j.value("notes", std::string());
        if (j.contains("part"))
        {
            if (!partFromName(j.value("part", std::string()), out.part))
            {
                error = file.string() + ": its \"part\" is none of whole, engine, car, tune";
                return false;
            }
            out.declared = true;
        }
        const auto specIt = j.find("spec");
        if (specIt == j.end() || !specIt->is_object())
            return true;
        const nlohmann::json& spec = *specIt;
        std::vector<std::string> sections;
        for (std::size_t i = 0; i < fields.all().size(); ++i)
        {
            const FieldInfo& f = fields.all()[i];
            if (f.block != "spec" || f.section < 0 || f.hidden || f.kind == FieldKind::Rev)
                continue;
            // "spec.a.b" walked through the file's objects
            const nlohmann::json* at = &spec;
            std::size_t from = 5;
            bool found = true;
            while (found && from <= f.path.size())
            {
                std::size_t dot = f.path.find('.', from);
                if (dot == std::string::npos)
                    dot = f.path.size();
                const std::string key = f.path.substr(from, dot - from);
                const auto it = at->is_object() ? at->find(key) : at->end();
                found = at->is_object() && it != at->end();
                if (found)
                    at = &*it;
                from = dot + 1;
            }
            if (!found)
                continue;
            out.fields.push_back(static_cast<int>(i));
            const std::string section = sectionOf(fields, f);
            if (std::find(sections.begin(), sections.end(), section) == sections.end())
                sections.push_back(section);
        }
        // the part: from the sections the file names (whether this program knows their fields or not)
        sections.clear();
        for (auto it = spec.begin(); it != spec.end(); ++it)
            sections.push_back(it.key());
        if (!out.declared && !sections.empty())
        {
            for (const BuildPart p : { BuildPart::Engine, BuildPart::Car, BuildPart::Tune })
                if (std::all_of(sections.begin(), sections.end(), [p](const std::string& sec) { return inPart(sec, p); }))
                {
                    out.part = p;
                    break;
                }
        }
        return true;
    }

    bool applyFields(tee::Build& build, const fs::path& file, const FileContents& contents, BuildPart part,
                     std::vector<std::string>& warnings, std::string& error)
    {
        // the file read whole into a scratch build of the same program, then the fields it names copied over
        tee::Build scratch(build.fields());
        if (!scratch.loadFile(file, BuildPart::Whole, warnings, error))
            return false;
        const Fields& fields = build.fields();
        build.beginGesture();
        for (const int i : contents.fields)
        {
            const FieldInfo& f = fields.all()[static_cast<std::size_t>(i)];
            if (part != BuildPart::Whole && !inPart(sectionOf(fields, f), part))
                continue;
            build.setValues(i, scratch.values(i));
        }
        build.endGesture();
        return true;
    }

    bool copyBuild(const fs::path& from, const fs::path& to, const std::string& newName, std::string& error)
    {
        nlohmann::json j;
        if (!readJson(from, j, error))
            return false;
        std::error_code ec;
        if (fs::exists(to, ec))
        {
            error = to.string() + " is there already";
            return false;
        }
        j["name"] = newName;
        if (to.has_parent_path())
            fs::create_directories(to.parent_path(), ec);
        std::ofstream out(to, std::ios::binary);
        out << j.dump(2) << "\n";
        if (!out)
        {
            error = "cannot write " + to.string();
            return false;
        }
        return true;
    }
}
