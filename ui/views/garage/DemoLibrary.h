// ui/views/garage/DemoLibrary.h: a Library (Library.h) in memory over a FieldDoc: ten made-up builds and parts
// (whole builds shipped and the user's, an engine, a car and a tune alone), each a set of the document's field
// values, with a torque curve made up from its size and aspiration. For tests and pictures, and for GARAGE whenever
// the application gives it no library (the stage then says DEMO). No clock, no randomness, no files.
#pragma once

#include "views/garage/Library.h"

#include "kit/bind/Fields.h"

#include <cstdint>
#include <string>
#include <vector>

namespace tee::ui::garage
{
    class DemoLibrary final : public Library
    {
    public:
        // `doc` is the document builds load into; the first build (a whole one) is loaded into it now.
        explicit DemoLibrary(FieldDoc& doc);

        const std::vector<BuildSummary>& builds() const override { return builds_; }
        std::uint64_t revision() const override { return revision_; }
        void tick(float dt) override;
        bool load(const std::string& id, Part part, std::string& error) override;
        bool save(std::string& error) override;
        bool saveAs(const std::string& name, std::string& error) override;
        bool duplicate(const std::string& id, std::string& newId, std::string& error) override;
        bool remove(const std::string& id, std::string& error) override;
        bool revert(std::string& error) override;
        bool unsaved() const override;
        std::string notes() const override;
        void setNotes(const std::string& text) override;

        // Whether a section is in a part (Build.h's inPart: engine, valve, intake, boost, exhaust and fuel are the
        // engine's; car the car's; ecu the tune's; every section the whole's).
        static bool inPart(std::string_view section, Part part) noexcept;
        // A torque curve for an engine of that swept volume (m^3) and aspiration (0 natural, 1 turbo, 2 twin, 3 quad,
        // 4 supercharged; a diesel's lower and fuller).
        static DynoTrace curveFor(double displacement, int aspiration, bool diesel, double compression);

    private:
        BuildSummary* find(const std::string& id);
        int current() const;
        void takeDocument(BuildSummary& b) const;   // the document's values of b's part into b
        void refreshCurrent();
        void makeCurve(BuildSummary& b) const;

        FieldDoc& doc_;
        std::vector<BuildSummary> builds_;
        std::vector<std::vector<FieldValue>> files_;   // each build's values as last saved (its file), builds_'s order
        std::uint64_t revision_ = 1, docSeen_ = 0, savedRevision_ = 0;
        bool notesChanged_ = false;
        int made_ = 0;
    };
}
