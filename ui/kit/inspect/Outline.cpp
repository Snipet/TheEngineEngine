// ui/kit/inspect/Outline.cpp: see Outline.h.
#include "kit/inspect/Outline.h"

#include <algorithm>
#include <set>

namespace tee::ui
{
    Outline outlineOf(const FieldPage& page, const FieldDoc& doc)
    {
        Outline out;
        std::set<int> seen;
        const std::vector<GuiNode>& nodes = page.spec().gui.nodes;
        FieldParams& params = const_cast<FieldPage&>(page).params();
        for (std::size_t n = 0; n < nodes.size(); ++n)
        {
            const GuiNode& node = nodes[n];
            switch (node.kind)
            {
                case GuiKind::section:
                    if (!node.text.empty())
                        out.push_back({ OutlineEntry::Kind::heading, node.text, -1, {}, {} });
                    break;
                case GuiKind::page:
                    if (!node.text.empty())
                        out.push_back({ OutlineEntry::Kind::heading, node.text, -1, {}, {} });
                    break;
                case GuiKind::label:
                    if (!node.text.empty())
                        out.push_back({ OutlineEntry::Kind::note, node.text, -1, {}, {} });
                    break;
                case GuiKind::knob: case GuiKind::slider: case GuiKind::toggle: case GuiKind::buttons: case GuiKind::menu:
                    if (node.slot >= 0 && node.slot < params.count())
                    {
                        const int f = params.field(node.slot);
                        if (seen.insert(f).second)
                            out.push_back({ OutlineEntry::Kind::field, {}, f, {}, {} });
                    }
                    break;
                case GuiKind::widget:
                {
                    std::vector<int> fs = fieldsOfWidget(doc, node);
                    if (fs.empty())
                        break;
                    std::string title = node.text;
                    if (title.empty())
                    {
                        const FieldInfo& f = doc.info(fs.front());
                        title = !f.arrayLabel.empty() ? f.arrayLabel : (f.label.empty() ? f.path : f.label);
                    }
                    for (const int f : fs)
                        seen.insert(f);
                    out.push_back({ OutlineEntry::Kind::table, title, -1, std::move(fs), node.widget });
                    break;
                }
                default: break;
            }
        }
        // Headings with nothing under them before the next go.
        Outline kept;
        for (std::size_t i = 0; i < out.size(); ++i)
        {
            if (out[i].kind == OutlineEntry::Kind::heading
                && (i + 1 == out.size() || out[i + 1].kind == OutlineEntry::Kind::heading))
                continue;
            kept.push_back(std::move(out[i]));
        }
        return kept;
    }

    Outline outlineOf(const FieldDoc& doc, const std::vector<int>& fields)
    {
        Outline out;
        std::string at;
        std::set<std::string> arrays;
        for (const int f : fields)
        {
            const FieldInfo& info = doc.info(f);
            const std::string head = std::string(info.section()) + "|" + info.group;
            if (head != at)
            {
                at = head;
                out.push_back({ OutlineEntry::Kind::heading, upper(info.group.empty() ? std::string(info.section()) : info.group), -1, {}, {} });
            }
            if (info.table())
                out.push_back({ OutlineEntry::Kind::table, info.label.empty() ? info.path : info.label, -1, { f },
                                info.kind == FieldKind::map ? "map" : "curve" });
            else if (arrayAsTable(info))
            {
                if (arrays.insert(info.array).second)
                    out.push_back({ OutlineEntry::Kind::table, info.arrayLabel, -1, arrayElements(doc, info.array), "array" });
            }
            else
                out.push_back({ OutlineEntry::Kind::field, {}, f, {}, {} });
        }
        return out;
    }

    std::vector<int> fieldsOf(const Outline& outline)
    {
        std::vector<int> out;
        for (const OutlineEntry& e : outline)
        {
            if (e.field >= 0)
                out.push_back(e.field);
            out.insert(out.end(), e.fields.begin(), e.fields.end());
        }
        return out;
    }
}
