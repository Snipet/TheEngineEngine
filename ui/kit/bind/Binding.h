// ui/kit/bind/Binding.h: a Spec's fields bound to controls (card J2, item 8).
//
//   FieldPort     FunkGui's ParamPort over one field of a FieldDoc: 0..1 by its range and taper, steps for a choice,
//                 an integer and a switch, its default, its text through Units, and every gesture bracketed in the
//                 document (one step of undo a drag). For a widget placed by C++.
//   FieldParams   the fields of a page as a tee::ParamSource, in the user's units (a temperature in C has an offset, so
//                 the values are converted here rather than scaled by the panel), and a GestureSink: J1's GuiPanel
//                 binds a .gui file to it as to any ParamSource, a control naming its field by path (`knob boost.level`),
//                 and each control's gestures reach the document.
//   pageGui       the generated page of some fields: a section a ui::group (in the order they come), a control a field
//                 by its kind (a knob, a menu, a switch; a MapEdit, a CurveEdit), as .gui text on one line. `auto
//                 "boost"` in a .gui file stands for the page of the section boost (GuiPanel::Options::autoPage).
//   fieldWidget   the items of the .gui widget elements: map and curve of a field; gauge, bar, lamp and plot of a
//                 telemetry value (a plot of one: its last ten seconds).
//   FieldPage     a GuiPanel over a .gui text bound to a FieldDoc: what the application's screens are made of.
//
// UI thread only.
#pragma once

#include "kit/bind/Fields.h"
#include "kit/bind/Units.h"
#include "kit/gui/GuiPanel.h"
#include "kit/gui/Ports.h"
#include "kit/widgets/Plot.h"

#include <funkgui/params/ParamPort.h>
#include <tee/app/Sources.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tee::ui
{
    // A field's value as 0..1 of its control's travel, and back (linear, or by decades under ui::taper("log")).
    float norm01(const FieldInfo& f, double si) noexcept;
    double plainOf01(const FieldInfo& f, float v01) noexcept;
    // A field's value as a person reads it: a choice's name, ON or OFF, a number in the user's unit (or the field's
    // fixed unit).
    std::string fieldText(const FieldInfo& f, double si, const Units& units);
    // Typed text to SI: a number in the user's unit (or any unit of the quantity, named), a choice's name or index,
    // on/off. False: not a value of this field.
    bool parseField(const FieldInfo& f, std::string_view text, const Units& units, double& si);

    class FieldPort final : public funkgui::ParamPort
    {
    public:
        FieldPort(FieldDoc& doc, int field, const Units& units);

        float value01() const override;
        float default01() const override;
        int numSteps() const override;
        void beginGesture() override;
        void setValue01(float v) override;
        void endGesture() override;
        const char* id() const override { return path_.c_str(); }
        void* native() const override { return nullptr; }

        int field() const noexcept { return field_; }
        const FieldInfo& info() const { return doc_.info(field_); }
        std::string text(float v01) const;               // the value at v01, as fieldText says it
        int gesturesOpen() const noexcept { return open_; }

    private:
        FieldDoc& doc_;
        int field_;
        const Units& units_;
        std::string path_;
        int open_ = 0;
    };

    class FieldParams final : public ParamSource, public GestureSink
    {
    public:
        // `fields`: the document's indexes of the fields it offers (a table's are left out: a page's map and curve bind
        // through fieldWidget). Their order is the parameters' order.
        FieldParams(FieldDoc& doc, const Units& units, std::vector<int> fields);

        int count() const override { return static_cast<int>(fields_.size()); }
        const ParamInfo& info(int i) const override { return infos_[static_cast<std::size_t>(i)]; }
        double get(int i) const override;
        void set(int i, double value) override;
        int findParam(std::string_view id) const override;
        void beginGesture(int) override { doc_.beginGesture(); }
        void endGesture(int) override { doc_.endGesture(); }

        int field(int i) const noexcept { return fields_[static_cast<std::size_t>(i)]; }
        // The units changed: every parameter described again (a panel bound to it must be made again).
        void refresh();

    private:
        FieldDoc& doc_;
        const Units& units_;
        std::vector<int> fields_;
        std::vector<ParamInfo> infos_;
    };

    // How a generated page shows an array of numbers: up to kArrayKnobs elements a control each (a knob, a menu, a
    // switch, in the group's row); from kArrayKnobs + 1 to kTableMax elements one row of cells (the `array` element: a
    // MapEdit of one row, typed or dragged); more than that a control each again.
    inline constexpr int kArrayKnobs = 8;
    bool arrayAsTable(const FieldInfo& f) noexcept;
    // The fields a widget element shows: a map's or a curve's field, every element of an `array`'s array; none else.
    std::vector<int> fieldsOfWidget(const FieldDoc& doc, const GuiNode& node);

    // The fields of `section` a generated page shows (not hidden), in the Spec's order.
    std::vector<int> sectionFields(const FieldDoc& doc, std::string_view section);
    // Those cut into pages of at most `maxControls` controls (a table counts as none), a group kept whole when it fits.
    std::vector<std::vector<int>> splitPages(const FieldDoc& doc, const std::vector<int>& fields, int maxControls = 60);
    // The generated page of `fields`: .gui text on one line.
    std::string pageGui(const FieldDoc& doc, const std::vector<int>& fields, std::string_view title = {});
    // The fields a .gui text names (a control's or a widget's path, a word that is a field's path) and every field of
    // each section it names with `auto`, in the document's order, scalars first.
    std::vector<int> fieldsOfGui(const FieldDoc& doc, std::string_view gui);

    // A telemetry value as the widgets show it: its quantity, and the range a gauge spans (SI).
    struct TelemetryLook
    {
        Quantity quantity = Quantity::none;
        double lo = 0.0, hi = 1.0;
        std::string label;
    };
    TelemetryLook telemetryLook(std::string_view name);

    // The item of a .gui widget element (GuiPanel::Options::widget), bound to `doc` and `telemetry`; nullptr when its
    // name is nothing of theirs.
    std::unique_ptr<Item> fieldWidget(const GuiNode& node, uint32_t id, FieldDoc& doc, const Units& units,
                                      TelemetrySource& telemetry);

    namespace detail
    {
        // FieldPage's parameters, made before the GuiPanel that binds to them.
        struct FieldPageParams
        {
            FieldParams fieldParams;
            FieldPageParams(FieldDoc& doc, const Units& units, std::vector<int> fields)
                : fieldParams(doc, units, std::move(fields))
            {
            }
        };
    }

    // A GuiPanel over `files` (a .gui file and its shaders) whose controls name fields of `doc` by path, with `auto`
    // pages and the widget elements bound (options.widget and options.autoPage are set unless the caller set them).
    class FieldPage final : private detail::FieldPageParams, public GuiPanel
    {
    public:
        FieldPage(FieldDoc& doc, const Units& units, TelemetrySource& telemetry, Sources files, Options options);

        FieldParams& params() noexcept { return fieldParams; }

    private:
        static std::vector<int> fieldsOf(const FieldDoc& doc, const Sources& files);
        static Options withBinding(Options o, FieldDoc& doc, const Units& units, TelemetrySource& telemetry);
    };
}
