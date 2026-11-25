// tests/ui/views/StageHelpTests.cpp: hover help on the workspaces' stages (the owner's ask, 25 Nov: "tooltips for when
// the user hovers over a dial"). U1's ui.build checks every Spec field a person sets has its ui::help, and so every
// bound control on an inspector page says something; this checks the rest: every item on every registered stage that
// the pointer can rest on (a button, a dial, a plot, a list, a lamp...) has its sentence (Widget::setHelp,
// Item::help), so the next control added without one fails here. A label takes no pointer (Label::contains is false):
// it is the text it shows.
#include "ViewKit.h"

#include "app/AppFrame.h"
#include "kit/widgets/Widget.h"

#include <string>
#include <utility>

namespace
{
    using namespace tee::test::views;

    // "" when every item the pointer can rest on says something; else a line each (its kind and title).
    std::string unhelped(tee::ui::Workspace& ws)
    {
        std::string out;
        for (int i = 0; i < ws.count(); ++i)
        {
            tee::ui::Item& it = ws.item(i);
            if (!it.shown() || !it.contains(it.centre()))
                continue;
            std::string title, body;
            funkgui::Rect anchor{};
            const bool says = it.help(title, body, anchor);
            char spec[160];
            it.specLine(spec, sizeof spec);
            if (!says || body.empty())
                out += std::string("  ") + ws.id() + ": " + spec + " has no help\n";
            else if (body == title)
                out += std::string("  ") + ws.id() + ": " + spec + "'s help only repeats its title\n";
        }
        return out;
    }

    // The shown item titled `title` (a Widget's title), or nullptr.
    tee::ui::Widget* titled(tee::ui::Workspace& ws, const std::string& title)
    {
        for (int i = 0; i < ws.count(); ++i)
            if (auto* w = dynamic_cast<tee::ui::Widget*>(&ws.item(i)); w != nullptr && w->shown() && w->title() == title)
                return w;
        return nullptr;
    }

    // The pointer rested on `title`'s item long enough for its help: the tooltip's text ("" if none showed).
    std::string hover(StageRig& rig, const std::string& title)
    {
        tee::ui::Widget* w = titled(*rig.ws, title);
        if (w == nullptr)
            return {};
        rig.host->moveTo(w->bounds().centreX(), w->bounds().centreY(), 2);
        rig.host->tick(40);
        return rig.ws->tooltip().shown() ? rig.ws->tooltip().text() : std::string();
    }

    // Every registered workspace's stage, at the smallest window's stage and the large one's, each with its own demo
    // hook; every item the pointer can rest on says what it does or shows.
    VIEW_TEST(testStagesHelp, "stages.help")
    {
        int seen = 0;
        for (const tee::ui::WorkspaceEntry& e : tee::ui::registeredWorkspaces())
            for (const FrameSize size : { FrameSize::smallest, FrameSize::large })
            {
                StageRig rig(e.make, size);
                const std::string why = unhelped(*rig.ws);
                CHECK_MSG(why.empty(), std::string("\n") + why);
                ++seen;
            }
        CHECK(seen >= 2);
        // And it shows: the pointer resting on SCOPE's FREEZE brings its line, beside it and not over it.
        for (const tee::ui::WorkspaceEntry& e : tee::ui::registeredWorkspaces())
            if (std::string(e.id) == "scope")
            {
                StageRig rig(e.make, FrameSize::smallest);
                const std::string text = hover(rig, "FREEZE");
                CHECK_MSG(text.find("Hold every plot") != std::string::npos, text);
                const funkgui::Rect box = rig.ws->tooltip().box(), at = titled(*rig.ws, "FREEZE")->bounds();
                CHECK(!(box.x < at.right() && at.x < box.right() && box.y < at.bottom() && at.y < box.bottom()));
            }
        return 0;
    }

    // Pictures, no checks (labelled nightly by its name's ".pictures"): a tooltip showing on four stages, for a person
    // to look at (out/ui/stages.help.*.png).
    VIEW_TEST(testStagesHelpPictures, "stages.help.pictures")
    {
        const std::pair<const char*, const char*> shots[] = { { "scope", "Cylinder pressure" }, { "dyno", "Steps" },
                                                              { "listen", "TUNNEL" }, { "garage", "POWER" } };
        for (const tee::ui::WorkspaceEntry& e : tee::ui::registeredWorkspaces())
            for (const auto& [id, title] : shots)
                if (std::string(e.id) == id)
                {
                    StageRig rig(e.make, FrameSize::smallest);
                    hover(rig, title);
                    picture(std::string("stages.help.") + id, *rig.host);
                }
        return 0;
    }
}
