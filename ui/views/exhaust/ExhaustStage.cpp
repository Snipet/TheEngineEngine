// ui/views/exhaust/ExhaustStage.cpp: see ExhaustStage.h.
//
// Left to right, the gas's way: the heads (a bank each, the ports numbered), the primaries (each straight run to scale
// with the others, so unequal lengths look unequal), the first collectors, the second merges (4-2-1), the turbine
// housings, then each side's chain in columns shared by both sides so a crossover lies straight across: crossover at
// the head, converter, main pipe, crossover after it, resonator, muffler, tail, the radiating end. Two sides are two
// lanes (the left bank's above); one side is one lane in the middle.
#include "ExhaustStage.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tee::ui::exhaust
{
    using funkgui::Point;
    using funkgui::Rect;

    Point bezier(const Point p[4], float t)
    {
        const float u = 1.0f - t;
        const float a = u * u * u, b = 3.0f * u * u * t, c = 3.0f * u * t * t, d = t * t * t;
        return { a * p[0].x + b * p[1].x + c * p[2].x + d * p[3].x, a * p[0].y + b * p[1].y + c * p[2].y + d * p[3].y };
    }

    namespace
    {
        // A horizontal-ended S from a to b (the pipes' bends), sampled.
        void sBend(std::vector<Point>& out, Point a, Point b, int n = 14)
        {
            const float mx = (a.x + b.x) * 0.5f;
            const Point p[4] = { a, { mx, a.y }, { mx, b.y }, b };
            for (int i = out.empty() ? 0 : 1; i <= n; ++i)
                out.push_back(bezier(p, static_cast<float>(i) / static_cast<float>(n)));
        }

        std::string sideWord(const ExhaustBuild& b, int s)
        {
            return b.sides == 2 ? (s == 0 ? ", LEFT" : ", RIGHT") : "";
        }
    }

    Geo layoutExhaust(const ExhaustBuild& b, const Rect& d)
    {
        Geo g;
        g.diagram = d;
        const float W = d.w;

        // ---- the heads and the ports ----------------------------------------------------------------------------
        const float headW = 44.0f;
        g.headX = d.x + headW;
        int nb[2] = { 0, 0 };
        for (int i = 0; i < b.ports; ++i)
            nb[b.bank[static_cast<std::size_t>(i)] != 0 ? 1 : 0] += 1;
        g.nBanks = nb[1] > 0 ? 2 : 1;
        {
            int k = 0;
            for (int bank = 0; bank < 2; ++bank)
                for (int i = 0; i < b.ports; ++i)
                    if ((b.bank[static_cast<std::size_t>(i)] != 0 ? 1 : 0) == bank)
                        g.order[static_cast<std::size_t>(k++)] = i;
        }
        for (int bank = 0; bank < g.nBanks; ++bank)
        {
            const float top = g.nBanks == 2 ? (bank == 0 ? d.y : d.y + d.h * 0.54f) : d.y + d.h * 0.06f;
            const float span = g.nBanks == 2 ? d.h * 0.46f : d.h * 0.88f;
            const int n = nb[bank];
            // the ports no further apart than 46 px: a single's one port sits in the middle of its head
            const float pitch = std::min(58.0f, span / static_cast<float>(std::max(1, n)));
            const float y0 = top + (span - pitch * static_cast<float>(n)) * 0.5f;
            int k = 0;
            for (int i = 0; i < b.ports; ++i)
                if ((b.bank[static_cast<std::size_t>(i)] != 0 ? 1 : 0) == bank)
                    g.portY[static_cast<std::size_t>(i)] = y0 + pitch * (static_cast<float>(k++) + 0.5f);
            g.banks[static_cast<std::size_t>(bank)] = { d.x + 4.0f, y0 - 4.0f, headW - 4.0f, pitch * static_cast<float>(n) + 8.0f };
            const char* bankName = g.nBanks == 2 ? (bank == 0 ? "LEFT BANK" : "RIGHT BANK") : "HEAD";
            g.hits.push_back({ g.banks[static_cast<std::size_t>(bank)], "engine", bankName });
        }

        // ---- the primaries, to scale --------------------------------------------------------------------------------
        float maxL = 0.05f;
        for (int i = 0; i < b.ports; ++i)
            maxL = std::max(maxL, b.primLen[static_cast<std::size_t>(i)]);
        const bool stacks = b.openStacks();
        const float primZone = stacks ? std::max(150.0f, W * (b.ports == 1 ? 0.5f : 0.34f)) : std::max(150.0f, W * 0.23f);
        const float bend = 36.0f;
        g.scale = (primZone - bend) / maxL;
        const float coneX = g.headX + primZone, coneW = 30.0f;
        for (int i = 0; i < b.ports; ++i)
        {
            const auto k = static_cast<std::size_t>(i);
            g.primEnd[k] = g.headX + b.primLen[k] * g.scale;
            const float y = g.portY[k];
            char name[48];
            std::snprintf(name, sizeof name, "PRIMARY %d", i + 1);
            g.hits.push_back({ { g.headX, y - 7.0f, g.primEnd[k] - g.headX + (b.portTo[k] < 0 ? 24.0f : 0.0f), 14.0f },
                               "exhaust.primary[" + std::to_string(i) + "]", name });
        }

        // ---- the first collectors and the merges ------------------------------------------------------------------
        const bool merges = b.anyMerge();
        const float mergeX = coneX + coneW + 46.0f, mergeW = 24.0f;
        for (int c = 0; c < kCollectors; ++c)
        {
            const auto k = static_cast<std::size_t>(c);
            if (!b.colOn[k] || b.colFed[k])
                continue;
            float lo = 1e9f, hi = -1e9f;
            for (int i = 0; i < b.ports; ++i)
                if (b.portTo[static_cast<std::size_t>(i)] == c)
                {
                    lo = std::min(lo, g.portY[static_cast<std::size_t>(i)]);
                    hi = std::max(hi, g.portY[static_cast<std::size_t>(i)]);
                }
            if (lo > hi)
                continue;
            g.colY[k] = (lo + hi) * 0.5f;
            const bool log = b.colVol[k] > 0.0f;
            g.colH[k] = log ? hi - lo + 18.0f : std::clamp(hi - lo, 14.0f, 40.0f);
            g.colX[k] = log ? coneX - 10.0f : coneX;
            g.colExit[k] = log ? coneX + 8.0f : coneX + coneW;
            char name[48];
            std::snprintf(name, sizeof name, log ? "LOG MANIFOLD %d" : "COLLECTOR %d", c + 1);
            g.hits.push_back({ { g.colX[k] - 2.0f, g.colY[k] - g.colH[k] * 0.5f - 2.0f, g.colExit[k] - g.colX[k] + 4.0f,
                                 g.colH[k] + 4.0f },
                               "exhaust.collector[" + std::to_string(c) + "]", name });
        }
        // first collectors that would overlap (a 4-2-1's pairs, 180-degree headers' crossing sets) are spread apart,
        // in the order of their ports, so each cone stands on its own
        {
            std::vector<int> cs;
            for (int c = 0; c < kCollectors; ++c)
                if (b.colOn[static_cast<std::size_t>(c)] && !b.colFed[static_cast<std::size_t>(c)] && g.colExit[static_cast<std::size_t>(c)] > 0)
                    cs.push_back(c);
            std::stable_sort(cs.begin(), cs.end(), [&](int a, int c2) {
                return g.colY[static_cast<std::size_t>(a)] < g.colY[static_cast<std::size_t>(c2)];
            });
            for (int pass = 0; pass < 8; ++pass)
                for (std::size_t i = 1; i < cs.size(); ++i)
                {
                    const auto a = static_cast<std::size_t>(cs[i - 1]), c2 = static_cast<std::size_t>(cs[i]);
                    const float need = (g.colH[a] + g.colH[c2]) * 0.5f + 26.0f;
                    const float have = g.colY[c2] - g.colY[a];
                    if (have < need)
                    {
                        const float push = (need - have) * 0.5f;
                        g.colY[a] -= push;
                        g.colY[c2] += push;
                    }
                }
            for (const int c : cs)
            {
                const auto k = static_cast<std::size_t>(c);
                for (Hit& h : g.hits)
                    if (h.id == "exhaust.collector[" + std::to_string(c) + "]")
                        h.r.y = g.colY[k] - g.colH[k] * 0.5f - 2.0f;
            }
        }
        for (int c = 0; c < kCollectors; ++c)
        {
            const auto k = static_cast<std::size_t>(c);
            if (!b.colOn[k] || !b.colFed[k])
                continue;
            float sum = 0;
            int n = 0;
            for (int j = 0; j < kCollectors; ++j)
                if (b.colOn[static_cast<std::size_t>(j)] && b.colTo[static_cast<std::size_t>(j)] == c + 2)
                {
                    sum += g.colY[static_cast<std::size_t>(j)];
                    n += 1;
                }
            g.colY[k] = n > 0 ? sum / static_cast<float>(n) : d.y + d.h * 0.5f;
            g.colH[k] = 18.0f;
            g.colX[k] = mergeX;
            g.colExit[k] = mergeX + mergeW;
            char name[48];
            std::snprintf(name, sizeof name, "MERGE %d", c + 1);
            g.hits.push_back({ { g.colX[k] - 2.0f, g.colY[k] - 11.0f, mergeW + 4.0f, 22.0f },
                               "exhaust.collector[" + std::to_string(c) + "]", name });
        }

        // ---- the sides' heads and the turbines ----------------------------------------------------------------------
        g.laneX = (merges ? mergeX + mergeW : coneX + coneW) + 44.0f;
        if (b.sides == 2)
            g.laneY = { d.y + d.h * 0.25f, d.y + d.h * 0.75f };
        else
        {
            // one side: level with the first collector in use, so the pipe runs straight on
            float y = d.y + d.h * 0.5f;
            for (int c = 0; c < kCollectors; ++c)
                if (b.colOn[static_cast<std::size_t>(c)] && b.colTo[static_cast<std::size_t>(c)] < 2)
                {
                    y = g.colY[static_cast<std::size_t>(c)];
                    break;
                }
            g.laneY = { y, y };
        }
        float chainX = g.laneX + 12.0f;
        g.chainX = g.laneX;
        if (b.turbos != Turbos::none && !stacks)
        {
            const float r = 17.0f, x = g.laneX + 26.0f;
            const float mid = (g.laneY[0] + g.laneY[b.sides - 1]) * 0.5f;
            switch (b.turbos)
            {
                case Turbos::one: g.housings.push_back({ x, mid, r, 0 }); break;
                case Turbos::sequential:
                    g.housings.push_back({ x, mid - 22.0f, r * 0.85f, 0 });
                    g.housings.push_back({ x, mid + 22.0f, r * 0.85f, b.sides - 1 });
                    break;
                case Turbos::compound:
                    g.housings.push_back({ x, mid, r, 0 });
                    g.housings.push_back({ x + 44.0f, mid, r * 0.75f, 0 });
                    break;
                case Turbos::perBank:
                case Turbos::fourPerBank:
                    for (int s = 0; s < 2; ++s)
                    {
                        const float y = b.sides == 2 ? g.laneY[static_cast<std::size_t>(s)] : mid + (s == 0 ? -24.0f : 24.0f);
                        if (b.turbos == Turbos::perBank)
                            g.housings.push_back({ x, y, r * 0.9f, std::min(s, b.sides - 1) });
                        else
                        {
                            g.housings.push_back({ x, y - 14.0f, r * 0.7f, std::min(s, b.sides - 1) });
                            g.housings.push_back({ x, y + 14.0f, r * 0.7f, std::min(s, b.sides - 1) });
                        }
                    }
                    break;
                case Turbos::none: break;
            }
            float right = x;
            for (const Housing& h : g.housings)
                right = std::max(right, h.x + h.r);
            // a housing that feeds both sides needs room for its downpipes to part
            const bool parts = b.sides == 2 && (b.turbos == Turbos::one || b.turbos == Turbos::sequential || b.turbos == Turbos::compound);
            chainX = right + (parts ? 64.0f : 22.0f);
            g.chainX = chainX;
            for (std::size_t i = 0; i < g.housings.size(); ++i)
            {
                const Housing& h = g.housings[i];
                char name[32];
                std::snprintf(name, sizeof name, g.housings.size() > 1 ? "TURBINE %d" : "TURBINE", static_cast<int>(i) + 1);
                g.hits.push_back({ { h.x - h.r - 2.0f, h.y - h.r - 2.0f, 2.0f * h.r + 4.0f, 2.0f * h.r + 4.0f }, "boost", name });
            }
        }

        // ---- the chain's columns, shared by both sides --------------------------------------------------------------
        bool anyConv = false, anyRes = false, anyMuf = false;
        for (int s = 0; s < b.sides; ++s)
        {
            const auto k = static_cast<std::size_t>(s);
            if (!b.sideOn[k])
                continue;
            anyConv = anyConv || b.conv[k] != 0;
            anyRes = anyRes || b.res[k] != 0;
            anyMuf = anyMuf || b.muf[k] != 0;
        }
        const bool chain = b.sideOn[0] || b.sideOn[1];
        const float endW = 118.0f, gap = 16.0f;
        float fixed = endW;
        if (b.cross != 0)
            fixed += 54.0f + gap;
        if (anyConv)
            fixed += 58.0f + gap;
        if (anyRes)
            fixed += 54.0f + gap;
        if (anyMuf)
            fixed += 112.0f + gap;
        const float flex = std::max(80.0f, d.right() - chainX - fixed);
        float x = chainX;
        if (chain)
        {
            if (b.cross != 0 && b.crossAt == 0)
            {
                g.crossX0 = x;
                g.crossX1 = x + 54.0f;
                x = g.crossX1 + gap;
            }
            if (anyConv)
            {
                g.convX0 = x;
                g.convX1 = x + 58.0f;
                x = g.convX1 + gap;
            }
            g.mainX0 = x;
            g.mainX1 = x + flex * 0.62f;
            x = g.mainX1;
            if (b.cross != 0 && b.crossAt == 1)
            {
                g.crossX0 = x + gap;
                g.crossX1 = x + gap + 54.0f;
                x = g.crossX1;
            }
            if (anyRes)
            {
                g.resX0 = x + gap;
                g.resX1 = x + gap + 54.0f;
                x = g.resX1;
            }
            if (anyMuf)
            {
                g.mufX0 = x + gap;
                g.mufX1 = x + gap + 112.0f;
                x = g.mufX1;
            }
            g.tailX0 = x;
            g.tailX1 = x + flex * 0.38f;
            g.endX = g.tailX1;
        }
        for (int s = 0; s < b.sides && chain; ++s)
        {
            const auto k = static_cast<std::size_t>(s);
            if (!b.sideOn[k])
                continue;
            const float y = g.laneY[k];
            const std::string i = "[" + std::to_string(s) + "]";
            if (b.conv[k] != 0)
                g.hits.push_back({ { g.convX0, y - 15.0f, g.convX1 - g.convX0, 30.0f }, "exhaust.converter" + i,
                                   (b.conv[k] == 2 ? "HIGH-FLOW CONVERTER" : "CONVERTER") + sideWord(b, s) });
            g.hits.push_back({ { g.mainX0, y - 9.0f, g.mainX1 - g.mainX0, 18.0f }, "exhaust.main" + i, "MAIN PIPE" + sideWord(b, s) });
            if (b.res[k] != 0)
            {
                static const char* const kRes[] = { "", "QUARTER-WAVE", "HELMHOLTZ", "PACKED BOTTLE" };
                const float out = b.sides == 2 && s == 1 ? 1.0f : -1.0f;   // stubs point away from the other side
                const float top = out < 0 ? y - 46.0f : y - 12.0f;
                g.hits.push_back({ { g.resX0, top, g.resX1 - g.resX0, 58.0f }, "exhaust.resonator" + i,
                                   std::string(kRes[b.res[k]]) + sideWord(b, s) });
            }
            if (b.muf[k] != 0)
            {
                static const char* const kMuf[] = { "", "STRAIGHT-THROUGH", "CHAMBERED", "THREE-PASS" };
                g.hits.push_back({ { g.mufX0, y - 19.0f, g.mufX1 - g.mufX0, 38.0f }, "exhaust.muffler" + i,
                                   std::string(kMuf[b.muf[k]]) + " MUFFLER" + sideWord(b, s) });
                if (b.byp[k] != 0)
                {
                    const float out = b.sides == 2 && s == 1 ? 1.0f : -1.0f;
                    const float vy = y + out * 36.0f, vx = (g.mufX0 + g.mufX1) * 0.5f;
                    g.hits.push_back({ { vx - 12.0f, vy - 12.0f, 24.0f, 24.0f }, "exhaust.bypass" + i, "BYPASS VALVE" + sideWord(b, s) });
                }
            }
            g.hits.push_back({ { g.tailX0, y - 12.0f, g.endX - g.tailX0 + 28.0f, 24.0f }, "exhaust.tail" + i, "TAIL" + sideWord(b, s) });
        }
        if (b.cross != 0)
            g.hits.push_back({ { g.crossX0, g.laneY[0] - 8.0f, g.crossX1 - g.crossX0, g.laneY[1] - g.laneY[0] + 16.0f },
                               "exhaust.crossover", b.cross == 2 ? "X-PIPE" : "H-PIPE" });

        // ---- the Scope's path: cylinder 1's primary, its collectors, its side's chain to the first tail ------------
        if (b.ports > 0)
        {
            const int c0 = b.portTo[0];
            Piece prim;
            prim.metres = b.primLen[0];
            prim.pts.push_back({ g.headX, g.portY[0] });
            prim.pts.push_back({ g.primEnd[0], g.portY[0] });
            if (c0 >= 0)
                sBend(prim.pts, { g.primEnd[0], g.portY[0] }, { g.colX[static_cast<std::size_t>(c0)], g.colY[static_cast<std::size_t>(c0)] });
            g.path.push_back(prim);
            int c = c0;
            while (c >= 0)
            {
                const auto k = static_cast<std::size_t>(c);
                Piece pc;
                pc.metres = b.colLen[k];
                pc.pts.push_back({ g.colX[k], g.colY[k] });
                const int to = b.colTo[k];
                if (to >= 2)
                {
                    sBend(pc.pts, { g.colExit[k], g.colY[k] }, { g.colX[static_cast<std::size_t>(to - 2)], g.colY[static_cast<std::size_t>(to - 2)] });
                    g.path.push_back(pc);
                    c = to - 2;
                    continue;
                }
                const auto s = static_cast<std::size_t>(std::min(to, b.sides - 1));
                if (!b.sideOn[s])
                    break;
                sBend(pc.pts, { g.colExit[k], g.colY[k] }, { g.laneX, g.laneY[s] });
                g.path.push_back(pc);
                const float y = g.laneY[s];
                Piece main;
                main.metres = b.mainLen[s];
                main.pts.push_back({ g.laneX, y });
                main.pts.push_back({ g.mainX1, y });
                g.path.push_back(main);
                Piece link;
                link.metres = (b.cross != 0 && b.crossAt == 1 ? 0.4f : 0.0f) + (b.res[s] != 0 ? 0.4f : 0.0f) + 0.05f;
                link.pts.push_back({ g.mainX1, y });
                link.pts.push_back({ g.mufX0 > 0 ? g.mufX0 : g.tailX0, y });
                g.path.push_back(link);
                if (g.mufX0 > 0)
                {
                    Piece muf;
                    muf.metres = b.muf[s] != 0 ? b.mufLen[s] * 2.0f : 0.05f;
                    muf.pts.push_back({ g.mufX0, y });
                    muf.pts.push_back({ g.mufX1, y });
                    g.path.push_back(muf);
                }
                Piece tail;
                tail.metres = b.tailLen[s];
                tail.pts.push_back({ g.tailX0, y });
                tail.pts.push_back({ g.endX, y });
                g.path.push_back(tail);
                break;
            }
            for (const Piece& p : g.path)
                g.pathMetres += p.metres;
        }
        return g;
    }
}
