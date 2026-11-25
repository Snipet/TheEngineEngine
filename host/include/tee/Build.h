// host/include/tee/Build.h: the document: what the machine is, as values by path in SI, the truth on the UI thread of
// which the simulator's Spec is a copy (02-contracts.md, sections 7 and 8).
//
//   tee::Build build(host.fields());                       // every field at its default
//   build.load(text, tee::BuildPart::Whole, warnings, error);
//   build.attach(&host);                                   // sends every value; then each change as it is made
//   build.beginGesture();  build.set("spec.engine.bore", 0.1);  build.endGesture();
//   build.undo();
//   std::string json = build.save();
//
// A value is the field's numbers in SI: one for a number, a switch or a choice (its index from the range's minimum);
// an array's elements; a table's own fields one after the other (a curve's x[16], y[16], n; a map's x[16], y[16],
// z[16][16], nx, ny). Every write is clamped to the field's range (a table's points are taken as given). Paths are the
// Fields' ("spec.engine.bore"); "engine.bore" is taken too.
//
// The file (docs/formats.md, "A build"): {"format": "tee-build", "version": 1, "name", "notes", "spec": {...}}, the
// Spec nested by section, numbers as numbers, choices by name, switches as true and false, arrays as arrays, a curve
// as {"x": [...], "y": [...]} and a map as {"x": [...], "y": [...], "z": [[...], ...]} (z[i] the row at x[i]), each
// only as long as its points in use. A field the file lacks takes its default; a field the program lacks is kept and
// written back (a newer build opened by an older application loses nothing), with one warning. A part (the engine,
// the car, the tune) is saved and loaded alone: loading one touches only its sections.
//
// Undo and redo are by gesture: the changes made between beginGesture and endGesture (a knob's drag, a file loaded)
// are one step. Outside a gesture each change is a step of its own.
//
// Threads: the UI thread's (or one thread's): no lock inside.
#pragma once

#include "tee/Fields.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    class SimHost;

    enum class BuildPart
    {
        Whole,
        Engine,   // spec.engine, valve, intake, boost, exhaust and fuel: the machine without the car and the tune
        Car,      // spec.car
        Tune,     // spec.ecu
    };

    const char* partName(BuildPart part) noexcept;
    bool partFromName(std::string_view name, BuildPart& part) noexcept;
    // Whether a section belongs to a part (every section is in Whole; env and sound in no other).
    bool inPart(std::string_view section, BuildPart part) noexcept;
    // Whether a field is saved and loaded: not a hidden one (working state), save a hidden fingerprint (unit "hash").
    bool inFile(const FieldInfo& f) noexcept;

    class Build
    {
    public:
        explicit Build(const Fields& fields);

        const Fields& fields() const noexcept { return *fields_; }

        std::string name;
        std::string notes;

        // ---- values ---------------------------------------------------------------------------------------------
        // The Spec field of that path (with or without "spec."), or -1.
        int fieldOf(std::string_view path) const noexcept;
        // A number, an element of an array (`index`), a switch (0 or 1), a choice (its index). 0 for no such field.
        double get(std::string_view path, std::size_t index = 0) const noexcept;
        // Sets one (clamped). False when there is no such Spec field or element.
        bool set(std::string_view path, double value, std::size_t index = 0);
        // A choice by its name.
        bool setChoice(std::string_view path, std::string_view choice);
        // A field's values whole, laid out as above; setValues takes them (the count must be the field's).
        const std::vector<double>& values(int field) const;
        bool setValues(int field, std::vector<double> values);
        // A curve from its points (at most 16), a map from its axes and z[i][j] at x[i], y[j].
        bool setCurve(std::string_view path, const std::vector<double>& x, const std::vector<double>& y);
        bool setMap(std::string_view path, const std::vector<double>& x, const std::vector<double>& y,
                    const std::vector<std::vector<double>>& z);
        // Every field of a part back to its default (one step).
        void reset(BuildPart part = BuildPart::Whole);

        // ---- undo -----------------------------------------------------------------------------------------------
        void beginGesture();
        void endGesture();
        bool undo();
        bool redo();
        bool canUndo() const noexcept { return !undo_.empty(); }
        bool canRedo() const noexcept { return !redo_.empty(); }
        // Each step has an id, given when it is made (a gesture's when it ends), counted from 1 and never reused, kept
        // through undo and redo: what undo() would take back now (0: nothing), and what redo() would put back. A
        // caller that made a step (the assistant's card with its Undo) offers to undo it only while it is on top.
        // A rebind that keeps the steps (the same fields in the same order: a quality change) keeps their ids, so
        // undoStep() answers the same id after it; one that drops the steps drops their ids (0 until the next step).
        std::uint64_t undoStep() const noexcept { return undoIds_.empty() ? 0 : undoIds_.back(); }
        std::uint64_t redoStep() const noexcept { return redoIds_.empty() ? 0 : redoIds_.back(); }
        // A gesture is open (beginGesture without its endGesture yet: a knob held): a change made now joins it.
        bool gestureOpen() const noexcept { return gestureDepth_ > 0; }

        // ---- files ----------------------------------------------------------------------------------------------
        // Reads a build (or a part's file). False, with `error` saying what is wrong, when it is no build or not of
        // the part asked for; `warnings` gets what was kept unread and what was out of its range. One undo step.
        bool load(std::string_view json, BuildPart part, std::vector<std::string>& warnings, std::string& error);
        bool loadFile(const std::filesystem::path& file, BuildPart part, std::vector<std::string>& warnings,
                      std::string& error);
        std::string save(BuildPart part = BuildPart::Whole) const;
        bool saveFile(const std::filesystem::path& file, BuildPart part, std::string& error) const;
        // What the build holds that the program does not know: dotted paths under "spec", each with its JSON.
        const std::map<std::string, std::string>& unknown() const noexcept { return unknown_; }

        // ---- comparing ------------------------------------------------------------------------------------------
        struct Difference
        {
            std::string path;
            std::vector<double> a, b;
        };
        // The Spec fields whose values differ, in declaration order (the two builds must be of one program).
        static std::vector<Difference> diff(const Build& a, const Build& b);
        // A hash of the Spec's values (not the name or the notes): the dyno's cache's key.
        std::uint64_t hash() const;

        // ---- the simulator --------------------------------------------------------------------------------------
        // Sends every value to `host` now and each change after (nullptr: none). The host must be of these Fields.
        void attach(SimHost* host);
        // Sends what the host refused (its queue was full). True when nothing is left waiting. Call it each frame.
        bool flush();
        // The program was reloaded: these Fields are its new ones. Each value is kept by path where its type is
        // the same, else the field starts at its default; everything is sent again. The undo and redo steps are kept
        // when the new Fields have the same fields in the same order (a quality change), else dropped.
        void rebind(const Fields& fields);

        // The bytes of a field as the simulator lays it out (the field's size).
        std::vector<unsigned char> bytes(int field) const;

    private:
        struct Change
        {
            int field;
            std::vector<double> before, after;
        };

        void defaults();
        std::vector<double> defaultsOf(const FieldInfo& f) const;
        void change(int field, std::vector<double> after);
        void send(int field);
        void clampValues(const FieldInfo& f, std::vector<double>& v) const;

        const Fields* fields_;
        std::vector<std::vector<double>> values_;   // a Spec field's, by its index in Fields::all(); empty otherwise
        std::map<std::string, std::string> unknown_;
        std::vector<std::vector<Change>> undo_, redo_;
        std::vector<std::uint64_t> undoIds_, redoIds_;   // each step's id, beside it
        std::uint64_t nextStep_ = 1;
        std::vector<Change> gesture_;
        int gestureDepth_ = 0;
        SimHost* host_ = nullptr;
        std::set<int> waiting_;
    };

    // The builds a person can pick: content/builds/ and the user's own folder.
    struct PresetEntry
    {
        std::string name;                  // the file's name without .json
        std::filesystem::path file;
        bool user = false;                 // the user's own (else shipped)
    };

    class Presets
    {
    public:
        // Every .json under each directory (shipped first, then the user's), sorted by name within each.
        static std::vector<PresetEntry> list(const std::filesystem::path& shipped, const std::filesystem::path& user);
        // The user's folder: TEE_USER_DIR, else the platform's (~/Library/Application Support/TheEngineEngine/builds,
        // %APPDATA%\TheEngineEngine\builds, $XDG_DATA_HOME or ~/.local/share/TheEngineEngine/builds).
        static std::filesystem::path userDirectory();
    };
} // namespace tee
