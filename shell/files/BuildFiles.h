// shell/files/BuildFiles.h: builds on disk (docs/plan/w1-b-shell.md, slice 2, item 9). A build is card C's tee::Build
// (host/include/tee/Build.h), its file the format of docs/formats.md ("tee-build"). Three things here:
//
//   BuildFile     the open document: which file it came from, which part, whether it has changed since it was loaded or
//                 saved (the dirty flag: the Spec's hash, the name and the notes), and the commands a File menu has:
//                 new, open, save, save as, and a part (engine, car, tune) loaded or saved apart from the rest.
//   RecentFiles   the files opened or saved last, newest first, at most ten; the settings file keeps them.
//   BuildLibrary  the builds a person can pick by name (the GARAGE workspace's): the shipped ones (content/builds, read
//                 only) and the user's own (Presets::userDirectory(), or TEE_USER_DIR): list, a summary of each read
//                 from its file without loading it, load, save, duplicate, delete. The user's own are the only ones
//                 written or deleted; a shipped build saved is saved as the user's.
//
// UI thread only: everything here touches the disk. The open dialogs are the window's (app/Main.cpp, FunkGui's
// HostServices::chooseFiles); nothing here knows of a window.
#pragma once

#include "tee/Build.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace tee::files
{
    namespace fs = std::filesystem;

    class RecentFiles
    {
    public:
        static constexpr std::size_t kMost = 10;

        const std::vector<fs::path>& list() const noexcept { return files_; }
        // To the front (once; a path already there moves); the oldest past kMost go.
        void add(const fs::path& file);
        void remove(const fs::path& file);
        // Those that are no longer there go.
        void prune();
        // As the settings file keeps them (a string a path), and back.
        std::vector<std::string> save() const;
        void load(const std::vector<std::string>& files);

    private:
        std::vector<fs::path> files_;
    };

    class BuildFile
    {
    public:
        explicit BuildFile(tee::Build& build);

        tee::Build& build() noexcept { return build_; }
        // The file the build was last opened from or saved to (empty: none yet, an untitled build).
        const fs::path& file() const noexcept { return file_; }
        // Changed since it was loaded, saved or made new.
        bool dirty() const;
        // What dirty() compares with is the build as it is now (after a load the application makes itself).
        void markClean();

        // Every field to its default, no file, clean (one undo step, as Build::reset).
        void makeNew();
        // Reads a whole build, or a part's file into its sections (`part` must be the file's own, else it is refused).
        // A whole build becomes the file; a part leaves the file as it was and makes the build dirty. False, with
        // `error`; `warnings` gets what was kept unread or out of range. The recent files get the file.
        bool open(const fs::path& file, BuildPart part, std::vector<std::string>& warnings, std::string& error);
        // Writes the build to file() (false, "no file yet", for an untitled build: the caller asks for one).
        bool save(std::string& error);
        // Writes it to `file` (".json" added when it has no extension); a whole build becomes that file and clean. A
        // part is written alone and changes neither the file nor the flag.
        bool saveAs(const fs::path& file, BuildPart part, std::string& error);

        RecentFiles& recent() noexcept { return recent_; }

        // The window's title: "name *" (the star when dirty).
        std::string title() const;

    private:
        struct Mark
        {
            std::uint64_t hash = 0;
            std::string name, notes;
        };
        Mark now() const;

        tee::Build& build_;
        fs::path file_;
        Mark clean_;
        RecentFiles recent_;
    };

    // A build in the library, read from its file's head without loading it.
    struct BuildSummary
    {
        std::string name;              // what list() and load() call it: the file's name without .json
        std::string title;             // the build's own "name" in the file ("" : none)
        std::string notes;
        std::string part;              // "" for a whole build, else "engine", "car" or "tune"
        std::string layout;            // spec.engine.layout, as the file has it ("V8 crossplane"; "" : not said)
        double displacement = 0.0;     // spec.engine.displacement, m^3 (0: not said)
        std::string boost;             // spec.boost.kind ("Natural", "Turbo", ...)
        bool user = false;             // the user's own (else shipped)
        fs::path file;
        fs::file_time_type modified{};
    };

    // A whole build of the library by its folder-qualified name (BuildLibrary::wholeBuilds).
    struct WholeBuild
    {
        std::string name;              // "default", "archetypes/flat-plane-v8-45": the path under its folder, "/"
                                       // between folders, without .json (top-level and the user's: the file's stem)
        fs::path file;
        bool user = false;             // the user's own (else shipped)
    };

    class BuildLibrary
    {
    public:
        // The test suite's fixtures under the shipped folder (combo/: the combination archetypes, their pack and the
        // fuzzer's findings, builds reduced until they diverge): never a build a person opens. The rule for which files
        // are builds an owner can open is this class's; GARAGE (app/GarageLibrary) and the assistant read it.
        static constexpr const char* kFixtureFolders[] = { "combo" };
        // Whether `relative` (a path under the shipped folder) lies in a fixture folder.
        static bool isFixture(const fs::path& relative);

        // `shipped` may be empty (none); `user` is made when first written to.
        BuildLibrary(fs::path shipped, fs::path user);

        const fs::path& shippedDirectory() const noexcept { return shipped_; }
        const fs::path& userDirectory() const noexcept { return user_; }

        // The whole builds (not the parts' folders): shipped first, then the user's, each by name. A user's build of
        // a shipped one's name is listed once, as the user's.
        std::vector<BuildSummary> list() const;
        // Every whole build a person can open: the shipped folder's, recursively (its folders included: archetypes/,
        // ...; the fixtures never), each by its folder-qualified name, then the user's folder's (its top, as list()).
        // Whole against part as GARAGE decides it (readContents with `fields`: a file that says its part, or whose
        // fields all lie in one part's sections, is that part, not listed). A user's build of a top-level shipped
        // one's name is listed once, as the user's. Sorted by name, shipped first.
        std::vector<WholeBuild> wholeBuilds(const Fields& fields) const;
        // One by name: a top-level or user's name, or a folder-qualified one ("archetypes/flat-plane-v8-45"). False
        // when there is none, or it is a fixture.
        bool summary(const std::string& name, BuildSummary& out) const;
        // Into `file` (the open document), by name (as summary() takes it). As BuildFile::open. A fixture is refused,
        // and so is a part (by readContents with the document's fields), each with the reason in words.
        bool load(BuildFile& file, const std::string& name, std::vector<std::string>& warnings, std::string& error) const;
        // The open build saved in the user's folder as `name` (its "name" set to it); the document becomes that file.
        bool save(BuildFile& file, const std::string& name, std::string& error) const;
        // A copy of `name` as `as` in the user's folder (false when `as` is there already).
        bool duplicate(const std::string& name, const std::string& as, std::string& error) const;
        // The user's build of that name, gone (a shipped one is refused).
        bool remove(const std::string& name, std::string& error) const;

        // Whether `name` can be a build's file name: not empty, no separator, no character a file system refuses.
        static bool validName(const std::string& name, std::string& why);

    private:
        fs::path find(const std::string& name, bool& user, std::string* why = nullptr) const;

        fs::path shipped_, user_;
    };

    // The summary of the build in `file` (false: not a build this can read).
    bool readSummary(const fs::path& file, BuildSummary& out);

    // What a build's file sets: the indexes (in fields.all()) of the Spec fields it gives a value for, in the Spec's
    // order, and the part it holds: its "part" when it says one (`declared` true), else the smallest part whose
    // sections hold every field it sets (a file of the valve section alone is the engine's), else the whole.
    struct FileContents
    {
        std::vector<int> fields;
        BuildPart part = BuildPart::Whole;
        bool declared = false;
        std::string name, notes;
    };
    bool readContents(const fs::path& file, const Fields& fields, FileContents& out, std::string& error);

    // The fields `contents` names, from `file` into `build` (one undo step), and no other: a part that names a few
    // fields of its sections (a camshaft) changes those, where Build::loadFile with the part would put the rest of
    // the part's sections back to their defaults. Only the fields in `part`'s sections.
    bool applyFields(tee::Build& build, const fs::path& file, const FileContents& contents, BuildPart part,
                     std::vector<std::string>& warnings, std::string& error);

    // A copy of a build's file at `to`, its "name" `newName`. False when `to` exists or cannot be written.
    bool copyBuild(const fs::path& from, const fs::path& to, const std::string& newName, std::string& error);
}
