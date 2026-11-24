// ui/kit/widgets/Board.cpp: see Board.h.
#include "kit/widgets/Board.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Theme.h>

#include <algorithm>

namespace tee::ui
{
    Board::Board(int width, int height, int minWidth, int minHeight)
        : width_(std::max(width, minWidth)), height_(std::max(height, minHeight)), minW_(minWidth), minH_(minHeight)
    {
    }

    Board::~Board() = default;

    int Board::add(std::unique_ptr<Item> item)
    {
        items_.push_back(std::move(item));
        ++revision_;
        return static_cast<int>(items_.size()) - 1;
    }

    void Board::attach(funkgui::HostServices& host)
    {
        host_ = &host;
        gestures_ = std::make_unique<funkgui::GestureController>(host);
        attached();
        arrange();
    }

    void Board::resize(int w, int h)
    {
        w = std::max(w, minW_);
        h = std::max(h, minH_);
        if (w == width_ && h == height_)
            return;
        width_ = w;
        height_ = h;
        arrange();
        ++revision_;
    }

    void Board::tick(float dt)
    {
        if (funkgui::Panel* in = inner())
            in->tick(dt);
        const int hovered = pressed_ >= 0 ? pressed_ : (hasPointer_ && !innerPress_ ? itemAt(pointer_) : -1);
        const funkgui::Point far{ -1.0e4f, -1.0e4f };
        for (std::size_t i = 0; i < items_.size(); ++i)
            items_[i]->tick(dt, hovered == static_cast<int>(i), ring_ && focus_ == static_cast<int>(i),
                            hasPointer_ ? pointer_ : far);
        if (gestures_ && host_ != nullptr)
            gestures_->poll(host_->nowSeconds());
    }

    void Board::drawGround(funkgui::Canvas& c, const funkgui::Theme& th) const
    {
        c.rrect(0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_), 0.0f, th.ground);
    }

    void Board::draw(funkgui::Canvas& c, const funkgui::Theme& th)
    {
        drawGround(c, th);
        if (funkgui::Panel* in = inner())
            in->draw(c, th);
        drawOver(c, th);
        for (std::size_t i = 0; i < items_.size(); ++i)
            if (items_[i]->shown())
                items_[i]->draw(c, th, ring_ && focus_ == static_cast<int>(i));
    }

    bool Board::wantsFullRate() const
    {
        if (const funkgui::Panel* in = inner(); in != nullptr && in->wantsFullRate())
            return true;
        return pressed_ >= 0 || (gestures_ && (gestures_->dragging() || gestures_->wheeling()))
            || std::any_of(items_.begin(), items_.end(), [](const auto& it) { return it->shown() && !it->settled(); });
    }

    int Board::itemAt(funkgui::Point p) const
    {
        for (std::size_t i = items_.size(); i-- > 0;)  // the last drawn is on top
            if (items_[i]->shown() && items_[i]->contains(p))
                return static_cast<int>(i);
        return -1;
    }

    void Board::setFocus(int item)
    {
        if (item >= 0 && !items_[static_cast<std::size_t>(item)]->shown())
            item = -1;
        if (focus_ == item)
            return;
        if (focus_ >= 0)
            items_[static_cast<std::size_t>(focus_)]->focusChanged(false);
        focus_ = item;
        if (item >= 0)
            items_[static_cast<std::size_t>(item)]->focusChanged(true);
    }

    bool Board::moveFocus(int step)
    {
        const int n = static_cast<int>(items_.size());
        const bool wrap = inner() == nullptr && tabWraps_;   // with an inner Panel, past the last is its turn
        int at = focus_;
        for (int k = 0; k < n; ++k)
        {
            at = at < 0 ? (step > 0 ? 0 : n - 1) : at + step;
            if (wrap)
                at = (at + n) % n;
            else if (at < 0 || at >= n)
                break;
            const Item& it = *items_[static_cast<std::size_t>(at)];
            if (it.shown() && it.focusable())
            {
                setFocus(at);
                return true;
            }
        }
        setFocus(-1);
        return false;
    }

    void Board::pointerMove(const funkgui::PointerEvent& e)
    {
        const int before = hasPointer_ ? itemAt(pointer_) : -1;
        pointer_ = { e.x, e.y };
        hasPointer_ = true;
        const int now = itemAt(pointer_);
        if (before >= 0 && before != now)
            items_[static_cast<std::size_t>(before)]->pointerExit();
        if (now >= 0)
            items_[static_cast<std::size_t>(now)]->pointerMove(e);
        if (funkgui::Panel* in = inner())
        {
            if (now >= 0)
                in->pointerExit();
            else
                in->pointerMove(e);
        }
    }

    void Board::pointerExit()
    {
        if (hasPointer_)
            if (const int before = itemAt(pointer_); before >= 0)
                items_[static_cast<std::size_t>(before)]->pointerExit();
        hasPointer_ = false;
        if (funkgui::Panel* in = inner())
            in->pointerExit();
    }

    void Board::pointerDown(const funkgui::PointerEvent& e)
    {
        pointer_ = { e.x, e.y };
        hasPointer_ = true;
        ring_ = false;
        pressed_ = itemAt(pointer_);
        innerPress_ = false;
        if (pressed_ < 0)
        {
            setFocus(-1);
            if (funkgui::Panel* in = inner())
            {
                innerPress_ = true;
                in->pointerDown(e);
            }
            return;
        }
        if (items_[static_cast<std::size_t>(pressed_)]->focusable())
            setFocus(pressed_);
        if (gestures_ && host_ != nullptr)
            items_[static_cast<std::size_t>(pressed_)]->pointerDown(e, *gestures_, *host_);
    }

    void Board::pointerDrag(const funkgui::PointerEvent& e)
    {
        pointer_ = { e.x, e.y };
        if (innerPress_)
        {
            if (funkgui::Panel* in = inner())
                in->pointerDrag(e);
            return;
        }
        if (pressed_ >= 0 && gestures_)
            items_[static_cast<std::size_t>(pressed_)]->pointerDrag(e, *gestures_);
    }

    void Board::pointerUp(const funkgui::PointerEvent& e)
    {
        if (std::exchange(innerPress_, false))
        {
            if (funkgui::Panel* in = inner())
                in->pointerUp(e);
            return;
        }
        if (pressed_ >= 0 && gestures_)
            items_[static_cast<std::size_t>(pressed_)]->pointerUp(e, *gestures_);
        pressed_ = -1;
    }

    void Board::doubleClick(const funkgui::PointerEvent& e)
    {
        const int i = itemAt({ e.x, e.y });
        if (i >= 0 && gestures_)
            items_[static_cast<std::size_t>(i)]->doubleClick(e, *gestures_);
        else if (i < 0)
            if (funkgui::Panel* in = inner())
                in->doubleClick(e);
    }

    bool Board::wheel(const funkgui::WheelEvent& e)
    {
        const int i = itemAt({ e.x, e.y });
        if (i < 0)
        {
            funkgui::Panel* in = inner();
            return in != nullptr && in->wheel(e);
        }
        if (!gestures_ || host_ == nullptr)
            return false;
        return items_[static_cast<std::size_t>(i)]->wheel(e, *gestures_, host_->nowSeconds());
    }

    bool Board::key(const funkgui::KeyEvent& e)
    {
        if (focus_ >= 0 && gestures_)
        {
            Item& it = *items_[static_cast<std::size_t>(focus_)];
            if (it.takesAllKeys())
            {
                if (it.key(e, *gestures_))
                    return true;
                if (e.key == funkgui::Key::escape)
                {
                    setFocus(-1);
                    return true;
                }
            }
        }
        if (e.key == funkgui::Key::tab)
        {
            // The board's items first, then the inner Panel's, round and round.
            funkgui::Panel* in = inner();
            if (focus_ < 0 && in != nullptr && !e.mods.shift && in->key(e))
                return true;
            ring_ = moveFocus(e.mods.shift ? -1 : 1);
            if (!ring_ && in != nullptr)
                return in->key(e);
            return ring_;
        }
        if (e.key == funkgui::Key::escape && ring_ && focus_ >= 0)
        {
            ring_ = false;
            return true;
        }
        if (focus_ >= 0 && gestures_)
            return items_[static_cast<std::size_t>(focus_)]->key(e, *gestures_);
        funkgui::Panel* in = inner();
        return in != nullptr && in->key(e);
    }

    funkgui::Cursor Board::cursor() const
    {
        if (innerPress_)
            if (const funkgui::Panel* in = inner())
                return in->cursor();
        const int i = pressed_ >= 0 ? pressed_ : (hasPointer_ ? itemAt(pointer_) : -1);
        if (i >= 0)
            return items_[static_cast<std::size_t>(i)]->cursorAt(pointer_);
        const funkgui::Panel* in = inner();
        return in != nullptr ? in->cursor() : funkgui::Cursor::normal;
    }

    void Board::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        if (const funkgui::Panel* in = inner())
            in->accessibility(out);
        for (const auto& it : items_)
            if (it->shown())
                it->accessibility(out);
    }

    uint32_t Board::a11yRevision() const
    {
        const funkgui::Panel* in = inner();
        return revision_ * 7919u + (in != nullptr ? in->a11yRevision() : 0u);
    }

    void Board::a11yAction(uint32_t id, funkgui::A11yAction a, double value)
    {
        if (id < kIdBase)
        {
            if (funkgui::Panel* in = inner())
                in->a11yAction(id, a, value);
            return;
        }
        const std::size_t item = (id - kIdBase) / kStride;
        if (item < 1 || item > items_.size() || !gestures_ || !items_[item - 1]->shown())
            return;
        items_[item - 1]->a11yAction(id, a, value, *gestures_);
    }

    void Board::closeGestures()
    {
        if (funkgui::Panel* in = inner())
            in->closeGestures();
        if (pressed_ >= 0 && gestures_)
            items_[static_cast<std::size_t>(pressed_)]->pointerUp({}, *gestures_);
        pressed_ = -1;
        innerPress_ = false;
        if (gestures_)
            gestures_->closeAll();
    }
}
