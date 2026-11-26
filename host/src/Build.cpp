// host/src/Build.cpp: tee::Build and tee::Presets (Build.h).
#include "tee/Build.h"

#include "tee/SimHost.h"
#include "tee/Telemetry.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace tee
{
    using nlohmann::ordered_json;

    namespace
    {
        // "engine.torqueShape" -> {"engine", "torqueShape"}; "boost.turbo[1].size" -> {"boost", "turbo[1]", "size"}
        std::vector<std::string> split(std::string_view path)
        {
            std::vector<std::string> parts;
            std::string part;
            for (const char c : path)
            {
                if (c == '.')
                {
                    parts.push_back(part);
                    part.clear();
                }
                else
                    part += c;
            }
            parts.push_back(part);
            return parts;
        }

        // "turbo[1][2]" -> name "turbo", indexes {1, 2}
        void indexes(const std::string& token, std::string& name, std::vector<std::size_t>& out)
        {
            const std::size_t b = token.find('[');
            name = token.substr(0, b);
            out.clear();
            std::size_t at = b;
            while (at != std::string::npos && at < token.size())
            {
                const std::size_t e = token.find(']', at);
                out.push_back(static_cast<std::size_t>(std::strtoul(token.c_str() + at + 1, nullptr, 10)));
                at = e == std::string::npos ? e : token.find('[', e);
            }
        }

        // The JSON node at a field's place, made on the way (writing).
        ordered_json& place(ordered_json& root, const std::vector<std::string>& tokens)
        {
            ordered_json* node = &root;
            for (const std::string& token : tokens)
            {
                std::string name;
                std::vector<std::size_t> idx;
                indexes(token, name, idx);
                node = &(*node)[name];
                for (const std::size_t i : idx)
                {
                    if (!node->is_array())
                        *node = ordered_json::array();
                    while (node->size() <= i)
                        node->push_back(ordered_json::object());
                    node = &(*node)[i];
                }
            }
            return *node;
        }

        // The JSON node at a field's place, or nullptr (reading).
        const ordered_json* find(const ordered_json& root, const std::vector<std::string>& tokens)
        {
            const ordered_json* node = &root;
            for (const std::string& token : tokens)
            {
                std::string name;
                std::vector<std::size_t> idx;
                indexes(token, name, idx);
                if (!node->is_object() || !node->contains(name))
                    return nullptr;
                node = &(*node)[name];
                for (const std::size_t i : idx)
                {
                    if (!node->is_array() || node->size() <= i)
                        return nullptr;
                    node = &(*node)[i];
                }
            }
            return node;
        }

        std::uint64_t fnv1a(std::string_view text)
        {
            std::uint64_t h = 1469598103934665603ull;
            for (const char c : text)
            {
                h ^= static_cast<unsigned char>(c);
                h *= 1099511628211ull;
            }
            return h;
        }

        std::size_t leafIndex(const FieldInfo& f, std::string_view name)
        {
            std::size_t at = 0;
            for (const FieldInfo::Leaf& leaf : f.leaves)
            {
                if (leaf.name == name)
                    return at;
                at += leaf.count;
            }
            return static_cast<std::size_t>(-1);
        }

        std::size_t totalCount(const FieldInfo& f)
        {
            std::size_t n = 0;
            for (const FieldInfo::Leaf& leaf : f.leaves)
                n += leaf.count;
            return n;
        }

        // Every leaf path of a JSON object, dotted (arrays of objects as name[i]); arrays of numbers are leaves.
        void leavesOf(const ordered_json& node, const std::string& prefix, std::vector<std::string>& out)
        {
            if (node.is_object())
            {
                for (auto it = node.begin(); it != node.end(); ++it)
                    leavesOf(it.value(), prefix.empty() ? it.key() : prefix + "." + it.key(), out);
                return;
            }
            if (node.is_array() && !node.empty() && node.front().is_object())
            {
                for (std::size_t i = 0; i < node.size(); ++i)
                    leavesOf(node[i], prefix + "[" + std::to_string(i) + "]", out);
                return;
            }
            out.push_back(prefix);
        }
    } // namespace

    // Whether a field is written to a build's file and read from it: every one but the hidden, which are the program's
    // or the host's working state (a section's rev, a settle's counter), except a hidden fingerprint (unit "hash":
    // spec.ecu.maps.knockLimitKey, what a baked table was made from), which belongs to the file it describes.
    bool inFile(const FieldInfo& f) noexcept
    {
        return !f.hidden || f.unit == "hash";
    }

    const char* partName(BuildPart part) noexcept
    {
        switch (part)
        {
            case BuildPart::Whole: return "whole";
            case BuildPart::Engine: return "engine";
            case BuildPart::Car: return "car";
            case BuildPart::Tune: return "tune";
        }
        return "whole";
    }

    bool partFromName(std::string_view name, BuildPart& part) noexcept
    {
        for (const BuildPart p : {BuildPart::Whole, BuildPart::Engine, BuildPart::Car, BuildPart::Tune})
            if (name == partName(p))
            {
                part = p;
                return true;
            }
        return false;
    }

    bool inPart(std::string_view section, BuildPart part) noexcept
    {
        switch (part)
        {
            case BuildPart::Whole: return true;
            case BuildPart::Engine:
                return section == "engine" || section == "valve" || section == "intake" || section == "boost"
                       || section == "exhaust" || section == "fuel";
            case BuildPart::Car: return section == "car";
            case BuildPart::Tune: return section == "ecu";
        }
        return false;
    }

    Build::Build(const Fields& fields) : fields_(&fields)
    {
        defaults();
    }

    std::vector<double> Build::defaultsOf(const FieldInfo& f) const
    {
        if (f.table())
            return std::vector<double>(totalCount(f), 0.0);
        return std::vector<double>(f.count, f.clamp(f.fallback()));
    }

    void Build::defaults()
    {
        values_.assign(fields_->all().size(), {});
        for (std::size_t i = 0; i < fields_->all().size(); ++i)
        {
            const FieldInfo& f = fields_->all()[i];
            if (f.block == "spec" && f.kind != FieldKind::Rev)
                values_[i] = defaultsOf(f);
        }
    }

    int Build::fieldOf(std::string_view path) const noexcept
    {
        int i = fields_->indexOf(path);
        if (i < 0 && path.substr(0, 5) != "spec.")
            i = fields_->indexOf("spec." + std::string(path));
        if (i < 0)
            return -1;
        const FieldInfo& f = fields_->all()[static_cast<std::size_t>(i)];
        return f.block == "spec" && f.kind != FieldKind::Rev ? i : -1;
    }

    double Build::get(std::string_view path, std::size_t index) const noexcept
    {
        const int i = fieldOf(path);
        if (i < 0)
            return 0.0;
        const std::vector<double>& v = values_[static_cast<std::size_t>(i)];
        return index < v.size() ? v[index] : 0.0;
    }

    const std::vector<double>& Build::values(int field) const
    {
        static const std::vector<double> none;
        return field >= 0 && static_cast<std::size_t>(field) < values_.size() ? values_[static_cast<std::size_t>(field)]
                                                                               : none;
    }

    void Build::clampValues(const FieldInfo& f, std::vector<double>& v) const
    {
        if (!f.table())
        {
            for (double& x : v)
                x = f.clamp(x);
            return;
        }
        std::size_t at = 0;
        for (const FieldInfo::Leaf& leaf : f.leaves)
        {
            for (std::size_t k = 0; k < leaf.count && at + k < v.size(); ++k)
            {
                double& x = v[at + k];
                if (!std::isfinite(x))
                    x = 0.0;
                if (leaf.scalar == Scalar::Int)
                    x = std::clamp(std::trunc(x), 0.0, 16.0);   // a table's counts
            }
            at += leaf.count;
        }
    }

    bool Build::set(std::string_view path, double value, std::size_t index)
    {
        const int i = fieldOf(path);
        if (i < 0)
            return false;
        std::vector<double> v = values_[static_cast<std::size_t>(i)];
        if (index >= v.size())
            return false;
        v[index] = value;
        change(i, std::move(v));
        return true;
    }

    bool Build::accepts(std::string_view path, double value, std::string* why) const
    {
        const int i = fieldOf(path);
        if (i < 0)
            return false;
        const FieldInfo& f = fields_->all()[static_cast<std::size_t>(i)];
        if (!underStatedLeast(f.path, value))
            return true;
        if (why != nullptr)
        {
            char line[160];
            std::snprintf(line, sizeof line, "under the least stated bore, %.0f mm (0: derived)",
                          1000.0 * statedLeast(f.path));
            *why = line;
        }
        return false;
    }

    bool Build::setChoice(std::string_view path, std::string_view choice)
    {
        const int i = fieldOf(path);
        if (i < 0)
            return false;
        const FieldInfo& f = fields_->all()[static_cast<std::size_t>(i)];
        const int c = f.choiceIndex(choice);
        if (c < 0)
            return false;
        return set(path, f.minimum.value_or(0.0) + c);
    }

    bool Build::setValues(int field, std::vector<double> v)
    {
        if (field < 0 || static_cast<std::size_t>(field) >= values_.size() || values_[static_cast<std::size_t>(field)].empty()
            || v.size() != values_[static_cast<std::size_t>(field)].size())
            return false;
        change(field, std::move(v));
        return true;
    }

    bool Build::setCurve(std::string_view path, const std::vector<double>& x, const std::vector<double>& y)
    {
        const int i = fieldOf(path);
        if (i < 0)
            return false;
        const FieldInfo& f = fields_->all()[static_cast<std::size_t>(i)];
        if (f.kind != FieldKind::Curve || x.size() != y.size() || x.size() > 16)
            return false;
        std::vector<double> v(totalCount(f), 0.0);
        const std::size_t ox = leafIndex(f, "x"), oy = leafIndex(f, "y"), on = leafIndex(f, "n");
        for (std::size_t k = 0; k < x.size(); ++k)
        {
            v[ox + k] = x[k];
            v[oy + k] = y[k];
        }
        v[on] = static_cast<double>(x.size());
        change(i, std::move(v));
        return true;
    }

    bool Build::setMap(std::string_view path, const std::vector<double>& x, const std::vector<double>& y,
                       const std::vector<std::vector<double>>& z)
    {
        const int i = fieldOf(path);
        if (i < 0)
            return false;
        const FieldInfo& f = fields_->all()[static_cast<std::size_t>(i)];
        if (f.kind != FieldKind::Map || x.size() > 16 || y.size() > 16 || z.size() != x.size())
            return false;
        std::vector<double> v(totalCount(f), 0.0);
        const std::size_t ox = leafIndex(f, "x"), oy = leafIndex(f, "y"), oz = leafIndex(f, "z");
        for (std::size_t k = 0; k < x.size(); ++k)
            v[ox + k] = x[k];
        for (std::size_t k = 0; k < y.size(); ++k)
            v[oy + k] = y[k];
        for (std::size_t a = 0; a < x.size(); ++a)
        {
            if (z[a].size() != y.size())
                return false;
            for (std::size_t b = 0; b < y.size(); ++b)
                v[oz + a * 16 + b] = z[a][b];
        }
        v[leafIndex(f, "nx")] = static_cast<double>(x.size());
        v[leafIndex(f, "ny")] = static_cast<double>(y.size());
        change(i, std::move(v));
        return true;
    }

    void Build::reset(BuildPart part)
    {
        beginGesture();
        for (std::size_t i = 0; i < fields_->all().size(); ++i)
        {
            const FieldInfo& f = fields_->all()[i];
            if (values_[i].empty() || f.section < 0)
                continue;
            if (inPart(fields_->sections()[static_cast<std::size_t>(f.section)].name, part))
                change(static_cast<int>(i), defaultsOf(f));
        }
        endGesture();
    }

    void Build::change(int field, std::vector<double> after)
    {
        const FieldInfo& f = fields_->all()[static_cast<std::size_t>(field)];
        clampValues(f, after);
        std::vector<double>& now = values_[static_cast<std::size_t>(field)];
        if (now == after)
            return;
        Change c{field, now, after};
        now = std::move(after);
        send(field);
        redo_.clear();
        redoIds_.clear();
        if (gestureDepth_ > 0)
        {
            for (Change& had : gesture_)
                if (had.field == field)
                {
                    had.after = c.after;
                    return;
                }
            gesture_.push_back(std::move(c));
        }
        else
        {
            undo_.push_back({std::move(c)});
            undoIds_.push_back(nextStep_++);
        }
    }

    void Build::beginGesture()
    {
        ++gestureDepth_;
    }

    void Build::endGesture()
    {
        if (gestureDepth_ == 0)
            return;
        if (--gestureDepth_ == 0 && !gesture_.empty())
        {
            undo_.push_back(std::move(gesture_));
            undoIds_.push_back(nextStep_++);
            gesture_.clear();
        }
    }

    bool Build::undo()
    {
        if (undo_.empty())
            return false;
        std::vector<Change> step = std::move(undo_.back());
        undo_.pop_back();
        redoIds_.push_back(undoIds_.back());
        undoIds_.pop_back();
        for (auto it = step.rbegin(); it != step.rend(); ++it)
        {
            values_[static_cast<std::size_t>(it->field)] = it->before;
            send(it->field);
        }
        redo_.push_back(std::move(step));
        return true;
    }

    bool Build::redo()
    {
        if (redo_.empty())
            return false;
        std::vector<Change> step = std::move(redo_.back());
        redo_.pop_back();
        undoIds_.push_back(redoIds_.back());
        redoIds_.pop_back();
        for (const Change& c : step)
        {
            values_[static_cast<std::size_t>(c.field)] = c.after;
            send(c.field);
        }
        undo_.push_back(std::move(step));
        return true;
    }

    // ---- the simulator ----------------------------------------------------------------------------------------------

    std::vector<unsigned char> Build::bytes(int field) const
    {
        const FieldInfo& f = fields_->all()[static_cast<std::size_t>(field)];
        std::vector<unsigned char> out(f.size, 0);
        const std::vector<double>& v = values_[static_cast<std::size_t>(field)];
        std::size_t at = 0;
        for (const FieldInfo::Leaf& leaf : f.leaves)
        {
            const std::size_t step = scalarBytes(leaf.scalar);
            for (std::size_t k = 0; k < leaf.count && at + k < v.size(); ++k)
                if (leaf.offset + (k + 1) * step <= out.size())
                    writeScalar(out.data() + leaf.offset + k * step, leaf.scalar, v[at + k]);
            at += leaf.count;
        }
        return out;
    }

    void Build::send(int field)
    {
        if (host_ == nullptr)
            return;
        const std::vector<unsigned char> b = bytes(field);
        if (!host_->setBytes(fields_->ref(field), b.data(), b.size()))
            waiting_.insert(field);
        else
            waiting_.erase(field);
    }

    void Build::attach(SimHost* host)
    {
        host_ = host;
        waiting_.clear();
        if (host_ == nullptr)
            return;
        for (std::size_t i = 0; i < values_.size(); ++i)
            if (!values_[i].empty())
                send(static_cast<int>(i));
    }

    bool Build::flush()
    {
        if (host_ == nullptr)
            return true;
        const std::set<int> again = waiting_;
        for (const int f : again)
            send(f);
        return waiting_.empty();
    }

    void Build::rebind(const Fields& fields)
    {
        const Fields* old = fields_;
        // The same fields in the same order (a quality change; the compile of the program the preview read): the
        // undo steps, which name fields by index, still hold.
        bool same = old->all().size() == fields.all().size();
        for (std::size_t i = 0; same && i < fields.all().size(); ++i)
        {
            const FieldInfo& a = old->all()[i];
            const FieldInfo& b = fields.all()[i];
            same = a.path == b.path && a.type == b.type && a.size == b.size && a.count == b.count;
        }
        std::vector<std::vector<double>> oldValues = std::move(values_);
        fields_ = &fields;
        defaults();
        for (std::size_t i = 0; i < old->all().size(); ++i)
        {
            if (oldValues[i].empty())
                continue;
            const FieldInfo& was = old->all()[i];
            const int now = fields_->indexOf(was.path);
            if (now < 0)
                continue;
            const FieldInfo& is = fields_->all()[static_cast<std::size_t>(now)];
            if (is.type == was.type && values_[static_cast<std::size_t>(now)].size() == oldValues[i].size())
            {
                values_[static_cast<std::size_t>(now)] = oldValues[i];
                clampValues(is, values_[static_cast<std::size_t>(now)]);
            }
        }
        if (!same)
        {
            undo_.clear();
            redo_.clear();
            undoIds_.clear();
            redoIds_.clear();
            gesture_.clear();
        }
        attach(host_);
    }

    // ---- files ------------------------------------------------------------------------------------------------------

    std::string Build::save(BuildPart part) const
    {
        ordered_json root;
        root["format"] = "tee-build";
        root["version"] = 1;
        if (part != BuildPart::Whole)
            root["part"] = partName(part);
        root["name"] = name;
        root["notes"] = notes;
        ordered_json spec = ordered_json::object();
        for (const Section& s : fields_->sections())
            if (inPart(s.name, part))
                spec[s.name] = ordered_json::object();
        for (std::size_t i = 0; i < values_.size(); ++i)
        {
            const FieldInfo& f = fields_->all()[i];
            if (values_[i].empty() || !inFile(f) || f.section < 0)
                continue;
            if (!inPart(fields_->sections()[static_cast<std::size_t>(f.section)].name, part))
                continue;
            std::vector<std::string> tokens = split(f.path);
            tokens.erase(tokens.begin());   // "spec"
            ordered_json& at = place(spec, tokens);
            const std::vector<double>& v = values_[i];
            switch (f.kind)
            {
                case FieldKind::Switch: at = v[0] != 0.0; break;
                case FieldKind::Choice:
                {
                    const long c = std::lround(v[0] - f.minimum.value_or(0.0));
                    if (c >= 0 && static_cast<std::size_t>(c) < f.choices.size())
                        at = f.choices[static_cast<std::size_t>(c)];
                    else
                        at = v[0];
                    break;
                }
                case FieldKind::Integer: at = static_cast<long long>(std::llround(v[0])); break;
                case FieldKind::Array:
                {
                    ordered_json a = ordered_json::array();
                    for (const double x : v)
                        if (f.scalar == Scalar::Bool)
                            a.push_back(x != 0.0);
                        else if (f.scalar == Scalar::Int)
                            a.push_back(static_cast<long long>(std::llround(x)));
                        else
                            a.push_back(x);
                    at = a;
                    break;
                }
                case FieldKind::Curve:
                {
                    const std::size_t n = static_cast<std::size_t>(v[leafIndex(f, "n")]);
                    ordered_json x = ordered_json::array(), y = ordered_json::array();
                    for (std::size_t k = 0; k < n && k < 16; ++k)
                    {
                        x.push_back(v[leafIndex(f, "x") + k]);
                        y.push_back(v[leafIndex(f, "y") + k]);
                    }
                    at = ordered_json{{"x", x}, {"y", y}};
                    break;
                }
                case FieldKind::Map:
                {
                    const std::size_t nx = static_cast<std::size_t>(v[leafIndex(f, "nx")]);
                    const std::size_t ny = static_cast<std::size_t>(v[leafIndex(f, "ny")]);
                    ordered_json x = ordered_json::array(), y = ordered_json::array(), z = ordered_json::array();
                    for (std::size_t k = 0; k < nx && k < 16; ++k)
                        x.push_back(v[leafIndex(f, "x") + k]);
                    for (std::size_t k = 0; k < ny && k < 16; ++k)
                        y.push_back(v[leafIndex(f, "y") + k]);
                    for (std::size_t a = 0; a < nx && a < 16; ++a)
                    {
                        ordered_json row = ordered_json::array();
                        for (std::size_t b = 0; b < ny && b < 16; ++b)
                            row.push_back(v[leafIndex(f, "z") + a * 16 + b]);
                        z.push_back(row);
                    }
                    at = ordered_json{{"x", x}, {"y", y}, {"z", z}};
                    break;
                }
                case FieldKind::Number:
                case FieldKind::Rev: at = v[0]; break;
            }
        }
        // what this program does not know, where the file had it
        for (const auto& [path, text] : unknown_)
        {
            if (!inPart(split(path).front(), part))
                continue;
            ordered_json value = ordered_json::parse(text, nullptr, false);
            if (!value.is_discarded())
                place(spec, split(path)) = value;
        }
        root["spec"] = spec;
        return root.dump(2) + "\n";
    }

    bool Build::load(std::string_view json, BuildPart part, std::vector<std::string>& warnings, std::string& error)
    {
        const ordered_json root = ordered_json::parse(json, nullptr, false);
        if (root.is_discarded() || !root.is_object())
        {
            error = "not JSON, or not an object";
            return false;
        }
        if (root.value("format", std::string()) != "tee-build")
        {
            error = "its \"format\" is not \"tee-build\"";
            return false;
        }
        if (root.value("version", 0) > 1)
            warnings.push_back("a build of version " + std::to_string(root.value("version", 0))
                               + ", newer than this application's (1): read as far as it goes");
        BuildPart filePart = BuildPart::Whole;
        if (root.contains("part") && !partFromName(root.value("part", std::string()), filePart))
        {
            error = "its \"part\" is none of whole, engine, car, tune";
            return false;
        }
        if (part != BuildPart::Whole && filePart != BuildPart::Whole && filePart != part)
        {
            error = std::string("the file is the ") + partName(filePart) + "'s, not the " + partName(part) + "'s";
            return false;
        }
        if (part == BuildPart::Whole)
            part = filePart;
        const ordered_json* spec = root.contains("spec") ? &root["spec"] : nullptr;
        if (spec != nullptr && !spec->is_object())
        {
            error = "its \"spec\" is not an object";
            return false;
        }
        const ordered_json empty = ordered_json::object();
        if (spec == nullptr)
            spec = &empty;

        beginGesture();
        if (part == BuildPart::Whole)
        {
            name = root.value("name", std::string());
            notes = root.value("notes", std::string());
        }
        std::vector<std::string> outOfRange;
        for (std::size_t i = 0; i < values_.size(); ++i)
        {
            const FieldInfo& f = fields_->all()[i];
            if (values_[i].empty() || f.section < 0)
                continue;
            if (!inPart(fields_->sections()[static_cast<std::size_t>(f.section)].name, part))
                continue;
            std::vector<std::string> tokens = split(f.path);
            tokens.erase(tokens.begin());
            const ordered_json* at = find(*spec, tokens);
            std::vector<double> v = defaultsOf(f);
            if (at != nullptr && inFile(f))
            {
                const auto number = [&](const ordered_json& j, double& out) {
                    if (j.is_number())
                        out = j.get<double>();
                    else if (j.is_boolean())
                        out = j.get<bool>() ? 1.0 : 0.0;
                    else if (j.is_string() && !f.choices.empty() && f.choiceIndex(j.get<std::string>()) >= 0)
                        out = f.minimum.value_or(0.0) + f.choiceIndex(j.get<std::string>());
                    else
                        return false;
                    return true;
                };
                bool ok = true;
                if (f.kind == FieldKind::Curve || f.kind == FieldKind::Map)
                {
                    const auto axis = [&](const char* key, std::vector<double>& out) {
                        out.clear();
                        if (!at->is_object() || !at->contains(key) || !(*at)[key].is_array())
                            return false;
                        for (const ordered_json& x : (*at)[key])
                            if (x.is_number() && out.size() < 16)
                                out.push_back(x.get<double>());
                        return true;
                    };
                    std::vector<double> x, y;
                    ok = axis("x", x) && axis("y", y);
                    if (ok && f.kind == FieldKind::Curve && x.size() == y.size())
                    {
                        for (std::size_t k = 0; k < x.size(); ++k)
                        {
                            v[leafIndex(f, "x") + k] = x[k];
                            v[leafIndex(f, "y") + k] = y[k];
                        }
                        v[leafIndex(f, "n")] = static_cast<double>(x.size());
                    }
                    else if (ok && f.kind == FieldKind::Map && (*at).contains("z") && (*at)["z"].is_array())
                    {
                        for (std::size_t k = 0; k < x.size(); ++k)
                            v[leafIndex(f, "x") + k] = x[k];
                        for (std::size_t k = 0; k < y.size(); ++k)
                            v[leafIndex(f, "y") + k] = y[k];
                        const ordered_json& z = (*at)["z"];
                        for (std::size_t a = 0; a < x.size() && a < z.size(); ++a)
                            for (std::size_t b = 0; b < y.size() && z[a].is_array() && b < z[a].size(); ++b)
                                if (z[a][b].is_number())
                                    v[leafIndex(f, "z") + a * 16 + b] = z[a][b].get<double>();
                        v[leafIndex(f, "nx")] = static_cast<double>(x.size());
                        v[leafIndex(f, "ny")] = static_cast<double>(y.size());
                    }
                    else
                        ok = false;
                }
                else if (f.kind == FieldKind::Array)
                {
                    ok = at->is_array();
                    for (std::size_t k = 0; ok && k < at->size() && k < v.size(); ++k)
                        ok = number((*at)[k], v[k]);
                }
                else
                    ok = number(*at, v[0]);
                if (!ok)
                {
                    warnings.push_back(f.path + ": not a value of its kind (" + kindName(f.kind) + "): its default");
                    v = defaultsOf(f);
                }
                else if (!f.table())
                {
                    for (const double x : v)
                        if (f.clamp(x) != x)
                        {
                            outOfRange.push_back(f.path);
                            break;
                        }
                    for (const double x : v)
                        if (underStatedLeast(f.path, x))
                        {
                            char line[200];
                            std::snprintf(line, sizeof line,
                                          ": %.4g m is under the least stated bore, %.3g m: refused, derived (0)", x,
                                          statedLeast(f.path));
                            warnings.push_back(f.path + line);
                            v = defaultsOf(f);
                            break;
                        }
                }
            }
            change(static_cast<int>(i), std::move(v));
        }
        endGesture();
        if (!outOfRange.empty())
        {
            std::string list;
            for (const std::string& p : outOfRange)
                list += (list.empty() ? "" : ", ") + p;
            warnings.push_back("outside their ranges, clamped: " + list);
        }

        // what the program does not know: kept, and written back
        std::vector<std::string> leaves;
        leavesOf(*spec, "", leaves);
        std::vector<std::string> kept;
        for (auto it = unknown_.begin(); it != unknown_.end();)
            it = inPart(split(it->first).front(), part) ? unknown_.erase(it) : std::next(it);
        for (const std::string& leaf : leaves)
        {
            const std::string path = "spec." + leaf;
            if (fields_->indexOf(path) >= 0)
                continue;
            // a table's own fields ("engine.torqueShape.x") are its field's
            bool inside = false;
            for (const FieldInfo& f : fields_->all())
                if (f.table() && path.size() > f.path.size() && path.compare(0, f.path.size(), f.path) == 0
                    && path[f.path.size()] == '.')
                    inside = true;
            if (inside || !inPart(split(leaf).front(), part))
                continue;
            const ordered_json* at = find(*spec, split(leaf));
            if (at == nullptr)
                continue;
            unknown_[leaf] = at->dump();
            kept.push_back(path);
        }
        if (!kept.empty())
        {
            std::string list;
            for (const std::string& p : kept)
                list += (list.empty() ? "" : ", ") + p;
            warnings.push_back(std::to_string(kept.size()) + " field" + (kept.size() == 1 ? "" : "s")
                               + " this program does not know, kept and written back: " + list);
        }
        return true;
    }

    bool Build::loadFile(const std::filesystem::path& file, BuildPart part, std::vector<std::string>& warnings,
                         std::string& error)
    {
        std::ifstream in(file, std::ios::binary);
        if (!in)
        {
            error = file.generic_string() + ": cannot be read";
            return false;
        }
        std::ostringstream text;
        text << in.rdbuf();
        if (!load(text.str(), part, warnings, error))
        {
            error = file.generic_string() + ": " + error;
            return false;
        }
        if (name.empty() && part == BuildPart::Whole)
            name = file.stem().generic_string();
        return true;
    }

    bool Build::saveFile(const std::filesystem::path& file, BuildPart part, std::string& error) const
    {
        std::error_code ec;
        if (file.has_parent_path())
            std::filesystem::create_directories(file.parent_path(), ec);
        const std::filesystem::path temporary = file.string() + ".writing";
        {
            std::ofstream out(temporary, std::ios::binary);
            out << save(part);
            if (!out)
            {
                error = file.generic_string() + ": cannot be written";
                return false;
            }
        }
        std::filesystem::rename(temporary, file, ec);
        if (ec)
        {
            error = file.generic_string() + ": " + ec.message();
            return false;
        }
        return true;
    }

    std::vector<Build::Difference> Build::diff(const Build& a, const Build& b)
    {
        std::vector<Difference> out;
        const std::size_t n = std::min(a.values_.size(), b.values_.size());
        for (std::size_t i = 0; i < n; ++i)
            if (a.values_[i] != b.values_[i])
                out.push_back({a.fields_->all()[i].path, a.values_[i], b.values_[i]});
        return out;
    }

    std::uint64_t Build::hash() const
    {
        std::string text;
        char buffer[40];
        for (std::size_t i = 0; i < values_.size(); ++i)
        {
            if (values_[i].empty())
                continue;
            text += fields_->all()[i].path;
            for (const double x : values_[i])
            {
                std::snprintf(buffer, sizeof buffer, " %.17g", x);
                text += buffer;
            }
            text += '\n';
        }
        return fnv1a(text);
    }

    // ---- presets ----------------------------------------------------------------------------------------------------

    std::vector<PresetEntry> Presets::list(const std::filesystem::path& shipped, const std::filesystem::path& user)
    {
        std::vector<PresetEntry> out;
        for (const bool isUser : {false, true})
        {
            const std::filesystem::path& dir = isUser ? user : shipped;
            std::vector<PresetEntry> here;
            std::error_code ec;
            if (dir.empty() || !std::filesystem::is_directory(dir, ec))
                continue;
            for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
                if (entry.path().extension() == ".json")
                    here.push_back({entry.path().stem().generic_string(), entry.path(), isUser});
            std::sort(here.begin(), here.end(), [](const PresetEntry& x, const PresetEntry& y) { return x.name < y.name; });
            out.insert(out.end(), here.begin(), here.end());
        }
        return out;
    }

    std::filesystem::path Presets::userDirectory()
    {
        if (const char* d = std::getenv("TEE_USER_DIR"); d != nullptr && *d != '\0')
            return d;
#if defined(_WIN32)
        if (const char* a = std::getenv("APPDATA"))
            return std::filesystem::path(a) / "TheEngineEngine" / "builds";
#elif defined(__APPLE__)
        if (const char* h = std::getenv("HOME"))
            return std::filesystem::path(h) / "Library" / "Application Support" / "TheEngineEngine" / "builds";
#else
        if (const char* x = std::getenv("XDG_DATA_HOME"); x != nullptr && *x != '\0')
            return std::filesystem::path(x) / "TheEngineEngine" / "builds";
        if (const char* h = std::getenv("HOME"))
            return std::filesystem::path(h) / ".local" / "share" / "TheEngineEngine" / "builds";
#endif
        return {};
    }
} // namespace tee
