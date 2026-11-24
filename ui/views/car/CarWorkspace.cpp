// ui/views/car/CarWorkspace.cpp: the CAR workspace (docs/plan/ui-design.md section 3; docs/ui/car.md). Four drawings
// of the vehicle as built, each from the Spec's fields and moved by telemetry:
//
//   the car from the side   to scale: wheelbase, the centre of mass at its height, the tyres; the load on each axle as
//                           a bar at that axle, live (squat and dive as numbers and bars); drag and downforce as arrows
//                           with numbers at the speed it is doing
//   the driveline           for the layout chosen: engine, clutch or converter, gearbox, the differentials by kind, the
//                           wheels; the torque reaching each axle, live; a slipping wheel marked with a word
//   the tyre's grip         force over load against slip for the compound and the surface, each driven wheel's live
//                           point on it
//   the gearing             road speed against engine speed in each gear, the limiter, the shift points, the speed in
//                           each gear at the limiter, and where the build's predicted power meets drag: the top speed,
//                           marked as derived
//
// Clicking a part selects its group of fields (inspectorPage() lists them). Every field is read through kFields with a
// fallback: a value that is not in the build yet is drawn from the Spec's default and marked with a star.
#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/bind/Binding.h"
#include "views/car/Car.h"
#include "views/dyno/Chart.h"

#include <algorithm>
#include <array>
#include <span>
#include <cmath>
#include <string>
#include <vector>

namespace tee::ui
{
    namespace
    {
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Rect;
        using chart::kRpm;

        // The fields, by path (sim/vehicle/vspec.fm's VehSpec as appended to spec.car), with the Spec's defaults.
        enum F : int
        {
            fMass, fWheelbase, fCg, fFront, fTrack, fCdA, fClA, fAeroBalance, fTyreRadius, fGearbox, fGears, fFinal,
            fEfficiency, fBrakeDecel, fBrakeBias, fCentreSplit, fCount
        };
        struct Bind
        {
            const char* paths[2];
            double fallback;
        };
        const Bind kFields[fCount] = {
            { { "car.mass", nullptr }, 1400.0 },
            { { "car.wheelbase", nullptr }, 2.6 },
            { { "car.cgHeight", nullptr }, 0.5 },
            { { "car.frontWeight", nullptr }, 0.53 },
            { { "car.track", nullptr }, 1.55 },
            { { "car.cdA", nullptr }, 0.7 },
            { { "car.clA", nullptr }, 0.0 },
            { { "car.aeroBalance", nullptr }, 0.45 },
            { { "car.tyreRadius", "car.tyre" }, 0.33 },      // the demo's car.tyre is a radius; the Spec's a compound
            { { "car.gearbox", nullptr }, 0.0 },
            { { "car.gears", nullptr }, 6.0 },
            { { "car.finalDrive", nullptr }, 3.6 },
            { { "car.gearEfficiency", nullptr }, 0.97 },
            { { "car.brakeDecel", nullptr }, 1.2 },
            { { "car.brakeBias", nullptr }, 0.65 },
            { { "car.centreSplit", nullptr }, 0.4 },
        };
        // A six-speed's ratios, for a build whose ratios are not in the Spec yet (marked with a star where shown).
        const double kTypicalRatios[] = { 3.36, 2.07, 1.43, 1.00, 0.84, 0.56 };

        // The tyre model's compounds and surfaces (sim/vehicle/tyre.fm's VEH_COMPOUND columns B, C, E, mu and the
        // rolling resistance, and VEH_SURFACE): the grip curve drawn is the one the simulator uses.
        struct Compound
        {
            const char* name;
            double b, c, e, mu, crr;
            double surface[5];   // dry, damp, wet, snow, a prepared strip
        };
        const Compound kCompounds[] = {
            { "Economy", 15.0, 1.50, 0.1, 0.90, 0.0080, { 1.00, 0.85, 0.70, 0.25, 1.05 } },
            { "Street", 21.5, 1.65, 0.8, 1.00, 0.0100, { 1.00, 0.85, 0.70, 0.22, 1.08 } },
            { "Sport", 13.0, 1.55, -0.8, 1.10, 0.0110, { 1.00, 0.82, 0.65, 0.18, 1.10 } },
            { "Semi-slick", 19.5, 1.65, 0.5, 1.25, 0.0120, { 1.00, 0.75, 0.50, 0.12, 1.15 } },
            { "Slick", 17.5, 1.65, 0.0, 1.45, 0.0130, { 1.00, 0.60, 0.30, 0.08, 1.20 } },
            { "Drag radial", 9.5, 1.75, 0.5, 1.30, 0.0150, { 1.00, 0.70, 0.45, 0.10, 1.30 } },
            { "Drag slick", 6.0, 1.75, -0.2, 1.30, 0.0250, { 1.00, 0.50, 0.20, 0.06, 2.00 } },
            { "Winter", 10.5, 1.40, -0.9, 0.85, 0.0120, { 1.00, 0.90, 0.80, 0.45, 1.00 } },
            { "Truck", 20.5, 1.60, 0.8, 0.80, 0.0055, { 1.00, 0.85, 0.70, 0.28, 1.02 } },
            { "Motorcycle", 13.5, 1.55, -0.2, 1.10, 0.0120, { 1.00, 0.82, 0.65, 0.15, 1.08 } },
        };
        // A motorcycle's tyres (sim/vehicle/bike.fm: BIKE_TYRE_MU, BIKE_TYRE_COMPOUND): the peak grip on dry asphalt and
        // the compound whose curve it has.
        const double kBikeTyreMu[] = { 1.10, 1.25, 1.40, 1.55, 1.00, 1.05 };
        const int kBikeTyreCompound[] = { 9, 9, 9, 4, 9, 9 };
        const char* const kBikeTyreName[] = { "Touring", "Sport", "Hypersport", "Race slick", "Dual sport", "Cruiser" };
        const char* const kSurfaces[] = { "dry", "damp", "wet", "snow", "prepared strip" };
        // The force over the load's share of mu at slip kappa, as tyre.fm tabulates it: the Magic Formula's
        // S(k) = sin(C atan(B k - E (B k - atan(B k)))) over its peak for k in 0..1, odd in k, held at its k = 1 value
        // beyond.
        double rawShape(const Compound& k, double kappa)
        {
            const double bk = k.b * std::min(std::fabs(kappa), 1.0);
            return std::sin(k.c * std::atan(bk - k.e * (bk - std::atan(bk))));
        }
        double shape(const Compound& k, double kappa)
        {
            double peak = 0.0;
            for (int i = 0; i <= 128; ++i)
                peak = std::max(peak, rawShape(k, i / 128.0));
            const double s = peak > 0.0 ? rawShape(k, kappa) / peak : 0.0;
            return kappa < 0.0 ? -s : s;
        }

        // Telemetry, by name: wheels are front left, front right, rear left, rear right (VEH_WHEELS).
        struct Tel
        {
            const char* speed = "car.speed";
            const char* accel = "car.accel";
            const char* gear = "car.gear";
            const char* engine = "engine.speed";
            const char* redline = "engine.redline";
            const char* load = "car.wheelLoad";
            const char* slip = "car.wheelSlip";
            const char* torque = "car.wheelTorque";
            const char* flags = "car.flags";
        };
        constexpr Tel kTel{};
        const char* const kWheelName[] = { "FL", "FR", "RL", "RR" };
        constexpr double kG = 9.81, kRho = 1.2;

        enum class Drive
        {
            front,
            rear,
            all
        };

        // The groups of fields a click selects, and what each lists in the inspector.
        struct Group
        {
            const char* path;
            const char* title;
            std::vector<const char*> fields;
        };
        const Group kGroups[] = {
            { "car.body", "Mass and geometry", { "car.model", "car.kind", "car.mass", "car.wheelbase", "car.cgHeight", "car.frontWeight", "car.track" } },
            { "car.tyres", "Tyres", { "car.tyre", "car.tyreRadius", "car.wheelInertia", "env.surface", "env.surfaceLeft" } },
            { "car.diffs", "Differentials", { "car.drive", "car.centreSplit", "car.centrePreload", "car.centreKSpeed", "car.centreKTorque",
                                              "car.centreLocked", "car.frontPreload", "car.frontKSpeed", "car.frontKTorque", "car.frontLocked",
                                              "car.rearPreload", "car.rearKSpeed", "car.rearKTorque", "car.rearLocked", "car.liveAxle",
                                              "car.tractionControl" } },
            { "car.gearbox", "Gearbox and ratios", { "car.gearbox", "car.gears", "car.autoGearing", "car.ratio[0]", "car.ratio[1]",
                                                     "car.ratio[2]", "car.ratio[3]", "car.ratio[4]", "car.ratio[5]", "car.ratio[6]",
                                                     "car.ratio[7]", "car.reverseRatio", "car.finalDrive", "car.clutchTorque",
                                                     "car.gearEfficiency", "car.shiftTime", "car.converterK", "car.converterRatio",
                                                     "car.couplingPoint", "car.lockUp", "car.cvtLow", "car.cvtHigh", "car.cvtEfficiency",
                                                     "car.autoClutch" } },
            { "car.aero", "Aero", { "car.cdA", "car.clA", "car.aeroBalance" } },
            { "car.brakes", "Brakes", { "car.brakeDecel", "car.brakeBias", "car.abs" } },
        };

        // A motorcycle's groups (docs/models/bike.md: what a bike reads; what it ignores is not listed).
        const Group kBikeGroups[] = {
            { "car.body", "Bike and rider", { "car.model", "car.kind", "car.mass", "car.wheelbase", "car.cgHeight", "car.frontWeight",
                                              "car.riderMass", "car.riderHeight", "car.riderForward", "car.tuck", "car.pitchInertia" } },
            { "car.tyres", "Tyres", { "car.bikeTyre", "car.tyreRadius", "car.wheelInertia", "env.surface" } },
            { "car.diffs", "Chain, clutch and wheelie", { "car.primaryRatio", "car.frontSprocket", "car.rearSprocket", "car.chainEfficiency",
                                                         "car.chainLash", "car.slipper", "car.slipperTorque", "car.clutchDry", "car.wheelieControl",
                                                         "car.wheelieAngle" } },
            { "car.gearbox", "Gearbox and shifting", { "car.gears", "car.autoGearing", "car.ratio[0]", "car.ratio[1]", "car.ratio[2]",
                                                       "car.ratio[3]", "car.ratio[4]", "car.ratio[5]", "car.clutchTorque",
                                                       "car.gearEfficiency", "car.quickshifter", "car.quickshiftCut", "car.autoBlip",
                                                       "car.primaryRatio", "car.frontSprocket", "car.rearSprocket" } },
            { "car.aero", "Aero", { "car.cdA", "car.cdATuck", "car.tuck" } },
            { "car.brakes", "Brakes", { "car.brakeDecel", "car.brakeBias", "car.abs" } },
        };

        std::string lower(std::string s)
        {
            for (char& ch : s)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            return s;
        }
    }

    class CarWorkspace final : public Workspace
    {
    public:
        explicit CarWorkspace(WorkspaceContext& c) : Workspace(c) {}

        const char* id() const override { return "car"; }
        const char* title() const override { return "CAR"; }
        std::vector<std::string> sections() const override { return { "car", "env" }; }
        bool moving() const override { return wantsFullRate(); }

        std::string inspectorPage() const override
        {
            const bool b = bike();
            for (const Group& g : b ? std::span<const Group>(kBikeGroups) : std::span<const Group>(kGroups))
                if (ctx_.selection.is(g.path))
                {
                    std::vector<int> fields;
                    for (const char* path : g.fields)
                        if (const int f = ctx_.doc.find(path); f >= 0 && !ctx_.doc.info(f).hidden)
                            fields.push_back(f);
                    return pageGui(ctx_.doc, fields, g.title);
                }
            return {};
        }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            k.mix(ctx_.doc.revision()).mix(ctx_.selection.revision()).mix(ctx_.units.revision()).mix(ctx_.dyno.revision());
            for (const char* n : { kTel.speed, kTel.accel, kTel.gear, kTel.engine, "car.pitch" })
                k.mix(static_cast<float>(ctx_.live.get(n)));
            for (int i = 0; i < 4; ++i)
                k.mix(static_cast<float>(wheelTel(kTel.load, i))).mix(static_cast<float>(wheelTel(kTel.slip, i)))
                    .mix(static_cast<float>(wheelTel(kTel.torque, i)));
            return k.h;
        }

        void pointerDown(const funkgui::PointerEvent& e) override
        {
            const funkgui::Point at{ e.x, e.y };
            const Side sv = side();
            const std::string hit = [&]() -> std::string {
                for (int i = 0; i < 2; ++i)
                    if (sv.brake[i].contains(at))
                        return "car.brakes";
                for (int i = 0; i < 2; ++i)
                    if (sv.wheel[i].contains(at))
                        return "car.tyres";
                if (sv.drag.contains(at))
                    return "car.aero";
                if (sv.body.contains(at))
                    return "car.body";
                if (tyreBox_.contains(at))
                    return "car.tyres";
                if (gearBox_.contains(at))
                    return "car.gearbox";
                if (lineBox_.contains(at))
                {
                    const Line l = line();
                    for (const Rect& d : l.diffs)
                        if (d.contains(at))
                            return "car.diffs";
                    if (l.gearbox.contains(at) || l.clutch.contains(at))
                        return "car.gearbox";
                    for (const Rect& w : l.wheels)
                        if (w.contains(at))
                            return "car.tyres";
                    return "car.diffs";
                }
                return {};
            }();
            if (!hit.empty())
                ctx_.selection.select(hit);
            else
                Board::pointerDown(e);
        }

        // For tests: a field's value and whether it came from the build, the top speed (m/s; NaN: none).
        double field(int f, bool* fromBuild = nullptr) const
        {
            for (const char* p : kFields[f].paths)
                if (p != nullptr)
                    if (const int i = ctx_.doc.find(p); i >= 0 && ctx_.doc.info(i).kind != FieldKind::choice &&
                                                        ctx_.doc.info(i).kind != FieldKind::map && ctx_.doc.info(i).kind != FieldKind::curve)
                    {
                        if (fromBuild != nullptr)
                            *fromBuild = true;
                        return ctx_.doc.get(i);
                    }
            if (fromBuild != nullptr)
                *fromBuild = false;
            return kFields[f].fallback;
        }
        double topSpeed(int* gear = nullptr, bool* limiterBound = nullptr) const;

    protected:
        void layout(const Rect& s) override
        {
            const float pad = 16.0f;
            const float topH = std::floor((s.h - 3.0f * pad) * 0.48f);
            const float leftW = std::floor((s.w - 3.0f * pad) * 0.58f);
            sideBox_ = { s.x + pad, s.y + pad, leftW, topH };
            tyreBox_ = { sideBox_.right() + pad, s.y + pad, s.w - 3.0f * pad - leftW, topH };
            const float lowW = std::floor((s.w - 3.0f * pad) * (s.w < 900.0f ? 0.52f : 0.44f));
            lineBox_ = { s.x + pad, sideBox_.bottom() + pad, lowW, s.h - 3.0f * pad - topH };
            gearBox_ = { lineBox_.right() + pad, lineBox_.y, s.w - 3.0f * pad - lowW, lineBox_.h };
        }

        void drawStage(Canvas& c, const funkgui::Theme& th) const override
        {
            const look::Palette p = look::Palette::of(th);
            drawSide(c, p);
            drawTyre(c, p);
            drawLine(c, p);
            drawGearing(c, p);
        }

    private:
        // ---- reading -----------------------------------------------------------------------------------------------
        double wheelTel(const char* name, int i) const
        {
            return ctx_.live.get(std::string(name) + "[" + std::to_string(i) + "]");
        }
        int choiceOf(const char* path, std::string& text) const
        {
            const int f = ctx_.doc.find(path);
            if (f < 0 || ctx_.doc.info(f).kind != FieldKind::choice)
                return -1;
            const FieldInfo& info = ctx_.doc.info(f);
            const int v = static_cast<int>(std::lround(ctx_.doc.get(f) - info.min));
            text = v >= 0 && v < static_cast<int>(info.choices.size()) ? info.choices[static_cast<std::size_t>(v)] : "";
            return v;
        }
        Drive drive(bool* fromBuild = nullptr) const
        {
            std::string t;
            const bool there = choiceOf("car.drive", t) >= 0;
            if (fromBuild != nullptr)
                *fromBuild = there;
            const std::string l = lower(t);
            return l.find("front") != std::string::npos ? Drive::front : l.find("all") != std::string::npos ? Drive::all : Drive::rear;
        }
        // What is carried: the vehicle's mass, and a bike's rider (car.riderMass).
        double totalMass() const
        {
            const int r = bike() ? ctx_.doc.find("car.riderMass") : -1;
            return field(fMass) + (r >= 0 ? ctx_.doc.get(r) : 0.0);
        }
        // A motorcycle: car.kind says so (spec.car.kind "Motorcycle").
        bool bike() const
        {
            std::string t;
            return choiceOf("car.kind", t) >= 0 && lower(t).find("motor") != std::string::npos;
        }
        // The tyre drawn: its compound's curve, its peak grip on a dry road, its name.
        struct Tyre
        {
            const Compound* k;
            double muDry;
            std::string name;
            bool fromBuild;
        };
        Tyre tyre() const
        {
            if (bike())
            {
                std::string t;
                const int i = choiceOf("car.bikeTyre", t);
                const int j = std::clamp(i >= 0 ? i : 1, 0, 5);
                return { &kCompounds[kBikeTyreCompound[j]], kBikeTyreMu[j], kBikeTyreName[j], i >= 0 };
            }
            bool from = true;
            const Compound& k = compound(&from);
            return { &k, k.mu, k.name, from };
        }
        const Compound& compound(bool* fromBuild = nullptr) const
        {
            std::string t;
            const int i = choiceOf("car.tyre", t);
            if (fromBuild != nullptr)
                *fromBuild = i >= 0;
            return kCompounds[i >= 0 && i < static_cast<int>(std::size(kCompounds)) ? i : 1];
        }
        // The surface's column in the tyre model (-1: a surface the model has no column for: drawn as dry).
        int surface(std::string& name) const
        {
            if (choiceOf("env.surface", name) < 0)
            {
                name = "dry";
                return 0;
            }
            const std::string l = lower(name);
            for (int i = 0; i < 5; ++i)
                if (l.find(i == 4 ? "strip" : kSurfaces[i]) != std::string::npos)
                    return i;
            return -1;
        }
        double grip() const
        {
            std::string n;
            const int s = surface(n);
            const Compound& k = compound();
            return k.mu * k.surface[s < 0 ? 0 : s];
        }
        int gears() const { return std::clamp(static_cast<int>(std::lround(field(fGears))), 1, 18); }
        double ratio(int g, bool* fromBuild = nullptr) const   // g from 0
        {
            const int f = ctx_.doc.find("car.ratio[" + std::to_string(g) + "]");
            if (fromBuild != nullptr)
                *fromBuild = f >= 0;
            if (f >= 0)
                return ctx_.doc.get(f);
            return kTypicalRatios[std::min<std::size_t>(static_cast<std::size_t>(g), std::size(kTypicalRatios) - 1)];
        }
        // The limiter's speed, rad/s: the ECU's field when it is set, else the engine's redline.
        double limiter() const
        {
            for (const char* p : { "ecu.limiter.speed", "ecu.limiter" })
                if (const int f = ctx_.doc.find(p); f >= 0 && ctx_.doc.info(f).kind == FieldKind::number && ctx_.doc.get(f) > 0.0)
                    return ctx_.doc.get(f);
            const double r = ctx_.live.get(kTel.redline);
            return std::isfinite(r) && r > 0.0 ? r : 6500.0 * kRpm;
        }
        // Road speed (m/s) at crank speed w (rad/s) in gear g.
        // The overall ratio in gear g: a car's gear and final drive; a bike's primary, gear and sprockets.
        double overall(int g) const
        {
            if (bike())
            {
                const auto f = [&](const char* path, double fallback) {
                    const int i = ctx_.doc.find(path);
                    return i >= 0 ? ctx_.doc.get(i) : fallback;
                };
                return f("car.primaryRatio", 1.7) * ratio(g) * f("car.rearSprocket", 42.0) / std::max(1.0, f("car.frontSprocket", 16.0));
            }
            return ratio(g) * field(fFinal);
        }
        double roadSpeed(double w, int g) const { return w / overall(g) * field(fTyreRadius); }

        // ---- the side view's geometry (also what a press hits) -----------------------------------------------------
        struct Side
        {
            float k = 1.0f, ground = 0.0f, front = 0.0f, rear = 0.0f, r = 0.0f;   // px a metre, the ground's y, the axles' x
            float squat = 0.0f;                                       // px: the rear down
            float lift = 0.0f;                                        // a bike's front lifted, degrees (car.liftAngle)
            std::array<float, look::kCarPoints> xs{}, ys{};           // look::carProfile's outline
            Rect body, wheel[2], brake[2], drag;
        };
        // The load moved to the rear (N): the simulator's own (car.pitch) where it gives it, else from the wheels' loads
        // against their loads at rest; NaN with neither.
        double movedLoad() const
        {
            const double pitch = ctx_.live.get("car.pitch");
            if (std::isfinite(pitch))
                return pitch;
            const double l2 = wheelTel(kTel.load, 2), l3 = wheelTel(kTel.load, 3);
            if (!std::isfinite(l2) || !std::isfinite(l3))
                return std::nan("");
            return l2 + l3 - totalMass() * kG * (1.0 - field(fFront));
        }

        Side side() const
        {
            Side s;
            const Rect a = sideBox_;
            const double wb = field(fWheelbase), r = field(fTyreRadius);
            // look::carProfile's car: 1.62 wheelbases long (0.28 of one ahead of the front axle), 3.9 wheel radii tall; a
            // bike: its wheels and the rider's head, 1.75 m.
            const bool b = bike();
            const double length = b ? wb + 2.0 * r + 0.2 : wb * 1.62 + r * 0.1, ahead = b ? r + 0.1 : 0.48 * 1.62 * wb - 0.5 * wb;
            const double tall = b ? 1.75 : 3.9 * r;
            // Room: two lines of words at the top, the wheelbase over the roof, the axle bars under the ground.
            const float top = a.y + 62.0f + 28.0f;
            s.ground = a.bottom() - 66.0f;
            s.k = static_cast<float>(std::min((a.w - 2.0f * look::kPad - 130.0f) / length, (s.ground - top) / tall));
            s.front = a.x + look::kPad + 8.0f + static_cast<float>(ahead) * s.k;
            s.rear = s.front + static_cast<float>(wb) * s.k;
            s.r = static_cast<float>(r) * s.k;
            // Squat (the rear down) or dive, a few px for the load moved, as DRIVE's car does.
            const double moved = movedLoad();
            s.squat = std::isfinite(moved) ? static_cast<float>(std::clamp(moved / (totalMass() * kG) * 40.0, -6.0, 6.0)) : 0.0f;
            float x0 = 1e9f, x1 = -1e9f, y0 = 1e9f, y1 = -1e9f;
            if (b)
            {
                s.squat = 0.0f;
                const double lift = ctx_.live.get("car.liftAngle");
                s.lift = std::isfinite(lift) ? static_cast<float>(lift) : 0.0f;
                for (const auto& line : bikeLines(s))
                    for (const funkgui::Point& q : line)
                    {
                        x0 = std::min(x0, q.x);
                        x1 = std::max(x1, q.x);
                        y0 = std::min(y0, q.y);
                        y1 = std::max(y1, q.y);
                    }
            }
            else
            {
                look::carProfile(s.front, s.rear, s.ground - s.r, s.r, s.squat, s.xs.data(), s.ys.data());
                for (int i = 0; i < look::kCarPoints; ++i)
                {
                    x0 = std::min(x0, s.xs[static_cast<std::size_t>(i)]);
                    x1 = std::max(x1, s.xs[static_cast<std::size_t>(i)]);
                    y0 = std::min(y0, s.ys[static_cast<std::size_t>(i)]);
                    y1 = std::max(y1, s.ys[static_cast<std::size_t>(i)]);
                }
            }
            s.body = { x0, y0, x1 - x0, y1 - y0 };
            for (int i = 0; i < 2; ++i)
            {
                const float x = i == 0 ? s.front : s.rear;
                s.wheel[i] = { x - s.r, s.ground - 2.0f * s.r, 2.0f * s.r, 2.0f * s.r };
                s.brake[i] = { x - s.r * 0.25f, s.ground - s.r - s.r * 0.55f, s.r * 0.5f, s.r * 0.35f };
            }
            s.drag = { s.body.right() + 4.0f, s.body.y, 120.0f, s.body.h };
            return s;
        }
        // A motorcycle from the side, as polylines in px: swingarm, frame and tank, fairing, engine, fork, the rider (from
        // the seat, tucked by car.tuck), each point in metres ahead of the rear axle and up from the ground, the whole
        // turned about the rear tyre's contact by the lift (car.liftAngle): no Canvas rotation, the points are turned.
        // The last line is the front wheel's centre (one point) and the head's (one point).
        std::vector<std::vector<funkgui::Point>> bikeLines(const Side& s) const
        {
            const double wb = field(fWheelbase), r = field(fTyreRadius);
            const int ti = ctx_.doc.find("car.tuck");
            const double t = ti >= 0 ? std::clamp(ctx_.doc.get(ti), 0.0, 1.0) : 1.0;
            const double a = s.lift * chart::kPi / 180.0, ca = std::cos(a), sa = std::sin(a);
            const auto P = [&](double x, double y) {
                const double xr = x * ca - y * sa, yr = x * sa + y * ca;   // turned about the contact, front up
                return funkgui::Point{ s.rear - static_cast<float>(xr) * s.k, s.ground - static_cast<float>(yr) * s.k };
            };
            const double sx = 0.48 * wb + 0.08 * wb * t, sy = 1.38 - 0.25 * t;   // the rider's shoulder
            return {
                { P(0.0, r), P(0.36 * wb, r + 0.10) },                                                       // swingarm
                { P(-0.12 * wb, 0.78), P(0.05 * wb, 0.80), P(0.30 * wb, 0.80), P(0.42 * wb, 0.86), P(0.58 * wb, 0.98),
                  P(0.74 * wb, 0.95) },                                                                      // tail, seat, tank
                { P(0.74 * wb, 0.95), P(0.88 * wb, 0.99), P(0.92 * wb, 0.72), P(0.80 * wb, 0.50) },            // fairing
                { P(0.38 * wb, 0.30), P(0.68 * wb, 0.30), P(0.70 * wb, 0.62), P(0.42 * wb, 0.66), P(0.38 * wb, 0.30) },   // engine
                { P(0.74 * wb, 0.95), P(wb, r) },                                                              // fork
                { P(0.28 * wb, 0.88), P(sx, sy), P(0.74 * wb, 1.02) },                                         // back, arm
                { P(0.28 * wb, 0.88), P(0.45 * wb, 0.62), P(0.32 * wb, 0.40) },                                // leg
                { P(wb, r), P(sx + 0.06 * wb, sy + 0.16) },                                                    // centres
            };
        }

        // The top of the body at x (the profile's highest point over it).
        static float bodyTop(const Side& s, float x)
        {
            float y = s.ground;
            for (int i = 0; i + 1 < look::kCarPoints; ++i)
            {
                const float xa = s.xs[static_cast<std::size_t>(i)], xb = s.xs[static_cast<std::size_t>(i + 1)];
                if ((x - xa) * (x - xb) > 0.0f || xa == xb)
                    continue;
                const float t = (x - xa) / (xb - xa);
                y = std::min(y, s.ys[static_cast<std::size_t>(i)] + t * (s.ys[static_cast<std::size_t>(i + 1)] - s.ys[static_cast<std::size_t>(i)]));
            }
            return y;
        }

        // ---- drawing: the car from the side ------------------------------------------------------------------------
        void drawSide(Canvas& c, const look::Palette& p) const
        {
            const bool b = bike();
            look::region(c, p, sideBox_, b ? "THE BIKE FROM THE SIDE, TO SCALE" : "THE CAR FROM THE SIDE, TO SCALE",
                         ctx_.selection.is("car.body") || ctx_.selection.is("car.aero"));
            const Side s = side();
            const Units& u = ctx_.units;
            bool fromWb = true, fromCg = true, fromFw = true, fromMass = true;
            const double wb = field(fWheelbase, &fromWb), cg = field(fCg, &fromCg), fw = field(fFront, &fromFw);
            field(fMass, &fromMass);
            const double mass = totalMass();
            bool any = !(fromWb && fromCg && fromFw && fromMass);
            const auto star = [](bool from) { return from ? "" : "*"; };
            // The ground.
            c.segment(sideBox_.x + look::kPad, s.ground, sideBox_.right() - look::kPad, s.ground, 1.0f, p.edge);
            // The body as its profile (look::carProfile, DRIVE's car), the side window's line; selected: in the accent, ringed.
            if (b)
            {
                const bool sel = ctx_.selection.is("car.body");
                const auto lines = bikeLines(s);
                for (std::size_t i = 0; i + 1 < lines.size(); ++i)
                {
                    std::vector<float> xs, ys;
                    for (const funkgui::Point& q : lines[i])
                    {
                        xs.push_back(q.x);
                        ys.push_back(q.y);
                    }
                    c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), i >= 5 ? 1.75f : 2.0f,
                               i >= 5 ? p.label : sel ? p.selected : p.value);
                }
                const funkgui::Point head = lines.back()[1];
                c.disc(head.x, head.y, 0.12f * s.k, p.panel, 1.75f, p.label);
                if (sel)
                    look::selectRing(c, p, s.body, 6.0f);
            }
            else
            {
                const bool sel = ctx_.selection.is("car.body");
                c.polyline(s.xs.data(), s.ys.data(), look::kCarPoints, 2.0f, sel ? p.selected : p.value);
                float wx0 = 0.0f, wy0 = 0.0f, wx1 = 0.0f, wy1 = 0.0f;
                look::carWindow(s.front, s.rear, s.ground - s.r, s.r, s.squat, wx0, wy0, wx1, wy1);
                c.segment(wx0, wy0, wx1, wy1, 1.25f, p.label);
                if (sel)
                    look::selectRing(c, p, s.body, 6.0f);
            }
            // The wheels: tyre, rim, the brake's caliper.
            for (int i = 0; i < 2; ++i)
            {
                Rect w = s.wheel[i];
                if (b && i == 0)   // a bike's front wheel goes up with the lift
                {
                    const funkgui::Point fc = bikeLines(s).back()[0];
                    w = { fc.x - s.r, fc.y - s.r, 2.0f * s.r, 2.0f * s.r };
                }
                c.disc(w.centreX(), w.centreY(), s.r, p.ground, 2.0f, p.value);
                c.disc(w.centreX(), w.centreY(), s.r * (b ? 0.8f : 0.62f), p.panel, 1.25f, p.label);
                const Rect& cal = s.brake[i];
                if (!b || i == 1)
                    c.rrect(cal.x, cal.y, cal.w, cal.h, 2.0f, ctx_.selection.is("car.brakes") ? p.selected : p.label);
                if (ctx_.selection.is("car.tyres"))
                    c.disc(w.centreX(), w.centreY(), s.r + 4.0f, kClear, 2.0f, p.selected);
            }
            // The wheelbase, over the roof.
            const float dy = s.body.y - 14.0f;
            c.segment(s.front, dy, s.rear, dy, 1.0f, p.label);
            c.segment(s.front, dy - 5.0f, s.front, dy + 5.0f, 1.0f, p.label);
            c.segment(s.rear, dy - 5.0f, s.rear, dy + 5.0f, 1.0f, p.label);
            const std::string wbt = "WHEELBASE " + u.format(Quantity::length, wb, 0) + star(fromWb);
            c.text(wbt.c_str(), (s.front + s.rear) * 0.5f, dy - 15.0f, look::kCaps, p.text, funkgui::Align::centre);
            // The centre of mass: a disc with a cross, at its height and its place between the axles.
            const float cx = s.front + static_cast<float>((1.0 - fw) * wb) * s.k, cy = s.ground - static_cast<float>(cg) * s.k;
            c.disc(cx, cy, 7.0f, p.ground, 2.0f, p.value);
            c.segment(cx - 7.0f, cy, cx + 7.0f, cy, 1.5f, p.value);
            c.segment(cx, cy - 7.0f, cx, cy + 7.0f, 1.5f, p.value);
            c.segment(cx, cy + 9.0f, cx, s.ground, 1.0f, p.dim);
            c.text("CM", cx - 8.0f, cy - 22.0f, look::kCaps, p.value);
            const std::string cgWords = u.format(Quantity::mass, mass, 0) + star(fromMass) + (b ? " WITH RIDER" : "") + ", " + look::num(fw * 100.0) +
                                    star(fromFw) + " % ON THE FRONT; CM " + u.format(Quantity::length, cg, 0) + star(fromCg) +
                                     " HIGH";
            // The load on each axle, live: a bar under it, a tick at the load at rest, the number and its share.
            const double weight = mass * kG;
            const double rest[2] = { weight * fw, weight * (1.0 - fw) };
            double live[2] = { std::nan(""), std::nan("") };
            for (int a = 0; a < 2; ++a)
            {
                const double l0 = wheelTel(kTel.load, a * 2), l1 = wheelTel(kTel.load, a * 2 + 1);
                if (std::isfinite(l0) && std::isfinite(l1))
                    live[a] = l0 + l1;
            }
            for (int a = 0; a < 2; ++a)
            {
                const float x = a == 0 ? s.front : s.rear;
                // The axles close together (a bike, a small stage): narrower bars, the share under the number. Each
                // block kept inside the panel.
                const float spacing = s.rear - s.front, bw = std::min(92.0f, spacing - 14.0f);
                const bool close = spacing < 120.0f;
                const float half = close ? std::max(bw, 60.0f) * 0.5f : 70.0f;
                const float xc = std::clamp(x, sideBox_.x + look::kPad + half, sideBox_.right() - look::kPad - half);
                const Rect bar{ xc - bw * 0.5f, s.ground + 10.0f, bw, 8.0f };
                const double share = std::isfinite(live[a]) ? live[a] / weight : std::nan("");
                look::level(c, p, bar, share, a == 0 ? p.cold : p.hot, rest[a] / weight);
                const std::string n = std::isfinite(live[a]) ? look::num(live[a] / 1000.0, "%.2f") + " kN" : "- kN";
                const std::string sh = std::isfinite(share) ? look::num(share * 100.0) + " %" : "";
                c.text(a == 0 ? (close ? "FRONT" : "FRONT AXLE") : (close ? "REAR" : "REAR AXLE"), xc, s.ground + 24.0f, look::kCaps,
                       p.label, funkgui::Align::centre);
                c.text((close ? n : n + "  " + sh).c_str(), xc, s.ground + 38.0f, close ? look::kUnit : look::kNumberS,
                       std::isfinite(live[a]) ? p.value : p.dim, funkgui::Align::centre);
                if (close && !sh.empty())
                    c.text(sh.c_str(), xc, s.ground + 50.0f, look::kUnit, p.label, funkgui::Align::centre);
            }
            // Squat or dive, in words: the load moved from where it rests.
            if (std::isfinite(live[1]))
            {
                const double moved = movedLoad();
                const std::string w = std::fabs(moved) < 50.0 ? "LOADS AT REST"
                                    : moved > 0.0 ? "SQUAT: " + look::num(moved / 1000.0, "%.2f") + " kN MOVED TO THE REAR"
                                                  : "DIVE: " + look::num(-moved / 1000.0, "%.2f") + " kN MOVED TO THE FRONT";
                const float hw = c.textWidth(w.c_str(), look::kCaps) * 0.5f;
                const bool close = s.rear - s.front < 120.0f;
                const float wx = close ? s.rear + 70.0f + hw   // beside the rear axle's block when the axles are close
                                       : (s.front + s.rear) * 0.5f;
                c.text(w.c_str(), std::clamp(wx, sideBox_.x + look::kPad + hw, sideBox_.right() - look::kPad - hw),
                       close ? s.ground + 24.0f : s.ground + 56.0f, look::kCaps,
                       std::fabs(moved) < 50.0 ? p.label : p.value, funkgui::Align::centre);
            }

            // Aero at the speed it is doing: drag as an arrow back from the tail, downforce as arrows down on the axles.
            const double v = ctx_.live.get(kTel.speed);
            bool fromCd = true, fromCl = true, fromBal = true;
            const double cdA = field(fCdA, &fromCd), clA = field(fClA, &fromCl), bal = field(fAeroBalance, &fromBal);
            any = any || !fromCd || !fromCl || !fromBal;
            const double q = std::isfinite(v) ? 0.5 * kRho * v * v : std::nan("");
            const double drag = q * cdA, down = q * clA;
            const float ay = s.body.centreY();
            const float len = std::isfinite(drag) ? static_cast<float>(std::min(110.0, 18.0 + drag / 40.0)) : 0.0f;
            const bool aeroSel = ctx_.selection.is("car.aero");
            if (len > 0.0f)
            {
                const float x0 = s.body.right() + 6.0f, x1 = x0 + len;
                c.segment(x0, ay, x1, ay, 2.0f, aeroSel ? p.selected : p.cold);
                c.segment(x1, ay, x1 - 7.0f, ay - 5.0f, 2.0f, aeroSel ? p.selected : p.cold);
                c.segment(x1, ay, x1 - 7.0f, ay + 5.0f, 2.0f, aeroSel ? p.selected : p.cold);
            }
            const std::string dt = "DRAG " + (std::isfinite(drag) ? look::num(drag / 1000.0, "%.2f") + " kN" : std::string("-"));
            c.text(dt.c_str(), s.body.right() + 8.0f, ay + 10.0f, look::kCaps, p.text);
            if (std::isfinite(down) && std::fabs(clA) > 1e-6 && !b)
                for (int a = 0; a < 2; ++a)
                {
                    const double f = down * (a == 0 ? bal : 1.0 - bal);
                    // Down onto the bonnet over the front axle and onto the deck over the rear one.
                    const float x = a == 0 ? s.front : s.rear;
                    const float surface = bodyTop(s, x) - 3.0f;
                    const float l = static_cast<float>(std::min(26.0, 10.0 + std::fabs(f) / 30.0));
                    const float y0 = surface - l, y1 = surface;
                    const float tip = f >= 0.0 ? y1 : y0, tail = f >= 0.0 ? y0 : y1, d = f >= 0.0 ? -6.0f : 6.0f;
                    c.segment(x, tail, x, tip, 2.0f, p.cold);
                    c.segment(x, tip, x - 5.0f, tip + d, 2.0f, p.cold);
                    c.segment(x, tip, x + 5.0f, tip + d, 2.0f, p.cold);
                    // The number beside it, outward: left of the front arrow, right of the rear one, at its tail.
                    c.text((look::num(f / 1000.0, "%.2f") + " kN").c_str(), a == 0 ? x - 8.0f : x + 8.0f, y0 - 6.0f, look::kUnit,
                           p.text, a == 0 ? funkgui::Align::right : funkgui::Align::left);
                }
            // The words: two lines (what the aero is drawn from; the mass and the centre of mass), and the note on
            // starred values at the title's right.
            const float ix = sideBox_.x + look::kPad, iw = sideBox_.w - 2.0f * look::kPad;
            const std::string l1 = "AERO AT " + (std::isfinite(v) ? u.format(Quantity::speed, v, 0) : std::string("-")) + ": CD A " +
                                   look::num(cdA, "%.2f") + star(fromCd) +
                                   (b ? std::string(" M2") : ", CL A " + look::num(clA, "%.2f") + star(fromCl) + " M2");
            chart::wrap(c, l1, ix, sideBox_.y + 30.0f, iw, look::kUnit, p.label, 13.0f, 1);
            chart::wrap(c, cgWords, ix, sideBox_.y + 44.0f, iw, look::kUnit, p.label, 13.0f, 1);
            if (any)
            {
                const char* note = c.textWidth("THE CAR FROM THE SIDE, TO SCALE", look::kCaps) + 200.0f < iw
                                       ? "* NOT IN THIS BUILD: THE SPEC'S DEFAULT" : "* SPEC DEFAULT";
                c.text(note, sideBox_.right() - look::kPad, sideBox_.y + 9.0f, look::kUnit, p.dim, funkgui::Align::right);
            }
        }

        // ---- the tyre's grip curve ---------------------------------------------------------------------------------
        void drawTyre(Canvas& c, const look::Palette& p) const
        {
            const Tyre ty = tyre();
            const Compound& k = *ty.k;
            const bool fromC = ty.fromBuild;
            std::string sname;
            const int s = surface(sname);
            const std::string title = "GRIP: " + upper(ty.name) + (fromC ? "" : "*") + " TYRES ON " + upper(sname);
            look::region(c, p, tyreBox_, title, ctx_.selection.is("car.tyres"));
            const Rect a{ tyreBox_.x + 46.0f, tyreBox_.y + 40.0f, tyreBox_.w - 46.0f - look::kPad - 4.0f, tyreBox_.h - 40.0f - 40.0f };
            const double mu = ty.muDry * k.surface[s < 0 ? 0 : s];
            const double yHi = std::ceil(std::max(mu, ty.muDry) * 1.15 * 5.0) / 5.0;
            const chart::Scale xs{ 0.0, 0.5, a.x, a.right() }, ys{ 0.0, yHi, a.bottom(), a.y };
            for (double y = 0.0; y <= yHi + 1e-9; y += 0.2)
            {
                c.hairlineH(a.x, ys.px(y), a.w, y == 0.0 ? p.edge : p.grid);
                c.text(look::num(y, "%.1f").c_str(), a.x - 6.0f, ys.px(y) - 5.0f, look::kUnit, p.label, funkgui::Align::right);
            }
            for (int i = 0; i <= 5; ++i)
            {
                const double x = 0.1 * i;
                c.hairlineV(xs.px(x), a.y, a.h, p.grid);
                c.text((look::num(x * 100.0) + "%").c_str(), xs.px(x), a.bottom() + 5.0f, look::kUnit, p.label, funkgui::Align::centre);
            }
            c.text("SLIP", a.right(), a.bottom() + 18.0f, look::kCaps, p.label, funkgui::Align::right);
            c.text("FORCE / LOAD", a.x - 40.0f, a.y - 14.0f, look::kCaps, p.label);
            std::vector<float> px, py, pd;
            double peakK = 0.0, peak = 0.0;
            for (int i = 0; i <= 100; ++i)
            {
                const double kappa = 0.005 * i;
                const double f = mu * shape(k, kappa);
                px.push_back(xs.px(kappa));
                py.push_back(ys.px(f));
                pd.push_back(ys.px(ty.muDry * shape(k, kappa)));
                if (f > peak)
                {
                    peak = f;
                    peakK = kappa;
                }
            }
            if (s != 0)   // the same tyre on a dry road, for comparison
                chart::stroke(c, px, pd, 1.25f, p.dim, { 4.0f, 4.0f });
            chart::stroke(c, px, py, 2.25f, p.text);
            c.disc(xs.px(peakK), ys.px(peak), 4.0f, p.text);
            const std::string pk = "PEAK " + look::num(peak, "%.2f") + " AT " + look::num(peakK * 100.0) + " % SLIP";
            c.text(pk.c_str(), xs.px(peakK) + 8.0f, ys.px(peak) - 16.0f, look::kCaps, p.text);
            if (s != 0)
                c.text("DASHED: DRY", a.right(), a.y - 14.0f, look::kCaps, p.dim, funkgui::Align::right);
            if (s < 0)
                c.text(("the tyre model has no " + sname + ": drawn as dry").c_str(), a.x + 8.0f, a.bottom() - 16.0f, look::kUnit, p.dim);
            // Each driven wheel's live point; a wheel past the peak is slipping (amber, and the word).
            const bool b = bike();
            const Drive d = b ? Drive::rear : drive();
            int shown = 0;
            for (int w = 0; w < 4; ++w)
            {
                const bool driven = d == Drive::all || (d == Drive::front) == (w < 2);
                const double sl = wheelTel(kTel.slip, w);
                if (!driven || !std::isfinite(sl) || (b && w % 2 == 1))   // a bike's wheels are [0] and [2]
                    continue;
                const double kap = std::clamp(std::fabs(sl), 0.0, 0.5);
                const double f = mu * shape(k, kap);
                const bool past = kap > peakK;
                const float x = xs.px(kap), y = ys.px(f);
                c.disc(x, y, 5.5f, past ? p.warning : p.ok, 2.0f, p.ground);
                const std::string lab = std::string(b ? (w < 2 ? "FRONT" : "REAR") : kWheelName[w]) + (past ? " PAST THE PEAK" : "");
                const float ly = y + 10.0f + 14.0f * static_cast<float>(shown);
                c.rrect(x + 2.0f, ly - 2.0f, c.textWidth(lab.c_str(), look::kCaps) + 8.0f, 14.0f, 3.0f, p.panel);
                c.text(lab.c_str(), x + 6.0f, ly, look::kCaps, past ? p.warning : p.ok);
                ++shown;
            }
            if (shown == 0)
            {
                const char* w = "- NO WHEEL'S SLIP IN THIS PROGRAM YET";
                c.rrect(a.x + 6.0f, a.bottom() - 28.0f, c.textWidth(w, look::kUnit) + 8.0f, 16.0f, 3.0f, p.panel);
                c.text(w, a.x + 10.0f, a.bottom() - 25.0f, look::kUnit, p.dim);
            }
        }

        // ---- the driveline -----------------------------------------------------------------------------------------
        struct Line
        {
            Rect engine, clutch, gearbox, wheels[4];
            std::vector<Rect> diffs;   // front, rear, centre, as used
            Rect front, rear, centre;  // empty when not used
            bool compact = false;      // a narrow box: the clutch drawn in the gearbox, all-wheel drive's centre too
            bool bike = false;         // a motorcycle: a chain to the rear wheel, one wheel an axle
        };
        Line line() const
        {
            Line l;
            const Rect a{ lineBox_.x + look::kPad, lineBox_.y + 48.0f, lineBox_.w - 2.0f * look::kPad, lineBox_.h - 48.0f - 40.0f };
            const float fx = a.x + a.w * 0.12f, rx = a.right() - a.w * 0.10f, cy = a.centreY();
            const float ww = std::min(64.0f, a.w * 0.1f), wh = std::min(28.0f, a.h * 0.14f);
            l.wheels[0] = { fx - ww * 0.5f, a.y, ww, wh };
            l.wheels[1] = { fx - ww * 0.5f, a.bottom() - wh, ww, wh };
            l.wheels[2] = { rx - ww * 0.5f, a.y, ww, wh };
            l.wheels[3] = { rx - ww * 0.5f, a.bottom() - wh, ww, wh };
            l.compact = a.w < 400.0f;
            const float bh = std::min(52.0f, a.h * 0.26f), bw = std::max(66.0f, std::min(124.0f, a.w * 0.19f));
            const float gap = std::max(8.0f, std::min(18.0f, a.w * 0.025f));
            const float dw = std::max(56.0f, std::min(104.0f, a.w * 0.17f)), dh = bh * 0.8f;
            const Drive d = drive();
            const bool b = bike();
            float x = b ? fx + ww * 0.5f + gap : fx + (d == Drive::rear ? -bw * 0.4f : dw * 0.5f + gap);
            l.engine = { x, cy - bh * 0.5f, bw, bh };
            x += bw + gap;
            if (!l.compact)
            {
                l.clutch = { x, cy - bh * 0.4f, std::max(58.0f, bw * 0.62f), bh * 0.8f };
                x += l.clutch.w + gap;
            }
            l.gearbox = { x, cy - bh * 0.5f, bw, bh };
            if (d != Drive::rear)
                l.front = { fx - dw * 0.5f, cy - dh * 0.5f, dw, dh };
            if (d != Drive::front)
                l.rear = { rx - dw * 0.5f, cy - dh * 0.5f, dw, dh };
            if (d == Drive::all && !l.compact)
                l.centre = { l.gearbox.right() + gap, cy - dh * 0.5f, dw, dh };
            if (b)
            {
                // One wheel an axle, on the centre line; the chain (the sprockets) just ahead of the rear wheel.
                l.wheels[0] = { fx - ww * 0.5f, cy - wh * 0.5f, ww, wh };
                l.wheels[2] = { rx - ww * 0.5f, cy - wh * 0.5f, ww, wh };
                l.wheels[1] = l.wheels[3] = Rect{};
                l.front = l.centre = Rect{};
                // Engine, clutch, gearbox and chain, evenly between the wheels.
                const float x0 = l.wheels[0].right() + gap, x1 = l.wheels[2].x - gap;
                const float n = l.compact ? 3.0f : 4.0f, w = std::min(bw, (x1 - x0 - gap * (n - 1.0f)) / n);
                float bx = x0;
                l.engine = { bx, cy - bh * 0.5f, w, bh };
                bx += w + gap;
                if (!l.compact)
                {
                    l.clutch = { bx, cy - bh * 0.4f, w, bh * 0.8f };
                    bx += w + gap;
                }
                l.gearbox = { bx, cy - bh * 0.5f, w, bh };
                bx += w + gap;
                l.rear = { bx, cy - dh * 0.5f, w, dh };
                l.bike = true;
            }
            for (const Rect* r : { &l.front, &l.rear, &l.centre })
                if (!r->isEmpty())
                    l.diffs.push_back(*r);
            return l;
        }
        std::string diffKind(const char* which) const
        {
            const auto get = [&](const char* f) {
                const int i = ctx_.doc.find(std::string("car.") + which + f);
                return i >= 0 ? ctx_.doc.get(i) : std::nan("");
            };
            const double locked = get("Locked"), pre = get("Preload"), ks = get("KSpeed"), kt = get("KTorque");
            if (!std::isfinite(locked) && !std::isfinite(pre))
                return "KIND -";
            if (locked >= 0.5)
                return "LOCKED";
            if (kt > 0.0)
                return "T. BIAS";
            if (ks > 0.0)
                return "VISCOUS";
            if (pre > 0.0)
                return "LSD";
            return "OPEN";
        }

        void drawLine(Canvas& c, const look::Palette& p) const
        {
            bool fromD = true;
            const Drive d = bike() ? Drive::rear : drive(&fromD);
            fromD = fromD || bike();
            const char* layout = bike() ? "CHAIN TO THE REAR WHEEL"
                               : d == Drive::front ? "FRONT-WHEEL DRIVE" : d == Drive::rear ? "REAR-WHEEL DRIVE" : "ALL-WHEEL DRIVE";
            look::region(c, p, lineBox_, std::string("DRIVELINE  ") + layout + (fromD ? "" : "*"), ctx_.selection.is("car.diffs"));
            const Line l = line();
            const Units& u = ctx_.units;
            const Col shaft = p.label;
            const float cy = l.engine.centreY();
            // Shafts first, under the boxes.
            if (l.compact)
                c.segment(l.engine.right(), cy, l.gearbox.x, cy, 4.0f, shaft);
            else
            {
                c.segment(l.engine.right(), cy, l.clutch.x, cy, 4.0f, shaft);
                c.segment(l.clutch.right(), cy, l.gearbox.x, cy, 4.0f, shaft);
            }
            const auto axle = [&](const Rect& diff, int w0) {
                const float x = diff.centreX();
                c.segment(x, l.wheels[w0].bottom(), x, diff.y, 3.0f, shaft);
                c.segment(x, diff.bottom(), x, l.wheels[w0 + 1].y, 3.0f, shaft);
            };
            const auto deadAxle = [&](int w0) {
                const float x = l.wheels[w0].centreX();
                c.segment(x, l.wheels[w0].bottom(), x, l.wheels[w0 + 1].y, 1.0f, p.dim);
            };
            const float under = cy + l.engine.h * 0.5f + 14.0f;
            if (l.bike)
            {
                // The gearbox's sprocket to the chain's (two runs of chain), the chain to the wheel.
                c.segment(l.gearbox.right(), cy - 6.0f, l.rear.x, cy - 6.0f, 2.0f, shaft);
                c.segment(l.gearbox.right(), cy + 6.0f, l.rear.x, cy + 6.0f, 2.0f, shaft);
                c.segment(l.rear.right(), cy, l.wheels[2].x, cy, 4.0f, shaft);
            }
            else if (d == Drive::rear)
            {
                c.segment(l.gearbox.right(), cy, l.rear.x, cy, 4.0f, shaft);
                axle(l.rear, 2);
                deadAxle(0);
            }
            else if (d == Drive::front)
            {
                // A transaxle: the gearbox's output back under it to the front differential.
                c.segment(l.gearbox.centreX(), l.gearbox.bottom(), l.gearbox.centreX(), under, 3.0f, shaft);
                c.segment(l.gearbox.centreX(), under, l.front.centreX(), under, 3.0f, shaft);
                c.segment(l.front.centreX(), under, l.front.centreX(), l.front.bottom(), 3.0f, shaft);
                axle(l.front, 0);
                deadAxle(2);
            }
            else
            {
                // The centre differential: its own box, or (compact) inside the gearbox as a transfer case.
                const Rect& cd = l.centre.isEmpty() ? l.gearbox : l.centre;
                if (!l.centre.isEmpty())
                    c.segment(l.gearbox.right(), cy, l.centre.x, cy, 4.0f, shaft);
                c.segment(cd.right(), cy, l.rear.x, cy, 4.0f, shaft);
                c.segment(cd.centreX(), cd.bottom(), cd.centreX(), under, 3.0f, shaft);
                c.segment(cd.centreX(), under, l.front.centreX(), under, 3.0f, shaft);
                c.segment(l.front.centreX(), under, l.front.centreX(), l.front.bottom(), 3.0f, shaft);
                axle(l.front, 0);
                axle(l.rear, 2);
            }
            const auto box = [&](const Rect& r, const std::string& name, const std::string& sub, bool sel) {
                c.rrect(r.x, r.y, r.w, r.h, 5.0f, p.raised, sel ? 2.0f : 1.25f, sel ? p.selected : p.text);
                c.text(name.c_str(), r.centreX(), r.y + 5.0f, look::kCaps, p.value, funkgui::Align::centre);
                if (!sub.empty())
                    c.text(sub.c_str(), r.centreX(), r.y + r.h - 15.0f, look::kUnit, p.label, funkgui::Align::centre);
            };
            std::string gb;
            const int gbx = choiceOf("car.gearbox", gb);
            const std::string kind = lower(gb);
            const bool conv = kind.find("automatic") != std::string::npos, cvt = kind.find("cvt") != std::string::npos;
            box(l.engine, "ENGINE", ctx_.live.has(kTel.engine) ? u.format(Quantity::rotation, ctx_.live.get(kTel.engine), 0) : "-", false);
            const int slipperField = ctx_.doc.find("car.slipper");
            const bool slipper = l.bike && slipperField >= 0 && ctx_.doc.get(slipperField) >= 0.5;
            if (!l.compact)
                box(l.clutch, slipper ? "SLIPPER" : conv ? "CONV." : cvt ? "BELT" : "CLUTCH", "", ctx_.selection.is("car.gearbox"));
            const double g = ctx_.live.get(kTel.gear);
            const std::string inGear = std::isfinite(g) ? (g < 0 ? "IN R" : g < 0.5 ? "IN N" : "IN " + look::num(g)) : "-";
            std::string gsub = look::num(gears()) + " GEARS  " + inGear;
            if (l.compact || c.textWidth(gsub.c_str(), look::kUnit) > l.gearbox.w - 10.0f)
                gsub = inGear;
            if (l.compact && d == Drive::all)
                gsub = look::num(field(fCentreSplit) * 100.0) + " % FRONT";
            box(l.gearbox, l.bike ? "GEARBOX" : gbx >= 0 ? upper(gb) : "GEARBOX", gsub, ctx_.selection.is("car.gearbox"));
            const bool dsel = ctx_.selection.is("car.diffs");
            // The torque reaching each axle: the sum of its wheels' drive torque.
            const auto axleTorque = [&](int w0) {
                const double a = wheelTel(kTel.torque, w0), b = wheelTel(kTel.torque, w0 + 1);
                return std::isfinite(a) && std::isfinite(b) ? a + b : std::nan("");
            };
            if (!l.front.isEmpty())
            {
                box(l.front, "FRONT", diffKind("front"), dsel);
                const double t = axleTorque(0);
                c.text("FRONT AXLE TORQUE", l.front.right() + 6.0f, l.front.y - 30.0f, look::kCaps, p.label);
                c.text(std::isfinite(t) ? u.format(Quantity::torque, t, 0).c_str() : "- N m", l.front.right() + 6.0f, l.front.y - 16.0f,
                       look::kNumberS, std::isfinite(t) ? p.value : p.dim);
            }
            if (!l.rear.isEmpty())
            {
                if (l.bike)
                {
                    const auto teeth = [&](const char* path, double fallback) {
                        const int i = ctx_.doc.find(path);
                        return look::num(i >= 0 ? ctx_.doc.get(i) : fallback);
                    };
                    box(l.rear, "CHAIN", teeth("car.frontSprocket", 16.0) + " / " + teeth("car.rearSprocket", 42.0), dsel);
                }
                else
                    box(l.rear, "REAR", diffKind("rear"), dsel);
                const double t = axleTorque(2);
                c.text(l.bike ? "REAR WHEEL TORQUE" : "REAR AXLE TORQUE", l.rear.x - 6.0f, l.rear.y - 30.0f, look::kCaps, p.label, funkgui::Align::right);
                c.text(std::isfinite(t) ? u.format(Quantity::torque, t, 0).c_str() : "- N m", l.rear.x - 6.0f, l.rear.y - 16.0f,
                       look::kNumberS, std::isfinite(t) ? p.value : p.dim, funkgui::Align::right);
            }
            if (!l.centre.isEmpty())
            {
                bool fromS = true;
                const double split = field(fCentreSplit, &fromS);
                box(l.centre, "CENTRE", look::num(split * 100.0) + (fromS ? "" : "*") + " % FRONT", dsel);
            }
            // The wheels: driven ones filled; a slipping one outlined in amber, with the word and its slip.
            for (int w = 0; w < 4; ++w)
            {
                const Rect& r = l.wheels[w];
                if (r.isEmpty())
                    continue;
                const bool driven = d == Drive::all || (d == Drive::front) == (w < 2);
                const double sl = wheelTel(kTel.slip, w);
                const bool slipping = driven && std::isfinite(sl) && std::fabs(sl) > 0.1;
                c.rrect(r.x, r.y, r.w, r.h, 4.0f, driven ? p.text : p.panel, slipping ? 2.5f : 1.25f, slipping ? p.warning : p.label);
                const bool top = w % 2 == 0;
                const float ty = top ? r.y - 14.0f : r.bottom() + 4.0f;
                std::string lab = l.bike ? (w < 2 ? "FRONT" : "REAR") : kWheelName[w];
                if (slipping)
                    lab += "  SLIP " + look::num(std::fabs(sl) * 100.0) + " %";
                c.text(lab.c_str(), r.centreX(), ty, look::kCaps, slipping ? p.warning : p.label, funkgui::Align::centre);
            }
            c.text("FRONT", lineBox_.x + look::kPad, lineBox_.bottom() - 22.0f, look::kCaps, p.dim);
            c.text("REAR", lineBox_.right() - look::kPad, lineBox_.bottom() - 22.0f, look::kCaps, p.dim, funkgui::Align::right);
        }

        // ---- the gearing chart -------------------------------------------------------------------------------------
        void drawGearing(Canvas& c, const look::Palette& p) const
        {
            const bool narrow = gearBox_.w < 520.0f;
            look::region(c, p, gearBox_, narrow ? "GEARING" : "GEARING: ROAD SPEED AGAINST ENGINE SPEED", ctx_.selection.is("car.gearbox"));
            const Units& u = ctx_.units;
            const float side = narrow ? 104.0f : 150.0f;
            const Rect a{ gearBox_.x + 52.0f, gearBox_.y + 40.0f, gearBox_.w - 52.0f - side, gearBox_.h - 40.0f - 62.0f };
            const int n = gears();
            const double lim = limiter();
            bool fromR = true, any = false;
            for (int g = 0; g < n; ++g)
            {
                bool f = true;
                ratio(g, &f);
                fromR = fromR && f;
            }
            double vMax = 0.0;
            for (int g = 0; g < n; ++g)
                vMax = std::max(vMax, roadSpeed(lim, g));
            int topGear = 0;
            bool bound = false;
            const double vTop = topSpeed(&topGear, &bound);
            if (std::isfinite(vTop))
                vMax = std::max(vMax, vTop);
            const double ux1 = u.toUser(Quantity::rotation, lim * 1.06), uy1 = u.toUser(Quantity::speed, vMax * 1.08);
            const double xs = chart::niceStep(ux1, std::max(3, static_cast<int>(a.w / 64.0f))), ys = chart::niceStep(uy1, 5);
            const chart::Scale X{ 0.0, std::ceil(ux1 / xs) * xs, a.x, a.right() }, Y{ 0.0, std::ceil(uy1 / ys) * ys, a.bottom(), a.y };
            for (double v = 0.0; v <= Y.d1 + 1e-9; v += ys)
            {
                c.hairlineH(a.x, Y.px(v), a.w, v == 0.0 ? p.edge : p.grid);
                c.text(look::num(v).c_str(), a.x - 6.0f, Y.px(v) - 5.0f, look::kUnit, p.label, funkgui::Align::right);
            }
            for (double v = 0.0; v <= X.d1 + 1e-9; v += xs)
            {
                c.hairlineV(X.px(v), a.y, a.h, p.grid);
                c.text(look::num(v).c_str(), X.px(v), a.bottom() + 5.0f, look::kUnit, p.label, funkgui::Align::centre);
            }
            c.text(("ENGINE SPEED  " + upper(u.unit(Quantity::rotation).name)).c_str(), a.right(), a.bottom() + 20.0f, look::kCaps,
                   p.label, funkgui::Align::right);
            c.text(("ROAD SPEED  " + upper(u.unit(Quantity::speed).name)).c_str(), a.x - 44.0f, a.y - 14.0f, look::kCaps, p.label);
            // The limiter.
            const float lx = X.px(u.toUser(Quantity::rotation, lim));
            c.segment(lx, a.y, lx, a.bottom(), 1.5f, p.hotter);
            c.text(("LIMITER " + u.format(Quantity::rotation, lim, 0)).c_str(), lx - 4.0f, a.y + 2.0f, look::kCaps, p.hotter,
                   funkgui::Align::right);
            // Each gear: a line from rest to the limiter, its number and its speed there at the right.
            const DynoTrace& pred = ctx_.dyno.predicted();
            const double g0 = ctx_.live.get(kTel.gear);
            const int liveGear = std::isfinite(g0) ? static_cast<int>(std::lround(g0)) - 1 : -1;
            float lastY = 1e9f;
            for (int g = 0; g < n; ++g)
            {
                const double vl = roadSpeed(lim, g);
                const float y = Y.px(u.toUser(Quantity::speed, vl));
                const bool now = g == liveGear;
                c.segment(X.px(0.0), Y.px(0.0), lx, y, now ? 2.25f : 1.5f, now ? p.selected : p.text);
                // The speed at the limiter, kept apart from the one under it.
                const float ty = std::min(y - 6.0f, lastY - 13.0f);
                lastY = ty;
                const std::string t = look::num(g + 1) + "   " + u.format(Quantity::speed, vl, 0);
                c.text(t.c_str(), a.right() + 10.0f, ty, look::kUnit, now ? p.selected : p.text);
                c.segment(lx, y, a.right() + 6.0f, ty + 6.0f, 1.0f, p.faint);
                // The shift point: where the next gear pulls harder, from the predicted torque; else the limiter.
                if (g + 1 < n && !pred.empty())
                {
                    const double r0 = ratio(g), r1 = ratio(g + 1);
                    double shift = lim;
                    for (double w = lim * 0.5; w <= lim; w += lim / 200.0)
                    {
                        const double t0 = chart::at(pred.speed, pred.torque, w) * r0;
                        const double t1 = chart::at(pred.speed, pred.torque, w * r1 / r0) * r1;
                        if (std::isfinite(t0) && std::isfinite(t1) && t1 > t0)
                        {
                            shift = w;
                            break;
                        }
                    }
                    const float sx = X.px(u.toUser(Quantity::rotation, shift)), sy = Y.px(u.toUser(Quantity::speed, roadSpeed(shift, g)));
                    c.disc(sx, sy, 3.5f, p.panel, 1.5f, p.ok);
                    any = true;
                }
            }
            // Where the car is now.
            const double we = ctx_.live.get(kTel.engine), v = ctx_.live.get(kTel.speed);
            if (std::isfinite(we) && std::isfinite(v))
                c.disc(X.px(u.toUser(Quantity::rotation, we)), Y.px(u.toUser(Quantity::speed, v)), 5.0f, p.selected, 2.0f, p.ground);
            // The top speed: derived from the predicted power against drag and rolling.
            float ky = gearBox_.bottom() - 18.0f;
            if (std::isfinite(vTop))
            {
                const float y = Y.px(u.toUser(Quantity::speed, vTop));
                chart::stroke(c, { a.x, a.right() }, { y, y }, 1.5f, p.ok, { 7.0f, 4.0f });
                std::string t = "TOP SPEED " + u.format(Quantity::speed, vTop, 0) + " IN " + look::num(topGear + 1) +
                                (bound ? ", AT THE LIMITER" : "");
                if (c.textWidth(t.c_str(), look::kCaps) > lx - a.x - 12.0f)   // not past the limiter's line
                    t = "TOP SPEED " + u.format(Quantity::speed, vTop, 0) + (bound ? " (LIMITER)" : "");
                c.rrect(a.x + 2.0f, y - 18.0f, c.textWidth(t.c_str(), look::kCaps) + 8.0f, 15.0f, 3.0f, p.panel);
                c.text(t.c_str(), a.x + 6.0f, y - 15.0f, look::kCaps, p.ok);
                c.text(narrow ? "DERIVED: POWER MEETS DRAG" : "DERIVED: THE PREDICTED POWER AGAINST DRAG AND ROLLING", a.x, ky,
                       look::kCaps, p.label);
            }
            else
                c.text(narrow ? "TOP SPEED  -  NO CURVE YET" : "TOP SPEED  -   NO PREDICTED POWER YET", a.x, ky, look::kCaps, p.dim);
            if (any)
                {
                c.disc(a.right() + 14.0f, ky - 9.0f, 3.5f, p.panel, 1.5f, p.ok);
                c.text("SHIFT POINT", a.right() + 24.0f, ky - 14.0f, look::kCaps, p.ok);
            }
            if (!fromR)
                c.text(narrow ? "* TYPICAL RATIOS" : "* RATIOS NOT IN THIS BUILD: A TYPICAL SIX-SPEED", gearBox_.right() - look::kPad,
                       gearBox_.y + 9.0f, look::kUnit, p.dim, funkgui::Align::right);
        }

        Rect sideBox_{}, tyreBox_{}, lineBox_{}, gearBox_{};
    };

    // The highest speed at which the predicted power at the wheels (through the gearbox's efficiency) still meets the
    // drag and the rolling resistance, over every gear: derived, not measured.
    double CarWorkspace::topSpeed(int* gear, bool* limiterBound) const
    {
        const DynoTrace& pred = ctx_.dyno.predicted();
        if (pred.empty())
            return std::nan("");
        const double cdA = field(fCdA), mass = totalMass(), eff = field(fEfficiency), crr = tyre().k->crr;
        const double lim = limiter();
        const auto need = [&](double v) { return (0.5 * kRho * cdA * v * v + crr * mass * kG) * v; };
        double best = std::nan("");
        for (int g = 0; g < gears(); ++g)
        {
            double prevV = 0.0, prevExcess = std::nan("");
            for (std::size_t i = 0; i < pred.speed.size() && i < pred.power.size(); ++i)
            {
                const double w = pred.speed[i];
                if (w > lim)
                    break;
                const double v = roadSpeed(w, g), excess = pred.power[i] * eff - need(v);
                if (std::isfinite(prevExcess) && prevExcess >= 0.0 && excess < 0.0)
                {
                    const double t = prevExcess / (prevExcess - excess);
                    const double vc = prevV + (v - prevV) * t;
                    if (!(vc <= best))
                    {
                        best = vc;
                        if (gear != nullptr)
                            *gear = g;
                        if (limiterBound != nullptr)
                            *limiterBound = false;
                    }
                }
                prevV = v;
                prevExcess = excess;
            }
            // Still more power than it needs at the last point under the limiter: the limiter holds it.
            if (std::isfinite(prevExcess) && prevExcess >= 0.0)
            {
                const double vl = roadSpeed(std::min(lim, pred.speed.back()), g);
                if (!(vl <= best))
                {
                    best = vl;
                    if (gear != nullptr)
                        *gear = g;
                    if (limiterBound != nullptr)
                        *limiterBound = true;
                }
            }
        }
        return best;
    }

    std::unique_ptr<Workspace> makeCarWorkspace(WorkspaceContext& c) { return std::make_unique<CarWorkspace>(c); }
}

namespace tee::ui
{
    double carTopSpeed(Workspace& w, int* gear, bool* limiterBound)
    {
        auto* c = dynamic_cast<CarWorkspace*>(&w);
        return c != nullptr ? c->topSpeed(gear, limiterBound) : std::nan("");
    }
}
