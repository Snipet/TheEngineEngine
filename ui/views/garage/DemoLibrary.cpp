// ui/views/garage/DemoLibrary.cpp: see DemoLibrary.h. The builds are made up but plausible.
#include "views/garage/DemoLibrary.h"

#include "views/garage/Describe.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tee::ui::garage
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kRpm = kPi / 30.0;

        struct Made
        {
            const char* id;
            const char* name;
            Part part;
            bool user;
            const char* notes;
            std::vector<std::pair<const char*, double>> set;   // over the document's defaults
        };

        // The demo Spec's choices by index (ui/app/Demo.cpp): layout I4 I6 V6 V8 V10 V12 Flat4 Flat6; fuel Petrol E85
        // Methanol Diesel; boost Natural Turbo "Twin turbo" Supercharger.
        std::vector<Made> made()
        {
            return {
                { "shipped/street-v8", "Street V8", Part::whole, false,
                  "A 5.7 L pushrod V8 with one turbo at a modest boost, long-tube headers and an X-pipe. The garage's "
                  "starting point: a car of 1550 kg on street tyres.",
                  { { "engine.layout", 3 }, { "engine.cylinders", 8 }, { "boost.kind", 1 }, { "boost.level", 80e3 } } },
                { "shipped/hot-six", "Hot Six", Part::whole, false,
                  "A 3.0 L straight six on two turbos and E85: high compression for a turbo engine, a long rod.",
                  { { "engine.layout", 1 }, { "engine.cylinders", 6 }, { "engine.bore", 0.086 }, { "engine.stroke", 0.086 },
                    { "engine.compression", 10.5 }, { "boost.kind", 2 }, { "boost.level", 140e3 }, { "fuel.kind", 1 },
                    { "car.mass", 1480 }, { "car.drive", 2 } } },
                { "shipped/rally-four", "Rally Four", Part::whole, false,
                  "A 2.0 L flat four with one big turbo and anti-lag pops, in a light all-wheel-drive car.",
                  { { "engine.layout", 6 }, { "engine.cylinders", 4 }, { "engine.bore", 0.092 }, { "engine.stroke", 0.075 },
                    { "boost.kind", 1 }, { "boost.level", 120e3 }, { "ecu.pops", 1 }, { "car.mass", 1300 }, { "car.drive", 2 } } },
                { "shipped/big-block", "Big Block", Part::whole, false,
                  "A 7.3 L V8, naturally aspirated, on a heavy flywheel: torque everywhere, and it knows it.",
                  { { "engine.layout", 3 }, { "engine.cylinders", 8 }, { "engine.bore", 0.108 }, { "engine.stroke", 0.1 },
                    { "boost.kind", 0 }, { "boost.level", 0 }, { "engine.flywheel", 0.5 } } },
                { "shipped/grand-tourer", "Grand Tourer V12", Part::whole, false,
                  "A 6.0 L V12 revving to 7500, naturally aspirated, in a long heavy coupe.",
                  { { "engine.layout", 5 }, { "engine.cylinders", 12 }, { "engine.bore", 0.089 }, { "engine.stroke", 0.08 },
                    { "boost.kind", 0 }, { "boost.level", 0 }, { "engine.compression", 11.5 }, { "car.mass", 1850 } } },
                { "shipped/v6-engine", "Supercharged V6", Part::engine, false,
                  "An engine alone: a 3.5 L V6 with a positive-displacement blower. Put it in the car you have.",
                  { { "engine.layout", 2 }, { "engine.cylinders", 6 }, { "engine.bore", 0.0935 }, { "engine.stroke", 0.0848 },
                    { "boost.kind", 3 }, { "boost.level", 60e3 } } },
                { "shipped/light-coupe", "Light Coupe", Part::car, false,
                  "A car alone: 1150 kg, rear drive, a short final drive.",
                  { { "car.mass", 1150 }, { "car.wheelbase", 2.45 }, { "car.finalDrive", 4.1 } } },
                { "shipped/track-tune", "Track Tune", Part::tune, false,
                  "A tune alone: two degrees more spark, the limiter at 7200, pops on the overrun.",
                  { { "ecu.timing", 2 }, { "ecu.limiter", 7200 * kRpm }, { "ecu.pops", 1 } } },
                { "user/tow-diesel", "Tow Diesel", Part::whole, true,
                  "My truck: a 6.5 L straight six diesel, one turbo, low gearing.",
                  { { "engine.layout", 1 }, { "engine.cylinders", 6 }, { "engine.bore", 0.107 }, { "engine.stroke", 0.12 },
                    { "engine.compression", 17 }, { "fuel.kind", 3 }, { "boost.kind", 1 }, { "boost.level", 150e3 },
                    { "car.kind", 1 }, { "car.mass", 3200 }, { "car.finalDrive", 4.3 } } },
                { "user/my-v10", "My V10", Part::whole, true,
                  "Mine: a 5.2 L V10 with the race cam.",
                  { { "engine.layout", 4 }, { "engine.cylinders", 10 }, { "engine.bore", 0.0845 }, { "engine.stroke", 0.0928 },
                    { "boost.kind", 0 }, { "boost.level", 0 }, { "valve.intakeDuration", 278 }, { "engine.compression", 12.5 } } },
            };
        }
    }

    bool DemoLibrary::inPart(std::string_view section, Part part) noexcept
    {
        switch (part)
        {
            case Part::whole: return true;
            case Part::engine:
                return section == "engine" || section == "valve" || section == "intake" || section == "boost"
                    || section == "exhaust" || section == "fuel";
            case Part::car: return section == "car";
            case Part::tune: return section == "ecu";
        }
        return false;
    }

    DynoTrace DemoLibrary::curveFor(double displacement, int aspiration, bool diesel, double compression)
    {
        DynoTrace c;
        c.name = "predicted";
        const double litres = std::isfinite(displacement) ? displacement * 1000.0 : 3.0;
        const double boost = aspiration == 0 ? 1.0 : aspiration == 4 || aspiration == 3 ? 1.45 : aspiration == 2 ? 1.75 : 1.6;
        const double top = diesel ? 4200.0 : 7400.0 - 120.0 * litres;
        const double peakAt = diesel ? 1900.0 : top * (aspiration == 0 ? 0.68 : 0.5);
        const double perLitre = (diesel ? 160.0 : 95.0) * (1.0 + 0.03 * (compression - 10.0));
        for (double rpm = 1000.0; rpm <= top + 1e-9; rpm += 250.0)
        {
            const double x = (rpm - peakAt) / top;
            const double spool = aspiration == 0 || aspiration == 3 ? 1.0 : std::clamp((rpm - 1500.0) / 2200.0, 0.25, 1.0);
            const double tq = litres * perLitre * (1.0 - 1.6 * x * x) * (1.0 + (boost - 1.0) * spool);
            c.speed.push_back(rpm * kRpm);
            c.torque.push_back(tq);
            c.power.push_back(tq * rpm * kRpm);
        }
        return c;
    }

    DemoLibrary::DemoLibrary(FieldDoc& doc) : doc_(doc)
    {
        for (const Made& m : made())
        {
            BuildSummary b;
            b.id = m.id;
            b.name = m.name;
            b.notes = m.notes;
            b.part = m.part;
            b.user = m.user;
            for (int f = 0; f < doc.count(); ++f)
            {
                const FieldInfo& fi = doc.info(f);
                if (fi.hidden || fi.table() || !inPart(fi.section(), m.part))
                    continue;
                double v = fi.def;
                for (const auto& [path, value] : m.set)
                    if (fi.path == path)
                        v = value;
                b.values.push_back({ fi.path, { clampTo(fi, v) } });
            }
            makeCurve(b);
            files_.push_back(b.values);
            builds_.push_back(std::move(b));
        }
        std::string error;
        load(builds_.front().id, Part::whole, error);
    }

    void DemoLibrary::makeCurve(BuildSummary& b) const
    {
        b.curve = {};
        if (b.part == Part::car || b.part == Part::tune)
            return;
        const double asp = numberOf(b, "boost.kind");
        const std::string fuel = choiceOf(b, doc_, "fuel.kind");
        const double cr = numberOf(b, "engine.compression");
        b.curve = curveFor(displacement(b, doc_), std::isfinite(asp) ? static_cast<int>(asp) : 0, fuel == "Diesel",
                           std::isfinite(cr) ? cr : 10.0);
    }

    BuildSummary* DemoLibrary::find(const std::string& id)
    {
        for (BuildSummary& b : builds_)
            if (b.id == id)
                return &b;
        return nullptr;
    }

    int DemoLibrary::current() const
    {
        for (std::size_t i = 0; i < builds_.size(); ++i)
            if (builds_[i].current)
                return static_cast<int>(i);
        return -1;
    }

    void DemoLibrary::takeDocument(BuildSummary& b) const
    {
        for (FieldValue& v : b.values)
        {
            const int f = doc_.find(v.path);
            if (f >= 0)
                v.v = { doc_.get(f) };
        }
    }

    void DemoLibrary::refreshCurrent()
    {
        const int c = current();
        if (c < 0)
            return;
        // The current build's card shows the document as it stands.
        BuildSummary& b = builds_[static_cast<std::size_t>(c)];
        BuildSummary now = b;
        takeDocument(now);
        makeCurve(now);
        b.values = std::move(now.values);
        b.curve = std::move(now.curve);
    }

    void DemoLibrary::tick(float)
    {
        if (doc_.revision() != docSeen_)
        {
            docSeen_ = doc_.revision();
            refreshCurrent();
            ++revision_;
        }
    }

    bool DemoLibrary::load(const std::string& id, Part part, std::string& error)
    {
        BuildSummary* b = find(id);
        if (b == nullptr)
        {
            error = "no build of that name";
            return false;
        }
        // A part's file gives only its part; a whole build any of its parts.
        if (b->part != Part::whole && part != b->part)
            part = b->part;
        const int was = current();
        const auto& file = files_[static_cast<std::size_t>(b - builds_.data())];
        doc_.beginGesture();
        for (const FieldValue& v : file)
        {
            const int f = doc_.find(v.path);
            if (f >= 0 && inPart(doc_.info(f).section(), part) && !v.v.empty())
                doc_.set(f, v.v[0]);
        }
        doc_.endGesture();
        if (part == Part::whole)
        {
            if (was >= 0)
            {
                // The one it was goes back to its file's values.
                BuildSummary& w = builds_[static_cast<std::size_t>(was)];
                w.current = false;
                w.values = files_[static_cast<std::size_t>(was)];
                makeCurve(w);
            }
            b->current = true;
            savedRevision_ = doc_.revision();
            notesChanged_ = false;
        }
        docSeen_ = 0;
        tick(0.0f);
        ++revision_;
        return true;
    }

    bool DemoLibrary::save(std::string& error)
    {
        const int c = current();
        if (c < 0)
        {
            error = "no current build: save it as a name";
            return false;
        }
        BuildSummary& b = builds_[static_cast<std::size_t>(c)];
        if (!b.user)
        {
            error = "a shipped build is not saved over: save it as a name";
            return false;
        }
        takeDocument(b);
        files_[static_cast<std::size_t>(c)] = b.values;
        savedRevision_ = doc_.revision();
        notesChanged_ = false;
        ++revision_;
        return true;
    }

    bool DemoLibrary::saveAs(const std::string& name, std::string& error)
    {
        if (name.empty())
        {
            error = "a build needs a name";
            return false;
        }
        for (const BuildSummary& b : builds_)
            if (b.user && b.name == name)
            {
                error = "you have a build of that name";
                return false;
            }
        const int c = current();
        BuildSummary b;
        b.id = "user/" + name;
        b.name = name;
        b.part = Part::whole;
        b.user = true;
        b.notes = c >= 0 ? builds_[static_cast<std::size_t>(c)].notes : std::string();
        for (int f = 0; f < doc_.count(); ++f)
            if (!doc_.info(f).hidden && !doc_.info(f).table())
                b.values.push_back({ doc_.info(f).path, { doc_.get(f) } });
        if (c >= 0)
        {
            // The one it was saved from goes back to its own file's values.
            BuildSummary& w = builds_[static_cast<std::size_t>(c)];
            w.current = false;
            w.values = files_[static_cast<std::size_t>(c)];
            makeCurve(w);
        }
        b.current = true;
        makeCurve(b);
        files_.push_back(b.values);
        builds_.push_back(std::move(b));
        savedRevision_ = doc_.revision();
        notesChanged_ = false;
        ++revision_;
        return true;
    }

    bool DemoLibrary::duplicate(const std::string& id, std::string& newId, std::string& error)
    {
        const BuildSummary* src = find(id);
        if (src == nullptr)
        {
            error = "no build of that name";
            return false;
        }
        BuildSummary b = *src;
        std::vector<FieldValue> file = files_[static_cast<std::size_t>(src - builds_.data())];
        b.values = file;
        makeCurve(b);
        b.current = false;
        b.user = true;
        std::string name = b.name + " copy";
        for (int n = 2; std::any_of(builds_.begin(), builds_.end(), [&](const BuildSummary& o) { return o.name == name; }); ++n)
            name = b.name + " copy " + std::to_string(n);
        b.name = name;
        b.id = "user/" + name + "#" + std::to_string(++made_);
        newId = b.id;
        files_.push_back(std::move(file));
        builds_.push_back(std::move(b));
        ++revision_;
        return true;
    }

    bool DemoLibrary::remove(const std::string& id, std::string& error)
    {
        for (auto it = builds_.begin(); it != builds_.end(); ++it)
            if (it->id == id)
            {
                if (!it->user)
                {
                    error = "a shipped build is not deleted";
                    return false;
                }
                files_.erase(files_.begin() + (it - builds_.begin()));
                builds_.erase(it);
                ++revision_;
                return true;
            }
        error = "no build of that name";
        return false;
    }

    bool DemoLibrary::revert(std::string& error)
    {
        const int c = current();
        if (c < 0)
        {
            error = "no current build";
            return false;
        }
        doc_.beginGesture();
        for (const FieldValue& v : files_[static_cast<std::size_t>(c)])
        {
            const int f = doc_.find(v.path);
            if (f >= 0 && !v.v.empty())
                doc_.set(f, v.v[0]);
        }
        doc_.endGesture();
        savedRevision_ = doc_.revision();
        notesChanged_ = false;
        docSeen_ = 0;
        tick(0.0f);
        ++revision_;
        return true;
    }

    bool DemoLibrary::unsaved() const { return current() < 0 || doc_.revision() != savedRevision_ || notesChanged_; }

    std::string DemoLibrary::notes() const
    {
        const int c = current();
        return c < 0 ? std::string() : builds_[static_cast<std::size_t>(c)].notes;
    }

    void DemoLibrary::setNotes(const std::string& text)
    {
        const int c = current();
        if (c < 0 || builds_[static_cast<std::size_t>(c)].notes == text)
            return;
        builds_[static_cast<std::size_t>(c)].notes = text;
        notesChanged_ = true;
        ++revision_;
    }
}
