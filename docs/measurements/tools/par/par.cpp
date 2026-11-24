// docs/measurements/tools/par/par.cpp: what a FMinus parallel { } region costs (q5-design-note.md). Built as build.sh links qbench,
// against FMinus alone: ./par par.fm
#include "fminus/FMinus.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
int main(int argc, char** argv)
{
    std::ifstream in(argv[1]);
    std::stringstream ss; ss << in.rdbuf();
    std::string errors;
    fminus::Program::Options o;
    o.denormals = fminus::Program::Denormals::Flush;
    auto p = fminus::Program::compile(ss.str(), errors, o);
    if (!p) { std::fprintf(stderr, "%s\n", errors.c_str()); return 1; }
    auto ser = p->function<float(void*, int, int)>("Two::serial", errors);
    auto spl = p->function<float(void*, int, int)>("Two::split", errors);
    if (!ser || !spl) { std::fprintf(stderr, "%s\n", errors.c_str()); return 1; }
    const int parts = 6000;
    static float mem[16384] = {};   // a second of 16-sample parts at 96 kHz
    for (int work : {0, 200, 800, 1600, 3200, 6400, 12800})
    {
        double best[2] = {1e9, 1e9};
        float sink = 0;
        for (int r = 0; r < 5; ++r)
            for (int k = 0; k < 2; ++k)
            {
                auto t0 = std::chrono::steady_clock::now();
                sink += (k == 0 ? ser : spl)(mem, parts, work);
                double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                best[k] = std::min(best[k], s);
            }
        std::printf("| %d | %.2f | %.2f | %.2f |\n", work, 1e6 * best[0] / parts, 1e6 * best[1] / parts, 1e6 * (best[1] - best[0] / 2) / parts);
        (void)sink;
    }
}
