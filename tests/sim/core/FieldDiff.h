// tests/sim/core/FieldDiff.h: every number field of two copies of the Engine instance compared, by the Schema's paths
// (card D: the init routes' test and the custom engine's null; the probes in EngineExplore.cpp)
#pragma once

#include "tee/Schema.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace tee::test::core
{
    struct Diff
    {
        std::string path;   // "intk.run[0].sb0[1]"
        double a, b;
    };

    inline double fieldElement(const unsigned char* p, tee::Scalar s)
    {
        switch (s)
        {
        case tee::Scalar::Float: { float v; std::memcpy(&v, p, 4); return v; }
        case tee::Scalar::Double: { double v; std::memcpy(&v, p, 8); return v; }
        case tee::Scalar::Int: { std::int32_t v; std::memcpy(&v, p, 4); return v; }
        case tee::Scalar::Bool: return *p ? 1.0 : 0.0;
        case tee::Scalar::Char: return *p;
        default: return 0.0;
        }
    }

    // Every element of `nodes` (at `base` in both copies) whose bytes differ, NaN against NaN apart; pointers and
    // what the host reads as no number are skipped
    inline void walk(const std::vector<tee::Schema::Node>& nodes, std::size_t base, const std::string& prefix,
                     const unsigned char* a, const unsigned char* b, std::vector<Diff>& out)
    {
        for (const auto& n : nodes)
        {
            const std::size_t count = n.count();
            for (std::size_t i = 0; i < count; ++i)
            {
                const std::size_t off = base + n.offset + i * n.stride;
                const std::string path = prefix + n.name + (n.extents.empty() ? "" : "[" + std::to_string(i) + "]");
                if (!n.children.empty())
                    walk(n.children, off, path + ".", a, b, out);
                else if (n.scalar != tee::Scalar::None && n.scalar != tee::Scalar::Pointer)
                {
                    const double x = fieldElement(a + off, n.scalar), y = fieldElement(b + off, n.scalar);
                    if (std::memcmp(a + off, b + off, n.stride) != 0 && !(std::isnan(x) && std::isnan(y)))
                        out.push_back({path, x, y});
                }
            }
        }
    }
} // namespace tee::test::core
