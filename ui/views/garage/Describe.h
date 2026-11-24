// ui/views/garage/Describe.h: a build in words and numbers, from its values (Library.h FieldValue) and what the
// document says each field means (FieldInfo): the engine in a sentence ("3.0 L straight six, twin turbo, E85"), a
// field's value as text in the user's units, and the fields that differ between two builds, by section.
#pragma once

#include "views/garage/Library.h"

#include "kit/bind/Fields.h"
#include "kit/bind/Units.h"

#include <string>
#include <string_view>
#include <vector>

namespace tee::ui::garage
{
    // A build's value of `path`, or nullptr.
    const FieldValue* valueOf(const BuildSummary& b, std::string_view path) noexcept;
    // Its first number (NaN when absent).
    double numberOf(const BuildSummary& b, std::string_view path) noexcept;
    // A choice's name ("" when absent or not a choice in `doc`).
    std::string choiceOf(const BuildSummary& b, const FieldDoc& doc, std::string_view path);

    // The engine in a sentence: displacement, the layout in words, the aspiration, a fuel other than petrol.
    // "" for a car or a tune alone.
    std::string sentence(const BuildSummary& b, const FieldDoc& doc);
    // The layout's name in words: "Inline 6" "straight six", "V8 crossplane" "V8 crossplane", "Flat 4" "flat four".
    std::string layoutWords(std::string_view layoutName);
    // The engine's swept volume, m^3 (engine.displacement, else bore, stroke and cylinders); NaN when it cannot say.
    double displacement(const BuildSummary& b, const FieldDoc& doc);

    // A value as text in the user's units by what `doc` says field `path` is: a choice's name, a switch's on/off, a
    // number with its unit, an array's elements; "-" when absent.
    std::string valueText(const FieldValue* v, const FieldDoc& doc, std::string_view path, const Units& units);

    // One field that differs: its path, its label, and both texts.
    struct Difference
    {
        std::string section, path, label, a, b;
    };
    // The fields of the document's whose values differ between a and b (a field one sets and the other does not,
    // too), in the document's order; tables compared as wholes.
    std::vector<Difference> differences(const BuildSummary& a, const BuildSummary& b, const FieldDoc& doc,
                                        const Units& units);
    // A section's word for a heading ("engine" "ENGINE", "ecu" "TUNE").
    std::string sectionWord(std::string_view section);
}
