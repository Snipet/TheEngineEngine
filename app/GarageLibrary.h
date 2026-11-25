// app/GarageLibrary.h: the GARAGE workspace's Library (ui/views/garage/Library.h, package U5) over the builds on disk
// (shell/files/BuildFiles.h), card C's tee::Build and its dyno runner:
//
//   builds()     every .json under content/builds (shipped, read only, its folders included: engines/, cars/,
//                cams/...; never the test suite's fixtures, kFixtureFolders) and the user's folder
//                (Presets::userDirectory()), shipped first, each with what its file sets (SI, by path without "spec.", the Spec's order) and its part (the file's "part", else the part
//                whose sections hold all it sets). id: the file's path. current: the file the document was last opened
//                from or saved to, whole; its values are the document's as they stand.
//   load(id, part)  a whole build: Build::loadFile, the document's file from then on. A part: the fields the file
//                sets in that part's sections, nothing else (files::applyFields), one undo step; a file that says its
//                part is read by Build::loadFile with it.
//   save, saveAs, duplicate, remove   in the user's folder only; a shipped build is never written or deleted.
//   revert       the document's file read again; unsaved: its dirty flag; notes: Build::notes.
//   curves       each whole build's predicted torque and power, made one at a time in the background by a
//                tee::DynoService of its own (never the audio thread), cached by the build's hash; empty until made,
//                and while the program cannot be put on a dyno (it says why: error()).
//
// The folders are read again every few seconds (and at once after this class writes), so a file copied in by hand
// appears. UI thread only.
#pragma once

#if defined(TEE_HAS_UI_FRAME)

#include "views/garage/Library.h"

#include "files/BuildFiles.h"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tee
{
    class Sim;
    class DynoService;
}

namespace tee::app
{
    class GarageLibrary final : public ui::garage::Library
    {
    public:
        // The shipped directory's folders that hold the test suite's fixtures, not builds the application ships (combo/:
        // the combination archetypes, their pack and the fuzzer's findings, builds reduced until they diverge). Never
        // listed: the owner could load one. The user's own folder is listed whole.
        static constexpr const char* kFixtureFolders[] = { "combo" };

        // `sim` may be null (no curves), or say later what it is (a background compile): asked each tick until it has one.
        GarageLibrary(files::BuildFile& document, std::shared_ptr<Sim> sim, std::filesystem::path shipped,
                      std::filesystem::path user);
        GarageLibrary(files::BuildFile& document, std::function<std::shared_ptr<Sim>()> sim, std::filesystem::path shipped,
                      std::filesystem::path user);
        ~GarageLibrary() override;

        const std::vector<ui::garage::BuildSummary>& builds() const override { return builds_; }
        std::uint64_t revision() const override { return revision_; }
        void tick(float dt) override;
        bool load(const std::string& id, ui::garage::Part part, std::string& error) override;
        bool save(std::string& error) override;
        bool saveAs(const std::string& name, std::string& error) override;
        bool duplicate(const std::string& id, std::string& newId, std::string& error) override;
        bool remove(const std::string& id, std::string& error) override;
        bool revert(std::string& error) override;
        bool unsaved() const override;
        std::string notes() const override;
        void setNotes(const std::string& text) override;

        // Read the folders again now.
        void rescan();
        // Why there are no curves ("" : none, or they are coming).
        const std::string& curveError() const noexcept { return curveError_; }
        // Whether a curve is being made.
        bool makingCurves() const noexcept { return asking_; }

    private:
        struct Entry
        {
            std::filesystem::path file;
            files::FileContents contents;
            bool user = false;
            std::uint64_t hash = 0;         // the whole build's (curves' key); 0 for a part
            std::vector<ui::garage::FieldValue> values;
        };
        const Entry* find(const std::string& id) const;
        void summarise();
        void askNextCurve();

        files::BuildFile& document_;
        std::shared_ptr<Sim> sim_;
        std::function<std::shared_ptr<Sim>()> simLater_;
        std::filesystem::path shipped_, user_;
        std::vector<Entry> entries_;
        std::vector<ui::garage::BuildSummary> builds_;
        std::uint64_t revision_ = 1;
        double sinceScan_ = 0.0;
        std::uint64_t docHash_ = 0;
        std::filesystem::path docFile_;
        bool docDirty_ = false;
        // the curves
        std::unique_ptr<DynoService> dyno_;
        std::map<std::uint64_t, ui::DynoTrace> curves_;
        std::uint64_t askedFor_ = 0;
        bool asking_ = false;
        std::string curveError_;
    };
}

#endif
