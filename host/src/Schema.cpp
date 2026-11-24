// host/src/Schema.cpp: tee::Schema (Schema.h).
#include "tee/Schema.h"

#include <cctype>
#include <cstdio>

namespace tee
{
    namespace
    {
        using Module = fminus::Program::Module;

        bool endsWith(std::string_view text, std::string_view tail)
        {
            return text.size() >= tail.size() && text.substr(text.size() - tail.size()) == tail;
        }

        // A type FMinus writes, as what the host reads of it.
        Scalar scalarOfType(std::string_view type)
        {
            if (endsWith(type, "*"))
                return Scalar::Pointer;
            if (type == "float")
                return Scalar::Float;
            if (type == "double")
                return Scalar::Double;
            if (type == "int")
                return Scalar::Int;
            if (type == "bool")
                return Scalar::Bool;
            if (type == "char")
                return Scalar::Char;
            return Scalar::None;
        }

        std::size_t product(const std::vector<std::size_t>& extents)
        {
            std::size_t n = 1;
            for (const std::size_t e : extents)
                n *= e;
            return n;
        }

        // The fields of `module`, the modules they hold followed. A module holds another by value only when that one
        // is declared before it, so this ends; `depth` guards a program description that would say otherwise.
        std::vector<Schema::Node> nodesOf(const fminus::Program& program, const Module& module, int depth)
        {
            std::vector<Schema::Node> nodes;
            nodes.reserve(module.fields.size());
            for (const Module::Field& field : module.fields)
            {
                Schema::Node node;
                node.name = field.name;
                node.type = field.type;
                node.offset = field.offset;
                node.size = field.size;
                node.extents = field.extents;
                node.attributes = field.attributes;
                const std::string& element = field.element.empty() ? field.type : field.element;
                if (element.rfind("param ", 0) == 0)
                {
                    // A parameter's slot is struct { const float* stream; T value; }: the host reads the value.
                    node.parameter = true;
                    node.scalar = scalarOfType(std::string_view(element).substr(6));
                    for (const fminus::Program::Parameter& parameter : module.parameters)
                    {
                        if (parameter.name == field.name)
                        {
                            node.param = &parameter;
                            node.offset = parameter.valueOffset;
                            node.size = node.scalar == Scalar::Bool ? 1 : 4;
                        }
                    }
                    node.stride = node.size;
                }
                else
                {
                    node.scalar = scalarOfType(element);
                    const std::size_t elements = product(node.extents);
                    node.stride = elements > 0 ? node.size / elements : 0;
                    if (node.scalar == Scalar::None && depth < 32)
                    {
                        if (const Module* nested = program.module(element))
                        {
                            node.module = nested->name;
                            node.children = nodesOf(program, *nested, depth + 1);
                        }
                    }
                }
                nodes.push_back(std::move(node));
            }
            return nodes;
        }

        void visitNodes(const std::vector<Schema::Node>& nodes, const std::string& prefix, std::size_t base,
                        const Schema::Visitor& visit)
        {
            for (const Schema::Node& node : nodes)
            {
                const std::string path = prefix.empty() ? node.name : prefix + "." + node.name;
                if (node.children.empty())
                {
                    if (node.scalar != Scalar::None)
                        visit(path, node, base + node.offset);
                    continue;
                }
                // a module, or an array of them: each element's fields in turn
                const std::size_t n = node.count();
                for (std::size_t i = 0; i < n; ++i)
                {
                    std::string element = path;
                    std::size_t left = i;
                    std::size_t span = n;
                    for (const std::size_t extent : node.extents)
                    {
                        span /= extent;
                        element += "[" + std::to_string(left / span) + "]";
                        left %= span;
                    }
                    visitNodes(node.children, element, base + node.offset + i * node.stride, visit);
                }
            }
        }

        std::string attributesText(const std::vector<Schema::Attribute>& attributes)
        {
            std::string text;
            for (const Schema::Attribute& attribute : attributes)
            {
                text += " [[" + attribute.name;
                if (!attribute.arguments.empty())
                {
                    text += "(";
                    for (std::size_t i = 0; i < attribute.arguments.size(); ++i)
                    {
                        const Schema::Attribute::Argument& argument = attribute.arguments[i];
                        text += i == 0 ? "" : ", ";
                        text += argument.isString ? "\"" + argument.text + "\"" : argument.text;
                    }
                    text += ")";
                }
                text += "]]";
            }
            return text;
        }

        void describeNodes(const std::vector<Schema::Node>& nodes, std::size_t base, int indent, std::string& out)
        {
            for (const Schema::Node& node : nodes)
            {
                char line[64];
                std::snprintf(line, sizeof line, "%8zu %8zu  ", base + node.offset, node.size);
                out += line;
                out.append(static_cast<std::size_t>(indent) * 2, ' ');
                out += node.type + " " + node.name + attributesText(node.attributes) + "\n";
                if (!node.children.empty())
                    describeNodes(node.children, base + node.offset, indent + 1, out);
            }
        }
    } // namespace

    std::size_t Schema::Node::count() const noexcept
    {
        return product(extents);
    }

    const Schema::Attribute* Schema::Node::attribute(std::string_view attributeName) const noexcept
    {
        return fminus::Program::findAttribute(attributes, attributeName);
    }

    std::unique_ptr<Schema> Schema::of(const fminus::Program& program, std::string_view name, std::string& error)
    {
        const Module* module = program.module(name);
        if (module == nullptr)
        {
            error = "the program has no module '" + std::string(name) + "'";
            return nullptr;
        }
        std::unique_ptr<Schema> schema(new Schema());
        schema->module_ = module->name;
        schema->size_ = module->size;
        schema->alignment_ = module->alignment;
        schema->fields_ = nodesOf(program, *module, 0);
        return schema;
    }

    bool Schema::find(std::string_view path, Handle& handle, std::string& error) const
    {
        const auto fail = [&](const std::string& why) {
            error = "'" + std::string(path) + "' in module " + module_ + ": " + why;
            return false;
        };
        const std::vector<Node>* nodes = &fields_;
        std::size_t offset = 0;
        std::size_t at = 0;
        while (true)
        {
            // a name
            std::size_t end = at;
            while (end < path.size() && (std::isalnum(static_cast<unsigned char>(path[end])) != 0 || path[end] == '_'))
                ++end;
            if (end == at)
                return fail("a field's name is expected at character " + std::to_string(at + 1));
            const std::string_view name = path.substr(at, end - at);
            const Node* node = nullptr;
            for (const Node& candidate : *nodes)
                if (candidate.name == name)
                    node = &candidate;
            if (node == nullptr)
                return fail("no field '" + std::string(name) + "'");
            at = end;
            offset += node->offset;

            // its indexes, outermost first
            std::size_t span = node->count();
            std::size_t dimension = 0;
            while (at < path.size() && path[at] == '[')
            {
                std::size_t digits = at + 1;
                std::size_t index = 0;
                while (digits < path.size() && std::isdigit(static_cast<unsigned char>(path[digits])) != 0)
                {
                    index = index * 10 + static_cast<std::size_t>(path[digits] - '0');
                    if (index > 1'000'000'000)
                        return fail("an index is too large");
                    ++digits;
                }
                if (digits == at + 1 || digits >= path.size() || path[digits] != ']')
                    return fail("an index is written [n], at character " + std::to_string(at + 1));
                if (dimension >= node->extents.size())
                    return fail("'" + node->name + "' is '" + node->type + "': one index too many");
                if (index >= node->extents[dimension])
                    return fail("index " + std::to_string(index) + " is outside '" + node->name + "', which is '"
                                + node->type + "'");
                span /= node->extents[dimension];
                offset += index * span * node->stride;
                ++dimension;
                at = digits + 1;
            }

            if (at == path.size())
            {
                handle.node = node;
                handle.offset = offset;
                handle.count = span;
                return true;
            }
            if (path[at] != '.')
                return fail("unexpected '" + std::string(1, path[at]) + "' at character " + std::to_string(at + 1));
            if (node->children.empty())
                return fail("'" + node->name + "' is '" + node->type + "', which has no fields to name"
                            + (node->scalar == Scalar::Pointer ? " (a path does not follow a pointer)" : ""));
            if (dimension != node->extents.size())
                return fail("'" + node->name + "' is '" + node->type + "': name one element of it before its field");
            nodes = &node->children;
            ++at;
        }
    }

    void Schema::forEach(const Visitor& visit) const
    {
        visitNodes(fields_, std::string(), 0, visit);
    }

    std::string Schema::describe() const
    {
        std::string out = "module " + module_ + ": " + std::to_string(size_) + " bytes, aligned to "
                          + std::to_string(alignment_) + "\n  offset     size  field\n";
        describeNodes(fields_, 0, 0, out);
        return out;
    }
} // namespace tee
