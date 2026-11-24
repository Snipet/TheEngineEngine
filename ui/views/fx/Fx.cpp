// ui/views/fx/Fx.cpp: see Fx.h.
#include "views/fx/Fx.h"

#include "kit/shaders/Registry.h"

#include <funkgui/gpu/ShaderLibrary.h>

#include <algorithm>
#include <atomic>
#include <cmath>

namespace tee::ui::fx
{
    namespace
    {
        std::atomic<bool> g_enabled{ true };

        // The id FunkGui holds a shader's program under: its bytes' FNV-1a (kit/shaders/ShaderText.h shaderId), 0 when
        // this build has no binary for it.
        uint32_t idOf(Effect e)
        {
            const shaders::Built* b = shaders::find("fx", fileOf(e));
            if (b == nullptr || b->size == 0)
                return 0;
            uint32_t h = 2166136261u;
            for (uint32_t i = 0; i < b->size; ++i)
                h = (h ^ b->data[i]) * 16777619u;
            return h == 0 ? 1u : h;
        }

        // The shader over r with its params, when it is there to draw (Fx.h's list); false and nothing recorded else.
        bool draw(funkgui::Canvas& c, Effect e, const funkgui::Rect& r, const float* params, int n, float amount)
        {
            if (!(amount > 0.0f) || !g_enabled.load(std::memory_order_relaxed) || !(r.w > 0.0f) || !(r.h > 0.0f))
                return false;
            const uint32_t id = idOf(e);
            if (id == 0 || !funkgui::shaders::has(id))
                return false;
            c.shader(id, r, params, n);
            return true;
        }

        float unit(std::uint8_t v) { return static_cast<float>(v) / 255.0f; }
        float clamp01(float v) { return std::isfinite(v) ? std::clamp(v, 0.0f, 1.0f) : 0.0f; }
        float wrap(float phase) { return std::isfinite(phase) ? phase - std::floor(phase) : 0.0f; }
    }

    const char* fileOf(Effect e) noexcept
    {
        switch (e)
        {
            case Effect::glow: return "glow.sc";
            case Effect::flame: return "flame.sc";
            case Effect::smoke: return "smoke.sc";
            case Effect::shimmer: return "shimmer.sc";
        }
        return "";
    }

    bool available(Effect e)
    {
        const uint32_t id = idOf(e);
        return g_enabled.load(std::memory_order_relaxed) && id != 0 && funkgui::shaders::has(id);
    }

    void setEnabled(bool on) noexcept { g_enabled.store(on, std::memory_order_relaxed); }
    bool enabled() noexcept { return g_enabled.load(std::memory_order_relaxed); }

    bool glow(funkgui::Canvas& c, const funkgui::Rect& r, float heat, funkgui::Col color, float phase, float inner)
    {
        const float p[6] = { clamp01(heat), unit(color.r), unit(color.g), unit(color.b), wrap(phase),
                             std::isfinite(inner) ? std::clamp(inner, 0.0f, 0.9f) : 0.4f };
        return draw(c, Effect::glow, r, p, 6, p[0]);
    }

    bool flame(funkgui::Canvas& c, const funkgui::Rect& r, float strength, float phase, bool fromRight)
    {
        const float p[3] = { clamp01(strength), wrap(phase), fromRight ? 1.0f : 0.0f };
        return draw(c, Effect::flame, r, p, 3, p[0]);
    }

    bool smoke(funkgui::Canvas& c, const funkgui::Rect& r, float density, funkgui::Col color, float phase)
    {
        const float p[5] = { clamp01(density), wrap(phase), unit(color.r), unit(color.g), unit(color.b) };
        return draw(c, Effect::smoke, r, p, 5, p[0]);
    }

    bool shimmer(funkgui::Canvas& c, const funkgui::Rect& r, float heat, funkgui::Col color, float phase)
    {
        const float p[5] = { clamp01(heat), wrap(phase), unit(color.r), unit(color.g), unit(color.b) };
        return draw(c, Effect::shimmer, r, p, 5, p[0]);
    }
}
