// ui/app/Workspace.cpp: see Workspace.h and Look.h.
#include "app/Workspace.h"

#include "app/Look.h"

#include <funkgui/canvas/Canvas.h>

#include <cmath>
#include <vector>

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
        setTabWraps(false);   // in the frame, Tab goes on from the stage's last control to the frame's
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
        void pictogram(funkgui::Canvas& c, std::string_view id, float cx, float cy, float size, Col ink)
        {
            const float h = size * 0.5f, w = 1.25f;
            const float x0 = cx - h, y0 = cy - h, x1 = cx + h, y1 = cy + h;
            if (id == "garage")   // a roof over a door
            {
                const float xs[] = { x0, cx, x1 }, ys[] = { cy - h * 0.1f, y0, cy - h * 0.1f };
                c.polyline(xs, ys, 3, w, ink);
                c.rrect(cx - h * 0.7f, cy - h * 0.1f, h * 1.4f, h * 1.1f, 0.0f, kClear, w, ink);
                c.segment(cx, cy + h * 0.25f, cx, y1, w, ink);
            }
            else if (id == "engine")   // a block of four bores
            {
                c.rrect(x0, cy - h * 0.55f, size, h * 1.1f, 2.0f, kClear, w, ink);
                for (int k = 0; k < 4; ++k)
                    c.disc(x0 + size * (0.17f + 0.22f * static_cast<float>(k)), cy, size * 0.08f, kClear, w, ink);
            }
            else if (id == "valves")   // a poppet valve: a head and its stem
            {
                c.segment(cx, y0, cx, cy + h * 0.4f, w, ink);
                const float xs[] = { cx - h * 0.8f, cx - h * 0.3f, cx + h * 0.3f, cx + h * 0.8f };
                const float ys[] = { y1, cy + h * 0.45f, cy + h * 0.45f, y1 };
                c.polyline(xs, ys, 4, w, ink);
            }
            else if (id == "air")   // three streams
            {
                for (int k = 0; k < 3; ++k)
                {
                    const float y = y0 + size * (0.2f + 0.3f * static_cast<float>(k));
                    const funkgui::Point p[4] = { { x0, y }, { cx - h * 0.3f, y - h * 0.3f }, { cx + h * 0.3f, y + h * 0.3f }, { x1, y } };
                    c.cubic(p, w, ink);
                }
            }
            else if (id == "exhaust")   // a pipe and two puffs
            {
                c.segment(x0, cy + h * 0.4f, cx, cy + h * 0.4f, w * 2.0f, ink);
                c.disc(cx + h * 0.35f, cy + h * 0.1f, h * 0.25f, kClear, w, ink);
                c.disc(x1 - h * 0.2f, cy - h * 0.45f, h * 0.35f, kClear, w, ink);
            }
            else if (id == "tune")   // a map's grid
            {
                c.rrect(x0, y0, size, size, 1.0f, kClear, w, ink);
                for (int k = 1; k < 3; ++k)
                {
                    const float t = static_cast<float>(k) / 3.0f;
                    c.segment(x0 + size * t, y0, x0 + size * t, y1, 1.0f, ink);
                    c.segment(x0, y0 + size * t, x1, y0 + size * t, 1.0f, ink);
                }
            }
            else if (id == "car")   // the car, small (Look's profile)
            {
                float xs[kCarPoints], ys[kCarPoints];
                const float r = size * 0.14f;
                carProfile(cx + h * 0.55f, cx - h * 0.55f, cy + h * 0.6f, r, 0.0f, xs, ys);
                c.polyline(xs, ys, kCarPoints, w, ink);
                c.disc(cx + h * 0.55f, cy + h * 0.6f, r, kClear, w, ink);
                c.disc(cx - h * 0.55f, cy + h * 0.6f, r, kClear, w, ink);
            }
            else if (id == "dyno")   // a torque curve on its axes
            {
                c.segment(x0, y0, x0, y1, 1.0f, ink);
                c.segment(x0, y1, x1, y1, 1.0f, ink);
                const funkgui::Point p[4] = { { x0 + 2.0f, y1 - h * 0.4f }, { cx - h * 0.2f, y0 - h * 0.2f }, { cx + h * 0.2f, y0 },
                                     { x1, cy } };
                c.cubic(p, w, ink);
            }
            else if (id == "drive")   // a dial and its needle
            {
                c.arc({ cx, cy + h * 0.2f }, h * 0.9f, 2.6f, 6.8f, w, ink);
                c.segment(cx, cy + h * 0.2f, cx + h * 0.55f, cy - h * 0.35f, w, ink);
            }
            else if (id == "scope")   // a trace on a screen
            {
                c.rrect(x0, y0, size, size, 2.0f, kClear, 1.0f, ink);
                float xs[9], ys[9];
                for (int k = 0; k < 9; ++k)
                {
                    xs[k] = x0 + 2.0f + (size - 4.0f) * static_cast<float>(k) / 8.0f;
                    ys[k] = cy - h * 0.5f * std::sin(static_cast<float>(k) * 0.785f);
                }
                c.polyline(xs, ys, 9, w, ink);
            }
            else
                c.rrect(cx - h * 0.5f, cy - h * 0.5f, h, h, 1.0f, kClear, w, ink);
        }

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
