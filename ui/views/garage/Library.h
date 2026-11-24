// ui/views/garage/Library.h: the builds on disk, as GARAGE needs them (docs/ui/garage.md): a list with a summary of
// each, and loading, saving, duplicating and deleting them. The view never reaches the file system: the shell
// implements this over card C's tee::Build and tee::Presets (shell/files/), and a demo implementation
// (DemoLibrary.h) stands in for tests and whenever nothing else is given.
//
// A build is three parts that load apart (host/include/tee/Build.h, BuildPart): the engine (engine, valve, intake,
// boost, exhaust, fuel), the car (car) and the tune (ecu). A file holds a whole build or one part
// (content/builds/engines/, cars/, cams/...). Values are SI by the Spec's paths without "spec." ("engine.layout"), as
// FieldDoc names them; what a field means (its label, quantity, choices) is read from the document's FieldInfo.
//
// UI thread only. Nothing here blocks: a summary's curve the shell has not made yet is empty, and revision() moves
// when it is in.
#pragma once

#include "app/Workspace.h"

#include <cstdint>
#include <string>
#include <vector>

namespace tee::ui::garage
{
    enum class Part : std::uint8_t
    {
        whole,    // a whole build
        engine,   // the engine alone (tee::BuildPart::Engine)
        car,      // the car alone (Car)
        tune,     // the tune alone (Tune)
    };
    const char* partWord(Part p) noexcept;          // "WHOLE BUILD", "ENGINE", "CAR", "TUNE"

    // One field's value in a build (SI; a choice: its index; a switch: 0 or 1; an array: its elements).
    struct FieldValue
    {
        std::string path;
        std::vector<double> v;
    };

    struct BuildSummary
    {
        std::string id;                 // the library's key (the shell: the file's path); stable while it exists
        std::string name;               // the build's own name (the file's "name")
        std::string notes;
        Part part = Part::whole;        // what the file holds
        bool user = false;              // the user's own (it may be saved over and deleted); else shipped
        bool current = false;           // the build in the document now (at most one)
        std::vector<FieldValue> values; // every field the file sets, in the Spec's order (a part's: its sections)
        DynoTrace curve;                // its predicted torque and power (card C's dyno runner, cached by the build's
                                        // hash); empty until made, and always for a car or a tune alone
    };

    class Library
    {
    public:
        virtual ~Library() = default;

        // Every build and part there is, shipped first, then the user's; the current one's values are the document's
        // as they stand.
        virtual const std::vector<BuildSummary>& builds() const = 0;
        // Moves when the list or any summary changes (a curve made, a file saved, the document edited).
        virtual std::uint64_t revision() const = 0;
        // Once a frame (the frame's tick): takes finished summaries, notices the document's edits.
        virtual void tick(float /*dt*/) {}

        // Loads build `id`'s `part` into the document, one undo step: Part::whole replaces everything (it becomes the
        // current build); a part replaces only its sections (an engine put in this car, a tune tried on this
        // engine), and the current build stays current, now unsaved. False with `error` in words.
        virtual bool load(const std::string& id, Part part, std::string& error) = 0;
        // The current build over its own file (refused for a shipped one: save it as a name).
        virtual bool save(std::string& error) = 0;
        // The current build as a new file of the user's, named `name`; it becomes the current build.
        virtual bool saveAs(const std::string& name, std::string& error) = 0;
        // A copy of `id` among the user's, named "<name> copy" (numbered when taken); its id in `newId`.
        virtual bool duplicate(const std::string& id, std::string& newId, std::string& error) = 0;
        // Deletes a build of the user's (a shipped one is refused). The view asks for a confirmation first.
        virtual bool remove(const std::string& id, std::string& error) = 0;
        // The current build back to what was last loaded or saved (one undo step).
        virtual bool revert(std::string& error) = 0;
        // Whether the document differs from what was last loaded or saved.
        virtual bool unsaved() const = 0;
        // The current build's notes (kept with it, saved with it).
        virtual std::string notes() const = 0;
        virtual void setNotes(const std::string& text) = 0;
    };
}
