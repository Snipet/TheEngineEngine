// ui/kit/inspect/PartCard.h: the card on the picture (the owner, 25 Nov: Rows now, "the card on the picture next"):
// press a part on the stage and a card opens beside it with the part's first fields, typed, scrubbed and nudged as in
// the rows. An addition to Rows, not a replacement: the column still holds every field of the part.
//
// The frame owns the card: a FieldInspector of Style::list on its own board (drawn over the stage), which PartCard
// opens, places and closes. The frame tells it of each press on the stage (afterPress, with the selection's path
// before and after the press and the part under the pointer), then, once its pages are made for the selection, gives
// it the shown page's outline (update). A press that selects a part (or presses the part already selected) opens the
// card there; a press on no part, the selection cleared, Escape or another workspace closes it.
//
// No workspace changes for a card opened by a press: the press says where the part is. (A card opened from the
// keyboard or the column would need each workspace to say where its parts are; not built.)
#pragma once

#include "kit/inspect/FieldInspector.h"

#include <string>

namespace tee::ui
{
    class PartCard
    {
    public:
        static constexpr float kWidth = 300.0f;
        static constexpr int kFields = 6;    // the first fields of the part's page
        static constexpr float kGap = 56.0f; // at least this from the press to the card's side

        explicit PartCard(FieldInspector& card);   // the card's FieldInspector (Style::list), hidden until opened

        // A press on the stage at `at` ended in `pathNow` selected (`pathBefore` before it; `hovered`: the part under
        // the pointer). Remembers whether a card is wanted there; closes it when the press chose nothing.
        void afterPress(const std::string& pathBefore, const std::string& pathNow, const std::string& hovered,
                        funkgui::Point at);
        // Once the frame's pages are made for the selection: opens the card wanted (page: the outline of the
        // selection's first page, as its rows show it) inside the stage. Closes it when the selection is no longer
        // what it was opened for.
        void update(const std::string& path, const Outline* page, const funkgui::Rect& stage);
        // At every layout: in the half of the stage away from the press, against its far edge, inside the stage.
        void place(const funkgui::Rect& stage);
        void close();
        bool isOpen() const noexcept { return open_; }
        const std::string& path() const noexcept { return path_; }

        // "engine.cylinder[2]": CYLINDER 3; "exhaust.header": HEADER.
        static std::string titleOf(const std::string& path);
        // The first `n` fields of an outline (headings and notes dropped; a table counts as one).
        static Outline firstFields(const Outline& page, int n);

    private:
        FieldInspector& card_;
        bool open_ = false, wanted_ = false;
        std::string path_, wantedPath_;
        funkgui::Point at_{};
    };
}
