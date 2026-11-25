// assistant/include/tee/assistant/Cards.h: what Claude changed, as the app shows it: a card for each call that changed
// the build or the library, with an Undo that is true (docs/plan/w3-a-assistant.md, "Undo that is true"). The view of
// the cards is the interface's (U1's frame); this is their model.
//
// A card of changed fields carries the build's undo step its batch made (Build::undoStep). Its Undo is offered only while
// that step is on top of the build's undo stack: once anything else has been done since (a knob turned, another batch),
// it is not, since Build::undo() would take back that instead. A load is no card with Undo (it moves the document's file
// and its clean mark, which undo does not put back); a save-as neither (it wrote a new file, which nothing deletes).
//
// Threads: add() from any thread (the link's worker); the rest on the frame's thread, where the build is.
#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace tee
{
    class Build;
}

namespace tee::assistant
{
    struct Card
    {
        std::uint64_t id = 0;          // the card's own, from 1
        std::string tool;              // "set_fields", "load_build", "save_build_as"
        std::string summary;           // what it did, for a person: "Bore 0.1 m -> 0.105 m; Stroke ..."
        std::uint64_t step = 0;        // the build's undo step it made (0: none, no Undo)
        bool undone = false;           // its Undo was used
    };

    class Cards
    {
    public:
        static constexpr std::size_t kMost = 50;   // the oldest go

        // Any thread.
        void add(Card card);
        std::vector<Card> list() const;
        // Moves when a card is added or undone.
        std::uint64_t revision() const;

        // The frame's thread. Whether the card's Undo is live now: its step is the one on top.
        bool canUndo(std::uint64_t card, const Build& build) const;
        // Its step taken back: false, with `why`, when it is not on top (nothing is done then).
        bool undo(std::uint64_t card, Build& build, std::string& why);

    private:
        mutable std::mutex mutex_;
        std::vector<Card> cards_;
        std::uint64_t next_ = 1;
        std::uint64_t revision_ = 0;
    };
}
