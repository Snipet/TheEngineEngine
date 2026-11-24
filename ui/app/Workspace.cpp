// ui/app/Workspace.cpp: see Workspace.h and Look.h.
#include "app/Workspace.h"

#include "app/Look.h"

#include <funkgui/canvas/Canvas.h>

namespace tee::ui
{
    const DynoTrace& DynoSource::run(int) const
    {
        static const DynoTrace none;
        return none;
    }

    Workspace::Workspace(WorkspaceContext& context, int width, int height)
        : Board(width, height, 400, 300), ctx_(context)
    {
    }

    void Workspace::place(const funkgui::Rect& stage)
    {
        stage_ = stage;
        arrange();
        bump();
    }

    void Workspace::tick(float dt)
    {
        step(dt);
        Board::tick(dt);
    }

    void Workspace::arrange()
    {
        if (!stage_.isEmpty())
            layout(stage_);
    }

    void Workspace::drawGround(funkgui::Canvas& c, const funkgui::Theme& th) const
    {
        c.rrect(stage_.x, stage_.y, stage_.w, stage_.h, 0.0f, th.ground);
    }

    void Workspace::drawOver(funkgui::Canvas& c, const funkgui::Theme& th) const
    {
        const funkgui::Canvas::ClipScope clip(c, stage_);
        drawStage(c, th);
    }

    namespace look
    {
        void placeholder(funkgui::Canvas& c, const Palette& p, const funkgui::Rect& r, std::string_view title,
                         std::string_view body)
        {
            const float w = std::min(560.0f, r.w - 2 * kPad);
            const float x = r.centreX() - w * 0.5f;
            // Lines cut to the width at spaces.
            std::vector<std::string> lines;
            std::string line;
            std::size_t at = 0;
            while (at <= body.size())
            {
                const std::size_t sp = std::min(body.find_first_of(" \n", at), body.size());
                const std::string word(body.substr(at, sp - at));
                const std::string tried = line.empty() ? word : line + " " + word;
                if (!line.empty() && c.textWidth(tried.c_str(), kBody) > w)
                {
                    lines.push_back(line);
                    line = word;
                }
                else
                    line = tried;
                if (sp < body.size() && body[sp] == '\n')
                {
                    lines.push_back(line);
                    line.clear();
                }
                at = sp + 1;
            }
            if (!line.empty())
                lines.push_back(line);
            const float lh = 17.0f, h = 30.0f + lh * static_cast<float>(lines.size());
            float y = std::max(r.y + kPad, r.centreY() - h * 0.5f);
            c.text(std::string(title).c_str(), x, y, kTitle, p.text);
            c.rrect(x, y + 20.0f, 24.0f, 2.0f, 1.0f, p.selected);
            y += 32.0f;
            for (const std::string& l : lines)
            {
                c.text(l.c_str(), x, y, kBody, p.label);
                y += lh;
            }
        }
    }
}
