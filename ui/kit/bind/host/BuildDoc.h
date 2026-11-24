// ui/kit/bind/host/BuildDoc.h: card C's tee::Build (host/include/tee/Build.h) as the binding's FieldDoc (card J2,
// item 8), and the frame's services over it (the garage's builds, undo, the name). Built only where the host library
// is (the root's build: target tee_ui_build); `ui/` alone has MemoryDoc instead.
//
// What it maps:
//   - the Spec's fields (block "spec") of the kinds a person sets: a number, an integer, a switch, a choice, a curve, a
//     map; an array of numbers as one field an element ("sound.harmonics[2]"); a section's `rev` never. Paths lose
//     their "spec." ("boost.level"), as the .gui files and the generated pages name them.
//   - the attributes (02-contracts.md section 3) into FieldInfo; a range the Spec does not give is 0..1.
//   - a table's axes: ui::axes' quantities; their ranges, which the Spec does not give, a quantity's usual span
//     (axisRange) or, for a quantity with none, its points' own span and half again.
//   - a map's z: Build's z[i][j] at x[i], y[j]; MapEdit's z[row j][column i]. Transposed here, both ways.
//   - revision(): Build::hash(), which moves with every value, undo and load included.
#pragma once

#include "app/AppFrame.h"
#include "kit/bind/Fields.h"

#include "tee/Build.h"

#include <filesystem>
#include <string>
#include <vector>

namespace tee::ui
{
    // The span a table's axis of that quantity usually takes (SI), for its editor's range. False: no usual span.
    bool axisRange(Quantity q, double& lo, double& hi) noexcept;

    class BuildDoc final : public FieldDoc
    {
    public:
        explicit BuildDoc(tee::Build& build);

        int count() const override { return static_cast<int>(infos_.size()); }
        const FieldInfo& info(int i) const override { return infos_[static_cast<std::size_t>(i)]; }
        double get(int i) const override;
        void set(int i, double si) override;
        Curve16 curve(int i) const override;
        void setCurve(int i, const Curve16& c) override;
        Map16 map(int i) const override;
        void setMap(int i, const Map16& m) override;
        void beginGesture() override { build_.beginGesture(); }
        void endGesture() override { build_.endGesture(); }
        std::uint64_t revision() const override { return build_.hash(); }

        tee::Build& build() noexcept { return build_; }

    private:
        struct Source
        {
            std::string path;          // the Build's ("spec.boost.level")
            int field = -1;            // in Fields::all()
            std::size_t index = 0;     // an array's element
        };
        tee::Build& build_;
        std::vector<FieldInfo> infos_;
        std::vector<Source> sources_;
    };

    // The frame's services over a build: its name (rename), undo and redo, the garage's builds (the shipped ones and
    // the user's: tee::Presets), loading one (one undo step) and saving as a name into the user's folder.
    FrameServices buildServices(tee::Build& build, std::filesystem::path shipped, std::filesystem::path user,
                                AppServices app);
}
