// ui/views/exhaust/ExhaustStage.h: the geometry of the EXHAUST stage: where each part of the system as built is drawn
// (ExhaustModel.h), what a press there selects, and the path the Scope's pipe pressure is taken along. Made again when
// the build or the stage changes; the workspace draws from it and hit-tests against it.
#pragma once

#include "ExhaustModel.h"

#include <funkgui/core/Geometry.h>

#include <string>
#include <vector>

namespace tee::ui::exhaust
{
    // The heat sections of sim/exhaust/heat2.fm, in the gas's order.
    enum Section : int { kPrim = 0, kColl, kMerge, kMain, kLink, kMuff, kTail, kSections };

    // Something a press selects: its bounds, its selection path, and the name shown beside its ring.
    struct Hit
    {
        funkgui::Rect r;
        std::string id, name;
    };

    // One stretch of the Scope's path: its centreline on the stage, and its length in the pipe (m).
    struct Piece
    {
        std::vector<funkgui::Point> pts;
        float metres = 0.0f;
    };

    struct Housing
    {
        float x = 0, y = 0, r = 0;
        int side = 0;
    };

    struct Geo
    {
        funkgui::Rect diagram{};
        float headX = 0;                                   // the ports' face
        std::array<float, kPorts> portY{}, primEnd{};      // each primary's straight run ends at primEnd
        std::array<int, kPorts> order{};                   // the ports from the top
        std::array<funkgui::Rect, 2> banks{};              // the heads
        int nBanks = 1;
        std::array<float, kCollectors> colX{}, colY{}, colH{}, colExit{};   // the cone's mouth, its height, its exit
        float laneX = 0;                                   // the sides' heads
        float chainX = 0;                                  // where the sides' chains start (after the housings)
        std::array<float, kSides> laneY{};
        std::vector<Housing> housings;
        // the chain's columns (x0, x1); -1 when the build has none
        float crossX0 = -1, crossX1 = -1, convX0 = -1, convX1 = -1, mainX0 = 0, mainX1 = 0, resX0 = -1, resX1 = -1;
        float mufX0 = -1, mufX1 = -1, tailX0 = 0, tailX1 = 0, endX = 0;
        float scale = 0;                                   // px a metre of primary
        std::vector<Hit> hits;
        std::vector<Piece> path;                           // cylinder 1's: primary to tail
        float pathMetres = 0;
    };

    Geo layoutExhaust(const ExhaustBuild& b, const funkgui::Rect& diagram);

    // A cubic's point at t.
    funkgui::Point bezier(const funkgui::Point p[4], float t);
}
