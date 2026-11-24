// ui/kit/gui/Ports.h: what a panel's controls are bound to. Ours (not from TestingPlayground): it stands where the
// plugin's host parameters stood. A control on a panel turns a funkgui::ParamPort, a value 0..1; SourcePort is that
// port over one parameter of a tee::ParamSource (card B's interface: shell/include/tee/app/Sources.h), and controlOf()
// is the parameter described as the panel code wants it (kit/gui/Controls.h). Card J2's FieldPort (a Spec field of a
// tee::Build) is another port of the same kind; the panel code cannot tell them apart.
//
// UI thread only, as the ParamSource is.
#pragma once

#include "kit/gui/Controls.h"

#include <funkgui/params/ParamPort.h>
#include <tee/app/Sources.h>

#include <string>
#include <vector>

namespace tee::ui
{
    // `info` as a Control: its range, taper, unit, decimals and scale; a choice is an integer whose options are its
    // choices' names (optionsOf).
    Control controlOf(const ParamInfo& info);
    // A choice's names, cut to what a panel shows (kMaxOptionLabel characters); none for another kind.
    std::vector<std::string> optionsOf(const ParamInfo& info);

    // J2: a ParamSource that also wants each control's gestures (a document's undo: one step a drag) implements this as
    // well; SourcePort finds it and brackets its writes with it.
    struct GestureSink
    {
        virtual ~GestureSink() = default;
        virtual void beginGesture(int index) = 0;
        virtual void endGesture(int index) = 0;
    };

    class SourcePort final : public funkgui::ParamPort
    {
    public:
        SourcePort(ParamSource& source, int index);

        float value01() const override;
        float default01() const override;
        int numSteps() const override;                   // 0 for a real one, else its values less one
        void beginGesture() override;
        void setValue01(float v) override;               // ParamSource::set, in the parameter's own units
        void endGesture() override;
        const char* id() const override { return id_.c_str(); }
        void* native() const override { return nullptr; }

        int index() const noexcept { return index_; }
        const Control& control() const noexcept { return control_; }
        // The control as a .gui file left it (its taper=, unit=, decimals=, scale=): the port maps 0..1 by it.
        void setControl(const Control& c) noexcept { control_ = c; }
        // Gestures begun and ended so far (a test's: every begin has its end).
        int gesturesBegun() const noexcept { return gestures_; }
        int gesturesEnded() const noexcept { return ended_; }

    private:
        ParamSource& source_;
        GestureSink* sink_ = nullptr;
        int index_;
        Control control_;
        std::string id_;
        int gestures_ = 0, ended_ = 0;
    };
}
