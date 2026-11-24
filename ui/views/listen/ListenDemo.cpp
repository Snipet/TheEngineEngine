// ui/views/listen/ListenDemo.cpp: LISTEN's own demo data (U1's hook, TEE_UI_DEMO in register.h): package L's fields
// (sim/spec/sound.fm) that U1's made-up Spec lacks, the Free camera's (coming from L), and the listener's telemetry
// made up from the script: the sources' places for a front engine (docs/models/listen.md) and their levels at a
// listener at the tailpipe camera, the mix, the limiter working near full load.
#include "app/Demo.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace tee::ui
{
    namespace
    {
        void addField(std::vector<FieldInfo>& out, FieldInfo f)
        {
            if (std::none_of(out.begin(), out.end(), [&](const FieldInfo& g) { return g.path == f.path; }))
                out.push_back(std::move(f));
        }
        FieldInfo choiceField(std::string path, std::string label, std::vector<std::string> names, int def)
        {
            FieldInfo f;
            f.path = std::move(path);
            f.kind = FieldKind::choice;
            f.min = 0;
            f.max = static_cast<double>(names.size() - 1);
            f.def = def;
            f.label = std::move(label);
            f.choices = std::move(names);
            return f;
        }
        FieldInfo numberField(std::string path, std::string label, double lo, double hi, double def, Quantity q)
        {
            FieldInfo f;
            f.path = std::move(path);
            f.min = lo;
            f.max = hi;
            f.def = def;
            f.label = std::move(label);
            f.quantity = q;
            return f;
        }
    }

    void listenDemo(DemoExtras& x)
    {
        if (x.fields != nullptr)
        {
            std::vector<FieldInfo>& f = *x.fields;
            addField(f, choiceField("sound.model", "Listener", { "Reference", "Places" }, 1));
            addField(f, choiceField("sound.camera", "Camera",
                                    { "Tailpipe", "Rear", "Side", "Engine bay", "Driver", "Trackside", "Chase", "Free" }, 0));
            addField(f, choiceField("sound.place", "Place", { "Hall", "Field", "Street", "Tunnel", "Garage" }, 2));
            addField(f, choiceField("sound.enginePlace", "Engine place", { "Front", "Mid", "Rear" }, 0));
            addField(f, numberField("sound.windows", "Windows", 0, 1, 0, Quantity::fraction));
            addField(f, choiceField("sound.ground", "Ground", { "Asphalt", "Grass", "None" }, 0));
            addField(f, numberField("sound.listenerX", "Listener across", -60, 60, 3, Quantity::length));
            addField(f, numberField("sound.listenerY", "Listener along", -60, 60, 2.2, Quantity::length));
            addField(f, numberField("sound.listenerZ", "Listener height", 0.1, 10, 1.5, Quantity::length));
            {
                FieldInfo fc = numberField("sound.facing", "Listener facing", -180, 180, -90, Quantity::none);
                fc.unit = "deg";
                addField(f, fc);
            }
            addField(f, numberField("car.track", "Track", 0.8, 2.6, 1.55, Quantity::length));
            return;
        }
        if (x.engine == nullptr)
            return;
        DemoEngine& e = *x.engine;
        // The sources at a front engine's places; their levels at a listener 2.9 m behind the tailpipes.
        static const double px[10] = { -0.55, 0.4, 0.35, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.3 };
        static const double py[10] = { 0.0, 0.0, 3.9, 3.4, 3.4, 2.2, 2.4, 2.4, 0.9, 0.8 };
        static const double base[10] = { -8, -9, -26, -24, -32, -40, -48, -36, -38, -200 };   // the tank silent (L's -200)
        for (int i = 0; i < 10; ++i)
        {
            const std::string k = "[" + std::to_string(i) + "]";
            e.addTelemetry("sound.srcX" + k, [i](const DemoEngine&) { return px[i]; });
            e.addTelemetry("sound.srcY" + k, [i](const DemoEngine&) { return py[i]; });
            e.addTelemetry("sound.srcLevel" + k, [i](const DemoEngine& d) {
                return base[i] + (i < 5 ? 12.0 * d.throttle() : 0.0) + (i == 5 || i == 6 ? d.rpm() / 1000.0 : 0.0);
            });
        }
        e.addTelemetry("sound.loudness", [](const DemoEngine&) { return 3.5; });
        e.addTelemetry("sound.mixLevel", [](const DemoEngine& d) { return -24.0 + 14.0 * d.throttle(); });
        e.addTelemetry("sound.limiter", [](const DemoEngine& d) { return d.throttle() > 0.9 ? -2.5 : 0.0; });
        e.addTelemetry("sound.ceilingParts", [](const DemoEngine& d) { return d.throttle() > 0.9 ? 12.0 : 0.0; });
        e.addTelemetry("sound.doppler", [](const DemoEngine&) { return 1.0; });
        e.addTelemetry("sound.range", [](const DemoEngine&) { return 5.1; });
        // the cabin's ways in, at the ears: only in the driver's seat (camera 4), -200 otherwise (as L's)
        const auto way = [](double shut, double load) {
            return [shut, load](const DemoEngine& d) {
                return std::lround(d.field("sound.camera", 0.0)) == 4 ? shut + load * d.throttle() : -200.0;
            };
        };
        e.addTelemetry("sound.pathFloor", way(-30.0, 10.0));
        e.addTelemetry("sound.pathFirewall", way(-34.0, 8.0));
        e.addTelemetry("sound.pathGlass", way(-46.0, 0.0));
        e.addTelemetry("sound.pathStructure", way(-38.0, 6.0));
    }
}
