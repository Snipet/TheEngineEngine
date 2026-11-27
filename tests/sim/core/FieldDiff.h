// tests/sim/core/FieldDiff.h: every number field of two copies of the Engine instance compared, by the Schema's paths
// (card D: the init routes' test and the custom engine's null; the probes in EngineExplore.cpp)
#pragma once

#include "tee/Schema.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <set>
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

    // "name[3]...": true with `index` 3 when the path starts with name and an index
    inline bool indexAfter(const std::string& path, const std::string& name, int& index)
    {
        if (path.rfind(name + "[", 0) != 0)
            return false;
        index = std::atoi(path.c_str() + name.size() + 1);
        return true;
    }

    // The counts an engine has in use, by which a field of a part past them is read by nothing
    struct InUse
    {
        int n = 0;          // cylinders (lay.n)
        int ports = 0;      // ports (lay.ports): the intake's runners
        int exhModel = 0;   // the exhaust's model (exh.model): under 1 the model-0 pipes are unused
        int nUsed = 0;      // the builder's pipes in use (exh.b.net.nUsed), and which (used)
        std::set<int> used;
        int nj = 0;         // the builder's junctions (exh.b.net.nj), and each one's branches (jn)
        std::vector<int> jn;
        int nBands = 0;     // the builder's noise bands
        int ne = 0;         // the builder's open ends
        int nSides = 0;     // the builder's sides (a muffler each)

        bool operator==(const InUse& o) const
        {
            return n == o.n && ports == o.ports && exhModel == o.exhModel && nUsed == o.nUsed && used == o.used && nj == o.nj && jn == o.jn &&
                   nBands == o.nBands && ne == o.ne && nSides == o.nSides;
        }
    };

    // Read by `field` (a path to its value, as Bench::field)
    template <typename Field>
    InUse readInUse(Field field)
    {
        InUse u;
        u.n = int(field("lay.n"));
        u.ports = int(field("lay.ports"));
        u.exhModel = int(field("exh.model"));
        u.nUsed = int(field("exh.b.net.nUsed"));
        for (int k = 0; k < u.nUsed; ++k)
            u.used.insert(int(field("exh.b.net.used[" + std::to_string(k) + "]")));
        u.nj = int(field("exh.b.net.nj"));
        for (int j = 0; j < u.nj; ++j)
            u.jn.push_back(int(field("exh.b.net.jN[" + std::to_string(j) + "]")));
        u.nBands = int(field("exh.b.net.nBands"));
        u.ne = int(field("exh.b.net.ne"));
        u.nSides = int(field("exh.b.nSides"));
        return u;
    }

    // The kinds of field two routes into the same engine may leave different with nothing wrong, each with its reason;
    // nullptr: none (then a difference is a finding). `draws`: also a configure's own draws (each configure draws the
    // cylinders' bias again, and the pipes' delay smoothing starts from where the last left it), for a case that cannot
    // write the same draws on both routes (X's switch case)
    inline const char* inertKind(const std::string& p, const InUse& u, bool draws = false)
    {
        int i = 0;
        if (p.size() > 4 && p.compare(p.size() - 4, 4, ".rev") == 0)
            return "a revision count: each route writes the Spec, and builds the exhaust, a different number of times";
        if (p.rfind("revSeen[", 0) == 0 || p.rfind("modelRev[", 0) == 0 || p == "exh.seenRev" || p == "boost.mach.mapRev")
            return "a revision count: each route writes the Spec, and builds the exhaust, a different number of times";
        if ((indexAfter(p, "cyl", i) || indexAfter(p, "geo.fire", i) || indexAfter(p, "geo.tdc", i) || indexAfter(p, "geo.order", i) ||
             indexAfter(p, "geo.axisX", i) || indexAfter(p, "geo.axisY", i) || indexAfter(p, "geo.z", i) || indexAfter(p, "mm.cph", i) ||
             indexAfter(p, "mm.sph", i) || indexAfter(p, "lay.fire", i) || indexAfter(p, "lay.bank", i)) && i >= u.n)
            return "a cylinder past the engine's count: the last engine's, read by nothing";
        if ((indexAfter(p, "intk.run", i) || indexAfter(p, "intk.endRun", i) || indexAfter(p, "exh.b.net.portD", i)) && i >= u.ports)
            return "a port's pipe past the engine's count: the last engine's, read by nothing";
        if (u.exhModel != 0)
            for (const char* m0 : {"exh.prim[", "exh.ch1[", "exh.ch2[", "exh.pipe[", "exh.tail[", "exh.mid[", "exh.sec[", "exh.cross.", "exh.lanes.",
                                   "exh.laneKind[", "exh.laneIdx[", "exh.endR[", "exh.kMuf[", "exh.kExit[", "exh.kHin", "exh.kHout", "exh.kInM",
                                   "exh.kOutM", "exh.kSin", "exh.kSout", "exh.kXp", "exh.rCat", "exh.rPerf", "exh.tick128"})
                if (p.rfind(m0, 0) == 0)
                    return "the model-0 exhaust's pipes: unused under exhaust.model 1";
        if (indexAfter(p, "exh.b.net.d", i) && u.used.count(i) == 0)
            return "a builder's pipe not in use (exh.b.net.used): the last engine's";
        if (p.rfind("exh.b.net.d[", 0) == 0 && (p.find(".pgIn[") != std::string::npos || p.find(".pgLast") != std::string::npos ||
                                                p.find(".pgHave") != std::string::npos))
            return "the passivity check's cache (XBuild::filterFirst): inputs as last checked, refreshed when they move";
        if (indexAfter(p, "exh.b.net.used", i) && i >= u.nUsed)
            return "the builder's list of pipes past its count";
        for (const char* jf : {"exh.b.net.jKind", "exh.b.net.jN", "exh.b.net.jPop", "exh.b.net.jRef", "exh.b.net.jT", "exh.b.net.jP",
                               "exh.b.net.jDrive", "exh.b.net.jAdmit", "exh.b.net.jPx"})
            if (indexAfter(p, jf, i) && i >= u.nj)
                return "a junction past the builder's count";
        for (const char* bf : {"exh.b.net.bD", "exh.b.net.bE", "exh.b.net.bR", "exh.b.net.bT", "exh.b.net.bKf", "exh.b.net.bKr", "exh.b.net.bS",
                               "exh.b.net.bA", "exh.b.net.bBand", "exh.b.net.bNa", "exh.b.net.bNrho", "exh.b.net.bNin", "exh.b.net.bNout"})
            if (indexAfter(p, bf, i) && (i / 14 >= u.nj || i % 14 >= u.jn[std::size_t(i / 14)]))
                return "a junction's branch past its count (XN_BR 14 a junction)";
        for (const char* gf : {"exh.b.net.fnHp", "exh.b.net.fnLp", "exh.b.net.fnLp2", "exh.b.net.fnN", "exh.b.net.fnG", "exh.b.net.drawOrder"})
            if (indexAfter(p, gf, i) && i >= u.nBands)
                return "a noise band past the builder's count";
        if (indexAfter(p, "exh.b.net.ends", i) && i >= u.ne)
            return "an open end past the builder's count";
        if ((indexAfter(p, "exh.b.jMuf", i) && i / 5 >= u.nSides) || (indexAfter(p, "exh.b.prevMuf", i) && i >= u.nSides))
            return "a muffler's side past the build's count";
        // (the pipes' delay history under exh. and intk. alike; noise.rs: the bias's draws come from the engine's stream)
        if (draws && (p.find(".bias") != std::string::npos || p == "noise.rs" || p.find(".ageF") != std::string::npos ||
                      p.find(".dPrevF") != std::string::npos || p.find(".rF") != std::string::npos))
            return "a configure's draws: the cylinders' bias drawn again from the engine's noise, the pipes' delay smoothing from where "
                   "the last left it";
        return nullptr;
    }
} // namespace tee::test::core
