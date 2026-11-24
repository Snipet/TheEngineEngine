// host/src/Fields.cpp: tee::Fields (Fields.h).
#include "tee/Fields.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <map>
#include <sstream>

namespace tee
{
    namespace
    {
        using Attribute = Schema::Attribute;

        std::atomic<std::uint32_t> gGeneration{0};

        struct Inherited
        {
            bool hidden = false;
        };

        const Attribute* attr(const Schema::Node& node, std::string_view name)
        {
            return node.attribute(name);
        }

        std::string text(const Attribute* a, std::size_t i = 0)
        {
            if (a == nullptr || i >= a->arguments.size())
                return {};
            return a->arguments[i].text;
        }

        std::optional<double> number(const Attribute* a, std::size_t i = 0)
        {
            if (a == nullptr || i >= a->arguments.size() || a->arguments[i].isString)
                return std::nullopt;
            return a->arguments[i].number;
        }

        std::string elementPath(const std::string& path, std::size_t i, const std::vector<std::size_t>& extents)
        {
            std::string out = path;
            std::size_t span = 1;
            for (const std::size_t e : extents)
                span *= e;
            std::size_t left = i;
            for (const std::size_t e : extents)
            {
                span /= e;
                out += "[" + std::to_string(left / span) + "]";
                left %= span;
            }
            return out;
        }

        bool isTableModule(const std::string& module)
        {
            return module == "Curve16" || module == "Map16";
        }

        class Walker
        {
        public:
            Walker(std::vector<FieldInfo>& out, const std::string& block) : out_(out), block_(block) {}

            void nodes(const std::vector<Schema::Node>& list, const std::string& prefix, std::size_t base,
                       Inherited inherited)
            {
                std::string group;   // ui::group holds until the next one, within one module
                for (const Schema::Node& node : list)
                {
                    if (const Attribute* g = attr(node, "ui::group"))
                        group = text(g);
                    Inherited mine = inherited;
                    mine.hidden = mine.hidden || attr(node, "ui::hidden") != nullptr;
                    const std::string path = prefix + "." + node.name;
                    const std::size_t offset = base + node.offset;

                    if (!node.children.empty() && !isTableModule(node.module))
                    {
                        const std::size_t n = node.count();
                        if (node.extents.empty())
                            nodes(node.children, path, offset, mine);
                        else
                            for (std::size_t i = 0; i < n; ++i)
                                nodes(node.children, elementPath(path, i, node.extents), offset + i * node.stride, mine);
                        continue;
                    }
                    if (!node.children.empty())
                    {
                        // a table: one field each element (an array of curves is unrolled)
                        const std::size_t n = node.count();
                        for (std::size_t i = 0; i < n; ++i)
                        {
                            FieldInfo info = describe(node, node.extents.empty() ? path : elementPath(path, i, node.extents),
                                                      offset + i * node.stride, group, mine);
                            info.kind = node.module == "Curve16" ? FieldKind::Curve : FieldKind::Map;
                            info.scalar = Scalar::None;
                            info.size = node.stride;
                            info.count = 1;
                            info.extents.clear();
                            info.type = node.module;
                            for (const Schema::Node& child : node.children)
                            {
                                if (child.scalar == Scalar::None || child.scalar == Scalar::Pointer)
                                    continue;
                                FieldInfo::Leaf leaf;
                                leaf.name = child.name;
                                leaf.offset = child.offset;
                                leaf.scalar = child.scalar;
                                leaf.count = child.count();
                                leaf.stride = child.stride;
                                leaf.extents = child.extents;
                                info.leaves.push_back(std::move(leaf));
                            }
                            out_.push_back(std::move(info));
                        }
                        continue;
                    }
                    if (node.scalar == Scalar::None || node.scalar == Scalar::Pointer || node.parameter)
                        continue;
                    FieldInfo info = describe(node, path, offset, group, mine);
                    FieldInfo::Leaf leaf;
                    leaf.scalar = node.scalar;
                    leaf.count = node.count();
                    leaf.stride = node.stride;
                    leaf.extents = node.extents;
                    info.leaves.push_back(std::move(leaf));
                    out_.push_back(std::move(info));
                }
            }

        private:
            FieldInfo describe(const Schema::Node& node, const std::string& path, std::size_t offset,
                               const std::string& group, Inherited inherited)
            {
                FieldInfo info;
                info.path = path;
                info.block = block_;
                info.type = node.type;
                info.scalar = node.scalar;
                info.offset = offset;
                info.size = node.size;
                info.count = node.count();
                info.extents = node.extents;
                info.group = group;
                info.hidden = inherited.hidden;
                const Attribute* range = attr(node, "fminus::range");
                info.minimum = number(range, 0);
                info.maximum = number(range, 1);
                info.defaultValue = number(attr(node, "fminus::default"));
                info.label = text(attr(node, "ui::label"));
                info.quantity = text(attr(node, "ui::quantity"));
                info.unit = text(attr(node, "ui::unit"));
                info.taper = text(attr(node, "ui::taper"));
                info.help = text(attr(node, "ui::help"));
                if (const Attribute* d = attr(node, "ui::decimals"); d != nullptr && !d->arguments.empty())
                    info.decimals = static_cast<int>(d->arguments[0].number);
                if (const Attribute* c = attr(node, "ui::choices"))
                    for (const Attribute::Argument& a : c->arguments)
                        info.choices.push_back(a.text);
                if (const Attribute* a = attr(node, "ui::axes"))
                {
                    info.axisX = text(a, 0);
                    info.axisY = text(a, 1);
                }
                if (!node.extents.empty())
                    info.kind = FieldKind::Array;
                else if (node.scalar == Scalar::Bool)
                    info.kind = FieldKind::Switch;
                else if (node.scalar == Scalar::Int)
                    info.kind = info.choices.empty() ? FieldKind::Integer : FieldKind::Choice;
                else
                    info.kind = FieldKind::Number;
                return info;
            }

            std::vector<FieldInfo>& out_;
            std::string block_;
        };

        std::string number(double v)
        {
            char buffer[48];
            std::snprintf(buffer, sizeof buffer, "%.9g", v);
            return buffer;
        }
    } // namespace

    const char* kindName(FieldKind kind) noexcept
    {
        switch (kind)
        {
            case FieldKind::Number: return "number";
            case FieldKind::Integer: return "integer";
            case FieldKind::Switch: return "switch";
            case FieldKind::Choice: return "choice";
            case FieldKind::Array: return "array";
            case FieldKind::Curve: return "curve";
            case FieldKind::Map: return "map";
            case FieldKind::Rev: return "rev";
        }
        return "?";
    }

    const std::vector<std::string>& quantities()
    {
        static const std::vector<std::string> list = {
            "pressure", "temperature", "length", "area",  "volume", "mass",      "inertia",  "speed",   "rotation",
            "torque",   "power",       "angle",  "time",  "frequency", "flow",   "ratio",    "fraction",
        };
        return list;
    }

    bool isQuantity(std::string_view name) noexcept
    {
        for (const std::string& q : quantities())
            if (q == name)
                return true;
        return false;
    }

    double FieldInfo::clamp(double value) const noexcept
    {
        if (!std::isfinite(value))
            value = fallback();
        if (hasRange())
            value = std::clamp(value, std::min(*minimum, *maximum), std::max(*minimum, *maximum));
        if (scalar == Scalar::Int || scalar == Scalar::Char)
            value = std::trunc(value);
        if (scalar == Scalar::Bool)
            value = value != 0.0 ? 1.0 : 0.0;
        return value;
    }

    int FieldInfo::choiceIndex(std::string_view name) const noexcept
    {
        for (std::size_t i = 0; i < choices.size(); ++i)
            if (choices[i] == name)
                return static_cast<int>(i);
        return -1;
    }

    std::unique_ptr<Fields> Fields::of(const Schema& schema, std::string& error)
    {
        std::unique_ptr<Fields> f(new Fields());
        f->schema_ = &schema;
        f->generation_ = ++gGeneration;
        const char* const blocks[] = {"spec", "ctl", "tel", "scope"};
        Block* places[] = {&f->spec_, &f->ctl_, &f->tel_, &f->scope_};
        for (int b = 0; b < 4; ++b)
        {
            const Schema::Node* node = nullptr;
            for (const Schema::Node& candidate : schema.fields())
                if (candidate.name == blocks[b])
                    node = &candidate;
            if (node == nullptr || node->children.empty() || !node->extents.empty())
            {
                error = "module " + schema.module() + " has no block '" + blocks[b]
                        + "' (a field of a module, held by value: 02-contracts.md, section 1)";
                return nullptr;
            }
            places[b]->offset = node->offset;
            places[b]->size = node->size;
            const std::size_t first = f->fields_.size();
            Walker walker(f->fields_, blocks[b]);
            walker.nodes(node->children, blocks[b], node->offset, Inherited{});

            if (b == 0)
            {
                // the sections: spec's own fields that start with `int rev;`
                for (const Schema::Node& child : node->children)
                {
                    if (child.children.empty() || !child.extents.empty() || isTableModule(child.module))
                        continue;
                    const Schema::Node& head = child.children.front();
                    if (head.name != "rev" || head.scalar != Scalar::Int || !head.extents.empty())
                        continue;
                    Section s;
                    s.name = child.name;
                    s.path = std::string("spec.") + child.name;
                    s.offset = node->offset + child.offset;
                    s.size = child.size;
                    s.revOffset = s.offset + head.offset;
                    f->sections_.push_back(std::move(s));
                }
                for (std::size_t i = first; i < f->fields_.size(); ++i)
                {
                    FieldInfo& info = f->fields_[i];
                    for (std::size_t s = 0; s < f->sections_.size(); ++s)
                    {
                        const Section& sec = f->sections_[s];
                        if (info.offset >= sec.offset && info.offset < sec.offset + sec.size)
                            info.section = static_cast<int>(s);
                    }
                    if (info.section >= 0 && info.offset == f->sections_[static_cast<std::size_t>(info.section)].revOffset)
                        info.kind = FieldKind::Rev;
                }
            }
        }
        return f;
    }

    const FieldInfo* Fields::find(std::string_view path) const noexcept
    {
        const int i = indexOf(path);
        return i < 0 ? nullptr : &fields_[static_cast<std::size_t>(i)];
    }

    int Fields::indexOf(std::string_view path) const noexcept
    {
        for (std::size_t i = 0; i < fields_.size(); ++i)
            if (fields_[i].path == path)
                return static_cast<int>(i);
        return -1;
    }

    int Fields::sectionOf(std::string_view nameOrPath) const noexcept
    {
        for (std::size_t s = 0; s < sections_.size(); ++s)
            if (sections_[s].name == nameOrPath || sections_[s].path == nameOrPath)
                return static_cast<int>(s);
        return -1;
    }

    FieldRef Fields::ref(int index) const noexcept
    {
        FieldRef r;
        if (index < 0 || static_cast<std::size_t>(index) >= fields_.size())
            return r;
        const FieldInfo& info = fields_[static_cast<std::size_t>(index)];
        r.offset = static_cast<std::uint32_t>(info.offset);
        r.size = static_cast<std::uint32_t>(info.size);
        r.count = static_cast<std::uint32_t>(info.count);
        r.scalar = info.scalar;
        r.section = static_cast<std::int16_t>(info.section);
        r.field = index;
        r.generation = generation_;
        return r;
    }

    FieldRef Fields::ref(std::string_view path, std::string& error) const
    {
        const int exact = indexOf(path);
        if (exact >= 0)
            return ref(exact);
        Schema::Handle handle;
        if (!schema_->find(path, handle, error))
            return {};
        if (handle.node->scalar == Scalar::None || handle.node->scalar == Scalar::Pointer)
        {
            error = "'" + std::string(path) + "' is '" + handle.node->type + "': name a field of numbers or a table";
            return {};
        }
        // the field that holds it: the longest path that is a prefix of this one at a '.' or '['
        int owner = -1;
        std::size_t best = 0;
        for (std::size_t i = 0; i < fields_.size(); ++i)
        {
            const std::string& p = fields_[i].path;
            if (p.size() > best && path.size() > p.size() && path.substr(0, p.size()) == p
                && (path[p.size()] == '.' || path[p.size()] == '['))
            {
                owner = static_cast<int>(i);
                best = p.size();
            }
        }
        if (owner < 0)
        {
            error = "'" + std::string(path) + "' is in no field the host writes";
            return {};
        }
        FieldRef r = ref(owner);
        r.offset = static_cast<std::uint32_t>(handle.offset);
        r.count = static_cast<std::uint32_t>(handle.count);
        r.size = static_cast<std::uint32_t>(handle.count * handle.node->stride);
        r.scalar = handle.node->scalar;
        return r;
    }

    std::string Fields::json() const
    {
        using nlohmann::ordered_json;
        ordered_json root;
        root["module"] = schema_->module();
        root["size"] = schema_->size();
        ordered_json sections = ordered_json::array();
        for (const Section& s : sections_)
            sections.push_back({{"path", s.path}, {"offset", s.offset}, {"size", s.size}, {"rev", s.revOffset}});
        root["sections"] = sections;
        ordered_json list = ordered_json::array();
        for (const FieldInfo& f : fields_)
        {
            ordered_json j;
            j["path"] = f.path;
            j["type"] = f.type;
            j["kind"] = kindName(f.kind);
            j["offset"] = f.offset;
            j["size"] = f.size;
            if (!f.extents.empty())
                j["extents"] = f.extents;
            if (f.minimum)
                j["min"] = *f.minimum;
            if (f.maximum)
                j["max"] = *f.maximum;
            if (f.defaultValue)
                j["default"] = *f.defaultValue;
            if (!f.label.empty())
                j["label"] = f.label;
            if (!f.quantity.empty())
                j["quantity"] = f.quantity;
            if (!f.unit.empty())
                j["unit"] = f.unit;
            if (!f.choices.empty())
                j["choices"] = f.choices;
            if (!f.taper.empty())
                j["taper"] = f.taper;
            if (f.decimals >= 0)
                j["decimals"] = f.decimals;
            if (!f.group.empty())
                j["group"] = f.group;
            if (!f.help.empty())
                j["help"] = f.help;
            if (f.hidden)
                j["hidden"] = true;
            if (!f.axisX.empty() || !f.axisY.empty())
                j["axes"] = {f.axisX, f.axisY};
            if (f.table())
            {
                ordered_json leaves = ordered_json::array();
                for (const FieldInfo::Leaf& leaf : f.leaves)
                    leaves.push_back({{"name", leaf.name}, {"offset", leaf.offset}, {"count", leaf.count}});
                j["leaves"] = leaves;
            }
            list.push_back(std::move(j));
        }
        root["fields"] = list;
        return root.dump(2) + "\n";
    }

    std::string Fields::snapshot() const
    {
        std::string out;
        for (const FieldInfo& f : fields_)
            out += f.path + "\t" + f.type + "\t" + kindName(f.kind) + "\n";
        return out;
    }

    std::string Fields::markdown() const
    {
        std::string out;
        out += "# Parameters\n\n*Generated by `tee-schema --markdown` from the simulator's Spec: do not edit by hand.*\n";
        for (std::size_t s = 0; s < sections_.size(); ++s)
        {
            out += "\n## `" + sections_[s].path + "`\n\n";
            out += "| Field | Label | Kind | Range | Default | Unit | Help |\n|---|---|---|---|---|---|---|\n";
            for (const FieldInfo& f : fields_)
            {
                if (f.section != static_cast<int>(s) || f.kind == FieldKind::Rev)
                    continue;
                std::string range = f.hasRange() ? number(*f.minimum) + " to " + number(*f.maximum) : "";
                std::string kind = kindName(f.kind);
                if (!f.choices.empty())
                {
                    kind += ": ";
                    for (std::size_t c = 0; c < f.choices.size(); ++c)
                        kind += (c == 0 ? "" : ", ") + f.choices[c];
                }
                std::string unit = !f.quantity.empty() ? f.quantity : f.unit;
                if (f.table())
                    unit = f.axisX + " by " + f.axisY;
                std::string def = f.defaultValue ? number(*f.defaultValue) : "";
                if (f.kind == FieldKind::Choice && f.defaultValue && f.minimum)
                {
                    const long i = std::lround(*f.defaultValue - *f.minimum);
                    if (i >= 0 && static_cast<std::size_t>(i) < f.choices.size())
                        def = f.choices[static_cast<std::size_t>(i)];
                }
                std::string name = "`" + f.path.substr(sections_[s].path.size() + 1) + "`";
                if (f.hidden)
                    name += " (hidden)";
                out += "| " + name + " | " + f.label + " | " + kind + " | " + range + " | " + def + " | " + unit + " | "
                       + f.help + " |\n";
            }
        }
        return out;
    }

    std::vector<std::string> lint(const Fields& fields)
    {
        std::vector<std::string> problems;
        const auto say = [&](const FieldInfo& f, const std::string& what) { problems.push_back(f.path + ": " + what); };

        // every field directly in spec is a section
        const Schema::Node* spec = nullptr;
        for (const Schema::Node& n : fields.schema().fields())
            if (n.name == "spec")
                spec = &n;
        if (spec != nullptr)
            for (const Schema::Node& child : spec->children)
                if (fields.sectionOf(child.name) < 0)
                    problems.push_back("spec." + child.name
                                       + ": not a section (a module held by value whose first field is `int rev;`)");

        for (const FieldInfo& f : fields.all())
        {
            if (f.kind == FieldKind::Rev)
                continue;
            if (!f.quantity.empty() && !isQuantity(f.quantity))
                say(f, "ui::quantity(\"" + f.quantity + "\") is none of the list (02-contracts.md, section 3)");
            if (!f.quantity.empty() && !f.unit.empty())
                say(f, "has both ui::quantity and ui::unit: a quantity's unit is the user's");
            if (f.block == "tel" && f.label.empty())
                say(f, "a telemetry field has no ui::label");
            if (f.block != "spec")
                continue;
            if (f.label.empty())
                say(f, "no ui::label");
            if (f.table())
            {
                if (f.axisX.empty() || f.axisY.empty())
                    say(f, "a table has no ui::axes(x, y)");
                for (const std::string& a : {f.axisX, f.axisY})
                    if (!a.empty() && !isQuantity(a))
                        say(f, "ui::axes names \"" + a + "\", which is no quantity");
                continue;
            }
            if (!f.defaultValue)
                say(f, "no fminus::default");
            if (f.kind != FieldKind::Switch && !f.hasRange())
                say(f, "no fminus::range");
            if (f.hasRange() && !(*f.minimum < *f.maximum))
                say(f, "its range's minimum is not below its maximum");
            if (f.hasRange() && f.defaultValue && (*f.defaultValue < *f.minimum || *f.defaultValue > *f.maximum))
                say(f, "its default " + number(*f.defaultValue) + " is outside its range");
            if (!f.choices.empty())
            {
                if (f.scalar != Scalar::Int)
                    say(f, "ui::choices on a field that is no int");
                else if (f.hasRange())
                {
                    const double span = *f.maximum - *f.minimum + 1;
                    if (span != static_cast<double>(f.choices.size()))
                        say(f, std::to_string(f.choices.size()) + " choices for a range of " + number(span) + " values");
                }
            }
            if (!f.taper.empty() && f.taper != "log" && f.taper != "linear")
                say(f, "ui::taper(\"" + f.taper + "\") is neither \"log\" nor \"linear\"");
            if (f.taper == "log" && f.hasRange() && !(*f.minimum > 0))
                say(f, "a log taper wants a range above 0");
        }
        return problems;
    }

    std::vector<std::string> compareSnapshot(const Fields& fields, std::string_view snapshot)
    {
        std::vector<std::string> problems;
        std::istringstream in{std::string(snapshot)};
        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            const std::size_t tab = line.find('\t');
            const std::size_t tab2 = tab == std::string::npos ? tab : line.find('\t', tab + 1);
            const std::string path = line.substr(0, tab);
            const std::string type = tab == std::string::npos ? "" : line.substr(tab + 1, tab2 - tab - 1);
            const FieldInfo* f = fields.find(path);
            if (f == nullptr)
                problems.push_back(path + ": disappeared since the snapshot (the Spec is append-only)");
            else if (f->type != type)
                problems.push_back(path + ": was '" + type + "', is '" + f->type + "' (a field never changes its type)");
        }
        return problems;
    }
} // namespace tee
