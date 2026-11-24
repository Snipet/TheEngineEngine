// ui/kit/gui/GuiPanel.cpp: from TestingPlayground, Source/editor/PanelView.cpp at 47bac40327ed9e8e59928500de9f3beb63fe8636 (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.
// GuiPanel.cpp: see GuiPanel.h. The construction, the frame and the drawing follow the sandbox's panel; the input
// follows the plugin's own Panel (which item a point is, who has the focus, what the wheel scrolls), written again
// here without the plugin's cards and editor around it.
#include "kit/gui/GuiPanel.h"

#include "kit/gui/GuiSpec.h"
#include "kit/gui/Style.h"
#include "kit/shaders/Registry.h"
#include "kit/shaders/ShaderText.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/canvas/Tags.h>
#include <funkgui/core/Theme.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>

namespace tee::ui
{
    namespace
    {
        bool nameStart(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
        bool nameChar(char c) { return nameStart(c) || (c >= '0' && c <= '9'); }

        const SourceFile* guiFileOf(const Sources& files)
        {
            for (const SourceFile& f : files)
                if (isGuiFile(f.name))
                    return &f;
            return nullptr;
        }

        // The name asked of the telemetry for a field as a .gui file writes it: what follows "->", else all of it.
        std::string_view telemetryName(std::string_view field)
        {
            const std::size_t arrow = field.find("->");
            return arrow == std::string_view::npos ? field : field.substr(arrow + 2);
        }
    }

    std::vector<PanelField> telemetryFields(const std::string& gui, const TelemetrySource& telemetry)
    {
        std::vector<PanelField> out;
        std::set<std::string> seen;
        const auto offer = [&](const std::string& word) {
            if (word == "in" || word == "out" || !seen.insert(word).second)
                return;
            if (telemetry.find(telemetryName(word)) >= 0)
                out.push_back({ word, "float", {} });
        };
        // Every name in the file, in and outside "texts" (a shader's inputs="rpm shared->throttle" is a text):
        // letters, digits and _, with one "->" between two of them.
        for (std::size_t i = 0; i < gui.size();)
        {
            if (!nameStart(gui[i]))
            {
                ++i;
                continue;
            }
            std::size_t k = i;
            while (k < gui.size() && nameChar(gui[k]))
                ++k;
            if (k + 2 < gui.size() && gui[k] == '-' && gui[k + 1] == '>' && nameStart(gui[k + 2]))
            {
                std::size_t m = k + 2;
                while (m < gui.size() && nameChar(gui[m]))
                    ++m;
                offer(gui.substr(i, m - i));
                i = m;
                continue;
            }
            offer(gui.substr(i, k - i));
            i = k;
        }
        return out;
    }

    GuiPanel::GuiPanel(Sources files, ParamSource& params, TelemetrySource& telemetry, Options options)
        : options_(std::move(options)), files_(std::move(files)), params_(params), telemetry_(telemetry),
          ports_([&] {
              std::vector<std::unique_ptr<SourcePort>> out;
              const int n = std::min(params.count(), kMaxControls);
              for (int i = 0; i < n; ++i)
                  out.push_back(std::make_unique<SourcePort>(params, i));
              return out;
          }()),
          portList_([this] {
              std::vector<funkgui::ParamPort*> out;
              for (const auto& p : ports_)
                  out.push_back(p.get());
              return out;
          }()),
          // The displays first, as the plugin's Panel made them before the interface's items.
          displays_([this] {
              std::vector<int> out;
              for (int d = 0; d < kMaxDisplays; ++d)
              {
                  items_.push_back(std::make_unique<DisplayItem>(nextId()));
                  out.push_back(static_cast<int>(items_.size()) - 1);
              }
              return out;
          }()),
          view_(items_, portList_, displays_, [this] { return nextId(); }, [this] {
              GuiView::Hooks h;
              h.menuTheme = [this] { return theme_; };
              h.menuHost = [this] { return host_; };
              h.choosePage = [this](int node, int page) {
                  if (view_.choosePage(spec_, node, page))
                  {
                      layout();
                      ++revision_;
                  }
              };
              h.playNote = [](int, float) {};            // no MIDI: a keyboard is never shown, so never played
              h.playWheel = [](bool, float) {};
              return h;
          }())
    {
        firstOwn_ = items_.size();
        if (params_.count() > kMaxControls)
            notes_ += "panel: note: the source has " + std::to_string(params_.count()) + " parameters and a panel shows "
                    + std::to_string(kMaxControls) + ": the rest have no control\n";
        std::vector<std::vector<std::string>> choices(ports_.size());
        for (std::size_t i = 0; i < ports_.size(); ++i)
        {
            controls_[i] = ports_[i]->control();
            choices[i] = optionsOf(params_.info(static_cast<int>(i)));
        }
        const SourceFile* gui = guiFileOf(files_);
        if (gui == nullptr)
            notes_ += "panel: note: there is no .gui file among the panel's files: it shows nothing\n";
        else
        {
            std::vector<std::string> names;
            for (const SourceFile& f : files_)
                names.push_back(f.name);
            // We have no input, as an instrument has none: `in` is left out with the reader's own note.
            // A Player's parameters and telemetry may answer one find() (both interfaces declare it): a name that is a
            // parameter's id is the parameter, never a field.
            std::vector<PanelField> fields = telemetryFields(gui->text, telemetry_);
            std::erase_if(fields, [this](const PanelField& f) {
                const int i = params_.find(f.name);
                return i >= 0 && i < params_.count() && params_.info(i).id == f.name;
            });
            spec_ = readGuiSpec(*gui, controls_, fields, true, names, notes_);
        }
        // What the file did not say comes from the parameter: its label, and a choice's names.
        for (std::size_t i = 0; i < ports_.size(); ++i)
        {
            const ParamInfo& info = params_.info(static_cast<int>(i));
            if (spec_.labels[i].empty() && !info.label.empty())
                spec_.labels[i] = cutText(upper(asciiText(info.label)), kMaxDialLabel);
            if (spec_.options[i].empty())
                spec_.options[i] = choices[i];
            ports_[i]->setControl(controls_[i]);         // the taper, unit and scale the file gave it
        }
        // Each display's field, by its tap.
        tapHandle_.assign(static_cast<std::size_t>(kMaxDisplays), -1);
        for (const PanelDisplay& d : spec_.displays)
            if (d.tap >= 0 && d.tap < kMaxDisplays && !d.audio())
                tapHandle_[static_cast<std::size_t>(d.tap)] = telemetry_.find(telemetryName(d.source));
        // The shaders the build made for this panel (kit/shaders/Registry.h; none in a build without a GPU layer).
        for (PanelShader& sh : spec_.shaders)
            if (const shaders::Built* built = shaders::find(options_.name, sh.file))
            {
                sh.binary.assign(built->data, built->data + built->size);
                sh.id = shaderId(sh.binary);
            }
        view_.setControls(controls_, spec_);
        std::string shaderNotes = notes_;
        for (const PanelShader& sh : spec_.shaders)
            if (sh.binary.empty())
                shaderNotes += sh.file + ":1:1: note: " + shaders::whyNone() + "\n";
        view_.registerShaders(spec_, files_, shaderNotes);
        // Its natural size: the root's when it is given no room at all.
        view_.layout(spec_, controls_, funkgui::Rect{ 0.0f, 0.0f, 1.0f, 1.0f });
        naturalW_ = std::max(1, static_cast<int>(std::ceil(view_.places().contentW)));
        naturalH_ = std::max(1, static_cast<int>(std::ceil(view_.places().contentH)));
        width_ = options_.width > 0 ? options_.width : std::max(naturalW_, options_.minWidth);
        height_ = options_.height > 0 ? options_.height : std::max(naturalH_, options_.minHeight);
        view_.resetScroll();
        layout();
        fillMeters();
    }

    GuiPanel::~GuiPanel() = default;

    uint32_t GuiPanel::nextId() const noexcept { return kA11yStride * static_cast<uint32_t>(items_.size() + 1); }

    int GuiPanel::addItem(std::unique_ptr<Item> item)
    {
        items_.push_back(std::move(item));
        ++revision_;
        dirty_ = true;
        return static_cast<int>(items_.size()) - 1;
    }

    void GuiPanel::setBottomReserve(int px)
    {
        px = std::max(px, 0);
        naturalH_ += px - reserve_;
        reserve_ = px;
        if (options_.height <= 0)
            height_ = std::max(naturalH_, options_.minHeight);
        layout();
        ++revision_;
    }

    void GuiPanel::layout()
    {
        view_.layout(spec_, controls_, funkgui::Rect{ 0.0f, 0.0f, static_cast<float>(width_),
                                                      static_cast<float>(std::max(1, height_ - reserve_)) });
        placed();
        dirty_ = true;
    }

    void GuiPanel::resize(int w, int h)
    {
        w = std::max(w, options_.minWidth);
        h = std::max(h, options_.minHeight);
        if (w == width_ && h == height_)
            return;
        width_ = w;
        height_ = h;
        layout();
        ++revision_;
    }

    std::vector<funkgui::Rect> GuiPanel::keyboardRects() const
    {
        std::vector<funkgui::Rect> out;
        const GuiLayout& gl = view_.places();
        for (std::size_t i = 0; i < spec_.gui.nodes.size() && i < gl.rects.size(); ++i)
            if (spec_.gui.nodes[i].kind == GuiKind::keyboard && i < gl.visible.size() && gl.visible[i])
                out.push_back(gl.rects[i]);
        return out;
    }

    int GuiPanel::slotOf(std::string_view paramId) const
    {
        const int i = params_.find(paramId);
        return i >= 0 && i < static_cast<int>(ports_.size()) ? i : -1;
    }

    SourcePort* GuiPanel::port(int slot) noexcept
    {
        return slot >= 0 && slot < static_cast<int>(ports_.size()) ? ports_[static_cast<std::size_t>(slot)].get() : nullptr;
    }

    funkgui::Rect GuiPanel::controlRect(std::string_view paramId) const
    {
        const int item = view_.controlItem(slotOf(paramId));
        if (item < 0 || !items_[static_cast<std::size_t>(item)]->shown())
            return {};
        return static_cast<const GuiControlItem&>(*items_[static_cast<std::size_t>(item)]).cell();
    }

    funkgui::Rect GuiPanel::tabRect(std::string_view pageTitle) const
    {
        const std::vector<GuiNode>& nodes = spec_.gui.nodes;
        for (const int t : view_.tabs())
        {
            const auto& tabs = static_cast<const GuiTabsItem&>(*items_[static_cast<std::size_t>(t)]);
            if (!tabs.shown() || tabs.node() < 0 || static_cast<std::size_t>(tabs.node()) >= nodes.size())
                continue;
            const std::vector<int>& pages = nodes[static_cast<std::size_t>(tabs.node())].children;
            for (std::size_t k = 0; k < pages.size() && k < tabs.tabs().size(); ++k)
                if (nodes[static_cast<std::size_t>(pages[k])].text == pageTitle)
                    return tabs.tabs()[k];
        }
        return {};
    }

    void GuiPanel::attach(funkgui::HostServices& host)
    {
        host_ = &host;
        gestures_ = std::make_unique<funkgui::GestureController>(host);
    }

    // The frame's values: each display's field, and the output's level. The source gives a peak and no RMS: a VU
    // meter of `out` is fed the RMS a sine of that peak has.
    void GuiPanel::fillMeters()
    {
        ++meters_.frame;
        for (std::size_t t = 0; t < tapHandle_.size(); ++t)
            meters_.taps[t] = tapHandle_[t] >= 0 ? static_cast<float>(telemetry_.value(tapHandle_[t])) : 0.0f;
        const float peak = std::clamp(telemetry_.outputPeak(), 0.0f, 4.0f);
        meters_.outPeak[0] = meters_.outPeak[1] = peak;
        meters_.outRms[0] = meters_.outRms[1] = peak * 0.70710678f;
    }

    void GuiPanel::tick(float dt)
    {
        fillMeters();
        updateDisplays(items_, displays_, meters_,
                       [this](bool input, float* out, int n) {
                           std::fill(out, out + n, 0.0f);
                           if (!input && options_.scope)
                               options_.scope(out, n);
                       },
                       feed_, dt);
        view_.tick(spec_, meters_, true);
        const int hovered = hasPointer_ && pressed_ < 0 ? itemAt(pointer_) : pressed_;
        const funkgui::Point far{ -1.0e4f, -1.0e4f };
        for (std::size_t i = 0; i < items_.size(); ++i)
        {
            const int idx = static_cast<int>(i);
            items_[i]->tick(dt, hovered == idx, ring_ && focus_ == idx, hasPointer_ ? pointer_ : far);
        }
        if (gestures_ && host_ != nullptr)
            gestures_->poll(host_->nowSeconds());
        dirty_ = false;
    }

    // The ground, then the interface's own ground, frames, labels and dividers, its items in its theme (clipped to the
    // area), its scroll bars, and the subclass's items over it.
    void GuiPanel::draw(funkgui::Canvas& c, const funkgui::Theme& th)
    {
        theme_ = th;
        c.rrect(0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_), 0.0f, style::palette(th).card);
        const funkgui::Theme gth = view_.theme(th, spec_);
        view_.drawGround(c, gth, spec_, funkgui::tags::none);
        const funkgui::Rect& area = view_.places().area;
        const std::vector<int>& keyboards = view_.keyboards();
        for (std::size_t i = 0; i < firstOwn_; ++i)
        {
            if (!items_[i]->shown() || !view_.owns(i))
                continue;
            if (std::find(keyboards.begin(), keyboards.end(), static_cast<int>(i)) != keyboards.end())
                continue;                                // no MIDI: its room is kept, nothing is drawn (GuiPanel.h)
            const funkgui::Canvas::ClipScope clip(c, area);
            items_[i]->draw(c, gth, ring_ && focus_ == static_cast<int>(i));
        }
        view_.drawScrollBars(c, th);
        for (std::size_t i = firstOwn_; i < items_.size(); ++i)
            if (items_[i]->shown())
                items_[i]->draw(c, gth, ring_ && focus_ == static_cast<int>(i));
    }

    bool GuiPanel::wantsFullRate() const
    {
        return dirty_ || pressed_ >= 0 || barPress_ || (gestures_ && (gestures_->dragging() || gestures_->wheeling()))
            || std::any_of(items_.begin(), items_.end(), [](const auto& it) { return it->shown() && !it->settled(); });
    }

    // The subclass's items first (they are drawn over the interface), then the interface's in their order (its
    // pointer items last: GuiView.h). Never a keyboard.
    int GuiPanel::itemAt(funkgui::Point p) const
    {
        for (std::size_t i = firstOwn_; i < items_.size(); ++i)
            if (items_[i]->shown() && items_[i]->contains(p))
                return static_cast<int>(i);
        const std::vector<int>& keyboards = view_.keyboards();
        for (std::size_t i = 0; i < firstOwn_; ++i)
            if (items_[i]->shown() && items_[i]->contains(p)
                && std::find(keyboards.begin(), keyboards.end(), static_cast<int>(i)) == keyboards.end())
                return static_cast<int>(i);
        return -1;
    }

    void GuiPanel::setFocus(int item)
    {
        if (item >= 0 && !items_[static_cast<std::size_t>(item)]->shown())
            item = -1;
        if (focus_ != item && focus_ >= 0)
            items_[static_cast<std::size_t>(focus_)]->focusChanged(false);
        const bool moved = focus_ != item;
        focus_ = item;
        if (item >= 0 && moved)
            items_[static_cast<std::size_t>(item)]->focusChanged(true);
        dirty_ = true;
    }

    // The next (or the one before) of the items that are shown and take the focus, round and round; false when
    // there is none.
    bool GuiPanel::moveFocus(int step)
    {
        const int n = static_cast<int>(items_.size());
        const std::vector<int>& keyboards = view_.keyboards();
        int at = focus_;
        for (int k = 0; k < n; ++k)
        {
            at = at < 0 ? (step > 0 ? 0 : n - 1) : (at + step + n) % n;
            const Item& it = *items_[static_cast<std::size_t>(at)];
            if (it.shown() && it.focusable() && std::find(keyboards.begin(), keyboards.end(), at) == keyboards.end())
            {
                setFocus(at);
                return true;
            }
        }
        return false;
    }

    void GuiPanel::pointerMove(const funkgui::PointerEvent& e)
    {
        const int before = hasPointer_ ? itemAt(pointer_) : -1;
        pointer_ = { e.x, e.y };
        hasPointer_ = true;
        const int now = itemAt(pointer_);
        if (before >= 0 && before != now)
            items_[static_cast<std::size_t>(before)]->pointerExit();
        if (now >= 0)
            items_[static_cast<std::size_t>(now)]->pointerMove(e);
        dirty_ = true;
    }

    void GuiPanel::pointerExit()
    {
        if (hasPointer_)
            if (const int before = itemAt(pointer_); before >= 0)
                items_[static_cast<std::size_t>(before)]->pointerExit();
        hasPointer_ = false;
        dirty_ = true;
    }

    void GuiPanel::pointerDown(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        pointer_ = { e.x, e.y };
        hasPointer_ = true;
        ring_ = false;                                   // the pointer's focus shows no ring
        pressed_ = -1;
        if (e.button == funkgui::Button::middle)
        {
            const int i = itemAt(pointer_);
            if (i < 0 || !items_[static_cast<std::size_t>(i)]->takesButtons(pointer_))
                return;                                  // a middle press reaches only an item that asks for it
        }
        barPress_ = !e.popup && e.button == funkgui::Button::left && view_.pressBar(spec_, pointer_);
        if (barPress_)
        {
            ++revision_;
            return;
        }
        pressed_ = itemAt(pointer_);
        if (pressed_ >= 0 && items_[static_cast<std::size_t>(pressed_)]->focusable())
            setFocus(pressed_);
        else if (pressed_ < 0)
            setFocus(-1);
        if (pressed_ >= 0 && gestures_ && host_ != nullptr)
            items_[static_cast<std::size_t>(pressed_)]->pointerDown(e, *gestures_, *host_);
    }

    void GuiPanel::pointerDrag(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        pointer_ = { e.x, e.y };
        if (barPress_)
        {
            if (view_.dragBar(spec_, pointer_))
            {
                placed();
                ++revision_;
            }
            return;
        }
        if (pressed_ >= 0 && gestures_)
            items_[static_cast<std::size_t>(pressed_)]->pointerDrag(e, *gestures_);
    }

    void GuiPanel::pointerUp(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        if (std::exchange(barPress_, false))
        {
            view_.releaseBar();
            pressed_ = -1;
            return;
        }
        if (pressed_ >= 0 && gestures_)
            items_[static_cast<std::size_t>(pressed_)]->pointerUp(e, *gestures_);
        pressed_ = -1;
    }

    void GuiPanel::doubleClick(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        if (e.button == funkgui::Button::middle)
            return;
        const int i = itemAt({ e.x, e.y });
        if (i >= 0 && gestures_)
            items_[static_cast<std::size_t>(i)]->doubleClick(e, *gestures_);
    }

    void GuiPanel::scrollBy(float dx, float dy)
    {
        if (view_.scrollBy(spec_, dx, dy))
        {
            placed();
            ++revision_;
        }
    }

    // An interface taller or wider than its area scrolls, but for a control under the pointer (it takes the wheel
    // itself): the deltas as they came (kit/gui/Scroll.h), Shift turning a vertical wheel sideways.
    bool GuiPanel::wheel(const funkgui::WheelEvent& e)
    {
        dirty_ = true;
        const int i = itemAt({ e.x, e.y });
        const GuiLayout& gl = view_.places();
        const bool own = i >= static_cast<int>(firstOwn_);
        if (!own && !spec_.gui.empty() && gl.area.contains({ e.x, e.y }) && (gl.maxScrollX > 0.0f || gl.maxScrollY > 0.0f)
            && !view_.isControl(i))
        {
            const float k = GuiView::wheelPx(e.smooth);
            if (e.mods.shift && e.dx == 0.0f)
                scrollBy(-e.dy * k, 0.0f);
            else
                scrollBy(-e.dx * k, -e.dy * k);
            return true;
        }
        if (i < 0 || !gestures_ || host_ == nullptr)
            return false;
        return items_[static_cast<std::size_t>(i)]->wheel(e, *gestures_, host_->nowSeconds());
    }

    bool GuiPanel::key(const funkgui::KeyEvent& e)
    {
        dirty_ = true;
        const bool text = focus_ >= 0 && items_[static_cast<std::size_t>(focus_)]->shown()
                       && items_[static_cast<std::size_t>(focus_)]->takesAllKeys();
        if (text && gestures_)
        {
            // A text field holding the caret: every key is its own first; Escape (when it does not use it) leaves it.
            if (items_[static_cast<std::size_t>(focus_)]->key(e, *gestures_))
                return true;
            if (e.key == funkgui::Key::escape)
            {
                setFocus(-1);
                return true;
            }
        }
        if (e.key == funkgui::Key::tab)
        {
            ring_ = moveFocus(e.mods.shift ? -1 : 1);
            return ring_;
        }
        if (e.key == funkgui::Key::escape && ring_)
        {
            ring_ = false;
            return true;
        }
        if (focus_ < 0 || !gestures_ || !items_[static_cast<std::size_t>(focus_)]->shown())
            return false;
        return items_[static_cast<std::size_t>(focus_)]->key(e, *gestures_);
    }

    funkgui::Cursor GuiPanel::cursor() const
    {
        const int i = pressed_ >= 0 ? pressed_ : (hasPointer_ ? itemAt(pointer_) : -1);
        return i >= 0 ? items_[static_cast<std::size_t>(i)]->cursorAt(pointer_) : funkgui::Cursor::normal;
    }

    void GuiPanel::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        const std::vector<int>& keyboards = view_.keyboards();
        for (std::size_t i = 0; i < items_.size(); ++i)
            if (items_[i]->shown() && std::find(keyboards.begin(), keyboards.end(), static_cast<int>(i)) == keyboards.end())
                items_[i]->accessibility(out);
    }

    void GuiPanel::a11yAction(uint32_t id, funkgui::A11yAction a, double value)
    {
        const std::size_t item = id / kA11yStride;
        if (item < 1 || item > items_.size() || !gestures_ || !items_[item - 1]->shown())
            return;
        if (items_[item - 1]->a11yAction(id, a, value, *gestures_))
            dirty_ = true;
    }

    void GuiPanel::closeGestures()
    {
        view_.releaseKeys();
        if (pressed_ >= 0 && gestures_)
            items_[static_cast<std::size_t>(pressed_)]->pointerUp({}, *gestures_);
        pressed_ = -1;
        if (std::exchange(barPress_, false))
            view_.releaseBar();
        if (gestures_)
            gestures_->closeAll();
    }
}
