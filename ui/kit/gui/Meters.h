// ui/kit/gui/Meters.h: from TestingPlayground, Source/dsp/Meters.h at 47bac40327ed9e8e59928500de9f3beb63fe8636 (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.
// Meters.h: what a panel's displays, live= marks and shader inputs read each frame: plain data its owner fills on the
// UI thread (GuiPanel fills it from a tee::TelemetrySource). Cut down from what the plugin's audio engine published
// (its probes, voices, host transport and shift lights are gone; only what a display reads is kept).
#pragma once

#include "kit/gui/PanelSpec.h"

#include <cstdint>

namespace tee::ui
{
    struct Meters
    {
        uint32_t frame = 0;                           // bumps when the values are new (a display moves only then)
        float inPeak[2] = {}, inRms[2] = {};          // the input, left and right, linear (we have none: 0)
        float outPeak[2] = {}, outRms[2] = {};        // the output
        float taps[kMaxDisplays] = {};                // the fields the panel's displays show (PanelDisplay::tap)
        float rate = 48000.0f;                        // the sample rate (a spectrum's frequencies)
    };
}
