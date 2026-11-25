// assistant/src/Cards.cpp: the cards of what Claude changed (Cards.h).
#include "tee/assistant/Cards.h"

#include "tee/Build.h"

namespace tee::assistant
{
    void Cards::add(Card card)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        card.id = next_++;
        card.undone = false;
        cards_.push_back(std::move(card));
        if (cards_.size() > kMost)
            cards_.erase(cards_.begin(), cards_.begin() + static_cast<std::ptrdiff_t>(cards_.size() - kMost));
        ++revision_;
    }

    std::vector<Card> Cards::list() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return cards_;
    }

    std::uint64_t Cards::revision() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return revision_;
    }

    bool Cards::canUndo(std::uint64_t card, const Build& build) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const Card& c : cards_)
            if (c.id == card)
                return !c.undone && c.step != 0 && build.undoStep() == c.step && !build.gestureOpen();
        return false;
    }

    bool Cards::undo(std::uint64_t card, Build& build, std::string& why)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (Card& c : cards_)
            if (c.id == card)
            {
                if (c.undone)
                {
                    why = "already undone";
                    return false;
                }
                if (c.step == 0)
                {
                    why = "this change has no Undo";
                    return false;
                }
                if (build.gestureOpen() || build.undoStep() != c.step)
                {
                    why = "something else has been changed since: undo that first";
                    return false;
                }
                if (!build.undo())
                {
                    why = "nothing to undo";
                    return false;
                }
                c.undone = true;
                ++revision_;
                return true;
            }
        why = "no such card";
        return false;
    }
}
