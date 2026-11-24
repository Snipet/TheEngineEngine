// ui/views/listen/ListenWorkspace.h: LISTEN, where the car is heard from (docs/ui/listen.md; package L's listener,
// docs/models/listen.md).
//
//   THE PLAN    the car from above, drawn to the build's wheelbase and track (a bike when car.kind is Motorcycle);
//               every sound source a dot at its place, sized by its level at the listener and named; the listener a
//               point with its facing and rings of distance, dragged to place it (the Free camera) and turned by its
//               facing's handle; the place drawn round the car (the tunnel's walls, the garage's box, the street's
//               facades, the field's tree line); trackside, the road and the car's place along it.
//   THE BAR     the cameras, one click each (Tailpipe, Rear, Side, Engine bay, Driver, Trackside, Chase, Free), and the
//               places (Hall, Field, Street, Tunnel, Garage). With the reference's listener (sound.model 0) the
//               reference's five views instead, said so in words.
//   THE COLUMN  loudness, the mix's level, the limiter's gain reduction and the ceiling's count (a lamp and a word
//               while the limiter works), the range and the Doppler ratio; in the driver's seat the windows (also a
//               handle on the picture) and the four ways in, in words with their levels.
//
// Fields are written as one gesture each (a click, a drag). A name not there yet draws a dash; the sources' places,
// when the program does not publish them, are the model's documented places (docs/models/listen.md) and say so.
#pragma once

#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/widgets/Basic.h"

#include <array>
#include <string>
#include <vector>

namespace tee::ui
{
    class ListenWorkspace final : public Workspace
    {
    public:
        static constexpr int kSources = 10;   // L's LS_ order: TAIL0 TAIL1 INTAKE BAY BAYOPEN TYRES WIND DRIVE AXLE TANK
        static constexpr int kCameras = 8;    // Tailpipe .. Chase, and Free

        explicit ListenWorkspace(WorkspaceContext& c);

        const char* id() const override { return "listen"; }
        const char* title() const override { return "LISTEN"; }
        std::vector<std::string> sections() const override { return { "sound" }; }
        bool moving() const override;
        std::uint64_t drawKey() const override;

        // ---- what it shows (for tests) -----------------------------------------------------------------------------
        struct Source
        {
            const char* name;
            double x = 0, y = 0, level = 0;   // m in the car's frame; dB at the listener (NaN: not published)
            bool published = false;            // its place came from the program (else the model's documented place)
        };
        struct Scene
        {
            int model = 1, camera = 0, place = 0, enginePlace = 0;
            bool bike = false;
            double wheelbase = 2.6, track = 1.55, rearAxle = 0.85, length = 4.35, width = 1.8;
            double lx = 0, ly = 0, facing = 0, range = 0;   // the listener (m, rad: 0 faces forward, + turns right)
            bool listenerPublished = false;
            std::array<Source, kSources> src{};
        };
        Scene scene() const;
        // The plan's mapping (window px) and the listener's place on it.
        funkgui::Point toPx(double x, double y) const noexcept;
        void toCar(funkgui::Point p, double& x, double& y) const noexcept;
        funkgui::Point listenerPx() const;
        funkgui::Point facingHandlePx() const;
        const funkgui::Rect& plan() const noexcept { return plan_; }
        const funkgui::Rect& column() const noexcept { return column_; }
        const funkgui::Rect& bar() const noexcept { return bar_; }

        // funkgui::Panel
        void pointerDown(const funkgui::PointerEvent& e) override;
        void pointerDrag(const funkgui::PointerEvent& e) override;
        void pointerUp(const funkgui::PointerEvent& e) override;
        funkgui::Cursor cursor() const override;

    protected:
        void layout(const funkgui::Rect& stage) override;
        void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override;
        void step(float dt) override;

    private:
        double field(const char* path, double fallback) const;
        std::string choice(const char* path) const;
        bool has(const char* path) const { return ctx_.doc.find(path) >= 0; }
        void set(const char* path, double v);   // one gesture
        void fit();                             // the plan's scale for the scene now
        void drawPlan(funkgui::Canvas& c, const look::Palette& p, const Scene& s) const;
        void drawPlace(funkgui::Canvas& c, const look::Palette& p, const Scene& s) const;
        void drawCar(funkgui::Canvas& c, const look::Palette& p, const Scene& s) const;
        void drawColumn(funkgui::Canvas& c, const look::Palette& p, const Scene& s) const;

        std::array<Button*, kCameras> cameras_{};
        std::array<Button*, 5> places_{};
        std::array<Button*, 5> views_{};
        funkgui::Rect bar_{}, plan_{}, column_{}, windows_{};
        // the plan's scale: px a metre, and the car frame's point at the plan's centre
        double scale_ = 40.0, cx_ = 0.0, cy_ = 2.0;
        enum class Drag : std::uint8_t { none, listener, facing, windows } drag_ = Drag::none;
        std::uint64_t seen_ = 0, revision_ = 1;
    };

    std::unique_ptr<Workspace> makeListenWorkspace(WorkspaceContext& c);
}
