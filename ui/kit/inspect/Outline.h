// ui/kit/inspect/Outline.h: what an inspector page holds, in order, whatever draws it (card J2's inspector designs,
// docs/ui-kit.md "Inspector designs"): its headings, its fields, its tables and its notes. Read from the page the frame
// already makes (a FieldPage over a workspace's .gui text, or over a generated page), so a workspace's own arrangement
// (its sections, their order, its warnings) is kept by every design without the workspace knowing about them.
#pragma once

#include "kit/bind/Binding.h"
#include "kit/bind/Fields.h"

#include <string>
#include <vector>

namespace tee::ui
{
    struct OutlineEntry
    {
        enum class Kind : std::uint8_t { heading, field, table, note };
        Kind kind = Kind::field;
        std::string text;            // a heading's or a note's text; a table's title
        int field = -1;              // a field's index in the document
        std::vector<int> fields;     // a table's fields (a map or a curve: one; an array: its elements)
        std::string widget;          // a table's element: "map", "curve", "array"
    };
    using Outline = std::vector<OutlineEntry>;

    // The page's outline: a section's title is a heading, a control is its field, a map, a curve or an array is a
    // table, a label is a note; a field the page shows twice comes once.
    Outline outlineOf(const FieldPage& page, const FieldDoc& doc);
    // Fields in the document's order as an outline: a heading at each change of section or ui::group, tables in place.
    Outline outlineOf(const FieldDoc& doc, const std::vector<int>& fields);
    // Every field an outline holds (a table's too), in order.
    std::vector<int> fieldsOf(const Outline& outline);
}
