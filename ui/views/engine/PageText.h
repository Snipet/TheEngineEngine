// ui/views/engine/PageText.h: an inspector page (a `.gui` text, docs/gui-language.md) made of titled sections of
// fields, in the order given, naming only the fields the build has. A section none of whose fields is there is left
// out; a page with nothing says so. The ENGINE and VALVES workspaces arrange their pages with it where order matters.
#pragma once

#include "kit/bind/Fields.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace tee::ui::pages
{
    struct Section
    {
        Section() = default;
        Section(std::string t, std::vector<std::string> p, std::vector<std::string> l = {})
            : title(std::move(t)), paths(std::move(p)), labels(std::move(l))
        {
        }
        std::string title;                // "CYLINDER", in capitals
        std::vector<std::string> paths;   // fields, in order
        std::vector<std::string> labels;  // a short label for each (the section's title says the rest); "" : the field's
        std::vector<std::string> warnings;   // amber lines in words under the section's fields (the build's oddities)
    };

    inline std::string quoted(const std::string& s) { return "\"" + s + "\""; }

    // Text in lines of at most 46 characters (a label's most is 48), broken at spaces.
    inline std::vector<std::string> lines(const std::string& text)
    {
        std::vector<std::string> out;
        for (std::size_t at = 0; at < text.size();)
        {
            std::size_t end = std::min(text.size(), at + 46);
            if (end < text.size())
                if (const std::size_t sp = text.rfind(' ', end); sp != std::string::npos && sp > at)
                    end = sp;
            out.push_back(text.substr(at, end - at));
            at = end + (end < text.size() && text[end] == ' ' ? 1 : 0);
        }
        return out;
    }

    inline std::string page(const FieldDoc& doc, const std::vector<Section>& sections, const std::string& note = {})
    {
        std::string out = "column gap=12 pad=4 {";
        bool any = false;
        std::vector<int> seen;   // a field named twice (a fallback that is also listed) is shown once
        // the note in lines of at most 46 characters (a label's most is 48), broken at spaces
        for (std::size_t at = 0; at < note.size();)
        {
            std::size_t end = std::min(note.size(), at + 46);
            if (end < note.size())
                if (const std::size_t sp = note.rfind(' ', end); sp != std::string::npos && sp > at)
                    end = sp;
            out += " label " + quoted(note.substr(at, end - at)) + " size=small ;";
            at = end + (end < note.size() && note[end] == ' ' ? 1 : 0);
        }
        for (const Section& s : sections)
        {
            std::string row, tables;
            for (std::size_t k = 0; k < s.paths.size(); ++k)
            {
                const int f = doc.find(s.paths[k]);
                if (f < 0 || doc.info(f).hidden || std::find(seen.begin(), seen.end(), f) != seen.end())
                    continue;
                seen.push_back(f);
                const FieldInfo& i = doc.info(f);
                const std::string lab = k < s.labels.size() && !s.labels[k].empty() ? " label=" + quoted(s.labels[k]) : "";
                switch (i.kind)
                {
                    case FieldKind::number:
                    case FieldKind::integer: row += " knob " + i.path + lab + " ;"; break;
                    case FieldKind::choice: row += " menu " + i.path + lab + " ;"; break;
                    case FieldKind::toggle: row += " switch " + i.path + " \"OFF\" \"ON\"" + lab + " ;"; break;
                    case FieldKind::curve: tables += " curve " + i.path + " label=" + quoted(i.label) + " ;"; break;
                    case FieldKind::map: tables += " map " + i.path + " label=" + quoted(i.label) + " ;"; break;
                }
            }
            if (row.empty() && tables.empty())
                continue;
            any = true;
            out += " section " + quoted(s.title) + " layout=column gap=10 {";
            if (!row.empty())
                out += " row wrap gap=12 {" + row + " } ;";
            out += tables;
            for (const std::string& w : s.warnings)
                for (const std::string& line : lines(w))
                    out += " label " + quoted(line) + " size=small color=yellow ;";
            out += " } ;";
        }
        if (!any)
            out += " label \"NOTHING TO SET HERE IN THIS BUILD YET\" size=small ;";
        return out + " }";
    }
}
